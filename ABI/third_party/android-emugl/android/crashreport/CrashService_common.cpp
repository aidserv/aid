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

#ifdef _WIN32
// To avoid a compiler warning that asks to include this before <windows.h>
// which is included by <curl/curl.h>. This must appear before other includes.
#include <winsock2.h>
#endif

#include "android/crashreport/CrashService.h"

#include "android/crashreport/CrashReporter.h"

#ifdef _WIN32
#include "android/crashreport/CrashService_windows.h"
#elif defined(__APPLE__)
#include "android/crashreport/CrashService_darwin.h"
#elif defined(__linux__)
#include "android/crashreport/CrashService_linux.h"
#else
#error "Unsupported platform"
#endif

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashSystem.h"
#include "android/curl-support.h"
#include "android/utils/debug.h"
#include "google_breakpad/processor/fast_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/stackwalk_common.h"
#include "processor/pathname_stripper.h"

#include <curl/curl.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define WAIT_INTERVAL_MS 100

using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

const char kNameKey[] = "prod";
const char kVersionKey[] = "ver";
const char kName[] = "AndroidEmulator";
const char kCommentsKey[] = "comments";

// Callback to get the response data from server.
static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    if (!userp)
        return 0;

    auto& response = *static_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response.append(static_cast<char*>(ptr), real_size);
    return real_size;
}

namespace android {
namespace crashreport {

const char* const CrashService::kHwInfoName = "hw_info.txt";
const char* const CrashService::kMemInfoName = "mem_info.txt";

CrashService::CrashService(const std::string& version,
                           const std::string& build,
                           const char* dataDir)
    : mDumpRequestContext(),
      mServerState(),
      mDataDirectory(dataDir ? dataDir : ""),
      mVersion(version),
      mBuild(build),
      mVersionId(),
      mDumpFile(),
      mReportId(),
      mComments(),
      mDidCrashOnExit(false) {
    mVersionId = version;
    mVersionId += "-";
    mVersionId += build;
};

CrashService::~CrashService() {
    if (!mDeleteCrashDataOnExit) {
        return;
    }

    if (validDumpFile()) {
        remove(mDumpFile.c_str());
    }
    if (!mDataDirectory.empty()) {
        const auto files = System::get()->scanDirEntries(mDataDirectory, true);
        for (const std::string& file : files) {
            remove(file.c_str());
        }
        if (remove(mDataDirectory.c_str()) < 0) {
            // on Windows waiting a bit usually helps
            System::get()->sleepMs(1);
            remove(mDataDirectory.c_str());
        }
    }
}

void CrashService::setDumpFile(const std::string& dumpfile) {
    mDumpFile = dumpfile;
}

std::string CrashService::getDumpFile() const {
    return mDumpFile;
}

bool CrashService::validDumpFile() const {
    return ::android::crashreport::CrashSystem::get()->isDump(mDumpFile);
}

std::string CrashService::getReport() {
    // Capture printf's from PrintProcessState to file
    std::string reportFile(mDumpFile);
    reportFile += "report";

    FILE* fp = fopen(reportFile.c_str(), "w+");
    if (!fp) {
        std::string errmsg;
        errmsg += "Error, Couldn't open " + reportFile +
                  "for writing crash report.";
        errmsg += "\n";
        return errmsg;
    }
    google_breakpad::SetPrintStream(fp);
    google_breakpad::PrintProcessState(mProcessState, true, &mLineResolver);

    fprintf(fp, "thread requested=%d\n", mProcessState.requesting_thread());
    fclose(fp);

    std::string report(readFile(reportFile));
    remove(reportFile.c_str());

    return report;
}

std::string CrashService::getReportId() const {
    return mReportId;
}

const std::string& CrashService::getCustomDumpMessage() const {
    return mCustomDumpMessage;
}

const std::string& CrashService::getCrashOnExitMessage() const {
    return mCrashOnExitMessage;
}

bool CrashService::didCrashOnExit() const {
    return mDidCrashOnExit;
}

void CrashService::addReportValue(const std::string& key,
                                  const std::string& value) {
    mReportValues[key] = value;
}

void CrashService::addReportFile(const std::string& key,
                                 const std::string& path) {
    mReportFiles[key] = path;
}

void CrashService::addUserComments(const std::string& comments) {
    addReportValue(kCommentsKey, comments);
}

bool CrashService::uploadCrash() {
    curl_init(::android::crashreport::CrashSystem::get()
                      ->getCaBundlePath().c_str());
    char* error = nullptr;
    CURL* const curl = curl_easy_default_init(&error);
    if (!curl) {
        E("Curl instantiation failed: %s\n", error);
        ::free(error);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     CrashSystem::get()->getCrashURL().c_str());

    curl_httppost* formpost = nullptr;
    curl_httppost* lastptr = nullptr;

    addReportValue(kNameKey, kName);
    addReportValue(kVersionKey, mVersionId);
    addReportFile("upload_file_minidump", mDumpFile);

    for (auto const& x : mReportValues) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, x.first.c_str(),
                     CURLFORM_COPYCONTENTS, x.second.c_str(), CURLFORM_END);
    }

