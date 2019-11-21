// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

//DO NOT EDIT, USE update_enum.sh TO UPDATE

#pragma once

namespace emulator {
enum class VehiclePropertyType : int32_t {
    STRING = 1048576, // 0x00100000
    BOOLEAN = 2097152, // 0x00200000
    INT32 = 4194304, // 0x00400000
    INT32_VEC = 4259840, // 0x00410000
    INT64 = 5242880, // 0x00500000
    FLOAT = 6291456, // 0x00600000
    FLOAT_VEC = 6356992, // 0x00610000
    BYTES = 7340032, // 0x00700000
    COMPLEX = 14680064, // 0x00e00000
    MASK = 16711680, // 0x00ff0000
};

enum class VehicleArea : int32_t {
    GLOBAL = 16777216, // 0x01000000
    ZONE = 33554432, // 0x02000000
    WINDOW = 50331648, // 0x03000000
    MIRROR = 67108864, // 0x04000000
    SEAT = 83886080, // 0x05000000
    DOOR = 100663296, // 0x06000000
    MASK = 251658240, // 0x0f000000
};

enum class VehiclePropertyGroup : int32_t {
    SYSTEM = 268435456, // 0x10000000
    VENDOR = 536870912, // 0x20000000
    MASK = -268435456, // 0xf0000000
};

enum class VehicleProperty : int32_t {
    INVALID = 0, // 0x00000000
    INFO_VIN = 286261504, // (((0x0100 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    INFO_MAKE = 286261505, // (((0x0101 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    INFO_MODEL = 286261506, // (((0x0102 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    INFO_MODEL_YEAR = 289407235, // (((0x0103 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    INFO_FUEL_CAPACITY = 291504388, // (((0x0104 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    PERF_ODOMETER = 291504644, // (((0x0204 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    PERF_VEHICLE_SPEED = 291504647, // (((0x0207 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    ENGINE_COOLANT_TEMP = 291504897, // (((0x0301 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    ENGINE_OIL_TEMP = 291504900, // (((0x0304 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    ENGINE_RPM = 291504901, // (((0x0305 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    WHEEL_TICK = 291570438, // (((0x0306 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT_VEC) | VehicleArea:GLOBAL)
    GEAR_SELECTION = 289408000, // (((0x0400 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    CURRENT_GEAR = 289408001, // (((0x0401 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    PARKING_BRAKE_ON = 287310850, // (((0x0402 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    DRIVING_STATUS = 289408004, // (((0x0404 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    FUEL_LEVEL_LOW = 287310853, // (((0x0405 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    NIGHT_MODE = 287310855, // (((0x0407 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    TURN_SIGNAL_STATE = 289408008, // (((0x0408 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    IGNITION_STATE = 289408009, // (((0x0409 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    HVAC_FAN_SPEED = 306185472, // (((0x0500 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:ZONE)
    HVAC_FAN_DIRECTION = 306185473, // (((0x0501 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:ZONE)
    HVAC_TEMPERATURE_CURRENT = 308282626, // (((0x0502 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:ZONE)
    HVAC_TEMPERATURE_SET = 308282627, // (((0x0503 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:ZONE)
    HVAC_DEFROSTER = 320865540, // (((0x0504 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:WINDOW)
    HVAC_AC_ON = 304088325, // (((0x0505 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_MAX_AC_ON = 304088326, // (((0x0506 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_MAX_DEFROST_ON = 304088327, // (((0x0507 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_RECIRC_ON = 304088328, // (((0x0508 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_DUAL_ON = 304088329, // (((0x0509 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_AUTO_ON = 304088330, // (((0x050A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    HVAC_SEAT_TEMPERATURE = 356517131, // (((0x050B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    HVAC_SIDE_MIRROR_HEAT = 339739916, // (((0x050C | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    HVAC_STEERING_WHEEL_TEMP = 289408269, // (((0x050D | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    HVAC_TEMPERATURE_UNITS = 306185486, // (((0x050E | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:ZONE)
    HVAC_ACTUAL_FAN_SPEED_RPM = 306185487, // (((0x050F | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:ZONE)
    HVAC_FAN_DIRECTION_AVAILABLE = 306185489, // (((0x0511 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:ZONE)
    HVAC_POWER_ON = 304088336, // (((0x0510 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:ZONE)
    ENV_OUTSIDE_TEMPERATURE = 291505923, // (((0x0703 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    ENV_CABIN_TEMPERATURE = 291505924, // (((0x0704 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:FLOAT) | VehicleArea:GLOBAL)
    RADIO_PRESET = 289474561, // (((0x0801 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_FOCUS = 289474816, // (((0x0900 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_FOCUS_EXT_SYNC = 289474832, // (((0x0910 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_VOLUME = 289474817, // (((0x0901 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_VOLUME_EXT_SYNC = 289474833, // (((0x0911 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_VOLUME_LIMIT = 289474818, // (((0x0902 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_ROUTING_POLICY = 289474819, // (((0x0903 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_HW_VARIANT = 289409284, // (((0x0904 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    AUDIO_EXT_ROUTING_HINT = 289474821, // (((0x0905 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_STREAM_STATE = 289474822, // (((0x0906 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    AUDIO_PARAMETERS = 286263559, // (((0x907 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:STRING) | VehicleArea:GLOBAL)
    AP_POWER_STATE = 289475072, // (((0x0A00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    DISPLAY_BRIGHTNESS = 289409537, // (((0x0A01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    AP_POWER_BOOTUP_REASON = 289409538, // (((0x0A02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    HW_KEY_INPUT = 289475088, // (((0x0A10 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    INSTRUMENT_CLUSTER_INFO = 289475104, // (((0x0A20 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32_VEC) | VehicleArea:GLOBAL)
    UNIX_TIME = 290458160, // (((0x0A30 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT64) | VehicleArea:GLOBAL)
    CURRENT_TIME_IN_SECONDS = 289409585, // (((0x0A31 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    DOOR_POS = 373295872, // (((0x0B00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:DOOR)
    DOOR_MOVE = 373295873, // (((0x0B01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:DOOR)
    DOOR_LOCK = 371198722, // (((0x0B02 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:DOOR)
    MIRROR_Z_POS = 339741504, // (((0x0B40 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    MIRROR_Z_MOVE = 339741505, // (((0x0B41 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    MIRROR_Y_POS = 339741506, // (((0x0B42 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    MIRROR_Y_MOVE = 339741507, // (((0x0B43 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:MIRROR)
    MIRROR_LOCK = 287312708, // (((0x0B44 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    MIRROR_FOLD = 287312709, // (((0x0B45 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    SEAT_MEMORY_SELECT = 356518784, // (((0x0B80 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_MEMORY_SET = 356518785, // (((0x0B81 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BELT_BUCKLED = 354421634, // (((0x0B82 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:SEAT)
    SEAT_BELT_HEIGHT_POS = 356518787, // (((0x0B83 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BELT_HEIGHT_MOVE = 356518788, // (((0x0B84 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_FORE_AFT_POS = 356518789, // (((0x0B85 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_FORE_AFT_MOVE = 356518790, // (((0x0B86 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BACKREST_ANGLE_1_POS = 356518791, // (((0x0B87 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BACKREST_ANGLE_1_MOVE = 356518792, // (((0x0B88 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BACKREST_ANGLE_2_POS = 356518793, // (((0x0B89 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_BACKREST_ANGLE_2_MOVE = 356518794, // (((0x0B8A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEIGHT_POS = 356518795, // (((0x0B8B | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEIGHT_MOVE = 356518796, // (((0x0B8C | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_DEPTH_POS = 356518797, // (((0x0B8D | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_DEPTH_MOVE = 356518798, // (((0x0B8E | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_TILT_POS = 356518799, // (((0x0B8F | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_TILT_MOVE = 356518800, // (((0x0B90 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_LUMBAR_FORE_AFT_POS = 356518801, // (((0x0B91 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_LUMBAR_FORE_AFT_MOVE = 356518802, // (((0x0B92 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_LUMBAR_SIDE_SUPPORT_POS = 356518803, // (((0x0B93 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_LUMBAR_SIDE_SUPPORT_MOVE = 356518804, // (((0x0B94 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEADREST_HEIGHT_POS = 289409941, // (((0x0B95 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    SEAT_HEADREST_HEIGHT_MOVE = 356518806, // (((0x0B96 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEADREST_ANGLE_POS = 356518807, // (((0x0B97 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEADREST_ANGLE_MOVE = 356518808, // (((0x0B98 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEADREST_FORE_AFT_POS = 356518809, // (((0x0B99 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    SEAT_HEADREST_FORE_AFT_MOVE = 356518810, // (((0x0B9A | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:SEAT)
    WINDOW_POS = 289409984, // (((0x0BC0 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    WINDOW_MOVE = 289409985, // (((0x0BC1 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    WINDOW_VENT_POS = 289409986, // (((0x0BC2 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    WINDOW_VENT_MOVE = 289409987, // (((0x0BC3 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:INT32) | VehicleArea:GLOBAL)
    WINDOW_LOCK = 287312836, // (((0x0BC4 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:BOOLEAN) | VehicleArea:GLOBAL)
    VEHICLE_MAP_SERVICE = 299895808, // (((0x0C00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:COMPLEX) | VehicleArea:GLOBAL)
    OBD2_LIVE_FRAME = 299896064, // (((0x0D00 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:COMPLEX) | VehicleArea:GLOBAL)
    OBD2_FREEZE_FRAME = 299896065, // (((0x0D01 | VehiclePropertyGroup:SYSTEM) | VehiclePropertyType:COMPLEX) | VehicleArea:GLOBAL)
};

enum class VehicleHvacFanDirection : int32_t {
    FACE = 1, // 0x1
    FLOOR = 2, // 0x2
    FACE_AND_FLOOR = 3, // 0x3
    DEFROST = 4, // 0x4
    DEFROST_AND_FLOOR = 5, // 0x5
};

enum class VehicleRadioConstants : int32_t {
    VEHICLE_RADIO_PRESET_MIN_VALUE = 1,
};

enum class VehicleAudioFocusRequest : int32_t {
    REQUEST_GAIN = 1, // 0x1
    REQUEST_GAIN_TRANSIENT = 2, // 0x2
    REQUEST_GAIN_TRANSIENT_MAY_DUCK = 3, // 0x3
    REQUEST_GAIN_TRANSIENT_NO_DUCK = 4, // 0x4
    REQUEST_RELEASE = 5, // 0x5
};

enum class VehicleAudioFocusState : int32_t {
    STATE_GAIN = 1, // 0x1
    STATE_GAIN_TRANSIENT = 2, // 0x2
    STATE_LOSS_TRANSIENT_CAN_DUCK = 3, // 0x3
    STATE_LOSS_TRANSIENT = 4, // 0x4
    STATE_LOSS = 5, // 0x5
    STATE_LOSS_TRANSIENT_EXLCUSIVE = 6, // 0x6
};

enum class VehicleAudioStreamFlag : int32_t {
    STREAM0_FLAG = 1, // (0x1 << 0)
    STREAM1_FLAG = 2, // (0x1 << 1)
    STREAM2_FLAG = 4, // (0x1 << 2)
};

enum class VehicleAudioStream : int32_t {
    STREAM0 = 0,
    STREAM1 = 1,
};

enum class VehicleAudioExtFocusFlag : int32_t {
    NONE_FLAG = 0, // 0x0
    PERMANENT_FLAG = 1, // 0x1
    TRANSIENT_FLAG = 2, // 0x2
    PLAY_ONLY_FLAG = 4, // 0x4
    MUTE_MEDIA_FLAG = 8, // 0x8
};

enum class VehicleAudioFocusIndex : int32_t {
    FOCUS = 0,
    STREAMS = 1,
    EXTERNAL_FOCUS_STATE = 2,
    AUDIO_CONTEXTS = 3,
};

enum class VehicleAudioContextFlag : int32_t {
    MUSIC_FLAG = 1, // 0x1
    NAVIGATION_FLAG = 2, // 0x2
    VOICE_COMMAND_FLAG = 4, // 0x4
    CALL_FLAG = 8, // 0x8
    ALARM_FLAG = 16, // 0x10
    NOTIFICATION_FLAG = 32, // 0x20
    UNKNOWN_FLAG = 64, // 0x40
    SAFETY_ALERT_FLAG = 128, // 0x80
    CD_ROM_FLAG = 256, // 0x100
    AUX_AUDIO_FLAG = 512, // 0x200
    SYSTEM_SOUND_FLAG = 1024, // 0x400
    RADIO_FLAG = 2048, // 0x800
    EXT_SOURCE_FLAG = 4096, // 0x1000
};

enum class VehicleAudioVolumeCapabilityFlag : int32_t {
    PERSISTENT_STORAGE = 1, // 0x1
    MASTER_VOLUME_ONLY = 2, // 0x2
};

enum class VehicleAudioVolumeState : int32_t {
    STATE_OK = 0,
    LIMIT_REACHED = 1,
};

enum class VehicleAudioVolumeIndex : int32_t {
    INDEX_STREAM = 0,
    INDEX_VOLUME = 1,
    INDEX_STATE = 2,
};

enum class VehicleAudioVolumeLimitIndex : int32_t {
    STREAM = 0,
    MAX_VOLUME = 1,
};

enum class VehicleAudioRoutingPolicyIndex : int32_t {
    STREAM = 0,
    CONTEXTS = 1,
};

enum class VehicleAudioHwVariantConfigFlag : int32_t {
    INTERNAL_RADIO_FLAG = 1, // 0x1
};

enum class VehicleApPowerStateConfigFlag : int32_t {
    ENABLE_DEEP_SLEEP_FLAG = 1, // 0x1
    CONFIG_SUPPORT_TIMER_POWER_ON_FLAG = 2, // 0x2
};

enum class VehicleApPowerState : int32_t {
    OFF = 0,
    DEEP_SLEEP = 1,
    ON_DISP_OFF = 2,
    ON_FULL = 3,
    SHUTDOWN_PREPARE = 4,
};

enum class VehicleApPowerStateShutdownParam : int32_t {
    SHUTDOWN_IMMEDIATELY = 1,
    CAN_SLEEP = 2,
    SHUTDOWN_ONLY = 3,
};

enum class VehicleApPowerSetState : int32_t {
    BOOT_COMPLETE = 1, // 0x1
    DEEP_SLEEP_ENTRY = 2, // 0x2
    DEEP_SLEEP_EXIT = 3, // 0x3
    SHUTDOWN_POSTPONE = 4, // 0x4
    SHUTDOWN_START = 5, // 0x5
    DISPLAY_OFF = 6, // 0x6
    DISPLAY_ON = 7, // 0x7
};

enum class VehicleApPowerStateIndex : int32_t {
    STATE = 0,
    ADDITIONAL = 1,
};

enum class VehicleApPowerBootupReason : int32_t {
    USER_POWER_ON = 0,
    USER_UNLOCK = 1,
    TIMER = 2,
};

enum class VehicleHwKeyInputAction : int32_t {
    ACTION_DOWN = 0,
    ACTION_UP = 1,
};

enum class VehicleDisplay : int32_t {
    MAIN = 0,
    INSTRUMENT_CLUSTER = 1,
};

enum class VehicleInstrumentClusterType : int32_t {
    NONE = 0,
    HAL_INTERFACE = 1,
    EXTERNAL_DISPLAY = 2,
};

enum class VehicleUnit : int32_t {
    SHOULD_NOT_USE = 0, // 0x000
    METER_PER_SEC = 1, // 0x01
    RPM = 2, // 0x02
    HERTZ = 3, // 0x03
    PERCENTILE = 16, // 0x10
    MILLIMETER = 32, // 0x20
    METER = 33, // 0x21
    KILOMETER = 35, // 0x23
    CELSIUS = 48, // 0x30
    FAHRENHEIT = 49, // 0x31
    KELVIN = 50, // 0x32
    MILLILITER = 64, // 0x40
    NANO_SECS = 80, // 0x50
    SECS = 83, // 0x53
    YEAR = 89, // 0x59
};

enum class VehiclePropertyChangeMode : int32_t {
    STATIC = 0, // 0x00
    ON_CHANGE = 1, // 0x01
    CONTINUOUS = 2, // 0x02
    POLL = 3, // 0x03
    ON_SET = 4, // 0x04
};

enum class VehiclePropertyAccess : int32_t {
    NONE = 0, // 0x00
    READ = 1, // 0x01
    WRITE = 2, // 0x02
    READ_WRITE = 3, // 0x03
};

enum class VehicleDrivingStatus : int32_t {
    UNRESTRICTED = 0, // 0x00
    NO_VIDEO = 1, // 0x01
    NO_KEYBOARD_INPUT = 2, // 0x02
    NO_VOICE_INPUT = 4, // 0x04
    NO_CONFIG = 8, // 0x08
    LIMIT_MESSAGE_LEN = 16, // 0x10
};

enum class VehicleGear : int32_t {
    GEAR_NEUTRAL = 1, // 0x0001
    GEAR_REVERSE = 2, // 0x0002
    GEAR_PARK = 4, // 0x0004
    GEAR_DRIVE = 8, // 0x0008
    GEAR_LOW = 16, // 0x0010
    GEAR_1 = 16, // 0x0010
    GEAR_2 = 32, // 0x0020
    GEAR_3 = 64, // 0x0040
    GEAR_4 = 128, // 0x0080
    GEAR_5 = 256, // 0x0100
    GEAR_6 = 512, // 0x0200
    GEAR_7 = 1024, // 0x0400
    GEAR_8 = 2048, // 0x0800
    GEAR_9 = 4096, // 0x1000
};

enum class VehicleAreaZone : int32_t {
    ROW_1_LEFT = 1, // 0x00000001
    ROW_1_CENTER = 2, // 0x00000002
    ROW_1_RIGHT = 4, // 0x00000004
    ROW_1 = 8, // 0x00000008
    ROW_2_LEFT = 16, // 0x00000010
    ROW_2_CENTER = 32, // 0x00000020
    ROW_2_RIGHT = 64, // 0x00000040
    ROW_2 = 128, // 0x00000080
    ROW_3_LEFT = 256, // 0x00000100
    ROW_3_CENTER = 512, // 0x00000200
    ROW_3_RIGHT = 1024, // 0x00000400
    ROW_3 = 2048, // 0x00000800
    ROW_4_LEFT = 4096, // 0x00001000
    ROW_4_CENTER = 8192, // 0x00002000
    ROW_4_RIGHT = 16384, // 0x00004000
    ROW_4 = 32768, // 0x00008000
    WHOLE_CABIN = -2147483648, // 0x80000000
};

enum class VehicleAreaSeat : int32_t {
    ROW_1_LEFT = 1, // 0x0001
    ROW_1_CENTER = 2, // 0x0002
    ROW_1_RIGHT = 4, // 0x0004
    ROW_2_LEFT = 16, // 0x0010
    ROW_2_CENTER = 32, // 0x0020
    ROW_2_RIGHT = 64, // 0x0040
    ROW_3_LEFT = 256, // 0x0100
    ROW_3_CENTER = 512, // 0x0200
    ROW_3_RIGHT = 1024, // 0x0400
};

enum class VehicleAreaWindow : int32_t {
    FRONT_WINDSHIELD = 1, // 0x0001
    REAR_WINDSHIELD = 2, // 0x0002
    ROOF_TOP = 4, // 0x0004
    ROW_1_LEFT = 16, // 0x0010
    ROW_1_RIGHT = 32, // 0x0020
    ROW_2_LEFT = 256, // 0x0100
    ROW_2_RIGHT = 512, // 0x0200
    ROW_3_LEFT = 4096, // 0x1000
    ROW_3_RIGHT = 8192, // 0x2000
};

enum class VehicleAreaDoor : int32_t {
    ROW_1_LEFT = 1, // 0x00000001
    ROW_1_RIGHT = 4, // 0x00000004
    ROW_2_LEFT = 16, // 0x00000010
    ROW_2_RIGHT = 64, // 0x00000040
    ROW_3_LEFT = 256, // 0x00000100
    ROW_3_RIGHT = 1024, // 0x00000400
    HOOD = 268435456, // 0x10000000
    REAR = 536870912, // 0x20000000
};

enum class VehicleAreaMirror : int32_t {
    DRIVER_LEFT = 1, // 0x00000001
    DRIVER_RIGHT = 2, // 0x00000002
    DRIVER_CENTER = 4, // 0x00000004
};

enum class VehicleTurnSignal : int32_t {
    NONE = 0, // 0x00
    RIGHT = 1, // 0x01
    LEFT = 2, // 0x02
    EMERGENCY = 4, // 0x04
};

enum class VehicleIgnitionState : int32_t {
    UNDEFINED = 0,
    LOCK = 1,
    OFF = 2,
    ACC = 3,
    ON = 4,
    START = 5,
};

enum class VehiclePropertyOperation : int32_t {
    GENERIC = 0,
    SET = 1,
    GET = 2,
    SUBSCRIBE = 3,
};

enum class SubscribeFlags : int32_t {
    UNDEFINED = 0, // 0x0
    HAL_EVENT = 1, // 0x1
    SET_CALL = 2, // 0x2
    DEFAULT = 1, // HAL_EVENT
};

enum class StatusCode : int32_t {
    OK = 0,
    TRY_AGAIN = 1,
    INVALID_ARG = 2,
    NOT_AVAILABLE = 3,
    ACCESS_DENIED = 4,
    INTERNAL_ERROR = 5,
};

enum class FuelSystemStatus : int32_t {
    OPEN_INSUFFICIENT_ENGINE_TEMPERATURE = 1,
    CLOSED_LOOP = 2,
    OPEN_ENGINE_LOAD_OR_DECELERATION = 4,
    OPEN_SYSTEM_FAILURE = 8,
    CLOSED_LOOP_BUT_FEEDBACK_FAULT = 16,
};

enum class IgnitionMonitorKind : int32_t {
    SPARK = 0,
    COMPRESSION = 1,
};

enum class CommonIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
};

