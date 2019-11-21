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

#include "android/metrics/FileMetricsWriter.h"

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/Uuid.h"
#include "android/metrics/MetricsLogging.h"
#include "android/protobuf/DelimitedSerialization.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"

#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using android::base::AutoLock;
using android::base::Looper;
using android::base::makeCustomScopedPtr;
using android::base::PathUtils;
using android::base::System;
using android::base::StringView;
using android::base::StringFormat;
using android::base::Uuid;

namespace android {
namespace metrics {

// Only Windows has a No-inherit open() flag, and only Windows cares about
// difference between binary/text files.
#ifdef _WIN32
static constexpr int kCommonOpenFlags = O_BINARY | O_NOINHERIT;
#else
static constexpr int kCommonOpenFlags = 0;
#endif

// Extensions for an active temporary logging file and for the finalized one.
static constexpr StringView kTempExtension = ".open";
static constexpr StringView kFinalExtension = ".trk";

// A limit for a counter appended to the log filename to enable multiple files
// with the same session ID.
static constexpr int kMaxFilenameCounter = 1000000;
// Number of retries for a failed file operation, e.g. open(), rename(), etc.
static constexpr int kMaxFileOperationTries = 1000;

// Create a log filename with the passed |sessionId|, |counter| and |extension|.
static std::string formatFilename(StringView sessionId,
                                  int counter,
                                  StringView extension) {
    return StringFormat("emulator-metrics-%s-%d-%d%s", sessionId,
                        (int)System::get()->getCurrentProcessId(), counter,
                        extension);
}

// A cross-platform function to rename an |from| file to |to| name only if |to|
// doesn't exist.
// On POSIX one needs to make sure the |to| doesn't exist, otherwise it will be
// owerwritten. On Windows rename() never overwrites target.
static bool renameIfNotExists(StringView from, StringView to) {
#ifdef _WIN32
    return rename(from.c_str(), to.c_str()) == 0;
#else
    if (System::get()->pathExists(to)) {
        return false;
    }
    // There's a small chance of race condition here, but we don't actually
    // rename files in several threads, so it's OK.
    // TODO(zyy): use renameat2() on Linux to make it atomic.
    if (HANDLE_EINTR(rename(from.c_str(), to.c_str())) != 0) {
        return false;
    }
    return true;
#endif
}

FileMetricsWriter::FileMetricsWriter(StringView spoolDir,
                                     const std::string& sessionId,
                                     int recordCountLimit,
                                     Looper* looper,
                                     System::Duration timeLimitMs)
    : MetricsWriter(sessionId),
      mSpoolDir(spoolDir),
      mTimeLimitMs(timeLimitMs),
      mRecordCountLimit(recordCountLimit),
      mLooper(looper),
      mActiveFileLock(
          makeCustomScopedPtr<FileLock*>(nullptr, filelock_release)) {
    D("created a FileMetricsWriter");
    assert(strlen(spoolDir.data()) == spoolDir.size());
    path_mkdir_if_needed(spoolDir.data(), 0744);
    assert(System::get()->pathIsDir(spoolDir));

    // It's OK to call this without a lock in ctor - there's no external
    // references to the object yet.
    openNewFileNoLock();
}

FileMetricsWriter::Ptr FileMetricsWriter::create(StringView spoolDir,
                                                 const std::string& sessionId,
                                                 int recordCountLimit,
                                                 Looper* looper,
                                                 System::Duration timeLimitMs) {
    return Ptr(new FileMetricsWriter(spoolDir, sessionId, recordCountLimit,
                                     looper, timeLimitMs));
}

MetricsWriter::AbandonedSessions
FileMetricsWriter::finalizeAbandonedSessionFiles(StringView spoolDir) {
    AbandonedSessions abandonedSessions;
    const std::vector<std::string> files =
            System::get()->scanDirEntries(spoolDir);
    for (const std::string& file : files) {
        if (PathUtils::extension(file) != kTempExtension) {
            continue;
        }

        // try to lock the file to find out if it's abandoned
        const auto lock = makeCustomScopedPtr(
                filelock_create(PathUtils::join(spoolDir, file).c_str()),
                filelock_release);
        if (!lock) {
            continue;
        }

        if (!System::get()->pathIsFile(PathUtils::join(spoolDir, file))) {
            D("Saw a ghost file right before it disappeared from the file "
              "system; most probably a rename: '%s'", file.c_str());
            continue;
        }

        // parse the file name to get session ID and the filename counter
        int filenameCounter = 0;
        std::string sessionId = Uuid::nullUuidStr;
        assert(sessionId.size() >= 36);
        int scanned = sscanf(file.c_str(), "emulator-metrics-%36s-%*d-%d",
                             &sessionId[0], &filenameCounter);
        assert(scanned == 2);
        (void)scanned;

        // now rename it
        if (!runFileOperationWithRetries(&filenameCounter, [spoolDir,
                                                            &sessionId,
                                                            &filenameCounter,
                                                            &file]() -> bool {
                const std::string finalName = PathUtils::join(
                        spoolDir, formatFilename(sessionId, filenameCounter,
                                                 kFinalExtension));
                return renameIfNotExists(PathUtils::join(spoolDir, file),
                                         finalName);
            })) {
            W("failed to rename an abandoned log file '%s'", file.c_str());
        }

        abandonedSessions.insert(std::move(sessionId));
    }

    return abandonedSessions;
}

FileMetricsWriter::~FileMetricsWriter() {
    AutoLock lock(mLock);
    if (mTimer) {
        mTimer->stop();
        mTimer.reset();
    }
    finalizeActiveFileNoLock();
}

void FileMetricsWriter::finalizeActiveFileNoLock() {
    if (!mActiveFile) {
        return;
    }

    mActiveFile.reset();
    if (!runFileOperationWithRetries(&mFilenameCounter, [this]() -> bool {
            const std::string finalName = PathUtils::join(
                    mSpoolDir, formatFilename(sessionId(), mFilenameCounter,
                                              kFinalExtension));
            return renameIfNotExists(mActiveFileName, finalName);
        })) {
        W("failed to rename an active log file '%s'", mActiveFileName.c_str());
    } else {
        D("finalized active log file '%s'", mActiveFileName.c_str());
    }
    mActiveFileLock.reset();
    mActiveFileName.clear();
    mRecordCount = 0;
    // We've done with the current file no, make sure the next one gets a
    // new counter value.
    advanceFilenameCounter(&mFilenameCounter);
}

void FileMetricsWriter::openNewFileNoLock() {
    std::string newName;
    int fd = -1;
    if (!runFileOperationWithRetries(&mFilenameCounter, [this, &newName,
                                                         &fd]() -> bool {
            std::string testName = PathUtils::join(
                    mSpoolDir, formatFilename(sessionId(), mFilenameCounter,
                                              kTempExtension));

            mActiveFileLock.reset(filelock_create(testName.c_str()));
            if (!mActiveFileLock) {
                return false;
            }
            const int testFd = HANDLE_EINTR(
                    open(testName.c_str(),
                         O_WRONLY | O_CREAT | O_EXCL | kCommonOpenFlags,
                         S_IREAD | S_IWRITE));
            if (testFd < 0) {
                mActiveFileLock.reset();
                return false;
            }
            newName = std::move(testName);
            fd = testFd;
            return true;
        })) {
        W("failed to open a new log file");
        return;
    }

    assert(fd >= 0);
    assert(!newName.empty());

    mActiveFile.reset(new google::protobuf::io::FileOutputStream(fd));
    if (!mActiveFile) {
        E("memory allocation failed");
        close(fd);
        HANDLE_EINTR(unlink(newName.c_str()));
        mActiveFileLock.reset();
        return;
    }
    mActiveFile->SetCloseOnDelete(true);
    mActiveFileName = std::move(newName);
    mRecordCount = 0;

    D("opened new metrics file %s", mActiveFileName.c_str());
}

bool FileMetricsWriter::countLimitReached() const {
    // Only check the count limit if there is one.
    return mRecordCountLimit > 0 && mRecordCount >= mRecordCountLimit;
}

void FileMetricsWriter::advanceFilenameCounter(int* counter) {
    *counter = (*counter + 1) % kMaxFilenameCounter;
}

void FileMetricsWriter::ensureTimerStarted() {
    if (mTimer || mTimeLimitMs <= 0 || !mLooper) {
        return;
    }

    mTimer.reset(mLooper->createTimer(
            [](void* weakSelf, Looper::Timer* timer) {
                auto weakPtr =
                        static_cast<FileMetricsWriter::WPtr*>(weakSelf);
                if (auto ptr = weakPtr->lock()) {
                    ptr->onTimer();
                } else {
                    delete weakPtr;  // we're done
                }
            },
            new WPtr(shared_from_this())));
    if (mTimer) {
        mTimer->startRelative(mTimeLimitMs);

        D("started metrics timer with a period of %d ms",
          mTimeLimitMs);
    }
}

void FileMetricsWriter::onTimer() {
    D("timer tick");

    AutoLock lock(mLock);
    if (mRecordCount > 0) {
        finalizeActiveFileNoLock();
        openNewFileNoLock();
    }

    mTimer->startRelative(mTimeLimitMs);
}

template <class Operation>
bool FileMetricsWriter::runFileOperationWithRetries(int* filenameCounter,
                                                    Operation op) {
    for (int i = 0; i < kMaxFileOperationTries;
         ++i, advanceFilenameCounter(filenameCounter)) {
        if (op()) {
            return true;
        }
    }

    return false;
}

void FileMetricsWriter::write(
        const android_studio::AndroidStudioEvent& asEvent,
        wireless_android_play_playlog::LogEvent* logEvent) {
    D("writing a log event with uptime %ld ms", logEvent->event_uptime_ms());

    asEvent.SerializeToString(logEvent->mutable_source_extension());

    AutoLock lock(mLock);

    if (!mActiveFile) {
        openNewFileNoLock();
    }
    if (!mActiveFile) {
        W("No active log file during write(), event lost:\n%s",
          logEvent->DebugString().c_str());
        return;
    }
    ensureTimerStarted();

    protobuf::writeOneDelimited(*logEvent, mActiveFile.get());
    // FileOutputStream buffers everything it can, but it means we may lose
    // a message on a crash, so let's just flush it every time.
    // Shouldn't affect performance as it's expected that only asynchronous
    // metrics reporter is used in the emulator process.
    mActiveFile->Flush();
    ++mRecordCount;

    if (countLimitReached()) {
        finalizeActiveFileNoLock();
        openNewFileNoLock();
    }
}

}  // namespace metrics
}  // namespace android
