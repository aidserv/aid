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

#include "android/console_auth.h"
#include "android/console_auth_internal.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace android {

using base::ScopedFd;

namespace console_auth {

static std::string temp_auth_token_file;
std::string get_temp_auth_token_file() {
    return temp_auth_token_file;
}

TEST(ConsoleAuth, get_auth_token_from_exists) {
    std::string auth_token;

    const char* test_data[] = {
            "test pattern",
            "test pattern\r\n",
            "  test pattern",
            "  test pattern\r\n",
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    TempFile* tf = tempfile_create();
    tempfile_close(tf);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        int fd = HANDLE_EINTR(
                open(tempfile_path(tf), O_CREAT | O_TRUNC | O_WRONLY, 0600));
        EXPECT_NE(-1, fd);
        EXPECT_TRUE(android::writeStringToFile(fd, test_data[i]));
        HANDLE_EINTR(close(fd));

        EXPECT_TRUE(tokenLoadOrCreate(tempfile_path(tf), &auth_token));
        EXPECT_STREQ("test pattern", auth_token.c_str());
        tempfile_close(tf);
    }
}

bool isbase64(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || c == '+' || c == '/' || c == '=';
}

TEST(ConsoleAuth, get_auth_token_from_create) {
    std::string auth_token;

    TempFile* tf = tempfile_create();
    tempfile_close(tf);

    const int kBase64Len = 16;
    EXPECT_TRUE(tokenLoadOrCreate(tempfile_path(tf), &auth_token));
    struct stat stat_info;
    EXPECT_EQ(0, stat(tempfile_path(tf), &stat_info));
    EXPECT_EQ(kBase64Len, stat_info.st_size);
    // all we know about this token is that it will be 16 bytes base64 encoded
    for (int i = 0; i < kBase64Len; i++) {
        EXPECT_TRUE(isbase64(auth_token[i]));
    }
    EXPECT_EQ(static_cast<size_t>(kBase64Len), auth_token.size());

    tempfile_close(tf);
}

TEST(ConsoleAuth, get_auth_token_from_fail) {
    std::string auth_token;
    EXPECT_FALSE(tokenLoadOrCreate("/absolutely/bogus/path", &auth_token));
}

std::string read_file(const std::string& path) {
    ScopedFd fd(HANDLE_EINTR(open(path.c_str(), O_RDONLY)));
    EXPECT_NE(-1, fd.get());
    std::string result;
    EXPECT_TRUE(android::readFileIntoString(fd.get(), &result));
    return result;
}

TEST(ConsoleAuth, android_console_auth_get_status) {
    TempFile* tf = tempfile_create();

    // override default auth token file path
    temp_auth_token_file.assign(tempfile_path(tf));
    g_get_auth_token_path = get_temp_auth_token_file;

    // test with an empty auth token file
    EXPECT_EQ(CONSOLE_AUTH_STATUS_DISABLED, android_console_auth_get_status());

    // test with no auth token file (it should be created)
    tempfile_close(tf);
    EXPECT_EQ(CONSOLE_AUTH_STATUS_REQUIRED, android_console_auth_get_status());

    temp_auth_token_file.assign("/very/bad/path");
    EXPECT_EQ(CONSOLE_AUTH_STATUS_ERROR, android_console_auth_get_status());

    char* dup = android_console_auth_token_path_dup();
    EXPECT_STREQ("/very/bad/path", dup);
    free(dup);
}

}  // namespace console_auth
}  // namespace android
