/* Copyright (C) 2016 The Android Open Source Project
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

#include <windows.h>
#include "android/skin/qt/windows-native-window.h"

int numHeldMouseButtons() {

    // We care about the left and right mouse buttons because these functions
    // query the physical mouse state, but those buttons can be re-mapped in
    // the system virtually.
    int numHeld = 0;
    if (GetKeyState(VK_LBUTTON) & 0x80) {
        numHeld++;
    }
    if (GetKeyState(VK_RBUTTON) & 0x80) {
        numHeld++;
    }
    return numHeld;
}
