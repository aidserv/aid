// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/exec.h"

#include "android/base/system/Win32UnicodeString.h"

#include <unistd.h>

#ifdef _WIN32
#include <vector>
#endif

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Console control handler has no way of passing data, so let's have
// a global variable here.
static HANDLE sChildProcessHandle = INVALID_HANDLE_VALUE;

static BOOL WINAPI ctrlHandler(DWORD type)
{
    fflush(stdout);
    fflush(stderr);

    if (sChildProcessHandle == INVALID_HANDLE_VALUE) {
        // Just invoke the next handler - this one has nothing to do.
        return FALSE;
    }

    // Windows 7 kills application when the function returns.
    // Sleep here to give QEMU engine a chance for closing.
    // Windows also kills the program after 10 seconds anyway.
    if (::WaitForSingleObject(sChildProcessHandle, 9000) != WAIT_OBJECT_0) {
        ::TerminateProcess(sChildProcessHandle, 100);
    }
    exit(1);

    return TRUE;
}

using android::base::Win32UnicodeString;

int safe_execv(const char* path, char* const* argv) {
   std::vector<Win32UnicodeString> arguments;
   for (size_t i = 0; argv[i] != nullptr; ++i) {
      arguments.push_back(Win32UnicodeString(argv[i]));
   }
   // Do this in two steps since the pointers might change because of push_back
   // in the loop above.
   std::vector<const wchar_t*> argumentPointers;
   for (const auto& arg : arguments) {
      argumentPointers.push_back(arg.c_str());
   }
   argumentPointers.push_back(nullptr);
   Win32UnicodeString program(path);

   ::SetConsoleCtrlHandler(ctrlHandler, TRUE);

   sChildProcessHandle = (HANDLE)_wspawnv(_P_NOWAIT, program.c_str(),
                                  &argumentPointers[0]);
   if (sChildProcessHandle == INVALID_HANDLE_VALUE) {
       ::SetConsoleCtrlHandler(ctrlHandler, FALSE);
       return 1;
   }
   ::WaitForSingleObject(sChildProcessHandle, INFINITE);
   DWORD exitCode;
   if (!::GetExitCodeProcess(sChildProcessHandle, &exitCode)) {
       exitCode = 2;
   }
   exit(exitCode);
}

#else

int safe_execv(const char* path, char* const* argv) {
    return execv(path, argv);
}

#endif
