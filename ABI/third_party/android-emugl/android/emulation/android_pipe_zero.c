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

#include "android/emulation/android_pipe_zero.h"

#include "android/emulation/android_pipe_host.h"
#include "android/utils/system.h"

#include <stdio.h>

#define DEBUG 0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define D(...)  (void)0
#endif

/***********************************************************************
 ***********************************************************************
 *****
 *****    Z E R O   P I P E S
 *****
 *****/

/* A simple pipe service that mimics /dev/zero, you can write anything to
 * it, and you can always read any number of zeros from it. Useful for debugging
 * the kernel driver.
 */

typedef struct {
    void* hwpipe;
} ZeroPipe;

static void*
zeroPipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    ZeroPipe*  zpipe;

    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ANEW0(zpipe);
    zpipe->hwpipe = hwpipe;
    return zpipe;
}

static void
zeroPipe_close( void* opaque )
{
    ZeroPipe*  zpipe = opaque;

    D("%s: hwpipe=%p", __FUNCTION__, zpipe->hwpipe);
    AFREE(zpipe);
}

static int
zeroPipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    int  ret = 0;
    while (numBuffers > 0) {
        ret += buffers[0].size;
        buffers++;
        numBuffers--;
    }
    return ret;
}

static int
zeroPipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    int  ret = 0;
    while (numBuffers > 0) {
        ret += buffers[0].size;
        memset(buffers[0].data, 0, buffers[0].size);
        buffers++;
        numBuffers--;
    }
    return ret;
}

static unsigned
zeroPipe_poll( void* opaque )
{
    return PIPE_POLL_IN | PIPE_POLL_OUT;
}

static void
zeroPipe_wakeOn( void* opaque, int flags )
{
    /* nothing to do here */
}

static const AndroidPipeFuncs  zeroPipe_funcs = {
    zeroPipe_init,
    zeroPipe_close,
    zeroPipe_sendBuffers,
    zeroPipe_recvBuffers,
    zeroPipe_poll,
    zeroPipe_wakeOn,
};

void android_pipe_add_type_zero(void) {
    android_pipe_add_type("zero", NULL, &zeroPipe_funcs);
}
