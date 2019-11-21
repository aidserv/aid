// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emuctl-client.h"
#include "android/base/ArraySize.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/globals.h"
#include "android/hw-sensors.h"
#include "android/skin/rect.h"
#include "android/skin/linux_keycodes.h"
#include "android/multitouch-screen.h"
#include "android/jpeg-compress.h"
#include "android/gpu_frame.h"

#include <cmath>
#include <limits>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

enum PacketType {
    PACKET_TYPE_SENSOR_DATA_ACCELEROMETER = (0x01),
    PACKET_TYPE_SENSOR_DATA_MAGNETOMETER = (0x02),
    PACKET_TYPE_SENSOR_DATA_ORIENTATION = (0x03), /* deprecated */
    PACKET_TYPE_SENSOR_DATA_AMBIENT_TEMPERATURE = (0x04),
    PACKET_TYPE_SENSOR_DATA_PROXIMITY = (0x05),
    PACKET_TYPE_SENSOR_DATA_LIGHT = (0x06),
    PACKET_TYPE_SENSOR_DATA_PRESSURE = (0x07),
    PACKET_TYPE_SENSOR_DATA_HUMIDITY = (0x08),
    PACKET_TYPE_MT_ACTION_DOWN = (0x09),
    PACKET_TYPE_MT_ACTION_MOVE = (0x0A),
    PACKET_TYPE_MT_ACTION_UP = (0x0B),
    PACKET_TYPE_MT_ACTION_CANCEL = (0x0C)
};

namespace {

// A data packet sent by the controller app.
// All values are in network byte order.
struct InboundPacket {
    // One of the PACKET_TYPE_* constants, indicates what data is contained
    // in this packet.
    uint32_t type;

    // For SENSOR_DATA_* data packets, x, y and z are the readings of
    // the sensors.
    // For MT_ACTION_* packets, x and y contain the coordinates of the touch
    // event in screen pixels, while z is the pressure.
    float x;
    float y;
    float z;

