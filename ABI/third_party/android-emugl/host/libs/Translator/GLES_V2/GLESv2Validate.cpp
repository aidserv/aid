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
#include "GLESv2Validate.h"
#include <string.h>

#define LIST_VALID_TEXFORMATS(f) \
    f(GL_DEPTH_COMPONENT) \
    f(GL_DEPTH_STENCIL) \
    f(GL_RED) \
    f(GL_RED_INTEGER) \
    f(GL_RG) \
    f(GL_RGB) \
    f(GL_RGBA) \
    f(GL_RGBA_INTEGER) \
    f(GL_RGB_INTEGER) \
    f(GL_RG_INTEGER) \

#define LIST_VALID_TEXTYPES(f) \
    f(GL_BYTE) \
    f(GL_FLOAT) \
    f(GL_FLOAT_32_UNSIGNED_INT_24_8_REV) \
    f(GL_HALF_FLOAT) \
    f(GL_HALF_FLOAT_OES) \
    f(GL_INT) \
    f(GL_SHORT) \
    f(GL_UNSIGNED_BYTE) \
    f(GL_UNSIGNED_INT) \
    f(GL_UNSIGNED_INT_10F_11F_11F_REV) \
    f(GL_UNSIGNED_INT_2_10_10_10_REV) \
    f(GL_UNSIGNED_INT_24_8) \
    f(GL_UNSIGNED_INT_5_9_9_9_REV) \
    f(GL_UNSIGNED_SHORT) \
    f(GL_UNSIGNED_SHORT_4_4_4_4) \
    f(GL_UNSIGNED_SHORT_5_5_5_1) \
    f(GL_UNSIGNED_SHORT_5_6_5) \

#define LIST_VALID_TEXFORMAT_COMBINATIONS(f) \
    f(GL_R8, GL_RED, GL_UNSIGNED_BYTE) \
    f(GL_R8_SNORM, GL_RED, GL_BYTE) \
    f(GL_R16F, GL_RED, GL_FLOAT) \
    f(GL_R16F, GL_RED, GL_HALF_FLOAT) \
    f(GL_R32F, GL_RED, GL_FLOAT) \
    f(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE) \
    f(GL_R8I, GL_RED_INTEGER, GL_BYTE) \
    f(GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT) \
    f(GL_R16I, GL_RED_INTEGER, GL_SHORT) \
    f(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT) \
    f(GL_R32I, GL_RED_INTEGER, GL_INT) \
    f(GL_RG8, GL_RG, GL_UNSIGNED_BYTE) \
    f(GL_RG8_SNORM, GL_RG, GL_BYTE) \
    f(GL_RG16F, GL_RG, GL_HALF_FLOAT) \
    f(GL_RG16F, GL_RG, GL_FLOAT) \
    f(GL_RG32F, GL_RG, GL_FLOAT) \
    f(GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE) \
    f(GL_RG8I, GL_RG_INTEGER, GL_BYTE) \
    f(GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT) \
    f(GL_RG16I, GL_RG_INTEGER, GL_SHORT) \
    f(GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT) \
    f(GL_RG32I, GL_RG_INTEGER, GL_INT) \
    f(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE) \
    f(GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE) \
    f(GL_RGB565, GL_RGB, GL_UNSIGNED_BYTE) \
    f(GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5) \
    f(GL_RGB8_SNORM, GL_RGB, GL_BYTE) \
    f(GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV) \
    f(GL_R11F_G11F_B10F, GL_RGB, GL_HALF_FLOAT) \
    f(GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT) \
    f(GL_RGB9_E5, GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV) \
    f(GL_RGB9_E5, GL_RGB, GL_HALF_FLOAT) \
    f(GL_RGB9_E5, GL_RGB, GL_FLOAT) \
    f(GL_RGB16F, GL_RGB, GL_HALF_FLOAT) \
    f(GL_RGB16F, GL_RGB, GL_FLOAT) \
    f(GL_RGB32F, GL_RGB, GL_FLOAT) \
    f(GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE) \
    f(GL_RGB8I, GL_RGB_INTEGER, GL_BYTE) \
    f(GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT) \
    f(GL_RGB16I, GL_RGB_INTEGER, GL_SHORT) \
    f(GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT) \
    f(GL_RGB32I, GL_RGB_INTEGER, GL_INT) \
    f(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_RGBA8_SNORM, GL_RGBA, GL_BYTE) \
    f(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1) \
    f(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV) \
    f(GL_RGBA4, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4) \
    f(GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV) \
    f(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT) \
    f(GL_RGBA16F, GL_RGBA, GL_FLOAT) \
    f(GL_RGBA32F, GL_RGBA, GL_FLOAT) \
    f(GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE) \
    f(GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE) \
    f(GL_RGB10_A2UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV) \
    f(GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT) \
    f(GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT) \
    f(GL_RGBA32I, GL_RGBA_INTEGER, GL_INT) \
    f(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT) \
    f(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT) \
    f(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT) \
    f(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT) \
    f(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT) \
    f(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8) \
    f(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV) \
    f(GL_COMPRESSED_R11_EAC, GL_RED, GL_FLOAT) \
    f(GL_COMPRESSED_SIGNED_R11_EAC, GL_RED, GL_FLOAT) \
    f(GL_COMPRESSED_RG11_EAC, GL_RG, GL_FLOAT) \
    f(GL_COMPRESSED_SIGNED_RG11_EAC, GL_RG, GL_FLOAT) \
    f(GL_COMPRESSED_RGB8_ETC2, GL_RGB, GL_UNSIGNED_BYTE) \
    f(GL_COMPRESSED_SRGB8_ETC2, GL_RGB, GL_UNSIGNED_BYTE) \
    f(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_COMPRESSED_RGBA8_ETC2_EAC, GL_RGBA, GL_UNSIGNED_BYTE) \
    f(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, GL_RGBA, GL_UNSIGNED_BYTE) \

