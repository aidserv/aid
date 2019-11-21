/* Copyright (C) 2007-2015 The Android Open Source Project
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
#include "android/globals.h"

#include "android/skin/file.h"

#include "android/skin/keycode.h"
#include "android/utils/path.h"
#include "android/utils/bufprint.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"

#include <stdlib.h>

/** UTILITY ROUTINES
 **/
static SkinImage*
skin_image_find_in( const char*  dirname, const char*  filename )
{
    char   buffer[1024];
    char*  p   = buffer;
    char*  end = p + sizeof(buffer);

    p = bufprint( p, end, "%s" PATH_SEP "%s", dirname, filename );
    if (p >= end)
        return SKIN_IMAGE_NONE;

    return skin_image_find_simple(buffer);
}

/** SKIN BACKGROUND
 **/

static void
skin_background_done( SkinBackground*  background )
{
    if (background->image)
        skin_image_unref(&background->image);
}

static int skin_background_init_from(SkinBackground* background,
                                     const AConfig* node,
                                     const char* basepath) {
    const char* img = aconfig_str(node, "image", NULL);
    int         x   = aconfig_int(node, "x", 0);
    int         y   = aconfig_int(node, "y", 0);

    background->valid = 0;

    if (img == NULL)   /* no background */
        return -1;

    background->image = skin_image_find_in( basepath, img );
    if (background->image == SKIN_IMAGE_NONE) {
        background->image = NULL;
        return -1;
    }

    background->rect.pos.x  = x;
    background->rect.pos.y  = y;
    background->rect.size.w = skin_image_w( background->image );
    background->rect.size.h = skin_image_h( background->image );

    background->valid = 1;

    return 0;
}

/** SKIN DISPLAY
 **/

static void
skin_display_done(SkinDisplay*  display)
{
    if (display->framebuffer_funcs && display->owns_framebuffer) {
        display->framebuffer_funcs->free_framebuffer(display->framebuffer);
    }
}

static int skin_display_init_from(SkinDisplay* display,
                                  const AConfig* node,
                                  const SkinFramebufferFuncs* fb_funcs) {
    display->rect.pos.x  = aconfig_int(node, "x", 0);
    display->rect.pos.y  = aconfig_int(node, "y", 0);
    display->rect.size.w = aconfig_int(node, "width", 0);
    display->rect.size.h = aconfig_int(node, "height", 0);
    display->rotation    = aconfig_unsigned(node, "rotation", SKIN_ROTATION_0);
    display->bpp         = aconfig_int(node, "bpp", android_hw->hw_lcd_depth);
    display->owns_framebuffer = 1;

    display->valid = ( display->rect.size.w > 0 && display->rect.size.h > 0 );
    display->framebuffer_funcs = fb_funcs;
    if (display->valid && fb_funcs) {
        SkinRect  r;
        skin_rect_rotate( &r, &display->rect, -display->rotation );
        display->framebuffer = fb_funcs->create_framebuffer(
                r.size.w,
                r.size.h,
                display->bpp);
#if 0
        qframebuffer_init( display->qfbuff,
                           r.size.w,
                           r.size.h,
                           0,
                           display->bpp == 32 ? QFRAME_BUFFER_RGBX_8888
                                              : QFRAME_BUFFER_RGB565 );

        qframebuffer_fifo_add( display->qfbuff );
#endif
    }
    return display->valid ? 0 : -1;
}

/** SKIN BUTTON
 **/


static void
skin_button_free( SkinButton*  button )
{
    if (button) {
        skin_image_unref( &button->image );
        AFREE(button);
    }
}

