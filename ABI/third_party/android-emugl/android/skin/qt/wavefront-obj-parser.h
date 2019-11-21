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

#include <QTextStream>

// This function can be used to parse certain subset of the Wavefront OBJ format.
// See here for details: https://en.wikipedia.org/wiki/Wavefront_.obj_file
// It has the following limitations:
// 1. Materials are not supported.
// 2. Position coordinates must have only x, y and z (w defaults to 1)
// 3. Texture coordinates must have only u and v (w defaults to 0)
// 4. Parameter space vertices are not supported.
// 5. Only triangular faces are allowed.
// 6. A face must be specified as "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3", where v* are
//    the indices of position coordinates, vt* are the indices of texture
//    coordinates and vn* are indices of normals.
// |stream| is the input stream from which the OBJ data is read
// |vtx_buf| is an output parameter, it will contain the vertex data in the
//           following format: <X> <Y> <Z> <X'> <Y'> <Z'> <U> <V>.
//           Where XYZ is position, X'Y'Z' is the normal and UV are texture
//           coordinates. It is suitable for creating a buffer object of
//           vertex data for a shader.
// |idx_buf| is an output parameter, it will contain indices for vertices
//           from |vtxbuf|. It is suitable for creating an element array buffer
//           for a shader.
// This function returns true if loading the mesh was successful, and false
// otherwise.
bool parseWavefrontOBJ(QTextStream& stream,
                       std::vector<float>& vtxbuf,
                       std::vector<GLuint>& idxbuf);