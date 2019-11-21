/* Copyright (C) 2014 The Android Open Source Project
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

#include <memory>

// Save the current window position for after the app exit.
// This must be called in the aboutToQuit() signal handle for the
// application window.
extern void skin_winsys_save_window_pos();

// Returns a shared pointer to EmulatorQtWindow instance
// Used for qemu1 workaround for unresolved crash: NSWindow release on OSX
// Can remove after b.android.com/198256 is resolved.
extern std::shared_ptr<void> skin_winsys_get_shared_ptr();