static unsigned translate_button_name(const char* name) {
    typedef struct {
        const char*     name;
        SkinKeyCode  code;
    } KeyInfo;

    static const KeyInfo keyinfo_table[] = {
        { "dpad-up",      kKeyCodeDpadUp },
        { "dpad-down",    kKeyCodeDpadDown },
        { "dpad-left",    kKeyCodeDpadLeft },
        { "dpad-right",   kKeyCodeDpadRight },
        { "dpad-center",  kKeyCodeDpadCenter },
        { "soft-left",    kKeyCodeSoftLeft },
        { "soft-right",   kKeyCodeSoftRight },
        { "search",       kKeyCodeSearch },
        { "camera",       kKeyCodeCamera },
        { "volume-up",    kKeyCodeVolumeUp },
        { "volume-down",  kKeyCodeVolumeDown },
        { "power",        kKeyCodePower },
        { "home",         kKeyCodeHome },
        { "homepage",     kKeyCodeHomePage },
        { "back",         kKeyCodeBack },
        { "del",          kKeyCodeDel },
        { "0",            kKeyCode0 },
        { "1",            kKeyCode1 },
        { "2",            kKeyCode2 },
        { "3",            kKeyCode3 },
        { "4",            kKeyCode4 },
        { "5",            kKeyCode5 },
        { "6",            kKeyCode6 },
        { "7",            kKeyCode7 },
        { "8",            kKeyCode8 },
        { "9",            kKeyCode9 },
        { "star",         kKeyCodeStar },
        { "pound",        kKeyCodePound },
        { "phone-dial",   kKeyCodeCall },
        { "phone-hangup", kKeyCodeEndCall },
        { "q",            kKeyCodeQ },
        { "w",            kKeyCodeW },
        { "e",            kKeyCodeE },
        { "r",            kKeyCodeR },
        { "t",            kKeyCodeT },
        { "y",            kKeyCodeY },
        { "u",            kKeyCodeU },
        { "i",            kKeyCodeI },
        { "o",            kKeyCodeO },
        { "p",            kKeyCodeP },
        { "a",            kKeyCodeA },
        { "s",            kKeyCodeS },
        { "d",            kKeyCodeD },
        { "f",            kKeyCodeF },
        { "g",            kKeyCodeG },
        { "h",            kKeyCodeH },
        { "j",            kKeyCodeJ },
        { "k",            kKeyCodeK },
        { "l",            kKeyCodeL },
        { "DEL",          kKeyCodeDel },
        { "z",            kKeyCodeZ },
        { "x",            kKeyCodeX },
        { "c",            kKeyCodeC },
        { "v",            kKeyCodeV },
        { "b",            kKeyCodeB },
        { "n",            kKeyCodeN },
        { "m",            kKeyCodeM },
        { "COMMA",        kKeyCodeComma },
        { "PERIOD",       kKeyCodePeriod },
        { "ENTER",        kKeyCodeNewline },
        { "AT",           kKeyCodeAt },
        { "SPACE",        kKeyCodeSpace },
        { "SLASH",        kKeyCodeSlash },
        { "CAP",          kKeyCodeCapLeft },
        { "SYM",          kKeyCodeSym },
        { "ALT",          kKeyCodeAltLeft },
        { "ALT2",         kKeyCodeAltRight },
        { "CAP2",         kKeyCodeCapRight },
        { "tv",           kKeyCodeTV },
        { "epg",          kKeyCodeEPG },
        { "dvr",          kKeyCodeDVR },
        { "prev",         kKeyCodePrevious },
        { "next",         kKeyCodeNext },
        { "play",         kKeyCodePlay },
        { "playpause",    kKeyCodePlaypause },
        { "pause",        kKeyCodePause },
        { "stop",         kKeyCodeStop },
        { "rev",          kKeyCodeRewind },
        { "ffwd",         kKeyCodeFastForward },
        { "bookmarks",    kKeyCodeBookmarks },
        { "window",       kKeyCodeCycleWindows },
        { "channel-up",   kKeyCodeChannelUp },
        { "channel-down", kKeyCodeChannelDown },
        { "app-switch",   kKeyCodeAppSwitch },
        { "sleep",        kKeyCodeSleep },
        { "stem-primary", kKeyCodeStemPrimary },
        { "stem1",        kKeyCodeStem1 },
        { "stem2",        kKeyCodeStem2 },
        { "stem3",        kKeyCodeStem3 },
        { 0, 0 },
    };

    const KeyInfo *ki = keyinfo_table;
    while(ki->name) {
        if(!strcmp(name, ki->name))
            return ki->code;
        ki++;
    }
    return 0;
}