    for (auto const& x : mReportFiles) {
        curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, x.first.c_str(),
                     CURLFORM_FILE, x.second.c_str(), CURLFORM_END);
    }

    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    // The 4th pointer to be passed to the WRITEFUNCTION above
    std::string http_response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     static_cast<void*>(&http_response_data));

    bool success = true;
    CURLcode curlRes = curl_easy_perform(curl);

    if (curlRes != CURLE_OK) {
        success = false;
        E("curl_easy_perform() failed with code %d (%s)", curlRes,
          curl_easy_strerror(curlRes));
    } else {
        long http_response = 0;
        curlRes =
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
        if (curlRes == CURLE_OK) {
            if (http_response != 200) {
                E("Got HTTP response code %ld", http_response);
                success = false;
            }
        } else if (curlRes == CURLE_UNKNOWN_OPTION) {
            E("Can not get a valid response code: not supported");
            success = false;
        } else {
            E("Unexpected error while checking http response: %s",
              curl_easy_strerror(curlRes));
            success = false;
        }

        if (success) {
            I("Crash Report ID: %s\n", http_response_data.c_str());
            mReportId = http_response_data;
        }
    }

    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    return success;
}

UserSuggestions CrashService::getSuggestions() {
    return UserSuggestions(&mProcessState);
}

bool CrashService::processCrash() {
    mMinidump.reset(new google_breakpad::Minidump(mDumpFile));
    google_breakpad::MinidumpProcessor minidump_processor(nullptr,
                                                          &mLineResolver);

    // Process the minidump.
    if (!mMinidump->Read()) {
        E("Minidump %s could not be read", mMinidump->path().c_str());
        return false;
    }
    if (minidump_processor.Process(mMinidump.get(), &mProcessState) !=
        google_breakpad::PROCESS_OK) {
        E("MinidumpProcessor::Process failed");
        return false;
    }
    return true;
}

bool CrashService::collectSysInfo() {
    // As long as one of these succeed we consider it a success, it's better to
    // upload a partial report if we have something.
    bool success = getHWInfo();
    success |= getMemInfo();

    // Now that we have all the system information as well as any data the
    // emulator placed in the data directory we can collect the files and add
    // them to the list of files in the report.
    collectDataFiles();

    return success;
}

std::unique_ptr<CrashService> CrashService::makeCrashService(
        const std::string& version,
        const std::string& build,
        const char* dataDir) {
    return std::unique_ptr<CrashService>(
            new HostCrashService(version, build, dataDir));
}

std::string CrashService::readFile(StringView path) {
    std::ifstream is(path.c_str());

    if (!is) {
        std::string errmsg;
        errmsg = std::string("Error, Can't open file ") + path.c_str() +
                 " for reading. Errno=" + std::to_string(errno) + "\n";
        return errmsg;
    }

    std::ostringstream ss;
    ss << is.rdbuf();
    return ss.str();
}

std::string CrashService::getSysInfo() {
    const auto files = System::get()->scanDirEntries(mDataDirectory, true);
    std::string info;
    for (const std::string& file : files) {
        // By convention we only show files that end with .txt, other files
        // may have binary content that will not display in a readable way.
        if (strcasecmp(PathUtils::extension(file).c_str(), ".txt") == 0) {
            // Prepend each piece with the name of the file as a separator
            StringView basename;
            if (PathUtils::split(file, nullptr, &basename)) {
                info += "---- ";
                info += basename;
                info += " ----\n";
            }
            info += readFile(file);
            info += "\n";
        }
    }
    return info;
}

void CrashService::initCrashServer() {
    mServerState.waiting = true;
    mServerState.connected = 0;
}

const std::string& CrashService::getDataDirectory() const {
    return mDataDirectory;
}

int64_t CrashService::waitForDumpFile(int clientpid, int timeout) {
    int64_t waitduration_ms = 0;
    if (!setClient(clientpid)) {
        return -1;
    }
    while (true) {
        if (!isClientAlive()) {
            if ((!mServerState.waiting) && (mServerState.connected == 0)) {
                break;
            } else {
                E("CrashService client died unexpectedly\n");
                return -1;
            }
        }
        if (timeout >= 0 && waitduration_ms >= timeout) {
            return -1;
        }
        System::get()->sleepMs(WAIT_INTERVAL_MS);
        waitduration_ms += WAIT_INTERVAL_MS;
    }
    if (!mDumpRequestContext.file_path.empty()) {
        mDumpFile = mDumpRequestContext.file_path;
    }
    return waitduration_ms;
}

