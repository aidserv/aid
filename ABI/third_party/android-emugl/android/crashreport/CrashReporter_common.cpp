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

#include "android/crashreport/CrashReporter.h"

#include "android/crashreport/crash-handler.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/Uuid.h"
#include "android/metrics/metrics.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"

#include <fstream>

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#endif

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

using android::base::PathUtils;
using android::base::ScopedFd;
using android::base::StringView;
using android::base::System;
using android::base::Uuid;

namespace android {
namespace crashreport {

const char* const CrashReporter::kDumpMessageFileName =
        "internal-error-msg.txt";
const char* const CrashReporter::kProcessMemoryInfoFileName =
        "process-memory-info.txt";
const char* const CrashReporter::kCrashOnExitFileName =
        "crash-on-exit.txt";
const char* const CrashReporter::kProcessListFileName =
        "system-process-list.txt";
const char* const CrashReporter::kEmulatorHostFileName =
        "emulator-host.proto";
const char* const CrashReporter::kEmulatorDetailsFileName =
        "emulator-details.proto";
const char* const CrashReporter::kEmulatorPerformanceStatsFileName =
        "emulator-performance-stats.proto";
const char* const CrashReporter::kCrashOnExitPattern =
        "Crash on exit";

const char* const CrashReporter::kProcessCrashesQuietlyKey =
        "set/processCrashesQuietly";

const int CrashReporter::kCrashInfoProtobufStrInitialSize = 4096;

CrashReporter::CrashReporter()
    : mDumpDir(System::get()->getTempDir()),
      // TODO: add a function that can create a directory or error-out
      // if it exists atomically. For now let's just allow UUIDs to do their
      // job to keep these unique
      mDataExchangeDir(
              PathUtils::join(mDumpDir, Uuid::generateFast().toString())) {
    mProtobufData.reserve(kCrashInfoProtobufStrInitialSize);
    const auto res = path_mkdir_if_needed(mDataExchangeDir.c_str(), 0744);
    if (res < 0) {
        E("Failed to create temp directory for crash service communication: "
          "'%s'",
          mDataExchangeDir.c_str());
    }
}

CrashReporter::~CrashReporter() = default;

const std::string& CrashReporter::getDumpDir() const {
    return mDumpDir;
}

const std::string& CrashReporter::getDataExchangeDir() const {
    return mDataExchangeDir;
}

void CrashReporter::GenerateDump(const char* message) {
    passDumpMessage(message);
    writeDump();
}

void CrashReporter::GenerateDumpAndDie(const char* message) {
    android_metrics_stop(METRICS_STOP_CRASH);
    passDumpMessage(message);
    // this is the most cross-platform way of crashing
    // any other I know about has its flaws:
    //  - abort() isn't caught by Breakpad on Windows
    //  - null() may screw the callstack
    //  - explicit *null = 1 can be optimized out
    //  - requesting dump and exiting later has a very noticable delay in
    //    between, so some real crash could stick in the middle
    volatile int* volatile ptr = nullptr;
    *ptr = 1313;  // die
}

void CrashReporter::SetExitMode(const char* msg) {
    if (!mIsInExitMode.exchange(true)) {
        android::metrics::MetricsReporter::get().report(
                [](android_studio::AndroidStudioEvent* event) {
                    event->mutable_emulator_details()->set_exit_started(true);
                    event->mutable_emulator_details()->set_session_phase(
                            android_studio::EmulatorDetails::EXIT_GENERAL);
                });
        // Flush the metrics reporter to make sure we write down this message
        // before crashing on exit.
        android::metrics::MetricsReporter::get().finishPendingReports();
    }
    attachData(kCrashOnExitFileName, msg);
}

void CrashReporter::passDumpMessage(const char* message) {
    attachData(kDumpMessageFileName, message);
}

// Construct the full name of a file to put the data for the crash reporter
// Don't allocate!
template <size_t N>
static void formatDataFileName(char (&buffer)[N], StringView baseName) {
    static_assert(N >= PATH_MAX, "Too small buffer for a path");

    // don't do any dynamic allocation here - it might be called during dump
    // writing, e.g. because of OOM exception
    memset(&buffer[0], 0, N);
    snprintf(buffer, N - 1,
             "%s%c%s",
             CrashReporter::get()->getDataExchangeDir().c_str(),
             System::kDirSeparator,
             (baseName.empty() ? "additional_data.txt" : baseName.c_str()));
}

void CrashReporter::attachData(StringView name, StringView data, bool replace) {
    auto fd = openDataAttachFile(name, replace);
    HANDLE_EINTR(write(fd.get(), data.data(), data.size()));
    HANDLE_EINTR(write(fd.get(), "\n", 1));
}

void CrashReporter::attachBinaryData(StringView name, StringView data, bool replace) {
    auto fd = openDataAttachFile(name, replace, true);
    HANDLE_EINTR(write(fd.get(), data.data(), data.size()));
}

bool CrashReporter::attachFile(StringView sourceFullName,
                               StringView destBaseName) {
    char fullName[PATH_MAX + 1];
    formatDataFileName(fullName, destBaseName);

    return path_copy_file_safe(fullName, sourceFullName.c_str()) >= 0;
}

ScopedFd CrashReporter::openDataAttachFile(StringView name, bool replace, bool binary) {
    const int bufferLength = PATH_MAX + 1;
    char fullName[bufferLength];
    formatDataFileName(fullName, name);

    // Open the communication file in append mode to make sure we won't
    // overwrite any existing message (e.g. if several threads are writing at
    // once)
#ifdef _WIN32
    wchar_t wideFullPath[bufferLength] = {};
    android::base::Win32UnicodeString::convertFromUtf8(wideFullPath,
                                                       bufferLength,
                                                       fullName);
    int fd = _wopen(wideFullPath,
                    _O_WRONLY | _O_CREAT | _O_NOINHERIT |
                    (binary ? _O_BINARY : _O_TEXT) |
                    (replace ? _O_TRUNC : _O_APPEND),
                    _S_IREAD | _S_IWRITE);
#else
    int fd = HANDLE_EINTR(open(fullName,
                  O_WRONLY | O_CREAT | O_CLOEXEC
                  | (replace ? O_TRUNC : O_APPEND), 0644));
#endif
    if (fd < 0) {
        W("Failed to open a temp file '%s' for writing", fullName);
    }
    return base::ScopedFd(fd);
}

static void attachUptime() {
    const uint64_t wallClockTime= System::get()->getProcessTimes().wallClockMs;
    CommonReportedInfo::setUptime(wallClockTime);

    // format the time into a string buffer (no allocations, we've just crashed)
    // and put it both into the file and into the file name. This way
    // it's much easier to see the time in the crash report window
    char timeStr[32] = {};
    snprintf(timeStr, sizeof(timeStr) - 1, "%" PRIu64 "ms", wallClockTime);

    char fileName[sizeof(timeStr) + 16] = {};
    snprintf(fileName, sizeof(fileName) - 1, "uptime-%s.txt", timeStr);

    CrashReporter::get()->attachData(fileName, timeStr);
}

bool CrashReporter::onCrash() {
    // store the uptime first - as Breakpad doesn't do it sometimes
    attachUptime();

    for (const auto& callback : CrashReporter::get()->mCrashCallbacks) {
        callback();
    }

    bool platform_specific_res =
        CrashReporter::get()->onCrashPlatformSpecific();

    // By now, we assumed that structured info has collected every
    // possible piece of information. Record it here.
    {
        // Note that we cannot use attachData as that only works with
        // ASCII and not binary data, especially on Windows.
        CommonReportedInfo::writeHostInfo(&mProtobufData);
        attachBinaryData(kEmulatorHostFileName, mProtobufData, true);
        mProtobufData.clear();

        CommonReportedInfo::writeDetails(&mProtobufData);
        attachBinaryData(kEmulatorDetailsFileName, mProtobufData, true);
        mProtobufData.clear();

        CommonReportedInfo::writePerformanceStats(&mProtobufData);
        attachBinaryData(kEmulatorPerformanceStatsFileName, mProtobufData, true);
        mProtobufData.clear();

    }

    return platform_specific_res;
}

}  // namespace crashreport
}  // namespace android

