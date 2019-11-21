// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/StringView.h"
#include "android/featurecontrol/Features.h"

#include <vector>

namespace android {

namespace base { class IniFile; }

namespace featurecontrol {

// FeatureControlImpl is the implementation of feature control.
// Please access it through the API in android/featurecontrol/FeatureControl.h

// Features are controlled through ini files.
// There are two different types of features: host-only features that
// only need to be enabled in the emulator to take effect, and guest
// features that needs to simultaneously enabled in the host and the
// guest. Host-only features are defined in FeatureControlDefHost.h.
// Guest features are defined in FeatureControlDefGuest.

// User can override any feature options by providing a user ini.

// There are up to 3 ini files that would be loaded during
// initialization: defaultIniHost, defaultIniGuest and
// [optional] userIniHost. Each of them do the following:
//
// defaultIniHost specifies both host and guest features, the
// emulator ships with this file.

// defaultIniGuest specifies guest features only, the system image
// ships with this file. Guest features will only be default to ON
// when both defaultIniHost and defaultIniGuest set it on.
// If defaultIniGuest does not exist, all guest features will be
// default to OFF. If it does not specify a guest feature defined in
// defaultIniHost, it will also be default to OFF. It should not
// contain any host-only features. If it does, there is no effect.

// userIniHost specifies user override values. It can be manually
// created under the user's .android folder. It can override any
// features. This file is optional, and if a feature is not specified
// in userIniHost, it will use the default value from defaultIniHost
// and defaultIniGuest.

// There is a plan for a forth ini file which specifies per-avd
// override values. It is disabled due to lacking of use cases.

// The ini file paths are passed through calling
// FeatureControlImpl::init(). Calling FeatureControlImpl::get()
// will trigger FeatureControlImpl::init() with default paths. You
// can reinitialize it with different file paths by calling
// FeatureControlImpl::init() again.

// Please read FeatureControl_unittest.cpp for examples.

class FeatureControlImpl {
public:
    bool isEnabled(Feature feature) const;
    bool isOverridden(Feature feature) const;

    bool isGuestFeature(Feature feature) const;
    bool isEnabledByGuest(Feature feature) const;

    void setEnabledOverride(Feature feature, bool isEnabled);
    void resetEnabledToDefault(Feature feature);
    void setIfNotOverriden(Feature feature, bool isEnabled);
    void setIfNotOverridenOrGuestDisabled(Feature feature, bool isEnabled);

    static Feature fromString(base::StringView str);
    static base::StringView toString(Feature feature);

    std::vector<Feature> getEnabledNonOverride() const;
    std::vector<Feature> getEnabledOverride() const;
    std::vector<Feature> getDisabledOverride() const;
    std::vector<Feature> getEnabled() const;

    void init(android::base::StringView defaultIniHostPath,
              android::base::StringView defaultIniGuestPath,
              android::base::StringView userIniHostPath,
              android::base::StringView userIniGuestPath);
    static FeatureControlImpl& get();
    FeatureControlImpl();
private:
    struct FeatureOption {
        Feature name = static_cast<Feature>(0);
        bool defaultVal = false;
        bool currentVal = false;
        bool isOverridden = false;
    };
    FeatureOption mFeatures[Feature_n_items] = {};
    FeatureOption mGuestTriedEnabledFeatures[Feature_n_items] = {};
    void initEnabledDefault(Feature feature, bool isEnabled);
    void setGuestTriedEnable(Feature feature);

    void initHostFeatureAndParseDefault(
        android::base::IniFile& defaultIniHost,
        Feature featureName,
        const char* featureNameStr);
    void initGuestFeatureAndParseDefault(
        android::base::IniFile& defaultIniHost,
        android::base::IniFile& defaultIniGuest,
        Feature featureName,
        const char* featureNameStr);
    void loadUserOverrideFeature(
        android::base::IniFile& defaultIni,
        Feature featureName,
        const char* featureNameStr);
};

}  // namespace featurecontrol
}  // namespace android