bool GLESv2Validate::renderbufferParam(GLEScontext* ctx, GLenum pname){
    int glesMajorVersion = ctx->getMajorVersion();
    switch(pname){
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
    case GL_RENDERBUFFER_SAMPLES:
        return glesMajorVersion >= 3;
    }
    return false;
}

bool GLESv2Validate::framebufferTarget(GLEScontext* ctx, GLenum target) {
    int glesMajorVersion = ctx->getMajorVersion();
    switch (target) {
    case GL_FRAMEBUFFER:
        return true;
    case GL_DRAW_FRAMEBUFFER:
    case GL_READ_FRAMEBUFFER:
        return glesMajorVersion >= 3;
    }
    return false;
}

bool GLESv2Validate::framebufferAttachment(GLEScontext* ctx, GLenum attachment) {
    int glesMajorVersion = ctx->getMajorVersion();
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
    case GL_DEPTH_ATTACHMENT:
    case GL_STENCIL_ATTACHMENT:
        return true;
    case GL_COLOR_ATTACHMENT1:
    case GL_COLOR_ATTACHMENT2:
    case GL_COLOR_ATTACHMENT3:
    case GL_COLOR_ATTACHMENT4:
    case GL_COLOR_ATTACHMENT5:
    case GL_COLOR_ATTACHMENT6:
    case GL_COLOR_ATTACHMENT7:
    case GL_COLOR_ATTACHMENT8:
    case GL_COLOR_ATTACHMENT9:
    case GL_COLOR_ATTACHMENT10:
    case GL_COLOR_ATTACHMENT11:
    case GL_COLOR_ATTACHMENT12:
    case GL_COLOR_ATTACHMENT13:
    case GL_COLOR_ATTACHMENT14:
    case GL_COLOR_ATTACHMENT15:
    case GL_DEPTH_STENCIL_ATTACHMENT:
        return glesMajorVersion >= 3;
    }
    return false;
}

