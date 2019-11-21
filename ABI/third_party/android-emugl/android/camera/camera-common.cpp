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

#include "android/camera/camera-common.h"

static void reassign(char** pstr, const char* value) {
    if (*pstr) {
        ::free(*pstr);
        *pstr = nullptr;
    }
    if (value) {
        *pstr = ::strdup(value);
    }
}

void camera_info_done(CameraInfo* ci) {
    if (ci != nullptr) {
        reassign(&ci->display_name, nullptr);
        reassign(&ci->device_name, nullptr);
        ci->inp_channel = 0;
        ci->pixel_format = 0;
        reassign(&ci->direction, nullptr);
        if (ci->frame_sizes) {
            ::free(ci->frame_sizes);
            ci->frame_sizes = nullptr;
        }
        ci->frame_sizes_num = 0;
        ci->in_use = 0;
    }
}

void camera_info_copy(CameraInfo* ci, const CameraInfo* from) {
    if (!from) {
        camera_info_done(ci);
    } else if ((const CameraInfo*)ci != from) {
        reassign(&ci->display_name, from->display_name);
        reassign(&ci->device_name, from->device_name);
        ci->inp_channel = from->inp_channel;
        ci->pixel_format = from->pixel_format;
        reassign(&ci->direction, from->direction);
        if (ci->frame_sizes) {
            ::free(ci->frame_sizes);
            ci->frame_sizes = nullptr;
        }
        ci->frame_sizes_num = from->frame_sizes_num;
        if (ci->frame_sizes_num > 0) {
            ci->frame_sizes = static_cast<CameraFrameDim*>(
                    ::malloc(sizeof(ci->frame_sizes[0]) * ci->frame_sizes_num));
            ::memcpy(ci->frame_sizes, from->frame_sizes,
                     ci->frame_sizes_num * sizeof(ci->frame_sizes[0]));
        }
        ci->in_use = from->in_use;
    }
}
