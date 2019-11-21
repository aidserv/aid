/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file implements the 'unix:' goldfish pipe type which allows
 * guest clients to directly connect to a Unix domain port through
 * /dev/qemu_pipe.
 *
 * The list of allowed
 */


#include "android/emulation/android_pipe_unix.h"

// TECHNICAL
#ifndef _WIN32

#include "android/async-utils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/android_pipe_host.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/looper.h"
#include "android/utils/sockets.h"
#include "android/utils/system.h"

#include <string>
#include <vector>

// Sockets includes
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>

/* Implement the net pipes */

/* Set to 1 or 2 for debug traces */
#define DEBUG 0

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n")
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n")
#else
#define DD(...) ((void)0)
#endif

struct GlobalState {
    std::vector<std::string> allowedPaths;
};

static android::base::LazyInstance<GlobalState> sGlobal = LAZY_INSTANCE_INIT;

static bool android_unix_pipe_check_path(const char* path) {
    if (path) {
        D("%s: Checking for [%s]", __FUNCTION__, path);
        GlobalState* global = sGlobal.ptr();
        for (const auto& it : global->allowedPaths) {
            if (it == path) {
                D("   OK\n");
                return true;
            }
        }
        D("  UNKNOWN\N");
    }
    return false;
}

enum State {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_CLOSING_GUEST,
    STATE_CLOSING_SOCKET
};

struct SocketPipe {
    void* hwpipe = nullptr;
    State state = STATE_INIT;
    int wakeWanted = 0;
    LoopIo* io = nullptr;
    AsyncConnector connector[1] = {};
};

static void socketPipe_free(SocketPipe* pipe) {
    int fd;

    /* Close the socket */
    if (pipe->io) {
        fd = loopIo_fd(pipe->io);
        loopIo_free(pipe->io);
        socket_close(fd);
    }

    /* Release the pipe object */
    delete pipe;
}

static void socketPipe_resetState(SocketPipe* pipe) {
    if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
        loopIo_wantWrite(pipe->io);
    } else {
        loopIo_dontWantWrite(pipe->io);
    }

    if (pipe->state == STATE_CONNECTED &&
        (pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
        loopIo_wantRead(pipe->io);
    } else {
        loopIo_dontWantRead(pipe->io);
    }
}

/* This function is only called when the socket is disconnected.
 * See socketPipe_closeFromGuest() for the case when the guest requires
 * the disconnection. */
static void socketPipe_closeFromSocket(void* opaque) {
    auto pipe = static_cast<SocketPipe*>(opaque);

    D("%s", __FUNCTION__);

    /* If the guest already ordered the pipe to be closed, delete immediately */
    if (pipe->state == STATE_CLOSING_GUEST) {
        socketPipe_free(pipe);
        return;
    }

    /* Force the closure of the QEMUD channel - if a guest is blocked
     * waiting for a wake signal, it will receive an error. */
    if (pipe->hwpipe != NULL) {
        android_pipe_host_close(pipe->hwpipe);
        pipe->hwpipe = NULL;
    }

    pipe->state = STATE_CLOSING_SOCKET;
    socketPipe_resetState(pipe);
}

/* This is the function that gets called each time there is an asynchronous
 * event on the network pipe.
 */
static void socketPipe_io_func(void* opaque, int fd, unsigned events) {
    auto pipe = static_cast<SocketPipe*>(opaque);
    int wakeFlags = 0;

    /* Run the connector if we are in the CONNECTING state     */
    /* TODO: Add some sort of time-out, to deal with the case */
    /*        when the server is wedged.                      */
    if (pipe->state == STATE_CONNECTING) {
        AsyncStatus status = asyncConnector_run(pipe->connector);
        if (status == ASYNC_NEED_MORE) {
            return;
        } else if (status == ASYNC_ERROR) {
            /* Could not connect, tell our client by closing the channel. */

            socketPipe_closeFromSocket(pipe);
            return;
        }
        pipe->state = STATE_CONNECTED;
        socketPipe_resetState(pipe);
        return;
    }

    /* Otherwise, accept incoming data */
    if ((events & LOOP_IO_READ) != 0) {
        if ((pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
        }
    }

    if ((events & LOOP_IO_WRITE) != 0) {
        if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
        }
    }

    /* Send wake signal to the guest if needed */
    if (wakeFlags != 0) {
        android_pipe_host_signal_wake(pipe->hwpipe, wakeFlags);
        pipe->wakeWanted &= ~wakeFlags;
    }

    /* Reset state */
    socketPipe_resetState(pipe);
}

