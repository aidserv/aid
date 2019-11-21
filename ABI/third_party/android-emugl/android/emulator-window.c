/* Copyright (C) 2006-2016 The Android Open Source Project
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

#include "android/emulator-window.h"

#include "android/android.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/framebuffer.h"
#include "android/globals.h"
#include "android/gpu_frame.h"
#include "android/hw-control.h"
#include "android/hw-sensors.h"
#include "android/network/globals.h"
#include "android/opengles.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/set-ui-emu-agent.h"
#include "android/skin/winsys.h"
#include "android/ui-emu-agent.h"
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "android/utils/looper.h"

#include "android/telephony/modem_driver.h"


#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

/* EmulatorWindow structure instance. */
static EmulatorWindow   qemulator[1];

// Our very own stash of a pointer to a device that handles user events.
const QAndroidUserEventAgent* user_event_agent;

// Set to 1 to use an EmuGL sub-window to display GpU content, or 0 to use
// the frame post callback to retrieve every frame from the GPU, which will
// be slower, except for software-based renderers.
static bool s_use_emugl_subwindow = 1;

static void emulator_window_refresh(EmulatorWindow* emulator);
extern void qemu_system_shutdown_request(void);

static void write_window_name(char* buff,
                              size_t buff_len,
                              int base_port,
                              const char* avd_name) {
    snprintf(buff, buff_len, "Android Emulator - %s:%d", avd_name, base_port);
}


static void
emulator_window_light_brightness(void* opaque, const char*  light, int  value)
{
    EmulatorWindow*  emulator = opaque;

    VERBOSE_PRINT(hw_control,"%s: light='%s' value=%d ui=%p", __FUNCTION__, light, value, emulator->ui);

    if (!strcmp(light, "lcd_backlight")) {
        skin_ui_set_lcd_brightness(emulator->ui, value);
        return;
    }
}

static void emulator_window_trackball_event(int dx, int dy) {
    user_event_agent->sendMouseEvent(dx, dy, 1, 0);
}

static void emulator_window_window_key_event(unsigned keycode, int down) {
    user_event_agent->sendKey(keycode, down);
}

static void emulator_window_keycodes_event(int* keycodes, int count) {
    user_event_agent->sendKeyCodes(keycodes, count);
}

static void emulator_window_window_mouse_event(unsigned x,
                                         unsigned y,
                                         unsigned state) {
    /* NOTE: the 0 is used in hw/android/goldfish/events_device.c to
     * differentiate between a touch-screen and a trackball event
     */
    user_event_agent->sendMouseEvent(x, y, 0, state);
}

static void emulator_window_window_rotary_input_event(int delta) {
    user_event_agent->sendRotaryEvent(delta);
}

static void _emulator_window_update_rotation(SkinUI* ui, SkinRotation rotation) {
    skin_ui_update_rotation(ui, rotation);
}

static void emulator_window_set_device_orientation(SkinRotation rotation) {
    android_sensors_set_coarse_orientation((AndroidCoarseOrientation) rotation);
    if (qemulator->ui) {
        // Make sure UI knows about the updated orientation.
        _emulator_window_update_rotation(qemulator->ui, rotation);
    }
}

static bool emulator_window_network_toggle(void) {
    android_net_disable = !android_net_disable;
    if (android_modem) {
        amodem_set_data_registration(
                android_modem,
        android_net_disable ? A_REGISTRATION_UNREGISTERED
            : A_REGISTRATION_HOME);
    }
    return !android_net_disable;
}

static void emulator_window_framebuffer_invalidate(void) {
    qframebuffer_invalidate_all();
    qframebuffer_check_updates();
}

static void emulator_window_keyboard_event(void* opaque, SkinKeyCode keycode, int down) {
    (void)opaque;
    user_event_agent->sendKey(keycode, down);
}

static int emulator_window_opengles_show_window(
    void* window, int x, int y, int vw, int vh, int w, int h, float dpr, float rotation) {
    if (s_use_emugl_subwindow) {
        return android_showOpenglesWindow(window, x, y, vw, vh, w, h, dpr, rotation);
    } else {
        return 0;
    }
}

static int emulator_window_opengles_hide_window(void) {
    if (s_use_emugl_subwindow) {
        return android_hideOpenglesWindow();
    } else {
        return 0;
    }
}

static void emulator_window_opengles_set_translation(float dx, float dy) {
    if (s_use_emugl_subwindow) {
        android_setOpenglesTranslation(dx, dy);
    }
}

static void emulator_window_opengles_redraw_window(void) {
    if (s_use_emugl_subwindow) {
        android_redrawOpenglesWindow();
    }
}