bool GLESv2Validate::bufferTarget(GLEScontext* ctx, GLenum target) {
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch (target) {
    case GL_ARRAY_BUFFER: // Vertex attributes
    case GL_ELEMENT_ARRAY_BUFFER: // Vertex array indices
        return true;
        // GLES 3.0 buffers
    case GL_COPY_READ_BUFFER: // Buffer copy source
    case GL_COPY_WRITE_BUFFER: // Buffer copy destination
    case GL_PIXEL_PACK_BUFFER: // Pixel read target
    case GL_PIXEL_UNPACK_BUFFER: // Texture data source
    case GL_TRANSFORM_FEEDBACK_BUFFER: // Transform feedback buffer
    case GL_UNIFORM_BUFFER: // Uniform block storage
        return glesMajorVersion >= 3;
        // GLES 3.1 buffers
    case GL_ATOMIC_COUNTER_BUFFER: // Atomic counter storage
    case GL_DISPATCH_INDIRECT_BUFFER: // Indirect compute dispatch commands
    case GL_DRAW_INDIRECT_BUFFER: // Indirect command arguments
    case GL_SHADER_STORAGE_BUFFER: // Read-write storage for shaders
        return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    default:
        return false;
    }
}

bool GLESv2Validate::bufferUsage(GLEScontext* ctx, GLenum usage) {
    int glesMajorVersion = ctx->getMajorVersion();
    switch(usage) {
        case GL_STREAM_DRAW:
        case GL_STATIC_DRAW:
        case GL_DYNAMIC_DRAW:
            return true;
        case GL_STREAM_READ:
        case GL_STATIC_READ:
        case GL_DYNAMIC_READ:
        case GL_STREAM_COPY:
        case GL_STATIC_COPY:
        case GL_DYNAMIC_COPY:
            return glesMajorVersion >= 3;
    }
    return false;

}


bool GLESv2Validate::bufferParam(GLEScontext* ctx, GLenum pname) {
    int glesMajorVersion = ctx->getMajorVersion();
    switch (pname) {
    case GL_BUFFER_SIZE:
    case GL_BUFFER_USAGE:
        return true;
    case GL_BUFFER_ACCESS_FLAGS:
    case GL_BUFFER_MAPPED:
    case GL_BUFFER_MAP_LENGTH:
    case GL_BUFFER_MAP_OFFSET:
        return glesMajorVersion >= 3;
    default:
        return false;
    }
}


bool GLESv2Validate::blendEquationMode(GLEScontext* ctx, GLenum mode){
    int glesMajorVersion = ctx->getMajorVersion();
    switch (mode) {
    case GL_FUNC_ADD:
    case GL_FUNC_SUBTRACT:
    case GL_FUNC_REVERSE_SUBTRACT:
        return true;
    case GL_MIN:
    case GL_MAX:
        return glesMajorVersion >= 3;
    }
    return false;
}

bool GLESv2Validate::blendSrc(GLenum s) {
   switch(s) {
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
  }
  return false;
}


bool GLESv2Validate::blendDst(GLenum d) {
   switch(d) {
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
   }
   return false;
}

bool GLESv2Validate::textureTarget(GLEScontext* ctx, GLenum target) {
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch (target) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP:
        return true;
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_3D:
        return glesMajorVersion >= 3;
    case GL_TEXTURE_2D_MULTISAMPLE:
        return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    default:
        return false;
    }
}

bool GLESv2Validate::textureParams(GLEScontext* ctx, GLenum param) {
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch(param) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        return true;
    case GL_TEXTURE_SWIZZLE_R:
    case GL_TEXTURE_SWIZZLE_G:
    case GL_TEXTURE_SWIZZLE_B:
    case GL_TEXTURE_SWIZZLE_A:
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_IMMUTABLE_FORMAT:
    case GL_TEXTURE_IMMUTABLE_LEVELS:
        return glesMajorVersion >= 3;
    case GL_DEPTH_STENCIL_TEXTURE_MODE:
        return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    default:
        return false;
    }
}

bool GLESv2Validate::hintTargetMode(GLenum target,GLenum mode){

   switch(mode) {
   case GL_FASTEST:
   case GL_NICEST:
   case GL_DONT_CARE:
       break;
   default: return false;
   }
   return target == GL_GENERATE_MIPMAP_HINT || target == GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES;
}