static SkinButton* skin_button_create_from(const AConfig* node,
                                           const char* basepath) {
    SkinButton*  button;
    ANEW0(button);
    if (button) {
        const char*  img = aconfig_str(node, "image", NULL);
        int          x   = aconfig_int(node, "x", 0);
        int          y   = aconfig_int(node, "y", 0);

        button->name       = node->name;
        button->rect.pos.x = x;
        button->rect.pos.y = y;

        if (img != NULL)
            button->image = skin_image_find_in( basepath, img );

        if (button->image == SKIN_IMAGE_NONE) {
            skin_button_free(button);
            return NULL;
        }

        button->rect.size.w = skin_image_w( button->image );
        button->rect.size.h = skin_image_h( button->image );

        button->keycode = translate_button_name(button->name);
        if (button->keycode == 0) {
            dprint("Warning: skin file button uses unknown key name '%s'",
                   button->name);
        }
    }
    return button;
}

/** SKIN PART
 **/

static void
skin_part_free( SkinPart*  part )
{
    if (part) {
        skin_background_done( part->background );
        skin_display_done( part->display );

        SKIN_PART_LOOP_BUTTONS(part,button)
            skin_button_free(button);
        SKIN_PART_LOOP_END
        part->buttons = NULL;
        AFREE(part);
    }
}

static SkinLocation* skin_location_create_from_v2(const AConfig* node,
                                                  SkinPart* parts) {
    const char*    partname = aconfig_str(node, "name", NULL);
    int            x        = aconfig_int(node, "x", 0);
    int            y        = aconfig_int(node, "y", 0);
    SkinRotation   rot      = aconfig_int(node, "rotation", SKIN_ROTATION_0);
    SkinPart*      part;
    SkinLocation*  location;

    if (partname == NULL) {
        dprint( "### WARNING: ignoring part location without 'name' element" );
        return NULL;
    }

    for (part = parts; part; part = part->next)
        if (!strcmp(part->name, partname))
            break;

    if (part == NULL) {
        dprint( "### WARNING: ignoring part location with unknown name '%s'", partname );
        return NULL;
    }

    ANEW0(location);
    location->part     = part;
    location->anchor.x = x;
    location->anchor.y = y;
    location->rotation = rot;

    return location;
}

static SkinPart* skin_part_create_from_v1(
        const AConfig* root,
        const char* basepath,
        const SkinFramebufferFuncs* fb_funcs) {
    SkinPart* part;
    const AConfig* node;
    SkinBox box;

    ANEW0(part);
    part->name = root->name;

    node = aconfig_find_const(root, "background");
    if (node)
        skin_background_init_from(part->background, node, basepath);

    node = aconfig_find_const(root, "display");
    if (node)
        skin_display_init_from(part->display, node, fb_funcs);

    node = aconfig_find_const(root, "button");
    if (node) {
        for (node = node->first_child; node != NULL; node = node->next)
        {
            SkinButton*  button = skin_button_create_from(
                    node, basepath);

            if (button != NULL) {
                button->next  = part->buttons;
                part->buttons = button;
            }
        }
    }

    skin_box_minmax_init( &box );

    if (part->background->valid)
        skin_box_minmax_update( &box, &part->background->rect );

    if (part->display->valid)
        skin_box_minmax_update( &box, &part->display->rect );

    SKIN_PART_LOOP_BUTTONS(part, button)
        skin_box_minmax_update( &box, &button->rect );
    SKIN_PART_LOOP_END

    if ( !skin_box_minmax_to_rect( &box, &part->rect ) ) {
        skin_part_free(part);
        part = NULL;
    }

    return part;
}

