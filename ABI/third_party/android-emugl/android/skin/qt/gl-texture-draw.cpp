// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-texture-draw.h"

#include "android/skin/qt/gl-common.h"
#include "GLES2/gl2.h"

#include <cstring>

// Vertex shader for anti-aliasing - doesn't do anything special.
static const char VertexShaderSource[] = R"(
    attribute vec2 position;
    void main(void) {
        gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    })";

// Fragment shader
static const char FragmentShaderSource[] = R"(
    precision mediump float;
    uniform sampler2D texture;
    uniform vec2 resolution;
    void main(void) {
        vec2 pixel_coord = gl_FragCoord.xy;
        vec2 inverse_res = 1.0 / resolution;
        gl_FragColor = texture2D(texture, inverse_res * pixel_coord);
    }
)";

TextureDraw::TextureDraw(const GLESv2Dispatch* gl_dispatch) :
        mGLES2(gl_dispatch),
        mProgram(0),
        mVertexBuffer(0) {
    GLuint vertex_shader =
            createShader(mGLES2, GL_VERTEX_SHADER, VertexShaderSource);
    CHECK_GL_ERROR("Failed to create vertex shader");
    GLuint fragment_shader =
            createShader(mGLES2, GL_FRAGMENT_SHADER, FragmentShaderSource);
    CHECK_GL_ERROR("Failed to create fragment shader");

    mProgram = mGLES2->glCreateProgram();
    CHECK_GL_ERROR("Failed to create program object");
    mGLES2->glAttachShader(mProgram, vertex_shader);
    mGLES2->glAttachShader(mProgram, fragment_shader);
    mGLES2->glLinkProgram(mProgram);

    // Shader objects no longer needed.
    mGLES2->glDeleteShader(vertex_shader);
    mGLES2->glDeleteShader(fragment_shader);

    // Check for errors.
    GLint success;
    mGLES2->glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infolog[256];
        mGLES2->glGetProgramInfoLog(mProgram, sizeof(infolog), 0, infolog);
        fprintf(stderr, "Could not create/link program: %s\n", infolog);
        return;
    }

    // Get all the attributes and uniforms.
    mPositionAttribLocation =
            mGLES2->glGetAttribLocation(mProgram, "position");
    mInputUniformLocation =
            mGLES2->glGetUniformLocation(mProgram, "texture");
    mResolutionUniformLocation =
            mGLES2->glGetUniformLocation(mProgram, "resolution");
    CHECK_GL_ERROR("Failed to get attributes & uniforms for shader");

    // Create vertex and index buffers.
    mGLES2->glGenBuffers(1, &mVertexBuffer);
    CHECK_GL_ERROR("Failed to create vertex buffer for anti-aliasing shader");

    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

    // This is a right triangle that is larger than the screen.
    // The screen fits into the triangle so that its bottom left
    // corner coincides with the vertex at the 90-degree angle
    // and the top-right corner lies at the center of the hypothenuse.
    // (0, 0) is at the center of the screen rectangle.
    static const float vertex_data[] = {
        -1.0, -1.0,
        3.0, -1.0,
        -1.0, 3.0,
    };
    mGLES2->glBufferData(GL_ARRAY_BUFFER,
                         sizeof(vertex_data),
                         vertex_data,
                         GL_STATIC_DRAW);
    CHECK_GL_ERROR("Failed to populate vertex buffer");
}

TextureDraw::~TextureDraw() {
    if (mProgram) {
        mGLES2->glUseProgram(0);
        mGLES2->glDeleteProgram(mProgram);
        mProgram = 0;
    }

    if (mVertexBuffer) {
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
        mGLES2->glDeleteBuffers(1, &mVertexBuffer);
        mVertexBuffer = 0;
    }
}

void TextureDraw::draw(GLuint input_texture, int width, int height) {
    // Draw texture on-screen, applying the anti-aliasing shader.
    mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGLES2->glUseProgram(mProgram);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGLES2->glEnableVertexAttribArray(mPositionAttribLocation);
    mGLES2->glVertexAttribPointer(mPositionAttribLocation,
                                  2, // components per attrib
                                  GL_FLOAT,
                                  GL_FALSE,
                                  0, // stride
                                  0); // offset
    mGLES2->glActiveTexture(GL_TEXTURE0);
    mGLES2->glBindTexture(GL_TEXTURE_2D, input_texture);
    mGLES2->glUniform1i(mInputUniformLocation, 0);
    mGLES2->glUniform2f(mResolutionUniformLocation, width, height);
    mGLES2->glDrawArrays(GL_TRIANGLES, 0, 3);
}
