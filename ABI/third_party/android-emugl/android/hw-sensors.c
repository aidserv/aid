/* Copyright (C) 2009 The Android Open Source Project
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

#include "android/hw-sensors.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/sensors-port.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"
#include "android/utils/looper.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#define V(...) VERBOSE_PRINT(init, __VA_ARGS__)

/* define T_ACTIVE to 1 to debug transport communications */
#define T_ACTIVE 0

#if T_ACTIVE
#define T(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif

/* this code supports emulated sensor hardware
 *
 * Note that currently, only the accelerometer is really emulated, and only
 * for the purpose of allowing auto-rotating the screen in keyboard-less
 * configurations.
 *
 *
 */

static const struct {
    const char* name;
    int id;
} _sSensors[MAX_SENSORS] = {
#define SENSOR_(x, y) {y, ANDROID_SENSOR_##x},
        SENSORS_LIST
#undef SENSOR_
};

static int _sensorIdFromName(const char* name) {
    int nn;
    for (nn = 0; nn < MAX_SENSORS; nn++)
        if (!strcmp(_sSensors[nn].name, name))
            return _sSensors[nn].id;
    return -1;
}

static const char* _sensorNameFromId(int id) {
    int nn;
    for (nn = 0; nn < MAX_SENSORS; nn++)
        if (id == _sSensors[nn].id)
            return _sSensors[nn].name;
    return NULL;
}

/* For common Sensor Value struct */
typedef struct { float a, b, c; } SensorValues;

typedef struct { float x, y, z; } Acceleration;

typedef struct { float x, y, z; } Gyroscope;

typedef struct { float x, y, z; } MagneticField;

typedef struct {
    float azimuth;
    float pitch;
    float roll;
} Orientation;

typedef struct { float celsius; } Temperature;

typedef struct { float value; } Proximity;

typedef struct { float value; } Light;

typedef struct { float value; } Pressure;

typedef struct { float value; } Humidity;

typedef struct {
    bool valid;
    int length;
    char value[128];
} SerializedSensor;

typedef struct {
    bool enabled;
    union {
        SensorValues value;
        Acceleration acceleration;
        Gyroscope gyroscope;
        MagneticField magnetic;
        Orientation orientation;
        Temperature temperature;
        Proximity proximity;
        Light light;
        Pressure pressure;
        Humidity humidity;
        MagneticField magnetic_uncalibrated;
        Gyroscope gyroscope_uncalibrated;
    } u;
    SerializedSensor serialized;
} Sensor;

/*
 * - when the qemu-specific sensors HAL module starts, it sends
 *   "list-sensors"
 *
 * - this code replies with a string containing an integer corresponding
 *   to a bitmap of available hardware sensors in the current AVD
 *   configuration (e.g. "1" a.k.a (1 << ANDROID_SENSOR_ACCELERATION))
 *
 * - the HAL module sends "set:<sensor>:<flag>" to enable or disable
 *   the report of a given sensor state. <sensor> must be the name of
 *   a given sensor (e.g. "accelerometer"), and <flag> must be either
 *   "1" (to enable) or "0" (to disable).
 *
 * - Once at least one sensor is "enabled", this code should periodically
 *   send information about the corresponding enabled sensors. The default
 *   period is 200ms.
 *
 * - the HAL module sends "set-delay:<delay>", where <delay> is an integer
 *   corresponding to a time delay in milli-seconds. This corresponds to
 *   a new interval between sensor events sent by this code to the HAL
 *   module.
 *
 * - the HAL module can also send a "wake" command. This code should simply
 *   send the "wake" back to the module. This is used internally to wake a
 *   blocking read that happens in a different thread. This ping-pong makes
 *   the code in the HAL module very simple.
 *
 * - each timer tick, this code sends sensor reports in the following
 *   format (each line corresponds to a different line sent to the module):
 *
 *      acceleration:<x>:<y>:<z>
 *      magnetic-field:<x>:<y>:<z>
 *      orientation:<azimuth>:<pitch>:<roll>
 *      temperature:<celsius>
 *      light:<lux>
 *      pressure:<hpa>
 *      humidity:<percent>
 *      sync:<time_us>
 *
 *   Where each line before the sync:<time_us> is optional and will only
 *   appear if the corresponding sensor has been enabled by the HAL module.
 *
 *   Note that <time_us> is the VM time in micro-seconds when the report
 *   was "taken" by this code. This is adjusted by the HAL module to
 *   emulated system time (using the first sync: to compute an adjustment
 *   offset).
 */
