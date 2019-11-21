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

#include "android/base/async/Looper.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/metrics/MetricsWriter.h"
#include "android/utils/filelock.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <fstream>
#include <memory>
#include <string>

namespace android {
namespace metrics {

//
// FileMetricsWriter - a MetricsWriter implementation that serializes incoming
// messages into a binary file, using protobuf's writeDelimited() call.
// This is the way Android Studio serializes the messages and how its metrics
// sender expects them to be stored.
//
// Class also manages the lifetime of the files, rotating them every
// |recordCountLimit| messages or |timeLimitMs| milliseconds (if there was at
// least one message during that time).
//
// Android Studio's metrics sender monitors the spool directory for .trk files
// and sends them automatically if they aren't locked with Java's FileLock.
// Java implements FileLock in some unspecified 'platform-native' way, so
// instead of trying to mimic that, FileMetricsWriter uses a different approach:
//
//   1. Create a file with a different extension, .open
//   2. Lock that file with emulator's FileLock instead of the Java's one.
//   3. When finalizing the file (e.g. opening a new one or stopping the writer)
//      rename it .open -> .trk and unlock after that.
//   4. Provide a method to collect all 'abandoned' files that are not locked
//      but arent .trk yet. This is to detect crashed emulator instances and
//      make sure their logs are sent.
//   5. Include the session ID into the log file name to be able to report the
//      crashed emulator's sessionID without parsing the file (or if it crashed
//      before writing a single message).
//
// This approach allows emulator to reuse Android Studio's metrics sender
// without relying on its Java-specific implementation.
//

class FileMetricsWriter final
        : public MetricsWriter,
          public std::enable_shared_from_this<FileMetricsWriter> {
public:
    using Ptr = std::shared_ptr<FileMetricsWriter>;
    using WPtr = std::weak_ptr<FileMetricsWriter>;

    // Creates a new instance of FileMetricsWriter.
    //  |spoolDir| - a directory to put files into.
    //  |sessionId| - emulator session (run) ID for metrics logging.
    //  |recordCountLimit| - create new file after logging this many events.
    //      Disabled if <=0.
    //  |looper| - an instance of a |Looper| object to create timed events.
    //  |timeLimitMs| - create new file after this many milliseconds, if there
    //      was at least one event logged. Disabled if <=0 or |logger| is NULL.
    static Ptr create(base::StringView spoolDir,
                      const std::string& sessionId,
                      int recordCountLimit,
                      base::Looper* looper,
                      base::System::Duration timeLimitMs);

    // Scans the |spoolDir| directory for unlocked non-finalzed log files
    // (*.open), renames those to the correct .trk files and returns a set of
    // sessions that had such files. Useful for a cleanup or crash reporting
    // process.
    static AbandonedSessions finalizeAbandonedSessionFiles(
            base::StringView spoolDir);

    ~FileMetricsWriter() override;

    // MetricsWriter::write() implementation that writes into the current log
    // file.
    void write(const android_studio::AndroidStudioEvent& asEvent,
               wireless_android_play_playlog::LogEvent* logEvent) override;

private:
    // A private constructor to enforce the use of shared_ptr<>.
    FileMetricsWriter(base::StringView spoolDir,
                      const std::string& sessionId,
                      int recordCountLimit,
                      base::Looper* looper,
                      base::System::Duration timeLimitMs);
    void finalizeActiveFileNoLock();
    void openNewFileNoLock();
    bool countLimitReached() const;

    // Timer creation needs shared_from_this(), but it isn't available in class
    // ctor; that's why we have this function for a lazy timer creation.
    void ensureTimerStarted();
    void onTimer();

    // Runs a requested file operation until it succeeds or up to a predefined
    // number of tries.
    // |filenameCounter| - a pointer to a counter to increment on each try.
    // |op| - operation to run, should be callable with no arguments and
    //        return |true| when it succeeds.
    template <class Operation>
    static bool runFileOperationWithRetries(int* filenameCounter, Operation op);

    // Increments a passed *|counter|, wrapping it at a predefined limit.
    static void advanceFilenameCounter(int* counter);

private:
    const std::string mSpoolDir;
    const base::System::Duration mTimeLimitMs;
    const int mRecordCountLimit;
    base::Looper* const mLooper;

    // This lock protects all mutable members. It's OK to have a single lock for
    // everything as we don't really expect any contention - the idea is that
    // most of the methods are called from a single worker thread of
    // AsyncMetricsReporter.
    base::Lock mLock;

    std::unique_ptr<base::Looper::Timer> mTimer;
    std::string mActiveFileName;
    std::unique_ptr<google::protobuf::io::FileOutputStream> mActiveFile;
    base::ScopedCustomPtr<FileLock, decltype(&filelock_release)>
            mActiveFileLock;
    // Number of records logged into the current active file.
    int mRecordCount = 0;

    // A counter variable that's appended to the file name to make it unique.
    // We preserve it as an object state to make sure we start from the next
    // counter value every time we perform an open() or rename() - this way
    // we have the least number of name clashes, compared to always starting
    // from 0.
    int mFilenameCounter = 0;
};

}  // namespace metrics
}  // namespace android
