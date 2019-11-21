/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "android/telephony/sysdeps.h"

#include "android/utils/looper.h"
#include "android/utils/sockets.h"
#include "android/utils/stream.h"

#include <assert.h>
#include <stdio.h>

#define  DEBUG  0

#define  D_ACTIVE  DEBUG

#if DEBUG
#define  D(...)  do { if (D_ACTIVE) fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define  D(...)  ((void)0)
#endif

/** TIME
 **/

SysTime
sys_time_ms( void )
{
    return looper_nowWithClock(looper_getForThread(), LOOPER_CLOCK_REALTIME);
}

/** TIMERS
 **/

typedef struct SysTimerRec_ {
    LoopTimer*     timer;
    SysCallback    callback;
    void*          opaque;
    SysTimer       next;
} SysTimerRec;

#define  MAX_TIMERS  32

static SysTimerRec  _s_timers0[ MAX_TIMERS ];
static SysTimer     _s_free_timers;

static void sys_timer_callback(void* opaque, LoopTimer* unused) {
    SysTimer timer = (SysTimer)opaque;
    assert(timer && timer->callback);
    timer->callback(timer->opaque);
}

static void
sys_init_timers( void )
{
    int  nn;
    for (nn = 0; nn < MAX_TIMERS-1; nn++)
        _s_timers0[nn].next = _s_timers0 + (nn+1);

    _s_free_timers = _s_timers0;
}

static SysTimer
sys_timer_alloc( void )
{
    SysTimer  timer = _s_free_timers;

    if (timer != NULL) {
        _s_free_timers = timer->next;
        timer->next    = NULL;
        timer->timer   = NULL;
    }
    return timer;
}


static void
sys_timer_free( SysTimer  timer )
{
    if (timer->timer) {
        loopTimer_stop( timer->timer );
        loopTimer_free( timer->timer );
        timer->timer = NULL;
    }
    timer->next    = _s_free_timers;
    _s_free_timers = timer;
}


SysTimer   sys_timer_create( void )
{
    SysTimer  timer = sys_timer_alloc();
    return timer;
}

void
sys_timer_set( SysTimer  timer, SysTime  when, SysCallback callback, void*  opaque )
{
    if (callback == NULL) {  /* unsetting the timer */
        if (timer->timer) {
            loopTimer_stop( timer->timer );
            loopTimer_free( timer->timer );
            timer->timer = NULL;
        }
        timer->callback = callback;
        timer->opaque   = NULL;
        return;
    }

    if ( timer->timer ) {
        if ( timer->callback == callback && timer->opaque == opaque )
            goto ReuseTimer;

        /* need to replace the timer */
        loopTimer_stop( timer->timer );
        loopTimer_free( timer->timer );
    }

    timer->timer = loopTimer_newWithClock(
            looper_getForThread(), &sys_timer_callback, 
            timer, LOOPER_CLOCK_REALTIME);
    timer->callback = callback;
    timer->opaque   = opaque;

ReuseTimer:
    loopTimer_startAbsolute( timer->timer, when );
}

void
sys_timer_unset( SysTimer  timer )
{
    if (timer->timer) {
        loopTimer_stop( timer->timer );
    }
}

void
sys_timer_destroy( SysTimer  timer )
{
    sys_timer_free( timer );
}


/** CHANNELS
 **/

typedef struct SysChannelRec_ {
    LoopIo*             io;
    SysChannelCallback  callback;
    void*               opaque;
    SysChannel          next;
} SysChannelRec;

#define  MAX_CHANNELS  16

static SysChannelRec  _s_channels0[ MAX_CHANNELS ];
static SysChannel     _s_free_channels;

static void
sys_init_channels( void )
{
    int  nn;

    for ( nn = 0; nn < MAX_CHANNELS-1; nn++ ) {
        _s_channels0[nn].next = _s_channels0 + (nn+1);
    }
    _s_free_channels = _s_channels0;
}

static SysChannel
sys_channel_alloc( )
{
    SysChannel  channel = _s_free_channels;
    if (channel != NULL) {
        _s_free_channels  = channel->next;
        channel->next     = NULL;
        channel->io       = NULL;
        channel->callback = NULL;
        channel->opaque   = NULL;
    }
    return channel;
}

static void
sys_channel_free( SysChannel  channel )
{
    if (channel->io) {
        socket_close(loopIo_fd(channel->io));
        loopIo_free(channel->io);
        channel->io = NULL;
    }
    channel->next    = _s_free_channels;
    _s_free_channels = channel;
}


