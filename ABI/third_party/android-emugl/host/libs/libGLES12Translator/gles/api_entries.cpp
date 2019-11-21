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

#include <ETC1/etc1.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "common/alog.h"
#include "common/dlog.h"
#include "gles/buffer_data.h"
#include "gles/debug.h"
#include "gles/egl_image.h"
#include "gles/framebuffer_data.h"
#include "gles/gles_context.h"
#include "gles/gles_utils.h"
#include "gles/gles_validate.h"
#include "gles/macros.h"
#include "gles/paletted_texture_util.h"
#include "gles/program_data.h"
#include "gles/renderbuffer_data.h"
#include "gles/shader_data.h"
#include "gles/texture_codecs.h"
#include "gles/texture_data.h"

#include "gles/gles12_internal_dispatch.h"

typedef GlesContext* ContextPtr;

namespace {

    static const size_t kMaxParamElementSize = 16;

    enum ParamType {
        kParamTypeArray,
        kParamTypeScalar,
    };

    enum HandlingKind {
        kHandlingKindInvalid = 0,
        kHandlingKindLocalCopy = 1 << 0,    // Track capability in context.
        kHandlingKindTexture = 1 << 1,      // Track capability in texture context.
        kHandlingKindUniform = 1 << 2,      // Track capability in uniform context.
        kHandlingKindIgnored = 1 << 3,      // Unsupported capability, but can be
        // ignored.
        kHandlingKindUnsupported = 1 << 4,  // Unsupported capability.
        kHandlingKindPropagate = 1 << 5,    // Pass through to implementation.
    };

    // The calls to Enable and Disable are both handled very similarly, and we can
    // eliminate some code duplication by having a common set of logic that knows
    // how each capability should be handled.
    int GetCapabilityHandlingKind(GLenum cap) {
        switch (cap) {
            // These capabilities represent state we do not need to emulate.
            case GL_BLEND:
            case GL_CULL_FACE:
            case GL_DEPTH_TEST:
            case GL_DITHER:
            case GL_POLYGON_OFFSET_FILL:
            case GL_SAMPLE_ALPHA_TO_COVERAGE:
            case GL_SAMPLE_COVERAGE:
            case GL_SCISSOR_TEST:
            case GL_STENCIL_TEST:
                return kHandlingKindPropagate | kHandlingKindLocalCopy;

                // These capabilities represent GLES1 state we must emulate under GLES2.
            case GL_ALPHA_TEST:
            case GL_CLIP_PLANE0:
            case GL_CLIP_PLANE1:
            case GL_CLIP_PLANE2:
            case GL_CLIP_PLANE3:
            case GL_CLIP_PLANE4:
            case GL_CLIP_PLANE5:
            case GL_COLOR_MATERIAL:
            case GL_FOG:
            case GL_LIGHTING:
            case GL_LIGHT0:
            case GL_LIGHT1:
            case GL_LIGHT2:
            case GL_LIGHT3:
            case GL_LIGHT4:
            case GL_LIGHT5:
            case GL_LIGHT6:
            case GL_LIGHT7:
            case GL_MATRIX_PALETTE_OES:
            case GL_NORMALIZE:
            case GL_POINT_SMOOTH:
            case GL_POINT_SPRITE_OES:
            case GL_RESCALE_NORMAL:
                return kHandlingKindLocalCopy;

            case GL_TEXTURE_GEN_STR_OES:
                return kHandlingKindUniform;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_EXTERNAL_OES:
            case GL_TEXTURE_CUBE_MAP:
                return kHandlingKindTexture;

                // GL_LINE_SMOOTH is not supported, but ignore it for now.
            case GL_LINE_SMOOTH:
                return kHandlingKindIgnored | kHandlingKindUnsupported;

                // These capabilities represent GLES1 state which are not supported for
                // now.  It is an error to try to enable these, but disabling them or
                // testing for them is okay.
            case GL_COLOR_LOGIC_OP:
            case GL_MULTISAMPLE:
                return kHandlingKindUnsupported;

            default:
                return kHandlingKindInvalid;
        }
    }

    // Determines the number of elements for a given parameter.
    size_t ParamSize(GLenum param) {
        switch (param) {
            case GL_ALPHA_TEST_FUNC:
            case GL_ALPHA_TEST_REF:
            case GL_BLEND_DST:
            case GL_BLEND_SRC:
            case GL_CONSTANT_ATTENUATION:
            case GL_CULL_FACE_MODE:
            case GL_DEPTH_CLEAR_VALUE:
            case GL_DEPTH_WRITEMASK:
            case GL_FOG_DENSITY:
            case GL_FOG_END:
            case GL_FOG_MODE:
            case GL_FOG_START:
            case GL_FRONT_FACE:
            case GL_GENERATE_MIPMAP:
            case GL_LIGHT_MODEL_TWO_SIDE:
            case GL_LINEAR_ATTENUATION:
            case GL_LOGIC_OP_MODE:
            case GL_MATRIX_MODE:
            case GL_MAX_TEXTURE_SIZE:
            case GL_MAX_TEXTURE_UNITS:
            case GL_MODELVIEW_STACK_DEPTH:
            case GL_POINT_FADE_THRESHOLD_SIZE:
            case GL_POINT_SIZE:
            case GL_POINT_SIZE_MAX:
            case GL_POINT_SIZE_MIN:
            case GL_PROJECTION_STACK_DEPTH:
            case GL_QUADRATIC_ATTENUATION:
            case GL_SCISSOR_TEST:
            case GL_SHADE_MODEL:
            case GL_SHININESS:
            case GL_SPOT_EXPONENT:
            case GL_STENCIL_FAIL:
            case GL_STENCIL_PASS_DEPTH_FAIL:
            case GL_STENCIL_PASS_DEPTH_PASS:
            case GL_STENCIL_REF:
            case GL_STENCIL_WRITEMASK:
            case GL_TEXTURE_ENV_MODE:
            case GL_TEXTURE_MAG_FILTER:
            case GL_TEXTURE_MIN_FILTER:
            case GL_TEXTURE_STACK_DEPTH:
            case GL_TEXTURE_WRAP_S:
            case GL_TEXTURE_WRAP_T:
                return 1;
            case GL_ALIASED_LINE_WIDTH_RANGE:
            case GL_ALIASED_POINT_SIZE_RANGE:
            case GL_DEPTH_RANGE:
            case GL_MAX_VIEWPORT_DIMS:
            case GL_SMOOTH_LINE_WIDTH_RANGE:
            case GL_SMOOTH_POINT_SIZE_RANGE:
                return 2;
            case GL_CURRENT_NORMAL:
            case GL_POINT_DISTANCE_ATTENUATION:
            case GL_SPOT_DIRECTION:
                return 3;
            case GL_AMBIENT:
            case GL_CURRENT_COLOR:
            case GL_CURRENT_TEXTURE_COORDS:
            case GL_DIFFUSE:
            case GL_EMISSION:
            case GL_FOG_COLOR:
            case GL_LIGHT_MODEL_AMBIENT:
            case GL_POSITION:
            case GL_SCISSOR_BOX:
            case GL_SPECULAR:
            case GL_TEXTURE_CROP_RECT_OES:
            case GL_TEXTURE_ENV_COLOR:
            case GL_VIEWPORT:
                return 4;
            case GL_MODELVIEW_MATRIX:
            case GL_PROJECTION_MATRIX:
            case GL_TEXTURE_MATRIX:
                return 16;
            default:
                LOG_ALWAYS_FATAL("Unknown parameter name: %s (%x)", GetEnumString(param),
                        param);
                return 1;
        }
    }

    void Convert(GLfloat* data, size_t num, const GLfixed* params) {
        for (size_t i = 0; i < num; ++i) {
            data[i] = X2F(params[i]);
        }
    }

    void Convert(GLfixed* data, size_t num, const GLfloat* params) {
        for (size_t i = 0; i < num; ++i) {
            data[i] = F2X(params[i]);
        }
    }

    GLuint ArrayEnumToIndex(const ContextPtr& c, GLenum array) {
        switch (array) {
            case GL_VERTEX_ARRAY:
            case GL_VERTEX_ARRAY_POINTER:
                return kPositionVertexAttribute;
            case GL_NORMAL_ARRAY:
            case GL_NORMAL_ARRAY_POINTER:
                return kNormalVertexAttribute;
            case GL_COLOR_ARRAY:
            case GL_COLOR_ARRAY_POINTER:
                return kColorVertexAttribute;
            case GL_POINT_SIZE_ARRAY_OES:
            case GL_POINT_SIZE_ARRAY_POINTER_OES:
                return kPointSizeVertexAttribute;
            case GL_TEXTURE_COORD_ARRAY:
            case GL_TEXTURE_COORD_ARRAY_POINTER:
                return c->texture_context_.GetClientActiveTextureCoordAttrib();
            case GL_WEIGHT_ARRAY_OES:
            case GL_WEIGHT_ARRAY_POINTER_OES:
                return kWeightVertexAttribute;
            case GL_MATRIX_INDEX_ARRAY_OES:
            case GL_MATRIX_INDEX_ARRAY_POINTER_OES:
                return kMatrixIndexVertexAttribute;
            default:
                LOG_ALWAYS_FATAL("Unknown array %s(0x%x)", GetEnumString(array), array);
        }
    }

}  // namespace

// Unless we separate "entry points" by users of the library
// from internal uses of gl functions, we could end up in
// a situation where the host system's GL driver shadows
// OpenGL function names, causing segfaults and other badness.

// These macros do the following:
// 1. Forward-declare GLESv1 entry points.
// 2. Forward-declare "Implementation" functions that are then
//    called by the entry points.
// 3. Define the entry points in terms of just directly calling
//    the implementation.
// 4. Define a struct holding the ***Impl functions.
// 5. Make all calls to GLESv1 internal to translator use
//    ***Impl functions inside the struct, instead of a
//    global GLESv1 entry point. This avoids the shadowing.

#define FWD_DECLARE(return_type, func_name, signature, args) \
    EXPORT_DECL return_type func_name signature;

#ifdef _WIN32
#define FWD_DECLARE_IMPL(return_type, func_name, signature, args) \
    return_type __stdcall func_name##Impl signature;
#else
#define FWD_DECLARE_IMPL(return_type, func_name, signature, args) \
    return_type func_name##Impl signature;
#endif

#define DO_RETURN(return_type, func_name, args) DO_RETURN##return_type(func_name, args)

#define DO_RETURNvoid(func_name, args) func_name args;
#define DO_RETURNGLenum(func_name, args) return func_name args;
#define DO_RETURNGLuint(func_name, args) return func_name args;
#define DO_RETURNGLint(func_name, args) return func_name args;
#define DO_RETURNGLboolean(func_name, args) return func_name args;
#define DO_RETURNGLvoidptr(func_name, args) return func_name args;
#define DO_RETURNconstGLubyte(func_name, args) return func_name args;

#define DEFINE_ENTRYPT(return_type, func_name, signature, args) \
    EXPORT_DECL return_type func_name signature { \
        DO_RETURN(return_type, func_name##Impl, args) \
    }

LIST_GLES12TR_INTERNAL_FUNCTIONS(FWD_DECLARE)
LIST_GLES12TR_INTERNAL_FUNCTIONS(FWD_DECLARE_IMPL)
LIST_GLES12TR_INTERNAL_FUNCTIONS(DEFINE_ENTRYPT)

static GLES12TranslatorFuncs sTranslatorFuncs = {
#define ASSIGN_TRANSLATOR_FUNC(return_type, func_name, signature, callargs) \
    .internal##func_name = func_name##Impl,
    LIST_GLES12TR_INTERNAL_FUNCTIONS(ASSIGN_TRANSLATOR_FUNC)
};

#define TRCALL(X) sTranslatorFuncs.internal##X

#ifdef _WIN32
#define APIENTRY_IMPL(_return, _name, ...) \
    _return __stdcall gl##_name##Impl(__VA_ARGS__)
#else
#define APIENTRY_IMPL(_return, _name, ...) \
    _return gl##_name##Impl(__VA_ARGS__)
#endif

// Actual implementations of the GLESv1->2 functions are defined below.

// Selects the server texture unit state that will be modified by server
// texture state calls.
APIENTRY_IMPL(void, ActiveTexture, GLenum texture) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }
    if (!c->uniform_context_.SetActiveTexture(texture)) {
        GLES_ERROR_INVALID_ENUM(texture);
    }
    if (!c->texture_context_.SetActiveTexture(texture)) {
        GLES_ERROR_INVALID_ENUM(texture);
    }
    PASS_THROUGH(c, ActiveTexture, texture);
}

// Discard pixel writes based on an alpha value test.
APIENTRY_IMPL(void, AlphaFunc, GLenum func, GLclampf ref) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidAlphaFunc(func)) {
        GLES_ERROR_INVALID_ENUM(func);
        return;
    }

    AlphaTest& alpha = c->uniform_context_.MutateAlphaTest();
    alpha.func = func;
    alpha.value = ClampValue(ref, 0.f, 1.f);
}

APIENTRY_IMPL(void, AlphaFuncx, GLenum func, GLclampx ref) {
    APITRACE();
    TRCALL(glAlphaFunc(func, X2F(ref)));
}

APIENTRY_IMPL(void, AlphaFuncxOES, GLenum func, GLclampx ref) {
    APITRACE();
    TRCALL(glAlphaFunc(func, X2F(ref)));
}

// Attaches a shader object to a program object.
APIENTRY_IMPL(void, AttachShader, GLuint program, GLuint shader) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid shader %d", shader);
        return;
    }
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->AttachShader(shader_data);
}

// Requests a named shader attribute be assigned a particular attribute index.
APIENTRY_IMPL(void, BindAttribLocation, GLuint program, GLuint index, const GLchar* name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->BindAttribLocation(index, name);
}

// Bind the specified buffer as vertex buffer or index buffer.
APIENTRY_IMPL(void, BindBuffer, GLenum target, GLuint buffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();

    BufferDataPtr obj = sg->GetBufferData(buffer);
    if (obj == NULL && buffer != 0) {
        sg->CreateBufferData(buffer);
    }

    if (target == GL_ARRAY_BUFFER) {
        c->array_buffer_binding_ = buffer;
    } else {
        c->element_buffer_binding_ = buffer;
    }
    PASS_THROUGH(c, BindBuffer, target, sg->GetBufferGlobalName(buffer));
}

// Specify framebuffer for off-screen rendering.
APIENTRY_IMPL(void, BindFramebuffer, GLenum target, GLuint framebuffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidFramebufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();

    FramebufferDataPtr obj = sg->GetFramebufferData(framebuffer);
    if (obj == NULL && framebuffer != 0) {
        sg->CreateFramebufferData(framebuffer);
    }
    c->BindFramebuffer(framebuffer);
}

APIENTRY_IMPL(void, BindFramebufferOES, GLenum target, GLuint framebuffer) {
    APITRACE();
    TRCALL(glBindFramebuffer(target, framebuffer));
}

// Specify the renderbuffer that will be modified by renderbuffer calls.
APIENTRY_IMPL(void, BindRenderbuffer, GLenum target, GLuint renderbuffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidRenderbufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();

    RenderbufferDataPtr obj = sg->GetRenderbufferData(renderbuffer);
    if (obj == NULL && renderbuffer != 0) {
        sg->CreateRenderbufferData(renderbuffer);
    }

    c->renderbuffer_binding_ = renderbuffer;
    const GLuint global_name = sg->GetRenderbufferGlobalName(renderbuffer);
    PASS_THROUGH(c, BindRenderbuffer, GL_RENDERBUFFER, global_name);
}

APIENTRY_IMPL(void, BindRenderbufferOES, GLenum target, GLuint renderbuffer) {
    APITRACE();
    TRCALL(glBindRenderbuffer(target, renderbuffer));
}

