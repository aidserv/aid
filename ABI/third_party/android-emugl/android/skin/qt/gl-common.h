// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "GLES2/gl2.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <cstdio>

// Helper macro for checking error status and cleaning up.
#define CHECK_GL_ERROR(error_msg) \
    if (mGLES2->glGetError() != GL_NO_ERROR) {  \
        fprintf(stderr, error_msg); \
        return; \
    }

#define CHECK_GL_ERROR_RETURN(error_msg, retval) \
    if (mGLES2->glGetError() != GL_NO_ERROR) {  \
        fprintf(stderr, error_msg); \
        return (retval); \
    }

GLuint createShader(const GLESv2Dispatch* gles2,
                    GLint shader_type,
                    const char* shader_code);