static void sys_channel_handler(void* opaque, int fd, unsigned events) {
    SysChannel channel = (SysChannel)opaque;
    assert(channel && channel->callback);

    if ((events & LOOP_IO_READ) != 0) {
        D("%s: read event for channel %p:%d\n", __FUNCTION__,
           channel, loopIo_fd(channel->io));
        channel->callback( channel->opaque, SYS_EVENT_READ );
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        D("%s: write event for channel %p:%d\n", __FUNCTION__,
           channel, loopIo_fd(channel->io));
        channel->callback( channel->opaque, SYS_EVENT_WRITE );
    }
}

void
sys_channel_on( SysChannel          channel,
                int                 events,
                SysChannelCallback  event_callback,
                void*               event_opaque )
{
    channel->callback = event_callback;
    channel->opaque   = event_opaque;
    ((events & SYS_EVENT_READ) ? loopIo_wantRead : loopIo_dontWantRead)(channel->io);
    ((events & SYS_EVENT_WRITE) ? loopIo_wantWrite : loopIo_dontWantWrite)(channel->io);
}

int
sys_channel_read( SysChannel  channel, void*  buffer, int  size )
{
    int   len = size;
    char* buf = (char*) buffer;

    while (len > 0) {
        int  ret = socket_recv(loopIo_fd(channel->io), buf, len);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            D( "%s: after reading %d bytes, recv() returned error %d: %s\n",
                __FUNCTION__, size - len, errno, errno_str);
            return -1;
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
        }
    }
    return size - len;
}


int
sys_channel_write( SysChannel  channel, const void*  buffer, int  size )
{
    int         len = size;
    const char* buf = (const char*) buffer;

    while (len > 0) {
        int  ret = socket_send(loopIo_fd(channel->io), buf, len);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            D( "%s: send() returned error %d: %s\n",
                __FUNCTION__, errno, errno_str);
            return -1;
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
        }
    }
    return size - len;
}

void  sys_channel_close(SysChannel channel)
{
    sys_channel_free(channel);
}

void  sys_main_init( void )
{
    sys_init_channels();
    sys_init_timers();
}


int   sys_main_loop( void )
{
    /* no looping, qemu has its own event loop */
    return 0;
}




SysChannel
sys_channel_create_tcp_server( int port )
{
    SysChannel  channel = sys_channel_alloc();

    const int fd = socket_anyaddr_server( port, SOCKET_STREAM );
    if (fd < 0) {
        D( "%s: failed to created network socket on TCP:%d\n",
            __FUNCTION__, port );
        sys_channel_free( channel );
        return NULL;
    }

    channel->io = loopIo_new(
            looper_getForThread(), fd, &sys_channel_handler, channel);

    D( "%s: server channel %p:%d now listening on port %d\n",
       __FUNCTION__, channel, fd, port );

    return channel;
}


SysChannel
sys_channel_create_tcp_handler( SysChannel  server_channel )
{
    SysChannel  channel = sys_channel_alloc();

    D( "%s: creating handler from server channel %p:%d\n", __FUNCTION__,
       server_channel, loopIo_fd(server_channel->io) );

    const int fd = socket_accept_any(loopIo_fd(server_channel->io));
    if (fd < 0) {
        perror( "accept" );
        sys_channel_free( channel );
        return NULL;
    }

    /* disable Nagle algorithm */
    socket_set_nodelay(fd);

    channel->io = loopIo_new(
            looper_getForThread(), fd, &sys_channel_handler, channel);

    D( "%s: handler %p:%d created from server %p:%d\n", __FUNCTION__,
        server_channel, loopIo_fd(server_channel->io), channel, fd );

    return channel;
}


SysChannel
sys_channel_create_tcp_client( const char*  hostname, int  port )
{
    SysChannel  channel = sys_channel_alloc();

    const int fd = socket_network_client( hostname, port, SOCKET_STREAM );
    if (fd < 0) {
        sys_channel_free(channel);
        return NULL;
    };

    /* set to non-blocking and disable Nagle algorithm */
    socket_set_nonblock(fd);
    socket_set_nodelay(fd);

    channel->io = loopIo_new(
            looper_getForThread(), fd, &sys_channel_handler, channel);

    return channel;
}

void
sys_file_put_byte(SysFile* file, int c) {
    stream_put_byte((Stream*)file, c);
}

void
sys_file_put_be32(SysFile* file, uint32_t v) {
    stream_put_be32((Stream*)file, v);
}

void
sys_file_put_buffer(SysFile* file, const void* buff, int len) {
    stream_write((Stream*)file, buff, len);
}

uint8_t
sys_file_get_byte(SysFile* file) {
    return stream_get_byte((Stream*)file);
}

uint32_t
sys_file_get_be32(SysFile* file) {
    return stream_get_be32((Stream*)file);
}

void
sys_file_get_buffer(SysFile* file, void* buff, int len) {
    stream_read((Stream*)file, buff, len);
}