#define HEADER_SIZE 4
#define BUFFER_SIZE 512

typedef struct HwSensorClient HwSensorClient;

typedef struct {
    QemudService* service;
    Sensor sensors[MAX_SENSORS];
    HwSensorClient* clients;
    AndroidSensorsPort* sensors_port;
} HwSensors;

struct HwSensorClient {
    HwSensorClient* next;
    HwSensors* sensors;
    QemudClient* client;
    LoopTimer* timer;
    uint32_t enabledMask;
    int32_t delay_ms;
};

static void _hwSensorClient_free(HwSensorClient* cl) {
    /* remove from sensors's list */
    if (cl->sensors) {
        HwSensorClient** pnode = &cl->sensors->clients;
        for (;;) {
            HwSensorClient* node = *pnode;
            if (node == NULL)
                break;
            if (node == cl) {
                *pnode = cl->next;
                break;
            }
            pnode = &node->next;
        }
        cl->next = NULL;
        cl->sensors = NULL;
    }

    /* close QEMUD client, if any */
    if (cl->client) {
        qemud_client_close(cl->client);
        cl->client = NULL;
    }
    /* remove timer, if any */
    if (cl->timer) {
        loopTimer_stop(cl->timer);
        loopTimer_free(cl->timer);
        cl->timer = NULL;
    }
    AFREE(cl);
}

/* forward */
static void _hwSensorClient_tick(void* opaque, LoopTimer* timer);

static HwSensorClient* _hwSensorClient_new(HwSensors* sensors) {
    HwSensorClient* cl;

    ANEW0(cl);

    cl->sensors = sensors;
    cl->enabledMask = 0;
    cl->delay_ms = 800;
    cl->timer =
            loopTimer_newWithClock(looper_getForThread(), _hwSensorClient_tick,
                                   cl, LOOPER_CLOCK_VIRTUAL);

    cl->next = sensors->clients;
    sensors->clients = cl;

    return cl;
}

/* forward */

static void _hwSensorClient_receive(HwSensorClient* cl,
                                    uint8_t* query,
                                    int querylen);

/* Qemud service management */

static void _hwSensorClient_recv(void* opaque,
                                 uint8_t* msg,
                                 int msglen,
                                 QemudClient* client) {
    HwSensorClient* cl = opaque;

    _hwSensorClient_receive(cl, msg, msglen);
}

static void _hwSensorClient_close(void* opaque) {
    HwSensorClient* cl = opaque;

    /* the client is already closed here */
    cl->client = NULL;
    _hwSensorClient_free(cl);
}

/* send a one-line message to the HAL module through a qemud channel */
static void _hwSensorClient_send(HwSensorClient* cl,
                                 const uint8_t* msg,
                                 int msglen) {
    D("%s: '%s'", __FUNCTION__, quote_bytes((const void*)msg, msglen));
    qemud_client_send(cl->client, msg, msglen);
}

static int _hwSensorClient_enabled(HwSensorClient* cl, int sensorId) {
    return (cl->enabledMask & (1 << sensorId)) != 0;
}

/* a helper function that replaces commas (,) with points (.).
 * Each sensor string must be processed this way before being
 * sent into the guest. This is because the system locale may
 * cause decimal values to be formatted with a comma instead of
 * a decimal point, but that would not be parsed correctly
 * within the guest.
 */
static void _hwSensorClient_sanitizeSensorString(char* string, int maxlen) {
    int i;
    for (i = 0; i < maxlen && string[i] != '\0'; i++) {
        if (string[i] == ',') {
            string[i] = '.';
        }
    }
}

