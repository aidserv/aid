/* Copyright (C) 2009-2015 The Android Open Source Project
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

#include "android/utils/compiler.h"
#include "android/skin/android_keycodes.h"
#include "android/skin/linux_keycodes.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

/* Define layout-independent key codes. Think of it as "the user pressed
 * the key that corresponds to Q on a US QWERTY keyboard".
 *
 * The values here must match the Linux keycodes since they are sent
 * to the guest kernel directly.
 */

/* Keep it consistent with linux/input.h */
typedef enum {
    kKeyCodeSoftLeft = LINUX_KEY_SOFT1,
    kKeyCodeSoftRight = LINUX_KEY_SOFT2,
    kKeyCodeHome = LINUX_KEY_HOME,
    kKeyCodeHomePage = LINUX_KEY_HOMEPAGE,
    kKeyCodeBack = LINUX_KEY_BACK,
    kKeyCodeCall = LINUX_KEY_SEND,
    kKeyCodeEndCall = LINUX_KEY_END,
    kKeyCode0 = LINUX_KEY_0,
    kKeyCode1 = LINUX_KEY_1,
    kKeyCode2 = LINUX_KEY_2,
    kKeyCode3 = LINUX_KEY_3,
    kKeyCode4 = LINUX_KEY_4,
    kKeyCode5 = LINUX_KEY_5,
    kKeyCode6 = LINUX_KEY_6,
    kKeyCode7 = LINUX_KEY_7,
    kKeyCode8 = LINUX_KEY_8,
    kKeyCode9 = LINUX_KEY_9,
    kKeyCodeStar = LINUX_KEY_STAR,
    kKeyCodePound = LINUX_KEY_SHARP,
    kKeyCodeDpadUp = LINUX_KEY_UP,
    kKeyCodeDpadDown = LINUX_KEY_DOWN,
    kKeyCodeDpadLeft = LINUX_KEY_LEFT,
    kKeyCodeDpadRight = LINUX_KEY_RIGHT,
    kKeyCodeDpadCenter = LINUX_KEY_CENTER,
    kKeyCodeVolumeUp = LINUX_KEY_VOLUMEUP,
    kKeyCodeVolumeDown = LINUX_KEY_VOLUMEDOWN,
    kKeyCodePower = LINUX_KEY_POWER,
    kKeyCodeCamera = LINUX_KEY_CAMERA,
    kKeyCodeClear = LINUX_KEY_CLEAR,
    kKeyCodeA = LINUX_KEY_A,
    kKeyCodeB = LINUX_KEY_B,
    kKeyCodeC = LINUX_KEY_C,
    kKeyCodeD = LINUX_KEY_D,
    kKeyCodeE = LINUX_KEY_E,
    kKeyCodeF = LINUX_KEY_F,
    kKeyCodeG = LINUX_KEY_G,
    kKeyCodeH = LINUX_KEY_H,
    kKeyCodeI = LINUX_KEY_I,
    kKeyCodeJ = LINUX_KEY_J,
    kKeyCodeK = LINUX_KEY_K,
    kKeyCodeL = LINUX_KEY_L,
    kKeyCodeM = LINUX_KEY_M,
    kKeyCodeN = LINUX_KEY_N,
    kKeyCodeO = LINUX_KEY_O,
    kKeyCodeP = LINUX_KEY_P,
    kKeyCodeQ = LINUX_KEY_Q,
    kKeyCodeR = LINUX_KEY_R,
    kKeyCodeS = LINUX_KEY_S,
    kKeyCodeT = LINUX_KEY_T,
    kKeyCodeU = LINUX_KEY_U,
    kKeyCodeV = LINUX_KEY_V,
    kKeyCodeW = LINUX_KEY_W,
    kKeyCodeX = LINUX_KEY_X,
    kKeyCodeY = LINUX_KEY_Y,
    kKeyCodeZ = LINUX_KEY_Z,

    kKeyCodeComma = LINUX_KEY_COMMA,
    kKeyCodePeriod = LINUX_KEY_DOT,
    kKeyCodeAltLeft = LINUX_KEY_LEFTALT,
    kKeyCodeAltRight = LINUX_KEY_RIGHTALT,
    kKeyCodeCapLeft = LINUX_KEY_LEFTSHIFT,
    kKeyCodeCapRight = LINUX_KEY_RIGHTSHIFT,
    kKeyCodeTab = LINUX_KEY_TAB,
    kKeyCodeSpace = LINUX_KEY_SPACE,
    kKeyCodeSym = LINUX_KEY_COMPOSE,
    kKeyCodeExplorer = LINUX_KEY_WWW,
    kKeyCodeEnvelope = LINUX_KEY_MAIL,
    kKeyCodeNewline = LINUX_KEY_ENTER,
    kKeyCodeDel = LINUX_KEY_BACKSPACE,
    kKeyCodeGrave = 399,
    kKeyCodeMinus = LINUX_KEY_MINUS,
    kKeyCodeEquals = LINUX_KEY_EQUAL,
    kKeyCodeLeftBracket = LINUX_KEY_LEFTBRACE,
    kKeyCodeRightBracket = LINUX_KEY_RIGHTBRACE,
    kKeyCodeBackslash = LINUX_KEY_BACKSLASH,
    kKeyCodeSemicolon = LINUX_KEY_SEMICOLON,
    kKeyCodeApostrophe = LINUX_KEY_APOSTROPHE,
    kKeyCodeSlash = LINUX_KEY_SLASH,
    kKeyCodeAt = LINUX_KEY_EMAIL,
    kKeyCodeNum = LINUX_KEY_NUM,
    kKeyCodeHeadsetHook = LINUX_KEY_HEADSETHOOK,
    kKeyCodeFocus = LINUX_KEY_FOCUS,
    kKeyCodePlus = LINUX_KEY_PLUS,
    kKeyCodeMenu = LINUX_KEY_SOFT1,
    kKeyCodeNotification = LINUX_KEY_NOTIFICATION,
    kKeyCodeSearch = LINUX_KEY_SEARCH,
    kKeyCodeTV = LINUX_KEY_TV,
    kKeyCodeEPG = LINUX_KEY_PROGRAM,
    kKeyCodeDVR = LINUX_KEY_PVR,
    kKeyCodePrevious = LINUX_KEY_PREVIOUS,
    kKeyCodeNext = LINUX_KEY_NEXT,
    kKeyCodePlay = LINUX_KEY_PLAY,
    kKeyCodePlaypause = LINUX_KEY_PLAYPAUSE,
    kKeyCodePause = LINUX_KEY_PAUSE,
    kKeyCodeStop = LINUX_KEY_STOP,
    kKeyCodeRewind = LINUX_KEY_REWIND,
    kKeyCodeFastForward = LINUX_KEY_FASTFORWARD,
    kKeyCodeBookmarks = LINUX_KEY_BOOKMARKS,
    kKeyCodeCycleWindows = LINUX_KEY_CYCLEWINDOWS,
    kKeyCodeChannelUp = LINUX_KEY_CHANNELUP,
    kKeyCodeChannelDown = LINUX_KEY_CHANNELDOWN,
    kKeyCodeSleep = LINUX_KEY_SLEEP,
    kKeyCodeStemPrimary = KEY_STEM_PRIMARY,
    kKeyCodeStem1 = KEY_STEM_1,
    kKeyCodeStem2 = KEY_STEM_2,
    kKeyCodeStem3 = KEY_STEM_3,
    kKeyCodeAppSwitch = KEY_APPSWITCH,
} SkinKeyCode;