static void* socketPipe_initFromAddress(void* hwpipe,
                                        const SockAddress* address,
                                        Looper* looper) {
    SocketPipe* pipe = new SocketPipe();
    *pipe = {};
    pipe->hwpipe = hwpipe;
    pipe->state = STATE_INIT;

    {
        AsyncStatus status;

        int fd = socket_create(sock_address_get_family(address), SOCKET_STREAM);
        if (fd < 0) {
            D("%s: Could create socket from address family!", __FUNCTION__);
            socketPipe_free(pipe);
            return NULL;
        }

        pipe->io = loopIo_new(looper, fd, socketPipe_io_func, pipe);
        status = asyncConnector_init(pipe->connector, address, pipe->io);
        pipe->state = STATE_CONNECTING;

        if (status == ASYNC_ERROR) {
            D("%s: Could not connect to socket: %s", __FUNCTION__, errno_str);
            socketPipe_free(pipe);
            return NULL;
        }
        if (status == ASYNC_COMPLETE) {
            pipe->state = STATE_CONNECTED;
            socketPipe_resetState(pipe);
        }
    }

    return pipe;
}

/* Called when the guest wants to close the channel. This is different
 * from socketPipe_closeFromSocket() which is called when the socket is
 * disconnected. */
static void socketPipe_closeFromGuest(void* opaque) {
    auto pipe = static_cast<SocketPipe*>(opaque);
    socketPipe_free(pipe);
}

static int netPipeReadySend(SocketPipe* pipe) {
    if (pipe->state == STATE_CONNECTED)
        return 0;
    else if (pipe->state == STATE_CONNECTING)
        return PIPE_ERROR_AGAIN;
    else if (pipe->hwpipe == NULL)
        return PIPE_ERROR_INVAL;
    else
        return PIPE_ERROR_IO;
}

#ifdef _WIN32
int qemu_windows_send(int fd, const void* _buf, int len1) {
    int ret, len;
    auto buf = (const char*)_buf;

    len = len1;
    while (len > 0) {
        ret = send(fd, buf, len, 0);
        if (ret < 0) {
            errno = WSAGetLastError();
            if (errno != WSAEWOULDBLOCK) {
                return -1;
            }
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
            break;
        }
    }
    return len1 - len;
}

int qemu_windows_recv(int fd, void* _buf, int len1, bool single_read) {
    int ret, len;
    char* buf = (char*)_buf;

    len = len1;
    while (len > 0) {
        ret = recv(fd, buf, len, 0);
        if (ret < 0) {
            errno = WSAGetLastError();
            if (errno != WSAEWOULDBLOCK) {
                return -1;
            }
            continue;
        } else {
            if (single_read) {
                return ret;
            }
            buf += ret;
            len -= ret;
            break;
        }
    }
    return len1 - len;
}
#endif  // _WIN32

static int socketPipe_sendBuffers(void* opaque,
                               const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    auto pipe = static_cast<SocketPipe*>(opaque);
    int count = 0;
    int ret = 0;
    size_t buffStart = 0;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    ret = netPipeReadySend(pipe);
    if (ret != 0)
        return ret;

    for (; buff < buffEnd; buff++)
        count += buff->size;

    buff = buffers;
    while (count > 0) {
        int  avail = buff->size - buffStart;
        // NOTE:
        // Please take care to send data in a way that does not cause
        // pipe corruption.
        //
        // For instance:
        // One may want to call send() multiple times here, but the important
        // thing to notice is, what happens when multiple send()'s are issued,
        // especially when the last send() results in an error (e.g., EAGAIN)?
        //
        // If the last send() results in EAGAIN, we need to ensure that no
        // data was sent. If we end up sending data successfully through
        // send and then get EAGAIN on the last send() call, the pipe driver
        // will believe there has really not been any data sent,
        // which in turn will cause retransmission of actually-sent data,
        // corrupting the pipe.
        //
        // Otherwise, we need to retry when getting EAGAIN,
        // until the transfer completely goes through.
        // Only the following two situations are correct:
        //
        // a) transfer completely or partially succeeds,
        // returning # bytes written
        // b) nothing is transferred + error code
        //
        // We currently employ two solutions depending on platform:
        // 1. Spin on EAGAIN, with multiple send() calls.
        // 2. Only allow one possible send() success, which makes it easier
        // to deal with EAGAIN.
        //
        // We cannot use #1 on POSIX hosts at least, because if we keep
        // retrying on EAGAIN, it's possible to lock up the emulator.
        // This is because currently, there is a global lock
        // for all render threads.
        // If the wrong thread is waiting for the lock at the same time
        // we are retrying on the EAGAIN signal, no progress will be made.
        // So we just use #2: HANDLE_EINTR(send(...)), allowing only
        // one successful send() and returning EAGAIN without the possibility
        // of previous successful sends.
        //
        // Curiously, on Windows hosts, solution #2 will cause the emulator
        // to not be able to boot up, with repeated surfaceflinger failures.
        // On Windows hosts, solution #1 (spin on EAGAIN) must be used.
#ifndef _WIN32
        // Solution #2: Allow only one successful send()
        int  len = HANDLE_EINTR(send(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                     avail,
                                     0));
#else
        // Solution #1: Spin on EAGAIN (WSAEWOULDBLOCK)
        int  len = qemu_windows_send(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                 avail);
#endif

        /* the write succeeded */
        if (len > 0) {
            buffStart += len;
            if (buffStart >= buff->size) {
                buff++;
                buffStart = 0;
            }
            count -= len;
            ret += len;
            continue;
        }

        /* we reached the end of stream? */
        if (len == 0) {
            if (ret == 0)
                ret = PIPE_ERROR_IO;
            break;
        }

        /* if we already wrote some stuff, simply return */
        if (ret > 0) {
            break;
        }

        /* need to return an appropriate error code */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = PIPE_ERROR_AGAIN;
        } else {
            ret = PIPE_ERROR_IO;
        }
        break;
    }

    return ret;
}

