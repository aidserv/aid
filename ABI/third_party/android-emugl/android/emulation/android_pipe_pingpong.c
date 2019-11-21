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

#include "android/emulation/android_pipe_pingpong.h"

#include "android/emulation/android_pipe_host.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define D(...)  (void)0
#endif


/***********************************************************************
 ***********************************************************************
 *****
 *****    P I N G   P O N G   P I P E S
 *****
 *****/

/* Debug service that sends back anything it receives */
/* All data is kept in a circular dynamic buffer */

/* Initial buffer size */
#define PINGPONG_SIZE  1024

typedef struct {
    void*     hwpipe;
    uint8_t*  buffer;
    size_t    size;
    size_t    pos;
    size_t    count;
    unsigned  flags;
} PingPongPipe;

static void
pingPongPipe_init0( PingPongPipe* pipe, void* hwpipe, void* svcOpaque )
{
    pipe->hwpipe = hwpipe;
    pipe->size = PINGPONG_SIZE;
    pipe->buffer = malloc(pipe->size);
    pipe->pos = 0;
    pipe->count = 0;
}

static void*
pingPongPipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    PingPongPipe*  ppipe;

    ANEW0(ppipe);
    D("%s: pipe=%p hwpipe=%p", __FUNCTION__, ppipe, hwpipe);
    pingPongPipe_init0(ppipe, hwpipe, svcOpaque);
    return ppipe;
}

static void
pingPongPipe_close( void* opaque )
{
    PingPongPipe*  ppipe = opaque;

    D("%s: pipe=%p hwpipe=%p (pos=%d count=%d size=%d)", __FUNCTION__,
      ppipe, ppipe->hwpipe, (int)ppipe->pos, (int)ppipe->count, (int)ppipe->size);
    free(ppipe->buffer);
    AFREE(ppipe);
}

static int
pingPongPipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    PingPongPipe*  pipe = opaque;
    int  ret = 0;
    int  count;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    count = 0;
    for ( ; buff < buffEnd; buff++ )
        count += buff->size;

    D("%s: pipe=%p count=%d bytes=%d", __FUNCTION__, pipe, numBuffers, count);
    /* Do we need to grow the pingpong buffer? */
    while (count > pipe->size - pipe->count) {
        size_t    newsize = pipe->size*2;
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
        D("pingpong buffer is now %d bytes", newsize);
    }

    for ( buff = buffers; buff < buffEnd; buff++ ) {
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

    return ret;
}

static int
pingPongPipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    PingPongPipe*  pipe = opaque;
    int  ret = 0;

#if DEBUG
    {
        AndroidPipeBuffer* buff = buffers;
        AndroidPipeBuffer* buffEnd = buff + numBuffers;
        int count = 0;
        while (buff < buffEnd) {
            count += buff->size;
            buff++;
        }
        D("%s: pipe=%p count=%d bytes=%d", __FUNCTION__, pipe, numBuffers, count);
    }
#endif

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

    return ret;
}

static unsigned
pingPongPipe_poll( void* opaque )
{
    PingPongPipe*  pipe = opaque;
    unsigned       ret = 0;

    // You can always write to the pipe, since the buffer will
    // be dynamically resized.
    ret |= PIPE_POLL_OUT;

    if (pipe->count > 0)
        ret |= PIPE_POLL_IN;

    return ret;
}

static void
pingPongPipe_wakeOn( void* opaque, int flags )
{
    PingPongPipe* pipe = opaque;
    pipe->flags |= (unsigned)flags;
}

static const AndroidPipeFuncs  pingPongPipe_funcs = {
    pingPongPipe_init,
    pingPongPipe_close,
    pingPongPipe_sendBuffers,
    pingPongPipe_recvBuffers,
    pingPongPipe_poll,
    pingPongPipe_wakeOn,
};

void android_pipe_add_type_pingpong(void) {
    android_pipe_add_type("pingpong", NULL, &pingPongPipe_funcs);
}
