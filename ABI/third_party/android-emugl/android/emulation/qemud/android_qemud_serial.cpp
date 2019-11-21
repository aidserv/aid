// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/qemud/android_qemud_serial.h"

#include "android/emulation/qemud/android_qemud_common.h"

#include <errno.h>

#define  HEADER_SIZE    6

#define  LENGTH_OFFSET  2
#define  LENGTH_SIZE    4

#define  CHANNEL_OFFSET 0
#define  CHANNEL_SIZE   2

void qemud_serial_save(Stream* f, QemudSerial* s) {
    /* sl, recv_func and recv_opaque are not saved, as these are assigned only
     * during emulator init. A load within a session can re-use the values
     * already assigned, a newly launched emulator has freshly assigned values.
     */

    /* state of incoming packets from the serial port */
    stream_put_be32(f, s->need_header);
    stream_put_be32(f, s->overflow);
    stream_put_be32(f, s->in_size);
    stream_put_be32(f, s->in_channel);
#if SUPPORT_LEGACY_QEMUD
    stream_put_be32(f, s->version);
#endif
    qemud_sink_save(f, s->header);
    qemud_sink_save(f, s->payload);
    stream_put_be32(f, MAX_SERIAL_PAYLOAD + 1);
    stream_write(f, s->data0, MAX_SERIAL_PAYLOAD + 1);
}

int qemud_serial_load(Stream* f, QemudSerial* s) {
    /* state of incoming packets from the serial port */
    s->need_header = stream_get_be32(f);
    s->overflow = stream_get_be32(f);
    s->in_size = stream_get_be32(f);
    s->in_channel = stream_get_be32(f);
#if SUPPORT_LEGACY_QEMUD
    s->version = static_cast<QemudVersion>(stream_get_be32(f));
#endif
    qemud_sink_load(f, s->header);
    qemud_sink_load(f, s->payload);

    /* s->header and s->payload are only ever connected to s->data0 */
    s->header->buff = s->payload->buff = s->data0;

    int len = stream_get_be32(f);
    if (len - 1 > MAX_SERIAL_PAYLOAD) {
        D("%s: load failed: size of saved payload buffer (%d) exceeds "
                  "current maximum (%d)\n",
          __FUNCTION__, len - 1, MAX_SERIAL_PAYLOAD);
        return -EIO;
    }
    int ret;
    if ((ret = stream_read(f, s->data0, len)) != len) {
        D("%s: failed to load serial buffer contents (tried reading %d bytes, got %d)\n",
          __FUNCTION__, len, ret);
        return -EIO;
    }

    return 0;
}

#if SUPPORT_LEGACY_QEMUD
void qemud_serial_send_legacy_probe( QemudSerial*  s )
{
    /* we're going to send a specially crafted packet to the qemud
     * daemon, this will help us determine whether we're talking
     * to a legacy or a normal daemon.
     *
     * the trick is to known that a legacy daemon uses the following
     * header:
     *
     *    <length><channel><payload>
     *
     * while the normal one uses:
     *
     *    <channel><length><payload>
     *
     * where <channel> is a 2-hexchar string, and <length> a 4-hexchar
     * string.
     *
     * if we send a header of "000100", it is interpreted:
     *
     * - as the header of a 1-byte payload by the legacy daemon
     * - as the header of a 256-byte payload by the normal one.
     *
     * we're going to send something that looks like:
     *
     *   "000100" + "X" +
     *   "000b00" + "connect:gsm" +
     *   "000b00" + "connect:gps" +
     *   "000f00" + "connect:control" +
     *   "00c210" + "0"*194
     *
     * the normal daemon will interpret this as a 256-byte payload
     * for channel 0, with garbage content ("X000b00conn...") which
     * will be silently ignored.
     *
     * on the other hand, the legacy daemon will see it as a
     * series of packets:
     *
     *   one message "X" on channel 0, which will force the daemon
     *   to send back "001200ko:unknown command" as its first answer.
     *
     *   three "connect:<xxx>" messages used to receive the channel
     *   numbers of the three legacy services implemented by the daemon.
     *
     *   a garbage packet of 194 zeroes for channel 16, which will be
     *   silently ignored.
     */
    uint8_t  tab[194];

    memset(tab, 0, sizeof(tab));
    android_serialline_write(s->sl, (uint8_t*)"000100X", 7);
    android_serialline_write(s->sl, (uint8_t*)"000b00connect:gsm", 17);
    android_serialline_write(s->sl, (uint8_t*)"000b00connect:gps", 17);
    android_serialline_write(s->sl, (uint8_t*)"000f00connect:control", 21);
    android_serialline_write(s->sl, (uint8_t*)"00c210", 6);
    android_serialline_write(s->sl, tab, sizeof(tab));
}
#endif /* SUPPORT_LEGACY_QEMUD */

