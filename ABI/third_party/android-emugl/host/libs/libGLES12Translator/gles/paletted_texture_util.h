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
#ifndef GRAPHICS_TRANSLATION_GLES_PALETTED_TEXTURE_UTIL_H_
#define GRAPHICS_TRANSLATION_GLES_PALETTED_TEXTURE_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#include <GLES/gl.h>

struct PalettedTextureUtil {
  static size_t GetImageBpp(GLenum format);
  static size_t GetEntrySizeBytes(GLenum format);
  static GLenum GetEntryFormat(GLenum format);
  static GLenum GetEntryType(GLenum format);
  static size_t ComputePaletteSize(size_t image_bpp, size_t palette_entry_size);
  static size_t ComputeLevel0Size(int width, int height, size_t image_bpp);
  static size_t ComputeLevelSize(size_t level0_size, size_t mip_level);
  static size_t ComputeTotalSize(size_t palette_size, size_t level0_size,
                                 size_t mip_levels);
  static uint8_t* Decompress(size_t image_bpp, size_t level_size,
                             size_t palette_entry_size,
                             const uint8_t* src_image_data,
                             const uint8_t* src_palette_data, uint8_t* dst);
};

#endif  // GRAPHICS_TRANSLATION_GLES_PALETTED_TEXTURE_UTIL_H_