// Used as an emugl callback to get each frame of GPU display.
static void _emulator_window_on_gpu_frame(void* context,
                                          int width,
                                          int height,
                                          const void* pixels) {
    EmulatorWindow* emulator = (EmulatorWindow*)context;
    // This function is called from an EmuGL thread, which cannot
    // call the skin_ui_update_gpu_frame() function. Create a GpuFrame
    // instance, and send its address into the pipe.
    skin_ui_update_gpu_frame(emulator->ui, width, height, pixels);
}

static void
emulator_window_setup( EmulatorWindow*  emulator )
{
    user_event_agent = emulator->uiEmuAgent->userEvents;

    static const SkinWindowFuncs my_window_funcs = {
        .key_event = &emulator_window_window_key_event,
        .mouse_event = &emulator_window_window_mouse_event,
        .rotary_input_event = &emulator_window_window_rotary_input_event,
        .set_device_orientation = &emulator_window_set_device_orientation,
        .opengles_show = &emulator_window_opengles_show_window,
        .opengles_hide = &emulator_window_opengles_hide_window,
        .opengles_setTranslation = &emulator_window_opengles_set_translation,
        .opengles_redraw = &emulator_window_opengles_redraw_window,
    };

    static const SkinTrackBallParameters my_trackball_params = {
        .diameter = 60,
        .ring = 4,
        .ball_color = 0xffe0e0e0,
        .dot_color = 0xff202020,
        .ring_color = 0xff000000,
        .event_func = &emulator_window_trackball_event,
    };

    if (emulator->opts->no_window || emulator->ui) {
        return;
    }

    if (emulator->opts->scale) {
        dwarning("The -scale flag is obsolete and will be ignored.");
    }

    if (emulator->opts->dpi_device) {
        dwarning("The -dpi-device flag is obsolete and will be ignored.");
    }

    SkinUIParams my_ui_params = {
        .enable_touch = !androidHwConfig_isScreenNoTouch(android_hw),
        .enable_dpad = android_hw->hw_dPad != 0,
        .enable_keyboard = android_hw->hw_keyboard != 0,
        .enable_trackball = android_hw->hw_trackBall != 0,
        .enable_scale = !emulator->opts->fixed_scale,

        .window_x = emulator->win_x,
        .window_y = emulator->win_y,

        .keyboard_charmap = emulator->opts->charmap
    };

    write_window_name(my_ui_params.window_name,
                      sizeof(my_ui_params.window_name),
                      android_base_port,
                      avdInfo_getName(android_avdInfo));

    static const SkinUIFuncs my_ui_funcs = {
            .window_funcs = &my_window_funcs,
            .trackball_params = &my_trackball_params,
            .keyboard_event = &emulator_window_keyboard_event,
            .keyboard_flush = &emulator_window_keycodes_event,
            .network_toggle = &emulator_window_network_toggle,
            .framebuffer_invalidate = &emulator_window_framebuffer_invalidate,
    };

    // Determine whether to use an EmuGL sub-window or not.
    const char* env = getenv("ANDROID_GL_SOFTWARE_RENDERER");
    s_use_emugl_subwindow = !env || !env[0] || env[0] == '0';
    // for gpu off or gpu guest, we don't use the subwindow
    if (!android_hw->hw_gpu_enabled || !strcmp(android_hw->hw_gpu_mode, "guest")) {
        s_use_emugl_subwindow = 0;
    }


    if (s_use_emugl_subwindow) {
        VERBOSE_PRINT(gles, "Using EmuGL sub-window for GPU display");
    } else {
        VERBOSE_PRINT(gles, "Using glReadPixels() for GPU display");
    }

    emulator->ui = skin_ui_create(
            emulator->layout_file, android_hw->hw_initialOrientation,
            &my_ui_funcs, &my_ui_params, s_use_emugl_subwindow);
    if (!emulator->ui) {
        return;
    }

    if (emulator->onion) {
        skin_ui_set_onion(emulator->ui,
                          emulator->onion,
                          emulator->onion_rotation,
                          emulator->onion_alpha);
    }

    setUiEmuAgent(emulator->uiEmuAgent);

    // Determine whether to use an EmuGL sub-window or not.
    if (!s_use_emugl_subwindow) {
        gpu_frame_set_post_callback(looper_getForThread(),
                                    emulator,
                                    _emulator_window_on_gpu_frame);
    }

    skin_ui_reset_title(emulator->ui);
}

static void
emulator_window_fb_update( void*   _emulator, int  x, int  y, int  w, int  h )
{
    EmulatorWindow*  emulator = _emulator;

    if (emulator->opts->no_window) {
        return;
    }

    if (!emulator->ui) {
        emulator_window_setup(emulator);
    }

    if (!s_use_emugl_subwindow) {
        skin_ui_update_display(emulator->ui, x, y, w, h);
    }
}