// This call makes the given texture name the current texture of
// kind "target" (which is usually GL_TEXTURE_2D) which later
// environment/parameter calls will reference, and S/T texture
// mapping coordinates apply to.
APIENTRY_IMPL(void, BindTexture, GLenum target, GLuint texture) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTextureTarget(target)) {
        DLOG("Not a valid texture target!");
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    DLOG("Get share group");
    ShareGroupPtr sg = c->GetShareGroup();

    bool first_use = false;
    TextureDataPtr tex;

    // Handle special-case 0 texture.
    if (texture == 0) {
        DLOG("special-case 0 texture");
        tex = c->texture_context_.GetDefaultTextureData(target);
    } else {
        DLOG("Not the 0 texture");
        tex = sg->GetTextureData(texture);
        DLOG("Got texture data");
        if (tex == NULL) {
            tex = sg->CreateTextureData(texture);
            first_use = true;
        }
    }
    if (tex == NULL) {
        DLOG("tex is null, quit!");
    }
    LOG_ALWAYS_FATAL_IF(tex == NULL);
    if (!c->texture_context_.BindTextureToTarget(tex, target)) {
        GLES_ERROR(GL_INVALID_OPERATION, "Texture cannot be bound to target: %s",
                GetEnumString(target));
    }

    // Most textures use the same global texture target as used by the caller.
    // There is special class of exceptions -- external images targets, which
    // could be any target (potentially not even a normally valid GLES2 target!)
    bool init_external_oes_as_2d = false;
    GLenum global_texture_target = target;
    if (tex->IsEglImageAttached()) {
        DLOG("has egl image attached!");
        global_texture_target = tex->GetAttachedEglImage()->global_texture_target;
    } else if (target == GL_TEXTURE_EXTERNAL_OES) {
        DLOG("is external texture");
        // It is perfectly acceptable but problematic here to call
        // BindTexture(TEXTURE_EXTERNAL_OES, some_texture) before calling
        // EGLImageTargetTexture2DOES(). Since we are emulating
        // TEXTURE_EXTERNAL_OES, we do not want to pass it on to the underlying
        // implementation. So we assume that the global target should be TEXTURE_2D
        // for this case. If for some reason that is not the actual global target,
        // we will fix it up in handling the EGLImageTargetTexture2DOES call.
        global_texture_target = GL_TEXTURE_2D;
        init_external_oes_as_2d = first_use;
    }
    c->texture_context_.SetTargetTexture(target, texture, global_texture_target);

    if (init_external_oes_as_2d) {
        DLOG("init_external_oes_as_2d true");
        // When a new TEXTURE_2D is used to back the TEXTURE_EXTERNAL_OES,
        // we need to set the default texture state to match that default state
        // required by the extension, where different from the defaults for a
        // TEXTURE_2D.
        const GLenum global_target =
            c->texture_context_.EnsureCorrectBinding(target);
        LOG_ALWAYS_FATAL_IF(global_target != GL_TEXTURE_2D);
        PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);
        PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);
        PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);
        c->texture_context_.RestoreBinding(target);
    }

    // Only immediately bind the texture to the target if the global texture
    // target matches the local target. If we are remapping the texture target,
    // we will not know until later which texture should be bound.
    if (target == global_texture_target) {
        DLOG("target is global texture target, immediately call underlying bind");
        const GLuint global_texture_name =
            c->GetShareGroup()->GetTextureGlobalName(texture);
        PASS_THROUGH(c, BindTexture, target, global_texture_name);
    } 
    DLOG("exit function");
}

// Used in compositing the fragment shader output with the framebuffer.
APIENTRY_IMPL(void, BlendColor, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, BlendColor, red, green, blue, alpha);
}

// Used in compositing the fragment shader output with the framebuffer.
APIENTRY_IMPL(void, BlendEquation, GLenum mode) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, BlendEquation, mode);
}

APIENTRY_IMPL(void, BlendEquationOES, GLenum mode) {
    APITRACE();
    TRCALL(glBlendEquation(mode));
}

// Used in compositing the fragment shader output with the framebuffer.
APIENTRY_IMPL(void, BlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, BlendEquationSeparate, modeRGB, modeAlpha);
}

APIENTRY_IMPL(void, BlendEquationSeparateOES, GLenum modeRGB, GLenum modeAlpha) {
    APITRACE();
    TRCALL(glBlendEquationSeparate(modeRGB, modeAlpha));
}

// Used in compositing the fragment shader output with the framebuffer.
APIENTRY_IMPL(void, BlendFunc, GLenum sfactor, GLenum dfactor) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBlendFunc(sfactor)) {
        GLES_ERROR_INVALID_ENUM(sfactor);
        return;
    }
    if (!IsValidBlendFunc(dfactor)) {
        GLES_ERROR_INVALID_ENUM(dfactor);
        return;
    }

    c->blend_func_src_alpha_.Mutate() = sfactor;
    c->blend_func_src_rgb_.Mutate() = sfactor;
    c->blend_func_dst_alpha_.Mutate() = dfactor;
    c->blend_func_dst_rgb_.Mutate() = dfactor;
    PASS_THROUGH(c, BlendFunc, sfactor, dfactor);
}

// Used in compositing the fragment shader output with the framebuffer.
APIENTRY_IMPL(void, BlendFuncSeparate, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBlendFunc(srcRGB)) {
        GLES_ERROR_INVALID_ENUM(srcRGB);
        return;
    }
    if (!IsValidBlendFunc(dstRGB)) {
        GLES_ERROR_INVALID_ENUM(dstRGB);
        return;
    }
    if (!IsValidBlendFunc(srcAlpha)) {
        GLES_ERROR_INVALID_ENUM(srcAlpha);
        return;
    }
    if (!IsValidBlendFunc(dstAlpha)) {
        GLES_ERROR_INVALID_ENUM(dstAlpha);
        return;
    }

    c->blend_func_src_alpha_.Mutate() = srcAlpha;
    c->blend_func_src_rgb_.Mutate() = srcRGB;
    c->blend_func_dst_alpha_.Mutate() = dstAlpha;
    c->blend_func_dst_rgb_.Mutate() = dstRGB;
    PASS_THROUGH(c, BlendFuncSeparate, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

APIENTRY_IMPL(void, BlendFuncSeparateOES, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    APITRACE();
    TRCALL(glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha));
}

// Loads arbitrary binary data into a buffer.
APIENTRY_IMPL(void, BufferData, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidBufferUsage(usage)) {
        GLES_ERROR_INVALID_ENUM(usage);
        return;
    }
    if (size < 0) {
        GLES_ERROR_INVALID_VALUE_UINT((GLuint)size);
        return;
    }
    BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
    if (vbo == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Buffer not bound.");
        return;
    }
    vbo->SetBufferData(target, size, data, usage);
    PASS_THROUGH(c, BufferData, target, size, data, usage);
}

// Loads arbitrary binary data into a portion of a buffer.
APIENTRY_IMPL(void, BufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (size < 0) {
        GLES_ERROR_INVALID_VALUE_UINT((GLuint)size);
        return;
    }
    if (offset < 0) {
        GLES_ERROR_INVALID_VALUE_UINT((GLuint)offset);
        return;
    }
    BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
    if (vbo == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Buffer not bound.");
        return;
    }

    if (static_cast<GLuint>(offset + size) > vbo->GetSize()) {
        GLES_ERROR(GL_INVALID_VALUE, "Unable to set buffer subdata.");
        return;
    }
    vbo->SetBufferSubData(target, offset, size, data);
    PASS_THROUGH(c, BufferSubData, target, offset, size, data);
}

// Verifies if a framebuffer object is set up correctly.
APIENTRY_IMPL(GLenum, CheckFramebufferStatus, GLenum target) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }
    return PASS_THROUGH(c, CheckFramebufferStatus, target);
}

APIENTRY_IMPL(GLenum, CheckFramebufferStatusOES, GLenum target) {
    APITRACE();
    return TRCALL(glCheckFramebufferStatus(target));
}

// Clears the framebuffer to the current clear color.
APIENTRY_IMPL(void, Clear, GLbitfield mask) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }
    c->EnsureSurfaceReadyToDraw();
    PASS_THROUGH(c, Clear, mask);
}

// Sets the color to use when clearing the framebuffer.
APIENTRY_IMPL(void, ClearColor, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    red = ClampValue(red, 0.f, 1.f);
    green = ClampValue(green, 0.f, 1.f);
    blue = ClampValue(blue, 0.f, 1.f);
    alpha = ClampValue(alpha, 0.f, 1.f);

    GLfloat (&color_clear_value)[4] = c->color_clear_value_.Mutate();
    color_clear_value[0] = red;
    color_clear_value[1] = green;
    color_clear_value[2] = blue;
    color_clear_value[3] = alpha;
    PASS_THROUGH(c, ClearColor, red, green, blue, alpha);
}

APIENTRY_IMPL(void, ClearColorx, GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
    APITRACE();
    TRCALL(glClearColor(X2F(red), X2F(green), X2F(blue), X2F(alpha)));
}

APIENTRY_IMPL(void, ClearColorxOES, GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
    APITRACE();
    TRCALL(glClearColor(X2F(red), X2F(green), X2F(blue), X2F(alpha)));
}

// Sets the depth value to use when clearing the framebuffer.
APIENTRY_IMPL(void, ClearDepthf, GLfloat depth) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    depth = ClampValue(depth, 0.f, 1.f);

    c->depth_clear_value_.Mutate() = depth;
    PASS_THROUGH(c, ClearDepthf, depth);
}

APIENTRY_IMPL(void, ClearDepthfOES, GLclampf depth) {
    APITRACE();
    TRCALL(glClearDepthf(depth));
}

APIENTRY_IMPL(void, ClearDepthx, GLclampx depth) {
    APITRACE();
    TRCALL(glClearDepthf(X2F(depth)));
}

APIENTRY_IMPL(void, ClearDepthxOES, GLclampx depth) {
    APITRACE();
    TRCALL(glClearDepthf(X2F(depth)));
}

// Sets the stencil bit pattern to use when clearing the framebuffer.
APIENTRY_IMPL(void, ClearStencil, GLint s) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, ClearStencil, s);
}

// Selects the client texture unit state that will be modified by client
// texture state calls.
APIENTRY_IMPL(void, ClientActiveTexture, GLenum texture) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!c->texture_context_.SetClientActiveTexture(texture)) {
        GLES_ERROR_INVALID_ENUM(texture);
    }
}

// Specify a plane against which all geometry is clipped.
APIENTRY_IMPL(void, ClipPlanef, GLenum pname, const GLfloat* equation) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector* plane = c->uniform_context_.MutateClipPlane(pname);
    if (!plane) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    // Convert the plane equations coefficients into eye coordinates.  See
    // es_full_spec.1.1.12.pdf section 2.11.  We also require the transpose
    // due to how we store the matrices.
    emugl::Matrix mv = c->uniform_context_.GetModelViewMatrix();
    mv.Inverse();
    mv.Transpose();
    emugl::Vector value(equation[0], equation[1], equation[2], equation[3]);
    plane->AssignMatrixMultiply(mv, value);
}

APIENTRY_IMPL(void, ClipPlanefOES, GLenum pname, const GLfloat* equation) {
    APITRACE();
    TRCALL(glClipPlanef(pname, equation));
}

APIENTRY_IMPL(void, ClipPlanex, GLenum pname, const GLfixed* equation) {
    APITRACE();
    static const size_t kNumElements = 4;
    GLfloat tmp[kNumElements];
    Convert(tmp, kNumElements, equation);
    TRCALL(glClipPlanef(pname, tmp));
}

APIENTRY_IMPL(void, ClipPlanexOES, GLenum pname, const GLfixed* equation) {
    APITRACE();
    TRCALL(glClipPlanex(pname, equation));
}

// Sets the vertex color that is used when no vertex color array is enabled.
APIENTRY_IMPL(void, Color4f, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& color = c->uniform_context_.MutateColor();
    color = emugl::Vector(red, green, blue, alpha);
}

APIENTRY_IMPL(void, Color4fv, const GLfloat* components) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& color = c->uniform_context_.MutateColor();
    color.AssignLinearMapping(components, 4);
}

APIENTRY_IMPL(void, Color4ub, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& color = c->uniform_context_.MutateColor();
    GLubyte params[4] = {red, green, blue, alpha};
    color.AssignLinearMapping(params, 4);
}

APIENTRY_IMPL(void, Color4ubv, const GLubyte* components) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& color = c->uniform_context_.MutateColor();
    color.AssignLinearMapping(components, 4);
}

APIENTRY_IMPL(void, Color4x, GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
    APITRACE();
    TRCALL(glColor4f(X2F(red), X2F(green), X2F(blue), X2F(alpha)));
}

APIENTRY_IMPL(void, Color4xOES, GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
    APITRACE();
    TRCALL(glColor4f(X2F(red), X2F(green), X2F(blue), X2F(alpha)));
}

// Controls what components output from the fragment shader are written to the
// framebuffer.
APIENTRY_IMPL(void, ColorMask, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    GLboolean (&color_writemask)[4] = c->color_writemask_.Mutate();
    color_writemask[0] = red != GL_FALSE ? GL_TRUE : GL_FALSE;
    color_writemask[1] = green != GL_FALSE ? GL_TRUE : GL_FALSE;
    color_writemask[2] = blue != GL_FALSE ? GL_TRUE : GL_FALSE;
    color_writemask[3] = alpha != GL_FALSE ? GL_TRUE : GL_FALSE;
    PASS_THROUGH(c, ColorMask, red, green, blue, alpha);
}

// Specifies source array for per-vertex colors.
APIENTRY_IMPL(void, ColorPointer, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidColorPointerSize(size)) {
        GLES_ERROR_INVALID_VALUE_INT(size);
        return;
    }
    if (!IsValidColorPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (stride < 0) {
        GLES_ERROR_INVALID_VALUE_INT(stride);
        return;
    }
    const GLboolean normalized = (type != GL_FLOAT && type != GL_FIXED);
    c->pointer_context_.SetPointer(kColorVertexAttribute, size, type, stride,
            pointer, normalized);
}

APIENTRY_IMPL(void, ColorPointerBounds, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glColorPointer(size, type, stride, pointer));
}

// Compiles a shader from the source code loaded into the shader object.
APIENTRY_IMPL(void, CompileShader, GLuint shader) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
        return;
    }

    shader_data->Compile();
}

namespace {

    void HandleETC1CompressedTexImage2D(
            GLenum target, GLint level, GLenum internalformat, GLsizei width,
            GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {
        ContextPtr c = GetCurrentGlesContext();
        ALOG_ASSERT(c != NULL);

        if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
            GLES_ERROR_INVALID_VALUE_INT(level);
            return;
        }

        const TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex == NULL) {
            GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
            return;
        }

        size_t expected_size = etc1_get_encoded_data_size(width, height);
        if (static_cast<size_t>(image_size) < expected_size) {
            ALOGE("Expected to be given %d bytes of compressed data, but actually "
                    "given %d bytes.", static_cast<int>(expected_size), image_size);
            GLES_ERROR_INVALID_VALUE_INT(image_size);
            return;
        }

