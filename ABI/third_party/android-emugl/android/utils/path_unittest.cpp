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

#include "android/utils/path.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include "gtest/gtest.h"

using android::base::TestTempDir;

namespace android {
namespace path {

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#ifdef _WIN32
#define IF_WIN32(x, y)  x
#else
#define IF_WIN32(x, y)  y
#endif

TEST(Path, isAbsolute) {
    const bool isWin32 = IF_WIN32(true, false);
    static const struct {
        const char* path;
        bool expected;
    } kData[] = {
        { "foo", false },
        { "/foo", true },
        { "\\foo", isWin32 },
        { "/foo/bar", true },
        { "\\foo\\bar", isWin32 },
        { "C:foo", false },
        { "C:/foo", isWin32 },
        { "C:\\foo", isWin32 },
        { "//server", !isWin32 },
        { "//server/path", true },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        EXPECT_EQ(kData[n].expected, path_is_absolute(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

TEST(Path, GetAbsolute) {
    static const struct {
        const char* path;
        const char* expected;
    } kData[] = {
        { "foo", IF_WIN32("/home\\foo", "/home/foo") },
        { "/foo", "/foo" },
        { "\\foo", IF_WIN32("\\foo", "/home/\\foo") },
        { "/foo/bar", "/foo/bar" },
        { "\\foo\\bar", IF_WIN32("\\foo\\bar", "/home/\\foo\\bar") },
        { "C:/foo", IF_WIN32("C:/foo", "/home/C:/foo") },
        { "//server/path", "//server/path" },
        // NOTE: Per definition, if |path| is not absolute, prepend the
        // current directory. On Windows, C:foo and //server are not
        // absolute paths, hence the funky results. There is no way to
        // get a correct result otherwise.
        { "C:foo", IF_WIN32("/home\\C:\\foo", "/home/C:foo") },
        { "//server", IF_WIN32("/home\\//server", "//server") },
    };

    android::base::TestSystem testSystem("/bin", 32);
    testSystem.setCurrentDirectoryForTesting("/home");

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        char* result = path_get_absolute(path);
        EXPECT_STREQ(kData[n].expected, result)
                << "Testing '" << (path ? path : "<NULL>") << "'";
        free(result);
    }
}

TEST(Path, EscapePath) {
    const char linuxInputPath[]    = "/Linux/style_with/various,special==character%s";
    const char linuxOutputPath[]   = "/Linux/style_with/various%Cspecial%E%Echaracter%Ps";

    const char windowsInputPath[]  = "C:\\Windows\\style:=with,,other%\\types of characters";
    const char windowsOutputPath[] = "C:\\Windows\\style:%Ewith%C%Cother%P\\types of characters";

    char *result;

    // Escape Linux style paths
    result = path_escape_path(linuxInputPath);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(linuxOutputPath, result);

    path_unescape_path(result);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(linuxInputPath, result);

    free(result);

    // Escape Windows style paths
    result = path_escape_path(windowsInputPath);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(windowsOutputPath, result);

    path_unescape_path(result);
    EXPECT_NE(result, (char*)NULL);
    EXPECT_STREQ(windowsInputPath, result);

    free(result);
}

}  // namespace path
}  // namespace android
