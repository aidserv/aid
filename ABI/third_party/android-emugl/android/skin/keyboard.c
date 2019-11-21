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
#include "android/skin/keyboard.h"

#include "android/skin/charmap.h"
#include "android/skin/keycode.h"
#include "android/skin/keycode-buffer.h"
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "android/utils/system.h"
#include "android/utils/utf8_utils.h"

#include <stdio.h>

#define  DEBUG  1

#if DEBUG
#  define  D(...)  VERBOSE_PRINT(keys,__VA_ARGS__)
#else
#  define  D(...)  ((void)0)
#endif

struct SkinKeyboard {
    const SkinCharmap*  charmap;

    SkinRotation        rotation;

    SkinKeycodeBuffer   keycodes[1];
};

#define DEFAULT_ANDROID_CHARMAP  "qwerty2"

static bool skin_key_code_is_arrow(int code) {
    return code == kKeyCodeDpadLeft ||
           code == kKeyCodeDpadRight ||
           code == kKeyCodeDpadUp ||
           code == kKeyCodeDpadDown;
}

static SkinKeyCode
skin_keyboard_key_to_code(SkinKeyboard*  keyboard,
                          int            code,
                          int            mod,
                          int            down)
{
    D("key code=%d mod=%d str=%s",
      code,
      mod,
      skin_key_pair_to_string(code, mod));

    /* first, handle the arrow keys directly */
    if (skin_key_code_is_arrow(code)) {
        code = skin_keycode_rotate(code, -keyboard->rotation);
        D("handling arrow (code=%d mod=%d)", code, mod);
        int  doCapL, doCapR, doAltL, doAltR;

        doCapL = mod & kKeyModLShift;
        doCapR = mod & kKeyModRShift;
        doAltL = mod & kKeyModLAlt;
        doAltR = mod & kKeyModRAlt;

        if (down) {
            if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 1);
            if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 1);
            if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 1);
            if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 1);
        }
        skin_keyboard_add_key_event(keyboard, code, down);

        if (!down) {
            if (doAltR) skin_keyboard_add_key_event(keyboard, kKeyCodeAltRight, 0);
            if (doAltL) skin_keyboard_add_key_event(keyboard, kKeyCodeAltLeft, 0);
            if (doCapR) skin_keyboard_add_key_event(keyboard, kKeyCodeCapRight, 0);
            if (doCapL) skin_keyboard_add_key_event(keyboard, kKeyCodeCapLeft, 0);
        }
        code = 0;
        return code;
    }

    /* special case for keypad keys, ignore them here if numlock is on */
    if ((mod & kKeyModNumLock) != 0) {
        switch ((int)code) {
            case LINUX_KEY_KP0:
            case LINUX_KEY_KP1:
            case LINUX_KEY_KP2:
            case LINUX_KEY_KP3:
            case LINUX_KEY_KP4:
            case LINUX_KEY_KP5:
            case LINUX_KEY_KP6:
            case LINUX_KEY_KP7:
            case LINUX_KEY_KP8:
            case LINUX_KEY_KP9:
            case LINUX_KEY_KPPLUS:
            case LINUX_KEY_KPMINUS:
            case LINUX_KEY_KPASTERISK:
            case LINUX_KEY_KPSLASH:
            case LINUX_KEY_KPEQUAL:
            case LINUX_KEY_KPDOT:
            case LINUX_KEY_KPENTER:
                return 0;
            default:
                ;
        }
    }

    D("could not handle (code=%d, mod=%d, str=%s)", code, mod,
      skin_key_pair_to_string(code,mod));
    return -1;
}

