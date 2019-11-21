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

#include "android/base/system/System.h"

#include <iterator>
#include <numeric>

#include <assert.h>
#include <string.h>

namespace android {
namespace base {

const char* const PathUtils::kExeNameSuffixes[kHostTypeCount] = { "", ".exe" };

const char* const PathUtils::kExeNameSuffix =
        PathUtils::kExeNameSuffixes[PathUtils::HOST_TYPE];

std::string PathUtils::toExecutableName(StringView baseName,
                                        HostType hostType) {
    return static_cast<std::string>(baseName).append(
            kExeNameSuffixes[hostType]);
}

// static
bool PathUtils::isDirSeparator(int ch, HostType hostType) {
    return (ch == '/') || (hostType == HOST_WIN32 && ch == '\\');
}

// static
bool PathUtils::isPathSeparator(int ch, HostType hostType) {
    return (hostType == HOST_POSIX && ch == ':') ||
            (hostType == HOST_WIN32 && ch == ';');
}

// static
size_t PathUtils::rootPrefixSize(StringView path, HostType hostType) {
    if (path.empty())
        return 0;

    if (hostType != HOST_WIN32)
        return (path[0] == '/') ? 1U : 0U;

    size_t result = 0;
    if (path[1] == ':') {
        int ch = path[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
            result = 2U;
    } else if (!strncmp(path.begin(), "\\\\.\\", 4) ||
            !strncmp(path.begin(), "\\\\?\\", 4)) {
        // UNC prefixes.
        return 4U;
    } else if (isDirSeparator(path[0], hostType)) {
        result = 1;
        if (isDirSeparator(path[1], hostType)) {
            result = 2;
            while (path[result] && !isDirSeparator(path[result], HOST_WIN32))
                result++;
        }
    }
    if (result && path[result] && isDirSeparator(path[result], HOST_WIN32))
        result++;

    return result;
}

// static
bool PathUtils::isAbsolute(StringView path, HostType hostType) {
    size_t prefixSize = rootPrefixSize(path, hostType);
    if (!prefixSize) {
        return false;
    }
    if (hostType != HOST_WIN32) {
        return true;
    }
    return isDirSeparator(path[prefixSize - 1], HOST_WIN32);
}

// static
StringView PathUtils::extension(const StringView path, HostType hostType) {
    using riter = std::reverse_iterator<StringView::const_iterator>;

    for (auto it = riter(path.end()), itEnd = riter(path.begin());
         it != itEnd; ++it) {
        if (*it == '.') {
            // reverse iterator stores a base+1, so decrement it when returning
            return StringView(std::prev(it.base()), path.end());
        }
        if (isDirSeparator(*it, hostType)) {
            // no extension here - we've found the end of file name already
            break;
        }
    }

    // either there's no dot in the whole path, or we found directory separator
    // first - anyway, there's no extension in this name
    return StringView();
}

// static
StringView PathUtils::removeTrailingDirSeparator(StringView path,
                                                 HostType hostType) {
    size_t pathLen = path.size();
    // NOTE: Don't remove initial path separator for absolute paths.
    while (pathLen > 1U && isDirSeparator(path[pathLen - 1U], hostType)) {
        pathLen--;
    }
    return StringView(path.begin(), pathLen);
}

// static
std::string PathUtils::addTrailingDirSeparator(StringView path,
                                               HostType hostType) {
    std::string result = path;
    if (result.size() > 0 && !isDirSeparator(result[result.size() - 1U])) {
        result += getDirSeparator(hostType);
    }
    return result;
}

// static
bool PathUtils::split(StringView path,
                      HostType hostType,
                      StringView* dirName,
                      StringView* baseName) {
    if (path.empty()) {
        return false;
    }

    // If there is a trailing directory separator, return an error.
    size_t end = path.size();
    if (isDirSeparator(path[end - 1U], hostType)) {
        return false;
    }

    // Find last separator.
    size_t prefixLen = rootPrefixSize(path, hostType);
    size_t pos = end;
    while (pos > prefixLen && !isDirSeparator(path[pos - 1U], hostType)) {
        pos--;
    }

    // Handle common case.
    if (pos > prefixLen) {
        if (dirName) {
            *dirName = StringView(path.begin(), pos);
        }
        if (baseName) {
            *baseName = StringView(path.begin() + pos, end - pos);
        }
        return true;
    }

    // If there is no directory separator, the path is a single file name.
    if (dirName) {
        if (!prefixLen) {
            *dirName = ".";
        } else {
            *dirName = StringView(path.begin(), prefixLen);
        }
    }
    if (baseName) {
        *baseName = StringView(path.begin() + prefixLen, end - prefixLen);
    }
    return true;
}

// static
std::string PathUtils::join(StringView path1,
                            StringView path2,
                            HostType hostType) {
    if (path1.empty()) {
        return path2;
    }
    if (path2.empty()) {
        return path1;
    }
    if (isAbsolute(path2, hostType)) {
        return path2;
    }
    size_t prefixLen = rootPrefixSize(path1, hostType);
    std::string result(path1);
    size_t end = result.size();
    if (end > prefixLen && !isDirSeparator(result[end - 1U], hostType)) {
        result += getDirSeparator(hostType);
    }
    result += path2;
    return result;
}

// static
template <class String>
std::vector<String> PathUtils::decompose(const String& path,
                                                HostType hostType) {
    std::vector<String> result;
    if (path.empty())
        return result;

    size_t prefixLen = rootPrefixSize(path, hostType);
    auto it = path.begin();
    if (prefixLen) {
        result.emplace_back(it, it + prefixLen);
        it += prefixLen;
    }
    for (;;) {
        auto p = it;
        while (*p && !isDirSeparator(*p, hostType))
            p++;
        if (p > it) {
            result.emplace_back(it, p);
        }
        if (!*p) {
            break;
        }
        it = p + 1;
    }
    return result;
}

std::vector<std::string> PathUtils::decompose(std::string&& path,
                                              HostType hostType) {
    return decompose<std::string>(path, hostType);
}

std::vector<StringView> PathUtils::decompose(StringView path,
                                             HostType hostType) {
    return decompose<StringView>(path, hostType);
}

template <class String>
std::string PathUtils::recompose(const std::vector<String>& components,
                                 HostType hostType) {
    if (components.empty()) {
        return {};
    }

    const char dirSeparator = getDirSeparator(hostType);
    std::string result;
    // To reduce memory allocations, compute capacity before doing the
    // real append.
    const size_t capacity =
            components.size() - 1 +
            std::accumulate(components.begin(), components.end(), size_t(0),
                            [](size_t val, const String& next) {
                                return val + next.size();
                            });

    result.reserve(capacity);
    bool addSeparator = false;
    for (size_t n = 0; n < components.size(); ++n) {
        const auto& component = components[n];
        if (addSeparator)
            result += dirSeparator;
        addSeparator = true;
        if (n == 0) {
            size_t prefixLen = rootPrefixSize(component, hostType);
            if (prefixLen == component.size()) {
                addSeparator = false;
            }
        }
        result += component;
    }
    return result;
}

// static
std::string PathUtils::recompose(const std::vector<std::string>& components,
                                 HostType hostType) {
    return recompose<std::string>(components, hostType);
}

// static
std::string PathUtils::recompose(const std::vector<StringView>& components,
                                 HostType hostType) {
    return recompose<StringView>(components, hostType);
}

// static
template <class String>
void PathUtils::simplifyComponents(std::vector<String>* components) {
    std::vector<String> stack;
    for (auto& component : *components) {
        if (component == StringView(".")) {
            // Ignore any instance of '.' from the list.
            continue;
        }
        if (component == StringView("..")) {
            // Handling of '..' is specific: if there is a item on the
            // stack that is not '..', then remove it, otherwise push
            // the '..'.
            if (!stack.empty() && stack.back() != StringView("..")) {
                stack.pop_back();
            } else {
                stack.push_back(std::move(component));
            }
            continue;
        }
        // If not a '..', just push on the stack.
        stack.push_back(std::move(component));
    }
    if (stack.empty()) {
        stack.push_back(".");
    }
    components->swap(stack);
}

void PathUtils::simplifyComponents(std::vector<std::string>* components) {
    simplifyComponents<std::string>(components);
}

void PathUtils::simplifyComponents(std::vector<StringView>* components) {
    simplifyComponents<StringView>(components);
}

}  // namespace base
}  // namespace android
