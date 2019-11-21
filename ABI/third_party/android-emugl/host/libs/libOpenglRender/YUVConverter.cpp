/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "YUVConverter.h"

#include "DispatchTables.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define FATAL(fmt,...) do { \
    fprintf(stderr, "%s: FATAL: " fmt "\n", __func__, ##__VA_ARGS__); \
    assert(false); \
} while(0)

static void getPlanarYUVSizes(int width, int height,
                              FrameworkFormat format,
                              uint32_t* nBytes_out,
                              uint32_t* yStride_out,
                              uint32_t* cStride_out,
                              uint32_t* cHeight_out) {
    assert(nBytes_out);
    assert(yStride_out);
    assert(cStride_out);
    assert(cHeight_out);

    uint32_t align = 1;
    switch (format) {
    case FRAMEWORK_FORMAT_YV12:
        align = 16;
        break;
    case FRAMEWORK_FORMAT_YUV_420_888:
        align = 1;
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    }

    // 16-alignment means we need to get the
    // smallest multiple of 16 pixels that is
    // greater than or equal to |width| for |yStride| and
    // greater than or equal to |width / 2| for |cStride|
    uint32_t yStride = (width + (align - 1)) & ~(align - 1);
    uint32_t cStride = (yStride / 2 + (align - 1)) & ~(align - 1);
    uint32_t cHeight = height / 2;

    *nBytes_out = yStride * height + 2 * (cStride * cHeight);
    *yStride_out = yStride;
    *cStride_out = cStride;
    *cHeight_out = cHeight;
}

// getYUVSizes(): given |width| and |height|, return
// the total number of bytes of the YUV-formatted buffer
// in |nBytes_out|, and store aligned width and C height
// in |yStride_out|/|cStride_out| and |cHeight_out|.
static void getYUVSizes(int width, int height,
                        FrameworkFormat format,
                        uint32_t* nBytes_out,
                        uint32_t* yStride_out,
                        uint32_t* cStride_out,
                        uint32_t* cHeight_out) {
    switch (format) {
    case FRAMEWORK_FORMAT_YV12:
    case FRAMEWORK_FORMAT_YUV_420_888:
        getPlanarYUVSizes(width, height, format,
                          nBytes_out,
                          yStride_out,
                          cStride_out,
                          cHeight_out);
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    }
}

static void getPlanarYUVOffsets(int width, int height, FrameworkFormat format,
                                uint32_t* yoff, uint32_t* uoff, uint32_t* voff,
                                uint32_t* alignwidth, uint32_t* alignwidthc) {
    uint32_t totalSize, yStride, cStride, cHeight;
    cStride = 0;
    yStride = 0;
    totalSize = 0;
    getYUVSizes(width, height, format, &totalSize, &yStride, &cStride, &cHeight);
    int cSize = cStride * height / 2;

    switch (format) {
    case FRAMEWORK_FORMAT_YV12:
        // In our use cases (so far), the buffer should
        // always begin with Y.
        *yoff = 0;
        // The U and V planes are switched around so physically
        // YV12 is more of a "YVU" format
        *voff = (*yoff) + yStride * height;
        *uoff = (*voff) + cSize;
        *alignwidth = yStride;
        *alignwidthc = cStride;
        break;
    case FRAMEWORK_FORMAT_YUV_420_888:
        *yoff = 0;
        *uoff = (*yoff) + yStride * height;
        *voff = (*uoff) + cSize;
        *alignwidth = yStride;
        *alignwidthc = cStride;
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    }
}

// getYUVOffsets(), given a YUV-formatted buffer that is arranged
// according to the spec
// https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV
// In particular, Android YUV widths are aligned to 16 pixels.
// Inputs:
// |yv12|: the YUV-formatted buffer
// Outputs:
// |yoff|: offset into |yv12| of the start of the Y component
// |uoff|: offset into |yv12| of the start of the U component
// |voff|: offset into |yv12| of the start of the V component
static void getYUVOffsets(int width, int height, FrameworkFormat format,
                          uint32_t* yoff, uint32_t* uoff, uint32_t* voff,
                          uint32_t* alignwidth, uint32_t* alignwidthc) {
    switch (format) {
    case FRAMEWORK_FORMAT_YV12:
    case FRAMEWORK_FORMAT_YUV_420_888:
        getPlanarYUVOffsets(width, height, format,
                            yoff, uoff, voff,
                            alignwidth, alignwidthc);
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    }
}

