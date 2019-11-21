/*
 * Copyright (C) 2011 The Android Open Source Project
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
#include "gles/gles_validate.h"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>

#include "gles/state.h"

bool IsValidAlphaFunc(GLenum func) {
  switch (func) {
    case GL_NEVER:
    case GL_LESS:
    case GL_EQUAL:
    case GL_LEQUAL:
    case GL_GREATER:
    case GL_NOTEQUAL:
    case GL_GEQUAL:
    case GL_ALWAYS:
      return true;
    default:
      return false;
  }
}

bool IsValidBlendFunc(GLenum factor) {
  switch (factor) {
    case GL_ZERO:
    case GL_ONE:
    case GL_SRC_COLOR:
    case GL_ONE_MINUS_SRC_COLOR:
    case GL_DST_COLOR:
    case GL_ONE_MINUS_DST_COLOR:
    case GL_SRC_ALPHA:
    case GL_ONE_MINUS_SRC_ALPHA:
    case GL_DST_ALPHA:
    case GL_ONE_MINUS_DST_ALPHA:
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
    case GL_SRC_ALPHA_SATURATE:
      return true;
    default:
      return false;
  }
}

bool IsValidBufferTarget(GLenum target) {
  return target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER;
}

bool IsValidBufferUsage(GLenum usage) {
  return usage == GL_STATIC_DRAW || usage == GL_DYNAMIC_DRAW ||
         usage == GL_STREAM_DRAW;
}

bool IsValidClientState(GLenum state) {
  switch (state) {
    case GL_COLOR_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_VERTEX_ARRAY:
    case GL_POINT_SIZE_ARRAY_OES:
    case GL_WEIGHT_ARRAY_OES:
    case GL_MATRIX_INDEX_ARRAY_OES:
      return true;
    default:
      return false;
  }
}

bool IsValidColorPointerSize(GLint size) {
  return ((size >= 3) && (size <= 4));
}

bool IsValidColorPointerType(GLenum type) {
  switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_FIXED:
    case GL_FLOAT:
      return true;
  }
  return false;
}

bool IsValidCullFaceMode(GLenum mode) {
  switch (mode) {
    case GL_FRONT:
    case GL_BACK:
    case GL_FRONT_AND_BACK:
      return true;
    default:
      return false;
  }
}

bool IsValidDepthFunc(GLenum func) {
  switch (func) {
    case GL_NEVER:
    case GL_LESS:
    case GL_EQUAL:
    case GL_LEQUAL:
    case GL_GREATER:
    case GL_NOTEQUAL:
    case GL_GEQUAL:
    case GL_ALWAYS:
      return true;
    default:
      return false;
  }
}

bool IsValidDrawMode(GLenum mode) {
  switch (mode) {
    case GL_POINTS:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
    case GL_LINES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
      return true;
  }
  return false;
}

bool IsValidDrawType(GLenum mode) {
  switch (mode) {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_INT:
      return true;
  }
  return false;
}

bool IsValidEmulatedCompressedTextureFormats(GLenum type) {
  // Note: This should be kept in sync with kEmulatedCompressedTextureFormats
  // in gles_context.cpp.
  switch (type) {
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_RGB8_OES:
    case GL_PALETTE8_RGBA4_OES:
    case GL_PALETTE8_RGBA8_OES:
    case GL_ETC1_RGB8_OES:
      return true;

    default:
      return false;
  }
}

bool IsValidFramebufferAttachment(GLenum attachment) {
  switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
    case GL_DEPTH_ATTACHMENT:
    case GL_STENCIL_ATTACHMENT:
      return true;
  }
  return false;
}

bool IsValidFramebufferAttachmentParams(GLenum pname) {
  switch (pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
      return true;
  }
  return false;
}

bool IsValidFramebufferTarget(GLenum target) {
  return target == GL_FRAMEBUFFER;
}

bool IsValidFrontFaceMode(GLenum mode) {
  switch (mode) {
    case GL_CW:
    case GL_CCW:
      return true;
    default:
      return false;
  }
}

bool IsValidMatrixIndexPointerSize(GLint size) {
  return ((size >= 1) && (size <= UniformContext::kMaxVertexUnitsOES));
}

bool IsValidMatrixIndexPointerType(GLenum type) {
  return type == GL_UNSIGNED_BYTE;
}

bool IsValidMipmapHintMode(GLenum mode) {
  switch (mode) {
    case GL_FASTEST:
    case GL_NICEST:
    case GL_DONT_CARE:
      return true;

    default:
      return false;
  }
}

bool IsValidNormalPointerType(GLenum type) {
  switch (type) {
    case GL_BYTE:
    case GL_SHORT:
    case GL_FLOAT:
    case GL_FIXED:
      return true;
  }
  return false;
}

bool IsValidPixelFormat(GLenum format) {
  switch (format) {
    case GL_ALPHA:
    case GL_DEPTH_COMPONENT:
    case GL_RGB:
    case GL_RGBA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
      return true;
  }
  return false;
}

bool IsValidPixelFormatAndType(GLenum format, GLenum type) {
  switch (format) {
    case GL_RGBA:
      switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
          return true;
      }
      break;
    case GL_RGB:
      switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT_5_6_5:
          return true;
      }
      break;
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_ALPHA:
      switch (type) {
        case GL_UNSIGNED_BYTE:
          return true;
      }
      break;
  }
  return false;
}

bool IsValidPixelStoreAlignment(GLint value) {
  switch (value) {
    case 1:
    case 2:
    case 4:
    case 8:
      return true;
    default:
      return false;
  }
}

bool IsValidPixelStoreName(GLenum name) {
  switch (name) {
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
      return true;
    default:
      return false;
  }
}

bool IsValidPixelType(GLenum type) {
  switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_FLOAT:
      return true;
  }
  return false;
}

bool IsValidPointPointerType(GLenum type) {
  return type == GL_FIXED || type == GL_FLOAT;
}

bool IsValidPrecisionType(GLenum type) {
  switch (type) {
    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
    case GL_LOW_INT:
    case GL_MEDIUM_INT:
    case GL_HIGH_INT:
      return true;
  }
  return false;
}

bool IsValidRenderbufferFormat(GLenum internalformat) {
  switch (internalformat) {
    case GL_DEPTH24_STENCIL8_OES:
    case GL_DEPTH_COMPONENT16_OES:
    case GL_DEPTH_COMPONENT24_OES:
    case GL_DEPTH_COMPONENT32_OES:
    case GL_RGB565_OES:
    case GL_RGB5_A1_OES:
    case GL_RGB8_OES:
    case GL_RGBA4_OES:
    case GL_RGBA8_OES:
    case GL_STENCIL_INDEX1_OES:
    case GL_STENCIL_INDEX4_OES:
    case GL_STENCIL_INDEX8_OES:
      return true;
  }
  return false;
}

bool IsValidRenderbufferParams(GLenum pname) {
  switch (pname) {
    case GL_RENDERBUFFER_WIDTH:
    case GL_RENDERBUFFER_HEIGHT:
    case GL_RENDERBUFFER_INTERNAL_FORMAT:
    case GL_RENDERBUFFER_RED_SIZE:
    case GL_RENDERBUFFER_GREEN_SIZE:
    case GL_RENDERBUFFER_BLUE_SIZE:
    case GL_RENDERBUFFER_ALPHA_SIZE:
    case GL_RENDERBUFFER_DEPTH_SIZE:
    case GL_RENDERBUFFER_STENCIL_SIZE:
      return true;
  }
  return false;
}

bool IsValidRenderbufferTarget(GLenum target) {
  return target == GL_RENDERBUFFER;
}

bool IsValidShaderType(GLenum type) {
  return type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER;
}

bool IsValidStencilFunc(GLenum func) {
  switch (func) {
    case GL_NEVER:
    case GL_LESS:
    case GL_LEQUAL:
    case GL_GREATER:
    case GL_GEQUAL:
    case GL_EQUAL:
    case GL_NOTEQUAL:
    case GL_ALWAYS:
      return true;
    default:
      return false;
  }
}

bool IsValidTexCoordPointerSize(GLint size) {
  return ((size >= 2) && (size <= 4));
}

bool IsValidTexCoordPointerType(GLenum type) {
  switch (type) {
    case GL_BYTE:
    case GL_SHORT:
    case GL_FLOAT:
    case GL_FIXED:
      return true;
  }
  return false;
}

bool IsValidTexEnv(GLenum target, GLenum pname) {
  switch (pname) {
    case GL_TEXTURE_ENV_MODE:
    case GL_TEXTURE_ENV_COLOR:
    case GL_COMBINE_RGB:
    case GL_COMBINE_ALPHA:
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
    case GL_RGB_SCALE:
    case GL_ALPHA_SCALE:
    case GL_COORD_REPLACE_OES:
      break;
    default:
      return false;
  }
  return (target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES);
}

bool IsValidTexEnvCombineAlpha(GLenum param) {
  switch (param) {
    case GL_REPLACE:
    case GL_MODULATE:
    case GL_ADD:
    case GL_ADD_SIGNED:
    case GL_INTERPOLATE:
    case GL_SUBTRACT:
      return true;
    default:
      return false;
  }
}

bool IsValidTexEnvCombineRgb(GLenum param) {
  switch (param) {
    case GL_REPLACE:
    case GL_MODULATE:
    case GL_ADD:
    case GL_ADD_SIGNED:
    case GL_INTERPOLATE:
    case GL_SUBTRACT:
    case GL_DOT3_RGB:
    case GL_DOT3_RGBA:
      return true;
    default:
      return false;
  }
}

bool IsValidTexEnvMode(GLenum param) {
  switch (param) {
    case GL_REPLACE:
    case GL_MODULATE:
    case GL_DECAL:
    case GL_BLEND:
    case GL_ADD:
    case GL_COMBINE:
      return true;
    default:
      return false;
  }
}

bool IsValidTexEnvOperandAlpha(GLenum param) {
  switch (param) {
    case GL_SRC_ALPHA:
    case GL_ONE_MINUS_SRC_ALPHA:
      return true;
    default:
      return false;
  }
}

bool IsValidTexEnvOperandRgb(GLenum param) {
  switch (param) {
    case GL_SRC_COLOR:
    case GL_ONE_MINUS_SRC_COLOR:
    case GL_SRC_ALPHA:
    case GL_ONE_MINUS_SRC_ALPHA:
      return true;
    default:
      return false;
  }
}

bool IsValidTexEnvScale(GLfloat scale) {
  return (scale == 1.f || scale == 2.f || scale == 4.f);
}

bool IsValidTexEnvSource(GLenum param) {
  switch (param) {
    case GL_PREVIOUS:
    case GL_PRIMARY_COLOR:
    case GL_TEXTURE:
    case GL_CONSTANT:
      return true;
    default:
      return false;
  }
}

bool IsValidTexGenCoord(GLenum coord) {
  // GL_S, GL_T and GL_R are not defined in GLES1 and GLES1 headers.
  // Only GL_TEXTURE_GEN_STR_OES is supported.
  return coord == GL_TEXTURE_GEN_STR_OES;
}

bool IsValidTexGenMode(GLenum mode) {
  // opengles_spec_1_1_extension_pack.pdf 4.1
  // Only GL_NORMAL_MAP_OES & GL_REFLECTION_MAP_OES are supported.
  switch (mode) {
    case GL_NORMAL_MAP_OES:
    case GL_REFLECTION_MAP_OES:
      return true;
    default:
      return false;
  }
}

bool IsValidTextureDimensions(GLsizei width, GLsizei height,
                              GLint max_texture_size) {
  if (width < 0 || width > max_texture_size)
    return false;
  if (height < 0 || height > max_texture_size)
    return false;
  return IsPowerOf2(width) && IsPowerOf2(height);
}

bool IsValidTextureEnum(GLenum unit, GLuint max_num_textures) {
  return unit >= GL_TEXTURE0 && unit <= (GL_TEXTURE0 + max_num_textures);
}

bool IsValidTextureParam(GLenum param) {
  switch (param) {
    case GL_GENERATE_MIPMAP:
    case GL_TEXTURE_CROP_RECT_OES:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
      return true;
    default:
      return false;
  }
}

bool IsValidTextureTarget(GLenum target) {
  return target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP ||
         target == GL_TEXTURE_EXTERNAL_OES;
}

bool IsValidTextureTargetAndParam(GLenum target, GLenum pname) {
  switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_GENERATE_MIPMAP:
      break;
    default:
      return false;
  }
  return (target == GL_TEXTURE_2D) || (target == GL_TEXTURE_CUBE_MAP_OES);
}

bool IsValidTextureTargetEx(GLenum target) {
  switch (target) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
    case GL_TEXTURE_EXTERNAL_OES:
    case GL_TEXTURE_2D:
      return true;
  }
  return false;
}

bool IsValidTextureTargetLimited(GLenum target) {
  return target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES;
}

bool IsValidVertexAttribSize(GLint size) {
  return ((size >= 1) && (size <= 4));
}

bool IsValidVertexAttribType(GLenum type) {
  switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_FIXED:
    case GL_FLOAT:
      return true;
  }
  return false;
}

bool IsValidVertexPointerSize(GLint size) {
  return ((size >= 2) && (size <= 4));
}

bool IsValidVertexPointerType(GLenum type) {
  switch (type) {
    case GL_BYTE:
    case GL_SHORT:
    case GL_FLOAT:
    case GL_FIXED:
      return true;
  }
  return false;
}

bool IsValidWeightPointerSize(GLint size) {
  return ((size >= 1) && (size <= UniformContext::kMaxVertexUnitsOES));
}

bool IsValidWeightPointerType(GLenum type) {
  switch (type) {
    case GL_FIXED:
    case GL_FLOAT:
      return true;
  }
  return false;
}
