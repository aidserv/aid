// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "TextureDraw.h"

#include "DispatchTables.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#define ERR(...)  fprintf(stderr, __VA_ARGS__)

namespace {

// Helper function to create a new shader.
// |shaderType| is the shader type (e.g. GL_VERTEX_SHADER).
// |shaderText| is a 0-terminated C string for the shader source to use.
// On success, return the handle of the new compiled shader, or 0 on failure.
GLuint createShader(GLint shaderType, const char* shaderText) {
    // Create new shader handle and attach source.
    GLuint shader = s_gles2.glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shaderText);
    const GLint textLen = ::strlen(shaderText);
    s_gles2.glShaderSource(shader, 1, &text, &textLen);

    // Compiler the shader.
    GLint success;
    s_gles2.glCompileShader(shader);
    s_gles2.glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        s_gles2.glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// No scaling / projection since we want to fill the whole viewport with
// the texture, hence a trivial vertex shader that only supports translation.
// Note: we used to have a proper free-angle rotation support in this shader,
//  but looks like SwiftShader doesn't support either complicated calculations
//  for gl_Position/varyings or just doesn't like trigonometric functions in
//  shader; anyway the new code has hardcoded texture coordinate mapping for
//  different rotation angles and works in both native OpenGL and SwiftShader.
const char kVertexShaderSource[] =
    "attribute vec4 position;\n"
    "attribute vec2 inCoord;\n"
    "varying vec2 outCoord;\n"
    "uniform vec2 translation;\n"

    "void main(void) {\n"
    "  gl_Position.xy = position.xy - translation.xy;\n"
    "  gl_Position.zw = position.zw;\n"
    "  outCoord = inCoord;\n"
    "}\n";

// Similarly, just interpolate texture coordinates.
const char kFragmentShaderSource[] =
    "varying lowp vec2 outCoord;\n"
    "uniform sampler2D tex;\n"

    "void main(void) {\n"
    "  gl_FragColor = texture2D(tex, outCoord);\n"
    "}\n";

// Hard-coded arrays of vertex information.
struct Vertex {
    float pos[3];
    float coord[2];
};

const Vertex kVertices[] = {
    {{ +1, -1, +0 }, { +1, +1 }},
    {{ +1, +1, +0 }, { +1, +0 }},
    {{ -1, +1, +0 }, { +0, +0 }},
    {{ -1, -1, +0 }, { +0, +1 }},

    {{ +1, -1, +0 }, { +1, +0 }},
    {{ +1, +1, +0 }, { +0, +0 }},
    {{ -1, +1, +0 }, { +0, +1 }},
    {{ -1, -1, +0 }, { +1, +1 }},

    {{ +1, -1, +0 }, { +0, +0 }},
    {{ +1, +1, +0 }, { +0, +1 }},
    {{ -1, +1, +0 }, { +1, +1 }},
    {{ -1, -1, +0 }, { +1, +0 }},

    {{ +1, -1, +0 }, { +0, +1 }},
    {{ +1, +1, +0 }, { +1, +1 }},
    {{ -1, +1, +0 }, { +1, +0 }},
    {{ -1, -1, +0 }, { +0, +0 }},
};

// Vertex indices for predefined rotation angles.
const GLubyte kIndices[] = {
    0, 1, 2, 2, 3, 0,      // 0
    4, 5, 6, 6, 7, 4,      // 90
    8, 9, 10, 10, 11, 8,   // 180
    12, 13, 14, 14, 15, 12 // 270
};

const GLint kIndicesLen = sizeof(kIndices) / sizeof(kIndices[0]);
const GLint kIndicesPerDraw = 6;

}  // namespace

