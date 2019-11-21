// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/update-check/VersionExtractor.h"

#include <gtest/gtest.h>

#include <string>

using android::base::Version;
using android::studio::UpdateChannel;
using android::update_check::VersionExtractor;

TEST(VersionExtractorTest, validVersion) {
    std::string xml =
            R"(<sdk:sdk-repository
                xmlns:common="http://schemas.android.com/repository/android/common/01"
                xmlns:generic="http://schemas.android.com/repository/android/generic/01"
                xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"
                xmlns:sdk-common="http://schemas.android.com/sdk/android/repo/common/01"
                xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"> )"
            R"(<remotePackage path="tools">)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>3</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    auto ver = VersionExtractor().extractVersions(xml);
    EXPECT_EQ(1U, ver.size());
    EXPECT_EQ(Version(1,2,3), ver[UpdateChannel::Canary]);
}

TEST(VersionExtractorTest, toolsToEmulatorPackageChange) {
    {
        std::string xml =
                R"(<sdk:sdk-repository
                    xmlns:common="http://schemas.android.com/repository/android/common/01"
                    xmlns:generic="http://schemas.android.com/repository/android/generic/01"
                    xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"
                    xmlns:sdk-common="http://schemas.android.com/sdk/android/repo/common/01"
                    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"> )"
                R"(<remotePackage path="tools">)"
                R"(<revision>)"
                R"(<major>25</major>)"
                R"(<minor>3</minor>)"
                R"(<micro>3</micro>)"
                R"(</revision>)"
                R"(</remotePackage>)"
                R"(<remotePackage path="emulator">)"
                R"(<revision>)"
                R"(<major>25</major>)"
                R"(<minor>3</minor>)"
                R"(<micro>2</micro>)"
                R"(</revision>)"
                R"(</remotePackage>)"
                R"(</sdk:sdk-repository>)";

        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(1U, ver.size());
        // It should pick the version from "emulator" package as it's >=25.3.
        EXPECT_EQ(Version(25, 3, 2), ver[UpdateChannel::Canary]);
    }
    {
        std::string xml =
                R"(<sdk:sdk-repository
                    xmlns:common="http://schemas.android.com/repository/android/common/01"
                    xmlns:generic="http://schemas.android.com/repository/android/generic/01"
                    xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"
                    xmlns:sdk-common="http://schemas.android.com/sdk/android/repo/common/01"
                    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"> )"
                R"(<remotePackage path="tools">)"
                R"(<revision>)"
                R"(<major>24</major>)"
                R"(<minor>3</minor>)"
                R"(<micro>1</micro>)"
                R"(</revision>)"
                R"(</remotePackage>)"
                R"(<remotePackage path="emulator">)"
                R"(<revision>)"
                R"(<major>24</major>)"
                R"(<minor>3</minor>)"
                R"(<micro>2</micro>)"
                R"(</revision>)"
                R"(</remotePackage>)"
                R"(</sdk:sdk-repository>)";

        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(1U, ver.size());
        // It should pick the version from "tools" package as it's <25.3.
        EXPECT_EQ(Version(24, 3, 1), ver[UpdateChannel::Canary]);
    }
}

TEST(VersionExtractorTest, withBuild) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
            R"(<remotePackage path="tools">)"
            R"(<!--
            Generated from bid:2665432, branch:nyc_preview_release
            -->)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>3</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    auto ver = VersionExtractor().extractVersions(xml);
    EXPECT_EQ(1U, ver.size());
    EXPECT_EQ(Version(1,2,3,2665432), ver[UpdateChannel::Canary]);
}

TEST(VersionExtractorTest, withChannel) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
            R"(<channel id="ch1">stable</channel>)"
            R"(<remotePackage path="tools">)"
            R"(<!--
            Generated from bid:2665432, branch:nyc_preview_release
            -->)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>3</micro>)"
            R"(</revision>)"
            R"(<channelRef ref="ch1"/>)"
            R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    auto ver = VersionExtractor().extractVersions(xml);
    ASSERT_EQ(1U, ver.size());
    EXPECT_EQ(Version(1,2,3,2665432), ver[UpdateChannel::Stable]);
}

