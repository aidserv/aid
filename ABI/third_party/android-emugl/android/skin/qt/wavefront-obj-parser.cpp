// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/wavefront-obj-parser.h"

#include <QString>

#include <tuple>
#include <unordered_map>

namespace std {
template <>
struct hash<std::tuple<int, int, int>> {
    size_t operator()(const tuple<int, int, int>& t) const {
        return hash_combine(std::hash<int>()(get<0>(t)),
                            hash_combine(std::hash<int>()(get<1>(t)),
                                         std::hash<int>()(get<2>(t))));
    }

private:
    static int hash_combine(int seed, int v) {
        return std::hash<int>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
};
}

bool parseWavefrontOBJ(QTextStream& stream,
                       std::vector<float>& vtx_buf,
                       std::vector<GLuint>& idx_buf) {
    std::vector<float> pos, tex, norm;
    std::unordered_map<std::tuple<int, int, int>, size_t> idx_table;
    QString str;
    int vertices = 0;

    vtx_buf.clear();
    idx_buf.clear();

    for (stream >> str; !stream.atEnd(); stream >> str) {
        if (str.length() <= 0) {
            continue;
        }
        if (str[0] == '#') {
            // Comment, read till the end of line.
            stream.readLine();
        } else if (str == "v" || str == "vn") {
            // Vertex position or normal.
            // Both are specified with 3 floating point numbers
            // separated by whitespace.
            float xyz[3];
            stream >> xyz[0] >> xyz[1] >> xyz[2];
            if (stream.status() != QTextStream::Ok) {
                qWarning("OBJ parser: invalid position or normal");
                return false;
            }
            auto& container = (str == "v" ? pos : norm);
            container.insert(container.end(), xyz, xyz + 3);
        } else if (str == "vt") {
            // UV coords.
            // Specified with 2 floating point numbers separated by whitespace.
            float uv[2];
            stream >> uv[0] >> uv[1];
            if (stream.status() != QTextStream::Ok) {
                qWarning("OBJ parser: invalid UV");
                return false;
            }
            tex.insert(tex.end(), uv, uv + 2);
        } else if (str == "f") {
            // Face.
            // A face is specified with 3 vertices.
            // A vertex is specified like "vp/vt/vn" (no spaces)
            // where vp, vt and vn are indices into the position,
            // UV and normal arrays.
            QString v;
            size_t vp, vt, vn;
            for (size_t i = 0; i < 3; i++) {
                stream >> v;
                QStringList components = v.split('/', QString::SkipEmptyParts);
                if (components.size() != 3) {
                    qWarning("OBJ parser: invalid face specification");
                    return false;
                }
                bool pos_result, tex_result, nrm_result;
                // Note that indices in OBJ are 1-based.
                vp = components[0].toInt(&pos_result) - 1;
                vt = components[1].toInt(&tex_result) - 1;
                vn = components[2].toInt(&nrm_result) - 1;
                if (!(pos_result && tex_result && nrm_result) ||
                    vp * 3 >= pos.size() ||
                    vt * 2 >= tex.size() ||
                    vn * 3 >= norm.size()) {
                    qWarning("OBJ parser: invalid face specification");
                    return false;
                }
                auto vertex_idx = std::make_tuple(vp, vt, vn);
                auto vertex_it = idx_table.find(vertex_idx);
                if (vertex_it == idx_table.end()) {
                    // First time encountering this vertex, write its attributes
                    // into vertex buffer.
                    size_t element_array_idx = vertices++;
                    idx_buf.push_back(element_array_idx);
                    vtx_buf.insert(vtx_buf.end(), &pos[vp * 3], &pos[vp * 3 + 3]);
                    vtx_buf.insert(vtx_buf.end(), &norm[vn * 3], &norm[vn * 3 + 3]);
                    vtx_buf.insert(vtx_buf.end(), &tex[vt * 2], &tex[vt * 2 + 2]);
                    idx_table[vertex_idx] = element_array_idx;
                } else {
                    // We've already encountered this vertex, write its index into the
                    // index buffer.
                    idx_buf.push_back(vertex_it->second);
                }
            }
        } else {
            // Something's wrong, bail out.
            qWarning("OBJ parser: invalid input [%s]", str.toStdString().c_str());
            return false;
        }
    }

    return true;
}