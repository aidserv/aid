/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "EglConfig.h"

#include <functional>

EglConfig::EglConfig(EGLint     red_size,
                     EGLint     green_size,
                     EGLint     blue_size,
                     EGLint     alpha_size,
                     EGLenum    caveat,
                     EGLint     conformant,
                     EGLint     config_id,
                     EGLint     depth_size,
                     EGLint     frame_buffer_level,
                     EGLint     max_pbuffer_width,
                     EGLint     max_pbuffer_height,
                     EGLint     max_pbuffer_size,
                     EGLBoolean native_renderable,
                     EGLint     renderable_type,
                     EGLint     native_visual_id,
                     EGLint     native_visual_type,
                     EGLint     sample_buffers_num,
                     EGLint     samples_per_pixel,
                     EGLint     stencil_size,
                     EGLint     luminance_size,
                     EGLint     wanted_buffer_size,
                     EGLint     surface_type,
                     EGLenum    transparent_type,
                     EGLint     trans_red_val,
                     EGLint     trans_green_val,
                     EGLint     trans_blue_val,
                     EGLBoolean recordable_android,
                     EGLBoolean framebuffer_target_android,
                     EglOS::PixelFormat* frmt):
        m_buffer_size(red_size + green_size + blue_size + alpha_size),
        m_red_size(red_size),
        m_green_size(green_size),
        m_blue_size(blue_size),
        m_alpha_size(alpha_size),
        m_bind_to_tex_rgb(EGL_FALSE), //not supported for now
        m_bind_to_tex_rgba(EGL_FALSE), //not supported for now
        m_caveat(caveat),
        m_config_id(config_id),
        m_native_config_id(config_id),
        m_frame_buffer_level(frame_buffer_level),
        m_depth_size(depth_size),
        m_max_pbuffer_width(max_pbuffer_width),
        m_max_pbuffer_height(max_pbuffer_height),
        m_max_pbuffer_size(max_pbuffer_size),
        m_max_swap_interval(MAX_SWAP_INTERVAL),
        m_min_swap_interval(MIN_SWAP_INTERVAL),
        m_native_renderable(native_renderable),
        m_renderable_type(renderable_type),
        m_native_visual_id(native_visual_id),
        m_native_visual_type(native_visual_type),
        m_sample_buffers_num(sample_buffers_num),
        m_samples_per_pixel(samples_per_pixel),
        m_stencil_size(stencil_size),
        m_luminance_size(luminance_size),
        m_wanted_buffer_size(wanted_buffer_size),
        m_surface_type(surface_type),
        m_transparent_type(transparent_type),
        m_trans_red_val(trans_red_val),
        m_trans_green_val(trans_green_val),
        m_trans_blue_val(trans_blue_val),
        m_recordable_android(recordable_android),
        m_framebuffer_target_android(framebuffer_target_android),
        m_conformant(conformant),
        m_nativeFormat(frmt),
        m_color_buffer_type(EGL_RGB_BUFFER) {}


#define FB_TARGET_ANDROID_BUF_SIZE(size) (size == 16  || size == 32) ? EGL_TRUE : EGL_FALSE