static SkinPart* skin_part_create_from_v2(
        const AConfig* root,
        const char* basepath,
        const SkinFramebufferFuncs* fb_funcs) {
    SkinPart* part;
    const AConfig* node;
    SkinBox box;

    ANEW0(part);
    part->name = root->name;

    node = aconfig_find_const(root, "background");
    if (node)
        skin_background_init_from(part->background, node, basepath);

    node = aconfig_find_const(root, "display");
    if (node)
        skin_display_init_from(part->display, node, fb_funcs);

    node = aconfig_find_const(root, "buttons");
    if (node) {
        for (node = node->first_child; node != NULL; node = node->next)
        {
            SkinButton*  button = skin_button_create_from(
                    node, basepath);

            if (button != NULL) {
                button->next  = part->buttons;
                part->buttons = button;
            }
        }
    }

    skin_box_minmax_init( &box );

    if (part->background->valid)
        skin_box_minmax_update( &box, &part->background->rect );

    if (part->display->valid)
        skin_box_minmax_update( &box, &part->display->rect );

    SKIN_PART_LOOP_BUTTONS(part, button)
        skin_box_minmax_update( &box, &button->rect );
    SKIN_PART_LOOP_END

    if ( !skin_box_minmax_to_rect( &box, &part->rect ) ) {
        skin_part_free(part);
        part = NULL;
    }
    return part;
}

/** SKIN LAYOUT
 **/

static void
skin_layout_free( SkinLayout*  layout )
{
    if (layout) {
        SKIN_LAYOUT_LOOP_LOCS(layout,loc)
            AFREE(loc);
        SKIN_LAYOUT_LOOP_END
        layout->locations = NULL;
        if (layout->onion_image) {
          skin_image_unref( &layout->onion_image );
        }
        AFREE(layout);
    }
}

SkinDisplay*
skin_layout_get_display( SkinLayout*  layout )
{
    SKIN_LAYOUT_LOOP_LOCS(layout,loc)
        SkinPart*  part = loc->part;
        if (part->display->valid) {
            return part->display;
        }
    SKIN_LAYOUT_LOOP_END
    return NULL;
}

SkinRotation
skin_layout_get_dpad_rotation(SkinLayout* layout)
{
    if (layout->has_dpad_rotation)
        return layout->dpad_rotation;

    SKIN_LAYOUT_LOOP_LOCS(layout, loc)
        SkinPart*  part = loc->part;
        SKIN_PART_LOOP_BUTTONS(part,button)
            if (button->keycode == kKeyCodeDpadUp)
                return loc->rotation;
        SKIN_PART_LOOP_END
    SKIN_LAYOUT_LOOP_END

    return SKIN_ROTATION_0;
}


static int
skin_layout_event_decode( const char*  event, int  *ptype, int  *pcode, int *pvalue )
{
    typedef struct {
        const char*  name;
        int          value;
    } EventName;

    static const EventName  _event_names[] = {
        { "EV_SW", 0x05 },
        { NULL, 0 },
    };

    const char*       x = strchr(event, ':');
    const char*       y = NULL;
    const EventName*  ev = _event_names;

    if (x != NULL)
        y = strchr(x+1, ':');

    if (x == NULL || y == NULL) {
        dprint( "### WARNING: invalid skin layout event format: '%s', should be '<TYPE>:<CODE>:<VALUE>'", event );
        return -1;
    }

    for ( ; ev->name != NULL; ev++ )
        if (!memcmp( event, ev->name, x - event ) && ev->name[x-event] == 0)
            break;

    if (!ev->name) {
        dprint( "### WARNING: unrecognized skin layout event name: %.*s", x-event, event );
        return -1;
    }

    *ptype  = ev->value;
    *pcode  = strtol(x+1, NULL, 0);
    *pvalue = strtol(y+1, NULL, 0);
    return 0;
}

