// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/sockets/SocketUtils.h"

#include "android/base/EintrWrapper.h"
#include "android/base/EnumFlags.h"
#include "android/base/files/Fd.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketErrors.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#  include <sys/socket.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#endif

#include <vector>

#include <stdlib.h>
#include <string.h>

namespace android {
namespace base {

namespace {

#ifdef _WIN32

#  ifndef ESHUTDOWN
#    define ESHUTDOWN     10058
#  endif

#  ifndef ETOOMANYREFS
#    define ETOOMANYREFS  10059
#  endif

#  ifndef EHOSTDOWN
#    define EHOSTDOWN     10064
#  endif

// This macro is used to implement a mapping of Winsock error codes
// to equivalent errno ones. The point here is to ensure that our socket
// helper functions always update errno to an appropriate value, even on
// Windows, where Winsock normally leaves this variable untouched in case
// of error (client code needs to call WSAGetLastError() instead).
#  define  WINSOCK_ERRORS_LIST \
    EE(WSA_INVALID_HANDLE,EINVAL,"invalid handle") \
    EE(WSA_NOT_ENOUGH_MEMORY,ENOMEM,"not enough memory") \
    EE(WSA_INVALID_PARAMETER,EINVAL,"invalid parameter") \
    EE(WSAEINTR,EINTR,"interrupted function call") \
    EE(WSAEALREADY,EALREADY,"operation already in progress") \
    EE(WSAEBADF,EBADF,"bad file descriptor") \
    EE(WSAEACCES,EACCES,"permission denied") \
    EE(WSAEFAULT,EFAULT,"bad address") \
    EE(WSAEINVAL,EINVAL,"invalid argument") \
    EE(WSAEMFILE,EMFILE,"too many opened files") \
    EE(WSAEWOULDBLOCK,EWOULDBLOCK,"resource temporarily unavailable") \
    EE(WSAEINPROGRESS,EINPROGRESS,"operation now in progress") \
    EE(WSAEALREADY,EAGAIN,"operation already in progress") \
    EE(WSAENOTSOCK,EBADF,"socket operation not on socket") \
    EE(WSAEDESTADDRREQ,EDESTADDRREQ,"destination address required") \
    EE(WSAEMSGSIZE,EMSGSIZE,"message too long") \
    EE(WSAEPROTOTYPE,EPROTOTYPE,"wrong protocol type for socket") \
    EE(WSAENOPROTOOPT,ENOPROTOOPT,"bad protocol option") \
    EE(WSAEADDRINUSE,EADDRINUSE,"address already in use") \
    EE(WSAEADDRNOTAVAIL,EADDRNOTAVAIL,"cannot assign requested address") \
    EE(WSAENETDOWN,ENETDOWN,"network is down") \
    EE(WSAENETUNREACH,ENETUNREACH,"network unreachable") \
    EE(WSAENETRESET,ENETRESET,"network dropped connection on reset") \
    EE(WSAECONNABORTED,ECONNABORTED,"software caused connection abort") \
    EE(WSAECONNRESET,ECONNRESET,"connection reset by peer") \
    EE(WSAENOBUFS,ENOBUFS,"no buffer space available") \
    EE(WSAEISCONN,EISCONN,"socket is already connected") \
    EE(WSAENOTCONN,ENOTCONN,"socket is not connected") \
    EE(WSAESHUTDOWN,ESHUTDOWN,"cannot send after socket shutdown") \
    EE(WSAETOOMANYREFS,ETOOMANYREFS,"too many references") \
    EE(WSAETIMEDOUT,ETIMEDOUT,"connection timed out") \
    EE(WSAECONNREFUSED,ECONNREFUSED,"connection refused") \
    EE(WSAELOOP,ELOOP,"cannot translate name") \
    EE(WSAENAMETOOLONG,ENAMETOOLONG,"name too long") \
    EE(WSAEHOSTDOWN,EHOSTDOWN,"host is down") \
    EE(WSAEHOSTUNREACH,EHOSTUNREACH,"no route to host") \

struct WinsockError {
    int winsock_error;
    int unix_error;
};

const WinsockError kWinsockErrors[] = {
#define  EE(w,u,s)   { w, u, },
    WINSOCK_ERRORS_LIST
#undef   EE
};

const size_t kWinsockErrorsSize =
        sizeof(kWinsockErrors) / sizeof(kWinsockErrors[0]);

// Read the latest winsock error code and updates errno appropriately.
// Also always returns -1 so it can be chained in function calls, as in:
//
//     return fixErrno();
//
int fixErrno() {
    int unix_error = EINVAL;  /* generic error code */
    int winsock_error = WSAGetLastError();

    for (size_t n = 0; n < kWinsockErrorsSize; ++n) {
        if (kWinsockErrors[n].winsock_error == winsock_error) {
            unix_error = kWinsockErrors[n].unix_error;
            break;
        }
    }

    errno = unix_error;
    return -1;
}

// Called on process exit to cleanup Winsock data.
void winsockCleanup() {
    WSACleanup();
}

// Initialize Winsock, must be called early, or socket creation will fail.
int winsockInit() {
    WSADATA Data;
    int ret = WSAStartup(MAKEWORD(2,2), &Data);
    if (ret != 0) {
        (void) WSAGetLastError();
        return -1;
    }
    ::atexit(winsockCleanup);
    return 0;
}

void socketInitWinsock() {
    enum {
        UNINITIALIZED = 0,
        INITIALIZING = 1,
        COMPLETED = 2,
    };
    // Ensure thread-safe lazy initialization.
    //
    // TODO(digit): Create android::base::Once instead to make this a
    // little more portable / testable.
    static LONG volatile sWinsockInit = 0;

    LONG status = InterlockedCompareExchange(
            &sWinsockInit, INITIALIZING, UNINITIALIZED);
    if (status == COMPLETED) {
        // Winsock already initialized.
        return;
    }
    if (status == UNINITIALIZED) {
        // First thread to call this function.
        winsockInit();
        InterlockedExchange(&sWinsockInit, COMPLETED);
        return;
    }
    while (status == INITIALIZING) {
        ::Sleep(0);
        status = InterlockedCompareExchange(
                &sWinsockInit, INITIALIZING, INITIALIZING);
    }
}

#endif // !_WIN32

// Use ON_SOCKET_ERROR_RETURN_M1 to return immediately with value -1
// and errno set to an appropriate value, when |ret| corresponds to
// a socket error code.
//
// On Windows, if |ret| is SOCKET_ERROR, this calls fixErrno() to
// set errno to an appropriate value and return -1.
//
// On Posix, if |ret| is negative, this simply returns -1.
#ifdef _WIN32
#  define ON_SOCKET_ERROR_RETURN_M1(ret) \
        do { \
            if (ret == SOCKET_ERROR) { \
                return fixErrno(); \
            } \
        } while (0)
#else  // !_WIN32
#  define ON_SOCKET_ERROR_RETURN_M1(ret) \
        do { \
            if ((ret) < 0) { \
                return -1; \
            } \
        } while (0);
#endif  // !_WIN32

// Generic union to handle the mess of sockaddr variants without
// compiler warnings about aliasing.
union SockAddressStorage {
    struct sockaddr generic;
    struct sockaddr_in inet;
    struct sockaddr_in6 in6;

