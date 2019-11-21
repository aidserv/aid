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

#include "android/base/files/PathUtils.h"

#include <gtest/gtest.h>

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#define DUP(x) x, x

namespace android {
namespace base {

static const int kHostTypeCount = PathUtils::kHostTypeCount;

TEST(PathUtils, isDirSeparator) {
    static const struct {
        int ch;
        bool expected[kHostTypeCount];
    } kData[] = {
        { '/', { true, true }},
        { '\\', { false, true }},
        { '$', { false, false }},
        { ':', { false, false }},
        { ';', { false, false }},
    };

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        int ch = kData[n].ch;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isDirSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isDirSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isDirSeparator(ch))
                << "Testing '" << ch << "'";
    }
}

TEST(PathUtils, isPathSeparator) {
    static const struct {
        int ch;
        bool expected[kHostTypeCount];
    } kData[] = {
        { ':', { true, false }},
        { ';', { false, true }},
        { '/', { false, false }},
        { '\\', { false, false }},
        { '$', { false, false }},
    };

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        int ch = kData[n].ch;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isPathSeparator(ch, kHostPosix))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isPathSeparator(ch, kHostWin32))
                << "Testing '" << ch << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isPathSeparator(ch))
                << "Testing '" << ch << "'";
    }
}

TEST(PathUtils, getDirSeparator) {
    EXPECT_EQ('/', PathUtils::getDirSeparator(kHostPosix));
    EXPECT_EQ('\\', PathUtils::getDirSeparator(kHostWin32));
}

TEST(PathUtils, removeTrailingDirSeparator) {
    static const struct {
        const char* path;
        const char* expected[kHostTypeCount];
    } kData[] = {
        { "/", { "/", "/" } },
        { "\\", { "\\", "\\" } },
        { "foo", { "foo", "foo" } },
        { "foo/bar", { "foo/bar", "foo/bar" } },
        { "foo\\bar", { "foo\\bar", "foo\\bar" } },
        { "foo/bar/", { "foo/bar", "foo/bar" } },
        { "foo\\bar\\", { "foo\\bar\\", "foo\\bar" } },
        { "/foo", { "/foo", "/foo" } },
        { "/foo/", { "/foo", "/foo" } },
        { "/foo/bar", { "/foo/bar", "/foo/bar" } },
        { "/foo/bar/", { "/foo/bar", "/foo/bar" } },
        { "\\foo", { "\\foo", "\\foo" } },
        { "\\foo\\bar", { "\\foo\\bar", "\\foo\\bar" } },
        { "\\foo\\bar\\", { "\\foo\\bar\\", "\\foo\\bar" } }
    };

    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        std::string path;
        std::string input = kData[n].path;
        path = PathUtils::removeTrailingDirSeparator(input, kHostPosix);
        EXPECT_STREQ(kData[n].expected[kHostPosix], path.c_str())
                << "Testing '" << input.c_str() << "'";

        path = PathUtils::removeTrailingDirSeparator(input, kHostWin32);
        EXPECT_STREQ(kData[n].expected[kHostWin32], path.c_str())
                << "Testing '" << input.c_str() << "'";

        path = PathUtils::removeTrailingDirSeparator(input);
        EXPECT_STREQ(kData[n].expected[kHostType], path.c_str())
                << "Testing '" << input.c_str() << "'";
    }
}