// a function to serialize the sensor value based on its ID
static void serializeSensorValue(Sensor* sensor, AndroidSensor sensor_id) {
    switch (sensor_id) {
        case ANDROID_SENSOR_ACCELERATION: {
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "acceleration:%g:%g:%g", sensor->u.acceleration.x,
                    sensor->u.acceleration.y, sensor->u.acceleration.z);

            // TODO(grigoryj): debug output for investigating rotation bug
            static float prev_acceleration[] = {0.0f, 0.0f, 0.0f};
            if (VERBOSE_CHECK(rotation) &&
                (prev_acceleration[0] != sensor->u.acceleration.x ||
                 prev_acceleration[1] != sensor->u.acceleration.y ||
                 prev_acceleration[2] != sensor->u.acceleration.z)) {
                fprintf(stderr, "Sent %s to sensors HAL\n",
                        sensor->serialized.value);
                prev_acceleration[0] = sensor->u.acceleration.x;
                prev_acceleration[1] = sensor->u.acceleration.y;
                prev_acceleration[2] = sensor->u.acceleration.z;
            }
            break;
        }
        case ANDROID_SENSOR_GYROSCOPE: {
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "gyroscope:%g:%g:%g", sensor->u.gyroscope.x,
                    sensor->u.gyroscope.y, sensor->u.gyroscope.z);
            break;
        }
        case ANDROID_SENSOR_MAGNETIC_FIELD:
            /* NOTE: sensors HAL expects "magnetic", not "magnetic-field"
             * name here.
             */
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "magnetic:%g:%g:%g", sensor->u.magnetic.x,
                    sensor->u.magnetic.y, sensor->u.magnetic.z);
            break;
        case ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED:
            /* NOTE: sensors HAL expects "magnetic-uncalibrated",
             * not "magnetic-field-uncalibrated" name here.
             */
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "magnetic-uncalibrated:%g:%g:%g", sensor->u.magnetic_uncalibrated.x,
                    sensor->u.magnetic_uncalibrated.y, sensor->u.magnetic_uncalibrated.z);
            break;
        case ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "gyroscope-uncalibrated:%g:%g:%g", sensor->u.gyroscope_uncalibrated.x,
                    sensor->u.gyroscope_uncalibrated.y, sensor->u.gyroscope_uncalibrated.z);
            break;
        case ANDROID_SENSOR_ORIENTATION:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "orientation:%g:%g:%g", sensor->u.orientation.azimuth,
                    sensor->u.orientation.pitch, sensor->u.orientation.roll);
            break;
        case ANDROID_SENSOR_TEMPERATURE:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "temperature:%g", sensor->u.temperature.celsius);
            break;
        case ANDROID_SENSOR_PROXIMITY:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "proximity:%g", sensor->u.proximity.value);
            break;
        case ANDROID_SENSOR_LIGHT:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "light:%g", sensor->u.light.value);
            break;
        case ANDROID_SENSOR_PRESSURE:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "pressure:%g", sensor->u.pressure.value);
            break;
        case ANDROID_SENSOR_HUMIDITY:
            sensor->serialized.length = snprintf(
                    sensor->serialized.value, sizeof(sensor->serialized.value),
                    "humidity:%g", sensor->u.humidity.value);
            break;
        default:
            assert(false);  // should never happen
            return;
    }
    assert(sensor->serialized.length < sizeof(sensor->serialized.value));
    sensor->serialized.valid = true;

    _hwSensorClient_sanitizeSensorString(sensor->serialized.value,
                                         sensor->serialized.length);
}

/* this function is called periodically to send sensor reports
 * to the HAL module, and re-arm the timer if necessary
 */
