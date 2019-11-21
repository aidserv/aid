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

#include "android/base/Compiler.h"

#include <string>
#include <vector>

namespace android {
namespace crashreport {
// Singleton class constructs paths and provides crash related utility
//   functions
// Result from validatePaths will determine whether crash reporting
//   can be enabled on the system
// Paths checked:
//   Checks if crash dump directory exists, if not tries to create
//   Checks if caBundlePath for libcurl exists
//   checks if crash-service binary exists
class CrashSystem {
public:
    CrashSystem()
        : mCaBundlePath(), mCrashServicePath(), mProcessId(), mCrashPipe() {}

    virtual ~CrashSystem() {}

    enum class CrashType { PROD, STAGING, NONE };

    struct CrashPipe {
        CrashPipe() : mServer(), mClient(), mValid(false) {}
        CrashPipe(const char* name)
            : mServer(name), mClient(name), mValid(true) {}
        CrashPipe(const std::string& name)
            : mServer(name), mClient(name), mValid(true) {}
        CrashPipe(int server_fd, int client_fd)
            : mServer(std::to_string(server_fd)),
              mClient(std::to_string(client_fd)),
              mValid(true) {}
        CrashPipe(int fd)
            : mServer(std::to_string(fd)),
              mClient(std::to_string(fd)),
              mValid(true) {}
        bool isValid() const { return mValid; }
        std::string mServer;
        std::string mClient;

    private:
        bool mValid;
    };

    // Return predefined location of crash dump directory
    virtual const std::string& getCrashDirectory() = 0;

    // Returns URL for crash server
    virtual const std::string& getCrashURL() = 0;

    // Returns predefined path to bundle required for curl init
    virtual const std::string& getCaBundlePath();

    // Returns predefined path to crash-service
    virtual const std::string& getCrashServicePath();

    // Returns a string representation of the process used for passing between
    // processes
    virtual const std::string& getProcessId();

    // Returns pipe struct for crash reporting
    virtual const CrashPipe& getCrashPipe();

    // Return crash command line to execute crash service in
    // std::vector<std::string> format.
    // Takes a pipe identfier and a current proc identifier
    virtual std::vector<std::string> getCrashServiceCmdLine(
            const std::string& pipe,
            const std::string& proc);

    // Returns true if all required paths to directories, executables, files
    // are valid.
    virtual bool validatePaths();

    // Returns instance pointer to global structure
    static CrashSystem* get();

    // Returns true if path is a valid dump file
    // Takes path to crash dump as argument
    static bool isDump(const std::string& str);

    static int spawnService(const std::vector<std::string>& commandLine);

protected:
    static CrashSystem* setForTesting(CrashSystem* crashsystem);

private:
    DISALLOW_COPY_AND_ASSIGN(CrashSystem);
    std::string mCaBundlePath;
    std::string mCrashServicePath;
    std::string mProcessId;
    CrashPipe mCrashPipe;
};
}  // namespace crashreport
}  // namespace android
