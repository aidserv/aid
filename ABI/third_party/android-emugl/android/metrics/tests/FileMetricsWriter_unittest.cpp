// Copyright 2016 The Android Open Source Project
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

#include "android/metrics/FileMetricsWriter.h"

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/Uuid.h"
#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/protobuf/DelimitedSerialization.h"
#include "android/utils/path.h"

#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>

using namespace android::base;
using namespace android::metrics;

static constexpr StringView kSpoolDir = ".android/metrics/spool";

namespace {

class FileMetricsWriterTest : public ::testing::Test {
public:
    TestSystem mSystem{"/", System::kProgramBitness, "/home"};
    TestTempDir* mRoot = mSystem.getTempRoot();
    std::unique_ptr<TestLooper> mLooper{new TestLooper()};

    FileMetricsWriter::Ptr mWriter;

    void SetUp() override {
        ASSERT_TRUE(mRoot != nullptr);
        ASSERT_TRUE(mLooper != nullptr);
        mSystem.envSet("ANDROID_SDK_HOME", mRoot->pathString());
    }

    std::string spoolDir() { return mRoot->makeSubPath(kSpoolDir.c_str()); }
    static std::string sessionId() { return Uuid::nullUuidStr; }

    void makeSpoolDir() { path_mkdir_if_needed(spoolDir().c_str(), 0700); }
};

}  // namespace

TEST_F(FileMetricsWriterTest, createDestroy) {
    EXPECT_FALSE(mSystem.pathIsDir(spoolDir()));
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0, nullptr, 0);
    EXPECT_TRUE(mWriter != nullptr);
    EXPECT_TRUE(mSystem.pathIsDir(spoolDir()));
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(2, (int)files.size());

    // one of the files is the open metrics file, another one is the lock.
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".lock" ||
                PathUtils::extension(files[1]) == ".lock");
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".open" ||
                PathUtils::extension(files[1]) == ".open");

    mWriter.reset();

    // on destruction it finalizes and unlocks the metrics file.
    files = mSystem.scanDirEntries(spoolDir());
    ASSERT_FALSE(files.empty());
    ASSERT_EQ(1, (int)files.size());
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".trk");
}

TEST_F(FileMetricsWriterTest, createDestroyWithTimer) {
    EXPECT_FALSE(mSystem.pathIsDir(spoolDir()));
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0,
                                        mLooper.get(), 10);
    EXPECT_TRUE(mWriter != nullptr);
    EXPECT_TRUE(mSystem.pathIsDir(spoolDir()));
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(2, (int)files.size());

    // make sure we have no timer, as it is only created on the first message
    // write.
    EXPECT_EQ(0U, mLooper->timers().size());

    // one of the files is the open metrics file, another one is the lock.
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".lock" ||
                PathUtils::extension(files[1]) == ".lock");
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".open" ||
                PathUtils::extension(files[1]) == ".open");

    mWriter.reset();

    // on destruction it finalizes and unlocks the metrics file.
    files = mSystem.scanDirEntries(spoolDir());
    ASSERT_FALSE(files.empty());
    ASSERT_EQ(1, (int)files.size());
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".trk");

    // we should still have no timers
    EXPECT_EQ(0U, mLooper->timers().size());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesNoDir) {
    // no spool directory - no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesEmptyDir) {
    makeSpoolDir();
    // empty spool directory - no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesManyFiles) {
    // create several session-like files now in the spool directory
    makeSpoolDir();

    std::vector<std::string> testSessions;
    for (int i = 0; i < 10; ++i) {
        testSessions.push_back(Uuid::generateFast().toString());
        ASSERT_TRUE(mRoot->makeSubFile(PathUtils::join(
                kSpoolDir, StringFormat("emulator-metrics-%s-1-1.open",
                                        testSessions.back()))));
        ASSERT_TRUE(mRoot->makeSubFile(PathUtils::join(
                kSpoolDir, StringFormat("emulator-metrics-%s-2-1.open",
                                        testSessions.back()))));
    }

    // now finalize them
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());

    // all test sessions should be present in |sessions|
    EXPECT_EQ(sessions.size(), testSessions.size());
    for (const auto& session : testSessions) {
        EXPECT_NE(sessions.end(), sessions.find(session));
    }

    // make sure the open files are renamed and there are no other files
    auto files = mSystem.scanDirEntries(spoolDir());
    // there were twice as many files as sessions
    EXPECT_EQ(2 * sessions.size(), files.size());

    for (const auto& file : files) {
        EXPECT_STREQ(PathUtils::extension(file).c_str(), ".trk");

        // make sure all files have a known session ID in the name
        EXPECT_NE(sessions.end(),
                  std::find_if(sessions.begin(), sessions.end(),
                               [&file](const std::string& session) {
                                   return file.find(session) !=
                                          std::string::npos;
                               }));
    }
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesWrongNames) {
    makeSpoolDir();

    // create some non-open-like files
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.trk")));
    ASSERT_TRUE(
            mRoot->makeSubFile(PathUtils::join(kSpoolDir, "emulator-metrics")));
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.open.closed")));
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.lock")));

    // no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesLockedSession) {
    makeSpoolDir();

    // create a locked open session file and make sure it is not finalized
    auto openSessionName = PathUtils::join(
            kSpoolDir,
            StringFormat("emulator-metrics-%s-1-1.open", sessionId()));
    ASSERT_TRUE(mRoot->makeSubFile(openSessionName));

    {
        // lock the file and try finalizing
        auto scopedLock = makeCustomScopedPtr(
                filelock_create(mRoot->makeSubPath(openSessionName).c_str()),
                filelock_release);
        ASSERT_TRUE(scopedLock);

        auto sessions =
                FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
        // no sessions
        EXPECT_TRUE(sessions.empty());
    }

    // check that now, when there's no lock, the only file left in the directory
    // is the open session file
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(1U, files.size());
    EXPECT_TRUE(openSessionName.find(files[0]) != std::string::npos)
            << "Expected file name '" << files[0] << "' to be a file name from"
                                                     " session file path '"
            << openSessionName << '\'';
}

