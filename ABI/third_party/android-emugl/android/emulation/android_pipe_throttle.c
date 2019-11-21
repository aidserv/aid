/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/emulation/android_pipe_throttle.h"

#include "android/emulation/android_pipe_host.h"
#include "android/utils/looper.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define D(...)  (void)0
#endif

#if DEBUG >= 2
#  define DD(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define DD(...)  (void)0
#endif

#define TICKS_PER_SECOND  1000.

static int64_t clock_now(void) {
    return looper_now(looper_getForThread());
}

/***********************************************************************
 ***********************************************************************
 *****
 *****    T H R O T T L E   P I P E S
 *****
 *****/

/* Similar to PingPongPipe, but will throttle the bandwidth to test
 * blocking I/O.
 */

/* Initial buffer size */
#define PINGPONG_SIZE  1024

typedef struct {
    void*         hwpipe;
    uint8_t*      buffer;
    size_t        size;
    size_t        pos;
    size_t        count;
    unsigned      flags;
    double        sendRate;
    int64_t       sendExpiration;
    double        recvRate;
    int64_t       recvExpiration;
    LoopTimer*    timer;
} ThrottlePipe;

/* forward declaration */
static void throttlePipe_timerFunc(void* opaque, LoopTimer* timer);

static void*
throttlePipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    ThrottlePipe* pipe;

    ANEW0(pipe);
    pipe->hwpipe = hwpipe;
    pipe->size = PINGPONG_SIZE;
    pipe->buffer = malloc(pipe->size);
    pipe->pos = 0;
    pipe->count = 0;
    pipe->timer = loopTimer_new(looper_getForThread(), throttlePipe_timerFunc, pipe);
    /* For now, limit to 500 KB/s in both directions */
    pipe->sendRate = TICKS_PER_SECOND / (500. * 1024 * 8);
    pipe->recvRate = pipe->sendRate;
    return pipe;
}

static void
throttlePipe_close( void* opaque )
{
    ThrottlePipe* pipe = opaque;

    D("%s: hwpipe=%p (pos=%d count=%d size=%d)", __FUNCTION__,
      pipe->hwpipe, pipe->pos, pipe->count, pipe->size);

    loopTimer_free(pipe->timer);
    free(pipe->buffer);
    AFREE(pipe);
}

static void
throttlePipe_rearm( ThrottlePipe* pipe )
{
    int64_t  minExpiration = 0;

    DD("%s: sendExpiration=%lld recvExpiration=%lld\n", __FUNCTION__, pipe->sendExpiration, pipe->recvExpiration);

    if (pipe->sendExpiration) {
        if (minExpiration == 0 || pipe->sendExpiration < minExpiration)
            minExpiration = pipe->sendExpiration;
    }

    if (pipe->recvExpiration) {
        if (minExpiration == 0 || pipe->recvExpiration < minExpiration)
            minExpiration = pipe->recvExpiration;
    }

    if (minExpiration != 0) {
        DD("%s: Arming for %lld\n", __FUNCTION__, minExpiration);
        loopTimer_startRelative(pipe->timer, minExpiration);
    }
}

static void
throttlePipe_timerFunc(void* opaque, LoopTimer* unused)
{
    ThrottlePipe* pipe = opaque;
    int64_t  now = clock_now();

    DD("%s: TICK! now=%lld sendExpiration=%lld recvExpiration=%lld\n",
       __FUNCTION__, now, pipe->sendExpiration, pipe->recvExpiration);

    /* Timer has expired, signal wake up if needed */
    int      flags = 0;

    if (pipe->sendExpiration && now > pipe->sendExpiration) {
        flags |= PIPE_WAKE_WRITE;
        pipe->sendExpiration = 0;
    }
    if (pipe->recvExpiration && now > pipe->recvExpiration) {
        flags |= PIPE_WAKE_READ;
        pipe->recvExpiration = 0;
    }
    flags &= pipe->flags;
    if (flags != 0) {
        DD("%s: WAKE %d\n", __FUNCTION__, flags);
        android_pipe_host_signal_wake(pipe->hwpipe, flags);
    }

    throttlePipe_rearm(pipe);
}