enum class SparkIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
    EGR_AVAILABLE = 64, // (0x1 << 6)
    EGR_INCOMPLETE = 128, // (0x1 << 7)
    OXYGEN_SENSOR_HEATER_AVAILABLE = 256, // (0x1 << 8)
    OXYGEN_SENSOR_HEATER_INCOMPLETE = 512, // (0x1 << 9)
    OXYGEN_SENSOR_AVAILABLE = 1024, // (0x1 << 10)
    OXYGEN_SENSOR_INCOMPLETE = 2048, // (0x1 << 11)
    AC_REFRIGERANT_AVAILABLE = 4096, // (0x1 << 12)
    AC_REFRIGERANT_INCOMPLETE = 8192, // (0x1 << 13)
    SECONDARY_AIR_SYSTEM_AVAILABLE = 16384, // (0x1 << 14)
    SECONDARY_AIR_SYSTEM_INCOMPLETE = 32768, // (0x1 << 15)
    EVAPORATIVE_SYSTEM_AVAILABLE = 65536, // (0x1 << 16)
    EVAPORATIVE_SYSTEM_INCOMPLETE = 131072, // (0x1 << 17)
    HEATED_CATALYST_AVAILABLE = 262144, // (0x1 << 18)
    HEATED_CATALYST_INCOMPLETE = 524288, // (0x1 << 19)
    CATALYST_AVAILABLE = 1048576, // (0x1 << 20)
    CATALYST_INCOMPLETE = 2097152, // (0x1 << 21)
};

