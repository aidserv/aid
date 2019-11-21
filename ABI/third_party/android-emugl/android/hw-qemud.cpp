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

#include "android/hw-qemud.h"

#include "android/emulation/android_qemud.h"
#include "android/utils/debug.h"

#include <stddef.h>
#include <stdlib.h>

#define  D(...)    VERBOSE_PRINT(qemud,__VA_ARGS__)

/*
 *  This implements support for the 'qemud' multiplexing communication
 *  channel between clients running in the emulated system and 'services'
 *  provided by the emulator.
 *
 *  For additional details, please read docs/ANDROID-QEMUD.TXT
 *
 */

/*
 * IMPLEMENTATION DETAILS:
 *
 * We use one charpipe to connect the emulated serial port to the 'QemudSerial'
 * object. This object is used to receive data from the serial port, and
 * unframe messages (i.e. extract payload length + channel id from header,
 * then the payload itself), before sending them to a generic receiver.
 *
 * The QemudSerial object can also be used to send messages to the daemon
 * through the serial port (see qemud_serial_send())
 *
 * The multiplexer is connected to one or more 'service' objects.
 * are themselves connected through a charpipe to an emulated device or
 * control sub-module in the emulator.
 *
 *  tty <==charpipe==> QemudSerial ---> QemudMultiplexer ----> QemudClient
 *                          ^                                      |
 *                          |                                      |
 *                          +--------------------------------------+
 *
 */

/* this is the end of the serial charpipe that must be passed
 * to the emulated tty implementation. The other end of the
 * charpipe must be passed to qemud_multiplexer_init().
 */
static CSerialLine* android_qemud_serial_line = NULL;

CSerialLine* android_qemud_get_serial_line(void) {
    if (!android_qemud_serial_line) {
        CSerialLine* slIn;
        CSerialLine* slOut;
        if (android_serialline_pipe_open(&slIn, &slOut) < 0) {
            derror( "%s: can't create charpipe to serial port",
                    __FUNCTION__ );
            exit(1);
        }

        android_qemud_init(slOut);
        android_qemud_serial_line = slIn;
    }

    return android_qemud_serial_line;
}


/*
 * The following code is used for backwards compatibility reasons.
 * It allows you to implement a given qemud-based service through
 * a charpipe.
 *
 * In other words, this implements a QemudService and corresponding
 * QemudClient that connects a qemud client running in the emulated
 * system, to a CharDriverState object implemented through a charpipe.
 *
 *   QemudCharClient <===charpipe====> (char driver user)
 *
 * For example, this is used to implement the "gsm" service when the
 * modem emulation is provided through an external serial device.
 *
 * A QemudCharService can have only one client by definition.
 * There is no QemudCharClient object because we can store a single
 * CharDriverState handle in the 'opaque' field for simplicity.
 */

// A simple struct to pass some data to qemud client callbacks
struct qemud_char_client {
    CSerialLine* sl;
    QemudClient* client;
};

/* called whenever a new message arrives from a qemud client.
 * this simply sends the message through the charpipe to the user.
 */
static void _qemud_char_client_recv(void* opaque, uint8_t* msg, int msglen,
                                    QemudClient* client) {
    qemud_char_client* const data = static_cast<qemud_char_client*>(opaque);
    CSerialLine* sl = data->sl;
    android_serialline_write(sl, msg, msglen);
}

/* we don't expect clients of char. services to exit. Just
 * print an error to signal an unexpected situation. We should
 * be able to recover from these though, so don't panic.
 */
static void _qemud_char_client_close(void* opaque) {
    qemud_char_client* const data = static_cast<qemud_char_client*>(opaque);
    QemudClient* client = data->client;

    /* At this point modem driver still uses char pipe to communicate with
     * hw-qemud, while communication with the guest is done over qemu pipe.
     * So, when guest disconnects from the qemu pipe, and emulator-side client
     * goes through the disconnection process, this routine is called, since it
     * has been set to called during service registration. Unless modem driver
     * is changed to drop char pipe communication, this routine will be called
     * due to guest disconnection. As long as the client was a qemu pipe - based
     * client, it's fine, since we don't really need to do anything in this case.
     */
    if (!qemud_is_pipe_client(client)) {
        derror("unexpected qemud char. channel close");
    }
}

/* called by the charpipe to know how much data can be read from
 * the user. Since we send everything directly to the serial port
 * we can return an arbitrary number.
 */
static int _qemud_char_service_can_read(void* opaque) {
    return 8192;  /* whatever */
}

/* called to read data from the charpipe and send it to the client.
 * used qemud_service_broadcast() even if there is a single client
 * because we don't need a QemudCharClient object this way.
 */
static void _qemud_char_service_read(void* opaque, const uint8_t* from, int len) {
    QemudService* sv = static_cast<QemudService*>(opaque);
    qemud_service_broadcast(sv, from, len);
}

/* called when a qemud client tries to connect to a char. service.
 * we simply create a new client and open the charpipe to receive
 * data from it.
 */
static QemudClient* _qemud_char_service_connect(void* opaque,
                                                QemudService* sv,
                                                int channel,
                                                const char* client_param) {
    CSerialLine* sl = static_cast<CSerialLine*>(opaque);

    qemud_char_client* const data = new qemud_char_client();
    data->sl = sl;
    data->client = qemud_client_new(sv, channel, client_param,
                                    data,
                                    _qemud_char_client_recv,
                                    _qemud_char_client_close,
                                    NULL, NULL);

    /* now we can open the gates :-) */
    android_serialline_addhandlers(sl, sv,
                                   _qemud_char_service_can_read,
                                   _qemud_char_service_read);

    return data->client;
}

static int android_qemud_channel_connect(const char* name, CSerialLine* sl) {
    qemud_service_register(name, 1, sl, _qemud_char_service_connect, NULL, NULL);
    return 0;
}

/* returns a charpipe endpoint that can be used by an emulated
 * device or external serial port to implement a char. service
 */
int android_qemud_get_channel(const char* name, CSerialLine** psl) {
    CSerialLine* sl;

    if (android_serialline_pipe_open(&sl, psl) < 0) {
        derror("can't open charpipe for '%s' qemud service", name);
        return -1;
    }

    return android_qemud_channel_connect(name, sl);
}

/* set the character driver state for a given qemud communication channel. this
 * is used to attach the channel to an external char driver device directly.
 * returns 0 on success, -1 on error
 */
int android_qemud_set_channel(const char* name, CSerialLine* peer_sl) {
    CSerialLine* buffer = android_serialline_buffer_open(peer_sl);
    if (!buffer) {
        return -1;
    }

    return android_qemud_channel_connect(name, buffer);
}