static int
throttlePipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    ThrottlePipe* pipe = opaque;

    if (pipe->sendExpiration > 0) {
        return PIPE_ERROR_AGAIN;
    }

    int  ret = 0;
    int  count;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    count = 0;
    for (; buff < buffEnd; buff++) {
        count += buff->size;
    }
    /* Do we need to grow the throttle buffer? */
    while (count > pipe->size - pipe->count) {
        size_t    newsize = pipe->size * 2;
        uint8_t*  newbuff = realloc(pipe->buffer, newsize);
        int       wpos    = pipe->pos + pipe->count;
        if (newbuff == NULL) {
            break;
        }
        if (wpos > pipe->size) {
            wpos -= pipe->size;
            memcpy(newbuff + pipe->size, newbuff, wpos);
        }
        pipe->buffer = newbuff;
        pipe->size   = newsize;
        D("throttle buffer is now %d bytes", newsize);
    }

    for (buff = buffers; buff < buffEnd; buff++) {
        int avail = pipe->size - pipe->count;
        if (avail <= 0) {
            if (ret == 0)
                ret = PIPE_ERROR_AGAIN;
            break;
        }
        if (avail > buff->size) {
            avail = buff->size;
        }

        int wpos = pipe->pos + pipe->count;
        if (wpos >= pipe->size) {
            wpos -= pipe->size;
        }
        if (wpos + avail <= pipe->size) {
            memcpy(pipe->buffer + wpos, buff->data, avail);
        } else {
            int  avail2 = pipe->size - wpos;
            memcpy(pipe->buffer + wpos, buff->data, avail2);
            memcpy(pipe->buffer, buff->data + avail2, avail - avail2);
        }
        pipe->count += avail;
        ret += avail;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count > 0 && (pipe->flags & PIPE_WAKE_READ)) {
        android_pipe_host_signal_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }

    if (ret > 0) {
        /* Compute next send expiration time */
        pipe->sendExpiration = clock_now() + ret * pipe->sendRate;
        throttlePipe_rearm(pipe);
    }
    return ret;
}

static int
throttlePipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    ThrottlePipe* pipe = opaque;
    int ret = 0;

    if (pipe->recvExpiration > 0) {
        return PIPE_ERROR_AGAIN;
    }

    while (numBuffers > 0) {
        int avail = pipe->count;
        if (avail <= 0) {
            if (ret == 0)
                ret = PIPE_ERROR_AGAIN;
            break;
        }
        if (avail > buffers[0].size) {
            avail = buffers[0].size;
        }

        int rpos = pipe->pos;

        if (rpos + avail <= pipe->size) {
            memcpy(buffers[0].data, pipe->buffer + rpos, avail);
        } else {
            int  avail2 = pipe->size - rpos;
            memcpy(buffers[0].data, pipe->buffer + rpos, avail2);
            memcpy(buffers[0].data + avail2, pipe->buffer, avail - avail2);
        }
        pipe->count -= avail;
        pipe->pos   += avail;
        if (pipe->pos >= pipe->size) {
            pipe->pos -= pipe->size;
        }
        ret += avail;
        numBuffers--;
        buffers++;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count < PINGPONG_SIZE && (pipe->flags & PIPE_WAKE_WRITE)) {
        android_pipe_host_signal_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
    }

    if (ret > 0) {
        pipe->recvExpiration = clock_now() + ret * pipe->recvRate;
        throttlePipe_rearm(pipe);
    }
    return ret;
}

static unsigned
throttlePipe_poll( void* opaque )
{
    ThrottlePipe*  pipe = opaque;
    unsigned       ret = 0;

    if (pipe->count < pipe->size)
        ret |= PIPE_POLL_OUT;

    if (pipe->count > 0)
        ret |= PIPE_POLL_IN;

    if (pipe->sendExpiration > 0)
        ret &= ~PIPE_POLL_OUT;

    if (pipe->recvExpiration > 0)
        ret &= ~PIPE_POLL_IN;

    return ret;
}

static void
throttlePipe_wakeOn( void* opaque, int flags )
{
    ThrottlePipe* pipe = opaque;
    pipe->flags |= (unsigned)flags;
}

static const AndroidPipeFuncs  throttlePipe_funcs = {
    throttlePipe_init,
    throttlePipe_close,
    throttlePipe_sendBuffers,
    throttlePipe_recvBuffers,
    throttlePipe_poll,
    throttlePipe_wakeOn,
};

void android_pipe_add_type_throttle(void) {
    android_pipe_add_type("throttle", NULL, &throttlePipe_funcs);
}