bool GLESv2Validate::capability(GLenum cap){
    switch(cap){
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
        return true;
    }
    return false;
}

bool GLESv2Validate::pixelStoreParam(GLEScontext* ctx, GLenum param){
    int glesMajorVersion = ctx->getMajorVersion();
    switch(param) {
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        return true;
    case GL_PACK_ROW_LENGTH:
    case GL_PACK_SKIP_PIXELS:
    case GL_PACK_SKIP_ROWS:
    case GL_UNPACK_ROW_LENGTH:
    case GL_UNPACK_IMAGE_HEIGHT:
    case GL_UNPACK_SKIP_PIXELS:
    case GL_UNPACK_SKIP_ROWS:
    case GL_UNPACK_SKIP_IMAGES:
        return glesMajorVersion >= 3;
    default:
        return false;
    }
}

bool GLESv2Validate::readPixelFrmt(GLenum format){
    switch(format) {
    case GL_ALPHA:
    case GL_LUMINANCE_ALPHA:
    case GL_RGB:
    case GL_RGBA:
        return true;
    }
    return false;
}


bool GLESv2Validate::shaderType(GLEScontext* ctx, GLenum type){
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch (type) {
    case GL_VERTEX_SHADER:
    case GL_FRAGMENT_SHADER:
        return true;
    case GL_COMPUTE_SHADER:
        return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    }
    return false;
}