    void initLoopbackFor(int port, int domain) {
        if (domain == AF_INET) {
            memset(&inet, 0, sizeof(inet));
            inet.sin_family = AF_INET;
            inet.sin_port = htons(port);
            inet.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else if (domain == AF_INET6) {
            memset(&in6, 0, sizeof(in6));
            in6.sin6_family = AF_INET6;
            in6.sin6_port = htons(port);
            in6.sin6_addr = IN6ADDR_LOOPBACK_INIT;
        } else {
            CHECK(false) << "Invalid domain " << domain;
        }
    }

    // Initialize from a generic BSD socket address.
    // |from| points to a sockaddr_in or sockaddr_in6 structure.
    // |fromLen| is its length in bytes.
    // Return true on success, false/errno otherwise.
    bool initFromBsd(const void* from, size_t fromLen) {
        auto src = static_cast<const struct sockaddr*>(from);
        switch(src->sa_family) {
            case AF_INET:
                if (fromLen != sizeof(inet)) {
                    errno = EINVAL;
                    return false;
                }
                inet = *static_cast<const struct sockaddr_in*>(from);
                break;

            case AF_INET6:
                if (fromLen != sizeof(in6)) {
                    errno = EINVAL;
                    return false;
                }
                in6 = *static_cast<const struct sockaddr_in6*>(from);
                break;

            default:
                errno = EINVAL;
                return false;
        }
        return true;
    }

    void setPort(int port) {
        switch (generic.sa_family) {
            case AF_INET: inet.sin_port = htons(port); break;
            case AF_INET6: in6.sin6_port = htons(port); break;
            default: ;
        }
    }

    int getPort() {
        switch (generic.sa_family) {
            case AF_INET: return ntohs(inet.sin_port);
            case AF_INET6: return ntohs(in6.sin6_port);
            default: return -1;
        }
    }

    int family() const {
        return generic.sa_family;
    }

    socklen_t size() const {
        size_t sz;
        switch (generic.sa_family) {
            case AF_INET: sz = sizeof(inet); break;
            case AF_INET6: sz = sizeof(in6); break;
            default: sz = sizeof(generic);
        }
        return static_cast<socklen_t>(sz);
    }

    enum ResolveOption {
        kDefaultResolution = 0,
        kPreferIpv6 = (1 << 0),
    };

    // Resolve host name |hostName| into a list of SockAddressStorage.
    // If |preferIpv6| is true, prefer IPv6 addresses if available.
    // On success, return true and sets |*out| to the result.
    static bool resolveHostNameToList(
            const char* hostName, ResolveOption resolveOption,
            std::vector<SockAddressStorage>* out) {
        addrinfo* res = nullptr;
        bool preferIpv6 = (resolveOption == kPreferIpv6);
        addrinfo hints = {};
        hints.ai_family = preferIpv6 ? AF_INET6 : AF_UNSPEC;
        int ret = ::getaddrinfo(hostName, nullptr, &hints, &res);
        if (ret != 0) {
            // Handle errors.
            int err = 0;
            switch (ret) {
                case EAI_AGAIN:  // server is down
                case EAI_FAIL:   // server is sick
                    err = EHOSTDOWN;
                    break;
/* NOTE that in x86_64-w64-mingw32 both EAI_NODATA and EAI_NONAME are the same */
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
                case EAI_NODATA:
#endif
                case EAI_NONAME:
                    err = ENOENT;
                    break;

                case EAI_MEMORY:
                    err = ENOMEM;
                    break;

                default:
                    err = EINVAL;
            }
            errno = err;
            return -1;
        }

        // Ensure |res| is always deleted on exit.
        std::vector<SockAddressStorage> result;
        for (auto r = res; r != nullptr; r = r->ai_next) {
            SockAddressStorage addr;
            if (!addr.initFromBsd(r->ai_addr, r->ai_addrlen)) {
                continue;
            }
            result.emplace_back(std::move(addr));
        }
        ::freeaddrinfo(res);

        if (result.empty()) {
            return false;
        }
        out->swap(result);
        return true;
    }

    // Initialize a SockAddressStorage instance by resolving |hostname| to
    // its preferred address, according to |resolveOptions|, with port |port|.
    // Return true on success, false/errno on error.
    bool initResolve(const char* hostname, int port,
                     ResolveOption resolveOptions = kDefaultResolution) {
        std::vector<SockAddressStorage> addresses;
        if (!resolveHostNameToList(hostname, resolveOptions, &addresses)) {
            return false;
        }
        const bool preferIpv6 = (resolveOptions & kPreferIpv6);
        const SockAddressStorage* addr6 = nullptr;
        const SockAddressStorage* addr4 = nullptr;
        for (const auto& addr : addresses) {
            if (addr.family() == AF_INET && !addr4) {
                addr4 = &addr;
                if (!preferIpv6) {
                    break;
                }
            } else if (addr.family() == AF_INET6 && !addr6) {
                addr6 = &addr;
                if (preferIpv6) {
                    break;
                }
            }
        }
        const SockAddressStorage* addr;
        if (preferIpv6) {
            addr = addr6 ? addr6 : addr4;
        } else {
            addr = addr4 ? addr4 : addr6;
        }
        if (!addr) {
            return false;
        }
        *this = *addr;
        return true;
    }
};

int socketSetOption(int socket, int  domain, int option, int  _flag) {
#ifdef _WIN32
    DWORD flag = (DWORD) _flag;
#else
    int flag = _flag;
#endif
    errno = 0;
    int ret = ::setsockopt(socket,
                           domain,
                           option,
                           (const char*)&flag,
                           sizeof(flag));
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return 0;
}

int socketSetXReuseAddr(int socket) {
#ifdef _WIN32
   /* on Windows, SO_REUSEADDR is used to indicate that several programs can
    * bind to the same port. this is completely different from the Unix
    * semantics. Use instead SO_EXCLUSIVEADDR to explicitely prevent this.
    */
    return socketSetOption(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
    return socketSetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
}

int socketTcpBindAndListen(int socket, const SockAddressStorage* addr) {
    int kBacklog = 5;

    errno = 0;
    int ret = ::bind(socket, &addr->generic, addr->size());
    ON_SOCKET_ERROR_RETURN_M1(ret);

    errno = 0;
    ret = ::listen(socket, kBacklog);
    ON_SOCKET_ERROR_RETURN_M1(ret);

    return 0;
}

#ifdef _WIN32
int socketAccept(int socket) {
    errno = 0;
    int ret = ::accept(socket, NULL, NULL);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}
#endif  // _WIN32

}  // namespace

void socketClose(int socket) {
    int save_errno = errno;
#ifdef _WIN32
    ::closesocket(socket);
#else
    IGNORE_EINTR(::close(socket));
#endif
    errno = save_errno;
}

ssize_t socketRecv(int socket, void* buffer, size_t bufferLen) {
    errno = 0;
    ssize_t ret = ::recv(socket, reinterpret_cast<char*>(buffer), bufferLen, 0);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}

ssize_t socketSend(int socket, const void* buffer, size_t bufferLen) {
    errno = 0;
#ifdef MSG_NOSIGNAL
    // Prevent SIGPIPE generation on Linux when writing to a broken pipe.
    // ::send() will return -1/EPIPE instead.
    const int sendFlags = MSG_NOSIGNAL;
#else
    // For Darwin, this is handled by setting SO_NOSIGPIPE when creating
    // the socket. On Windows, there is no SIGPIPE signal to consider.
    const int sendFlags = 0;
#endif
    ssize_t ret = ::send(socket,
                         reinterpret_cast<const char*>(buffer),
                         bufferLen, sendFlags);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}

bool socketSendAll(int socket, const void* buffer, size_t bufferLen) {
    auto buf = static_cast<const char*>(buffer);
    while (bufferLen > 0) {
        ssize_t ret = socketSend(socket, buf, bufferLen);
        if (ret <= 0) {
            return false;
        }
        buf += ret;
        bufferLen -= ret;
    }
    return true;
}

bool socketRecvAll(int socket, void* buffer, size_t bufferLen) {
    auto buf = static_cast<char*>(buffer);
    while (bufferLen > 0) {
        ssize_t ret = socketRecv(socket, buf, bufferLen);
        if (ret <= 0) {
            return false;
        }
        buf += ret;
        bufferLen -= ret;
    }
    return true;
}

void socketShutdownWrites(int socket) {
#ifdef _WIN32
    ::shutdown(socket, SD_SEND);
#else
    ::shutdown(socket, SHUT_WR);
#endif
}

void socketShutdownReads(int socket) {
#ifdef _WIN32
    ::shutdown(socket, SD_RECEIVE);
#else
    ::shutdown(socket, SHUT_RD);
#endif
}

void socketSetNonBlocking(int socket) {
#ifdef _WIN32
    unsigned long opt = 1;
    ioctlsocket(socket, FIONBIO, &opt);
#else
    int   flags = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
}

void socketSetBlocking(int socket) {
#ifdef _WIN32
    unsigned long opt = 0;
    ioctlsocket(socket, FIONBIO, &opt);
#else
    int   flags = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

void socketSetNoDelay(int socket) {
    socketSetOption(socket, IPPROTO_TCP, TCP_NODELAY, 1);
}

static int socketCreateTcpFor(int domain) {
#ifdef _WIN32
    socketInitWinsock();
#endif
    errno = 0;
    int s = ::socket(domain, SOCK_STREAM | SOCK_CLOEXEC, 0);
    ON_SOCKET_ERROR_RETURN_M1(s);
#ifdef SO_NOSIGPIPE
    // Disable SIGPIPE generation on Darwin.
    // When writing to a broken pipe, send() will return -1 and
    // set errno to EPIPE.
    socketSetOption(s, SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
    fdSetCloexec(s);
    return s;
}

static int socketTcpLoopbackServerFor(int port, int domain) {
    ScopedSocket s(socketCreateTcpFor(domain));
    if (s.get() < 0) {
        DPLOG(ERROR) << "Could not create TCP socket\n";
        return -1;
    }

    socketSetXReuseAddr(s.get());

    SockAddressStorage addr;
    addr.initLoopbackFor(port, domain);

    if (socketTcpBindAndListen(s.get(), &addr) < 0) {
        DPLOG(ERROR) << "Could not bind to TCP loopback port "
                     << port
                     << "\n";
        return -1;
    }

    return s.release();
}

int socketTcp4LoopbackServer(int port) {
    return socketTcpLoopbackServerFor(port, AF_INET);
}

int socketTcp6LoopbackServer(int port) {
    return socketTcpLoopbackServerFor(port, AF_INET6);
}

static int socketTcpLoopbackClientFor(int port, int domain) {
    ScopedSocket s(socketCreateTcpFor(domain));
    if (s.get() < 0) {
        DPLOG(ERROR) << "Could not create TCP socket\n";
        return -1;
    }

    SockAddressStorage addr;
    addr.initLoopbackFor(port, domain);

    if (::connect(s.get(), &addr.generic, addr.size()) < 0) {
        return -1;
    }

    return s.release();
}

int socketTcp4LoopbackClient(int port) {
    return socketTcpLoopbackClientFor(port, AF_INET);
}

int socketTcp6LoopbackClient(int port) {
    return socketTcpLoopbackClientFor(port, AF_INET6);
}

int socketAcceptAny(int serverSocket) {
    errno = 0;
#ifdef __linux__
    int s = HANDLE_EINTR(::accept4(serverSocket, NULL, NULL, SOCK_CLOEXEC));
#else  // !__linux__
    int s = HANDLE_EINTR(::accept(serverSocket, NULL, NULL));
    fdSetCloexec(s);
#endif  // !__linux__
    ON_SOCKET_ERROR_RETURN_M1(s);
    return s;
}

int socketCreatePair(int* fd1, int* fd2) {
#ifndef _WIN32
    int fds[2];
    int ret = ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);

    if (!ret) {
        socketSetNonBlocking(fds[0]);
        socketSetNonBlocking(fds[1]);
        fdSetCloexec(fds[0]);
        fdSetCloexec(fds[1]);
#ifdef SO_NOSIGPIPE
        socketSetOption(fds[0], SOL_SOCKET, SO_NOSIGPIPE, 1);
        socketSetOption(fds[1], SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
        *fd1 = fds[0];
        *fd2 = fds[1];
    } else {
        DPLOG(ERROR) << "Could not create socket pair\n";
    }
    return ret;
#else /* _WIN32 */
    /* on Windows, select() only works with network sockets, which
     * means we absolutely cannot use Win32 PIPEs to implement
     * socket pairs with the current event loop implementation.
     * We're going to do like Cygwin: create a random pair
     * of localhost TCP sockets and connect them together
     */

    /* first, create the 'server' socket.
     * a port number of 0 means 'any port between 1024 and 5000.
     * see Winsock bind() documentation for details */
    ScopedSocket s0(socketTcp4LoopbackServer(0));
    if (s0.get() < 0) {
        return -1;
    }

    // IMPORTANT: Keep the s0 socket in blocking mode, or the accept()
    // below may fail with WSAEWOULDBLOCK randomly.

    /* now connect a client socket to it, we first need to
     * extract the server socket's port number */
    int port = socketGetPort(s0.get());

    ScopedSocket s2(socketTcp4LoopbackClient(port));
    if (!s2.valid()) {
        return -1;
    }

    /* we need to accept the connection on the server socket
     * this will create the second socket for the pair
     */
    ScopedSocket s1(socketAccept(s0.get()));
    if (!s1.valid()) {
            DPLOG(ERROR) << "Could not accept connection from server socket\n";
        return -1;
    }
    socketSetNonBlocking(s1.get());

    *fd1 = s1.release();
    *fd2 = s2.release();

    return 0;
#endif /* _WIN32 */
}

int socketCreateTcp4() {
    return socketCreateTcpFor(AF_INET);
}

int socketCreateTcp6() {
    return socketCreateTcpFor(AF_INET6);
}

int socketGetPort(int socket) {
    SockAddressStorage addr;
    socklen_t addrLen = static_cast<socklen_t>(sizeof(addr));
    if (getsockname(socket, &addr.generic, &addrLen) < 0) {
        DPLOG(ERROR) << "Could not get socket name!\n";
        return -1;
    }
    return addr.getPort();
}

int socketGetPeerPort(int socket) {
    SockAddressStorage addr;
    socklen_t addrLen = static_cast<socklen_t>(sizeof(addr));
    if (getpeername(socket, &addr.generic, &addrLen) < 0) {
        DPLOG(ERROR) << "Could not get socket peer name!";
        return -1;
    }
    return addr.getPort();
}

}  // namespace base
}  // namespace android