    // For MT_ACTION_* packets, tracking_id contains the pointer id.
    uint32_t tracking_id;
};

// A header that is sent to the controller app with every screen update.
struct FramebufferPacketHeader {
    uint32_t buf_size;
    uint32_t screen_orientation;
};

struct Globals {
    ::android::base::Looper* looper;
    const QAndroidSensorsAgent* sensors;
    ::android::base::ScopedSocketWatch socket;
    uint8_t pkt_buf[sizeof(InboundPacket)];
    int pkt_bytes_remaining;
    int pkt_bytes_read;
    SkinRotation last_known_screen_orientation;
    AJPEGDesc* jpeg_compressor;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// Converts a float from network to host byte-order.
inline static float float_ntohl(float a) {
    union {
        float f;
        uint32_t i;
    } v;
    v.f = a;
    v.i = ntohl(v.i);
    return v.f;
}

// Helper function for sending screen updates to the controller app.
// It tries to guess the current screen orientation of the device from
// the accelerometer values.
// This method is not ideal, although it works in the majority of cases.
// It relies on gravity giving the most significant
// contribution to the acceleration vector. Also, some apps might lock the
// screen in a particular orientation so that it's not affected by the
// accelerometer at all, which isn't taken into account by this function.
// TODO: implement a method for retrieving the actual screen orientation from
// the emulated device.
static SkinRotation guessScreenOrientation() {
    // Values for gravity in different orientations
    float gravity[][3] = {
        // Face up/down
        {0.0f, 0.0f, 9.8f},
        {0.0f, 0.0f, -9.8f},

        // Portrait/reverse portrait
        {0.0f, 9.8f, 0.0f},
        {0.0f, -9.8f, 0.0f},

        // Landscape/reverse landscape
        {9.8f, 0.0f, 0.0f},
        {-9.8f, 0.0f, 0.0f}
    };
    SkinRotation orientations[] = {
        SKIN_ROTATION_0,
        SKIN_ROTATION_180,
        SKIN_ROTATION_270,
        SKIN_ROTATION_90
    };

    float min_diff_norm = std::numeric_limits<float>::max();
    int gravity_direction_index = 2;
    float acceleration[3];
    sGlobals->sensors->getSensor(
        ANDROID_SENSOR_ACCELERATION,
        &acceleration[0],
        &acceleration[1],
        &acceleration[2]);

    for (size_t i = 0; i <  ARRAY_SIZE(gravity); i++) {
        float diff_component = 0.0f;
        float diff_norm = 0.0f;
        for (size_t j = 0; j < ARRAY_SIZE(acceleration); j++) {
            diff_component = acceleration[j] - gravity[i][j];
            diff_component *= diff_component;
            diff_norm += diff_component;
        }
        if (min_diff_norm > diff_norm) {
            min_diff_norm = diff_norm;
            gravity_direction_index = i;
        }
    }

    // Lying face up/down doesn't change the screen orientation.
    // In other cases, update the last known orientation
    if (gravity_direction_index > 1) {
        sGlobals->last_known_screen_orientation =
            orientations[gravity_direction_index - 2];
    }

    return sGlobals->last_known_screen_orientation;
}

// This function gets called every time the contents of the window changes.
static void onFramebufferPosted(void*, int w, int h, const void* pixels) {
    // We expect screen contents to in RGB format, one byte per channel.
    static const int bytes_per_pixel = 3;
    static const int jpeg_quality = 50;
    static const int direction = 1;

    if (sGlobals->socket) {
        // JPEG-compress the contents of the window.
        const uint8_t* fb = static_cast<const uint8_t*>(pixels);
        jpeg_compressor_compress_fb(
            sGlobals->jpeg_compressor,
            0, 0, w, h,
            h,
            bytes_per_pixel, w * bytes_per_pixel,
            fb,
            jpeg_quality,
            direction);
        void* compr_buf =
            jpeg_compressor_get_buffer(sGlobals->jpeg_compressor);

        // Fill out header fields.
        FramebufferPacketHeader* hdr =
            static_cast<FramebufferPacketHeader*>(compr_buf);
        hdr->buf_size =
            jpeg_compressor_get_jpeg_size(sGlobals->jpeg_compressor);
        hdr->screen_orientation =
            static_cast<int>(guessScreenOrientation());

        // Send the update to the controller app.
        android::base::socketSend(
            sGlobals->socket->fd(),
            compr_buf,
            sizeof(FramebufferPacketHeader) + hdr->buf_size);
    }

}

// Helper to convert touch coordinates in screen pixels into ones used by
// the system.
static int scaleForTouchpad(float in, int axis_in) {
    // Constant defined in QEMU2 glue code, redefining it here to avoid
    // including QEMU2-specific headers.
    static const int touchpad_size = 0x8000;
    return in * (touchpad_size-1) / (axis_in-1);
}

// Handles an incoming packet.
static void handlePacket(InboundPacket* p) {
    uint32_t type = ntohl(p->type);
    if (type < PACKET_TYPE_MT_ACTION_DOWN && sGlobals->sensors) {
        // Sensor data packet.
        sGlobals->sensors->setSensor(
            ANDROID_SENSOR_ACCELERATION + (type-1),
            float_ntohl(p->x),
            float_ntohl(p->y),
            float_ntohl(p->z));
    } else {
        // Multitouch data packet.
        int x =
            scaleForTouchpad(float_ntohl(p->x), android_hw->hw_lcd_width);
        int y =
            scaleForTouchpad(float_ntohl(p->y), android_hw->hw_lcd_height);
        int pressure = static_cast<int>(100.0f * float_ntohl(p->z));
        switch (type) {
        case PACKET_TYPE_MT_ACTION_DOWN:
        case PACKET_TYPE_MT_ACTION_MOVE:
            multitouch_update_pointer(
                MTES_DEVICE,
                ntohl(p->tracking_id),
                x,
                y,
                pressure,
                true);
            break;

        case PACKET_TYPE_MT_ACTION_UP:
        case PACKET_TYPE_MT_ACTION_CANCEL:
            multitouch_update_pointer(
                MTES_DEVICE,
                ntohl(p->tracking_id),
                0,
                0,
                0,
                true);
            break;
        }
    }
}

// Called when there's some data available to read from the controller app
// connection.
static void onDataAvailable(void* opaque, int sock, unsigned int) {
    ssize_t size = 0;
    bool first_read = true;

    // Read as much data as possible. As soon as we get enough bytes
    // for a packet, process that packet.
    do {
        size =
            ::android::base::socketRecv(
                sock,
                sGlobals->pkt_buf + sGlobals->pkt_bytes_read,
                sGlobals->pkt_bytes_remaining);
        if (first_read && size == 0) {
            // This happens when the remote side closes connection.
            android_emuctl_client_disconnect();
            return;
        }
        first_read = false;
        if (size > 0) {
            sGlobals->pkt_bytes_remaining -= size;
            sGlobals->pkt_bytes_read += size;
            if (sGlobals->pkt_bytes_remaining == 0) {
                InboundPacket* p =
                    reinterpret_cast<InboundPacket*>(&sGlobals->pkt_buf);
                handlePacket(p);
                sGlobals->pkt_bytes_remaining = sizeof(InboundPacket);
                sGlobals->pkt_bytes_read = 0;
            }
        }
    } while (size > 0);
    if (size < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
        android_emuctl_client_disconnect();
    }
}

}  // namespace

void android_emuctl_client_init(void) {
    sGlobals->looper = android::base::ThreadLooper::get();
    sGlobals->sensors = nullptr;
    sGlobals->last_known_screen_orientation = SKIN_ROTATION_0;
    sGlobals->jpeg_compressor =
        jpeg_compressor_create(
            sizeof(FramebufferPacketHeader),
            4096);
}

void android_emuctl_client_connect(int port) {
    android_emuctl_client_disconnect();
    android::base::ScopedSocket sock (
        android::base::socketTcp4LoopbackClient(port));
    android::base::socketSetNonBlocking(sock.get());
    if (sock.get() > 0) {
        sGlobals->pkt_bytes_remaining = sizeof(InboundPacket);
        sGlobals->pkt_bytes_read = 0;
        sGlobals->socket.reset(
            sGlobals->looper->createFdWatch(
                sock.release(),
                onDataAvailable,
                nullptr));
        sGlobals->socket->wantRead();
        gpu_frame_set_post_callback(
            reinterpret_cast<Looper*>(sGlobals->looper),
            nullptr,
            onFramebufferPosted);
    }
}

void android_emuctl_client_disconnect(void) {
    gpu_frame_set_post_callback(
            reinterpret_cast<Looper*>(sGlobals->looper),
            nullptr,
            nullptr);
    sGlobals->socket.reset(0);
}

void android_emuctl_client_setsensors(const QAndroidSensorsAgent* agent) {
    sGlobals->sensors = agent;
}

