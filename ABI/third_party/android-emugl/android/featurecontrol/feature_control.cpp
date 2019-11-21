// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "feature_control.h"
#include "FeatureControl.h"

bool feature_is_enabled(Feature feature) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::isEnabled(value);
}

void feature_set_enabled_override(Feature feature, bool isEnabled) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::setEnabledOverride(value, isEnabled);
}

void feature_reset_enabled_to_default(Feature feature) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::resetEnabledToDefault(value);
}

void feature_update_from_server() {
    android::featurecontrol::applyCachedServerFeaturePatterns();
    android::featurecontrol::asyncUpdateServerFeaturePatterns();
}