using android::crashreport::CrashReporter;
using android::crashreport::CrashSystem;

extern "C" {

bool crashhandler_init() {
    if (CrashSystem::CrashType::CRASHUPLOAD == CrashSystem::CrashType::NONE) {
        return false;
    }

    if (!CrashSystem::get()->validatePaths()) {
        return false;
    }

    CrashSystem::CrashPipe crashpipe(CrashSystem::get()->getCrashPipe());

    const std::string procident = CrashSystem::get()->getProcessId();

    if (procident.empty()) {
        return false;
    }

    if (!crashpipe.isValid()) {
        return false;
    }

    std::vector<std::string> cmdline =
            CrashSystem::get()->getCrashServiceCmdLine(crashpipe.mServer,
                                                       procident);

    int pid = CrashSystem::spawnService(cmdline);
    if (pid > 0) {
        CrashReporter::get()->setupChildCrashProcess(pid);
    } else {
        W("Could not spawn crash service\n");
        return false;
    }

    if (!CrashReporter::get()->waitServicePipeReady(crashpipe.mClient)) {
        W("Crash service did not start\n");
        return false;
    }

    return CrashReporter::get()->attachCrashHandler(crashpipe);
}

void crashhandler_die(const char* message) {
    if (const auto reporter = CrashReporter::get()) {
        reporter->GenerateDumpAndDie(message);
        // convince the compiler and everyone else that we will never return
        abort();
    } else {
        I("Emulator: exiting becase of the internal error '%s'\n", message);
        _exit(1);
    }
}

void crashhandler_die_format(const char* format, ...) {
    char message[2048] = {};
    va_list args;

    va_start(args, format);
    vsnprintf(message, sizeof(message) - 1, format, args);
    va_end(args);

    crashhandler_die(message);
}

void crashhandler_add_string(const char* name, const char* string) {
    CrashReporter::get()->attachData(name, string);
}

void crashhandler_exitmode(const char* message) {
    CrashReporter::get()->SetExitMode(message);
}

bool crashhandler_copy_attachment(const char* destination, const char* source) {
    return CrashReporter::get()->attachFile(source, destination);
}

}  // extern "C"