EglConfig::EglConfig(EGLint     red_size,
                     EGLint     green_size,
                     EGLint     blue_size,
                     EGLint     alpha_size,
                     EGLenum    caveat,
                     EGLint     config_id,
                     EGLint     depth_size,
                     EGLint     frame_buffer_level,
                     EGLint     max_pbuffer_width,
                     EGLint     max_pbuffer_height,
                     EGLint     max_pbuffer_size,
                     EGLBoolean native_renderable,
                     EGLint     renderable_type,
                     EGLint     native_visual_id,
                     EGLint     native_visual_type,
                     EGLint     samples_per_pixel,
                     EGLint     stencil_size,
                     EGLint     surface_type,
                     EGLenum    transparent_type,
                     EGLint     trans_red_val,
                     EGLint     trans_green_val,
                     EGLint     trans_blue_val,
                     EGLBoolean recordable_android,
                     EglOS::PixelFormat* frmt):
        m_buffer_size(red_size + green_size + blue_size + alpha_size),
        m_red_size(red_size),
        m_green_size(green_size),
        m_blue_size(blue_size),
        m_alpha_size(alpha_size),
        m_bind_to_tex_rgb(EGL_FALSE), //not supported for now
        m_bind_to_tex_rgba(EGL_FALSE), //not supported for now
        m_caveat(caveat),
        m_config_id(config_id),
        m_native_config_id(config_id),
        m_frame_buffer_level(frame_buffer_level),
        m_depth_size(depth_size),
        m_max_pbuffer_width(max_pbuffer_width),
        m_max_pbuffer_height(max_pbuffer_height),
        m_max_pbuffer_size(max_pbuffer_size),
        m_max_swap_interval(MAX_SWAP_INTERVAL),
        m_min_swap_interval(MIN_SWAP_INTERVAL),
        m_native_renderable(native_renderable),
        m_renderable_type(renderable_type),
        m_native_visual_id(native_visual_id),
        m_native_visual_type(native_visual_type),
        m_sample_buffers_num(samples_per_pixel > 0 ? 1 : 0),
        m_samples_per_pixel(samples_per_pixel),
        m_stencil_size(stencil_size),
        m_luminance_size(0),
        m_wanted_buffer_size(EGL_DONT_CARE),
        m_surface_type(surface_type),
        m_transparent_type(transparent_type),
        m_trans_red_val(trans_red_val),
        m_trans_green_val(trans_green_val),
        m_trans_blue_val(trans_blue_val),
        m_recordable_android(recordable_android),
        m_framebuffer_target_android(EGL_FALSE),
        m_conformant(((red_size + green_size + blue_size + alpha_size > 0)  &&
                     (caveat != EGL_NON_CONFORMANT_CONFIG)) ?
                     m_renderable_type : 0),
        m_nativeFormat(frmt),
        m_color_buffer_type(EGL_RGB_BUFFER) {}


EglConfig::EglConfig(const EglConfig& conf) :
        m_buffer_size(conf.m_buffer_size),
        m_red_size(conf.m_red_size),
        m_green_size(conf.m_green_size),
        m_blue_size(conf.m_blue_size),
        m_alpha_size(conf.m_alpha_size),
        m_bind_to_tex_rgb(conf.m_bind_to_tex_rgb),
        m_bind_to_tex_rgba(conf.m_bind_to_tex_rgba),
        m_caveat(conf.m_caveat),
        m_config_id(conf.m_config_id),
        m_native_config_id(conf.m_native_config_id),
        m_frame_buffer_level(conf.m_frame_buffer_level),
        m_depth_size(conf.m_depth_size),
        m_max_pbuffer_width(conf.m_max_pbuffer_width),
        m_max_pbuffer_height(conf.m_max_pbuffer_height),
        m_max_pbuffer_size(conf.m_max_pbuffer_size),
        m_max_swap_interval(conf.m_max_swap_interval),
        m_min_swap_interval(conf.m_min_swap_interval),
        m_native_renderable(conf.m_native_renderable),
        m_renderable_type(conf.m_renderable_type),
        m_native_visual_id(conf.m_native_visual_id),
        m_native_visual_type(conf.m_native_visual_type),
        m_sample_buffers_num(conf.m_sample_buffers_num),
        m_samples_per_pixel(conf.m_samples_per_pixel),
        m_stencil_size(conf.m_stencil_size),
        m_luminance_size(conf.m_luminance_size),
        m_wanted_buffer_size(conf.m_wanted_buffer_size),
        m_surface_type(conf.m_surface_type),
        m_transparent_type(conf.m_transparent_type),
        m_trans_red_val(conf.m_trans_red_val),
        m_trans_green_val(conf.m_trans_green_val),
        m_trans_blue_val(conf.m_trans_blue_val),
        m_recordable_android(conf.m_recordable_android),
        m_framebuffer_target_android(conf.m_framebuffer_target_android),
        m_conformant(conf.m_conformant),
        m_nativeFormat(conf.m_nativeFormat->clone()),
        m_color_buffer_type(EGL_RGB_BUFFER) {}


