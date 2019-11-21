/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRAPHICS_TRANSLATION_GLES_MACROS_H_
#define GRAPHICS_TRANSLATION_GLES_MACROS_H_

#include "android/utils/debug.h"
#include "common/alog.h"
#include "gles/pass_through.h"

class GlesContext;
GlesContext* GetCurrentGlesContext();

#ifdef _WIN32
#define EXPORT_DECL extern "C" __declspec(dllexport)
#else
#define EXPORT_DECL extern "C" __attribute__((visibility("default")))
#endif

#define GLES_APIENTRY(_return, _name, ...) \
    EXPORT_DECL _return gl##_name(__VA_ARGS__)

#define STUB_APIENTRY(_return, _name, ...) \
EXPORT_DECL _return gl##_name(__VA_ARGS__) { \
    LOG_ALWAYS_FATAL("Unimplemented: %s", __FUNCTION__); \
  }

#define TRANSLATOR_APIENTRY(_return, _name, ...) \
    EXPORT_DECL _return _name(__VA_ARGS__)

#define GLES_ERROR(_err, _msg, ...)                                          \
  do {                                                                       \
    derror("gles12 ERROR: func=%s err=0x%x msg=", __FUNCTION__, _err); derror(_msg, ##__VA_ARGS__); \
    GlesContext* _context = GetCurrentGlesContext();                         \
    ALOG_ASSERT(_context != NULL);                                           \
    _context->SetGLerror(_err, __FUNCTION__, __LINE__, _msg, ##__VA_ARGS__); \
  } while (0)

#define GLES_ERROR_INVALID_ENUM(_arg)                                  \
  GLES_ERROR(GL_INVALID_ENUM, "Invalid enum for %s: %s (0x%x)", #_arg, \
             GetEnumString(_arg), _arg)

#define _GLES_ERROR_INVALID_VALUE(_arg, _fmt)                              \
  GLES_ERROR(GL_INVALID_VALUE, "Invalid value for %s: " _fmt, #_arg, _arg)

#define GLES_ERROR_INVALID_VALUE_FLOAT(_arg) \
  _GLES_ERROR_INVALID_VALUE(_arg, "%f")

#define GLES_ERROR_INVALID_VALUE_INT(_arg) \
  _GLES_ERROR_INVALID_VALUE(_arg, "%d")

#define GLES_ERROR_INVALID_VALUE_UINT(_arg) \
  _GLES_ERROR_INVALID_VALUE(_arg, "%u")

#define GLES_ERROR_INVALID_VALUE_PTR(_arg) \
  _GLES_ERROR_INVALID_VALUE(_arg, "0x%p")

#endif  // GRAPHICS_TRANSLATION_GLES_MACROS_H_