        const GLint unpack_alignment = c->pixel_store_unpack_alignment_.Get();
        PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 1);

        tex->Set(level, width, height, GL_RGB, GL_UNSIGNED_BYTE);

        const size_t bytes_per_pixel = 3;
        const size_t stride = bytes_per_pixel * width;
        PASS_THROUGH(c, TexImage2D, target, level, GL_RGB, width,
                height, border, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level, 0,
                0, width, height, GL_RGB, GL_UNSIGNED_BYTE,
                GL_WRITE_ONLY_OES);
        const int rc = etc1_decode_image(static_cast<const etc1_byte*>(data),
                static_cast<etc1_byte*>(buffer), width,
                height, bytes_per_pixel, stride);
        ALOG_ASSERT(rc == 0);
        PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);
        PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, unpack_alignment);
    }

    void HandlePalettedCompressedTexImage2D(
            GLenum target, GLint level, GLenum internalformat, GLsizei width,
            GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {

        ContextPtr c = GetCurrentGlesContext();
        ALOG_ASSERT(c != NULL);

        GLsizei supplied_levels = -level + 1;
        if (level > 0 || supplied_levels > c->texture_context_.GetMaxLevels()) {
            // For paletted formats, the level parameter must not be positive, and when
            // the |level|+1 is computed, must not exceed the maximum allowed levels.
            // Zero means that only the base mip map level is being set, and every
            // number lower means that one more mipmap level is also being set.
            // For example -3 would mean 4 mipmap levels are being set (levels 0-3)
            GLES_ERROR_INVALID_VALUE_INT(level);
            return;
        }

        const TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex == NULL) {
            GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
            return;
        }

        const size_t image_bpp =
            PalettedTextureUtil::GetImageBpp(internalformat);
        const size_t palette_entry_size =
            PalettedTextureUtil::GetEntrySizeBytes(internalformat);
        const GLenum palette_format =
            PalettedTextureUtil::GetEntryFormat(internalformat);
        const GLenum palette_type =
            PalettedTextureUtil::GetEntryType(internalformat);
        const size_t palette_size =
            PalettedTextureUtil::ComputePaletteSize(
                    image_bpp, palette_entry_size);
        const size_t level0_size =
            PalettedTextureUtil::ComputeLevel0Size(
                    width, height, image_bpp);
        const size_t expected_size =
            PalettedTextureUtil::ComputeTotalSize(
                    palette_size, level0_size, supplied_levels);

        if (static_cast<size_t>(image_size) < expected_size) {
            ALOGE("Expected to be given %d bytes of compressed data, but actually "
                    "given %d bytes.", static_cast<int>(expected_size), image_size);
            GLES_ERROR_INVALID_VALUE_INT(image_size);
            return;
        }

        const uint8_t* const palette_data = static_cast<const uint8_t*>(data);
        const uint8_t* image_data = palette_data + palette_size;

        // We require a minimum uncompressed buffer size of 2 pixels here to handle
        // the corner case of a 1 x 1 4bpp image, which requires only 4 bits of
        // storage, but actually ends up taking a full byte. The code ends up
        // unpacking both the real 4 bit value as well as the "padding" 4 bits.
        std::vector<uint8_t> uncompressed_level(std::min(2, width * height) *
                palette_entry_size);

        const GLint unpack_alignment = c->pixel_store_unpack_alignment_.Get();
        PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 1);
        for (size_t i = 0; i < static_cast<size_t>(supplied_levels); ++i) {
            const int level_width = width >> i;
            const int level_height = height >> i;
            const int level_size =
                PalettedTextureUtil::ComputeLevelSize(level0_size, i);

            tex->Set(i, level_width, level_height, palette_format, palette_type);

            PASS_THROUGH(c, TexImage2D, target, i, palette_format, level_width,
                    level_height, border, palette_format, palette_type,
                    NULL);
            GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, i, 0,
                    0, level_width, level_height, palette_format,
                    palette_type, GL_WRITE_ONLY_OES);
            uint8_t* dst = static_cast<uint8_t*>(buffer);
            dst = PalettedTextureUtil::Decompress(
                    image_bpp, level_size, palette_entry_size, image_data, palette_data,
                    dst);
            ALOG_ASSERT(dst < static_cast<uint8_t*>(buffer) + level_size);
            PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);
            image_data += level_size;
        }
        ALOG_ASSERT(image_data - image_size <= data);
        PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, unpack_alignment);
    }

    void HandleEmulateCompressedTexImage2D(
            GLenum target, GLint level, GLenum internalformat, GLsizei width,
            GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {

        ContextPtr c = GetCurrentGlesContext();
        ALOG_ASSERT(c != NULL);

        if (width < 0 || width > c->max_texture_size_.Get() || !IsPowerOf2(width)) {
            GLES_ERROR_INVALID_VALUE_INT(width);
            return;
        }
        if (height < 0 || height > c->max_texture_size_.Get() || !IsPowerOf2(height)) {
            GLES_ERROR_INVALID_VALUE_INT(height);
            return;
        }
        if (border != 0) {
            GLES_ERROR_INVALID_VALUE_INT(border);
            return;
        }
        if (!IsValidEmulatedCompressedTextureFormats(internalformat)) {
            GLES_ERROR_INVALID_ENUM(internalformat);
            return;
        }

        if (internalformat == GL_ETC1_RGB8_OES) {
            HandleETC1CompressedTexImage2D(target, level, internalformat, width,
                    height, border, image_size, data);
        } else {
            HandlePalettedCompressedTexImage2D(target, level, internalformat, width,
                    height, border, image_size, data);
        }
    }

}  // namespace

APIENTRY_IMPL(void, CompressedTexImage2D, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    // TODO(crbug.com/441913): Record that this texture level uses a compressed
    // format, as well as other data we track for this level and this texture.
    if (c->IsCompressedFormatEmulationNeeded(internalformat)) {
        HandleEmulateCompressedTexImage2D(target, level, internalformat, width,
                height, border, image_size, data);
        return;
    }
    PASS_THROUGH(c, CompressedTexImage2D, target, level, internalformat, width,
            height, border, image_size, data);
}

APIENTRY_IMPL(void, CompressedTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei image_size, const GLvoid* data) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (c->IsCompressedFormatEmulationNeeded(format)) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not supported for paletted formats.");
        return;
    }
    PASS_THROUGH(c, CompressedTexSubImage2D, target, level, xoffset, yoffset,
            width, height, format, image_size, data);
}

// Extracts a portion of the framebuffer, loading it into the currently bound
// texture.
APIENTRY_IMPL(void, CopyTexImage2D, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

    // TODO(crbug.com/441914): GL_INVALID_OPERATION if the framebuffer format is
    // not a superset of internalformat.

    // TODO(crbug.com/441914): GL_INVALID_FRAMEBUFFER_OPERATION if the
    // framebuffer is not complete (ie. glCheckFramebufferStatus is not
    // GL_FRAMEBUFFER_COMPLETE).

    // TODO(crbug.com/441914): The framebuffer may be a sixteen bit type and the
    // logic here assumes that it will always be GL_UNSIGNED_BYTE.
    GLenum type = GL_UNSIGNED_BYTE;
    if (!IsValidPixelFormatAndType(internalformat, type)) {
        GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(internalformat), GetEnumString(type));
        return;
    }

    if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
        GLES_ERROR_INVALID_VALUE_INT(level);
        return;
    }
    if (width < 0 || width > c->max_texture_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(width);
        return;
    }
    if (height < 0 || height > c->max_texture_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(height);
        return;
    }
    if (border != 0) {
        GLES_ERROR_INVALID_VALUE_INT(border);
        return;
    }

    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
        return;
    }

    tex->Set(level, width, height, internalformat, type);

    PASS_THROUGH(c, CopyTexImage2D, target, level, internalformat, x, y, width,
            height, border);
}

// Extracts a portion of another texture, replacing the current bound texture.
APIENTRY_IMPL(void, CopyTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

    PASS_THROUGH(c, CopyTexSubImage2D, target, level, xoffset, yoffset, x, y,
            width, height);
}

// Creates and returns a new program object.
APIENTRY_IMPL(GLuint, CreateProgram) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return 0;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ObjectLocalName name = 0;
    sg->GenPrograms(1, &name);
    sg->CreateProgramData(name);
    return name;
}

// Creates and returns a new shader object.
APIENTRY_IMPL(GLuint, CreateShader, GLenum type) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return 0;
    }

    if (!IsValidShaderType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return 0;
    }

    ObjectLocalName name = 0;
    ShareGroupPtr sg = c->GetShareGroup();
    if (type == GL_VERTEX_SHADER) {
        sg->GenVertexShaders(1, &name);
        sg->CreateVertexShaderData(name);
    } else {
        sg->GenFragmentShaders(1, &name);
        sg->CreateFragmentShaderData(name);
    }
    return name;
}

// Selects which faces are culled (back, front, both) when culling is enabled.
APIENTRY_IMPL(void, CullFace, GLenum mode) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidCullFaceMode(mode)) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
    }

    c->cull_face_mode_.Mutate() = mode;
    PASS_THROUGH(c, CullFace, mode);
}

// Set the current patette matrix.
APIENTRY_IMPL(void, CurrentPaletteMatrixOES, GLuint index) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!c->uniform_context_.SetCurrentPaletteMatrix(index)) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
    }
}

// Delete a number of buffer objects.
APIENTRY_IMPL(void, DeleteBuffers, GLsizei n, const GLuint* buffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    for (int i = 0; i < n; ++i) {
        if (c->array_buffer_binding_ == buffers[i]) {
            c->array_buffer_binding_ = 0;
        }
        if (c->element_buffer_binding_ == buffers[i]) {
            c->element_buffer_binding_ = 0;
        }
    }

    ShareGroupPtr sg = c->GetShareGroup();
    sg->DeleteBuffers(n, buffers);
}

// Delete a number of frame buffer objects.
APIENTRY_IMPL(void, DeleteFramebuffers, GLsizei n, const GLuint* framebuffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    for (int i = 0; i < n; ++i) {
        if (c->framebuffer_binding_ == framebuffers[i]) {
            c->framebuffer_binding_ = 0;
        }
    }

    ShareGroupPtr sg = c->GetShareGroup();
    sg->DeleteFramebuffers(n, framebuffers);
}

APIENTRY_IMPL(void, DeleteFramebuffersOES, GLsizei n, const GLuint* framebuffers) {
    APITRACE();
    TRCALL(glDeleteFramebuffers(n, framebuffers));
}

// Delete a single program object.
APIENTRY_IMPL(void, DeleteProgram, GLuint program) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        return;
    }

    if (c->GetCurrentUserProgram() == program_data) {
        // TODO(crbug.com/424353): Do not automatically unuse a deleted program.
        c->SetCurrentUserProgram(ProgramDataPtr());
    }

    sg->DeletePrograms(1, &program);
}

// Delete a number of render buffer objects.
APIENTRY_IMPL(void, DeleteRenderbuffers, GLsizei n, const GLuint* renderbuffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    FramebufferDataPtr fb = c->GetBoundFramebufferData();
    for (int i = 0; i < n; ++i) {
        if (fb != NULL) {
            fb->ClearAttachment(renderbuffers[i], false);
        }
        if (c->renderbuffer_binding_ == renderbuffers[i]) {
            c->renderbuffer_binding_ = 0;
        }
    }

    ShareGroupPtr sg = c->GetShareGroup();
    sg->DeleteRenderbuffers(n, renderbuffers);
}

APIENTRY_IMPL(void, DeleteRenderbuffersOES, GLsizei n, const GLuint* renderbuffers) {
    APITRACE();
    TRCALL(glDeleteRenderbuffers(n, renderbuffers));
}

// Delete a single shader object.
APIENTRY_IMPL(void, DeleteShader, GLuint shader) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    sg->DeleteShaders(1, &shader);
}

// Delete a number of texture objects.
APIENTRY_IMPL(void, DeleteTextures, GLsizei n, const GLuint* textures) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    FramebufferDataPtr fb = c->GetBoundFramebufferData();
    for (int i = 0; i < n; ++i) {
        if (fb != NULL) {
            fb->ClearAttachment(textures[i], true);
        }
        // Do not delete texture if it is a target of EGLImage.
        TextureDataPtr tex = sg->GetTextureData(textures[i]);
        if (tex != NULL && tex->IsEglImageAttached()) {
            sg->SetTextureGlobalName(tex->GetLocalName(), 0);
        }
        c->texture_context_.DeleteTexture(textures[i]);
    }
    sg->DeleteTextures(n, textures);
}

// Discard pixel writes based on an depth value test.
APIENTRY_IMPL(void, DepthFunc, GLenum func) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidDepthFunc(func)) {
        GLES_ERROR_INVALID_ENUM(func);
        return;
    }

    c->depth_func_.Mutate() = func;
    PASS_THROUGH(c, DepthFunc, func);
}

// Sets whether or not the pixel depth value is written to the framebuffer.
APIENTRY_IMPL(void, DepthMask, GLboolean flag) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->depth_writemask_.Mutate() = flag != GL_FALSE ? GL_TRUE : GL_FALSE;
    PASS_THROUGH(c, DepthMask, flag);
}

// Pixels outside this range are clipped and the fragment depth is normalized
// on write based on them.
APIENTRY_IMPL(void, DepthRangef, GLfloat zNear, GLfloat zFar) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    zNear = ClampValue(zNear, 0.f, 1.f);
    zFar = ClampValue(zFar, 0.f, 1.f);

    GLfloat (&depth_range)[2] = c->depth_range_.Mutate();
    depth_range[0] = static_cast<GLfloat>(zNear);
    depth_range[1] = static_cast<GLfloat>(zFar);
    PASS_THROUGH(c, DepthRangef, zNear, zFar);
}

APIENTRY_IMPL(void, DepthRangefOES, GLclampf zNear, GLclampf zFar) {
    APITRACE();
    TRCALL(glDepthRangef(zNear, zFar));
}

APIENTRY_IMPL(void, DepthRangex, GLclampx zNear, GLclampx zFar) {
    APITRACE();
    TRCALL(glDepthRangef(X2F(zNear), X2F(zFar)));
}

APIENTRY_IMPL(void, DepthRangexOES, GLclampx zNear, GLclampx zFar) {
    APITRACE();
    TRCALL(glDepthRangef(X2F(zNear), X2F(zFar)));
}

// Removes a shader from a program.
APIENTRY_IMPL(void, DetachShader, GLuint program, GLuint shader) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid shader %d", shader);
        return;
    }
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->DetachShader(shader_data);
}

// Clear server state capability.
APIENTRY_IMPL(void, Disable, GLenum cap) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const int kind = GetCapabilityHandlingKind(cap);
    if (kind == kHandlingKindInvalid) {
        GLES_ERROR_INVALID_ENUM(cap);
        return;
    }

    if (kind & kHandlingKindLocalCopy) {
        c->enabled_set_.erase(cap);
    }
    if (kind & kHandlingKindUniform) {
        c->uniform_context_.Disable(cap);
    }
    if (kind & kHandlingKindTexture) {
        c->texture_context_.Disable(cap);
    }
    if (kind & kHandlingKindPropagate) {
        PASS_THROUGH(c, Disable, cap);
    }
}

// Clear client state capability.
APIENTRY_IMPL(void, DisableClientState, GLenum cap) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidClientState(cap)) {
        GLES_ERROR_INVALID_ENUM(cap);
        return;
    }

    const GLuint index = ArrayEnumToIndex(c, cap);
    if (!c->pointer_context_.IsArrayEnabled(index)) {
        return;
    }
    c->pointer_context_.DisableArray(index);
}

// Disables input from an array to the given vertex shader attribute.
APIENTRY_IMPL(void, DisableVertexAttribArray, GLuint index) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (index >= static_cast<GLuint>(c->max_vertex_attribs_.Get())) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
        return;
    }
    c->pointer_context_.DisableArray(index);
    PASS_THROUGH(c, DisableVertexAttribArray, index);
}

// Renders primitives using the current state.
APIENTRY_IMPL(void, DrawArrays, GLenum mode, GLint first, GLsizei count) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidDrawMode(mode)) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
    }
    if (first < 0) {
        GLES_ERROR_INVALID_VALUE_INT(first);
        return;
    }
    if (count < 0) {
        GLES_ERROR_INVALID_VALUE_INT(count);
        return;
    }
    c->Draw(GlesContext::kDrawArrays, mode, first, count, 0, NULL);
}

// Renders primitives using the current state.
APIENTRY_IMPL(void, DrawElements, GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidDrawMode(mode)) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
    }
    if (!IsValidDrawType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (count < 0) {
        GLES_ERROR_INVALID_VALUE_INT(count);
        return;
    }
    c->Draw(GlesContext::kDrawElements, mode, 0, count, type, indices);
}

APIENTRY_IMPL(void, DrawTexfOES, GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->DrawTex(x, y, z, width, height);
}

APIENTRY_IMPL(void, DrawTexfvOES, const GLfloat* coords) {
    APITRACE();
    TRCALL(glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]));
}

APIENTRY_IMPL(void, DrawTexiOES, GLint x, GLint y, GLint z, GLint width, GLint height) {
    APITRACE();
    TRCALL(glDrawTexfOES(x, y, z, width, height));
}

APIENTRY_IMPL(void, DrawTexivOES, const GLint* coords) {
    APITRACE();
    TRCALL(glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]));
}

APIENTRY_IMPL(void, DrawTexsOES, GLshort x, GLshort y, GLshort z, GLshort width, GLshort height) {
    APITRACE();
    TRCALL(glDrawTexfOES(x, y, z, width, height));
}

APIENTRY_IMPL(void, DrawTexsvOES, const GLshort* coords) {
    APITRACE();
    TRCALL(glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]));
}

APIENTRY_IMPL(void, DrawTexxOES, GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height) {
    APITRACE();
    TRCALL(glDrawTexfOES(X2F(x), X2F(y), X2F(z), X2F(width), X2F(height)));
}

APIENTRY_IMPL(void, DrawTexxvOES, const GLfixed* coords) {
    APITRACE();
    TRCALL(glDrawTexfOES(X2F(coords[0]), X2F(coords[1]), X2F(coords[2]), X2F(coords[3]),
            X2F(coords[4])));
}

