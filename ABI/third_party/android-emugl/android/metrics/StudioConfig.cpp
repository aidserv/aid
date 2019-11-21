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

#include "android/metrics/StudioConfig.h"

#include "android/base/ArraySize.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Optional.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/Version.h"
#include "android/emulation/ConfigDirs.h"
#include "android/metrics/MetricsPaths.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"

#include <libxml/tree.h>

#include <algorithm>
#include <iterator>
#include <fstream>

#include <stdlib.h>
#include <string.h>

using android::base::LazyInstance;
using android::base::Optional;
using std::string;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

/***************************************************************************/
// These consts are replicated as macros in StudioHelper_unittest.cpp
// changes to these will require equivalent changes to the unittests
//
#ifdef __APPLE__
static const char kAndroidStudioDir[] = "AndroidStudio";
#else   // ! ___APPLE__
static const char kAndroidStudioDir[] = ".AndroidStudio";
static const char kAndroidStudioDirInfix[] = "config";
#endif  // __APPLE__

static const char kAndroidStudioDirSuffix[] = "options";
static const char kAndroidStudioDirPreview[] = "Preview";

/***************************************************************************/

namespace {
// StudioXML describes the XML parameters we are seeking for in a
// Studio preferences file. StudioXml structs are defined statically
// in functions that call parseStudioXML().
struct StudioXml {
    const char* filename;
    const char* nodename;
    const char* propname;
    const char* propvalue;
    const char* keyname;
};
}  // namespace

using android::base::PathUtils;
using android::base::strDup;
using android::base::StringView;
using android::base::System;
using android::base::Version;
using android::base::stringLiteralLength;
using android::ConfigDirs;
using android::studio::UpdateChannel;