TEST(VersionExtractorTest, pickMaxVersion) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
            R"(<remotePackage path="tools">)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>30</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(<remotePackage path="tools">)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>0</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(<remotePackage path="tools">)"
            R"(<revision>)"
            R"(<major>0</major>)"
            R"(<minor>99</minor>)"
            R"(<micro>09</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    auto ver = VersionExtractor().extractVersions(xml);
    EXPECT_EQ(1U, ver.size());
    EXPECT_EQ(Version(1,2,30), ver[UpdateChannel::Canary]);
}

TEST(VersionExtractorTest, pickMaxVersionManyChannels) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
                R"(<channel id="ch1">stable</channel>)"
                R"(<channel id="ch2">dev</channel>)"
                R"(<channel id="ch3">beta</channel>)"
                R"(<remotePackage path="tools">)"
                    R"(<revision>)"
                    R"(<major>1</major>)"
                    R"(<minor>2</minor>)"
                    R"(<micro>30</micro>)"
                    R"(</revision>)"
                    R"(<channelRef ref="ch1"/>)"
                R"(</remotePackage>)"
                R"(<remotePackage path="tools">)"
                    R"(<revision>)"
                    R"(<major>1</major>)"
                    R"(<minor>2</minor>)"
                    R"(<micro>0</micro>)"
                    R"(</revision>)"
                    R"(<channelRef ref="ch1"/>)"
                R"(</remotePackage>)"
                R"(<remotePackage path="tools">)"
                    R"(<revision>)"
                    R"(<major>0</major>)"
                    R"(<minor>99</minor>)"
                    R"(<micro>09</micro>)"
                    R"(</revision>)"
                    R"(<channelRef ref="ch1"/>)"
                R"(</remotePackage>)"
                R"(<remotePackage path="tools">)"
                    R"(<revision>)"
                    R"(<major>2</major>)"
                    R"(<minor>0</minor>)"
                    R"(<micro>0</micro>)"
                    R"(</revision>)"
                    R"(<channelRef ref="ch3"/>)"
                R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    auto ver = VersionExtractor().extractVersions(xml);
    ASSERT_EQ(2U, ver.size());
    EXPECT_EQ(Version(1,2,30), ver[UpdateChannel::Stable]);
    EXPECT_EQ(Version(2,0,0), ver[UpdateChannel::Beta]);
}

TEST(VersionExtractorTest, badVersion) {
    // bad xml
    std::string xml =
            R"(sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
            R"(<remotePackage path="tools">)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>3</micro>)"
            R"(</revision>)"
            R"(</remotePackage>)"
            R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no namespace
    xml = R"(<sdk-repository>)"
          R"(<tool>)"
          R"(<revision>)"
          R"(<major>1</major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</tool>)"
          R"(</sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no 'tool' element
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<sdk:tool1>)"
          R"(<revision>)"
          R"(<major>1</major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</sdk:tool1>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // wrong namespace
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.windows-phone.com/sdk/iOS/repository/12"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<major>1</major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no 'revision'
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<sdk:evision>)"
          R"(<major>1</major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</sdk:evision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no major
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no minor
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<major>2</major>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // no micro
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<major>1</major>)"
          R"(<minor>2</minor>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // bad number in version
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<major>1asd</major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }

    // bad number in version 2
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repo/repository2/01"> )"
          R"(<remotePackage path="tools">)"
          R"(<revision>)"
          R"(<major></major>)"
          R"(<minor>2</minor>)"
          R"(<micro>3</micro>)"
          R"(</revision>)"
          R"(</remotePackage>)"
          R"(</sdk:sdk-repository>)";

    {
        auto ver = VersionExtractor().extractVersions(xml);
        EXPECT_EQ(0U, ver.size());
    }
}
