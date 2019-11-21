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

#pragma once

#include "android/base/StringView.h"
#include "android/base/files/ScopedFd.h"
#include "android/CommonReportedInfo.h"
#include "android/crashreport/CrashSystem.h"

#include <functional>

#include <atomic>
#include <cassert>

namespace android {
namespace crashreport {

// Class CrashReporter is a singleton class that wraps breakpad OOP crash
// client.
// It provides functions to attach to a crash server and to wait for a crash
// server to start crash communication pipes.
class CrashReporter {
public:
    using CrashCallback = std::function<void()>;

    static const int kWaitExpireMS = 500;
    static const int kWaitIntervalMS = 20;

    // Name of the file with the dump message passed from the emulator in
    // a dump data exchange directory
    static const char* const kDumpMessageFileName;

    // File with the process memory information
    static const char* const kProcessMemoryInfoFileName;

    // File to log crashes on exit
    static const char* const kCrashOnExitFileName;

    // File to log the process list
    static const char* const kProcessListFileName;

    static const char* const kEmulatorHostFileName;
    static const char* const kEmulatorDetailsFileName;
    static const char* const kEmulatorPerformanceStatsFileName;

    // Pattern to check for when detecting crashes on exit
    static const char* const kCrashOnExitPattern;

    // QSetting key that is saved when crash reporting automatically or not
    static const char* const kProcessCrashesQuietlyKey;

    // Preallocation size for a chunk of memory for the crash info protobuf
    // binary data. Let's try not to increase memory usage during the crash.
    static const int kCrashInfoProtobufStrInitialSize;

    CrashReporter();

    virtual ~CrashReporter();

    // Attach platform dependent crash handler.
    // Returns false if already attached or if attach fails.
    virtual bool attachCrashHandler(
            const CrashSystem::CrashPipe& crashpipe) = 0;

    // Waits for a platform dependent pipe to become valid or timeout occurs.
    // Returns false if timeout occurs.
    virtual bool waitServicePipeReady(const std::string& pipename,
                                      int timeout_ms = kWaitExpireMS) = 0;

    // Special config when crash service is in child process
    virtual void setupChildCrashProcess(int pid) = 0;

    // returns dump dir
    const std::string& getDumpDir() const;

    // returns the directory for data exchange files. All files from this
    // directory will go to the reporting server together with the crash dump.
    const std::string& getDataExchangeDir() const;

    // Gets a handle to single instance of crash reporter
    static CrashReporter* get();

    // Pass some data to the crash reporter, so in case of a crash it's uploaded
    // with the dump
    // |name| - a generic description of the data being added. Current
    //          implementation uploads the data in a file named |name|
    //          if |name| is empty the file gets some default generic name
    // |data| - a string of data to upload with the crash report
    // |replace| - replace all the data with the same name instead of appending
    void attachData(android::base::StringView name,
                    android::base::StringView data,
                    bool replace = false);

    // Same as attachData, but for binary data, so there is no text processing.
    void attachBinaryData(android::base::StringView name,
                          android::base::StringView data,
                          bool replace = true);

    // Pass some file to the crash reporter to upload it with the dump
    bool attachFile(android::base::StringView sourceFullName,
                    android::base::StringView destBaseName);

    // Opens a descriptor to an attachment file, so one can write there
    // directly.
    base::ScopedFd openDataAttachFile(
            base::StringView name,
            bool replace = false,
            bool binary = false);

    // The following two functions write a dump of current process state.
    // Both pass the |message| to the dump writer, so it is sent together with
    // the dump file
    // GenerateDumpAndDie() also doesn't return - it terminates process in a
    // fastest possible way. The process doesn't show/print any message to the
    // user with the possible exception of "Segmentation fault".
    void GenerateDump(const char* message);
    void GenerateDumpAndDie(const char* message);

    void SetExitMode(const char* message);

    void addCrashCallback(CrashCallback cb) {
        assert(bool(cb));
        mCrashCallbacks.push_back(std::move(cb));
    }

    bool onCrash();

private:
    virtual bool onCrashPlatformSpecific() = 0;
    virtual void writeDump() = 0;
    // Pass the |message| to the crash service process
    void passDumpMessage(const char* message);

private:
    DISALLOW_COPY_AND_ASSIGN(CrashReporter);

    std::vector<CrashCallback> mCrashCallbacks;
    const std::string mDumpDir;
    const std::string mDataExchangeDir;
    std::string mProtobufData;
    std::atomic<bool> mIsInExitMode { false };
};

}  // crashreport
}  // android