namespace android {
namespace studio {

// Find latest studio preferences directory and return the
// value of the xml entry described in |match|.
static Optional<string> parseStudioXML(const StudioXml* const match);

Version extractAndroidStudioVersion(const char* const dirName) {
    if (dirName == NULL) {
        return Version::invalid();
    }

    // get rid of kAndroidStudioDir prefix to get to version
    if (strncmp(dirName, kAndroidStudioDir,
                stringLiteralLength(kAndroidStudioDir)) != 0) {
        return Version::invalid();
    }

    const char* cVersion = dirName + stringLiteralLength(kAndroidStudioDir);

    // if this is a preview, get rid of kAndroidStudioDirPreview
    // prefix too and mark preview as build #1
    // (assume build #2 for releases)
    auto build = 2;
    if (strncmp(cVersion, kAndroidStudioDirPreview,
                stringLiteralLength(kAndroidStudioDirPreview)) == 0) {
        cVersion += stringLiteralLength(kAndroidStudioDirPreview);
        build = 1;
    }

    // cVersion should now contain at least a number; if not,
    // this is a very early AndroidStudio installation, let's
    // call it version 0
    if (cVersion[0] == '\0') {
        cVersion = "0";
    }

    // Create a major.micro.minor version string and append
    // a "2" for release and a "1" for previews; this will
    // allow proper sorting
    Version rawVersion(cVersion);
    if (rawVersion == Version::invalid()) {
        return Version::invalid();
    }

    return Version(rawVersion.component<Version::kMajor>(),
                   rawVersion.component<Version::kMinor>(),
                   rawVersion.component<Version::kMicro>(), build);
}

std::string latestAndroidStudioDir(const std::string& scanPath) {
    if (scanPath.empty()) {
        return {};
    }

    const auto scanner = android::base::makeCustomScopedPtr(
            dirScanner_new(scanPath.c_str()), dirScanner_free);
    if (!scanner) {
        return {};
    }

    std::string latest_path;
    System* system = System::get();
    Version latest_version(0, 0, 0);

    for (;;) {
        const char* full_path = dirScanner_nextFull(scanner.get());
        if (full_path == NULL) {
            // End of enumeration.
            break;
        }
        // ignore files, only interested in subDirs
        if (!system->pathIsDir(full_path)) {
            continue;
        }

        char* name = path_basename(full_path);
        if (name == NULL) {
            continue;
        }
        Version v = extractAndroidStudioVersion(name);
        free(name);
        if (v.isValid() && latest_version < v) {
            latest_version = v;
            latest_path = full_path;
        }
    }

    return latest_path;
}

std::string pathToStudioXML(const std::string& studioPath,
                            const std::string& filename) {
    if (studioPath.empty() || filename.empty())
        return std::string();

    // build /path/to/.AndroidStudio/subpath/to/file.xml
    std::vector<std::string> vpath;
    vpath.push_back(studioPath);
#ifndef __APPLE__
    vpath.push_back(std::string(kAndroidStudioDirInfix));
#endif  // !__APPLE__
    vpath.push_back(std::string(kAndroidStudioDirSuffix));
    vpath.push_back(filename);
    return PathUtils::recompose(vpath);
}

UpdateChannel updateChannel() {
    static const StudioXml channelInfo = {
            .filename = "updates.xml",
            .nodename = "option",
            .propname = "name",
            .propvalue = "UPDATE_CHANNEL_TYPE",
            .keyname = "value",
    };
    const auto channelStr = parseStudioXML(&channelInfo);
    if (!channelStr) {
        return UpdateChannel::Unknown;
    }

    static const struct NamedChannel {
        StringView name;
        UpdateChannel channel;
    } channelNames[] = {
            {"", UpdateChannel::Canary},  // this is the current default
            {"eap", UpdateChannel::Canary},
            {"release", UpdateChannel::Stable},
            {"beta", UpdateChannel::Beta},
            {"milestone", UpdateChannel::Dev}};

    const auto channelIt =
            std::find_if(std::begin(channelNames), std::end(channelNames),
                         [&channelStr](const NamedChannel& channel) {
                             return channel.name == *channelStr;
                         });
    if (channelIt != std::end(channelNames)) {
        return channelIt->channel;
    }
    return UpdateChannel::Unknown;
}

/*****************************************************************************/

// Recurse through studio xml doc and return the value described in match
// as a string, if one is set. Otherwise, return an empty string
//
static std::string eval_studio_config_xml(xmlDocPtr doc,
                                          xmlNodePtr root,
                                          const StudioXml* const match) {
    xmlNodePtr current = NULL;
    xmlChar* xmlval = NULL;
    std::string retVal;

    for (current = root; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(current->name, BAD_CAST match->nodename))) {
                xmlChar* propvalue =
                        xmlGetProp(current, BAD_CAST match->propname);
                int nomatch = xmlStrcmp(propvalue, BAD_CAST match->propvalue);
                xmlFree(propvalue);
                if (!nomatch) {
                    xmlval = xmlGetProp(current, BAD_CAST match->keyname);
                    if (xmlval != NULL) {
                        // xmlChar* is defined as unsigned char
                        // we are simply reading the result and don't
                        // operate on it, soe simply reinterpret as
                        // as char * array
                        retVal = std::string(reinterpret_cast<char*>(xmlval));
                        xmlFree(xmlval);
                        break;
                    }
                }
            }
        }
        retVal = eval_studio_config_xml(doc, current->children, match);
        if (!retVal.empty()) {
            break;
        }
    }

    return retVal;
}

// Returns:
//   - An empty Optional to indicate that parsing failed.
//   - Optional("") to indicate that we found the file, but |match| failed.
//   - Optional(matched_vaule) in case of success.
static Optional<string> parseStudioXML(const StudioXml* const match) {
    Optional<string> retval;

    System* sys = System::get();
    // Get path to .AndroidStudio
    std::string appDataPath;
    std::string studio = sys->envGet("ANDROID_STUDIO_PREFERENCES");
    if (studio.empty()) {
#ifdef __APPLE__
        appDataPath = sys->getAppDataDirectory();
#else
        appDataPath = sys->getHomeDirectory();
#endif  // __APPLE__
        if (appDataPath.empty()) {
            return retval;
        }
        studio = latestAndroidStudioDir(appDataPath);
    }
    if (studio.empty()) {
        return retval;
    }

    // Find match->filename xml file under .AndroidStudio
    std::string xml_path = pathToStudioXML(studio, match->filename);
    if (xml_path.empty()) {
        D("Failed to find %s in %s", match->filename, studio.c_str());
        return retval;
    }

    xmlDocPtr doc = xmlReadFile(xml_path.c_str(), NULL, 0);
    if (doc == NULL)
        return retval;

    // At this point, we assume that we have found the correct xml file.
    // If we fail to match within this file, we'll return an empty string
    // indicating that nothing matches.
    xmlNodePtr root = xmlDocGetRootElement(doc);
    retval = eval_studio_config_xml(doc, root, match);
    xmlFreeDoc(doc);

    return retval;
}