EglImagePtr GetEglImageFromNativeBuffer(GLeglImageOES buffer);

// Set the specified EGL image as the renderbuffer storage.
APIENTRY_IMPL(void, EGLImageTargetRenderbufferStorageOES, GLenum target, GLeglImageOES buffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    EglImagePtr image = GetEglImageFromNativeBuffer(buffer);
    if (image == NULL) {
        GLES_ERROR_INVALID_VALUE_PTR(static_cast<void *>(buffer));
        return;
    }
    if (!IsValidRenderbufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    if (!c->BindImageToRenderbuffer(image)) {
        GLES_ERROR(GL_INVALID_OPERATION, "Unable to bind image to renderbuffer.");
    }
}

// Set the specified EGLimage as the texture.
APIENTRY_IMPL(void, EGLImageTargetTexture2DOES, GLenum target, GLeglImageOES buffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    DLOG("call underlying glEGLImageTargetTexture2DOES");
    PASS_THROUGH(c, EGLImageTargetTexture2DOES, target, buffer);

    DLOG("should bind image to texture @ this point");
}

// Set server state capabity.
APIENTRY_IMPL(void, Enable, GLenum cap) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const int kind = GetCapabilityHandlingKind(cap);
    if (kind == kHandlingKindInvalid) {
        GLES_ERROR_INVALID_ENUM(cap);
        return;
    }

    if (kind & kHandlingKindLocalCopy) {
        c->enabled_set_.insert(cap);
    }
    if (kind & kHandlingKindUniform) {
        c->uniform_context_.Enable(cap);
    }
    if (kind & kHandlingKindTexture) {
        c->texture_context_.Enable(cap);
    }
    if (kind & kHandlingKindUnsupported) {
        if (kind & kHandlingKindIgnored) {
            ALOGW("Unsupported capability %s (%x), but ignored.",
                    GetEnumString(cap), cap);
        } else {
            LOG_ALWAYS_FATAL("Unsupported capability %s (%x)",
                    GetEnumString(cap), cap);
        }
    }
    if (kind & kHandlingKindPropagate) {
        PASS_THROUGH(c, Enable, cap);
    }
}

// Set client state capability.
APIENTRY_IMPL(void, EnableClientState, GLenum cap) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidClientState(cap)) {
        GLES_ERROR_INVALID_ENUM(cap);
        return;
    }

    const GLuint index = ArrayEnumToIndex(c, cap);
    if (c->pointer_context_.IsArrayEnabled(index)) {
        return;
    }
    c->pointer_context_.EnableArray(index);
}

// Enables input from an array to the given vertex shader attribute.
APIENTRY_IMPL(void, EnableVertexAttribArray, GLuint index) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (index >= static_cast<GLuint>(c->max_vertex_attribs_.Get())) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
        return;
    }
    c->pointer_context_.EnableArray(index);
    PASS_THROUGH(c, EnableVertexAttribArray, index);
}

// Finishes any commands in the GL command queue, and returns only when all
// commands are complete (including any rendering).
APIENTRY_IMPL(void, Finish) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, Finish);
}

// Flushes any commands in the GL command queue, ensuring they are actually
// being processed and not just waiting for some other event to trigger them.
APIENTRY_IMPL(void, Flush) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, Flush);
}

namespace {

    // Configures the fog computation of the fixed-function shader.
    void HandleFog(GLenum name, const GLfloat* params, ParamType param_type) {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return;
        }

        Fog& fog = c->uniform_context_.MutateFog();

        const GLenum mode = static_cast<GLenum>(params[0]);
        const GLfloat value = params[0];
        switch (name) {
            case GL_FOG_MODE:
                if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
                    GLES_ERROR_INVALID_ENUM(mode);
                    return;
                }
                fog.mode = mode;
                break;
            case GL_FOG_DENSITY:
                if (value < 0.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(value);
                    return;
                }
                fog.density = value;
                break;
            case GL_FOG_START:
                fog.start = value;
                break;
            case GL_FOG_END:
                fog.end = value;
                break;
            case GL_FOG_COLOR:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_FOG_COLOR.");
                    return;
                }
                for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                    fog.color.Set(i, ClampValue(params[i], 0.f, 1.f));
                }
                break;
            default:
                GLES_ERROR_INVALID_ENUM(name);
                return;
        }
    }

}  // namespace

APIENTRY_IMPL(void, Fogf, GLenum name, GLfloat param) {
    APITRACE();
    HandleFog(name, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, Fogfv, GLenum name, const GLfloat* params) {
    APITRACE();
    HandleFog(name, params, kParamTypeArray);
}

APIENTRY_IMPL(void, Fogx, GLenum pname, GLfixed param) {
    APITRACE();
    if (pname == GL_FOG_MODE) {
        TRCALL(glFogf(pname, static_cast<GLfloat>(param)));
    } else {
        TRCALL(glFogf(pname, X2F(param)));
    }
}

APIENTRY_IMPL(void, FogxOES, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glFogx(pname, param));
}

APIENTRY_IMPL(void, Fogxv, GLenum pname, const GLfixed* params) {
    APITRACE();
    if (pname == GL_FOG_MODE) {
        TRCALL(glFogf(pname, static_cast<GLfloat>(params[0])));
    } else {
        GLfloat tmp[kMaxParamElementSize];
        Convert(tmp, ParamSize(pname), params);
        TRCALL(glFogfv(pname, tmp));
    }
}

APIENTRY_IMPL(void, FogxvOES, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glFogxv(pname, params));
}

// Attaches a render buffer to a frame buffer.
APIENTRY_IMPL(void, FramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidFramebufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidFramebufferAttachment(attachment)) {
        GLES_ERROR_INVALID_ENUM(attachment);
        return;
    }
    if (!IsValidRenderbufferTarget(renderbuffertarget)) {
        GLES_ERROR_INVALID_ENUM(renderbuffertarget);
        return;
    }

    FramebufferDataPtr fb = c->GetBoundFramebufferData();
    if (renderbuffer == 0) {
        if (fb != NULL) {
            fb->ClearAttachment(attachment);
        }
        PASS_THROUGH(c, FramebufferRenderbuffer, target, attachment,
                renderbuffertarget, 0);
    } else {
        ShareGroupPtr sg = c->GetShareGroup();
        RenderbufferDataPtr rb = sg->GetRenderbufferData(renderbuffer);
        if (rb == NULL) {
            GLES_ERROR(GL_INVALID_OPERATION, "Invalid renderbuffer.");
            return;
        }

        if (fb != NULL) {
            fb->SetAttachment(attachment, renderbuffertarget, renderbuffer, rb);
        }

        GLint texture = rb->GetEglImageTexture();
        if (texture != 0) {
            // This renderbuffer object is an EGLImage target so attach the
            // EGLImage's texture instead the renderbuffer.
            PASS_THROUGH(c, FramebufferTexture2D, target, attachment, GL_TEXTURE_2D,
                    texture, 0);
        } else {
            const GLuint global_name = sg->GetRenderbufferGlobalName(renderbuffer);
            PASS_THROUGH(c, FramebufferRenderbuffer, target, attachment,
                    renderbuffertarget, global_name);
        }
    }
}

APIENTRY_IMPL(void, FramebufferRenderbufferOES, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    APITRACE();
    TRCALL(glFramebufferRenderbuffer(target, attachment, renderbuffertarget,
            renderbuffer));
}

// Sets up a texture to be used as the framebuffer for subsequent rendering.
APIENTRY_IMPL(void, FramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidFramebufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidFramebufferAttachment(attachment)) {
        GLES_ERROR_INVALID_ENUM(attachment);
        return;
    }
    if (!IsValidTextureTargetEx(textarget)) {
        GLES_ERROR_INVALID_ENUM(textarget);
        return;
    }
    if (level != 0) {
        GLES_ERROR_INVALID_VALUE_INT(level);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();

    // Update the framebuffer attachment.
    FramebufferDataPtr fb = c->GetBoundFramebufferData();
    if (fb != NULL) {
        fb->SetAttachment(attachment, textarget, texture, RenderbufferDataPtr());
    }

    const GLuint global_name = sg->GetTextureGlobalName(texture);
    PASS_THROUGH(c, FramebufferTexture2D, target, attachment, textarget,
            global_name, level);
}

APIENTRY_IMPL(void, FramebufferTexture2DOES, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    APITRACE();
    TRCALL(glFramebufferTexture2D(target, attachment, textarget, texture, level));
}

// Sets which order (clockwise, counter-clockwise) is considered front facing.
APIENTRY_IMPL(void, FrontFace, GLenum mode) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidFrontFaceMode(mode)) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
    }

    c->front_face_.Mutate() = mode;
    PASS_THROUGH(c, FrontFace, mode);
}

// Multiplies the current matrix with the specified perspective matrix.
APIENTRY_IMPL(void, Frustumf, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->uniform_context_.MutateActiveMatrix() *=
        emugl::Matrix::GeneratePerspective(left, right, bottom, top, zNear, zFar);
}

APIENTRY_IMPL(void, FrustumfOES, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    APITRACE();
    TRCALL(glFrustumf(left, right, bottom, top, zNear, zFar));
}

APIENTRY_IMPL(void, Frustumx, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    APITRACE();
    TRCALL(glFrustumf(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
            X2F(zFar)));
}

APIENTRY_IMPL(void, FrustumxOES, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    APITRACE();
    TRCALL(glFrustumf(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
            X2F(zFar)));
}

// Generate N buffer objects.
APIENTRY_IMPL(void, GenBuffers, GLsizei n, GLuint* buffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    c->GetShareGroup()->GenBuffers(n, buffers);
}

// Generate N framebuffer objects.
APIENTRY_IMPL(void, GenFramebuffers, GLsizei n, GLuint* framebuffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    c->GetShareGroup()->GenFramebuffers(n, framebuffers);
}

APIENTRY_IMPL(void, GenFramebuffersOES, GLsizei n, GLuint* framebuffers) {
    APITRACE();
    TRCALL(glGenFramebuffers(n, framebuffers));
}

// Generates N renderbuffer objects.
APIENTRY_IMPL(void, GenRenderbuffers, GLsizei n, GLuint* renderbuffers) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    c->GetShareGroup()->GenRenderbuffers(n, renderbuffers);
}

APIENTRY_IMPL(void, GenRenderbuffersOES, GLsizei n, GLuint* renderbuffers) {
    APITRACE();
    TRCALL(glGenRenderbuffers(n, renderbuffers));
}

// Creates N texture objects.
APIENTRY_IMPL(void, GenTextures, GLsizei n, GLuint* textures) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (n < 0) {
        GLES_ERROR_INVALID_VALUE_INT(n);
        return;
    }

    c->GetShareGroup()->GenTextures(n, textures);
}

// Generates all the mipmaps of a texture from the highest resolution image.
APIENTRY_IMPL(void, GenerateMipmap, GLenum target) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    // TODO(crbug.com/441913): Record texture format for texture conversion
    // workaround for Pepper GLES2.
    PASS_THROUGH(c, GenerateMipmap, target);
}

APIENTRY_IMPL(void, GenerateMipmapOES, GLenum target) {
    APITRACE();
    TRCALL(glGenerateMipmap(target));
}

// Gets the list of attribute indices used by the given program.
APIENTRY_IMPL(void, GetActiveAttrib, GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetActiveAttrib(index, bufsize, length, size, type, name);
}

// Gets the list of uniform indices used by the given program.
APIENTRY_IMPL(void, GetActiveUniform, GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetActiveUniform(index, bufsize, length, size, type, name);
}

// Gets the list of shader objects used by the given program.
APIENTRY_IMPL(void, GetAttachedShaders, GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetAttachedShaders(maxcount, count, shaders);
}

// Gets the index of an attribute for a program by name.
APIENTRY_IMPL(GLint, GetAttribLocation, GLuint program, const GLchar* name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return -1;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return -1;
    }

    return program_data->GetAttribLocation(name);
}

// Gets state as a boolean.
APIENTRY_IMPL(void, GetBooleanv, GLenum pname, GLboolean* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (c->GetBooleanv(pname, params)) {
        return;
    }
    *params = GL_FALSE;
    PASS_THROUGH(c, GetBooleanv, pname, params);
}

// Get buffer parameter value.
APIENTRY_IMPL(void, GetBufferParameteriv, GLenum target, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidBufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
    if (vbo == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Target buffer not bound.");
        return;
    }

    switch (pname) {
        case GL_BUFFER_SIZE:
            *params = vbo->GetSize();
            break;
        case GL_BUFFER_USAGE:
            *params = vbo->GetUsage();
            break;
        default:
            GLES_ERROR_INVALID_ENUM(pname);
            return;
    }
}

// Gets the current clip plane parameters.
APIENTRY_IMPL(void, GetClipPlanef, GLenum pname, GLfloat* eqn) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const emugl::Vector* plane = c->uniform_context_.GetClipPlane(pname);
    if (!plane) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }
    for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        eqn[i] = plane->Get(i);
    }
}

APIENTRY_IMPL(void, GetClipPlanefOES, GLenum pname, GLfloat* eqn) {
    APITRACE();
    TRCALL(glGetClipPlanef(pname, eqn));
}

APIENTRY_IMPL(void, GetClipPlanex, GLenum pname, GLfixed* eqn) {
    APITRACE();
    static const size_t kNumElements = 4;
    GLfloat tmp[kNumElements];
    TRCALL(glGetClipPlanef(pname, tmp));
    Convert(tmp, kNumElements, eqn);
}

APIENTRY_IMPL(void, GetClipPlanexOES, GLenum pname, GLfixed* eqn) {
    APITRACE();
    TRCALL(glGetClipPlanex(pname, eqn));
}

// Returns the oldest error code that was reported.
APIENTRY_IMPL(GLenum, GetError) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c || !c->AreChecksEnabled()) {
        return GL_NO_ERROR;
    }

    // N.B. We have no way of knowing which is older -- the oldest error we
    // recorded or any error that might be available downstream. We just return
    // any error we might have, and if none, we pass the request through.
    GLenum error = c->GetGLerror();
    if (error != GL_NO_ERROR) {
        ALOGV("Returning error %s directly.", GetEnumString(error));
    } else {
        error = PASS_THROUGH(c, GetError);
        ALOGV("Returning error %s from passthrough.", GetEnumString(error));
    }
    return error;
}

// Gets state as a fixed.
APIENTRY_IMPL(void, GetFixedv, GLenum pname, GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    TRCALL(glGetFloatv(pname, tmp));
    Convert(params, ParamSize(pname), tmp);
}

APIENTRY_IMPL(void, GetFixedvOES, GLenum pname, GLfixed* params) {
    APITRACE();
    TRCALL(glGetFixedv(pname, params));
}

// Gets state as a float.
APIENTRY_IMPL(void, GetFloatv, GLenum pname, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (c->GetFloatv(pname, params)) {
        return;
    }
    *params = 0.f;
    PASS_THROUGH(c, GetFloatv, pname, params);
}

APIENTRY_IMPL(void, GetFramebufferAttachmentParameteriv, GLenum target, GLenum attachment, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidFramebufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidFramebufferAttachment(attachment)) {
        GLES_ERROR_INVALID_ENUM(attachment);
        return;
    }
    if (!IsValidFramebufferAttachmentParams(pname)) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    // Take the attachment attribute from our state - if available
    FramebufferDataPtr fb = c->GetBoundFramebufferData();
    if (fb == NULL) {
        return;
    }

    GLenum attachment_target = 0;
    const GLuint name = fb->GetAttachment(attachment, &attachment_target);
    switch (pname) {
        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
            if (attachment_target == GL_TEXTURE_2D) {
                *params = GL_TEXTURE;
            } else if (attachment_target == GL_TEXTURE_EXTERNAL_OES) {
                *params = GL_TEXTURE;
            } else if (attachment_target == GL_RENDERBUFFER) {
                *params = GL_RENDERBUFFER;
            } else {
                LOG_ALWAYS_FATAL("Unknown attachment target: %d", attachment_target);
            }
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            *params = name;
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
            *params = 0;
            break;
        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
            DLOG("UNIMPLEMENTED: GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE");
            // PASS_THROUGH(c, GetFramebufferAttachmentParameteriv, target, attachment,
            //         pname, params);
            break;
    }
}