static void _hwSensorClient_tick(void* opaque, LoopTimer* unused) {
    HwSensorClient* cl = opaque;
    int64_t delay_ms = cl->delay_ms;
    const uint32_t mask = cl->enabledMask;

    // Grab the guest time before sending any sensor data:
    // the android.hardware CTS requires sync times to be no greater than the
    // time of the sensor event arrival. Since the CTS enforces this property,
    // other code may also rely on it.
    const DurationNs now_ns =
            looper_nowNsWithClock(looper_getForThread(), LOOPER_CLOCK_VIRTUAL);

    AndroidSensor sensor_id;
    for (sensor_id = 0; sensor_id < MAX_SENSORS; ++sensor_id) {
        if (!_hwSensorClient_enabled(cl, sensor_id)) {
            continue;
        }
        Sensor* sensor = &cl->sensors->sensors[sensor_id];
        if (!sensor->serialized.valid) {
            serializeSensorValue(sensor, sensor_id);
        }
        _hwSensorClient_send(cl, (uint8_t*)sensor->serialized.value,
                             sensor->serialized.length);
    }

    char buffer[64];
    int buffer_len =
            snprintf(buffer, sizeof(buffer), "sync:%" PRId64, now_ns / 1000);
    assert(buffer_len < sizeof(buffer));
    _hwSensorClient_send(cl, (uint8_t*)buffer, buffer_len);

    if (mask == 0)
        return;

    // Rearm the timer to fire a little bit early, so we can sustain the
    // requested frequency. Also make sure we have at least a minimal delay,
    // otherwise this timer would hijack the main loop thread and won't allow
    // guest to ever run.
    // Note: (-1) is needed to account for an overhead of the timer firing on
    //  the host and execution of the tick function. 1ms is enough, as the
    //  overhead is very small; on the other hand, it's always present - that's
    //  why we can't just have (delay_ms) here.
    // Note2: Let's cap the minimal tick interval to 10ms, to make sure:
    // - We never overload the main QEMU loop.
    // - Some CTS hardware test cases require a limit on the maximum update rate,
    //   which has been known to be in the low 100's of Hz.
    if (delay_ms < 10) {
        delay_ms = 10;
    } else if (delay_ms > 60 * 60 * 1000) {
        delay_ms = 60 * 60 * 1000;
    }
    loopTimer_startRelative(cl->timer, delay_ms - 1);
}

/* handle incoming messages from the HAL module */
static void _hwSensorClient_receive(HwSensorClient* cl,
                                    uint8_t* msg,
                                    int msglen) {
    HwSensors* hw = cl->sensors;

    D("%s: '%.*s'", __FUNCTION__, msglen, msg);

    /* "list-sensors" is used to get an integer bit map of
     * available emulated sensors. We compute the mask from the
     * current hardware configuration.
     */
    if (msglen == 12 && !memcmp(msg, "list-sensors", 12)) {
        char buff[12];
        int mask = 0;
        int nn;

        for (nn = 0; nn < MAX_SENSORS; nn++) {
            if (hw->sensors[nn].enabled) {
                mask |= (1 << nn);
            }
        }

        const int len = snprintf(buff, sizeof buff, "%d", mask);
        assert(len < sizeof(buff));
        _hwSensorClient_send(cl, (const uint8_t*)buff, len);
        return;
    }

    /* "wake" is a special message that must be sent back through
     * the channel. It is used to exit a blocking read.
     */
    if (msglen == 4 && !memcmp(msg, "wake", 4)) {
        _hwSensorClient_send(cl, (const uint8_t*)"wake", 4);
        return;
    }

    /* "set-delay:<delay>" is used to set the delay in milliseconds
     * between sensor events
     */
    if (msglen > 10 && !memcmp(msg, "set-delay:", 10)) {
        cl->delay_ms = atoi((const char*)msg + 10);
        if (cl->enabledMask != 0)
            _hwSensorClient_tick(cl, cl->timer);

        return;
    }

    /* "set:<name>:<state>" is used to enable/disable a given
     * sensor. <state> must be 0 or 1
     */
    if (msglen > 4 && !memcmp(msg, "set:", 4)) {
        char* q;
        int id, enabled, oldEnabledMask = cl->enabledMask;
        msg += 4;
        q = strchr((char*)msg, ':');
        if (q == NULL) { /* should not happen */
            D("%s: ignore bad 'set' command", __FUNCTION__);
            return;
        }
        *q++ = 0;

        id = _sensorIdFromName((const char*)msg);
        if (id < 0 || id >= MAX_SENSORS) {
            D("%s: ignore unknown sensor name '%s'", __FUNCTION__, msg);
            return;
        }

        if (!hw->sensors[id].enabled) {
            D("%s: trying to set disabled %s sensor", __FUNCTION__, msg);
            return;
        }
        enabled = (q[0] == '1');

        if (enabled)
            cl->enabledMask |= (1 << id);
        else
            cl->enabledMask &= ~(1 << id);

        if (cl->enabledMask != (uint32_t)oldEnabledMask) {
            D("%s: %s %s sensor", __FUNCTION__,
              (cl->enabledMask & (1 << id)) ? "enabling" : "disabling", msg);
        }

        /* If emulating device is connected update sensor state there too. */
        if (hw->sensors_port != NULL) {
            if (enabled) {
                sensors_port_enable_sensor(hw->sensors_port, (const char*)msg);
            } else {
                sensors_port_disable_sensor(hw->sensors_port, (const char*)msg);
            }
        }

        _hwSensorClient_tick(cl, cl->timer);
        return;
    }

    D("%s: ignoring unknown query", __FUNCTION__);
}