/* This function is used to rotate D-Pad keycodes, while leaving other ones
 * untouched. 'code' is the input keycode, which will be returned as is if
 * it doesn't correspond to a D-Pad arrow. 'rotation' is the number of
 * *clockwise* 90 degrees rotations to perform on D-Pad keys.
 *
 * Here are examples:
 *    skin_keycode_rotate( kKeyCodeDpadUp, 1 )  => kKeyCodeDpadRight
 *    skin_keycode_rotate( kKeyCodeDpadRight, 2 ) => kKeyCodeDpadLeft
 *    skin_keycode_rotate( kKeyCodeDpadDown, 3 ) => kKeyCodeDpadRight
 *    skin_keycode_rotate( kKeyCodeSpace, n ) => kKeyCodeSpace independent of n
 */
extern SkinKeyCode skin_keycode_rotate( SkinKeyCode  code, int  rotation );

extern int skin_key_code_count(void);
extern const char* skin_key_code_str(int index);


// Bit flag for key modifiers like Ctrl or Alt.
typedef enum {
    kKeyModLCtrl  = (1U << 0),  // left-control key
    kKeyModRCtrl  = (1U << 1),  // right-control
    kKeyModLAlt   = (1U << 2),  // left-alt
    kKeyModRAlt   = (1U << 3),  // right-alt
    kKeyModLShift = (1U << 4),  // left-shift
    kKeyModRShift = (1U << 5),  // right-shift
    kKeyModNumLock = (1U << 6),  // numlock

} SkinKeyMod;

// Convert a pair of (SkinKeyCode,SkinKeyMod) values into a human-readable
// string. Returns the address of a static string that is overwritten on
// each call.
// |keycode| is the layout-independent SkinKeyCode.
// |mod| is a set of SkinKeyMod bit flags.
extern const char* skin_key_pair_to_string(uint32_t keycode, uint32_t mod);

// Convert a textual key description |str| (e.g. "Ctrl-K") into a pair
// if (SkinKeyCode,SkinKeyMod) values. On success, return true and sets
// |*keycode| and |*mod|. On failure, return false.
extern bool skin_key_pair_from_string(const char* str,
                                      uint32_t* keycode,
                                      uint32_t* mod);

ANDROID_END_HEADER