enum class CompressionIgnitionMonitors : int32_t {
    COMPONENTS_AVAILABLE = 1, // (0x1 << 0)
    COMPONENTS_INCOMPLETE = 2, // (0x1 << 1)
    FUEL_SYSTEM_AVAILABLE = 4, // (0x1 << 2)
    FUEL_SYSTEM_INCOMPLETE = 8, // (0x1 << 3)
    MISFIRE_AVAILABLE = 16, // (0x1 << 4)
    MISFIRE_INCOMPLETE = 32, // (0x1 << 5)
    EGR_OR_VVT_AVAILABLE = 64, // (0x1 << 6)
    EGR_OR_VVT_INCOMPLETE = 128, // (0x1 << 7)
    PM_FILTER_AVAILABLE = 256, // (0x1 << 8)
    PM_FILTER_INCOMPLETE = 512, // (0x1 << 9)
    EXHAUST_GAS_SENSOR_AVAILABLE = 1024, // (0x1 << 10)
    EXHAUST_GAS_SENSOR_INCOMPLETE = 2048, // (0x1 << 11)
    BOOST_PRESSURE_AVAILABLE = 4096, // (0x1 << 12)
    BOOST_PRESSURE_INCOMPLETE = 8192, // (0x1 << 13)
    NOx_SCR__AVAILABLE = 16384, // (0x1 << 14)
    NOx_SCR_INCOMPLETE = 32768, // (0x1 << 15)
    NMHC_CATALYST_AVAILABLE = 65536, // (0x1 << 16)
    NMHC_CATALYST_INCOMPLETE = 131072, // (0x1 << 17)
};