static int socketPipe_recvBuffers(void* opaque,
                               AndroidPipeBuffer* buffers,
                               int numBuffers) {
    auto pipe = static_cast<SocketPipe*>(opaque);
    int count = 0;
    int ret = 0;
    size_t buffStart = 0;
    AndroidPipeBuffer* buff = buffers;
    AndroidPipeBuffer* buffEnd = buff + numBuffers;

    for (; buff < buffEnd; buff++)
        count += buff->size;

    buff = buffers;
    while (count > 0) {
        int  avail = buff->size - buffStart;
        // See comment in socketPipe_sendBuffers.
        // Although we have not observed it yet,
        // pipe corruption can potentially happen here too.
        // We use the same solutions to be safe.
#ifndef _WIN32
        int  len = HANDLE_EINTR(recv(loopIo_fd(pipe->io),
                                buff->data + buffStart,
                                avail,
                                0));
#else
        int  len = qemu_windows_recv(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                     avail,
                                     true);
#endif

        /* the read succeeded */
        if (len > 0) {
            buffStart += len;
            if (buffStart >= buff->size) {
                buff++;
                buffStart = 0;
            }
            count -= len;
            ret += len;
            continue;
        }

        /* we reached the end of stream? */
        if (len == 0) {
            if (ret == 0)
                ret = PIPE_ERROR_IO;
            break;
        }

        /* if we already read some stuff, simply return */
        if (ret > 0) {
            break;
        }

        /* need to return an appropriate error code */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = PIPE_ERROR_AGAIN;
        } else {
            ret = PIPE_ERROR_IO;
        }
        break;
    }
    return ret;
}

static unsigned socketPipe_poll(void* opaque) {
    auto pipe = static_cast<SocketPipe*>(opaque);
    unsigned mask = loopIo_poll(pipe->io);
    unsigned ret = 0;

    if (mask & LOOP_IO_READ)
        ret |= PIPE_POLL_IN;
    if (mask & LOOP_IO_WRITE)
        ret |= PIPE_POLL_OUT;

    return ret;
}

static void socketPipe_wakeOn(void* opaque, int flags) {
    auto pipe = static_cast<SocketPipe*>(opaque);

    DD("%s: flags=%d", __FUNCTION__, flags);

    pipe->wakeWanted |= flags;
    socketPipe_resetState(pipe);
}

void* socketPipe_initUnix(void* hwpipe, void* _looper, const char* args) {
    /* Build SockAddress from arguments. Acceptable formats are:
     *
     *   <path>
     */

    if (args == NULL || args[0] == '\0') {
        D("%s: Missing address!", __FUNCTION__);
        return NULL;
    }
    D("%s: Address is '%s'", __FUNCTION__, args);

    if (!android_unix_pipe_check_path(args)) {
        D("%s: Rejecting connection to unknown path '%s'", __FUNCTION__, args);
        return NULL;
    }

    SockAddress address;
    sock_address_init_unix(&address, args);

    void* ret = socketPipe_initFromAddress(hwpipe, &address,
                                           static_cast<Looper*>(_looper));

    sock_address_done(&address);
    return ret;
}

static const AndroidPipeFuncs s_unix_pipe_funcs = {
        socketPipe_initUnix,
        socketPipe_closeFromGuest,
        socketPipe_sendBuffers,
        socketPipe_recvBuffers,
        socketPipe_poll,
        socketPipe_wakeOn,
        NULL, /* we can't save these */
        NULL, /* we can't load these */
};

void android_unix_pipes_init(void) {
    Looper* looper = looper_getForThread();
    android_pipe_add_type("unix", looper, &s_unix_pipe_funcs);
}

void android_unix_pipes_add_allowed_path(const char* path) {
    if (path && path[0]) {
        D("%s: Adding [%s]", __FUNCTION__, path);
        GlobalState* global = sGlobal.ptr();
        global->allowedPaths.emplace_back(std::string(path));
    }
}

#else  // _WIN32

void android_unix_pipes_init(void) {
    // nothing to do here.
}

void android_unix_pipes_add_allowed_path(const char* path) {
    // nothing to do here.
}

#endif  // _WIN32