EglConfig::EglConfig(const EglConfig& conf,
                     EGLint config_id,
                     EGLint red_size,
                     EGLint green_size,
                     EGLint blue_size,
                     EGLint alpha_size) :
        m_buffer_size(red_size + green_size + blue_size + alpha_size),
        m_red_size(red_size),
        m_green_size(green_size),
        m_blue_size(blue_size),
        m_alpha_size(alpha_size),
        m_bind_to_tex_rgb(conf.m_bind_to_tex_rgb),
        m_bind_to_tex_rgba(conf.m_bind_to_tex_rgba),
        m_caveat(conf.m_caveat),
        m_config_id(config_id),
        m_native_config_id(conf.m_native_config_id),
        m_frame_buffer_level(conf.m_frame_buffer_level),
        m_depth_size(conf.m_depth_size),
        m_max_pbuffer_width(conf.m_max_pbuffer_width),
        m_max_pbuffer_height(conf.m_max_pbuffer_height),
        m_max_pbuffer_size(conf.m_max_pbuffer_size),
        m_max_swap_interval(conf.m_max_swap_interval),
        m_min_swap_interval(conf.m_min_swap_interval),
        m_native_renderable(conf.m_native_renderable),
        m_renderable_type(conf.m_renderable_type),
        m_native_visual_id(conf.m_native_visual_id),
        m_native_visual_type(conf.m_native_visual_type),
        m_sample_buffers_num(conf.m_sample_buffers_num),
        m_samples_per_pixel(conf.m_samples_per_pixel),
        m_stencil_size(conf.m_stencil_size),
        m_luminance_size(conf.m_luminance_size),
        m_wanted_buffer_size(conf.m_wanted_buffer_size),
        m_surface_type(conf.m_surface_type),
        m_transparent_type(conf.m_transparent_type),
        m_trans_red_val(conf.m_trans_red_val),
        m_trans_green_val(conf.m_trans_green_val),
        m_trans_blue_val(conf.m_trans_blue_val),
        m_recordable_android(conf.m_recordable_android),
        m_framebuffer_target_android(FB_TARGET_ANDROID_BUF_SIZE(m_buffer_size)),
        m_conformant(conf.m_conformant),
        m_nativeFormat(conf.m_nativeFormat->clone()),
        m_color_buffer_type(EGL_RGB_BUFFER) {};

