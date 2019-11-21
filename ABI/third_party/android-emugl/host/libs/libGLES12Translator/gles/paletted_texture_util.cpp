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

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <memory.h>

#include "common/alog.h"
#include "gles/paletted_texture_util.h"

size_t PalettedTextureUtil::GetImageBpp(GLenum format) {
  switch (format) {
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE4_RGBA8_OES:
      return 4;

    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_RGB8_OES:
    case GL_PALETTE8_RGBA4_OES:
    case GL_PALETTE8_RGBA8_OES:
      return 8;

    default:
      LOG_ALWAYS_FATAL("Not a known paletted format.");
      return 0;
  }
}

size_t PalettedTextureUtil::GetEntrySizeBytes(GLenum format) {
  switch (format) {
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_RGBA4_OES:
      return 2;

    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE8_RGB8_OES:
      return 3;

    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGBA8_OES:
      return 4;

    default:
      LOG_ALWAYS_FATAL("Not a known paletted format.");
      return 0;
  }
}

GLenum PalettedTextureUtil::GetEntryFormat(GLenum format) {
  switch (format) {
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB8_OES:
      return GL_RGB;

    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_RGBA4_OES:
    case GL_PALETTE8_RGBA8_OES:
      return GL_RGBA;

    default:
      LOG_ALWAYS_FATAL("Not a known paletted format.");
      return GL_RGBA;
  }
}

GLenum PalettedTextureUtil::GetEntryType(GLenum format) {
  switch (format) {
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
      return GL_UNSIGNED_SHORT_5_6_5;

    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE8_RGB5_A1_OES:
      return GL_UNSIGNED_SHORT_5_5_5_1;

    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE8_RGBA4_OES:
      return GL_UNSIGNED_SHORT_4_4_4_4;

    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGB8_OES:
    case GL_PALETTE8_RGBA8_OES:
      return GL_UNSIGNED_BYTE;

    default:
      LOG_ALWAYS_FATAL("Not a known paletted format.");
      return GL_UNSIGNED_BYTE;
  }
}

size_t PalettedTextureUtil::ComputePaletteSize(
    size_t image_bpp, size_t palette_entry_size) {
  // We support an arbitrary bits per pixel, but we only really
  // care about two cases.
  // For 4bpp images, we have a palette with 16 entries
  // For 8bpp images, we have a palette with 256 entries
  return (1 << image_bpp) * palette_entry_size;
}

size_t PalettedTextureUtil::ComputeLevel0Size(
    int width, int height, size_t image_bpp) {
  // Compute the base size for the first mipmap level of a texture.
  // We round up for special cases like a 1 x 1 4bpp image, which still occupies
  // one byte.
  return (image_bpp * width * height + 7) >> 3;
}

size_t PalettedTextureUtil::ComputeLevelSize(
    size_t level0_size, size_t mip_level) {
  // Compute the size of a particular mipmap level.
  // For example, for a 16 x 16 x 8bpp texture, the base level will have a size
  // of 256 bytes (16*16*1). Level 1 will have a size of 64 bytes (8*8*1). Level
  // 2 will have a size of 16 bytes (4*4*1), and so on.

  // Each mip level index means quartering the base size
  size_t shift = mip_level + mip_level;

  // We add (1 << shift) - 1 to round up. This is needed especially for the case
  // of a 4bpp texture going from a 2 x 2 level (two bytes) to a 1 x 1 level
  // (one half byte, but really has to be one).
  return (level0_size + (1 << shift) - 1) >> shift;
}

size_t PalettedTextureUtil::ComputeTotalSize(
    size_t palette_size, size_t level0_size, size_t mip_levels) {
  // Compute the total size needed for a compressed texture, given its palette
  // size, the size of the base mipmap level, and the number of mipmap levels
  // stored. The data is simply concatenated as bytes, with no padding or
  // alignment applied.
  size_t expected_size = palette_size + level0_size;
  for (size_t i = 1; i < mip_levels; i++) {
    expected_size += ComputeLevelSize(level0_size, i);
  }
  return expected_size;
}

uint8_t* PalettedTextureUtil::Decompress(
    size_t image_bpp, size_t level_size, size_t palette_entry_size,
    const uint8_t* src_image_data, const uint8_t* src_palette_data,
    uint8_t* dst) {

  if (image_bpp == 4) {
    for (size_t i = 0; i < level_size; ++i) {
      const uint_fast8_t index_pair = *src_image_data++;
      const uint_fast8_t index0 = (index_pair >> 4) & 15;
      const uint_fast8_t index1 = (index_pair >> 0) & 15;
      memcpy(dst, src_palette_data + index0 * palette_entry_size,
             palette_entry_size);
      dst += palette_entry_size;
      memcpy(dst, src_palette_data + index1 * palette_entry_size,
             palette_entry_size);
      dst += palette_entry_size;
    }
  } else {
    for (size_t i = 0; i < level_size; ++i) {
      const uint_fast8_t index = *src_image_data++;
      memcpy(dst, src_palette_data + index * palette_entry_size,
             palette_entry_size);
      dst += palette_entry_size;
    }
  }

  return dst;
}