void
skin_keyboard_process_event(SkinKeyboard*  kb, SkinEvent* ev, int  down)
{
    if (ev->type == kEventTextInput) {
        // TODO(digit): For each Unicode value in the input text.
        const uint8_t* text = ev->u.text.text;
        const uint8_t* end = text + sizeof(ev->u.text.text);
        while (text < end && *text) {
            uint32_t codepoint = 0;
            int len = android_utf8_decode(text, end - text, &codepoint);
            if (len < 0) {
                break;
            }
            skin_keyboard_process_unicode_event(kb, codepoint, 1);
            skin_keyboard_process_unicode_event(kb, codepoint, 0);
            text += len;
        }
        skin_keyboard_flush(kb);
    } else if (ev->type == kEventKeyDown || ev->type == kEventKeyUp) {
        int keycode = ev->u.key.keycode;
        int mod = ev->u.key.mod;

        /* first, try the keyboard-mode-independent keys */
        int code = skin_keyboard_key_to_code(kb, keycode, mod, down);
        if (code == 0) {
            return;
        }
        if ((int)code > 0) {
            skin_keyboard_add_key_event(kb, code, down);
            skin_keyboard_flush(kb);
            return;
        }

        code = keycode;

        if (code == kKeyCodeAltLeft  || code == kKeyCodeAltRight ||
            code == kKeyCodeCapLeft  || code == kKeyCodeCapRight ||
            code == kKeyCodeSym) {
            return;
        }

        if (code == KEY_APPSWITCH || code == LINUX_KEY_PLAYPAUSE ||
            code == LINUX_KEY_BACK || code == LINUX_KEY_POWER ||
            code == LINUX_KEY_BACKSPACE || code == LINUX_KEY_SOFT1 ||
            code == LINUX_KEY_CENTER || code == LINUX_KEY_REWIND ||
            code == LINUX_KEY_ENTER || code == LINUX_KEY_VOLUMEDOWN ||
            code == LINUX_KEY_FASTFORWARD || code == LINUX_KEY_VOLUMEUP ||
            code == LINUX_KEY_HOME || code == LINUX_KEY_SLEEP ||
            code == KEY_STEM_1 || code == KEY_STEM_2 ||
            code == KEY_STEM_3 || code == KEY_STEM_PRIMARY) {
            skin_keyboard_add_key_event(kb, code, down);
            skin_keyboard_flush(kb);
            return;
        }
        D("ignoring keycode %d", keycode);
    }
}

void
skin_keyboard_set_rotation( SkinKeyboard*     keyboard,
                            SkinRotation      rotation )
{
    keyboard->rotation = (rotation & 3);
}

void
skin_keyboard_add_key_event( SkinKeyboard*  kb,
                             unsigned       code,
                             unsigned       down )
{
    skin_keycode_buffer_add(kb->keycodes, code, down);
}


void
skin_keyboard_flush( SkinKeyboard*  kb )
{
    skin_keycode_buffer_flush(kb->keycodes);
}


int
skin_keyboard_process_unicode_event( SkinKeyboard*  kb,  unsigned int  unicode, int  down )
{
    return skin_charmap_reverse_map_unicode(kb->charmap, unicode, down,
                                            kb->keycodes);
}

static SkinKeyboard* skin_keyboard_create_from_charmap_name(
        const char* charmap_name,
        SkinRotation dpad_rotation,
        SkinKeyCodeFlushFunc keycode_flush) {
    SkinKeyboard*  kb;

    ANEW0(kb);

    kb->charmap = skin_charmap_get_by_name(charmap_name);
    if (!kb->charmap) {
        // Charmap name was not found. Default to "qwerty2" */
        kb->charmap = skin_charmap_get_by_name(DEFAULT_ANDROID_CHARMAP);
        fprintf(stderr, "### warning, skin requires unknown '%s' charmap, reverting to '%s'\n",
                charmap_name, kb->charmap->name );
    }
    skin_keycode_buffer_init(kb->keycodes, keycode_flush);
    skin_keyboard_set_rotation(kb, dpad_rotation);
    return kb;
}

SkinKeyboard* skin_keyboard_create(const char* kcm_file_path,
                                   SkinRotation dpad_rotation,
                                   SkinKeyCodeFlushFunc keycode_flush) {
    const char* charmap_name = DEFAULT_ANDROID_CHARMAP;
    char        cmap_buff[SKIN_CHARMAP_NAME_SIZE];

    if (kcm_file_path != NULL) {
        kcm_extract_charmap_name(kcm_file_path, cmap_buff, sizeof cmap_buff);
        charmap_name = cmap_buff;
    }
    return skin_keyboard_create_from_charmap_name(charmap_name, dpad_rotation,
                                                  keycode_flush);
}

void
skin_keyboard_free( SkinKeyboard*  keyboard )
{
    if (keyboard) {
        AFREE(keyboard);
    }
}