static SkinLayout* skin_layout_create_from_v2(const AConfig* root,
                                              SkinPart* parts,
                                              const char* basepath) {
    SkinLayout*    layout;
    int            width, height;
    SkinLocation** ptail;
    const AConfig* node;

    ANEW0(layout);

    width  = aconfig_int( root, "width", 400 );
    height = aconfig_int( root, "height", 400 );

    node = aconfig_find_const(root, "event");
    if (node != NULL) {
        skin_layout_event_decode( node->value,
                                  &layout->event_type,
                                  &layout->event_code,
                                  &layout->event_value );
    } else {
        layout->event_type  = 0x05;  /* close keyboard by default */
        layout->event_code  = 0;
        layout->event_value = 1;
    }

    layout->name  = root->name;
    layout->color = aconfig_unsigned( root, "color", 0x808080 ) | 0xff000000;
    ptail         = &layout->locations;

    node = aconfig_find_const(root, "dpad-rotation");
    if (node != NULL) {
        layout->dpad_rotation     = aconfig_int( root, "dpad-rotation", 0 );
        layout->has_dpad_rotation = 1;
    }

    node = aconfig_find_const(root, "onion");
    if (node != NULL) {
        const char* img = aconfig_str(node, "image", NULL);
        layout->onion_image = skin_image_find_in( basepath, img );
        if (layout->onion_image == SKIN_IMAGE_NONE) {
            layout->onion_image = NULL;
        }
        // In layout file, alpha is specified in range 0-100. Convert to
        // internal range 0-256 with default=128.
        int alpha = aconfig_int( node, "alpha", 50 );
        layout->onion_alpha = (256*alpha)/100;
        layout->onion_rotation = aconfig_int( node, "rotation", 0 );
    }

    for (node = root->first_child; node; node = node->next)
    {
        if (!memcmp(node->name, "part", 4)) {
            SkinLocation*  location = skin_location_create_from_v2( node, parts );
            if (location == NULL) {
                continue;
            }
            *ptail = location;
            ptail  = &location->next;
        }
    }

    if (layout->locations == NULL)
        goto Fail;

    layout->size.w = width;
    layout->size.h = height;

    return layout;

Fail:
    skin_layout_free(layout);
    return NULL;
}

// Rotates a given rect that is inside another rect around the center of its parent
// 'dst' is a pointer to the location where the result will be stored
// 'src' is a pointer to the rect that will be rotated and can be the same as 'dst'
// 'parent_size' is the width and height of src's parent rect
// 'by' is the amount of rotation, in 90-degree increments
static void
skin_rect_rotate_in_rect(SkinRect* dst, const SkinRect* src, const SkinSize* parent_size, SkinRotation by) {
    int x = src->pos.x;
    int y = src->pos.y;

    skin_size_rotate(&(dst->size), &(src->size), by);

    switch(by) {
    case SKIN_ROTATION_90:
        dst->pos.x = parent_size->w - y - dst->size.w;
        dst->pos.y = x;
        break;

    case SKIN_ROTATION_180:
        dst->pos.x = parent_size->w - x - dst->size.w;
        dst->pos.y = parent_size->h - y - dst->size.h;
        break;

    case SKIN_ROTATION_270:
        dst->pos.x = y;
        dst->pos.y = parent_size->h - x - dst->size.h;
        break;

    case SKIN_ROTATION_0:
    default:
        break;
    }
}

// Creates a version of the given button rotated around the center of the given part.
// 'src' is the button that needs rotation
// 'part_size' is the width and height of the part containing the button
// 'by' is the amount of rotation in 90-degree increments
static SkinButton*
skin_button_create_rotated(const SkinButton* src, const SkinSize* part_size, SkinRotation by) {
    SkinButton* new_button;
    ANEW0(new_button);
    *new_button = *src;
    new_button->next = NULL;
    skin_rect_rotate_in_rect(&(new_button->rect), &(src->rect), part_size, by);
    new_button->image = skin_image_clone_rotated(src->image, by);
    if (new_button->image == SKIN_IMAGE_NONE) {
        goto Fail;
    }
    return new_button;

Fail:
    skin_button_free(new_button);
    return NULL;
}