/* Saves sensor-specific client data to snapshot */
static void _hwSensorClient_save(Stream* f, QemudClient* client, void* opaque) {
    HwSensorClient* sc = opaque;

    stream_put_be32(f, sc->delay_ms);
    stream_put_be32(f, sc->enabledMask);
    stream_put_timer(f, sc->timer);
}

/* Loads sensor-specific client data from snapshot */
static int _hwSensorClient_load(Stream* f, QemudClient* client, void* opaque) {
    HwSensorClient* sc = opaque;

    sc->delay_ms = stream_get_be32(f);
    sc->enabledMask = stream_get_be32(f);
    stream_get_timer(f, sc->timer);

    return 0;
}

static QemudClient* _hwSensors_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param) {
    HwSensors* sensors = opaque;
    HwSensorClient* cl = _hwSensorClient_new(sensors);
    QemudClient* client = qemud_client_new(
            service, channel, client_param, cl, _hwSensorClient_recv,
            _hwSensorClient_close, _hwSensorClient_save, _hwSensorClient_load);
    qemud_client_set_framing(client, 1);
    cl->client = client;

    return client;
}

/* change the value of the emulated sensor vector */
static void _hwSensors_setSensorValue(HwSensors* h,
                                      int sensor_id,
                                      float a,
                                      float b,
                                      float c) {
    Sensor* s = &h->sensors[sensor_id];

    if (s->u.value.a != a || s->u.value.b != b || s->u.value.c != c) {
        s->u.value.a = a;
        s->u.value.b = b;
        s->u.value.c = c;
        s->serialized.valid = false;
    }
}

/* Saves available sensors to allow checking availability when loaded.
 */
static void _hwSensors_save(Stream* f, QemudService* sv, void* opaque) {
    HwSensors* h = opaque;

    // number of sensors
    stream_put_be32(f, MAX_SENSORS);
    AndroidSensor sensor_id;
    for (sensor_id = 0; sensor_id < MAX_SENSORS; sensor_id++) {
        Sensor* s = &h->sensors[sensor_id];
        stream_put_be32(f, s->enabled);

        /* this switch ensures that a warning is raised when a new sensor is
         * added and is not added here as well.
         */
        switch (sensor_id) {
            case ANDROID_SENSOR_ACCELERATION:
                stream_put_float(f, s->u.acceleration.x);
                stream_put_float(f, s->u.acceleration.y);
                stream_put_float(f, s->u.acceleration.z);
                break;
            case ANDROID_SENSOR_GYROSCOPE:
                stream_put_float(f, s->u.gyroscope.x);
                stream_put_float(f, s->u.gyroscope.y);
                stream_put_float(f, s->u.gyroscope.z);
                break;
            case ANDROID_SENSOR_MAGNETIC_FIELD:
                stream_put_float(f, s->u.magnetic.x);
                stream_put_float(f, s->u.magnetic.y);
                stream_put_float(f, s->u.magnetic.z);
                break;
            case ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED:
                stream_put_float(f, s->u.magnetic_uncalibrated.x);
                stream_put_float(f, s->u.magnetic_uncalibrated.y);
                stream_put_float(f, s->u.magnetic_uncalibrated.z);
                break;
            case ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED:
                stream_put_float(f, s->u.gyroscope_uncalibrated.x);
                stream_put_float(f, s->u.gyroscope_uncalibrated.y);
                stream_put_float(f, s->u.gyroscope_uncalibrated.z);
                break;
            case ANDROID_SENSOR_ORIENTATION:
                stream_put_float(f, s->u.orientation.azimuth);
                stream_put_float(f, s->u.orientation.pitch);
                stream_put_float(f, s->u.orientation.roll);
                break;
            case ANDROID_SENSOR_TEMPERATURE:
                stream_put_float(f, s->u.temperature.celsius);
                break;
            case ANDROID_SENSOR_PROXIMITY:
                stream_put_float(f, s->u.proximity.value);
                break;
            case ANDROID_SENSOR_LIGHT:
                stream_put_float(f, s->u.light.value);
                break;
            case ANDROID_SENSOR_PRESSURE:
                stream_put_float(f, s->u.pressure.value);
                break;
            case ANDROID_SENSOR_HUMIDITY:
                stream_put_float(f, s->u.humidity.value);
                break;

            case MAX_SENSORS:
                break;
        }
    }
}