bool EglConfig::getConfAttrib(EGLint attrib,EGLint* val) const {
    switch(attrib) {
    case EGL_BUFFER_SIZE:
        *val = m_buffer_size;
        break;
    case EGL_RED_SIZE:
        *val = m_red_size;
        break;
    case EGL_GREEN_SIZE:
        *val = m_green_size;
        break;
    case EGL_BLUE_SIZE:
        *val = m_blue_size;
        break;
    case EGL_LUMINANCE_SIZE:
        *val = m_luminance_size;
        break;
    case EGL_ALPHA_SIZE:
        *val = m_alpha_size;
        break;
    case EGL_BIND_TO_TEXTURE_RGB:
        *val = m_bind_to_tex_rgb;
        break;
    case EGL_BIND_TO_TEXTURE_RGBA:
        *val = m_bind_to_tex_rgba;
        break;
    case EGL_CONFIG_CAVEAT:
        *val = m_caveat;
        break;
    case EGL_CONFORMANT:
        *val = m_conformant;
        break;
    case EGL_CONFIG_ID:
        *val = m_config_id;
        break;
    case EGL_DEPTH_SIZE:
        *val = m_depth_size;
        break;
    case EGL_LEVEL:
        *val = m_frame_buffer_level;
        break;
    case EGL_MAX_PBUFFER_WIDTH:
        *val = m_max_pbuffer_width;
        break;
    case EGL_MAX_PBUFFER_HEIGHT:
        *val = m_max_pbuffer_height;
        break;
    case EGL_MAX_PBUFFER_PIXELS:
        *val = m_max_pbuffer_size;
        break;
    case EGL_MAX_SWAP_INTERVAL:
        *val = m_max_swap_interval;
        break;
    case EGL_MIN_SWAP_INTERVAL:
        *val = m_min_swap_interval;
        break;
    case EGL_NATIVE_RENDERABLE:
        *val = m_native_renderable;
        break;
    case EGL_NATIVE_VISUAL_ID:
        *val = m_native_visual_id;
        break;
    case EGL_NATIVE_VISUAL_TYPE:
        *val = m_native_visual_type;
        break;
    case EGL_RENDERABLE_TYPE:
        *val = m_renderable_type;
        break;
    case EGL_SAMPLE_BUFFERS:
        *val = m_sample_buffers_num;
        break;
    case EGL_SAMPLES:
        *val = m_samples_per_pixel;
        break;
    case EGL_STENCIL_SIZE:
        *val = m_stencil_size;
        break;
    case EGL_SURFACE_TYPE:
        *val = m_surface_type;
        break;
    case EGL_COLOR_BUFFER_TYPE:
        *val = EGL_RGB_BUFFER;
        break;
    case EGL_TRANSPARENT_TYPE:
        *val =m_transparent_type;
        break;
    case EGL_TRANSPARENT_RED_VALUE:
        *val = m_trans_red_val;
        break;
    case EGL_TRANSPARENT_GREEN_VALUE:
        *val = m_trans_green_val;
        break;
    case EGL_TRANSPARENT_BLUE_VALUE:
        *val = m_trans_blue_val;
        break;
    case EGL_RECORDABLE_ANDROID:
        *val = m_recordable_android;
        break;
    case EGL_FRAMEBUFFER_TARGET_ANDROID:
        *val = m_framebuffer_target_android;
        break;
    default:
        return false;
    }
    return true;
}

EGLint EglConfig::getConfAttrib(EGLint attrib) const {
    EGLint res;
    getConfAttrib(attrib, &res);
    return res;
}

// checking compitabilty between *this configuration and another configuration
// the compitability is checked againsed red,green,blue,buffer stencil and depth sizes
bool EglConfig::compatibleWith(const EglConfig& conf) const {

    return m_buffer_size == conf.m_buffer_size &&
           m_red_size == conf.m_red_size &&
           m_green_size == conf.m_green_size &&
           m_blue_size == conf.m_blue_size &&
           m_depth_size == conf.m_depth_size &&
           m_stencil_size == conf.m_stencil_size;
}

// For fixing dEQP EGL tests. This is based on the EGL spec
// and is inspired by the dEQP EGL test code itself.
static int ColorBufferTypeVal(EGLenum type) {
    switch (type) {
    case EGL_RGB_BUFFER: return 0;
    case EGL_LUMINANCE_BUFFER: return 1;
    case EGL_YUV_BUFFER_EXT: return 2;
    }
    return 3;
}