// Creates a version of the given skin part that is rotated around the center of the given
// layout.
// 'src' is the part that needs rotation
// 'layout_size' is the width and height of the parent layout
// 'by' is the amount of rotation in 90-degree increments
static SkinPart*
skin_part_create_rotated(const SkinPart* src, const SkinSize* layout_size, SkinRotation by) {
    SkinPart* new_part;
    ANEW0(new_part);

    *new_part = *src;
    new_part->next = NULL;
    new_part->buttons = NULL;

    // Rotate the background image
    if (new_part->background->image &&
        new_part->background->image != SKIN_IMAGE_NONE) {
        new_part->background->image =
            skin_image_clone_rotated(src->background->image, by);
        skin_size_rotate(&(new_part->background->rect.size), &(src->background->rect.size), by);
        if (new_part->background->image == SKIN_IMAGE_NONE) {
            goto Fail;
        }
    }

    // If the part has a display, rotate that too
    if (new_part->display->valid) {
        skin_size_rotate(&(new_part->display->rect.size), &(src->display->rect.size), by);
        new_part->display->rotation = skin_rotation_rotate(new_part->display->rotation, by);

        // Rotated part shares framebuffer with the original.
        new_part->display->owns_framebuffer = 0;
    }

    // Rotate each of the part's buttons so that they align with the rotated
    // background properly.
    SkinButton** new_btn = &(new_part->buttons);
    SKIN_PART_LOOP_BUTTONS(src, btn)
        *new_btn = skin_button_create_rotated(btn, &(new_part->background->rect.size), by);
        if (*new_btn == NULL) {
            goto Fail;
        }
        new_btn = &((*new_btn)->next);
    SKIN_PART_LOOP_END

    return new_part;

Fail:
    skin_part_free(new_part);
    return NULL;
}

// Rotates a given SkinLocation within its parent layout. It will create a rotated copy of
// the skin part that the location refernces.
static SkinLocation*
skin_location_create_rotated(const SkinLocation* src, const SkinSize* layout_size, SkinRotation by)
{
    SkinLocation* new_location;
    ANEW0(new_location);

    *new_location = *src;
    new_location->next = NULL;
    new_location->part = skin_part_create_rotated(src->part, layout_size, by);
    if (new_location->part == NULL) {
        goto Fail;
    }

    SkinRect r;
    r.pos = src->anchor;
    r.size = new_location->part->display->valid ?
        src->part->display->rect.size :
        src->part->background->rect.size;

    skin_rect_rotate_in_rect(&r, &r, layout_size, by);
    new_location->anchor.x = r.pos.x;
    new_location->anchor.y = r.pos.y;
    return new_location;

Fail:
    AFREE(new_location);
    return NULL;
}