enum class Wheel : int32_t {
    UNKNOWN = 0, // 0x0
    LEFT_FRONT = 1, // 0x1
    RIGHT_FRONT = 2, // 0x2
    LEFT_REAR = 4, // 0x4
    RIGHT_REAR = 8, // 0x8
};

enum class SecondaryAirStatus : int32_t {
    UPSTREAM = 1,
    DOWNSTREAM_OF_CATALYCIC_CONVERTER = 2,
    FROM_OUTSIDE_OR_OFF = 4,
    PUMP_ON_FOR_DIAGNOSTICS = 8,
};

enum class FuelType : int32_t {
    NOT_AVAILABLE = 0,
    GASOLINE = 1,
    METHANOL = 2,
    ETHANOL = 3,
    DIESEL = 4,
    LPG = 5,
    CNG = 6,
    PROPANE = 7,
    ELECTRIC = 8,
    BIFUEL_RUNNING_GASOLINE = 9,
    BIFUEL_RUNNING_METHANOL = 10,
    BIFUEL_RUNNING_ETHANOL = 11,
    BIFUEL_RUNNING_LPG = 12,
    BIFUEL_RUNNING_CNG = 13,
    BIFUEL_RUNNING_PROPANE = 14,
    BIFUEL_RUNNING_ELECTRIC = 15,
    BIFUEL_RUNNING_ELECTRIC_AND_COMBUSTION = 16,
    HYBRID_GASOLINE = 17,
    HYBRID_ETHANOL = 18,
    HYBRID_DIESEL = 19,
    HYBRID_ELECTRIC = 20,
    HYBRID_RUNNING_ELECTRIC_AND_COMBUSTION = 21,
    HYBRID_REGENERATIVE = 22,
    BIFUEL_RUNNING_DIESEL = 23,
};