//following the sorting EGLconfig as in spec
// Note that we also need to sort during eglChooseConfig
// when returning configs to user, as sorting order
// can depend on which attributes the user has requested.
bool EglConfig::operator<(const EglConfig& conf) const {
    //0
    if(m_conformant != conf.m_conformant) {
       return m_conformant != 0; //We want the conformant ones first
    }
    //1
    if(m_caveat != conf.m_caveat) {
       return m_caveat < conf.m_caveat; // EGL_NONE < EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG
    }

    //2
    if (m_color_buffer_type != conf.m_color_buffer_type) {
        return ColorBufferTypeVal(m_color_buffer_type) <
               ColorBufferTypeVal(conf.m_color_buffer_type);
    }

    //3
    if (m_buffer_size != conf.m_buffer_size) {
        return m_buffer_size < conf.m_buffer_size;
    }

    //4
    if(m_sample_buffers_num != conf.m_sample_buffers_num) {
       return m_sample_buffers_num < conf.m_sample_buffers_num;
    }
    //5
    if(m_samples_per_pixel != conf.m_samples_per_pixel) {
       return m_samples_per_pixel < conf.m_samples_per_pixel;
    }
    //6
    if(m_depth_size != conf.m_depth_size) {
       return m_depth_size < conf.m_depth_size;
    }
    //7
    if(m_stencil_size != conf.m_stencil_size) {
       return m_stencil_size < conf.m_stencil_size;
    }

    return m_config_id < conf.m_config_id;
}

bool EglConfig::operator>=(const EglConfig& conf) const {
    return  !((*this) < conf);
}

// static
bool EglConfig::operator==(const EglConfig& other) const {
#define EGLCONFIG_EQ(field) \
    (field == other.field)

    return
    EGLCONFIG_EQ(m_buffer_size) &&
    EGLCONFIG_EQ(m_red_size) &&
    EGLCONFIG_EQ(m_green_size) &&
    EGLCONFIG_EQ(m_blue_size) &&
    EGLCONFIG_EQ(m_alpha_size) &&
    EGLCONFIG_EQ(m_bind_to_tex_rgb) &&
    EGLCONFIG_EQ(m_bind_to_tex_rgba) &&
    EGLCONFIG_EQ(m_caveat) &&
    // Not using config id, we are only concerned with properties.
    // EGLCONFIG_EQ(m_config_id) &&
    // EGLCONFIG_EQ(m_native_config_id) &&
    EGLCONFIG_EQ(m_frame_buffer_level) &&
    EGLCONFIG_EQ(m_depth_size) &&
    EGLCONFIG_EQ(m_max_pbuffer_width) &&
    EGLCONFIG_EQ(m_max_pbuffer_height) &&
    EGLCONFIG_EQ(m_max_pbuffer_size) &&
    EGLCONFIG_EQ(m_max_swap_interval) &&
    EGLCONFIG_EQ(m_min_swap_interval) &&
    EGLCONFIG_EQ(m_native_renderable) &&
    EGLCONFIG_EQ(m_renderable_type) &&
    // EGLCONFIG_EQ(m_native_visual_id) &&
    // EGLCONFIG_EQ(m_native_visual_type) &&
    EGLCONFIG_EQ(m_sample_buffers_num) &&
    EGLCONFIG_EQ(m_samples_per_pixel) &&
    EGLCONFIG_EQ(m_stencil_size) &&
    EGLCONFIG_EQ(m_luminance_size) &&
    // EGLCONFIG_EQ(m_wanted_buffer_size) &&
    EGLCONFIG_EQ(m_surface_type) &&
    EGLCONFIG_EQ(m_transparent_type) &&
    EGLCONFIG_EQ(m_trans_red_val) &&
    EGLCONFIG_EQ(m_trans_green_val) &&
    EGLCONFIG_EQ(m_trans_blue_val) &&
    EGLCONFIG_EQ(m_recordable_android) &&
    EGLCONFIG_EQ(m_framebuffer_target_android) &&
    EGLCONFIG_EQ(m_conformant) &&
    EGLCONFIG_EQ(m_color_buffer_type);

#undef EGLCONFIG_EQ
}