TextureDraw::TextureDraw() :
        mVertexShader(0),
        mFragmentShader(0),
        mProgram(0),
        mPositionSlot(-1),
        mInCoordSlot(-1),
        mTextureSlot(-1),
        mTranslationSlot(-1) {
    // Create shaders and program.
    mVertexShader = createShader(GL_VERTEX_SHADER, kVertexShaderSource);
    mFragmentShader = createShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);

    mProgram = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(mProgram, mVertexShader);
    s_gles2.glAttachShader(mProgram, mFragmentShader);

    GLint success;
    s_gles2.glLinkProgram(mProgram);
    s_gles2.glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar messages[256];
        s_gles2.glGetProgramInfoLog(
                mProgram, sizeof(messages), 0, &messages[0]);
        ERR("%s: Could not create/link program: %s\n", __FUNCTION__, messages);
        s_gles2.glDeleteProgram(mProgram);
        mProgram = 0;
        return;
    }

    s_gles2.glUseProgram(mProgram);

    // Retrieve attribute/uniform locations.
    mPositionSlot = s_gles2.glGetAttribLocation(mProgram, "position");
    s_gles2.glEnableVertexAttribArray(mPositionSlot);

    mInCoordSlot = s_gles2.glGetAttribLocation(mProgram, "inCoord");
    s_gles2.glEnableVertexAttribArray(mInCoordSlot);

    mTranslationSlot = s_gles2.glGetUniformLocation(mProgram, "translation");
    mTextureSlot = s_gles2.glGetUniformLocation(mProgram, "texture");

#if 0
    printf("SLOTS position=%d inCoord=%d texture=%d translation=%d\n",
          mPositionSlot, mInCoordSlot, mTextureSlot, mTranslationSlot);
#endif

    // Create vertex and index buffers.
    s_gles2.glGenBuffers(1, &mVertexBuffer);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glBufferData(
            GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

    s_gles2.glGenBuffers(1, &mIndexBuffer);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    s_gles2.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(kIndices),
                         kIndices,
                         GL_STATIC_DRAW);
}

bool TextureDraw::draw(GLuint texture, float rotation, float dx, float dy) {
    if (!mProgram) {
        ERR("%s: no program\n", __FUNCTION__);
        return false;
    }

    // TODO(digit): Save previous program state.

    s_gles2.glUseProgram(mProgram);

#ifndef NDEBUG
    GLenum err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not use program error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // Setup the |position| attribute values.
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not bind GL_ARRAY_BUFFER error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    s_gles2.glEnableVertexAttribArray(mPositionSlot);
    s_gles2.glVertexAttribPointer(mPositionSlot,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  0);

#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could glVertexAttribPointer with mPositionSlot error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // Setup the |inCoord| attribute values.
    s_gles2.glEnableVertexAttribArray(mInCoordSlot);
    s_gles2.glVertexAttribPointer(mInCoordSlot,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<GLvoid*>(
                                        static_cast<uintptr_t>(
                                                sizeof(float) * 3)));

    // setup the |texture| uniform value.
    s_gles2.glActiveTexture(GL_TEXTURE0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, texture);
    s_gles2.glUniform1i(mTextureSlot, 0);

    // setup the |translation| uniform value.
    s_gles2.glUniform2f(mTranslationSlot, dx, dy);

#ifndef NDEBUG
    // Validate program, just to be sure.
    s_gles2.glValidateProgram(mProgram);
    GLint validState = 0;
    s_gles2.glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &validState);
    if (validState == GL_FALSE) {
        GLchar messages[256];
        s_gles2.glGetProgramInfoLog(
                mProgram, sizeof(messages), 0, &messages[0]);
        ERR("%s: Could not run program: %s\n", __FUNCTION__, messages);
        return false;
    }
#endif

    // Do the rendering.
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glBindBuffer(GL_ELEMENT_ARRAY_BUFFER) error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // We may only get 0, 90, 180, 270 in |rotation| so far.
    const int intRotation = ((int)rotation)/90;
    assert(intRotation >= 0 && intRotation <= 3);
    const intptr_t indexShift = intRotation * kIndicesPerDraw;

    s_gles2.glDrawElements(GL_TRIANGLES, kIndicesPerDraw, GL_UNSIGNED_BYTE,
                           (const GLvoid*)indexShift);
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glDrawElements() error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // TODO(digit): Restore previous program state.

    return true;
}

TextureDraw::~TextureDraw() {
    s_gles2.glDeleteBuffers(1, &mIndexBuffer);
    s_gles2.glDeleteBuffers(1, &mVertexBuffer);

    if (mFragmentShader) {
        s_gles2.glDeleteShader(mFragmentShader);
    }
    if (mVertexShader) {
        s_gles2.glDeleteShader(mVertexShader);
    }
}
