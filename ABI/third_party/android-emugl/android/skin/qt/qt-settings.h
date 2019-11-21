/* Copyright (C) 2015-2017 The Android Open Source Project
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

// This file contains values that are used for saving and
// restoring user settings associated with the Qt-based UI.
#pragma once

namespace Ui {
namespace Settings {

constexpr char ORG_NAME[] = "Android Open Source Project";
constexpr char ORG_DOMAIN[] = "android.com";
constexpr char APP_NAME[] = "Emulator";

constexpr char SHOW_ADB_WARNING[] = "showAdbWarning";
constexpr char SHOW_AVD_ARCH_WARNING[] = "showAvdArchWarning";
constexpr char SHOW_GPU_WARNING[] = "showGpuWarning";

// Note: The "set/" here is because these settings are
//       associated with "settings-page", not because
//       we're in "namespace Settings".

constexpr char ADB_PATH[] = "set/adbPath";
constexpr char AUTO_FIND_ADB[] = "set/autoFindAdb";
constexpr char ALWAYS_ON_TOP[] = "set/alwaysOnTop";
constexpr char FORWARD_SHORTCUTS_TO_DEVICE[] = "set/forwardShortcutsToDevice";
constexpr char FRAME_ALWAYS[] = "set/frameAlways";
constexpr char SAVE_PATH[] = "set/savePath";
constexpr char UI_THEME[] = "set/theme";

constexpr char CELLULAR_NETWORK_TYPE[]    = "cell/network_type";
constexpr char CELLULAR_SIGNAL_STRENGTH[] = "cell/signal_strength";
constexpr char CELLULAR_VOICE_STATUS[]    = "cell/voice_status";
constexpr char CELLULAR_DATA_STATUS[]     = "cell/data_status";

constexpr char CRASHREPORT_PREFERENCE[] = "set/crashReportPreference";
enum CRASHREPORT_PREFERENCE_VALUE {
    CRASHREPORT_PREFERENCE_ASK = 0,
    CRASHREPORT_PREFERENCE_ALWAYS = 1,
    CRASHREPORT_PREFERENCE_NEVER = 2
};
constexpr char CRASHREPORT_SAVEPREFERENCE_CHECKED[] = "set/crashReportSavePreferenceChecked";
constexpr int CRASHREPORT_COMBOBOX_ALWAYS = 0;
constexpr int CRASHREPORT_COMBOBOX_NEVER = 1;
constexpr int CRASHREPORT_COMBOBOX_ASK = 2;

constexpr char GLESBACKEND_PREFERENCE[] = "set/glesBackendPreference";
constexpr char GLESAPILEVEL_PREFERENCE[] ="set/glesApiLevelPreference";

constexpr char CLIPBOARD_SHARING[] = "set/clipboardSharing";

constexpr char HTTP_PROXY_USE_STUDIO[] = "set/proxy/useStudio";
constexpr char HTTP_PROXY_TYPE[] = "set/proxy/type";
enum HTTP_PROXY_TYPE {
    HTTP_PROXY_TYPE_NONE = 0,
    HTTP_PROXY_TYPE_MANUAL = 1
    // Future?: HTTP_PROXY_TYPE_AUTO = 2
};
constexpr char HTTP_PROXY_HOST[] = "set/proxy/host";
constexpr char HTTP_PROXY_PORT[] = "set/proxy/port";
constexpr char HTTP_PROXY_AUTHENTICATION[] = "set/proxy/authentication";
constexpr char HTTP_PROXY_USERNAME[] = "set/proxy/username";


constexpr char LOCATION_ENTERED_ALTITUDE[] = "loc/altitude";
constexpr char LOCATION_ENTERED_LATITUDE[] = "loc/latitude";
constexpr char LOCATION_ENTERED_LONGITUDE[] = "loc/longitude";
constexpr char LOCATION_PLAYBACK_FILE[] = "loc/playback_file_path";
constexpr char LOCATION_PLAYBACK_SPEED[] = "loc/playback_speed";
constexpr char LOCATION_RECENT_ALTITUDE[] = "loc/recent_altitude";
constexpr char LOCATION_RECENT_LATITUDE[] = "loc/recent_latitude";
constexpr char LOCATION_RECENT_LONGITUDE[] = "loc/recent_longitude";

}  // namespace Settings
}  // namespace Ui
