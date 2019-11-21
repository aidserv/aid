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

#include "android/skin/event.h"
#include "android/skin/file.h"
#include "android/skin/trackball.h"

typedef struct SkinWindow  SkinWindow;

typedef struct SkinWindowFuncs {
    void (*key_event)(unsigned keycode, int down);
    void (*mouse_event)(unsigned x, unsigned y, unsigned state);
    void (*rotary_input_event)(int delta);
    void (*set_device_orientation)(SkinRotation rotation);
    int (*opengles_show)(void* winhandle,
                         int wx,
                         int wy,
                         int ww,
                         int wh,
                         int fbw,
                         int fbh,
                         float dpr,
                         float rotation_degrees);
    int (*opengles_hide)(void);
    void (*opengles_setTranslation)(float px, float py);
    void (*opengles_redraw)(void);
} SkinWindowFuncs;

/* Note: if scale is <= 0, we interpret this as 'auto-detect'.
 *       The behaviour is to use 1.0 by default, unless the resulting
 *       window is too large, in which case the window will automatically
 *       be resized to fit the screen.
 */
extern SkinWindow* skin_window_create(SkinLayout* layout,
                                      int x,
                                      int y,
                                      bool enable_scale,
                                      bool use_emugl_subwindow,
                                      const SkinWindowFuncs* win_funcs);

extern void             skin_window_enable_touch( SkinWindow*  window, int  enabled );
extern void             skin_window_enable_trackball( SkinWindow*  window, int  enabled );
extern void             skin_window_enable_dpad( SkinWindow*  window, int  enabled );
extern void             skin_window_enable_qwerty( SkinWindow*  window, int  enabled );

extern int              skin_window_reset ( SkinWindow*  window, SkinLayout*  layout );
extern void             skin_window_free  ( SkinWindow*  window );
extern void             skin_window_redraw( SkinWindow*  window, SkinRect*  rect );
extern void             skin_window_process_event( SkinWindow*  window, SkinEvent*  ev );

extern void             skin_window_set_onion( SkinWindow*   window,
                                               SkinImage*    onion,
                                               SkinRotation  rotation,
                                               int           alpha );

extern void             skin_window_set_scale( SkinWindow*  window,
                                               double       scale );

extern void             skin_window_set_zoom( SkinWindow*  window,
                                              double       zoom,
                                              int          dw,
                                              int          dh,
                                              int          scroll_h );

extern void             skin_window_position_changed( SkinWindow*   window,
                                                      int x, int y );

extern void             skin_window_scroll_updated( SkinWindow*    window,
                                                    int dx, int xmax,
                                                    int dy, int ymax );

extern void             skin_window_set_title( SkinWindow*  window,
                                               const char*  title );

extern void             skin_window_set_trackball( SkinWindow*  window, SkinTrackBall*  ball );
extern void             skin_window_show_trackball( SkinWindow*  window, int  enable );

extern void             skin_window_zoomed_window_resized( SkinWindow*  window,
                                                           int dx, int dy, int w, int h,
                                                           int scroll_h);

/* change the brightness of the emulator LCD screen. 'brightness' will be clamped to 0..255 */
extern void             skin_window_set_lcd_brightness( SkinWindow*  window, int  brightness );

typedef struct {
    int           width;
    int           height;
    SkinRotation  rotation;
    void*         data;
} ADisplayInfo;

extern void             skin_window_update_display( SkinWindow*  window, int  x, int  y, int  w, int  h );

extern void skin_window_update_gpu_frame(SkinWindow* window, int w, int h, const void* pixels);

extern void skin_window_update_rotation(SkinWindow* window, SkinRotation rotation);