// Create a new layout which is the same as src, but rotated by a given amount.
// This will also create rotated clones of all the parts used by the source layout.
// 'src' is the source layout.
// 'new_parts_ptr' is the pointer to the location where the head of the linked list
//                 of the new parts should be stored.
// 'by' is the amount of rotation to apply, in 90-degree increments.
static SkinLayout*
skin_layout_create_rotated(const SkinLayout* src, SkinPart** new_parts_ptr, SkinRotation by)
{
    SkinLayout* new_layout;
    ANEW0(new_layout);

    *new_layout = *src;

    new_layout->next = NULL;
    new_layout->orientation = skin_rotation_rotate(new_layout->orientation, by);

    // DPad rotation is needed to ensure the arrow keys work correctly, even if
    // the device isn't configured for a DPad
    new_layout->has_dpad_rotation = true;
    new_layout->dpad_rotation = by;
    skin_size_rotate(&(new_layout->size), &(src->size), by);

    new_layout->locations = NULL;
    SkinLocation** current_loc_ptr = &(new_layout->locations);
    SkinPart* new_parts = *new_parts_ptr; // A linked list of newly generated, rotated parts.

    SKIN_LAYOUT_LOOP_LOCS(src, loc)
        *current_loc_ptr = skin_location_create_rotated(loc, &(new_layout->size), by);
        if (current_loc_ptr == NULL) {
            goto Fail;
        }
        // Prepend the part generated for this location to the linked list of new parts
        (*current_loc_ptr)->part->next = new_parts;
        new_parts = (*current_loc_ptr)->part;

        (*current_loc_ptr)->next = NULL;
        current_loc_ptr = &((*current_loc_ptr)->next);
    SKIN_LAYOUT_LOOP_END

    // Rotate the onion image if there is one.
    if (src->onion_image && src->onion_image != SKIN_IMAGE_NONE) {
        new_layout->onion_rotation =
            skin_rotation_rotate(src->onion_rotation, by);
        new_layout->onion_image = skin_image_clone_rotated(src->onion_image, by);
        if (new_layout->onion_image == SKIN_IMAGE_NONE) {
            goto Fail;
        }
        new_layout->onion_image =
            skin_image_clone_rotated(
                new_layout->onion_image,
                skin_rotation_rotate(skin_image_rot(new_layout->onion_image), by));
        if (new_layout->onion_image == SKIN_IMAGE_NONE) {
            goto Fail;
        }
    }

    *new_parts_ptr = new_parts;
    return new_layout;

Fail:
    // Free the newly created skins because they're not going to be used.
    while (new_parts) {
        SkinPart* next = new_parts->next;
        skin_part_free(new_parts);
        new_parts = next;
    }

    // Free the layout itself.
    skin_layout_free(new_layout);
    return NULL;
}

/** SKIN FILE
 **/

static int skin_file_load_from_v1(SkinFile* file,
                                  const AConfig* aconfig,
                                  const char* basepath,
                                  const SkinFramebufferFuncs* fb_funcs) {
    SkinPart*      part;
    SkinLayout*    layout;
    SkinLayout**   ptail = &file->layouts;
    SkinLocation*  location;
    int            nn;

    file->parts = part = skin_part_create_from_v1(
            aconfig, basepath, fb_funcs);
    if (part == NULL)
        return -1;

    for (nn = 0; nn < 4; nn++)
    {
        ANEW0(layout);

        layout->color = 0xff808080;

        ANEW0(location);

        layout->event_type  = 0x05;  /* close keyboard by default */
        layout->event_code  = 0;
        layout->event_value = 0;

        location->part     = part;
        switch (nn) {
            case 0:
                location->anchor.x = 0;
                location->anchor.y = 0;
                location->rotation = SKIN_ROTATION_0;
                layout->size       = part->rect.size;
                break;

            case 1:
                location->anchor.x = part->rect.size.h;
                location->anchor.y = 0;
                location->rotation = SKIN_ROTATION_90;
                layout->size.w     = part->rect.size.h;
                layout->size.h     = part->rect.size.w;
                break;

            case 2:
                location->anchor.x = part->rect.size.w;
                location->anchor.y = part->rect.size.h;
                location->rotation = SKIN_ROTATION_180;
                layout->size       = part->rect.size;
                break;

            default:
                location->anchor.x = 0;
                location->anchor.y = part->rect.size.w;
                location->rotation = SKIN_ROTATION_270;
                layout->size.w     = part->rect.size.h;
                layout->size.h     = part->rect.size.w;
                break;
        }
        layout->locations = location;
        layout->orientation = location->rotation;

        *ptail = layout;
        ptail  = &layout->next;
    }
    file->version = 1;
    return 0;
}

// Names for auto-generated layouts.
// We don't need "portrait" - it is the first layout in
// skin files by convention, and we generate other
// orientations from it.
static const char* auto_layout_names[] =
    { "reverse_landscape", "reverse_portrait", "landscape" };

