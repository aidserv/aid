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

#pragma once

#include "android/crashreport/CrashService.h"
#include "client/linux/crash_generation/crash_generation_server.h"

namespace android {
namespace crashreport {

class HostCrashService : public CrashService {
public:
    // Inherit CrashServer constructor
    using CrashService::CrashService;

    virtual ~HostCrashService();

    static void OnClientDumpRequest(
            void* context,
            const ::google_breakpad::ClientInfo* client_info,
            const ::std::string* file_path);

    static void OnClientExit(void* context,
                             const ::google_breakpad::ClientInfo* client_info);

    virtual bool startCrashServer(const std::string& pipe) override;

    virtual bool stopCrashServer() override;

    virtual bool isClientAlive() override;

protected:
    virtual bool getHWInfo() override;
    virtual bool getMemInfo() override;

private:

    std::unique_ptr<::google_breakpad::CrashGenerationServer> mCrashServer;
};

}  // namespace crashreport
}  // namespace android
