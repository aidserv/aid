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

#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"

#include <memory>
#include <set>
#include <string>

namespace android {
namespace crashreport {

// The Suggestion type represents possible suggestions
// presented to the user upon
// analyzing a crash report.
enum class Suggestion {
    // TODO: Add more suggestion types
    // (Update OS, Get more RAM, etc)
    // as we find them
    UpdateGfxDrivers
};

// Class UserSuggestions parses and generates user suggestions.

class UserSuggestions {
public:
    std::set<Suggestion> suggestions;

    UserSuggestions(google_breakpad::ProcessState* process_state);
};

// Class CrashService wraps Breakpad platform specific crash generation server
//
// Usage:
//    Instantiate with a version and build number to be used as default report
//    version
//
//    For manual processing:
//        setDumpFile
//    Otherwise:
//        startCrashServer
//        waitForDumpFile
//        stopCrashServer
//
//    Process the crash for details:
//        processCrash
//
//    Finally, to upload the crash:
//        uploadCrash
//
//    After uploading, the report id is populated:
//        getReportId
//
class CrashService {
public:
    // Context state used by breakpad crash server dump request callbacks
    struct DumpRequestContext {
        std::string file_path;
    };

    // Server state used by breakpad crash server connect / exit callbacks
    struct ServerState {
        bool waiting;
        int connected;
    };

    // version and build args will be used to construct breakpad version
    // identifier
    CrashService(const std::string& version,
                 const std::string& build,
                 const char* dataDir);

    virtual ~CrashService();

    // Save a dumpfile path
    void setDumpFile(const std::string& dumpfile);

    // Get saved dumpfile path
    std::string getDumpFile() const;

    // Check if saved dumpfile path is valid
    bool validDumpFile() const;

    // Wait for client to crashdump or exit or timeout in ms. |timeout| of -1
    // for no timeout.
    // Returns -1 if parent dies unexpectedly or timeout
    // Return ms time taken to receive crashdump otherwise
    int64_t waitForDumpFile(int clientpid, int timeout_ms = -1);

    // Platform specific start of the crash server
    // Returns false if already started or not successful
    virtual bool startCrashServer(const std::string& pipe) = 0;

    // Platform specific stop of the crash server
    // Returns false if not started
    virtual bool stopCrashServer() = 0;

    UserSuggestions getSuggestions();

    // Processes the saved dumpfile with breakpad processor libraries
    // Contains stack trace and loaded modules.
    // Is parsed for suggestions on debugging
    // (driver update etc)
    // Return true on success
    bool processCrash();

    // Collects system info, resulting in the "full" dump file.
    bool collectSysInfo();

    // Utility function for reading a txt file into string
    static std::string readFile(android::base::StringView path);

    // Return string containing collectSysInfo result
    std::string getSysInfo();

    // Uploads the saved dumpfile to server defined by CrashSystem
    bool uploadCrash();

    // Get the processed crash report
    std::string getReport();

    // Get the crash report ID returned by the crash servers
    std::string getReportId() const;

    // Returns the dump message passed by the emulator (or empty string if
    // there's none)
    const std::string& getCustomDumpMessage() const;
    const std::string& getCrashOnExitMessage() const;

    // Tells us whether or not this crash was an exit crash.
    bool didCrashOnExit() const;

    // Factory method
    static std::unique_ptr<CrashService> makeCrashService(
            const std::string& version,
            const std::string& build,
            const char* dataDir);

    // Key value pair to be added to crash report
    void addReportValue(const std::string& key, const std::string& value);

    // Key filepath pair to be added to crash report
    void addReportFile(const std::string& key, const std::string& path);

    // User comments to be added to crash report
    void addUserComments(const std::string& comments);

    // Read the dump message passed by the watched/dumped application
    void retrieveDumpMessage();

    // Collects the process list, default implementation runs the POSIX ps
    // command
    virtual void collectProcessList();

    // Changes the behavior on exit - should the object delete all associated
    // crash data from the disk?
    void shouldDeleteCrashDataOnExit(bool deleteData = true) {
        mDeleteCrashDataOnExit = deleteData;
    }

protected:
    // Initialize serverstate
    void initCrashServer();

    const std::string& getDataDirectory() const;

    // Platform specific set instance client
    // Returns false if invalid pid
    virtual bool setClient(int clientpid);

    // Platform specific check if instance client is still running
    // Returns false if client not set or not running
    virtual bool isClientAlive() = 0;

    // Uploads system-specific hardware information.
    virtual bool getHWInfo() = 0;

    // Uploads system-specific memory information.
    virtual bool getMemInfo() = 0;

    DumpRequestContext mDumpRequestContext;
    ServerState mServerState;

    int mClientPID = 0;
    std::string mDataDirectory;

    static const char* const kHwInfoName;
    static const char* const kMemInfoName;

private:
    CrashService();

    // Read the data files passed by the watched/dumped application
    void collectDataFiles();

    std::string mVersion;
    std::string mBuild;
    std::string mVersionId;
    std::string mDumpFile;
    std::string mReportId;
    std::string mComments;
    std::string mCustomDumpMessage;
    std::string mCrashOnExitMessage;
    std::map<std::string, std::string> mReportValues;
    std::map<std::string, std::string> mReportFiles;
    google_breakpad::ProcessState mProcessState;
    google_breakpad::BasicSourceLineResolver mLineResolver;
    std::unique_ptr<google_breakpad::Minidump> mMinidump;

    bool mDidCrashOnExit;
    bool mDeleteCrashDataOnExit = true;
};

}  // namespace crashreport
}  // namespace android