static int _hwSensors_load(Stream* f, QemudService* s, void* opaque) {
    HwSensors* h = opaque;

    /* check number of sensors */
    int32_t num_sensors = stream_get_be32(f);
    if (num_sensors > MAX_SENSORS) {
        D("%s: cannot load: snapshot requires %d sensors, %d available\n",
          __FUNCTION__, num_sensors, MAX_SENSORS);
        return -EIO;
    }

    /* load sensor state */
    AndroidSensor sensor_id;
    for (sensor_id = 0; sensor_id < (AndroidSensor)num_sensors; sensor_id++) {
        Sensor* s = &h->sensors[sensor_id];
        s->enabled = stream_get_be32(f);

        /* this switch ensures that a warning is raised when a new sensor is
         * added and is not added here as well.
         */
        switch (sensor_id) {
            case ANDROID_SENSOR_ACCELERATION:
                s->u.acceleration.x = stream_get_float(f);
                s->u.acceleration.y = stream_get_float(f);
                s->u.acceleration.z = stream_get_float(f);
                break;
            case ANDROID_SENSOR_GYROSCOPE:
                s->u.gyroscope.x = stream_get_float(f);
                s->u.gyroscope.y = stream_get_float(f);
                s->u.gyroscope.z = stream_get_float(f);
                break;
            case ANDROID_SENSOR_MAGNETIC_FIELD:
                s->u.magnetic.x = stream_get_float(f);
                s->u.magnetic.y = stream_get_float(f);
                s->u.magnetic.z = stream_get_float(f);
                break;
            case ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED:
                s->u.magnetic_uncalibrated.x = stream_get_float(f);
                s->u.magnetic_uncalibrated.y = stream_get_float(f);
                s->u.magnetic_uncalibrated.z = stream_get_float(f);
                break;
            case ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED:
                s->u.gyroscope_uncalibrated.x = stream_get_float(f);
                s->u.gyroscope_uncalibrated.y = stream_get_float(f);
                s->u.gyroscope_uncalibrated.z = stream_get_float(f);
                break;
            case ANDROID_SENSOR_ORIENTATION:
                s->u.orientation.azimuth = stream_get_float(f);
                s->u.orientation.pitch = stream_get_float(f);
                s->u.orientation.roll = stream_get_float(f);
                break;
            case ANDROID_SENSOR_TEMPERATURE:
                s->u.temperature.celsius = stream_get_float(f);
                break;
            case ANDROID_SENSOR_PROXIMITY:
                s->u.proximity.value = stream_get_float(f);
                break;
            case ANDROID_SENSOR_LIGHT:
                s->u.light.value = stream_get_float(f);
                break;
            case ANDROID_SENSOR_PRESSURE:
                s->u.pressure.value = stream_get_float(f);
                break;
            case ANDROID_SENSOR_HUMIDITY:
                s->u.humidity.value = stream_get_float(f);
                break;
            case MAX_SENSORS:
                break;
        }

        // Make sure we re-serialize it on the next tick.
        s->serialized.valid = false;
    }

    /* The following is necessary when we resume a snaphost
     * created by an older version of the emulator that provided
     * less hardware sensors.
     */
    for (; sensor_id < MAX_SENSORS; sensor_id++) {
        h->sensors[sensor_id].enabled = false;
        h->sensors[sensor_id].serialized.valid = false;
    }

    return 0;
}

