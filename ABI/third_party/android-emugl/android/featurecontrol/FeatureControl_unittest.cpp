// Copyright 2015 The Android Open Source Project
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

#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/FeatureControlImpl.h"

#include "android/base/StringView.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

namespace android {
namespace featurecontrol {

namespace {

class FeatureControlTest : public ::testing::Test {
public:
    void SetUp() override {
        mTempDir.reset(new base::TestTempDir("featurecontroltest"));
        mDefaultIniHostFilePath =
                mTempDir->makeSubPath("defaultSettingsHost.ini").c_str();
        mDefaultIniGuestFilePath =
                mTempDir->makeSubPath("defaultSettingsGuest.ini").c_str();
        mUserIniHostFilePath =
                mTempDir->makeSubPath("userSettingsHost.ini").c_str();
        mUserIniGuestFilePath =
                mTempDir->makeSubPath("userSettingsGuest.ini").c_str();

#define FEATURE_CONTROL_ITEM(item) \
        #item " = on\n"
        mAllOnIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = on\n"
        mAllOnIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
        mAllOffIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
        mAllOffIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
        mAllDefaultIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
        mAllDefaultIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM
    }
protected:
    void writeDefaultIniHost(android::base::StringView data) {
        writeIni(mDefaultIniHostFilePath, data);
    }
    void writeDefaultIniGuest(android::base::StringView data) {
        writeIni(mDefaultIniGuestFilePath, data);
    }
    void writeUserIniHost(android::base::StringView data) {
        writeIni(mUserIniHostFilePath, data);
    }
    void writeUserIniGuest(android::base::StringView data) {
        writeIni(mUserIniGuestFilePath, data);
    }
    void loadAllIni() {
        FeatureControlImpl::get().init(
                mDefaultIniHostFilePath, mDefaultIniGuestFilePath,
                mUserIniHostFilePath, mUserIniGuestFilePath);
    }
    std::unique_ptr<base::TestTempDir> mTempDir;
    std::string mDefaultIniHostFilePath;
    std::string mDefaultIniGuestFilePath;
    std::string mUserIniHostFilePath;
    std::string mUserIniGuestFilePath;
    std::string mAllOnIni;
    std::string mAllOnIniGuestOnly;
    std::string mAllOffIni;
    std::string mAllOffIniGuestOnly;
    std::string mAllDefaultIni;
    std::string mAllDefaultIniGuestOnly;

private:
    void writeIni(android::base::StringView filename,
                  android::base::StringView data) {
        std::ofstream outFile(filename,
                              std::ios_base::out | std::ios_base::trunc);
        ASSERT_TRUE(outFile.good());
        outFile.write(data.data(), data.size());
    }
};

}  // namespace

TEST_F(FeatureControlTest, overrideSetting) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        setEnabledOverride(feature, true);
        EXPECT_TRUE(isEnabled(feature));
        setEnabledOverride(feature, false);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, resetToDefault) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        bool defaultVal = isEnabled(feature);
        setEnabledOverride(feature, true);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
        setEnabledOverride(feature, false);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettings) {
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);

    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsWithNoUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsHostGuestDifferent) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");

#define FEATURE_CONTROL_ITEM(item) EXPECT_TRUE(isEnabled(item));
#include "FeatureControlDefHost.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) EXPECT_FALSE(isEnabled(item));
#include "FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);

    writeUserIniHost(mAllOnIni);
    writeUserIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeUserIniHost(mAllOffIni);
    writeUserIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, stringConversion) {
#define FEATURE_CONTROL_ITEM(item) \
    EXPECT_EQ(item, stringToFeature(#item)); \
    EXPECT_STREQ(#item, FeatureControlImpl::toString(item).c_str());
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM

    EXPECT_EQ(Feature_n_items,
              stringToFeature("somefeaturethatshouldneverexist"));
}

TEST_F(FeatureControlTest, setNonOverriden) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
    }

    Feature overriden = (Feature)0;
    setEnabledOverride(overriden, false);
    EXPECT_FALSE(isEnabled(overriden));

    setIfNotOverriden(overriden, true);
    EXPECT_FALSE(isEnabled(overriden));

    Feature nonOverriden = (Feature)1;
    EXPECT_TRUE(isEnabled(nonOverriden));
    EXPECT_FALSE(isOverridden(nonOverriden));
    setIfNotOverriden(nonOverriden, false);
    EXPECT_FALSE(isEnabled(nonOverriden));
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOn) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        EXPECT_TRUE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOff) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        if (isGuestFeature(feature)) {
            EXPECT_FALSE(isEnabled(feature));
        } else {
            EXPECT_TRUE(isEnabled(feature));
        }
    }
}

} // namespace featurecontrol
} // namespace android
