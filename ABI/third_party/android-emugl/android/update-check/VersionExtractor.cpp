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

#include "VersionExtractor.h"

#include "android/base/ArraySize.h"
#include "android/base/Compiler.h"
#include "android/base/containers/Lookup.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/Optional.h"
#include "android/utils/debug.h"
#include "android/version.h"
#include "config-host.h"

#include <libxml/tree.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <unordered_map>

#include <errno.h>
#include <stdlib.h>

namespace android {
namespace update_check {

using android::base::findOrDefault;
using android::base::Optional;
using android::base::Version;
using android::studio::UpdateChannel;

const char* const VersionExtractor::kXmlNamespace =
        "http://schemas.android.com/sdk/android/repo/repository2/01";

namespace {

struct DeleteXmlObject {
    template <class T>
    void operator()(T t) const {
        xmlFree(t);
    }

    void operator()(xmlDocPtr doc) const { xmlFreeDoc(doc); }
};
}

template <class T>
using xmlAutoPtr = std::unique_ptr<T, DeleteXmlObject>;

static Optional<Version::ComponentType> parseInt(const xmlChar* str,
                                                 const char* stopChars = "") {
    if (!str) {
        return {};
    }

    char* end;
    errno = 0;
    const auto value = strtoul(reinterpret_cast<const char*>(str), &end, 10);
    if (errno || strchr(stopChars, *end) == nullptr ||
        value > std::numeric_limits<Version::ComponentType>::max() ||
        value == Version::kNone) {
        dwarning("UpdateCheck: invalid value of a version attribute");
        return {};
    }
    return static_cast<Version::ComponentType>(value);
}

static const xmlChar* getContent(xmlNodePtr node) {
    // libxml2 has the following structure of the node content:
    // <a>aaa</a> ->
    //  xmlNode(name = "a", type=element_node)
    //    -> first child xmlNode(name = "text", type=element_text)
    //      ->content
    if (node->type != XML_ELEMENT_NODE || !node->children ||
        node->children->type != XML_TEXT_NODE || !node->children->content) {
        return {};
    }

    return node->children->content;
}

static std::pair<Version, std::string> parseVersion(xmlNodePtr node) {
    Optional<Version::ComponentType> major, minor, micro, build;
    xmlNodePtr revNode = nullptr;
    std::string channelName;

    for (auto child = node->children;
         child && (!revNode || !build || channelName.empty());
         child = child->next) {
        if (child->type == XML_COMMENT_NODE && child->content) {
            // the comment with build number has a string "bid:<build>"
            const auto buildStr = xmlStrstr(child->content, BAD_CAST "bid:");
            if (buildStr) {
                build = parseInt(buildStr + STRING_LITERAL_LENGTH("bid:"),
                                 ", \r\n\t");
            }
        } else if (child->type == XML_ELEMENT_NODE &&
                   xmlStrcmp(child->name, BAD_CAST "revision") == 0) {
            revNode = child;
        } else if (child->type == XML_ELEMENT_NODE &&
                   xmlStrcmp(child->name, BAD_CAST "channelRef") == 0) {
            const xmlAutoPtr<xmlChar> channelRef(
                    xmlGetProp(child, BAD_CAST "ref"));
            if (channelRef) {
                channelName = std::string(
                        reinterpret_cast<const char*>(channelRef.get()));
            }
        }
    }

    if (!revNode) {
        return {};
    }

    for (xmlNodePtr verPart = revNode->children; verPart;
         verPart = verPart->next) {
        const auto content = getContent(verPart);
        if (const auto val = parseInt(content)) {
            if (xmlStrcmp(verPart->name, BAD_CAST "major") == 0) {
                major = val;
            } else if (xmlStrcmp(verPart->name, BAD_CAST "minor") == 0) {
                minor = val;
            } else if (xmlStrcmp(verPart->name, BAD_CAST "micro") == 0) {
                micro = val;
            }
        }
    }

    if (!major || !minor || !micro) {
        return {};
    }

    return {Version(*major, *minor, *micro, build.valueOr(0)),
            std::move(channelName)};
}

static UpdateChannel parseUpdateChannelName(const xmlChar* name) {
    static const struct NamedChannel {
        const xmlChar* name;
        UpdateChannel channel;
    } channels[] = {{BAD_CAST "stable", UpdateChannel::Stable},
                    {BAD_CAST "beta", UpdateChannel::Beta},
                    {BAD_CAST "dev", UpdateChannel::Dev},
                    {BAD_CAST "canary", UpdateChannel::Canary}};
    const auto it = std::find_if(std::begin(channels), std::end(channels),
                                 [name](const NamedChannel& nc) {
                                     return xmlStrcmp(nc.name, name) == 0;
                                 });
    if (it == std::end(channels)) {
        return UpdateChannel::Unknown;
    }

    return it->channel;
}

IVersionExtractor::Versions VersionExtractor::extractVersions(
        StringView data) const {
    // make sure libxml2 is initialized and is of proper version
    LIBXML_TEST_VERSION

    const xmlAutoPtr<xmlDoc> doc(
            xmlReadMemory(data.c_str(), data.size(), "none.xml", nullptr, 0));
    if (!doc) {
        return {};
    }

    const xmlNodePtr root = xmlDocGetRootElement(doc.get());
    if (!root->ns || xmlStrcmp(root->ns->href, BAD_CAST kXmlNamespace) != 0) {
        return {};
    }

    std::unordered_map<std::string, UpdateChannel> channelMap;
    Versions res;

    // iterate over all nodes in the document and find all <tool> ones
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (xmlStrcmp(node->name, BAD_CAST "channel") == 0) {
            // <channel id="id">name</channel>
            const auto channelName = getContent(node);
            if (!channelName) {
                continue;
            }
            const xmlAutoPtr<xmlChar> id(xmlGetProp(node, BAD_CAST "id"));
            if (!id) {
                continue;
            }
            const UpdateChannel channel = parseUpdateChannelName(channelName);
            if (channel == UpdateChannel::Unknown) {
                continue;
            }
            channelMap[std::string(reinterpret_cast<const char*>(id.get()))] =
                    channel;
        } else if (xmlStrcmp(node->name, BAD_CAST "remotePackage") == 0) {
            // <remotePackage path="emulator|tools">
            //  !version
            //  !channel info
            //  !build info
            // </remotePachage>
            xmlAutoPtr<xmlChar> pathVal(xmlGetProp(node, BAD_CAST "path"));
            if (!pathVal) {
                continue;
            }

            const bool isEmulatorPackage =
                    xmlStrcmp(pathVal.get(), BAD_CAST "emulator") == 0;
            const bool isToolsPackage = !isEmulatorPackage &&
                    xmlStrcmp(pathVal.get(), BAD_CAST "tools") == 0;
            if (!isEmulatorPackage && !isToolsPackage) {
                continue;
            }

            Version nodeVer;
            std::string channelName;
            std::tie(nodeVer, channelName) = parseVersion(node);
            if (!nodeVer.isValid()) {
                continue;
            }
            // Emulator used to be a part of "tools" package until version 25.3.
            // Skip all "tools" packages after that one, and don't care about
            // the "emulator" packages before 25.3 as well.
            if ((isEmulatorPackage && nodeVer < Version(25, 3, 0)) ||
                (isToolsPackage && !(nodeVer < Version(25, 3, 0)))) {
                continue;
            }

            const auto channel = findOrDefault(channelMap, channelName,
                                               UpdateChannel::Canary);
            Version& existingVer = res[channel];
            if (!existingVer.isValid() || existingVer < nodeVer) {
                existingVer = nodeVer;
            }
        }
    }

    return res;
}

Version VersionExtractor::getCurrentVersion() const {
    static const Version currentVersion =
            Version(EMULATOR_FULL_VERSION_STRING);
    return currentVersion;
}

}  // namespace update_check
}  // namespace android