uint32_t EglConfig::u32hash() const {
    uint32_t res = 0xabcd9001;

#define EGLCONFIG_HASH(field) \
    res = res * 16777213 + \
          std::hash<unsigned int>()((unsigned int)field); \

    EGLCONFIG_HASH(m_buffer_size)
    EGLCONFIG_HASH(m_red_size)
    EGLCONFIG_HASH(m_green_size)
    EGLCONFIG_HASH(m_blue_size)
    EGLCONFIG_HASH(m_alpha_size)
    EGLCONFIG_HASH(m_bind_to_tex_rgb)
    EGLCONFIG_HASH(m_bind_to_tex_rgba)
    EGLCONFIG_HASH(m_caveat)
    // Only properties
    // EGLCONFIG_HASH(m_config_id)
    // EGLCONFIG_HASH(m_native_config_id)
    EGLCONFIG_HASH(m_frame_buffer_level)
    EGLCONFIG_HASH(m_depth_size)
    EGLCONFIG_HASH(m_max_pbuffer_width)
    EGLCONFIG_HASH(m_max_pbuffer_height)
    EGLCONFIG_HASH(m_max_pbuffer_size)
    EGLCONFIG_HASH(m_max_swap_interval)
    EGLCONFIG_HASH(m_min_swap_interval)
    EGLCONFIG_HASH(m_native_renderable)
    EGLCONFIG_HASH(m_renderable_type)
    // EGLCONFIG_HASH(m_native_visual_id)
    // EGLCONFIG_HASH(m_native_visual_type)
    EGLCONFIG_HASH(m_sample_buffers_num)
    EGLCONFIG_HASH(m_samples_per_pixel)
    EGLCONFIG_HASH(m_stencil_size)
    EGLCONFIG_HASH(m_luminance_size)
    // EGLCONFIG_HASH(m_wanted_buffer_size)
    EGLCONFIG_HASH(m_surface_type)
    EGLCONFIG_HASH(m_transparent_type)
    EGLCONFIG_HASH(m_trans_red_val)
    EGLCONFIG_HASH(m_trans_green_val)
    EGLCONFIG_HASH(m_trans_blue_val)
    EGLCONFIG_HASH(m_recordable_android)
    EGLCONFIG_HASH(m_framebuffer_target_android)
    EGLCONFIG_HASH(m_conformant)
    EGLCONFIG_HASH(m_color_buffer_type)

#undef EGLCONFIG_HASH
    return res;
}

