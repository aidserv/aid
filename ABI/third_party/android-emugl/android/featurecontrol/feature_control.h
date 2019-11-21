/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

#include "features_c.h"

// Get the access rules given by |name| if they exist, otherwise returns NULL
bool feature_is_enabled(Feature feature);
void feature_set_enabled_override(Feature feature, bool isEnabled);
void feature_reset_enabled_to_default(Feature feature);

// Runs applyCachedServerFeaturePatterns then
// asyncUpdateServerFeaturePatterns. See FeatureControl.h
// for more info. To be called only once on startup.
void feature_update_from_server();

ANDROID_END_HEADER