bool CrashService::setClient(int clientpid) {
    // return false for invalid pid
    if (clientpid <= 0) {
        return false;
    }
    mClientPID = clientpid;
    return true;
}

void CrashService::collectProcessList()
{
    if (mDataDirectory.empty()) {
        return;
    }

    const auto command = StringFormat(
                             "ps aux >%s/%s",
                             mDataDirectory,
                             CrashReporter::kProcessListFileName);
    system(command.c_str());
}

void CrashService::retrieveDumpMessage() {
    std::string path = PathUtils::join(mDataDirectory,
                                       CrashReporter::kDumpMessageFileName);
    if (System::get()->pathIsFile(path)) {
        // remember the dump message to show it instead of the default one
        mCustomDumpMessage = readFile(path);
    }
}

void CrashService::collectDataFiles() {
    if (mDataDirectory.empty()) {
        return;
    }

    const auto files = System::get()->scanDirEntries(mDataDirectory);
    for (const std::string& name : files) {
        const auto fullName = PathUtils::join(mDataDirectory, name);
        addReportFile(name.c_str(), fullName.c_str());
        if (name == CrashReporter::kDumpMessageFileName) {
            // remember the dump message to show it instead of the default one
            mCustomDumpMessage = readFile(fullName);
        } else if (name == CrashReporter::kCrashOnExitFileName) {
            // remember the dump message to show it instead of the default one
            mCrashOnExitMessage = readFile(fullName);
            mDidCrashOnExit = true;
        }
    }
}

// This is a list of all substrings that definitely belong to some graphics
// drivers.
// NB: all names have to be lower-case - we transform the string from the stack
// trace to lowe case before doing the comparisons

static const char* const gfx_drivers_lcase[] = {
    // NVIDIA
    "nvcpl", // Control Panel
    "nvshell", // Desktop Explorer
    "nvinit", // initializer
    "nv4_disp", // Windows 2000 (!) display driver
    "nvcod", // CoInstaller
    "nvcuda", // Cuda
    "nvopencl", // OpenCL
    "nvcuvid", // Video decode
    "nvogl", // OpenGL (modern)
    "ig4icd", // OpenGL (icd?)
    "geforcegldriver", // OpenGL (old)
    "nvd3d", // Direct3D
    "nvwgf2", // D3D10
    "nvdx", // D3D shim drivers
    "nvml", // management library
    "nvfbc", // front buffer capture library
    "nvapi", // NVAPI Library
    "libnvidia", // NVIDIA Linux

    // ATI
    "atioglxx", // OpenGL
    "atig6txx", // OpenGL
    "r600", // Radeon r600 series
    "aticfx", // DX11
    "atiumd", // D3D
    "atidxx", // "TMM Com Clone Control Module" (???)
    "atimpenc", // video encoder
    "atigl", // another ATI OpenGL driver

    // Intel
    "i915", // Intel i915 gpu
    "igd", // 'igd' series of Intel GPU's
    "igl", // ?
    "igfx", // ?
    "ig75icd", // Intel icd
    "intelocl", // Intel OpenCL

    // Others
    "libgl.", // Low-level Linux OpenGL
    "opengl32.dll", // Low-level Windows OpenGL
};

static bool containsGfxPattern(const std::string& str) {
    for (const char* pattern : gfx_drivers_lcase) {
        if (str.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

UserSuggestions::UserSuggestions(google_breakpad::ProcessState* process_state) {
    suggestions.clear();

    // Go through all frames of the stack of
    // the thread requesting dump.

    int crashed_thread_id = process_state->requesting_thread();
    if (crashed_thread_id < 0 ||
        crashed_thread_id >= (int)process_state->threads()->size()) {
        return;
    }
    google_breakpad::CallStack* crashed_stack =
            process_state->threads()->at(crashed_thread_id);
    if (!crashed_stack) {
        return;
    }

    const int frame_count = crashed_stack->frames()->size();
    for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
        google_breakpad::StackFrame* const frame =
                crashed_stack->frames()->at(frame_index);
        if (!frame) {
            continue;
        }
        const auto& module = frame->module;

        if (module) {
            std::string file = google_breakpad::PathnameStripper::File(
                    module->code_file());
            std::transform(file.begin(), file.end(), file.begin(), ::tolower);

            // Should the user update their graphics drivers?
            if (containsGfxPattern(file)) {
                suggestions.insert(Suggestion::UpdateGfxDrivers);
                break;
            }
        }
    }
}

}  // namespace crashreport
}  // namespace android
