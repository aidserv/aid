// Copyright (C) 2016 The Android Open Source Project
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

#include "emugl/common/misc.h"

static int s_apiLevel = -1;
static bool s_isPhone = false;

static int s_glesMajorVersion = 2;
static int s_glesMinorVersion = 0;

static SelectedRenderer s_renderer =
    SELECTED_RENDERER_HOST;

void emugl::setAvdInfo(bool phone, int apiLevel) {
    s_isPhone = phone;
    s_apiLevel = apiLevel;
}

void emugl::getAvdInfo(bool* phone, int* apiLevel) {
    if (phone) *phone = s_isPhone;
    if (apiLevel) *apiLevel = s_apiLevel;
}

void emugl::setGlesVersion(int maj, int min) {
    s_glesMajorVersion = maj;
    s_glesMinorVersion = min;
}

void emugl::getGlesVersion(int* maj, int* min) {
    if (maj) *maj = s_glesMajorVersion;
    if (min) *min = s_glesMinorVersion;
}

void emugl::setRenderer(SelectedRenderer renderer) {
    s_renderer = renderer;
}

SelectedRenderer emugl::getRenderer() {
    return s_renderer;
}