APIENTRY_IMPL(void, GetFramebufferAttachmentParameterivOES, GLenum target, GLenum attachment, GLenum pname, GLint* params) {
    APITRACE();
    TRCALL(glGetFramebufferAttachmentParameteriv(target, attachment, pname, params));
}

// Gets state as an integer.
APIENTRY_IMPL(void, GetIntegerv, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (c->GetIntegerv(pname, params)) {
        return;
    }
    *params = 0;
    PASS_THROUGH(c, GetIntegerv, pname, params);
}

// Gets fixed function lighting state.
APIENTRY_IMPL(void, GetLightfv, GLenum lightid, GLenum name, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const Light* light = c->uniform_context_.GetLight(lightid);
    if (!light) {
        GLES_ERROR_INVALID_ENUM(lightid);
        return;
    }
    switch (name) {
        case GL_AMBIENT:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = light->ambient.Get(i);
            }
            break;
        case GL_DIFFUSE:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = light->diffuse.Get(i);
            }
            break;
        case GL_SPECULAR:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = light->specular.Get(i);
            }
            break;
        case GL_POSITION:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = light->position.Get(i);
            }
            break;
        case GL_SPOT_DIRECTION:
            for (size_t i = 0; i < 3; ++i) {
                params[i] = light->direction.Get(i);
            }
            break;
        case GL_SPOT_EXPONENT:
            params[0] = light->spot_exponent;
            break;
        case GL_SPOT_CUTOFF:
            params[0] = light->spot_cutoff;
            break;
        case GL_CONSTANT_ATTENUATION:
            params[0] = light->const_attenuation;
            break;
        case GL_LINEAR_ATTENUATION:
            params[0] = light->linear_attenuation;
            break;
        case GL_QUADRATIC_ATTENUATION:
            params[0] = light->quadratic_attenuation;
            break;
        default:
            GLES_ERROR_INVALID_ENUM(name);
            return;
    }
}

APIENTRY_IMPL(void, GetLightxv, GLenum light, GLenum pname, GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    TRCALL(glGetLightfv(light, pname, tmp));
    Convert(params, ParamSize(pname), tmp);
}

APIENTRY_IMPL(void, GetLightxvOES, GLenum light, GLenum pname, GLfixed* params) {
    APITRACE();
    TRCALL(glGetLightxv(light, pname, params));
}

// Gets fixed function material state.
APIENTRY_IMPL(void, GetMaterialfv, GLenum face, GLenum name, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (face != GL_FRONT && face != GL_BACK) {
        GLES_ERROR_INVALID_ENUM(face);
        return;
    }

    const Material& material = c->uniform_context_.GetMaterial();
    switch (name) {
        case GL_AMBIENT:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = material.ambient.Get(i);
            }
            break;
        case GL_DIFFUSE:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = material.diffuse.Get(i);
            }
            break;
        case GL_SPECULAR:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = material.specular.Get(i);
            }
            break;
        case GL_EMISSION:
            for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                params[i] = material.emission.Get(i);
            }
            break;
        case GL_SHININESS:
            params[0] = material.shininess;
            break;
        default:
            GLES_ERROR_INVALID_ENUM(name);
            return;
    }
}

APIENTRY_IMPL(void, GetMaterialxv, GLenum face, GLenum pname, GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    TRCALL(glGetMaterialfv(face, pname, tmp));
    Convert(params, ParamSize(pname), tmp);
}

APIENTRY_IMPL(void, GetMaterialxvOES, GLenum face, GLenum pname, GLfixed* params) {
    APITRACE();
    TRCALL(glGetMaterialxv(face, pname, params));
}

// Gets pointer data for the specified array.
APIENTRY_IMPL(void, GetPointerv, GLenum pname, void** params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const GLuint index = ArrayEnumToIndex(c, pname);
    const PointerData* ptr = c->pointer_context_.GetPointerData(index);
    if (!ptr) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    } else {
        *params = const_cast<void*>(ptr->pointer);
    }
}

// Gets the output text messages from the last program link/validation request.
APIENTRY_IMPL(void, GetProgramInfoLog, GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetInfoLog(bufsize, length, infolog);
}

// Gets information about a program.
APIENTRY_IMPL(void, GetProgramiv, GLuint program, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetProgramiv(pname, params);
}

// Gets information about the render buffer bound to the indicated target.
APIENTRY_IMPL(void, GetRenderbufferParameteriv, GLenum target, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidRenderbufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidRenderbufferParams(pname)) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    RenderbufferDataPtr rb = c->GetBoundRenderbufferData();
    if (rb != NULL) {
        switch (pname) {
            case GL_RENDERBUFFER_WIDTH:
                *params = rb->GetWidth();
                break;
            case GL_RENDERBUFFER_HEIGHT:
                *params = rb->GetHeight();
                break;
            case GL_RENDERBUFFER_INTERNAL_FORMAT:
                *params = rb->GetFormat();
                break;
            default:
                LOG_ALWAYS_FATAL("Unsupported buffer parameter %s (%x)",
                        GetEnumString(pname), pname);
                return;
        }
    } else {
        PASS_THROUGH(c, GetRenderbufferParameteriv, target, pname, params);
    }
}

APIENTRY_IMPL(void, GetRenderbufferParameterivOES, GLenum target, GLenum pname, GLint* params) {
    APITRACE();
    TRCALL(glGetRenderbufferParameteriv(target, pname, params));
}

// Gets the output text messages from the last shader compile.
APIENTRY_IMPL(void, GetShaderInfoLog, GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
        return;
    }

    shader_data->GetInfoLog(bufsize, length, infolog);
}

// Gets the shaders precision.
APIENTRY_IMPL(void, GetShaderPrecisionFormat, GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, GetShaderPrecisionFormat, shadertype, precisiontype, range,
            precision);
}

// Gets the source code loaded into the indicated shader object.
APIENTRY_IMPL(void, GetShaderSource, GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
        return;
    }

    shader_data->GetSource(bufsize, length, source);
}

// Gets information about a shader.
APIENTRY_IMPL(void, GetShaderiv, GLuint shader, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
        return;
    }

    shader_data->GetShaderiv(pname, params);
}

// Gets string information about the GL implementation.
APIENTRY_IMPL(const GLubyte*, GetString, GLenum name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        fprintf(stderr, "%s: cannot get current gles context, quitting\n", __FUNCTION__);
        return NULL;
    }

    const GLubyte* str = c->GetString(name);
    if (str == NULL) {
        GLES_ERROR_INVALID_ENUM(name);
    }
    return str;
}

// Gets texture environment parameters.
namespace {

    template <typename T>
        void HandleGetTexEnv(GLenum env, GLenum pname, T* params) {
            ContextPtr c = GetCurrentGlesContext();
            if (!c) {
                return;
            }

            const TexEnv& texenv = c->uniform_context_.GetActiveTexEnv();
            if (env != GL_TEXTURE_ENV) {
                GLES_ERROR_INVALID_ENUM(env);
                return;
            }

            switch (pname) {
                case GL_TEXTURE_ENV_MODE:
                    params[0] = static_cast<T>(texenv.mode);
                    break;
                case GL_SRC0_RGB:
                case GL_SRC1_RGB:
                case GL_SRC2_RGB:
                    params[0] = static_cast<T>(texenv.src_rgb[pname - GL_SRC0_RGB]);
                    break;
                case GL_SRC0_ALPHA:
                case GL_SRC1_ALPHA:
                case GL_SRC2_ALPHA:
                    params[0] = static_cast<T>(texenv.src_alpha[pname - GL_SRC0_ALPHA]);
                    break;
                case GL_OPERAND0_RGB:
                case GL_OPERAND1_RGB:
                case GL_OPERAND2_RGB:
                    params[0] = static_cast<T>(texenv.operand_rgb[pname - GL_OPERAND0_RGB]);
                    break;
                case GL_OPERAND0_ALPHA:
                case GL_OPERAND1_ALPHA:
                case GL_OPERAND2_ALPHA:
                    params[0] =
                        static_cast<T>(texenv.operand_alpha[pname - GL_OPERAND0_ALPHA]);
                    break;
                case GL_COMBINE_RGB:
                    params[0] = static_cast<T>(texenv.combine_rgb);
                    break;
                case GL_COMBINE_ALPHA:
                    params[0] = static_cast<T>(texenv.combine_alpha);
                    break;
                case GL_RGB_SCALE:
                    params[0] = static_cast<T>(texenv.rgb_scale);
                    break;
                case GL_ALPHA_SCALE:
                    params[0] = static_cast<T>(texenv.alpha_scale);
                    break;
                case GL_TEXTURE_ENV_COLOR:
                    texenv.color.GetLinearMapping(params, 4);
                    break;
                case GL_COORD_REPLACE_OES:
                    LOG_ALWAYS_FATAL("GL_COORD_REPLACE_OES not supported.");
                    break;
                default:
                    GLES_ERROR_INVALID_ENUM(pname);
                    return;
            }
        }

}  // namespace

APIENTRY_IMPL(void, GetTexEnvfv, GLenum env, GLenum pname, GLfloat* params) {
    APITRACE();
    HandleGetTexEnv(env, pname, params);
}

APIENTRY_IMPL(void, GetTexEnviv, GLenum env, GLenum pname, GLint* params) {
    APITRACE();
    HandleGetTexEnv(env, pname, params);
}

APIENTRY_IMPL(void, GetTexEnvxv, GLenum env, GLenum pname, GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    TRCALL(glGetTexEnvfv(env, pname, tmp));
    Convert(params, ParamSize(pname), tmp);
}

APIENTRY_IMPL(void, GetTexEnvxvOES, GLenum env, GLenum pname, GLfixed* params) {
    APITRACE();
    TRCALL(glGetTexEnvxv(env, pname, params));
}

// Get texture parameter.
APIENTRY_IMPL(void, GetTexParameterfv, GLenum target, GLenum pname, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTextureTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    const GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
    PASS_THROUGH(c, GetTexParameterfv, global_target, pname, params);
    c->texture_context_.RestoreBinding(target);
}

APIENTRY_IMPL(void, GetTexParameteriv, GLenum target, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTextureTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    if (pname == GL_TEXTURE_CROP_RECT_OES) {
        TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex != NULL) {
            memcpy(params, tex->GetCropRect(), 4 * sizeof(GLint));
        }
        return;
    }

    const GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
    PASS_THROUGH(c, GetTexParameteriv, global_target, pname, params);
    c->texture_context_.RestoreBinding(target);
}

APIENTRY_IMPL(void, GetTexParameterxv, GLenum target, GLenum pname, GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    TRCALL(glGetTexParameterfv(target, pname, tmp));
    Convert(params, ParamSize(pname), tmp);
}

APIENTRY_IMPL(void, GetTexParameterxvOES, GLenum target, GLenum pname, GLfixed* params) {
    APITRACE();
    TRCALL(glGetTexParameterxv(target, pname, params));
}

// Gets the default uniform value for a program.
APIENTRY_IMPL(void, GetUniformfv, GLuint program, GLint location, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetUniformfv(location, params);
}

APIENTRY_IMPL(void, GetUniformiv, GLuint program, GLint location, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->GetUniformiv(location, params);
}

// Gets the index of an uniform for a program by name.
APIENTRY_IMPL(GLint, GetUniformLocation, GLuint program, const GLchar* name) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return -1;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return -1;
    }

    return program_data->GetUniformLocation(name);
}

// Gets a pointer value from the vertex attribute state.
APIENTRY_IMPL(void, GetVertexAttribPointerv, GLuint index, GLenum pname, GLvoid** pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    const PointerData* ptr = c->pointer_context_.GetPointerData(index);
    if (!ptr) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
        return;
    }
    *pointer = const_cast<void*>(ptr->pointer);
}

// Gets the vertex attribute state.
APIENTRY_IMPL(void, GetVertexAttribfv, GLuint index, GLenum pname, GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const PointerData* ptr = c->pointer_context_.GetPointerData(index);
    if (!ptr) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
        return;
    }

    switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = static_cast<GLfloat>(ptr->buffer_name);
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = static_cast<GLfloat>(ptr->enabled);
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = static_cast<GLfloat>(ptr->size);
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = static_cast<GLfloat>(ptr->stride);
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = static_cast<GLfloat>(ptr->type);
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = static_cast<GLfloat>(ptr->normalize);
            break;
        default:
            PASS_THROUGH(c, GetVertexAttribfv, index, pname, params);
            break;
    }
}

APIENTRY_IMPL(void, GetVertexAttribiv, GLuint index, GLenum pname, GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const PointerData* ptr = c->pointer_context_.GetPointerData(index);
    if (!ptr) {
        GLES_ERROR_INVALID_VALUE_UINT(index);
        return;
    }

    switch (pname) {
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = ptr->buffer_name;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = ptr->enabled;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = ptr->size;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = ptr->stride;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = ptr->type;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = ptr->normalize;
            break;
        default:
            PASS_THROUGH(c, GetVertexAttribiv, index, pname, params);
            break;
    }
}

// Provides a hint to the GL implementation. Generally the application uses
// hints to suggest where performance matters more than rendering quality.
APIENTRY_IMPL(void, Hint, GLenum target, GLenum mode) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    switch (target) {
        // GLES1 specific hints.  We will ignore them since they are only hints.
        case GL_FOG_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_PERSPECTIVE_CORRECTION_HINT:
        case GL_POINT_SMOOTH_HINT:
            break;
            // GLES2 supported hints. We pass through except for any caching.
        case GL_GENERATE_MIPMAP_HINT:
            if (!IsValidMipmapHintMode(mode)) {
                GLES_ERROR_INVALID_ENUM(mode);
                return;
            }
            c->generate_mipmap_hint_.Mutate() = mode;
            PASS_THROUGH(c, Hint, target, mode);
            break;
        default:
            GLES_ERROR_INVALID_ENUM(target);
            return;
    }
}

// Returns true if the specified object is a buffer.
APIENTRY_IMPL(GLboolean, IsBuffer, GLuint buffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    BufferDataPtr ptr = sg->GetBufferData(buffer);
    return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the given capability is enabled.
APIENTRY_IMPL(GLboolean, IsEnabled, GLenum cap) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }


    // IsEnabled is used for checking both client and server capabilities.
    // First check the client state.
    if (IsValidClientState(cap)) {
        const GLuint index = ArrayEnumToIndex(c, cap);
        return c->pointer_context_.IsArrayEnabled(index);
    }

    const int kind = GetCapabilityHandlingKind(cap);
    if (kind == kHandlingKindInvalid) {
        GLES_ERROR_INVALID_ENUM(cap);
        return 0;
    }

    if (kind & kHandlingKindLocalCopy) {
        return c->IsEnabled(cap);
    }
    if (kind & kHandlingKindUniform) {
        return c->uniform_context_.IsEnabled(cap);
    }
    if (kind & kHandlingKindTexture) {
        return c->texture_context_.IsEnabled(cap);
    }
    if (kind & kHandlingKindPropagate) {
        return PASS_THROUGH(c, IsEnabled, cap);
    }
    return 0;
}

// Returns true if the specified object is a framebuffer.
APIENTRY_IMPL(GLboolean, IsFramebuffer, GLuint framebuffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    FramebufferDataPtr fb = sg->GetFramebufferData(framebuffer);
    return fb != NULL ? GL_TRUE : GL_FALSE;
}

APIENTRY_IMPL(GLboolean, IsFramebufferOES, GLuint framebuffer) {
    APITRACE();
    return TRCALL(glIsFramebuffer(framebuffer));
}

// Returns true if the specified object is a program.
APIENTRY_IMPL(GLboolean, IsProgram, GLuint program) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr ptr = sg->GetProgramData(program);
    return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the specified object is a renderbuffer.
APIENTRY_IMPL(GLboolean, IsRenderbuffer, GLuint renderbuffer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    RenderbufferDataPtr rb = sg->GetRenderbufferData(renderbuffer);
    return rb != NULL ? GL_TRUE : GL_FALSE;
}

APIENTRY_IMPL(GLboolean, IsRenderbufferOES, GLuint renderbuffer) {
    APITRACE();
    return TRCALL(glIsRenderbuffer(renderbuffer));
}

// Returns true if the specified object is a shader.
APIENTRY_IMPL(GLboolean, IsShader, GLuint shader) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr ptr = sg->GetShaderData(shader);
    return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the specified object is a texture.
