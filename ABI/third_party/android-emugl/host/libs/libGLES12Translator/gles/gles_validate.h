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
#ifndef GRAPHICS_TRANSLATION_GLES_GLES_VALIDATE_H_
#define GRAPHICS_TRANSLATION_GLES_GLES_VALIDATE_H_

#include <GLES/gl.h>

inline bool IsPowerOf2(int num) {
  return (num & (num - 1)) == 0;
}

bool IsValidAlphaFunc(GLenum func);
bool IsValidBlendFunc(GLenum factor);
bool IsValidBufferTarget(GLenum target);
bool IsValidBufferUsage(GLenum usage);
bool IsValidClientState(GLenum state);
bool IsValidColorPointerSize(GLint size);
bool IsValidColorPointerType(GLenum type);
bool IsValidCullFaceMode(GLenum mode);
bool IsValidDepthFunc(GLenum func);
bool IsValidDrawMode(GLenum mode);
bool IsValidDrawType(GLenum mode);
bool IsValidEmulatedCompressedTextureFormats(GLenum type);
bool IsValidFramebufferAttachment(GLenum attachment);
bool IsValidFramebufferAttachmentParams(GLenum pname);
bool IsValidFramebufferTarget(GLenum target);
bool IsValidFrontFaceMode(GLenum mode);
bool IsValidMatrixIndexPointerSize(GLint size);
bool IsValidMatrixIndexPointerType(GLenum type);
bool IsValidMipmapHintMode(GLenum mode);
bool IsValidNormalPointerType(GLenum type);
bool IsValidPixelFormat(GLenum format);
bool IsValidPixelFormatAndType(GLenum format, GLenum type);
bool IsValidPixelStoreAlignment(GLint value);
bool IsValidPixelStoreName(GLenum name);
bool IsValidPixelType(GLenum type);
bool IsValidPointPointerType(GLenum type);
bool IsValidPrecisionType(GLenum type);
bool IsValidRenderbufferFormat(GLenum format);
bool IsValidRenderbufferParams(GLenum pname);
bool IsValidRenderbufferTarget(GLenum target);
bool IsValidShaderType(GLenum type);
bool IsValidStencilFunc(GLenum func);
bool IsValidTexCoordPointerSize(GLint size);
bool IsValidTexCoordPointerType(GLenum type);
bool IsValidTexEnv(GLenum target, GLenum pname);
bool IsValidTexEnvCombineAlpha(GLenum param);
bool IsValidTexEnvCombineRgb(GLenum param);
bool IsValidTexEnvMode(GLenum param);
bool IsValidTexEnvOperandAlpha(GLenum param);
bool IsValidTexEnvOperandRgb(GLenum param);
bool IsValidTexEnvScale(GLfloat scale);
bool IsValidTexEnvSource(GLenum param);
bool IsValidTexGenCoord(GLenum coord);
bool IsValidTexGenMode(GLenum mode);
bool IsValidTextureDimensions(GLsizei width, GLsizei height,
                              GLint max_texture_size);
bool IsValidTextureEnum(GLenum unit, GLuint max_num_texture);
bool IsValidTextureParam(GLenum pname);
bool IsValidTextureTarget(GLenum target);
bool IsValidTextureTargetAndParam(GLenum target, GLenum pname);
bool IsValidTextureTargetEx(GLenum target);
bool IsValidTextureTargetLimited(GLenum target);
bool IsValidVertexAttribSize(GLint size);
bool IsValidVertexAttribType(GLenum type);
bool IsValidVertexPointerSize(GLint size);
bool IsValidVertexPointerType(GLenum type);
bool IsValidWeightPointerSize(GLint size);
bool IsValidWeightPointerType(GLenum type);

#endif  // GRAPHICS_TRANSLATION_GLES_GLES_VALIDATE_H_