//checking if config stands for all the selection crateria of dummy as defined by EGL spec
#define CHECK_PROP(dummy,prop_name,op) \
    if((dummy.prop_name != EGL_DONT_CARE) && (dummy.prop_name op prop_name)) { \
        CHOOSE_CONFIG_DLOG(#prop_name " does not match: %d vs %d", dummy.prop_name, prop_name); \
        return false; \
    } else { \
        CHOOSE_CONFIG_DLOG(#prop_name " compatible."); \
    } \

#define CHECK_PROP_CAST(dummy,prop_name,op) \
    if((((EGLint)dummy.prop_name) != EGL_DONT_CARE) && (dummy.prop_name op prop_name)) { \
        CHOOSE_CONFIG_DLOG(#prop_name " does not match."); \
        return false; \
    } else { \
        CHOOSE_CONFIG_DLOG(#prop_name " compatible."); \
    } \

bool EglConfig::chosen(const EglConfig& dummy) const {

   CHOOSE_CONFIG_DLOG("checking config id 0x%x for compatibility\n", m_config_id);
   CHOOSE_CONFIG_DLOG("config info for 0x%x: "
                      "rgbads %d %d %d %d %d %d "
                      "samp spp %d %d fblvl %d n.vistype %d maxswap %d minswap %d"
                      "transrgb %d %d %d caveat %d n.renderable %d "
                      "transptype %d surftype %d conform %d rendertype %d",
                      m_config_id,

                      m_red_size,
                      m_green_size,
                      m_blue_size,
                      m_alpha_size,
                      m_depth_size,
                      m_stencil_size,

                      m_sample_buffers_num,
                      m_samples_per_pixel,

                      m_frame_buffer_level,

                      m_native_visual_type,

                      m_max_swap_interval,
                      m_min_swap_interval,

                      m_trans_red_val,
                      m_trans_green_val,
                      m_trans_blue_val,

                      m_caveat,
                      m_native_renderable,

                      m_transparent_type,
                      m_surface_type,
                      m_conformant,
                      m_renderable_type);

   //atleast
   CHECK_PROP(dummy,m_buffer_size,>);
   CHECK_PROP(dummy,m_red_size,>);
   CHECK_PROP(dummy,m_green_size,>);
   CHECK_PROP(dummy,m_blue_size,>);
   CHECK_PROP(dummy,m_alpha_size,>);
   CHECK_PROP(dummy,m_depth_size,>);
   CHECK_PROP(dummy,m_stencil_size,>);

   CHECK_PROP(dummy,m_luminance_size,>);

   // We distinguish here between the buffer size
   // desired by the user (dummy.m_wanted_buffer_size)
   // versus the actual config's buffer size
   // (m_wanted_buffer_size).
   if (dummy.isWantedAttrib(EGL_BUFFER_SIZE)) {
       if (dummy.m_wanted_buffer_size != EGL_DONT_CARE &&
           dummy.m_wanted_buffer_size > m_buffer_size) {
           return false;
       }
   }

   CHECK_PROP(dummy,m_sample_buffers_num,>);
   CHECK_PROP(dummy,m_samples_per_pixel,>);

   //exact
   CHECK_PROP(dummy,m_frame_buffer_level,!=);
   CHECK_PROP(dummy,m_config_id,!=);

   CHECK_PROP(dummy,m_native_visual_type,!=);
   CHECK_PROP(dummy,m_max_swap_interval ,!=);
   CHECK_PROP(dummy,m_min_swap_interval ,!=);
   CHECK_PROP(dummy,m_trans_red_val    ,!=);
   CHECK_PROP(dummy,m_trans_green_val ,!=);
   CHECK_PROP(dummy,m_trans_blue_val  ,!=);
   //exact - when cast to EGLint is needed when comparing to EGL_DONT_CARE
   CHECK_PROP_CAST(dummy,m_bind_to_tex_rgb ,!=);
   CHECK_PROP_CAST(dummy,m_bind_to_tex_rgba,!=);
   CHECK_PROP_CAST(dummy,m_caveat,!=);
   CHECK_PROP_CAST(dummy,m_native_renderable ,!=);
   CHECK_PROP_CAST(dummy,m_transparent_type   ,!=);

   //mask
   if(dummy.m_surface_type != EGL_DONT_CARE &&
      ((dummy.m_surface_type &
       (m_surface_type | EGL_WINDOW_BIT)) != // Note that we always advertise our configs'
                                             // EGL_SURFACE_TYPE has having EGL_WINDOW_BIT
                                             // capability, so we must also respect that here.
       dummy.m_surface_type)) {

       return false;
   }

   if(dummy.m_conformant != (EGLenum)EGL_DONT_CARE &&
      ((dummy.m_conformant & m_conformant) != dummy.m_conformant)) {
       CHOOSE_CONFIG_DLOG("m_conformant does not match.");
       return false;
   }

   if(dummy.m_renderable_type != EGL_DONT_CARE &&
      ((dummy.m_renderable_type & m_renderable_type) != dummy.m_renderable_type)) {
       CHOOSE_CONFIG_DLOG("m_renderable_type does not match.");
       return false;
   }

   if ((EGLint)(dummy.m_framebuffer_target_android) != EGL_DONT_CARE &&
       dummy.m_framebuffer_target_android !=
       m_framebuffer_target_android) {
       CHOOSE_CONFIG_DLOG("m_framebuffer_target_android does not match.");
       return false;
   }

   CHOOSE_CONFIG_DLOG("config id 0x%x passes.", m_config_id);

   //passed all checks
   return true;
}