/* change the emulated proximity */
static void _hwSensors_setProximity(HwSensors* h, float value) {
    Sensor* s = &h->sensors[ANDROID_SENSOR_PROXIMITY];
    if (s->u.proximity.value != value) {
        s->u.proximity.value = value;
        s->serialized.valid = false;
    }
}

/* change the coarse orientation (landscape/portrait) of the emulated device */
static void _hwSensors_setCoarseOrientation(HwSensors* h,
                                            AndroidCoarseOrientation orient) {
    /* The Android framework computes the orientation by looking at
     * the accelerometer sensor (*not* the orientation sensor !)
     *
     * That's because the gravity is a constant 9.81 vector that
     * can be determined quite easily.
     *
     * Also, for some reason, the framework code considers that the phone should
     * be inclined by 30 degrees along the phone's X axis to be considered
     * in its ideal "vertical" position
     *
     * If the phone is completely vertical, rotating it will not do anything !
     */
    const double g = 9.81;
    const double angle = 20.0;
    const double cos_angle = cos(angle / M_PI);
    const double sin_angle = sin(angle / M_PI);

    if (VERBOSE_CHECK(rotation)) {
        fprintf(stderr, "setCoarseOrientation - HwSensors %p\n", h);
    }
    switch (orient) {
        case ANDROID_COARSE_PORTRAIT:
            if (VERBOSE_CHECK(rotation)) {
                fprintf(stderr, "Setting coarse orientation to portrait\n");
            }
            _hwSensors_setSensorValue(h, ANDROID_SENSOR_ACCELERATION, 0.,
                                      g * cos_angle, g * sin_angle);
            break;

        case ANDROID_COARSE_REVERSE_LANDSCAPE:
            if (VERBOSE_CHECK(rotation)) {
                fprintf(stderr,
                        "Setting coarse orientation to reverse landscape\n");
            }
            _hwSensors_setSensorValue(h, ANDROID_SENSOR_ACCELERATION,
                                      -g * cos_angle, 0., -g * sin_angle);
            break;

        case ANDROID_COARSE_REVERSE_PORTRAIT:
            if (VERBOSE_CHECK(rotation)) {
                fprintf(stderr,
                        "Setting coarse orientation to reverse portrait\n");
            }
            _hwSensors_setSensorValue(h, ANDROID_SENSOR_ACCELERATION, 0.,
                                      -g * cos_angle, -g * sin_angle);
            break;

        case ANDROID_COARSE_LANDSCAPE:
            if (VERBOSE_CHECK(rotation)) {
                fprintf(stderr, "Setting coarse orientation to landscape\n");
            }
            _hwSensors_setSensorValue(h, ANDROID_SENSOR_ACCELERATION,
                                      g * cos_angle, 0., g * sin_angle);
            break;
        default:
            if (VERBOSE_CHECK(rotation)) {
                fprintf(stderr, "Invalid orientation\n");
            }
    }
}