bool GLESv2Validate::precisionType(GLenum type){
    switch(type){
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

bool GLESv2Validate::arrayIndex(GLEScontext * ctx,GLuint index) {
    return index < (GLuint)ctx->getCaps()->maxVertexAttribs;
}

#define GL_RED                              0x1903
#define GL_RG                               0x8227
#define GL_R8                               0x8229
#define GL_RG8                              0x822B
#define GL_R16F                             0x822D
#define GL_RG16F                            0x822F
#define GL_RGBA16F                          0x881A
#define GL_RGB16F                           0x881B
#define GL_R11F_G11F_B10F                   0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV     0x8C3B

bool GLESv2Validate::pixelType(GLEScontext * ctx,GLenum type) {
    int glesMajorVersion = ctx->getMajorVersion();
    if (glesMajorVersion < 3) {
        if(type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT
                || type == GL_UNSIGNED_INT_10F_11F_11F_REV)
            return true;

        return GLESvalidate::pixelType(ctx, type);
    }

#define GLES3_TYPE_CASE(type) \
    case type: \

    switch (type) {
        LIST_VALID_TEXTYPES(GLES3_TYPE_CASE)
            return glesMajorVersion >= 3;
        default:
            break;
    }

    return false;
}

bool GLESv2Validate::pixelFrmt(GLEScontext* ctx,GLenum format) {
    int glesMajorVersion = ctx->getMajorVersion();
    if (glesMajorVersion < 3) {
        switch (format) {
            case GL_DEPTH_COMPONENT:
                // GLES3 compatible
                // Required in dEQP
            case GL_RED:
            case GL_RG:
                return true;
        }
        return GLESvalidate::pixelFrmt(ctx, format);
    }

#define GLES3_FORMAT_CASE(format) \
    case format:

    switch (format) {
        LIST_VALID_TEXFORMATS(GLES3_FORMAT_CASE)
            return glesMajorVersion >= 3;
        default:
            break;
    }
    return GLESvalidate::pixelFrmt(ctx, format);
}

bool GLESv2Validate::pixelItnlFrmt(GLEScontext* ctx ,GLenum internalformat) {
    int glesMajorVersion = ctx->getMajorVersion();
    switch (internalformat) {
    case GL_R8:
    case GL_RG8:
    case GL_R16F:
    case GL_RG16F:
    case GL_RGBA16F:
    case GL_RGB16F:
    case GL_R11F_G11F_B10F:
    case GL_RGB8:
    case GL_RGBA8:
        return true;
    case GL_R8_SNORM:
    case GL_R32F:
    case GL_R8UI:
    case GL_R8I:
    case GL_R16UI:
    case GL_R16I:
    case GL_R32UI:
    case GL_R32I:
    case GL_RG8_SNORM:
    case GL_RG32F:
    case GL_RG8UI:
    case GL_RG8I:
    case GL_RG16UI:
    case GL_RG16I:
    case GL_RG32UI:
    case GL_RG32I:
    case GL_SRGB8:
    case GL_RGB565:
    case GL_RGB8_SNORM:
    case GL_RGB9_E5:
    case GL_RGB32F:
    case GL_RGB8UI:
    case GL_RGB8I:
    case GL_RGB16UI:
    case GL_RGB16I:
    case GL_RGB32UI:
    case GL_RGB32I:
    case GL_SRGB8_ALPHA8:
    case GL_RGBA8_SNORM:
    case GL_RGB5_A1:
    case GL_RGBA4:
    case GL_RGB10_A2:
    case GL_RGBA32F:
    case GL_RGBA8UI:
    case GL_RGBA8I:
    case GL_RGB10_A2UI:
    case GL_RGBA16UI:
    case GL_RGBA16I:
    case GL_RGBA32I:
    case GL_RGBA32UI:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        if (glesMajorVersion >= 3) {
            return true;
        }
        break;
    }
    return pixelFrmt(ctx, internalformat);
}

bool GLESv2Validate::pixelSizedFrmt(GLEScontext* ctx, GLenum internalformat,
                                    GLenum format, GLenum type) {
    int glesMajorVersion = ctx->getMajorVersion();
    if (internalformat == format) {
        return true;
    }

    if (glesMajorVersion < 3) {
        switch (format) {
            case GL_RED:
                switch (type) {
                    case GL_UNSIGNED_BYTE:
                        return internalformat == GL_R8;
                    case GL_HALF_FLOAT:
                    case GL_FLOAT:
                        return internalformat == GL_R16F;
                    case GL_BYTE:
                        return internalformat == GL_R8_SNORM;
                    default:
                        return false;
                }
                break;
            case GL_RG:
                switch (type) {
                    case GL_UNSIGNED_BYTE:
                        return internalformat == GL_RG8;
                    case GL_HALF_FLOAT:
                    case GL_FLOAT:
                        return internalformat == GL_RG16F;
                    default:
                        return false;
                }
                break;
            case GL_RGB:
                switch (type) {
                    case GL_HALF_FLOAT:
                    case GL_FLOAT:
                        return internalformat == GL_RGB16F
                            || internalformat == GL_R11F_G11F_B10F;
                    case GL_UNSIGNED_INT_10F_11F_11F_REV:
                        return internalformat == GL_R11F_G11F_B10F;
                    default:
                        return internalformat == GL_RGB8 ||
                               internalformat == GL_RGB;
                }
                break;
            case GL_RGBA:
                switch (type) {
                    case GL_HALF_FLOAT:
                    case GL_FLOAT:
                        return internalformat == GL_RGBA16F;
                    default:
                        return internalformat == GL_RGBA8 ||
                               internalformat == GL_RGBA;
                }
                break;
        }
    }

#define VALIDATE_FORMAT_COMBINATION(x, y, z) \
    if (internalformat == x && format == y && type == z) return true; \

    LIST_VALID_TEXFORMAT_COMBINATIONS(VALIDATE_FORMAT_COMBINATION)

    return false;
}

bool GLESv2Validate::isCompressedFormat(GLenum format) {
    switch (format) {
        case GL_ETC1_RGB8_OES:
        case GL_COMPRESSED_R11_EAC:
        case GL_COMPRESSED_SIGNED_R11_EAC:
        case GL_COMPRESSED_RG11_EAC:
        case GL_COMPRESSED_SIGNED_RG11_EAC:
        case GL_COMPRESSED_RGB8_ETC2:
        case GL_COMPRESSED_SRGB8_ETC2:
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return true;
    }
    return false;
}

void GLESv2Validate::getCompatibleFormatTypeForInternalFormat(GLenum internalformat, GLenum* format_out, GLenum* type_out) {
#define RETURN_COMPATIBLE_FORMAT(x, y, z) \
    if (internalformat == x) { \
        *format_out = y; \
        *type_out = z; \
        return; \
    } \

    LIST_VALID_TEXFORMAT_COMBINATIONS(RETURN_COMPATIBLE_FORMAT)
}
bool GLESv2Validate::attribName(const GLchar* name){
    const GLchar* found = strstr(name,"gl_");
    return (!found) ||
           (found != name) ; // attrib name does not start with gl_
}

bool GLESv2Validate::attribIndex(int index, int max){
    return index >=0 && index < max;
}

bool GLESv2Validate::programParam(GLEScontext* ctx, GLenum pname){
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch(pname){
        case GL_DELETE_STATUS:
        case GL_LINK_STATUS:
        case GL_VALIDATE_STATUS:
        case GL_INFO_LOG_LENGTH:
        case GL_ATTACHED_SHADERS:
        case GL_ACTIVE_ATTRIBUTES:
        case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        case GL_ACTIVE_UNIFORMS:
        case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            return true;
        case GL_ACTIVE_UNIFORM_BLOCKS:
        case GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH:
        case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
        case GL_PROGRAM_BINARY_LENGTH:
        case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
        case GL_TRANSFORM_FEEDBACK_VARYINGS:
        case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH:
        case GL_PROGRAM_SEPARABLE:
            return glesMajorVersion >= 3;
        case GL_ACTIVE_ATOMIC_COUNTER_BUFFERS:
        case GL_COMPUTE_WORK_GROUP_SIZE:
            return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    }
    return false;
}

bool GLESv2Validate::textureIsCubeMap(GLenum target){
    switch(target){
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return true;
    }
    return false;
}

bool GLESv2Validate::textureTargetEx(GLEScontext* ctx, GLenum textarget) {
    int glesMajorVersion = ctx->getMajorVersion();
    int glesMinorVersion = ctx->getMinorVersion();
    switch (textarget) {
    case GL_TEXTURE_2D_MULTISAMPLE:
        return glesMajorVersion >= 3 && glesMinorVersion >= 1;
    default:
        return GLESvalidate::textureTargetEx(textarget);
    }
}

int GLESv2Validate::sizeOfType(GLenum type) {
    size_t retval = 0;
    switch(type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        retval = 1;
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
    case GL_HALF_FLOAT_OES:
        retval = 2;
        break;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT:
    case GL_FIXED:
    case GL_BOOL:
        retval =  4;
        break;
#ifdef GL_DOUBLE
    case GL_DOUBLE:
        retval = 8;
        break;
#endif
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
    case GL_BOOL_VEC2:
        retval = 8;
        break;
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
    case GL_BOOL_VEC3:
    case GL_FLOAT_VEC3:
        retval = 12;
        break;
    case GL_FLOAT_VEC4:
    case GL_BOOL_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
    case GL_FLOAT_MAT2:
        retval = 16;
        break;
    case GL_FLOAT_MAT3:
        retval = 36;
        break;
    case GL_FLOAT_MAT4:
        retval = 64;
        break;
    case GL_FLOAT_MAT2x3:
    case GL_FLOAT_MAT3x2:
        retval = 4 * 6;
        break;
    case GL_FLOAT_MAT2x4:
    case GL_FLOAT_MAT4x2:
        retval = 4 * 8;
        break;
    case GL_FLOAT_MAT3x4:
    case GL_FLOAT_MAT4x3:
        retval = 4 * 12;
        break;
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
        retval = 4;
        break;
    case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
        retval = 2;
        break;
	case GL_INT_2_10_10_10_REV:
	case GL_UNSIGNED_INT_10F_11F_11F_REV:
	case GL_UNSIGNED_INT_5_9_9_9_REV:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_UNSIGNED_INT_24_8_OES:;
        retval = 4;
        break;
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
		retval = 4 + 4;
        break;
    default:
        fprintf(stderr, "%s: WARNING: unknown type 0x%x. assuming 32 bits.\n", __FUNCTION__, type);
        retval = 4;
    }
    return retval;
}

