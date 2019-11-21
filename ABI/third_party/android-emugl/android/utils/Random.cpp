// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/Random.h"

#include "android/base/files/ScopedStdioFile.h"
#include "android/utils/debug.h"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

#define E(...) derror(__VA_ARGS__)

using android::base::ScopedStdioFile;

namespace android {

bool generateRandomBytes(char* buf, size_t buf_len) {
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    DWORD dwFlags = CRYPT_VERIFYCONTEXT | CRYPT_SILENT;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, dwFlags)) {
        if (GetLastError() == (DWORD)NTE_BAD_KEYSET) {
            if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
                                     dwFlags | CRYPT_NEWKEYSET)) {
                return false;
            }
        }
    }

    BOOL ok = CryptGenRandom(hCryptProv, buf_len, (BYTE*)buf);

    CryptReleaseContext(hCryptProv, 0);

    if (!ok) {
        E("Can't read from CryptGenRandom");
        return false;
    }
    return true;
#else
    ScopedStdioFile fp(fopen("/dev/urandom", "rb"));
    if (fp.get() == nullptr) {
        E("Can't open /dev/urandom");
        return false;
    }
    size_t err = fread(buf, 1, buf_len, fp.get());
    if (err != buf_len) {
        E("Can't read from /dev/urandom");
        return false;
    }
    return true;
#endif
}

}  // namespace android