APIENTRY_IMPL(GLboolean, IsTexture, GLuint texture) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return GL_FALSE;
    }

    if (texture == 0) {
        return GL_FALSE;
    }
    ShareGroupPtr sg = c->GetShareGroup();
    TextureDataPtr tex = sg->GetTextureData(texture);
    return tex != NULL ? GL_TRUE : GL_FALSE;
}

// Configure fixed function lighting state.
namespace {
    void HandleLightModel(GLenum name, const GLfloat* params,
            ParamType param_type) {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return;
        }

        switch (name) {
            case GL_LIGHT_MODEL_TWO_SIDE:
                if (params[0] == 0.0f) {
                    c->enabled_set_.erase(name);
                } else {
                    c->enabled_set_.insert(name);
                }
                break;
            case GL_LIGHT_MODEL_AMBIENT:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM,
                            "Expected array for GL_LIGHT_MODEL_AMBIENT.");
                    return;
                }
                for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
                    c->uniform_context_.MutateAmbient().Set(i, params[i]);
                }
                break;
            default:
                GLES_ERROR_INVALID_ENUM(name);
                return;
        }
    }
}  // namespace

APIENTRY_IMPL(void, LightModelf, GLenum name, GLfloat param) {
    APITRACE();
    HandleLightModel(name, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, LightModelfv, GLenum name, const GLfloat* params) {
    APITRACE();
    HandleLightModel(name, params, kParamTypeArray);
}

APIENTRY_IMPL(void, LightModelx, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glLightModelf(pname, X2F(param)));
}

APIENTRY_IMPL(void, LightModelxOES, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glLightModelf(pname, X2F(param)));
}

APIENTRY_IMPL(void, LightModelxv, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    TRCALL(glLightModelfv(pname, tmp));
}

APIENTRY_IMPL(void, LightModelxvOES, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glLightModelxv(pname, params));
}

// Configure fixed function lighting state.
namespace {
    void HandleLight(GLenum lightid, GLenum name, const GLfloat* params,
            ParamType param_type) {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return;
        }

        Light* light = c->uniform_context_.MutateLight(lightid);
        if (!light) {
            GLES_ERROR_INVALID_ENUM(lightid);
            return;
        }

        const emugl::Matrix& mv = c->uniform_context_.GetModelViewMatrix();
        const GLfloat value = params[0];
        emugl::Vector vector(params[0], params[1], params[2], params[3]);

        switch (name) {
            case GL_SPOT_EXPONENT:
                if (value < 0.f || value > 128.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(value);
                    return;
                }
                light->spot_exponent = value;
                break;
            case GL_SPOT_CUTOFF:
                if ((value < 0.f || value > 90.f) && value != 180.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(value);
                    return;
                }
                light->spot_cutoff = value;
                break;
            case GL_CONSTANT_ATTENUATION:
                light->const_attenuation = value;
                break;
            case GL_LINEAR_ATTENUATION:
                light->linear_attenuation = value;
                break;
            case GL_QUADRATIC_ATTENUATION:
                light->quadratic_attenuation = value;
                break;
            case GL_AMBIENT:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_AMBIENT.");
                    return;
                }
                light->ambient = vector;
                break;
            case GL_DIFFUSE:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_DIFFUSE.");
                    return;
                }
                light->diffuse = vector;
                break;
            case GL_SPECULAR:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPECULAR.");
                    return;
                }
                light->specular = vector;
                break;
            case GL_POSITION:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_POSITION.");
                    return;
                }
                light->position.AssignMatrixMultiply(mv, vector);
                break;
            case GL_SPOT_DIRECTION:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPOT_DIRECTION.");
                    return;
                }
                vector.Set(3, 0.0f);  // No w-component for spot light direction.
                light->direction.AssignMatrixMultiply(mv, vector);
                light->direction.Set(3, 0.0f);
                break;
            default:
                GLES_ERROR_INVALID_ENUM(name);
                return;
        }
    }
}  // namespace

APIENTRY_IMPL(void, Lightf, GLenum lightid, GLenum name, GLfloat param) {
    APITRACE();
    HandleLight(lightid, name, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, Lightfv, GLenum lightid, GLenum name, const GLfloat* params) {
    APITRACE();
    HandleLight(lightid, name, params, kParamTypeArray);
}

APIENTRY_IMPL(void, Lightx, GLenum light, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glLightf(light, pname, X2F(param)));
}

APIENTRY_IMPL(void, LightxOES, GLenum light, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glLightf(light, pname, X2F(param)));
}

APIENTRY_IMPL(void, Lightxv, GLenum light, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    TRCALL(glLightfv(light, pname, tmp));
}

APIENTRY_IMPL(void, LightxvOES, GLenum light, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glLightxv(light, pname, params));
}

// Sets the width of a line
APIENTRY_IMPL(void, LineWidth, GLfloat width) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (width < 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(width);
        return;
    }
    width = ClampValue(width, c->aliased_line_width_range_.Get()[0],
            c->aliased_line_width_range_.Get()[1]);
    c->line_width_.Mutate() = width;
    PASS_THROUGH(c, LineWidth, width);
}

APIENTRY_IMPL(void, LineWidthx, GLfixed width) {
    APITRACE();
    TRCALL(glLineWidth(X2F(width)));
}

APIENTRY_IMPL(void, LineWidthxOES, GLfixed width) {
    APITRACE();
    TRCALL(glLineWidth(X2F(width)));
}

// Links the current program given the shaders that have been attached to it.
APIENTRY_IMPL(void, LinkProgram, GLuint program) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->Link();
}

// Loads the identity matrix into the top of theactive matrix stack.
APIENTRY_IMPL(void, LoadIdentity) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->uniform_context_.MutateActiveMatrix().AssignIdentity();
}

// Loads a matrix into the top of the active matrix stack.
APIENTRY_IMPL(void, LoadMatrixf, const GLfloat* m) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->uniform_context_.MutateActiveMatrix().SetColumnMajorArray(m);
}

APIENTRY_IMPL(void, LoadMatrixx, const GLfixed* m) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, 16, m);
    TRCALL(glLoadMatrixf(tmp));
}

APIENTRY_IMPL(void, LoadMatrixxOES, const GLfixed* m) {
    APITRACE();
    TRCALL(glLoadMatrixx(m));
}

// Copy the current model view matrix to a matrix in the matrix palette,
// specified by glCurrentPaletteMatrixOES.
APIENTRY_IMPL(void, LoadPaletteFromModelViewMatrixOES) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const emugl::Matrix& mv_matrix = c->uniform_context_.GetModelViewMatrix();
    c->uniform_context_.MutatePaletteMatrix() = mv_matrix;
}

// Configure fixed function material state.
namespace {
    void HandleMaterial(GLenum face, GLenum name, const GLfloat* params,
            ParamType param_type) {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return;
        }

        if (face != GL_FRONT_AND_BACK) {
            GLES_ERROR_INVALID_ENUM(face);
            return;
        }

        Material& material = c->uniform_context_.MutateMaterial();

        const GLfloat value = params[0];
        const emugl::Vector vector(params[0], params[1], params[2], params[3]);

        switch (name) {
            case GL_SHININESS:
                if (value < 0.f || value > 128.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(value);
                    return;
                }
                material.shininess = value;
                break;
            case GL_AMBIENT:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_AMBIENT.");
                    return;
                }
                material.ambient = vector;
                break;
            case GL_DIFFUSE:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_DIFFUSE.");
                    return;
                }
                material.diffuse = vector;
                break;
            case GL_SPECULAR:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPECULAR.");
                    return;
                }
                material.specular = vector;
                break;
            case GL_EMISSION:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_EMISSION.");
                    return;
                }
                material.emission = vector;
                break;
            case GL_AMBIENT_AND_DIFFUSE:
                if (param_type != kParamTypeArray) {
                    GLES_ERROR(GL_INVALID_ENUM,
                            "Expected array for GL_AMBIENT_AND_DIFFUSE.");
                    return;
                }
                material.ambient = vector;
                material.diffuse = vector;
                break;
            default:
                GLES_ERROR_INVALID_ENUM(name);
                return;
        }
    }
}  // namespace

APIENTRY_IMPL(GLvoid*, MapTexSubImage2DCHROMIUM, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum access) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return NULL;
    }

    return PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level, xoffset,
            yoffset, width, height, format, type, access);
}

APIENTRY_IMPL(void, Materialf, GLenum face, GLenum name, GLfloat param) {
    APITRACE();
    HandleMaterial(face, name, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, Materialfv, GLenum face, GLenum name, const GLfloat* params) {
    APITRACE();
    HandleMaterial(face, name, params, kParamTypeArray);
}

APIENTRY_IMPL(void, Materialx, GLenum face, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glMaterialf(face, pname, X2F(param)));
}

APIENTRY_IMPL(void, MaterialxOES, GLenum face, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glMaterialx(face, pname, param));
}

APIENTRY_IMPL(void, Materialxv, GLenum face, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    TRCALL(glMaterialfv(face, pname, tmp));
}

APIENTRY_IMPL(void, MaterialxvOES, GLenum face, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glMaterialxv(face, pname, params));
}

// Selects the active matrix statck.
APIENTRY_IMPL(void, MatrixMode, GLenum mode) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const bool success = c->uniform_context_.SetMatrixMode(mode);
    if (!success) {
        GLES_ERROR_INVALID_ENUM(mode);
    }
}

// Set matrix indices used to blend corresponding matrices for a given vertex.
APIENTRY_IMPL(void, MatrixIndexPointerOES, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }
    if (!IsValidMatrixIndexPointerSize(size)) {
        GLES_ERROR_INVALID_VALUE_INT(size);
        return;
    }
    if (!IsValidMatrixIndexPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    const GLboolean normalized = GL_FALSE;
    c->pointer_context_.SetPointer(kMatrixIndexVertexAttribute, size, type,
            stride, pointer, normalized);
}

APIENTRY_IMPL(void, MatrixIndexPointerOESBounds, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glMatrixIndexPointerOES(size, type, stride, pointer));
}

// Multiplies the matrix on top of the active matrix stack by the given matrix.
APIENTRY_IMPL(void, MultMatrixf, const GLfloat* m) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->uniform_context_.MutateActiveMatrix() *= emugl::Matrix(m);
}

APIENTRY_IMPL(void, MultMatrixx, const GLfixed* m) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, 16, m);
    TRCALL(glMultMatrixf(tmp));
}

APIENTRY_IMPL(void, MultMatrixxOES, const GLfixed* m) {
    APITRACE();
    TRCALL(glMultMatrixx(m));
}

// Sets up unvarying normal vector for the fixed function pipeline.
APIENTRY_IMPL(void, Normal3f, GLfloat nx, GLfloat ny, GLfloat nz) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& normal = c->uniform_context_.MutateNormal();
    normal = emugl::Vector(nx, ny, nz, 0.f);
}

APIENTRY_IMPL(void, Normal3fv, const GLfloat* coords) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& normal = c->uniform_context_.MutateNormal();
    for (int i = 0; i < 3; ++i) {
        normal.Set(i, coords[i]);
    }
}

APIENTRY_IMPL(void, Normal3sv, const GLshort* coords) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector& normal = c->uniform_context_.MutateNormal();
    normal.AssignLinearMapping(coords, 3);
    normal.Set(3, 0.f);
}

APIENTRY_IMPL(void, Normal3x, GLfixed nx, GLfixed ny, GLfixed nz) {
    APITRACE();
    TRCALL(glNormal3f(X2F(nx), X2F(ny), X2F(nz)));
}

APIENTRY_IMPL(void, Normal3xOES, GLfixed nx, GLfixed ny, GLfixed nz) {
    APITRACE();
    TRCALL(glNormal3f(X2F(nx), X2F(ny), X2F(nz)));
}

// Specifies source array for per-vertex normals.
APIENTRY_IMPL(void, NormalPointer, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidNormalPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (stride < 0) {
        GLES_ERROR_INVALID_VALUE_INT(stride);
        return;
    }
    const GLboolean normalized = (type != GL_FLOAT && type != GL_FIXED);
    c->pointer_context_.SetPointer(kNormalVertexAttribute, 3, type, stride,
            pointer, normalized);
}

APIENTRY_IMPL(void, NormalPointerBounds, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glNormalPointer(type, stride, pointer));
}

// Multiplies the current matrix with the specified orthogaphic matrix.
APIENTRY_IMPL(void, Orthof, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat z_near, GLfloat z_far) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->uniform_context_.MutateActiveMatrix() *=
        emugl::Matrix::GenerateOrthographic(left, right, bottom, top, z_near,
                z_far);
}

APIENTRY_IMPL(void, OrthofOES, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    APITRACE();
    TRCALL(glOrthof(left, right, bottom, top, zNear, zFar));
}

APIENTRY_IMPL(void, Orthox, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    APITRACE();
    TRCALL(glOrthof(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
            X2F(zFar)));
}

APIENTRY_IMPL(void, OrthoxOES, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    APITRACE();
    TRCALL(glOrthof(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
            X2F(zFar)));
}

// Configures pixel storage (write) operation, when transfering to GL.
APIENTRY_IMPL(void, PixelStorei, GLenum pname, GLint param) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidPixelStoreName(pname)) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }
    if (!IsValidPixelStoreAlignment(param)) {
        GLES_ERROR_INVALID_VALUE_INT(param);
        return;
    }

    switch (pname) {
        case GL_PACK_ALIGNMENT:
            c->pixel_store_pack_alignment_.Mutate() = param;
            break;
        case GL_UNPACK_ALIGNMENT:
            c->pixel_store_unpack_alignment_.Mutate() = param;
            break;
    }
    PASS_THROUGH(c, PixelStorei, pname, param);
}

// Configure point rendering.
namespace {
    void HandlePointParameter(GLenum pname, const GLfloat* params,
            ParamType param_type) {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return;
        }

        PointParameters& point_parameters =
            c->uniform_context_.MutatePointParameters();

        switch (pname) {
            case GL_POINT_SIZE_MIN:
                if (params[0] < 0.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
                    return;
                }
                point_parameters.size_min = params[0];
                return;

            case GL_POINT_SIZE_MAX:
                if (params[0] < 0.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
                    return;
                }
                point_parameters.size_max = params[0];
                return;

            case GL_POINT_FADE_THRESHOLD_SIZE:
                if (params[0] < 0.f) {
                    GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
                    return;
                }
                // TODO(crbug.com/441918): Support GL_POINT_FADE_THRESHOLD_SIZE.
                LOG_ALWAYS_FATAL(
                        "Unsupported PointParameter GL_POINT_FADE_THRESHOLD_SIZE.");
                return;

            case GL_POINT_DISTANCE_ATTENUATION:
                if (param_type == kParamTypeScalar) {
                    GLES_ERROR(GL_INVALID_ENUM,
                            "Expected array for GL_POINT_DISTANCE_ATTENUATION.");
                    return;
                }
                point_parameters.attenuation[0] = params[0];
                point_parameters.attenuation[1] = params[1];
                point_parameters.attenuation[2] = params[2];
                return;

            default:
                GLES_ERROR(GL_INVALID_ENUM, "Unknown point parameter %s (%d)",
                        GetEnumString(pname), pname);
                return;
        }
    }

}  // namespace

APIENTRY_IMPL(void, PointParameterf, GLenum pname, GLfloat param) {
    APITRACE();
    HandlePointParameter(pname, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, PointParameterfv, GLenum pname, const GLfloat* params) {
    APITRACE();
    HandlePointParameter(pname, params, kParamTypeArray);
}

APIENTRY_IMPL(void, PointParameterx, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glPointParameterf(pname, X2F(param)));
}

APIENTRY_IMPL(void, PointParameterxOES, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glPointParameterf(pname, X2F(param)));
}

APIENTRY_IMPL(void, PointParameterxv, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    TRCALL(glPointParameterfv(pname, tmp));
}

APIENTRY_IMPL(void, PointParameterxvOES, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glPointParameterxv(pname, params));
}

// Sets the point size that is used when no vertex point size array is enabled.
APIENTRY_IMPL(void, PointSize, GLfloat size) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (size <= 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(size);
        return;
    }
    c->uniform_context_.MutatePointParameters().current_size = size;
}

APIENTRY_IMPL(void, PointSizex, GLfixed size) {
    APITRACE();
    TRCALL(glPointSize(X2F(size)));
}

APIENTRY_IMPL(void, PointSizexOES, GLfixed size) {
    APITRACE();
    TRCALL(glPointSize(X2F(size)));
}

