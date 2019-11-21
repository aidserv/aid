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

#include "android/update-check/UpdateChecker.h"
#include "android/update-check/VersionExtractor.h"
#include "android/base/Optional.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/Version.h"

#include <gtest/gtest.h>

#include <string>

namespace android {
namespace update_check {

using android::base::Version;
using android::studio::UpdateChannel;

class MockVersionExtractor : public IVersionExtractor {
public:
    virtual Versions extractVersions(
            android::base::StringView data) const override {
        ++mExtractVersionCallCount;
        EXPECT_EQ(mExtractVersionDataParam, data);
        return mExtractVersionResult ? Versions{{UpdateChannel::Canary,
                                                 *mExtractVersionResult}}
                                     : Versions{};
    }

    virtual Version getCurrentVersion() const {
        ++mGetCurrentVersionCallCount;
        return mGetCurrentVersionResult;
    }

    std::string mExtractVersionDataParam;
    android::base::Optional<Version> mExtractVersionResult = {{0, 0, 0}};
    mutable int mExtractVersionCallCount = 0;
    Version mGetCurrentVersionResult = {0, 0, 0};
    mutable int mGetCurrentVersionCallCount = 0;
};

class MockDataLoader : public IDataLoader {
public:
    virtual std::string load() override {
        ++mLoadCallCount;
        return mLoadResult;
    }

    virtual std::string getUniqueDataKey() override {
        ++mGetUniqueDataKeyCallCount;
        return mUniqueDataKey;
    }

    std::string mLoadResult;
    int mLoadCallCount = 0;

    std::string mUniqueDataKey;
    int mGetUniqueDataKeyCallCount = 0;
};

class MockTimeStorage : public ITimeStorage {
public:
    virtual bool lock() override {
        ++mLockCallCount;
        return mLockResult;
    }

    virtual time_t getTime(const std::string& key) override {
        EXPECT_STREQ(mKeyParam.c_str(), key.c_str());
        ++mGetTimeCallCount;
        return mGetTimeResult;
    }

    virtual void setTime(const std::string& key, time_t time) override {
        EXPECT_STREQ(mKeyParam.c_str(), key.c_str());
        ++mSetTimeCallCount;
        EXPECT_EQ(mSetTimeParam, time);
    }

    int mLockCallCount = 0;
    int mGetTimeCallCount = 0;
    int mSetTimeCallCount = 0;
    bool mLockResult = false;
    time_t mGetTimeResult = 0;
    time_t mSetTimeParam = 0;
    std::string mKeyParam;
};

class MockNewerVersionReporter : public INewerVersionReporter {
public:
    virtual void reportNewerVersion(const Version& existing,
                                    const Version& newer) {
        EXPECT_EQ(mReportNewerVersionExistingParam, existing);
        EXPECT_EQ(mReportNewerVersionNewerParam, newer);
        ++mReportNewerVersionCallCount;
    }

    Version mReportNewerVersionExistingParam = {0, 0, 0};
    Version mReportNewerVersionNewerParam = {0, 0, 0};
    int mReportNewerVersionCallCount = 0;
};

class TestUpdateChecker : public UpdateChecker {
public:
    TestUpdateChecker(IVersionExtractor* extractor,
                      IDataLoader* loader,
                      ITimeStorage* storage,
                      INewerVersionReporter* reporter)
        : UpdateChecker(extractor, loader, storage, reporter) {}

    // make it public for testing
    using UpdateChecker::asyncWorker;
};

class TestData {
public:
    TestData() : mSystem("", 32) {
        mSystem.getTempRoot();
        mDataLoader = new MockDataLoader();
        mReporter = new MockNewerVersionReporter();
        mTimeStorage = new MockTimeStorage();
        mVersionExtractor = new MockVersionExtractor();
        mUC.reset(new TestUpdateChecker(mVersionExtractor, mDataLoader,
                                        mTimeStorage, mReporter));
    }

