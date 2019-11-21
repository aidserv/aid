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

#include "android/base/memory/LazyInstance.h"
#include "android/crashreport/crash-handler.h"
#include "android/utils/debug.h"
#include "client/linux/handler/exception_handler.h"

#include <memory>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

// Taken from breakpad to allow child based crash server out of process crash
// handling. See https://breakpad.appspot.com/166001/show
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

namespace android {
namespace crashreport {

namespace {

class HostCrashReporter : public CrashReporter {
public:
    HostCrashReporter() : CrashReporter(), mHandler() {}

    virtual ~HostCrashReporter() {}

    bool attachCrashHandler(const CrashSystem::CrashPipe& crashpipe) override {
        if (mHandler) {
            return false;
        }

        mHandler.reset(new google_breakpad::ExceptionHandler(
                google_breakpad::MinidumpDescriptor(getDumpDir()),
                &HostCrashReporter::exceptionFilterCallback,
                nullptr, nullptr, true, std::stoi(crashpipe.mClient)));

        return mHandler != nullptr;
    }

    bool waitServicePipeReady(const std::string& pipename,
                              int timeout_ms) override {
        return true;
    }

    void setupChildCrashProcess(int pid) override {
        // Taken from breakpad to allow child based crash server out of process
        // crash handling. See https://breakpad.appspot.com/166001/show
        sys_prctl(PR_SET_PTRACER, pid, 0, 0, 0);
    }

    void writeDump() override { mHandler->WriteMinidump(); }

    static bool exceptionFilterCallback(void* context);

private:
    bool onCrashPlatformSpecific() override;

    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler;
};

::android::base::LazyInstance<HostCrashReporter> sCrashReporter =
        LAZY_INSTANCE_INIT;

bool HostCrashReporter::exceptionFilterCallback(void*) {
    return CrashReporter::get()->onCrash();
}

bool HostCrashReporter::onCrashPlatformSpecific() {
    CommonReportedInfo::appendMemoryUsage();
    // collect the memory usage at the time of the crash
    crashhandler_copy_attachment(
                CrashReporter::kProcessMemoryInfoFileName, "/proc/self/status");
    return true;
}

}  // namespace anonymous

CrashReporter* CrashReporter::get() {
    return sCrashReporter.ptr();
}

}  // namespace crashreport
}  // namespace android
