// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-common.h"

#include <cstring>

#include <QtGlobal>

GLuint createShader(const GLESv2Dispatch* gles2,
                    GLint shader_type,
                    const char* shader_code) {
    GLuint shader = gles2->glCreateShader(shader_type);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shader_code);
    const GLint text_len = strlen(shader_code);
    gles2->glShaderSource(shader, 1, &text, &text_len);
    gles2->glCompileShader(shader);
    GLint success;
    gles2->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        char msgs[256];
        gles2->glGetShaderInfoLog(shader, sizeof(msgs), nullptr, msgs);
        qWarning("Error compiling %s shader: %s",
                 shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                 msgs);
    }
    return success == GL_TRUE ? shader : 0;
}