// Specifies source array for per-vertex point sizes.
APIENTRY_IMPL(void, PointSizePointerOES, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidPointPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (stride < 0) {
        GLES_ERROR_INVALID_VALUE_INT(stride);
        return;
    }
    LOG_ALWAYS_FATAL_IF(type != GL_FLOAT, "Untested");
    c->pointer_context_.SetPointer(kPointSizeVertexAttribute, 1, type, stride,
            pointer);
}

APIENTRY_IMPL(void, PointSizePointer, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    TRCALL(glPointSizePointerOES(type, stride, pointer));
}

APIENTRY_IMPL(void, PointSizePointerOESBounds, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glPointSizePointerOES(type, stride, pointer));
}

// Configures a depth offset applied to all fragments (eg. for "fixing"
// problems with rendering decals/overlays on top of faces).
APIENTRY_IMPL(void, PolygonOffset, GLfloat factor, GLfloat units) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    c->polygon_offset_factor_.Mutate() = factor;
    c->polygon_offset_units_.Mutate() = units;
    PASS_THROUGH(c, PolygonOffset, factor, units);
}

APIENTRY_IMPL(void, PolygonOffsetx, GLfixed factor, GLfixed units) {
    APITRACE();
    TRCALL(glPolygonOffset(X2F(factor), X2F(units)));
}

APIENTRY_IMPL(void, PolygonOffsetxOES, GLfixed factor, GLfixed units) {
    APITRACE();
    TRCALL(glPolygonOffset(X2F(factor), X2F(units)));
}

// Pops matrix state.
APIENTRY_IMPL(void, PopMatrix) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const bool success = c->uniform_context_.ActiveMatrixPop();
    if (!success) {
        GLES_ERROR(GL_STACK_UNDERFLOW, "PopMatrix called when stack is empty.");
    }
}

// Pushes matrix state.
APIENTRY_IMPL(void, PushMatrix) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    const bool success = c->uniform_context_.ActiveMatrixPush();
    if (!success) {
        GLES_ERROR(GL_STACK_OVERFLOW, "PushMatrix called when stack is full.");
    }
}

// Reads pixels from the frame buffer.
APIENTRY_IMPL(void, ReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, ReadPixels, x, y, width, height, format, type, pixels);
}

APIENTRY_IMPL(void, ReleaseShaderCompiler) {
    APITRACE();
    // Since this is only a hint that shader compilations are unlikely to occur,
    // we can ignore it.
}

// Configures a renderbuffer.
APIENTRY_IMPL(void, RenderbufferStorage, GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidRenderbufferTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidRenderbufferFormat(internalformat)) {
        GLES_ERROR_INVALID_ENUM(internalformat);
        return;
    }
    if (width < 0 || width > c->max_renderbuffer_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(width);
        return;
    }
    if (height < 0 || height > c->max_renderbuffer_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(height);
        return;
    }

    RenderbufferDataPtr obj = c->GetBoundRenderbufferData();
    if (obj == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "No renderbuffer bound.");
        return;
    }

    obj->SetDataStore(target, internalformat, width, height);
    PASS_THROUGH(c, RenderbufferStorage, target, internalformat, width, height);
}

APIENTRY_IMPL(void, RenderbufferStorageOES, GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    APITRACE();
    TRCALL(glRenderbufferStorage(target, internalformat, width, height));
}

// Applies a rotation to the top of the current matrix stack.
APIENTRY_IMPL(void, Rotatef, GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector v(x, y, z, 0);
    c->uniform_context_.MutateActiveMatrix() *=
        emugl::Matrix::GenerateRotationByDegrees(angle, v);
}

APIENTRY_IMPL(void, Rotatex, GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glRotatef(X2F(angle), X2F(x), X2F(y), X2F(z)));
}

APIENTRY_IMPL(void, RotatexOES, GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glRotatef(X2F(angle), X2F(x), X2F(y), X2F(z)));
}

// Changes the way fragment coverage is computed.
APIENTRY_IMPL(void, SampleCoverage, GLclampf value, GLboolean invert) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    value = ClampValue(value, 0.f, 1.f);
    c->sample_coverage_value_.Mutate() = value;
    c->sample_coverage_invert_.Mutate() = invert != GL_FALSE ? GL_TRUE : GL_FALSE;
    PASS_THROUGH(c, SampleCoverage, value, invert);
}

APIENTRY_IMPL(void, SampleCoveragex, GLclampx value, GLboolean invert) {
    APITRACE();
    TRCALL(glSampleCoverage(X2F(value), invert));
}

APIENTRY_IMPL(void, SampleCoveragexOES, GLclampx value, GLboolean invert) {
    APITRACE();
    TRCALL(glSampleCoverage(X2F(value), invert));
}

// Applies a scale to the top of the current matrix stack.
APIENTRY_IMPL(void, Scalef, GLfloat x, GLfloat y, GLfloat z) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector v(x, y, z, 1);
    c->uniform_context_.MutateActiveMatrix() *= emugl::Matrix::GenerateScale(v);
}

APIENTRY_IMPL(void, Scalex, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glScalef(X2F(x), X2F(y), X2F(z)));
}

APIENTRY_IMPL(void, ScalexOES, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glScalef(X2F(x), X2F(y), X2F(z)));
}

// Configures fragment rejection based on whether or not it is inside a
// screen-space rectangle.
APIENTRY_IMPL(void, Scissor, GLint x, GLint y, GLsizei width, GLsizei height) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, Scissor, x, y, width, height);
}

// Configures the shading model to use (how to interpolate colors across
// triangles).
APIENTRY_IMPL(void, ShadeModel, GLenum mode) {
    APITRACE();
    // There is no efficient way to emulate GL_FLAT mode with fragment
    // shaders since they automatically interpolate color values.
    // ALOGW_IF(mode != GL_SMOOTH, "Only GL_SMOOTH shading model supported");
    // We've turned this off for now as it results in spam whenever
    // we try API demos or apps with flag shading.
}

// Loads the source code for a shader into a shader object.
APIENTRY_IMPL(void, ShaderSource, GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (count < 0) {
        GLES_ERROR_INVALID_VALUE_INT(count);
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ShaderDataPtr shader_data = sg->GetShaderData(shader);
    if (shader_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
        return;
    }

    shader_data->SetSource(count, string, length);
}

// Set front and back function and reference value for stencil testing.
APIENTRY_IMPL(void, StencilFunc, GLenum func, GLint ref, GLuint mask) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidStencilFunc(func)) {
        GLES_ERROR_INVALID_ENUM(func);
        return;
    }

    c->stencil_func_.Mutate() = func;
    c->stencil_ref_.Mutate() = ref;
    c->stencil_value_mask_.Mutate() = mask;
    PASS_THROUGH(c, StencilFunc, func, ref, mask);
}

// Set front and/or back function and reference value for stencil testing.
APIENTRY_IMPL(void, StencilFuncSeparate, GLenum face, GLenum func, GLint ref, GLuint mask) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, StencilFuncSeparate, face, func, ref, mask);
}

// Control the front and back writing of individual bits in the stencil
// planes.
APIENTRY_IMPL(void, StencilMask, GLuint mask) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, StencilMask, mask);
}

// Control the front and/or back writing of individual bits in the stencil
// planes.
APIENTRY_IMPL(void, StencilMaskSeparate, GLenum face, GLuint mask) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, StencilMaskSeparate, face, mask);
}

// Set front and back stencil test actions.
APIENTRY_IMPL(void, StencilOp, GLenum sfail, GLenum zfail, GLenum zpass) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, StencilOp, sfail, zfail, zpass);
}

// Set front and/or back stencil test actions.
APIENTRY_IMPL(void, StencilOpSeparate, GLenum face, GLenum sfail, GLenum zfail, GLenum zpass) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, StencilOpSeparate, face, sfail, zfail, zpass);
}

// Specifies source array for per-vertex texture coordinates.
APIENTRY_IMPL(void, TexCoordPointer, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTexCoordPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (!IsValidTexCoordPointerSize(size)) {
        GLES_ERROR_INVALID_VALUE_INT(size);
        return;
    }
    if (stride < 0) {
        GLES_ERROR_INVALID_VALUE_INT(stride);
        return;
    }
    const GLuint index = c->texture_context_.GetClientActiveTextureCoordAttrib();
    c->pointer_context_.SetPointer(index, size, type, stride, pointer);
}

APIENTRY_IMPL(void, TexCoordPointerBounds, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glTexCoordPointer(size, type, stride, pointer));
}

// Sets up a texture environment for the current texture unit for the fixed
// function pipeline.
namespace {
    template <typename T>
        void HandleTexEnv(GLenum target, GLenum pname, T* params,
                ParamType param_type) {
            APITRACE();
            ContextPtr c = GetCurrentGlesContext();
            if (!c) {
                return;
            }

            TexEnv& texenv = c->uniform_context_.MutateActiveTexEnv();
            if (target != GL_TEXTURE_ENV) {
                GLES_ERROR_INVALID_ENUM(target);
                return;
            }

            const GLenum param = static_cast<GLenum>(params[0]);
            const float value = static_cast<float>(params[0]);
            switch (pname) {
                case GL_TEXTURE_ENV_MODE:
                    if (!IsValidTexEnvMode(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.mode = param;
                    break;
                case GL_SRC0_RGB:
                case GL_SRC1_RGB:
                case GL_SRC2_RGB:
                    if (!IsValidTexEnvSource(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.src_rgb[pname - GL_SRC0_RGB] = param;
                    break;
                case GL_SRC0_ALPHA:
                case GL_SRC1_ALPHA:
                case GL_SRC2_ALPHA:
                    if (!IsValidTexEnvSource(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.src_alpha[pname - GL_SRC0_ALPHA] = param;
                    break;
                case GL_OPERAND0_RGB:
                case GL_OPERAND1_RGB:
                case GL_OPERAND2_RGB:
                    if (!IsValidTexEnvOperandRgb(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.operand_rgb[pname - GL_OPERAND0_RGB] = param;
                    break;
                case GL_OPERAND0_ALPHA:
                case GL_OPERAND1_ALPHA:
                case GL_OPERAND2_ALPHA:
                    if (!IsValidTexEnvOperandAlpha(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.operand_alpha[pname - GL_OPERAND0_ALPHA] = param;
                    break;
                case GL_COMBINE_RGB:
                    if (!IsValidTexEnvCombineRgb(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.combine_rgb = param;
                    break;
                case GL_COMBINE_ALPHA:
                    if (!IsValidTexEnvCombineAlpha(param)) {
                        GLES_ERROR_INVALID_ENUM(param);
                        return;
                    }
                    texenv.combine_alpha = param;
                    break;
                case GL_RGB_SCALE:
                    if (!IsValidTexEnvScale(value)) {
                        GLES_ERROR_INVALID_VALUE_FLOAT(value);
                        return;
                    }
                    texenv.rgb_scale = value;
                    break;
                case GL_ALPHA_SCALE:
                    if (!IsValidTexEnvScale(value)) {
                        GLES_ERROR_INVALID_VALUE_FLOAT(value);
                        return;
                    }
                    texenv.alpha_scale = value;
                    break;
                case GL_TEXTURE_ENV_COLOR:
                    if (param_type != kParamTypeArray) {
                        GLES_ERROR(GL_INVALID_ENUM,
                                "Expected array for GL_TEXTURE_ENV_COLOR.");
                        return;
                    }
                    texenv.color.AssignLinearMapping(params, 4);
                    texenv.color.Clamp(0.f, 1.f);
                    break;
                case GL_COORD_REPLACE_OES:
                    LOG_ALWAYS_FATAL("GL_COORD_REPLACE_OES not supported.");
                    break;
                default:
                    GLES_ERROR_INVALID_ENUM(pname);
                    break;
            }
        }
}  // namespace

APIENTRY_IMPL(void, TexEnvf, GLenum target, GLenum pname, GLfloat param) {
    APITRACE();
    HandleTexEnv(target, pname, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, TexEnvfv, GLenum target, GLenum pname, const GLfloat* params) {
    APITRACE();
    HandleTexEnv(target, pname, params, kParamTypeArray);
}

APIENTRY_IMPL(void, TexEnvi, GLenum target, GLenum pname, GLint param) {
    APITRACE();
    HandleTexEnv(target, pname, &param, kParamTypeScalar);
}

APIENTRY_IMPL(void, TexEnviv, GLenum target, GLenum pname, const GLint* params) {
    APITRACE();
    HandleTexEnv(target, pname, params, kParamTypeArray);
}

APIENTRY_IMPL(void, TexEnvx, GLenum target, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glTexEnvf(target, pname, static_cast<GLfloat>(param)));
}

APIENTRY_IMPL(void, TexEnvxOES, GLenum target, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glTexEnvf(target, pname, static_cast<GLfloat>(param)));
}

APIENTRY_IMPL(void, TexEnvxv, GLenum target, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    if (pname == GL_TEXTURE_ENV_COLOR) {
        Convert(tmp, ParamSize(pname), params);
    } else {
        tmp[0] = static_cast<GLfloat>(params[0]);
    }
    TRCALL(glTexEnvfv(target, pname, tmp));
}

APIENTRY_IMPL(void, TexEnvxvOES, GLenum target, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glTexEnvxv(target, pname, params));
}

// Control the generation of texture coordinates.
APIENTRY_IMPL(void, TexGeniOES, GLenum coord, GLenum pname, GLint param) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTexGenCoord(coord)) {
        GLES_ERROR_INVALID_ENUM(coord);
        return;
    }

    // opengles_spec_1_1_extension_pack.pdf 4.1
    // Only GL_TEXTURE_GEN_MODE_OES is supported
    switch (pname) {
        case GL_TEXTURE_GEN_MODE_OES: {
                                          if (!IsValidTexGenMode(param)) {
                                              GLES_ERROR_INVALID_ENUM(param);
                                              return;
                                          }
                                          TexGen& texgen = c->uniform_context_.MutateActiveTexGen();
                                          texgen.mode = param;
                                          break;
                                      }
        default:
                                      GLES_ERROR_INVALID_ENUM(pname);
                                      break;
    }
}

APIENTRY_IMPL(void, TexGenivOES, GLenum coord, GLenum pname, const GLint* params) {
    APITRACE();
    TRCALL(glTexGeniOES(coord, pname, params[0]));
}

APIENTRY_IMPL(void, TexGenxOES, GLenum coord, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glTexGeniOES(coord, pname, param));
}

APIENTRY_IMPL(void, TexGenxvOES, GLenum coord, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glTexGenivOES(coord, pname, params));
}

// Loads pixel data into a texture object.
APIENTRY_IMPL(void, TexImage2D, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (target == GL_TEXTURE_EXTERNAL_OES) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }

    if (!IsValidTextureTargetEx(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidPixelFormat(format)) {
        GLES_ERROR_INVALID_ENUM(format);
        return;
    }
    if (!IsValidPixelType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (!IsValidPixelFormatAndType(format, type)) {
        GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(format), GetEnumString(type));
        return;
    }
    if (internalformat != ((GLint)format)) {
        GLES_ERROR(GL_INVALID_OPERATION, "Format must match internal format.");
        return;
    }
    if (border != 0) {
        GLES_ERROR_INVALID_VALUE_INT(border);
        return;
    }
    if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
        GLES_ERROR_INVALID_VALUE_INT(level);
        return;
    }
    if (width < 0 || width > c->max_texture_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(width);
        return;
    }
    if (height < 0 || height > c->max_texture_size_.Get()) {
        GLES_ERROR_INVALID_VALUE_INT(height);
        return;
    }

    TextureDataPtr tex = c->GetBoundTextureData(target);
    LOG_ALWAYS_FATAL_IF(tex == NULL, "There should always be a texture bound.");

    tex->Set(level, width, height, format, type);

    if (tex->IsEglImageAttached()) {
        DLOG("egl image attached to this texture");
        // This texture was a target of EGLImage, but now it is re-defined
        // so we need to detach from the EGLImage and re-generate a
        // global texture name for it, and reset the texture to be bound to a
        // TEXTURE_2D target.
        tex->DetachEglImage();
        GLuint global = 0;
        PASS_THROUGH(c, GenTextures, 1, &global);
        const GLuint name = tex->GetLocalName();
        c->GetShareGroup()->SetTextureGlobalName(name, global);
        c->texture_context_.SetTargetTexture(GL_TEXTURE_2D, name, GL_TEXTURE_2D);
        PASS_THROUGH(c, BindTexture, GL_TEXTURE_2D, global);
    }

    c->texture_context_.EnsureCorrectBinding(target);
    DLOG("call underlying");
    PASS_THROUGH(c, TexImage2D, target, level, internalformat, width, height,
            border, format, type, pixels);
    DLOG("call RestoreBinding (called pass through)");
    c->texture_context_.RestoreBinding(target);
    DLOG("Done restoring binding");

    if (tex->IsAutoMipmap() && level == 0) {
        // TODO(crbug.com/441913): Update information for all levels.
        PASS_THROUGH(c, GenerateMipmap, target);
    }
    DLOG("Exit");
}

// Configure a texture object.
APIENTRY_IMPL(void, TexParameterf, GLenum target, GLenum pname, GLfloat param) {
    APITRACE();
    TRCALL(glTexParameterfv(target, pname, &param));
}

APIENTRY_IMPL(void, TexParameterfv, GLenum target, GLenum pname, const GLfloat* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTextureTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidTextureParam(pname)) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    if (pname == GL_TEXTURE_CROP_RECT_OES) {
        TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex != NULL) {
            tex->SetCropRect(reinterpret_cast<const GLint*>(params));
        }
    } else if (pname == GL_GENERATE_MIPMAP) {
        TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex != NULL) {
            tex->SetAutoMipmap(*params);
        }
    } else {
        GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
        PASS_THROUGH(c, TexParameterfv, global_target, pname, params);
        c->texture_context_.RestoreBinding(target);
    }
}

APIENTRY_IMPL(void, TexParameteri, GLenum target, GLenum pname, GLint param) {
    APITRACE();
    TRCALL(glTexParameteriv(target, pname, &param));
}

APIENTRY_IMPL(void, TexParameteriv, GLenum target, GLenum pname, const GLint* params) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidTextureTarget(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidTextureParam(pname)) {
        GLES_ERROR_INVALID_ENUM(pname);
        return;
    }

    if (pname == GL_TEXTURE_CROP_RECT_OES) {
        TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex != NULL) {
            tex->SetCropRect(params);
        }
    } else if (pname == GL_GENERATE_MIPMAP) {
        TextureDataPtr tex = c->GetBoundTextureData(target);
        if (tex != NULL) {
            tex->SetAutoMipmap(*params);
        }
    } else {
        GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
        PASS_THROUGH(c, TexParameteriv, global_target, pname, params);
        c->texture_context_.RestoreBinding(target);
    }
}