//
// This is a simple ad-hoc JSON parser for the currently present values of the
// Android Studio metrics configuration file.
// Function doesn't try to be format-compliant or even implement more than we
// currently need, it just allows one to parse basic name:value pairs and pass
// a callback to handle the "value" part
// |name| - the key to extract value for, without quotes
// |extractValue| - a functor that's called with a whole rest of the current
//  line as an argument, starting with the first non-whitespace character after
//  ':'. E.g.:
//
//      "key" :   "value","otherkey":11
//                ^-------------------^
//                  |linePart|, inclusive.
//
template <class ValueType, class ValueExtractor>
static Optional<ValueType> getStudioConfigJsonValue(
        StringView name,
        ValueExtractor extractValue) {
    const std::string configPath = android::metrics::getSettingsFilePath();
    std::ifstream in(configPath.c_str());
    if (!in) {
        return {};
    }

    // Parse JSON to extract the needed option
    std::string line;
    while (std::getline(in, line)) {
        // All names start from a double quote character, so start with finding
        // one.
        for (auto it = std::find(line.begin(), line.end(), '\"');
             it != line.end();
             it = std::find(it + 1, line.end(), '\"')) {
            auto itName = it + 1;
            if (itName == line.end()) {
                continue;
            }
            // Check if this is the right key...
            if (strncmp(&*itName, name.data(), name.size()) != 0) {
                continue;
            }
            auto itNameEnd = itName + name.size();
            if (itNameEnd == line.end() || *itNameEnd != '\"') {
                continue;
            }

            auto itNext = itNameEnd + 1;
            // Skip the spaces before the colon...
            while (itNext < line.end() && isspace(*itNext)) {
                ++itNext;
            }
            // ... the colon itself...
            if (itNext == line.end() || *itNext != ':') {
                continue;
            }
            ++itNext;
            // ... and spaces after it.
            while (itNext < line.end() && isspace(*itNext)) {
                ++itNext;
            }
            if (itNext == line.end()) {
                continue;
            }
            // The value has to be somewhere in the remaining part of the line,
            // pass it to someone who knows how to extract it.
            auto val = extractValue(StringView(&*itNext, line.end() - itNext));
            return val;
        }
    }
    return {};
}

bool getUserMetricsOptIn() {
    return getStudioConfigJsonValue<bool>(
                   "hasOptedIn",
                   [](StringView linePart) -> bool {
                       static constexpr StringView trueValues[] = {"true", "1"};
                       auto it = linePart.begin();
                       for (const auto& trueVal : trueValues) {
                           if (strncmp(trueVal.data(), &*it, trueVal.size())) {
                               continue;
                           }
                           it += trueVal.size();
                           // make sure the value ends here
                           if (it == linePart.end()) {
                               return true;
                           }
                           if (it < linePart.end() &&
                               (isspace(*it) || ispunct(*it))) {
                               return true;
                           }
                       }
                       return false;
                   })
            .valueOr(false);
}

// Forward-declare a function to use here and in the unit test.
std::string extractInstallationId();

namespace {

// A collection of cached configuration values - these aren't supposed to change
// during the emulator run time, so we don't need to re-parse them on every
// call.
struct StaticValues {
    std::string installationId;

    StaticValues() {
        installationId = extractInstallationId();
    }
};

static LazyInstance<StaticValues> sStaticValues = {};

}  // namespace

// This is to be able to unit-test the implementation without hacking into the
// LazyInstance<> code.
std::string extractInstallationId() {
    auto optionalId = getStudioConfigJsonValue<std::string>(
            "userId", [](StringView linePart) -> std::string {
                if (linePart.empty()) {
                    return {};
                }
                auto it = linePart.begin();
                if (*it == '\"') {
                    // This is a quoted string.
                    // BTW, it doesn't support escaping as there was no need
                    // yet.
                    ++it;
                    auto itEnd = it;
                    while (itEnd != linePart.end() && *itEnd != '\"') {
                        ++itEnd;
                    }
                    return std::string(it, itEnd);
                } else {
                    // No quotes, just parse till the end of the value.
                    auto itEnd = it;
                    while (itEnd != linePart.end() && !isspace(*itEnd) &&
                           *itEnd != ',') {
                        ++itEnd;
                    }
                    return std::string(it, itEnd);
                }
            });
    return optionalId.valueOr({});
}

const std::string& getInstallationId() {
    return sStaticValues->installationId;
}

}  // namespace studio
}  // namespace android