static void
emulator_window_fb_rotate( void*  _emulator, int  rotation )
{
    EmulatorWindow*  emulator = _emulator;

    emulator_window_setup( emulator );
}

static void
emulator_window_fb_poll( void* _emulator )
{
    EmulatorWindow* emulator = _emulator;
    emulator_window_refresh(emulator);
}

EmulatorWindow*
emulator_window_get(void)
{
    return qemulator;
}

static void emulator_window_framebuffer_free(void* opaque) {
    QFrameBuffer* fb = opaque;

    qframebuffer_done(fb);
    free(fb);
}

static void* emulator_window_framebuffer_create(int width, int height, int bpp) {
    QFrameBuffer* fb = calloc(1, sizeof(*fb));

    qframebuffer_init(fb, width, height, 0,
                      bpp == 32 ? QFRAME_BUFFER_RGBX_8888
                                : QFRAME_BUFFER_RGB565 );

    qframebuffer_fifo_add(fb);
    return fb;
}

static void* emulator_window_framebuffer_get_pixels(void* opaque) {
    QFrameBuffer* fb = opaque;
    return fb->pixels;
}

static int emulator_window_framebuffer_get_depth(void* opaque) {
    QFrameBuffer* fb = opaque;
    return fb->bits_per_pixel;
}

int emulator_window_init(EmulatorWindow* emulator,
                         const AConfig* aconfig,
                         const char* basepath,
                         int x,
                         int y,
                         const AndroidOptions* opts,
                         const UiEmuAgent* uiEmuAgent) {
    static const SkinFramebufferFuncs skin_fb_funcs = {
        .create_framebuffer = &emulator_window_framebuffer_create,
        .free_framebuffer = &emulator_window_framebuffer_free,
        .get_pixels = &emulator_window_framebuffer_get_pixels,
        .get_depth = &emulator_window_framebuffer_get_depth,
    };

    emulator->aconfig = aconfig;

    // if not building for a gui-less window, create a skin layout file,
    // else skip as no skin will be displayed
    if (!opts->no_window) {
        emulator->layout_file = skin_file_create_from_aconfig(aconfig, basepath,
                                                              &skin_fb_funcs);
    }

    emulator->ui = NULL;
    emulator->win_x = x;
    emulator->win_y = y;
    *(emulator->opts) = *opts;
    *(emulator->uiEmuAgent) = *uiEmuAgent;

    /* register as a framebuffer clients for all displays defined in the skin file */
    if (emulator->layout_file) {
        SKIN_FILE_LOOP_PARTS(emulator->layout_file, part)
        SkinDisplay*  disp = part->display;
        if (disp->valid) {
            qframebuffer_add_client( disp->framebuffer,
                                    emulator,
                                    emulator_window_fb_update,
                                    emulator_window_fb_rotate,
                                    emulator_window_fb_poll,
                                    NULL );
        }
    SKIN_FILE_LOOP_END_PARTS
    }

    /* initialize hardware control support */
    AndroidHwControlFuncs funcs;
    funcs.light_brightness = emulator_window_light_brightness;
    android_hw_control_set(emulator, &funcs);

    return 0;
}

void
emulator_window_done(EmulatorWindow* emulator)
{
    if (emulator->ui) {
        skin_ui_free(emulator->ui);
        emulator->ui = NULL;
    }
    if (emulator->layout_file) {
        skin_file_free(emulator->layout_file);
        emulator->layout_file = NULL;
    }
}

/* called periodically to poll for user input events */
static void emulator_window_refresh(EmulatorWindow* emulator)
{
   /* this will eventually call sdl_update if the content of the VGA framebuffer
    * has changed */
    qframebuffer_check_updates();

    if (emulator->ui) {
        if (skin_ui_process_events(emulator->ui)) {
            // Quit program.
            skin_ui_free(emulator->ui);
            emulator->ui = NULL;
            qemu_system_shutdown_request();
        }
    }
}

void
android_emulator_set_base_port( int  port )
{
    if (qemulator->ui) {
        /* Base port is already set in the emulator's core. */
        char buff[256];
        write_window_name(buff,
                        sizeof(buff),
                        android_base_port,
                        avdInfo_getName(android_avdInfo));

        skin_ui_set_name(qemulator->ui, buff);
    }
}

SkinLayout*
emulator_window_get_layout(EmulatorWindow* emulator)
{
    if (emulator->ui) {
        return skin_ui_get_current_layout(emulator->ui);
    } else {
        if(emulator->opts->no_window) {
            // in no-window mode there is no skin layout
            return NULL;
        } else {
            return emulator->layout_file->layouts;
        }
    }
    return NULL;
}

bool emulator_window_rotate_90_clockwise(void) {
    if (qemulator->ui) {
        skin_ui_select_next_layout(qemulator->ui);
        return true;
    }
    return false;
}