TEST_F(FileMetricsWriterTest, writeSimple) {
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0, nullptr, 0);

    // create and write some event
    wireless_android_play_playlog::LogEvent event;
    event.set_is_user_initiated(true);
    event.set_tag("tag");
    event.set_source_extension("se");
    android_studio::AndroidStudioEvent asEvent;
    mWriter->write(asEvent, &event);

    mWriter.reset();

    // read the event back.
    auto files = mSystem.scanDirEntries(spoolDir(), true);
    ASSERT_EQ(1U, files.size());
    EXPECT_STREQ(PathUtils::extension(files[0]).c_str(), ".trk");

    std::ifstream in(files[0], std::ios_base::binary);
    EXPECT_TRUE(in);
    wireless_android_play_playlog::LogEvent readEvent;
    google::protobuf::io::IstreamInputStream inStream(&in);
    ASSERT_TRUE(android::protobuf::readOneDelimited(&readEvent, &inStream));

    // make sure it's the same event.
    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(event,
                                                                   readEvent));

    // and that there are no more events
    ASSERT_FALSE(android::protobuf::readOneDelimited(&readEvent, &inStream));
}

TEST_F(FileMetricsWriterTest, writeMultiple) {
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0, nullptr, 0);

    // create and write some events
    std::vector<wireless_android_play_playlog::LogEvent> events;
    for (int i = 0; i < 100; ++i) {
        wireless_android_play_playlog::LogEvent event;
        android_studio::AndroidStudioEvent asEvent;
        event.set_is_user_initiated(true);
        event.set_tag("tag");
        event.set_source_extension("se");
        event.set_event_code(i);
        mWriter->write(asEvent, &event);
        events.push_back(event);
    }

    mWriter.reset();

    // read the event back.
    auto files = mSystem.scanDirEntries(spoolDir(), true);
    ASSERT_EQ(1U, files.size());
    EXPECT_STREQ(PathUtils::extension(files[0]).c_str(), ".trk");

    std::ifstream in(files[0], std::ios_base::binary);
    EXPECT_TRUE(in);
    google::protobuf::io::IstreamInputStream inStream(&in);

    for (const auto& event : events) {
        wireless_android_play_playlog::LogEvent readEvent;
        ASSERT_TRUE(android::protobuf::readOneDelimited(&readEvent, &inStream));
        // make sure it's the same event.
        ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(event,
                                                                       readEvent));
    }

    // and that there are no more events
    wireless_android_play_playlog::LogEvent readEvent;
    ASSERT_FALSE(android::protobuf::readOneDelimited(&readEvent, &inStream));
}