// createYUVGLTex() allocates GPU memory that is enough
// to hold the raw data of the YV12 buffer.
// The memory is in the form of an OpenGL texture
// with one component (GL_LUMINANCE) and
// of type GL_UNSIGNED_BYTE.
// In order to process all Y, U, V components
// simultaneously in conversion, the simple thing to do
// is to use multiple texture units, hence
// the |texture_unit| argument.
// Returns a new OpenGL texture object in |texName_out|
// that is to be cleaned up by the caller.
static void createYUVGLTex(GLenum texture_unit,
                           GLsizei width,
                           GLsizei height,
                           GLuint* texName_out) {
    assert(texName_out);

    s_gles2.glActiveTexture(texture_unit);
    s_gles2.glGenTextures(1, texName_out);
    s_gles2.glBindTexture(GL_TEXTURE_2D, *texName_out);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                         width, height, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         NULL);
    s_gles2.glActiveTexture(GL_TEXTURE0);
}

// subUpdateYUVGLTex() updates a given YUV texture
// at the coordinates (x, y, width, height),
// with the raw YUV data in |pixels|.
// We cannot view the result properly until
// after conversion; this is to be used only
// as input to the conversion shader.
static void subUpdateYUVGLTex(GLenum texture_unit,
                              GLuint tex,
                              int x, int y, int width, int height,
                              void* pixels) {
    s_gles2.glActiveTexture(texture_unit);
    s_gles2.glBindTexture(GL_TEXTURE_2D, tex);
    s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0,
                            x, y, width, height,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            pixels);
    s_gles2.glActiveTexture(GL_TEXTURE0);
}

// createYUVGLShader() defines the vertex/fragment
// shader that does the actual work of converting
// YUV to RGB. The resulting program is stored in |program_out|.
static void createYUVGLShader(GLuint* program_out,
                              GLint* ywidthcutoffloc_out,
                              GLint* cwidthcutoffloc_out,
                              GLint* ysamplerloc_out,
                              GLint* usamplerloc_out,
                              GLint* vsamplerloc_out,
                              GLint* incoordloc_out,
                              GLint* posloc_out) {
    assert(program_out);

    static const char kVShader[] = R"(
precision highp float;
attribute mediump vec4 position;
attribute highp vec2 inCoord;
varying highp vec2 outCoord;
void main(void) {
  gl_Position = position;
  outCoord = inCoord;
}
    )";
    const GLchar* const kVShaders =
        static_cast<const GLchar*>(kVShader);

    // Based on:
    // http://stackoverflow.com/questions/11093061/yv12-to-rgb-using-glsl-in-ios-result-image-attached
    // + account for 16-pixel alignment using |yWidthCutoff| / |cWidthCutoff|
    // + use conversion matrix in
    // frameworks/av/media/libstagefright/colorconversion/ColorConverter.cpp (YUV420p)
    // + more precision from
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
    static const char kFShader[] = R"(
precision highp float;
varying highp vec2 outCoord;
uniform highp float yWidthCutoff;
uniform highp float cWidthCutoff;
uniform sampler2D ysampler;
uniform sampler2D usampler;
uniform sampler2D vsampler;
void main(void) {
    highp vec2 cutoffCoordsY;
    highp vec2 cutoffCoordsC;
    highp vec3 yuv;
    highp vec3 rgb;
    cutoffCoordsY.x = outCoord.x * yWidthCutoff;
    cutoffCoordsY.y = outCoord.y;
    cutoffCoordsC.x = outCoord.x * cWidthCutoff;
    cutoffCoordsC.y = outCoord.y;
    yuv[0] = texture2D(ysampler, cutoffCoordsY).r - 0.0625;
    yuv[1] = texture2D(usampler, cutoffCoordsC).r - 0.5;
    yuv[2] = texture2D(vsampler, cutoffCoordsC).r - 0.5;
    highp float yscale = 1.1643835616438356;
    rgb = mat3(yscale,                           yscale,            yscale,
               0,                  -0.39176229009491365, 2.017232142857143,
               1.5960267857142856, -0.8129676472377708,                  0) * yuv;
    gl_FragColor = vec4(rgb, 1);
}
    )";

    const GLchar* const kFShaders =
        static_cast<const GLchar*>(kFShader);

    GLuint vshader = s_gles2.glCreateShader(GL_VERTEX_SHADER);
    GLuint fshader = s_gles2.glCreateShader(GL_FRAGMENT_SHADER);

    const GLint vtextLen = strlen(kVShader);
    const GLint ftextLen = strlen(kFShader);
    s_gles2.glShaderSource(vshader, 1, &kVShaders, &vtextLen);
    s_gles2.glShaderSource(fshader, 1, &kFShaders, &ftextLen);
    s_gles2.glCompileShader(vshader);
    s_gles2.glCompileShader(fshader);

    *program_out = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(*program_out, vshader);
    s_gles2.glAttachShader(*program_out, fshader);
    s_gles2.glLinkProgram(*program_out);

    *ywidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "yWidthCutoff");
    *cwidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "cWidthCutoff");
    *ysamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "ysampler");
    *usamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "usampler");
    *vsamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "vsampler");
    *posloc_out = s_gles2.glGetAttribLocation(*program_out, "position");
    *incoordloc_out = s_gles2.glGetAttribLocation(*program_out, "inCoord");

    s_gles2.glDeleteShader(vshader);
    s_gles2.glDeleteShader(fshader);
}