enum class Obd2IntegerSensorIndex : int32_t {
    FUEL_SYSTEM_STATUS = 0,
    MALFUNCTION_INDICATOR_LIGHT_ON = 1,
    IGNITION_MONITORS_SUPPORTED = 2,
    IGNITION_SPECIFIC_MONITORS = 3,
    INTAKE_AIR_TEMPERATURE = 4,
    COMMANDED_SECONDARY_AIR_STATUS = 5,
    NUM_OXYGEN_SENSORS_PRESENT = 6,
    RUNTIME_SINCE_ENGINE_START = 7,
    DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON = 8,
    WARMUPS_SINCE_CODES_CLEARED = 9,
    DISTANCE_TRAVELED_SINCE_CODES_CLEARED = 10,
    ABSOLUTE_BAROMETRIC_PRESSURE = 11,
    CONTROL_MODULE_VOLTAGE = 12,
    AMBIENT_AIR_TEMPERATURE = 13,
    TIME_WITH_MALFUNCTION_LIGHT_ON = 14,
    TIME_SINCE_TROUBLE_CODES_CLEARED = 15,
    MAX_FUEL_AIR_EQUIVALENCE_RATIO = 16,
    MAX_OXYGEN_SENSOR_VOLTAGE = 17,
    MAX_OXYGEN_SENSOR_CURRENT = 18,
    MAX_INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 19,
    MAX_AIR_FLOW_RATE_FROM_MASS_AIR_FLOW_SENSOR = 20,
    FUEL_TYPE = 21,
    FUEL_RAIL_ABSOLUTE_PRESSURE = 22,
    ENGINE_OIL_TEMPERATURE = 23,
    DRIVER_DEMAND_PERCENT_TORQUE = 24,
    ENGINE_ACTUAL_PERCENT_TORQUE = 25,
    ENGINE_REFERENCE_PERCENT_TORQUE = 26,
    ENGINE_PERCENT_TORQUE_DATA_IDLE = 27,
    ENGINE_PERCENT_TORQUE_DATA_POINT1 = 28,
    ENGINE_PERCENT_TORQUE_DATA_POINT2 = 29,
    ENGINE_PERCENT_TORQUE_DATA_POINT3 = 30,
    ENGINE_PERCENT_TORQUE_DATA_POINT4 = 31,
    LAST_SYSTEM_INDEX = 31, // ENGINE_PERCENT_TORQUE_DATA_POINT4
    VENDOR_START_INDEX = 32, // (LAST_SYSTEM_INDEX + 1)
};

