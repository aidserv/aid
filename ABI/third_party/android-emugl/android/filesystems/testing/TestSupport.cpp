// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/filesystems/testing/TestSupport.h"

#include "android/base/EintrWrapper.h"
#include "android/base/Log.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace android {
namespace testing {

std::string CreateTempFilePath() {
#ifdef _WIN32
    char tempDir[MAX_PATH];
    DWORD ret = GetTempPath(MAX_PATH, tempDir);
    CHECK(ret > 0 && ret < MAX_PATH)
            << "Could not get temporary directory path";

    std::string result;
    result.resize(MAX_PATH);
    UINT ret2 = GetTempFileName(tempDir, "emulator-test-", 0,  &result[0]);
    CHECK(ret2 != ERROR_BUFFER_OVERFLOW) << "Could not create temporary file name";
    result.resize(::strlen(result.c_str()));
    return result;
#else
    std::string result("/tmp/emulator-test.XXXXXX");
    int ret = HANDLE_EINTR(mkstemp(&result[0]));
    PCHECK(ret >= 0) << "Could not create temporary filepath";
    ::close(ret);
    return result;
#endif
}

}  // namespace testing
}  // namespace android