/* called by the charpipe to see how much bytes can be
 * read from the serial port.
 */

static int _qemud_serial_can_read(void* opaque) {
    QemudSerial* s = static_cast<QemudSerial*>(opaque);

    if (s->overflow > 0) {
        return s->overflow;
    }

    /* if in_size is 0, we're reading the header */
    if (s->need_header)
        return qemud_sink_needed(s->header);

    /* otherwise, we're reading the payload */
    return qemud_sink_needed(s->payload);
}

/* called by the charpipe to read data from the serial
 * port. 'len' cannot be more than the value returned
 * by 'qemud_serial_can_read'.
 */
static void _qemud_serial_read(void* opaque, const uint8_t* from, int len) {
    QemudSerial* s = static_cast<QemudSerial*>(opaque);

    TRACE("%s: received %3d bytes: '%s'", __FUNCTION__, len, quote_bytes((const void*) from, len));

    while (len > 0) {
        int avail;

        /* skip overflow bytes */
        if (s->overflow > 0) {
            avail = s->overflow;
            if (avail > len)
                avail = len;

            from += avail;
            len -= avail;
            continue;
        }

        /* read header if needed */
        if (s->need_header) {
            if (!qemud_sink_fill(s->header, (const uint8_t**) &from, &len))
                break;

#if SUPPORT_LEGACY_QEMUD
            if (s->version == QEMUD_VERSION_UNKNOWN) {
                /* if we receive "001200" as the first header, then we
                 * detected a legacy qemud daemon. See the comments
                 * in qemud_serial_send_legacy_probe() for details.
                 */
                if ( !memcmp(s->data0, "001200", 6) ) {
                    D("%s: legacy qemud detected.", __FUNCTION__);
                    s->version = QEMUD_VERSION_LEGACY;
                    /* tell the modem to use legacy emulation mode */
                    amodem_set_legacy(android_modem);
                } else {
                    D("%s: normal qemud detected.", __FUNCTION__);
                    s->version = QEMUD_VERSION_NORMAL;
                }
            }

            if (s->version == QEMUD_VERSION_LEGACY) {
                s->in_size     = hex2int( s->data0 + LEGACY_LENGTH_OFFSET,  LENGTH_SIZE );
                s->in_channel  = hex2int( s->data0 + LEGACY_CHANNEL_OFFSET, CHANNEL_SIZE );
            } else {
                s->in_size     = hex2int( s->data0 + LENGTH_OFFSET,  LENGTH_SIZE );
                s->in_channel  = hex2int( s->data0 + CHANNEL_OFFSET, CHANNEL_SIZE );
            }
#else
            /* extract payload length + channel id */
            s->in_size = hex2int(s->data0 + LENGTH_OFFSET, LENGTH_SIZE);
            s->in_channel = hex2int(s->data0 + CHANNEL_OFFSET, CHANNEL_SIZE);
#endif
            s->header->used = 0;

            if (s->in_size <= 0 || s->in_channel < 0) {
                D("%s: bad header: '%.*s'", __FUNCTION__, HEADER_SIZE, s->data0);
                continue;
            }

            if (s->in_size > MAX_SERIAL_PAYLOAD) {
                D("%s: ignoring huge serial packet: length=%d channel=%d",
                  __FUNCTION__, s->in_size, s->in_channel);
                s->overflow = s->in_size;
                continue;
            }

            /* prepare 'in_data' for payload */
            s->need_header = 0;
            qemud_sink_reset(s->payload, s->in_size, s->data0);
        }

        /* read payload bytes */
        if (!qemud_sink_fill(s->payload, &from, &len))
            break;

        /* zero-terminate payload, then send it to receiver */
        s->payload->buff[s->payload->size] = 0;
        D("%s: channel=%2d len=%3d '%s'", __FUNCTION__,
          s->in_channel, s->payload->size,
          quote_bytes((const char*) s->payload->buff, s->payload->size));

        s->recv_func(s->recv_opaque, s->in_channel, s->payload->buff, s->payload->size);

        /* prepare for new header */
        s->need_header = 1;
    }
}

