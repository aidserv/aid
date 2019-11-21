/*
* Copyright 2011 The Android Open Source Project
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

#ifndef GLES_V2_VALIDATE_H
#define GLES_V2_VALIDATE_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLcommon/GLESvalidate.h>

struct GLESv2Validate : public GLESvalidate {

    static bool renderbufferParam(GLEScontext* ctx, GLenum param);
    static bool framebufferTarget(GLEScontext* ctx, GLenum target);
    static bool framebufferAttachment(GLEScontext* ctx, GLenum attachment);
    static bool bufferTarget(GLEScontext* ctx, GLenum target);
    static bool bufferUsage(GLEScontext* ctx, GLenum usage);
    static bool bufferParam(GLEScontext* ctx, GLenum pname);
    static bool blendEquationMode(GLEScontext* ctx, GLenum mode);
    static bool blendSrc(GLenum s);
    static bool blendDst(GLenum d);
    static bool textureTarget(GLEScontext* ctx, GLenum param);
    static bool textureParams(GLEScontext* ctx, GLenum param);
    static bool hintTargetMode(GLenum target,GLenum mode);
    static bool capability(GLenum cap);
    static bool pixelStoreParam(GLEScontext* ctx, GLenum param);
    static bool readPixelFrmt(GLenum format);
    static bool shaderType(GLEScontext* ctx, GLenum type);
    static bool precisionType(GLenum type);
    static bool arrayIndex(GLEScontext * ctx,GLuint index);
    static bool pixelType(GLEScontext * ctx,GLenum type);
    static bool pixelFrmt(GLEScontext* ctx,GLenum format);
    static bool pixelItnlFrmt(GLEScontext* ctx, GLenum internalformat);
    static bool pixelSizedFrmt(GLEScontext* ctx, GLenum internalformat,
            GLenum format, GLenum type);
    static bool isCompressedFormat(GLenum format);
    static void getCompatibleFormatTypeForInternalFormat(GLenum internalformat, GLenum* format_out, GLenum* type_out);
    static bool attribName(const GLchar* name);
    static bool attribIndex(int index, int max);
    static bool programParam(GLEScontext* ctx, GLenum pname);
    static bool textureIsCubeMap(GLenum target);
    static bool textureTargetEx(GLEScontext* ctx, GLenum textarget);

    static int sizeOfType(GLenum type);
};

#endif
