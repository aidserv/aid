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

#include "android/camera/camera-list.h"

#include "android/camera/camera-capture.h"

#include <stdio.h>

/* Maximum number of supported emulated cameras. */
#define MAX_CAMERA 8

void android_camera_list_webcams(void) {
    CameraInfo ci[MAX_CAMERA] = {};

    /* Enumerate camera devices connected to the host. */
    int connected_cnt = camera_enumerate_devices(ci, MAX_CAMERA);
    if (connected_cnt <= 0) {
        return;
    }

    printf("List of web cameras connected to the computer:\n");
    for (int i = 0; i < connected_cnt; i++) {
        printf(" Camera '%s' is connected to device '%s' on channel %d "
               "using pixel format '%.4s'\n",
               ci[i].display_name, ci[i].device_name, ci[i].inp_channel,
               (const char*)&ci[i].pixel_format);

        camera_info_done(&ci[i]);
    }
    printf("\n");
}