// When converting YUV to RGB with shaders,
// we are using the OpenGL graphics pipeline to do compute,
// so we need to express the place to store the result
// with triangles and draw calls.
// createYUVGLFullscreenQuad() defines a fullscreen quad
// with position and texture coordinates.
// The quad will be textured with the resulting RGB colors,
// and we will read back the pixels from the framebuffer
// to retrieve our RGB result.
static void createYUVGLFullscreenQuad(GLuint* vbuf_out,
                                      GLuint* ibuf_out,
                                      int picture_width,
                                      int aligned_width) {
    assert(vbuf_out);
    assert(ibuf_out);

    s_gles2.glGenBuffers(1, vbuf_out);
    s_gles2.glGenBuffers(1, ibuf_out);

    static const float kVertices[] = {
        +1, -1, +0, +1, +0,
        +1, +1, +0, +1, +1,
        -1, +1, +0, +0, +1,
        -1, -1, +0, +0, +0,
    };

    static const GLubyte kIndices[] = { 0, 1, 2, 2, 3, 0 };

    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, *vbuf_out);
    s_gles2.glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices,
                         GL_STATIC_DRAW);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibuf_out);
    s_gles2.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices,
                         GL_STATIC_DRAW);
}

// doYUVConversionDraw() does the actual work of setting up
// and submitting draw commands to the GPU.
// It uses the textures, shaders, and fullscreen quad defined above
// and executes the pipeline on them.
// Note, however, that it is up to the caller to dig out
// the result of the draw.
static void doYUVConversionDraw(GLuint program,
                                GLint yWidthCutoffLoc,
                                GLint cWidthCutoffLoc,
                                GLint ySamplerLoc,
                                GLint uSamplerLoc,
                                GLint vSamplerLoc,
                                GLint inCoordLoc,
                                GLint posLoc,
                                GLuint vbuf, GLuint ibuf,
                                int width, int ywidth,
                                int halfwidth, int cwidth,
                                float yWidthCutoff,
                                float cWidthCutoff) {

    const GLsizei kVertexAttribStride = 5 * sizeof(GL_FLOAT);
    const GLvoid* kVertexAttribPosOffset = (GLvoid*)0;
    const GLvoid* kVertexAttribCoordOffset = (GLvoid*)(3 * sizeof(GL_FLOAT));

    s_gles2.glUseProgram(program);

    s_gles2.glUniform1f(yWidthCutoffLoc, yWidthCutoff);
    s_gles2.glUniform1f(cWidthCutoffLoc, cWidthCutoff);

    s_gles2.glUniform1i(ySamplerLoc, 0);
    s_gles2.glUniform1i(uSamplerLoc, 1);
    s_gles2.glUniform1i(vSamplerLoc, 2);

    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    s_gles2.glEnableVertexAttribArray(posLoc);
    s_gles2.glEnableVertexAttribArray(inCoordLoc);

    s_gles2.glVertexAttribPointer(posLoc, 3, GL_FLOAT, false,
                                  kVertexAttribStride,
                                  kVertexAttribPosOffset);
    s_gles2.glVertexAttribPointer(inCoordLoc, 2, GL_FLOAT, false,
                                  kVertexAttribStride,
                                  kVertexAttribCoordOffset);

    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
    s_gles2.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    s_gles2.glDisableVertexAttribArray(posLoc);
    s_gles2.glDisableVertexAttribArray(inCoordLoc);
}