    std::unique_ptr<TestUpdateChecker> mUC;
    MockDataLoader* mDataLoader;
    MockNewerVersionReporter* mReporter;
    MockTimeStorage* mTimeStorage;
    MockVersionExtractor* mVersionExtractor;
    android::base::TestSystem mSystem;
};

TEST(UpdateChecker, init) {
    TestData test;
    test.mTimeStorage->mLockResult = false;
    EXPECT_FALSE(test.mUC->init());
    EXPECT_EQ(1, test.mTimeStorage->mLockCallCount);

    /////////////////////////////////////////////////////////////////////
    test.mTimeStorage->mLockResult = true;
    EXPECT_TRUE(test.mUC->init());
    EXPECT_EQ(2, test.mTimeStorage->mLockCallCount);
}

TEST(UpdateChecker, needsCheck) {
    TestData test;
    // set last check time and current time to be close enough -
    // no new check is needed
    test.mTimeStorage->mKeyParam = test.mDataLoader->mUniqueDataKey = "key";
    test.mTimeStorage->mGetTimeResult = time_t(1);
    test.mSystem.setUnixTime(time_t(2));

    EXPECT_FALSE(test.mUC->needsCheck());
    EXPECT_EQ(1, test.mDataLoader->mGetUniqueDataKeyCallCount);
    EXPECT_EQ(1, test.mTimeStorage->mGetTimeCallCount);

    /////////////////////////////////////////////////////////////////////
    // now set the current time to require the check (+one year from the epoch)
    test.mSystem.setUnixTime(time_t(365 * 24 * 60 * 60));
    EXPECT_TRUE(test.mUC->needsCheck());
    EXPECT_EQ(2, test.mTimeStorage->mGetTimeCallCount);
}

TEST(UpdateChecker, getVersion) {
    TestData test;

    test.mDataLoader->mLoadResult =
            test.mVersionExtractor->mExtractVersionDataParam = "test";
    test.mVersionExtractor->mExtractVersionResult = Version(1, 0, 0);

    auto returnedVersion = test.mUC->getLatestVersion();
    EXPECT_TRUE(returnedVersion);
    EXPECT_EQ(Version(1, 0, 0), returnedVersion->first);
    EXPECT_EQ(UpdateChannel::Canary, returnedVersion->second);
}

TEST(UpdateChecker, asyncWorker) {
    TestData test;

    // first set version to be the newest
    test.mDataLoader->mLoadResult =
            test.mVersionExtractor->mExtractVersionDataParam = "test";
    test.mVersionExtractor->mExtractVersionResult = Version(1, 0, 0);
    test.mVersionExtractor->mGetCurrentVersionResult = Version(1, 0, 0);
    test.mSystem.setUnixTime(time_t(1));
    test.mTimeStorage->mKeyParam = test.mDataLoader->mUniqueDataKey = "key";
    test.mTimeStorage->mSetTimeParam = time_t(1);

    test.mUC->asyncWorker();
    EXPECT_EQ(1, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(1, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(1, test.mVersionExtractor->mGetCurrentVersionCallCount);
    EXPECT_EQ(1, test.mTimeStorage->mSetTimeCallCount);

    // didn't report the newer version
    EXPECT_EQ(0, test.mReporter->mReportNewerVersionCallCount);

    /////////////////////////////////////////////////////////////////////
    // now there's a newer version available
    test.mVersionExtractor->mExtractVersionResult = Version(2, 0, 0);
    test.mReporter->mReportNewerVersionExistingParam = Version(1, 0, 0);
    test.mReporter->mReportNewerVersionNewerParam = Version(2, 0, 0);

    test.mUC->asyncWorker();
    EXPECT_EQ(2, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(2, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(2, test.mVersionExtractor->mGetCurrentVersionCallCount);
    EXPECT_EQ(2, test.mTimeStorage->mSetTimeCallCount);

    // reported the newer version
    EXPECT_EQ(1, test.mReporter->mReportNewerVersionCallCount);

    /////////////////////////////////////////////////////////////////////
    // failed to get the last version
    test.mVersionExtractor->mExtractVersionResult = {};

    test.mUC->asyncWorker();
    EXPECT_EQ(3, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(3, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(3, test.mVersionExtractor->mGetCurrentVersionCallCount);

    // didn't store the last check time
    EXPECT_EQ(2, test.mTimeStorage->mSetTimeCallCount);
    // didn't report the newer version
    EXPECT_EQ(1, test.mReporter->mReportNewerVersionCallCount);
}

}  // namespace update_check
}  // namespace android