enum class Obd2FloatSensorIndex : int32_t {
    CALCULATED_ENGINE_LOAD = 0,
    ENGINE_COOLANT_TEMPERATURE = 1,
    SHORT_TERM_FUEL_TRIM_BANK1 = 2,
    LONG_TERM_FUEL_TRIM_BANK1 = 3,
    SHORT_TERM_FUEL_TRIM_BANK2 = 4,
    LONG_TERM_FUEL_TRIM_BANK2 = 5,
    FUEL_PRESSURE = 6,
    INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 7,
    ENGINE_RPM = 8,
    VEHICLE_SPEED = 9,
    TIMING_ADVANCE = 10,
    MAF_AIR_FLOW_RATE = 11,
    THROTTLE_POSITION = 12,
    OXYGEN_SENSOR1_VOLTAGE = 13,
    OXYGEN_SENSOR1_SHORT_TERM_FUEL_TRIM = 14,
    OXYGEN_SENSOR1_FUEL_AIR_EQUIVALENCE_RATIO = 15,
    OXYGEN_SENSOR2_VOLTAGE = 16,
    OXYGEN_SENSOR2_SHORT_TERM_FUEL_TRIM = 17,
    OXYGEN_SENSOR2_FUEL_AIR_EQUIVALENCE_RATIO = 18,
    OXYGEN_SENSOR3_VOLTAGE = 19,
    OXYGEN_SENSOR3_SHORT_TERM_FUEL_TRIM = 20,
    OXYGEN_SENSOR3_FUEL_AIR_EQUIVALENCE_RATIO = 21,
    OXYGEN_SENSOR4_VOLTAGE = 22,
    OXYGEN_SENSOR4_SHORT_TERM_FUEL_TRIM = 23,
    OXYGEN_SENSOR4_FUEL_AIR_EQUIVALENCE_RATIO = 24,
    OXYGEN_SENSOR5_VOLTAGE = 25,
    OXYGEN_SENSOR5_SHORT_TERM_FUEL_TRIM = 26,
    OXYGEN_SENSOR5_FUEL_AIR_EQUIVALENCE_RATIO = 27,
    OXYGEN_SENSOR6_VOLTAGE = 28,
    OXYGEN_SENSOR6_SHORT_TERM_FUEL_TRIM = 29,
    OXYGEN_SENSOR6_FUEL_AIR_EQUIVALENCE_RATIO = 30,
    OXYGEN_SENSOR7_VOLTAGE = 31,
    OXYGEN_SENSOR7_SHORT_TERM_FUEL_TRIM = 32,
    OXYGEN_SENSOR7_FUEL_AIR_EQUIVALENCE_RATIO = 33,
    OXYGEN_SENSOR8_VOLTAGE = 34,
    OXYGEN_SENSOR8_SHORT_TERM_FUEL_TRIM = 35,
    OXYGEN_SENSOR8_FUEL_AIR_EQUIVALENCE_RATIO = 36,
    FUEL_RAIL_PRESSURE = 37,
    FUEL_RAIL_GAUGE_PRESSURE = 38,
    COMMANDED_EXHAUST_GAS_RECIRCULATION = 39,
    EXHAUST_GAS_RECIRCULATION_ERROR = 40,
    COMMANDED_EVAPORATIVE_PURGE = 41,
    FUEL_TANK_LEVEL_INPUT = 42,
    EVAPORATION_SYSTEM_VAPOR_PRESSURE = 43,
    CATALYST_TEMPERATURE_BANK1_SENSOR1 = 44,
    CATALYST_TEMPERATURE_BANK2_SENSOR1 = 45,
    CATALYST_TEMPERATURE_BANK1_SENSOR2 = 46,
    CATALYST_TEMPERATURE_BANK2_SENSOR2 = 47,
    ABSOLUTE_LOAD_VALUE = 48,
    FUEL_AIR_COMMANDED_EQUIVALENCE_RATIO = 49,
    RELATIVE_THROTTLE_POSITION = 50,
    ABSOLUTE_THROTTLE_POSITION_B = 51,
    ABSOLUTE_THROTTLE_POSITION_C = 52,
    ACCELERATOR_PEDAL_POSITION_D = 53,
    ACCELERATOR_PEDAL_POSITION_E = 54,
    ACCELERATOR_PEDAL_POSITION_F = 55,
    COMMANDED_THROTTLE_ACTUATOR = 56,
    ETHANOL_FUEL_PERCENTAGE = 57,
    ABSOLUTE_EVAPORATION_SYSTEM_VAPOR_PRESSURE = 58,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 59,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 60,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 61,
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 62,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 63,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 64,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 65,
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 66,
    RELATIVE_ACCELERATOR_PEDAL_POSITION = 67,
    HYBRID_BATTERY_PACK_REMAINING_LIFE = 68,
    FUEL_INJECTION_TIMING = 69,
    ENGINE_FUEL_RATE = 70,
    LAST_SYSTEM_INDEX = 70, // ENGINE_FUEL_RATE
    VENDOR_START_INDEX = 71, // (LAST_SYSTEM_INDEX + 1)
};

enum class VmsMessageType : int32_t {
    SUBSCRIBE = 1,
    UNSUBSCRIBE = 2,
    DATA = 3,
};

enum class VmsMessageIntegerValuesIndex : int32_t {
    VMS_MESSAGE_TYPE = 1,
    VMS_LAYER_ID = 2,
    VMS_LAYER_VERSION = 3,
};

} // namespace emulator