TEST(PathUtils, rootPrefixSize) {
    static const struct {
        const char* path;
        size_t prefixSize[kHostTypeCount];
    } kData[] = {
        { NULL, { 0u, 0u} },
        { "", { 0u, 0u } },
        { "foo", { 0u, 0u } },
        { "foo/bar", { 0u, 0u } },
        { "/foo", { 1u, 1u } },
        { "//foo", { 1u, 5u } },
        { "//foo/bar", { 1u, 6u } },
        { "c:", { 0u, 2u } },
        { "c:foo", { 0u, 2u } },
        { "c/foo", { 0u, 0u } },
        { "c:/foo", { 0u, 3u } },
        { "c:\\", { 0u, 3u } },
        { "c:\\\\", { 0u, 3u } },
        { "1:/foo", { 0u, 0u } },
        { "\\", { 0u, 1u } },
        { "\\foo", { 0u, 1u } },
        { "\\foo\\bar", { 0u, 1u } },
        { "\\\\foo", { 0u, 5u } },
        { "\\\\foo\\", { 0u, 6u } },
        { "\\\\foo\\\\bar", { 0u, 6u } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        EXPECT_EQ(kData[n].prefixSize[kHostPosix],
                  PathUtils::rootPrefixSize(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].prefixSize[kHostWin32],
                  PathUtils::rootPrefixSize(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].prefixSize[kHostType],
                  PathUtils::rootPrefixSize(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

TEST(PathUtils, isAbsolute) {
    static const struct {
        const char* path;
        bool expected[kHostTypeCount];
    } kData[] = {
        { "foo", { false, false } },
        { "/foo", { true, true } },
        { "\\foo", { false, true } },
        { "/foo/bar", { true, true } },
        { "\\foo\\bar", { false, true } },
        { "C:foo", { false, false } },
        { "C:/foo", { false, true } },
        { "C:\\foo", { false, true } },
        { "//server", { true, false } },
        { "//server/path", { true, true } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        EXPECT_EQ(kData[n].expected[kHostPosix],
                  PathUtils::isAbsolute(path, kHostPosix))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].expected[kHostWin32],
                  PathUtils::isAbsolute(path, kHostWin32))
                << "Testing '" << (path ? path : "<NULL>") << "'";
        EXPECT_EQ(kData[n].expected[kHostType],
                  PathUtils::isAbsolute(path))
                << "Testing '" << (path ? path : "<NULL>") << "'";
    }
}

TEST(PathUtils, split) {
    static const struct {
        const char* path;
        struct {
            bool result;
            const char* dirname;
            const char* basename;
        } expected[kHostTypeCount];
    } kData[] = {
            {".", {{true, ".", "."}, {true, ".", "."}}},
            {"/", {{false, nullptr, nullptr}, {false, nullptr, nullptr}}},
            {"foo", {{true, ".", "foo"}, {true, ".", "foo"}}},
            {"/foo", {{true, "/", "foo"}, {true, "/", "foo"}}},
            {"foo/bar", {{true, "foo/", "bar"}, {true, "foo/", "bar"}}},
            {"foo\\bar", {{true, ".", "foo\\bar"}, {true, "foo\\", "bar"}}},
            {"foo/", {{false, nullptr, nullptr}, {false, nullptr, nullptr}}},
            {"foo\\", {{true, ".", "foo\\"}, {false, nullptr, nullptr}}},
            {"C:foo", {{true, ".", "C:foo"}, {true, "C:", "foo"}}},
            {"C:/foo", {{true, "C:/", "foo"}, {true, "C:/", "foo"}}},
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        StringView dirname, basename;

        EXPECT_EQ(kData[n].expected[kHostPosix].result,
                  PathUtils::split(kData[n].path, kHostPosix, &dirname,
                                   &basename))
                << "when testing posix [" << kData[n].path << "]";
        if (kData[n].expected[kHostPosix].result) {
            EXPECT_EQ(kData[n].expected[kHostPosix].dirname, dirname)
                    << "when testing posix [" << kData[n].path << "]";
            EXPECT_EQ(kData[n].expected[kHostPosix].basename,
                         basename)
                    << "when testing posix [" << kData[n].path << "]";
        }

        EXPECT_EQ(kData[n].expected[kHostWin32].result,
                  PathUtils::split(kData[n].path, kHostWin32, &dirname,
                                   &basename))
                << "when testing win32 [" << kData[n].path << "]";
        if (kData[n].expected[kHostWin32].result) {
            EXPECT_EQ(kData[n].expected[kHostWin32].dirname, dirname)
                    << "when testing win32 [" << kData[n].path << "]";
            EXPECT_EQ(kData[n].expected[kHostWin32].basename,
                         basename)
                    << "when testing win32 [" << kData[n].path << "]";
        }

        EXPECT_EQ(kData[n].expected[kHostType].result,
                  PathUtils::split(kData[n].path, &dirname, &basename))
                << "when testing host [" << kData[n].path << "]";
        if (kData[n].expected[kHostType].result) {
            EXPECT_EQ(kData[n].expected[kHostType].dirname, dirname)
                    << "when testing host [" << kData[n].path << "]";
            EXPECT_EQ(kData[n].expected[kHostType].basename,
                         basename)
                    << "when testing host [" << kData[n].path << "]";
        }
    }
}

TEST(PathUtils, join) {
    static const struct {
        const char* path1;
        const char* path2;
        const char* expected[kHostTypeCount];
    } kData[] = {{".", "foo", {"./foo", ".\\foo"}},
                 {"foo", "/bar", {"/bar", "/bar"}},
                 {"foo", "bar", {"foo/bar", "foo\\bar"}},
                 {"foo/", "bar", {"foo/bar", "foo/bar"}},
                 {"foo\\", "bar", {"foo\\/bar", "foo\\bar"}},
                 {"C:", "foo", {"C:/foo", "C:foo"}},
                 {"C:/", "foo", {"C:/foo", "C:/foo"}},
                 {"C:\\", "foo", {"C:\\/foo", "C:\\foo"}}};
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        std::string path =
                PathUtils::join(kData[n].path1, kData[n].path2, kHostPosix);
        EXPECT_STREQ(kData[n].expected[kHostPosix], path.c_str())
                << "When testing posix [" << kData[n].path1 << ", "
                << kData[n].path2 << "]";

        path = PathUtils::join(kData[n].path1, kData[n].path2, kHostWin32);
        EXPECT_STREQ(kData[n].expected[kHostWin32], path.c_str())
                << "When testing win32 [" << kData[n].path1 << ", "
                << kData[n].path2 << "]";

        path = PathUtils::join(kData[n].path1, kData[n].path2);
        EXPECT_STREQ(kData[n].expected[kHostType], path.c_str())
                << "When testing host [" << kData[n].path1 << ", "
                << kData[n].path2 << "]";
    }
}

static const int kMaxComponents = 10;

typedef const char* ComponentList[kMaxComponents];

static void checkComponents(const ComponentList& expected,
                            const std::vector<StringView>& components,
                            const char* hostType,
                            const char* path) {
    size_t m;
    for (m = 0; m < components.size(); ++m) {
        if (!expected[m])
            break;
        const char* component = expected[m];
        EXPECT_EQ(component, components[m])
                << hostType << " component #" << (m + 1) << " in " << path;
    }
    EXPECT_EQ(m, components.size())
                << hostType << " component #" << (m + 1) << " in " << path;
}

TEST(PathUtils, decompose) {
    static const struct {
        const char* path;
        const ComponentList components[kHostTypeCount];
    } kData[] = {
        { "", { { NULL }, { NULL } } },
        { "foo", {
            { "foo", NULL },
            { "foo", NULL } } },
        { "foo/", {
            { "foo", NULL },
            { "foo", NULL } } },
        { "foo/bar", {
            { "foo", "bar", NULL },
            { "foo", "bar", NULL } } },
        { "foo//bar/zoo", {
            { "foo", "bar", "zoo", NULL },
            { "foo", "bar", "zoo", NULL } } },
        { "\\foo\\bar\\", {
            { "\\foo\\bar\\", NULL },
            { "\\", "foo", "bar", NULL } } },
        { "C:foo\\bar", {
            { "C:foo\\bar", NULL },
            { "C:", "foo", "bar", NULL } } },
        { "C:/foo", {
            { "C:", "foo", NULL },
            { "C:/", "foo", NULL } } },
        { "/foo", {
            { "/", "foo", NULL },
            { "/", "foo", NULL } } },
        { "\\foo", {
            { "\\foo", NULL },
            { "\\", "foo", NULL } } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* path = kData[n].path;
        checkComponents(kData[n].components[kHostPosix],
                        PathUtils::decompose(path, kHostPosix),
                        "posix",
                        path);

        checkComponents(kData[n].components[kHostWin32],
                        PathUtils::decompose(path, kHostWin32),
                        "win32",
                        path);

        checkComponents(kData[n].components[kHostType],
                        PathUtils::decompose(path),
                        "host",
                        path);
    }
}

static std::vector<std::string> componentListToVector(
        const ComponentList& input) {
    std::vector<std::string> result;
    for (size_t i = 0; input[i]; ++i)
        result.push_back(input[i]);
    return result;
}

TEST(PathUtils, recompose) {
    static const struct {
        const ComponentList input;
        const char* path[kHostTypeCount];
    } kData[] = {
        { { NULL }, { "", "" } },
        { { ".", NULL }, { ".", "." } },
        { { "..", NULL }, { "..", ".." } },
        { { "/", NULL }, { "/", "/" } },
        { { "/", "foo", NULL }, { "/foo", "/foo" } },
        { { "\\", "foo", NULL }, { "\\/foo", "\\foo" } },
        { { "foo", NULL }, { "foo", "foo" } },
        { { "foo", "bar", NULL }, { "foo/bar", "foo\\bar" } },
        { { ".", "foo", "..", NULL }, { "./foo/..", ".\\foo\\.." } },
        { { "C:", "foo", NULL }, { "C:/foo", "C:foo" } },
        { { "/foo/bar", "lib", NULL }, { "/foo/bar/lib", "/foo/bar\\lib" } },
        { { "\\foo\\bar", "lib", NULL }, { "\\foo\\bar/lib", "\\foo\\bar\\lib" } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        std::vector<std::string> components =
                componentListToVector(kData[n].input);
        EXPECT_STREQ(kData[n].path[kHostPosix],
                     PathUtils::recompose(components, kHostPosix).c_str());
        EXPECT_STREQ(kData[n].path[kHostWin32],
                     PathUtils::recompose(components, kHostWin32).c_str());
        EXPECT_STREQ(kData[n].path[kHostType],
                     PathUtils::recompose(components).c_str());
    }
}


// Convert a vector of strings |components| into a file path, using
// |separator| as the directory separator.
static std::string componentsToPath(const ComponentList& components,
                                    char separator) {
    std::string result;
    for (size_t n = 0; components[n]; ++n) {
        if (n)
            result += separator;
        result += components[n];
    }
    return result;
}

static std::string stringVectorToPath(const std::vector<std::string>& input,
                                      char separator) {
    std::string result;
    for (size_t n = 0; n < input.size(); ++n) {
        if (n)
            result += separator;
        result += input[n];
    }
    return result;
}

TEST(PathUtils, simplifyComponents) {
    static const struct {
        const ComponentList input;
        const ComponentList expected;
    } kData[] = {
        { { NULL }, { ".", NULL } },
        { { ".", NULL }, { ".", NULL } },
        { { "..", NULL }, { "..", NULL } },
        { { "..", "..", NULL }, { "..", "..", NULL } },
        { { "foo", NULL }, { "foo", NULL } },
        { { "foo", ".", NULL }, { "foo", NULL } },
        { { "foo", "bar", NULL }, { "foo", "bar", NULL } },
        { { ".", "foo", ".", "bar", ".", NULL }, { "foo", "bar", NULL } },
        { { "foo", "..", "bar", NULL }, { "bar", NULL } },
        { { ".", "..", "foo", "bar", NULL }, { "..", "foo", "bar", NULL } },
        { { "..", "foo", "..", "bar", NULL }, { "..", "bar", NULL } },
        { { "foo", "..", "..", "bar", NULL }, { "..", "bar", NULL } },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const ComponentList& input = kData[n].input;
        std::string inputPath = componentsToPath(input, '!');
        std::string expectedPath = componentsToPath(kData[n].expected, '!');
        std::vector<std::string> components = componentListToVector(input);
        PathUtils::simplifyComponents(&components);
        std::string path = stringVectorToPath(components, '!');

        EXPECT_STREQ(expectedPath.c_str(), path.c_str())
            << "When simplifying " << inputPath.c_str();
    }
}

TEST(PathUtils, toExecutableName) {
    static const struct {
        PathUtils::HostType host;
        const char* input;
        const char* expected;
    } kData[] = {
        { PathUtils::HOST_POSIX, "blah", "blah" },
        { PathUtils::HOST_WIN32, "blah", "blah.exe" },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        EXPECT_STREQ(
            kData[n].expected,
            PathUtils::toExecutableName(kData[n].input, kData[n].host).c_str());

        // make sure the overload without host parameter works correctly too
        if (kData[n].host == PathUtils::HOST_TYPE) {
            EXPECT_STREQ(
                PathUtils::toExecutableName(kData[n].input, kData[n].host).c_str(),
                PathUtils::toExecutableName(kData[n].input).c_str());
        }
    }
}

TEST(PathUtils, extension) {
    static const struct {
        PathUtils::HostType host;
        const char* input;
        const char* expected;
    } kData[] = {
        { PathUtils::HOST_POSIX, "", "" },
        { PathUtils::HOST_POSIX, "name.ext", ".ext" },
        { PathUtils::HOST_POSIX, "name.", "." },
        { PathUtils::HOST_POSIX, ".", "." },
        { PathUtils::HOST_POSIX, "dir/path", "" },
        { PathUtils::HOST_POSIX, "dir/path.ext", ".ext" },
        { PathUtils::HOST_POSIX, "dir.ext/path", "" },
        { PathUtils::HOST_POSIX, ".name", ".name" },

        { PathUtils::HOST_WIN32, "", "" },
        { PathUtils::HOST_WIN32, "name.ext", ".ext" },
        { PathUtils::HOST_WIN32, "name.", "." },
        { PathUtils::HOST_WIN32, ".", "." },
        { PathUtils::HOST_WIN32, "dir\\path", "" },
        { PathUtils::HOST_WIN32, "dir\\path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir/path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir\\path.ext", ".ext" },
        { PathUtils::HOST_WIN32, "dir.bad//path", "" },
        { PathUtils::HOST_WIN32, "dir.bad\\path", "" },
        { PathUtils::HOST_WIN32, ".name", ".name" },
        { PathUtils::HOST_WIN32, "c:\\path..ext", ".ext" },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        EXPECT_STREQ(
            kData[n].expected,
            PathUtils::extension(kData[n].input, kData[n].host).c_str());

        // make sure the overload without host parameter works correctly too
        if (kData[n].host == PathUtils::HOST_TYPE) {
            EXPECT_STREQ(
                PathUtils::extension(kData[n].input, kData[n].host).c_str(),
                PathUtils::extension(kData[n].input).c_str());
        }
    }
}

}  // namespace android
}  // namespace base