APIENTRY_IMPL(void, TexParameterx, GLenum target, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glTexParameterxv(target, pname, &param));
}

APIENTRY_IMPL(void, TexParameterxOES, GLenum target, GLenum pname, GLfixed param) {
    APITRACE();
    TRCALL(glTexParameterxv(target, pname, &param));
}

APIENTRY_IMPL(void, TexParameterxv, GLenum target, GLenum pname, const GLfixed* params) {
    APITRACE();
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    if (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ||
            pname == GL_TEXTURE_CROP_RECT_OES) {
        TRCALL(glTexParameteriv(target, pname, reinterpret_cast<const GLint*>(params)));
    } else {
        TRCALL(glTexParameterfv(target, pname, tmp));
    }
}

APIENTRY_IMPL(void, TexParameterxvOES, GLenum target, GLenum pname, const GLfixed* params) {
    APITRACE();
    TRCALL(glTexParameterxv(target, pname, params));
}

namespace {

    bool HandleTexSubImage2D(GLenum target, GLint level, GLint xoffset,
            GLint yoffset, GLsizei width, GLsizei height,
            GLenum format, GLenum type, const GLvoid* pixels) {
        ContextPtr c = GetCurrentGlesContext();
        ALOG_ASSERT(c != NULL);
        TextureDataPtr tex = c->GetBoundTextureData(target);
        const GLenum current_format = tex->GetFormat(level);
        const GLenum current_type = tex->GetType(level);

        const bool requires_conversion =
            (format != current_format || type != current_type);
        if (!requires_conversion) {
            PASS_THROUGH(c, TexSubImage2D, target, level, xoffset, yoffset, width,
                    height, format, type, pixels);
        } else {
            TextureConverter converter(format, type, current_format, current_type);
            if (!converter.IsValid()) {
                GLES_ERROR(GL_INVALID_OPERATION,
                        "Texture conversion from %s %s to %s %s not supported.",
                        GetEnumString(format), GetEnumString(type),
                        GetEnumString(current_format), GetEnumString(current_type));
                return false;
            }

            // Set unpack alignment to 4, because TextureConverter always generates
            // texture with alignment 4.
            if (c->pixel_store_unpack_alignment_.Get() != 4) {
                PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 4);
            }

            GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level,
                    xoffset, yoffset, width, height,
                    current_format, current_type,
                    GL_WRITE_ONLY_OES);
            if (!buffer) {
                GLES_ERROR(GL_OUT_OF_MEMORY, "Out of memory.");
                return false;
            }
            converter.Convert(width, height, c->pixel_store_unpack_alignment_.Get(),
                    pixels, buffer);
            PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);

            // Restore unpack alignment.
            if (c->pixel_store_unpack_alignment_.Get() != 4) {
                PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT,
                        c->pixel_store_unpack_alignment_.Get());
            }
        }
        return true;
    }

}  // namespace

// Redefines a contiguous subregion of a texture.  In GLES 2 profile
// width, height, format, type, and data must match the values originally
// specified to TexImage2D.  See es_full_spec_2.0.25.pdf section 3.7.2.
APIENTRY_IMPL(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

    if (!IsValidTextureTargetEx(target)) {
        GLES_ERROR_INVALID_ENUM(target);
        return;
    }
    if (!IsValidPixelFormat(format)) {
        GLES_ERROR_INVALID_ENUM(format);
        return;
    }
    if (!IsValidPixelType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (!IsValidPixelFormatAndType(format, type)) {
        GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(format), GetEnumString(type));
        return;
    }
    if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
        GLES_ERROR_INVALID_VALUE_INT(level);
        return;
    }
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (width < 0 || (xoffset + width) > static_cast<GLsizei>(tex->GetWidth())) {
        GLES_ERROR_INVALID_VALUE_INT(width);
        return;
    }
    if (height < 0 || (yoffset + height) > static_cast<GLsizei>(tex->GetHeight())) {
        GLES_ERROR_INVALID_VALUE_INT(height);
        return;
    }
    if (xoffset < 0) {
        GLES_ERROR_INVALID_VALUE_INT(xoffset);
        return;
    }
    if (yoffset < 0) {
        GLES_ERROR_INVALID_VALUE_INT(yoffset);
        return;
    }

    if (HandleTexSubImage2D(target, level, xoffset, yoffset, width, height,
                format, type, pixels)) {
        if (tex != NULL && tex->IsAutoMipmap() && level == 0) {
            // TODO(crbug.com/441913): Update information for all levels.
            PASS_THROUGH(c, GenerateMipmap, target);
        }
    }
}

// Applies a translation to the top of the current matrix stack.
APIENTRY_IMPL(void, Translatef, GLfloat x, GLfloat y, GLfloat z) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    emugl::Vector v(x, y, z, 1);
    c->uniform_context_.MutateActiveMatrix() *=
        emugl::Matrix::GenerateTranslation(v);
}

APIENTRY_IMPL(void, Translatex, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glTranslatef(X2F(x), X2F(y), X2F(z)));
}

APIENTRY_IMPL(void, TranslatexOES, GLfixed x, GLfixed y, GLfixed z) {
    APITRACE();
    TRCALL(glTranslatef(X2F(x), X2F(y), X2F(z)));
}

namespace {

    ProgramDataPtr GetCurrentProgramData() {
        ContextPtr c = GetCurrentGlesContext();
        if (!c) {
            return ProgramDataPtr();
        }

        ProgramDataPtr program_data = c->GetCurrentUserProgram();
        if (program_data == NULL) {
            GLES_ERROR(GL_INVALID_OPERATION, "No active program on this context");
        }
        return program_data;
    }

}  // namespace

// Loads values into the active uniform state.
APIENTRY_IMPL(void, Uniform1f, GLint location, GLfloat x) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformfv(location, GL_FLOAT, 1, &x);
    }
}

APIENTRY_IMPL(void, Uniform1fv, GLint location, GLsizei count, const GLfloat* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformfv(location, GL_FLOAT, count, v);
    }
}

APIENTRY_IMPL(void, Uniform1i, GLint location, GLint x) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformiv(location, GL_INT, 1, &x);
    }
}

APIENTRY_IMPL(void, Uniform1iv, GLint location, GLsizei count, const GLint* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformiv(location, GL_INT, count, v);
    }
}

APIENTRY_IMPL(void, Uniform2f, GLint location, GLfloat x, GLfloat y) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLfloat params[] = {x, y};
        program_data->Uniformfv(location, GL_FLOAT_VEC2, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform2fv, GLint location, GLsizei count, const GLfloat* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformfv(location, GL_FLOAT_VEC2, count, v);
    }
}

APIENTRY_IMPL(void, Uniform2i, GLint location, GLint x, GLint y) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLint params[] = {x, y};
        program_data->Uniformiv(location, GL_INT_VEC2, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform2iv, GLint location, GLsizei count, const GLint* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformiv(location, GL_INT_VEC2, count, v);
    }
}

APIENTRY_IMPL(void, Uniform3f, GLint location, GLfloat x, GLfloat y, GLfloat z) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLfloat params[] = {x, y, z};
        program_data->Uniformfv(location, GL_FLOAT_VEC3, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform3fv, GLint location, GLsizei count, const GLfloat* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformfv(location, GL_FLOAT_VEC3, count, v);
    }
}

APIENTRY_IMPL(void, Uniform3i, GLint location, GLint x, GLint y, GLint z) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLint params[] = {x, y, z};
        program_data->Uniformiv(location, GL_INT_VEC3, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform3iv, GLint location, GLsizei count, const GLint* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformiv(location, GL_INT_VEC3, count, v);
    }
}

APIENTRY_IMPL(void, Uniform4f, GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLfloat params[] = {x, y, z, w};
        program_data->Uniformfv(location, GL_FLOAT_VEC4, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform4fv, GLint location, GLsizei count, const GLfloat* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformfv(location, GL_FLOAT_VEC4, count, v);
    }
}

APIENTRY_IMPL(void, Uniform4i, GLint location, GLint x, GLint y, GLint z, GLint w) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        GLint params[] = {x, y, z, w};
        program_data->Uniformiv(location, GL_INT_VEC4, 1, params);
    }
}

APIENTRY_IMPL(void, Uniform4iv, GLint location, GLsizei count, const GLint* v) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->Uniformiv(location, GL_INT_VEC4, count, v);
    }
}

APIENTRY_IMPL(void, UniformMatrix2fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->UniformMatrixfv(
                location, GL_FLOAT_MAT2, count, transpose, value);
    }
}

APIENTRY_IMPL(void, UniformMatrix3fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->UniformMatrixfv(
                location, GL_FLOAT_MAT3, count, transpose, value);
    }
}

APIENTRY_IMPL(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    APITRACE();
    ProgramDataPtr program_data = GetCurrentProgramData();
    if (program_data != NULL) {
        program_data->UniformMatrixfv(
                location, GL_FLOAT_MAT4, count, transpose, value);
    }
}

APIENTRY_IMPL(void, UnmapTexSubImage2DCHROMIUM, const GLvoid* mem) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, mem);
}

// Selects a program to use for subsequent rendering.
APIENTRY_IMPL(void, UseProgram, GLuint program) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_ptr = sg->GetProgramData(program);
    if (program_ptr == NULL) {
        if (program) {
            GLES_ERROR(GL_INVALID_OPERATION, "Unknown program name %d", program);
        }
        return;
    }

    c->SetCurrentUserProgram(program_ptr);
}

// Validates the indicated program.
APIENTRY_IMPL(void, ValidateProgram, GLuint program) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    ShareGroupPtr sg = c->GetShareGroup();
    ProgramDataPtr program_data = sg->GetProgramData(program);
    if (program_data == NULL) {
        GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
        return;
    }

    program_data->Validate();
}

// Sets up a vertex attribute value or array to use for the vertex shader.
APIENTRY_IMPL(void, VertexAttrib1f, GLuint indx, GLfloat x) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib1f, indx, x);
}

APIENTRY_IMPL(void, VertexAttrib1fv, GLuint indx, const GLfloat* values) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib1fv, indx, values);
}

APIENTRY_IMPL(void, VertexAttrib2f, GLuint indx, GLfloat x, GLfloat y) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib2f, indx, x, y);
}

APIENTRY_IMPL(void, VertexAttrib2fv, GLuint indx, const GLfloat* values) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib2fv, indx, values);
}

APIENTRY_IMPL(void, VertexAttrib3f, GLuint indx, GLfloat x, GLfloat y, GLfloat z) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib3f, indx, x, y, z);
}

APIENTRY_IMPL(void, VertexAttrib3fv, GLuint indx, const GLfloat* values) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib3fv, indx, values);
}

APIENTRY_IMPL(void, VertexAttrib4f, GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib4f, indx, x, y, z, w);
}

APIENTRY_IMPL(void, VertexAttrib4fv, GLuint indx, const GLfloat* values) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    PASS_THROUGH(c, VertexAttrib4fv, indx, values);
}

// Specify an array of generic vertex attribute data.
APIENTRY_IMPL(void, VertexAttribPointer, GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (indx >= (GLuint)c->max_vertex_attribs_.Get()) {
        GLES_ERROR(GL_INVALID_VALUE, "Invalid index: %d", indx);
        return;
    }
    if (!IsValidVertexAttribSize(size)) {
        GLES_ERROR(GL_INVALID_VALUE, "Invalid size: %d", size);
        return;
    }
    if (!IsValidVertexAttribType(type)) {
        GLES_ERROR(GL_INVALID_ENUM, "Invalid type: %d", type);
        return;
    }
    if (stride < 0) {
        GLES_ERROR(GL_INVALID_VALUE, "Invalid stride: %d.", stride);
        return;
    }
    c->pointer_context_.SetPointer(indx, size, type, stride, pointer, normalized);
}

// Specifies source array for vertex coordinates.
APIENTRY_IMPL(void, VertexPointer, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidVertexPointerSize(size)) {
        GLES_ERROR_INVALID_VALUE_INT(size);
        return;
    }
    if (!IsValidVertexPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }
    if (stride < 0) {
        GLES_ERROR_INVALID_VALUE_INT(stride);
        return;
    }
    c->pointer_context_.SetPointer(kPositionVertexAttribute, size, type, stride,
            pointer);
}

APIENTRY_IMPL(void, VertexPointerBounds, GLint size, GLenum type, GLsizei stride, GLvoid* pointer, GLsizei count) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    TRCALL(glVertexPointer(size, type, stride, pointer));
}

// Sets up the current viewport.
APIENTRY_IMPL(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (width < 0) {
        GLES_ERROR_INVALID_VALUE_INT(width);
        return;
    }
    if (height < 0) {
        GLES_ERROR_INVALID_VALUE_INT(height);
        return;
    }

    width = ClampValue(width, 0, c->max_viewport_dims_.Get()[0]);
    height = ClampValue(height, 0, c->max_viewport_dims_.Get()[1]);
    GLint (&viewport)[4] = c->viewport_.Mutate();
    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = width;
    viewport[3] = height;
    PASS_THROUGH(c, Viewport, x, y, width, height);
}

// Set the weights used to blend corresponding matrices for a given vertex.
APIENTRY_IMPL(void, WeightPointerOES, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
    APITRACE();
    ContextPtr c = GetCurrentGlesContext();
    if (!c) {
        return;
    }

    if (!IsValidWeightPointerSize(size)) {
        GLES_ERROR_INVALID_VALUE_INT(size);
        return;
    }
    if (!IsValidWeightPointerType(type)) {
        GLES_ERROR_INVALID_ENUM(type);
        return;
    }

    const GLboolean normalized = GL_FALSE;
    c->pointer_context_.SetPointer(kWeightVertexAttribute, size, type, stride,
            pointer, normalized);
}

APIENTRY_IMPL(void, WeightPointerOESBounds, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, GLsizei count) {
    APITRACE();
    TRCALL(glWeightPointerOES(size, type, stride, pointer));
}