void qemud_serial_init(QemudSerial* s,
                       CSerialLine* sl,
                       QemudSerialReceive recv_func,
                       void* recv_opaque) {
    s->sl = sl;
    s->recv_func = recv_func;
    s->recv_opaque = recv_opaque;
    s->need_header = 1;
    s->overflow = 0;

    qemud_sink_reset(s->header, HEADER_SIZE, s->data0);
    s->in_size = 0;
    s->in_channel = -1;

#if SUPPORT_LEGACY_QEMUD
    s->version = QEMUD_VERSION_UNKNOWN;
    qemud_serial_send_legacy_probe(s);
#endif

    android_serialline_addhandlers(sl, s,
                                   _qemud_serial_can_read,
                                   _qemud_serial_read);
}

void qemud_serial_send(QemudSerial* s,
                       int channel,
                       ABool framing,
                       const uint8_t* msg,
                       int msglen) {
    uint8_t header[HEADER_SIZE];
    uint8_t frame[FRAME_HEADER_SIZE];
    int avail, len = msglen;

    if (msglen <= 0 || channel < 0)
        return;

    D("%s: channel=%2d len=%3d '%s'",
      __FUNCTION__, channel, msglen,
      quote_bytes((const char*) msg, msglen));

    if (framing) {
        len += FRAME_HEADER_SIZE;
    }

    /* packetize the payload for the serial MTU */
    while (len > 0) {
        avail = len;
        if (avail > MAX_SERIAL_PAYLOAD)
            avail = MAX_SERIAL_PAYLOAD;

        /* write this packet's header */
#if SUPPORT_LEGACY_QEMUD
        if (s->version == QEMUD_VERSION_LEGACY) {
            int2hex(header + LEGACY_LENGTH_OFFSET,  LENGTH_SIZE,  avail);
            int2hex(header + LEGACY_CHANNEL_OFFSET, CHANNEL_SIZE, channel);
        } else {
            int2hex(header + LENGTH_OFFSET,  LENGTH_SIZE,  avail);
            int2hex(header + CHANNEL_OFFSET, CHANNEL_SIZE, channel);
        }
#else
        int2hex(header + LENGTH_OFFSET, LENGTH_SIZE, avail);
        int2hex(header + CHANNEL_OFFSET, CHANNEL_SIZE, channel);
#endif
        TRACE("%s: '%.*s'", __FUNCTION__, HEADER_SIZE, header);
        android_serialline_write(s->sl, header, HEADER_SIZE);

        /* insert frame header when needed */
        if (framing) {
            int2hex(frame, FRAME_HEADER_SIZE, msglen);
            TRACE("%s: '%.*s'", __FUNCTION__, FRAME_HEADER_SIZE, frame);
            android_serialline_write(s->sl, frame, FRAME_HEADER_SIZE);
            avail -= FRAME_HEADER_SIZE;
            len -= FRAME_HEADER_SIZE;
            framing = 0;
        }

        /* write message content */
        TRACE("%s: '%.*s'", __FUNCTION__, avail, msg);
        android_serialline_write(s->sl, msg, avail);
        msg += avail;
        len -= avail;
    }
}