static int skin_file_load_from_v2(SkinFile* file,
                                  const AConfig* aconfig,
                                  const char* basepath,
                                  const SkinFramebufferFuncs* fb_funcs) {
    /* first, load all parts */
    const AConfig* node = aconfig_find_const(aconfig, "parts");
    if (node == NULL)
        return -1;
    else
    {
        SkinPart**  ptail = &file->parts;
        for (node = node->first_child; node != NULL; node = node->next)
        {
            SkinPart*  part = skin_part_create_from_v2(
                    node, basepath, fb_funcs);
            if (part == NULL) {
                dprint( "## WARNING: can't load part '%s' from skin\n", node->name ? "<NULL>" : node->name );
                continue;
            }

            part->next = NULL;
            *ptail     = part;
            ptail      = &part->next;
        }
    }

    if (file->parts == NULL)
        return -1;

    /* then load all layouts */
    node = aconfig_find_const(aconfig, "layouts");
    if (node == NULL)
        return -1;
    else
    {
        SkinLayout* next = skin_layout_create_from_v2(node->first_child, file->parts, basepath);
        next->orientation = SKIN_ROTATION_0;

        // We need to find the last element in the list of parts defined by the skin.
        // The automatically parts generated parts (generated by rotation) will be appended
        // after that part.
        SkinPart** last_part_ptr = &(file->parts);

        if (next == NULL) return -1;
        file->layouts = next;

        // Generate new layouts from the first layout. The new layouts are rotated
        // versions of the first layout.
        SkinLayout* layout = file->layouts;
        SkinLayout* first_layout = layout;
        SkinRotation r;

        for (r = SKIN_ROTATION_90; r <= SKIN_ROTATION_270; r++) {
            // Find the place in the list of parts at which to insert new
            // rotated versions of parts.
            while (*last_part_ptr) {
                last_part_ptr = &((*last_part_ptr)->next);
            }

            // Rotate the layout.
            layout->next = skin_layout_create_rotated(
                    first_layout, last_part_ptr,
                    (first_layout->dpad_rotation + r) % 4);
            if (layout->next != NULL) {
                // Update layout name.
                int layout_name_idx = r - SKIN_ROTATION_90;
                if (layout_name_idx >= 0 &&
                    layout_name_idx < sizeof(auto_layout_names) / sizeof(const char*)) {
                    layout->next->name = auto_layout_names[layout_name_idx];
                }

                layout = layout->next;
                layout->next = NULL;
            } else {
                dprint("## WARNING: failed to auto-generate rotated layout");
            }
        }
    }
    if (file->layouts == NULL)
        return -1;

    file->version = 2;
    return 0;
}

SkinFile* skin_file_create_from_aconfig(const AConfig* aconfig,
                                        const char* basepath,
                                        const SkinFramebufferFuncs* fb_funcs) {
    SkinFile*  file;

    ANEW0(file);

    if (aconfig_find_const(aconfig, "parts") != NULL) {
        if (skin_file_load_from_v2(
                file, aconfig, basepath, fb_funcs) < 0) {
            goto BAD_FILE;
        }
        file->version = aconfig_int(aconfig, "version", 2);
        /* The file version must be 1 or higher */
        if (file->version <= 0) {
            dprint( "## WARNING: invalid skin version: %d", file->version);
            goto BAD_FILE;
        }
    }
    else {
        if (skin_file_load_from_v1(
                file, aconfig, basepath, fb_funcs) < 0) {
            goto BAD_FILE;
        }
        file->version = 1;
    }
    return file;

BAD_FILE:
    skin_file_free( file );
    return NULL;
}

void
skin_file_free( SkinFile*  file )
{
    if (file) {
        SKIN_FILE_LOOP_LAYOUTS(file,layout)
            skin_layout_free(layout);
        SKIN_FILE_LOOP_END_LAYOUTS
        file->layouts = NULL;

        SKIN_FILE_LOOP_PARTS(file,part)
            skin_part_free(part);
        SKIN_FILE_LOOP_END_PARTS
        file->parts = NULL;

        AFREE(file);
    }
}