// initialize(): allocate GPU memory for YUV components,
// and create shaders and vertex data.
YUVConverter::YUVConverter(int width, int height, FrameworkFormat format) : mFormat(format){

    uint32_t totalSize, yStride, cStride, cHeight;
    totalSize = 0;
    getYUVSizes(width, height, mFormat, &totalSize, &yStride, &cStride, &cHeight);

    uint32_t yoff, uoff, voff,
             ywidth, cwidth, cheight;
    getYUVOffsets(width, height, mFormat,
                  &yoff, &uoff, &voff,
                  &ywidth, &cwidth);
    cheight = height / 2;

    createYUVGLTex(GL_TEXTURE0, ywidth, height, &mYtex);
    createYUVGLTex(GL_TEXTURE1, cwidth, cheight, &mUtex);
    createYUVGLTex(GL_TEXTURE2, cwidth, cheight, &mVtex);

    createYUVGLShader(&mProgram,
                      &mYWidthCutoffLoc,
                      &mCWidthCutoffLoc,
                      &mYSamplerLoc,
                      &mUSamplerLoc,
                      &mVSamplerLoc,
                      &mInCoordLoc,
                      &mPosLoc);

    createYUVGLFullscreenQuad(&mVbuf, &mIbuf, width, ywidth);
}

void YUVConverter::saveGLState() {
    s_gles2.glGetFloatv(GL_VIEWPORT, mCurrViewport);
    s_gles2.glGetIntegerv(GL_ACTIVE_TEXTURE, &mCurrTexUnit);
    s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, &mCurrTexBind);
    s_gles2.glGetIntegerv(GL_CURRENT_PROGRAM, &mCurrProgram);
    s_gles2.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mCurrVbo);
    s_gles2.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mCurrIbo);
}

void YUVConverter::restoreGLState() {
    s_gles2.glViewport(mCurrViewport[0], mCurrViewport[1],
                       mCurrViewport[2], mCurrViewport[3]);
    s_gles2.glActiveTexture(mCurrTexUnit);
    s_gles2.glUseProgram(mCurrProgram);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mCurrVbo);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mCurrIbo);
}

// drawConvert: per-frame updates.
// Update YUV textures, then draw the fullscreen
// quad set up above, which results in a framebuffer
// with the RGB colors.
void YUVConverter::drawConvert(int x, int y,
                               int width, int height,
                               char* pixels) {
    saveGLState();

    s_gles2.glViewport(x, y, width, height);

    uint32_t yoff, uoff, voff,
             ywidth, cwidth, cheight;
    getYUVOffsets(width, height, mFormat,
                  &yoff, &uoff, &voff,
                  &ywidth, &cwidth);
    cheight = height / 2;

    subUpdateYUVGLTex(GL_TEXTURE0, mYtex,
                      x, y, ywidth, height,
                      pixels + yoff);
    subUpdateYUVGLTex(GL_TEXTURE1, mUtex,
                      x, y, cwidth, cheight,
                      pixels + uoff);
    subUpdateYUVGLTex(GL_TEXTURE2, mVtex,
                      x, y, cwidth, cheight,
                      pixels + voff);


    updateCutoffs(width, ywidth, width / 2, cwidth);

    doYUVConversionDraw(mProgram,
                        mYWidthCutoffLoc,
                        mCWidthCutoffLoc,
                        mYSamplerLoc,
                        mUSamplerLoc,
                        mVSamplerLoc,
                        mInCoordLoc,
                        mPosLoc,
                        mVbuf, mIbuf,
                        width, ywidth,
                        width / 2, cwidth,
                        mYWidthCutoff,
                        mCWidthCutoff);
    restoreGLState();
}

void YUVConverter::updateCutoffs(float width, float ywidth,
                                 float halfwidth, float cwidth) {
    switch (mFormat) {
    case FRAMEWORK_FORMAT_YV12:
        mYWidthCutoff = ((float)width) / ((float)ywidth);
        mCWidthCutoff = ((float)halfwidth) / ((float)cwidth);
        break;
    case FRAMEWORK_FORMAT_YUV_420_888:
        mYWidthCutoff = 1.0f;
        mCWidthCutoff = 1.0f;
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    } 
}

YUVConverter::~YUVConverter() {
    if (mIbuf) s_gles2.glDeleteBuffers(1, &mIbuf);
    if (mVbuf) s_gles2.glDeleteBuffers(1, &mVbuf);
    if (mProgram) s_gles2.glDeleteProgram(mProgram);
    if (mYtex) s_gles2.glDeleteTextures(1, &mYtex);
    if (mUtex) s_gles2.glDeleteTextures(1, &mUtex);
    if (mVtex) s_gles2.glDeleteTextures(1, &mVtex);
}