/* initialize the sensors state */
static void _hwSensors_init(HwSensors* h) {
    h->sensors_port = NULL;

    h->service = qemud_service_register("sensors", 0, h, _hwSensors_connect,
                                        _hwSensors_save, _hwSensors_load);

    if (android_hw->hw_accelerometer) {
        h->sensors[ANDROID_SENSOR_ACCELERATION].enabled = true;
    }

    if (android_hw->hw_gyroscope) {
        h->sensors[ANDROID_SENSOR_GYROSCOPE].enabled = true;
    }

    if (android_hw->hw_sensors_proximity) {
        h->sensors[ANDROID_SENSOR_PROXIMITY].enabled = true;
    }

    if (android_hw->hw_sensors_magnetic_field) {
        h->sensors[ANDROID_SENSOR_MAGNETIC_FIELD].enabled = true;
    }

    if (android_hw->hw_sensors_magnetic_field_uncalibrated) {
        h->sensors[ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED].enabled = true;
    }

    if (android_hw->hw_sensors_magnetic_field_uncalibrated) {
        h->sensors[ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED].enabled = true;
    }

    if (android_hw->hw_sensors_orientation) {
        h->sensors[ANDROID_SENSOR_ORIENTATION].enabled = true;
    }

    if (android_hw->hw_sensors_temperature) {
        h->sensors[ANDROID_SENSOR_TEMPERATURE].enabled = true;
    }

    if (android_hw->hw_sensors_light) {
        h->sensors[ANDROID_SENSOR_LIGHT].enabled = true;
    }

    if (android_hw->hw_sensors_pressure) {
        h->sensors[ANDROID_SENSOR_PRESSURE].enabled = true;
    }

    if (android_hw->hw_sensors_humidity) {
        h->sensors[ANDROID_SENSOR_HUMIDITY].enabled = true;
    }

    /* XXX: TODO: Add other tests when we add the corresponding
        * properties to hardware-properties.ini et al. */

    _hwSensors_setCoarseOrientation(h, ANDROID_COARSE_PORTRAIT);
    _hwSensors_setProximity(h, 1);
}

static HwSensors _sensorsState[1] = {};

void android_hw_sensors_init(void) {
    HwSensors* hw = _sensorsState;

    if (hw->service == NULL) {
        _hwSensors_init(hw);
        D("%s: sensors qemud service initialized", __FUNCTION__);
    }
}

void android_hw_sensors_init_remote_controller(void) {
    HwSensors* hw = _sensorsState;

    if (!hw->sensors_port) {
        /* Try to see if there is a device attached that can be used for
        * sensor emulation. */
        hw->sensors_port = sensors_port_create(hw);
        if (hw->sensors_port == NULL) {
            V("Realistic sensor emulation is not available, since the remote "
              "controller is not accessible:\n %s",
              strerror(errno));
        }
    }
}

/* change the coarse orientation value */
extern void android_sensors_set_coarse_orientation(
        AndroidCoarseOrientation orient) {
    android_hw_sensors_init();
    _hwSensors_setCoarseOrientation(_sensorsState, orient);
}

/* Get sensor name from sensor id */
extern const char* android_sensors_get_name_from_id(int sensor_id) {
    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return NULL;

    return _sensorNameFromId(sensor_id);
}

/* Get sensor id from sensor name */
extern int android_sensors_get_id_from_name(char* sensorname) {
    HwSensors* hw = _sensorsState;

    if (sensorname == NULL)
        return SENSOR_STATUS_UNKNOWN;

    int id = _sensorIdFromName(sensorname);

    if (id < 0 || id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    if (hw->service != NULL) {
        if (!hw->sensors[id].enabled)
            return SENSOR_STATUS_DISABLED;
    } else
        return SENSOR_STATUS_NO_SERVICE;

    return id;
}

/* Interface of reading the data for all sensors */
extern int android_sensors_get(int sensor_id, float* a, float* b, float* c) {
    HwSensors* hw = _sensorsState;

    *a = 0;
    *b = 0;
    *c = 0;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    Sensor* sensor = &hw->sensors[sensor_id];
    if (hw->service != NULL) {
        if (!sensor->enabled)
            return SENSOR_STATUS_DISABLED;
    } else
        return SENSOR_STATUS_NO_SERVICE;

    *a = sensor->u.value.a;
    *b = sensor->u.value.b;
    *c = sensor->u.value.c;

    return SENSOR_STATUS_OK;
}

/* Interface of setting the data for all sensors */
extern int android_sensors_set(int sensor_id, float a, float b, float c) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    if (hw->service != NULL) {
        if (!hw->sensors[sensor_id].enabled)
            return SENSOR_STATUS_DISABLED;
    } else {
        return SENSOR_STATUS_NO_SERVICE;
    }

    _hwSensors_setSensorValue(hw, sensor_id, a, b, c);

    return SENSOR_STATUS_OK;
}

/* Get Sensor from sensor id */
extern uint8_t android_sensors_get_sensor_status(int sensor_id) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    return hw->sensors[sensor_id].enabled;
}