TEST_F(FileMetricsWriterTest, writeLimited) {
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(),
                                        1,  // records per file
                                        nullptr, 0);

    // create and write some event
    android_studio::AndroidStudioEvent asEvent;
    std::array<wireless_android_play_playlog::LogEvent, 2> event;
    event[0].set_is_user_initiated(true);
    event[0].set_tag("tag");
    event[0].set_source_extension("se");
    mWriter->write(asEvent, &event[0]);

    {
        auto files = mSystem.scanDirEntries(spoolDir());
        // make sure we have one open and one finalized file as of now
        ASSERT_EQ(1, std::count_if(files.begin(), files.end(),
                                   [](const std::string& name) {
                                       return PathUtils::extension(name) ==
                                              ".open";
                                   }));
        ASSERT_EQ(1, std::count_if(files.begin(), files.end(),
                                   [](const std::string& name) {
                                       return PathUtils::extension(name) ==
                                              ".trk";
                                   }));
    }

    event[1].set_event_uptime_ms(100);
    event[1].add_test_code(100);
    event[1].set_store("store");
    mWriter->write(asEvent, &event[1]);

    mWriter.reset();

    // read the events back and make sure they're correct.
    auto files = mSystem.scanDirEntries(spoolDir(), true);
    ASSERT_EQ(3U, files.size()) << std::accumulate(files.begin(), files.end(),
                                                  std::string());
    // the last file is an empty one: it's the file created for the following
    // event.
    System::FileSize size;
    ASSERT_TRUE(mSystem.pathFileSize(files.back(), &size));
    ASSERT_EQ(0U, size);

    for (size_t i = 0; i < event.size(); ++i) {
        EXPECT_STREQ(PathUtils::extension(files[i]).c_str(), ".trk");
        std::ifstream in(files[i], std::ios_base::binary);
        EXPECT_TRUE(in);
        wireless_android_play_playlog::LogEvent readEvent;
        google::protobuf::io::IstreamInputStream inStream(&in);
        ASSERT_TRUE(android::protobuf::readOneDelimited(&readEvent, &inStream));

        // make sure it's the same event.
        ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
                event[i], readEvent));

        // and that there are no more events
        ASSERT_FALSE(
                android::protobuf::readOneDelimited(&readEvent, &inStream));
    }
}

TEST_F(FileMetricsWriterTest, writeTimered) {
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0,
                                        mLooper.get(),
                                        1000);  // 1 sec per file.

    EXPECT_EQ(0U, mLooper->timers().size());

    // create and write some event
    android_studio::AndroidStudioEvent asEvent;
    wireless_android_play_playlog::LogEvent event;
    event.set_is_user_initiated(true);
    event.set_tag("tag");
    event.set_source_extension("se");
    mWriter->write(asEvent, &event);

    EXPECT_EQ(1U, mLooper->timers().size());
    EXPECT_EQ(1U, mLooper->activeTimers().size());
    EXPECT_EQ(0U, mLooper->pendingTimers().size());

    {
        auto files = mSystem.scanDirEntries(spoolDir());
        // make sure we have one open and no finalized files as of now
        ASSERT_EQ(1, std::count_if(files.begin(), files.end(),
                                   [](const std::string& name) {
                                       return PathUtils::extension(name) ==
                                              ".open";
                                   }));
        ASSERT_EQ(0, std::count_if(files.begin(), files.end(),
                                   [](const std::string& name) {
                                       return PathUtils::extension(name) ==
                                              ".trk";
                                   }));
    }

    // now update the timestamp and run the looper to trigger the timer
    mSystem.setUnixTimeUs(2000000);
    mLooper->runOneIterationWithDeadlineMs(2000);

    // there's still one active timer..
    EXPECT_EQ(1U, mLooper->timers().size());
    EXPECT_EQ(1U, mLooper->activeTimers().size());
    EXPECT_EQ(0U, mLooper->pendingTimers().size());

    // now we should have one open and one finalized file as
    auto files = mSystem.scanDirEntries(spoolDir(), true);
    ASSERT_EQ(1, std::count_if(files.begin(), files.end(),
                               [](const std::string& name) {
                                   return PathUtils::extension(name) ==
                                          ".open";
                               }));
    ASSERT_EQ(1, std::count_if(files.begin(), files.end(),
                               [](const std::string& name) {
                                   return PathUtils::extension(name) ==
                                          ".trk";
                               }));

    EXPECT_STREQ(PathUtils::extension(files.front()).c_str(), ".trk");

    // read the event back and make sure they're correct.
    std::ifstream in(files.front(), std::ios_base::binary);
    EXPECT_TRUE(in);
    wireless_android_play_playlog::LogEvent readEvent;
    google::protobuf::io::IstreamInputStream inStream(&in);
    ASSERT_TRUE(android::protobuf::readOneDelimited(&readEvent, &inStream));

    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
            event, readEvent));
    ASSERT_FALSE(
            android::protobuf::readOneDelimited(&readEvent, &inStream));

    mWriter.reset();

    // no timers anymore
    EXPECT_EQ(0U, mLooper->timers().size());
    EXPECT_EQ(0U, mLooper->activeTimers().size());
    EXPECT_EQ(0U, mLooper->pendingTimers().size());
}
