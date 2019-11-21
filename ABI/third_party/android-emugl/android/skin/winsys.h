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

#pragma once

// Window system related definitions.
#include "android/skin/rect.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
// Init args.
void skin_winsys_init_args(int argc, char** argv);

// Return window handle of main UI. MUST BE CALLED FROM THE MAIN UI THREAD.
void* skin_winsys_get_window_handle();

// Return rectangle of current monitor in pixels.
void skin_winsys_get_monitor_rect(SkinRect* rect);

// Returns the ratio between physical pixels and device-independent pixels for
// the window. Return 0 in case of success, -1 in case of failure.
int skin_winsys_get_device_pixel_ratio(double* dpr);

// Set main window position.
void skin_winsys_set_window_pos(int window_x, int window_y);

// Get main window position.
void skin_winsys_get_window_pos(int* window_x, int* window_y);

// Get the main window's frame's position.
void skin_winsys_get_frame_pos(int* window_x, int* window_y);

// Inform the main window of the subwindow geometry
void skin_winsys_set_device_geometry(const SkinRect* rect);

// Set window title.
void skin_winsys_set_window_title(const char* title);

// Return true iff the main window is fully visible
bool skin_winsys_is_window_fully_visible(void);

enum WinsysPreferredGlesBackend {
    WINSYS_GLESBACKEND_PREFERENCE_AUTO = 0,
    WINSYS_GLESBACKEND_PREFERENCE_ANGLE = 1,
    WINSYS_GLESBACKEND_PREFERENCE_ANGLE9 = 2,
    WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER = 3,
    WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL = 4,
    WINSYS_GLESBACKEND_PREFERENCE_NUM = 5,
};

enum WinsysPreferredGlesApiLevel {
    WINSYS_GLESAPILEVEL_PREFERENCE_GLES20 = 0,
    WINSYS_GLESAPILEVEL_PREFERENCE_MAX = 1,
    WINSYS_GLESAPILEVEL_PREFERENCE_NUM = 2,
};

// Overrides UI setting of gles backend value.
enum WinsysPreferredGlesBackend skin_winsys_override_glesbackend_if_auto(enum WinsysPreferredGlesBackend);
// Returns current preferred gles backend specified through the emulator UI.
enum WinsysPreferredGlesBackend skin_winsys_get_preferred_gles_backend();
// Sets current preferred gles backend.
void skin_winsys_set_preferred_gles_backend(enum WinsysPreferredGlesBackend);
// Returns current preferred gles api level specified through the emulator UI.
enum WinsysPreferredGlesApiLevel skin_winsys_get_preferred_gles_apilevel();

// Start main window support. |no_window| must be true to indicate that
// no window needs to be shown, but that the GUI backend still needs to be
// initialized.
void skin_winsys_start(bool no_window);

// Set the main window's icon.
// |icon_data| is the start of the icon in PNG format.
// |icon_data_size| is its size in bytes.
//
// Note: On Windows, the icon data is ignored, and the icon is directly
//       extracted from the executable.
void skin_winsys_set_window_icon(const unsigned char* icon_data,
                                 size_t icon_data_size);

// Notifies the UI that the orientation has changed.
void skin_winsys_update_rotation(SkinRotation rotation);

// Stop main window and quit program. Must be called from inside event loop.
void skin_winsys_quit_request(void);

// Kill the skin-related objects
void skin_winsys_destroy(void);

typedef void (*StartFunction)(int argc, char** argv);

// Spawn a thread and run the given function in it.
void skin_winsys_spawn_thread(bool no_window,
                              StartFunction f,
                              int argc,
                              char** argv);

// Enter the main event handling loop for the UI subsystem.
void skin_winsys_enter_main_loop(bool no_window);

// Run some UI update in a way that UI subsystem needs
// E.g. for Qt UI, it makes sure the function runs on the main Qt UI thread,
//   as Qt requires any UI interaction to come from there.
typedef void (*SkinGenericFunction)(void* data);
void skin_winsys_run_ui_update(SkinGenericFunction f, void* data);

// Show a blocking error dialog running on the UI thread.
void skin_winsys_error_dialog(const char* message, const char* title);

#ifdef __cplusplus
}
#endif
