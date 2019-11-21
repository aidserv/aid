/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "TagLengthValue.h"

#include "android/utils/misc.h"

#include <assert.h>
#include <vector>
#include <stdint.h>
#include <stdio.h>

namespace android {

// The maximum size that can be represented in one size byte, if the size value
// is less than this constant then that's the number of bytes of data. if the
// data is larger than this then the value stored minus this constant will be
// the number of bytes that are used to store the size. So if the value is 0x84
// then 4 bytes are used to store the size. Here are a few examples:
// Size: 0x70,    String: "70"
// Size: 0x80,    String: "80"
// Size: 0x81,    String: "8181"     -- First 81 indicates 1 byte needed (81-80)
// Size: 0x21f3,  String: "8221f3"   -- 82 indicates 2 bytes needed
// Size: 0x12345, String: "83012345" -- 83 indicates 3 bytes needed, zero padded
static const uint32_t kMaxSingleByteSize = 0x80;

// Convert a plain string to a string of hexadecimal values representing the
// value of each character of the plain string. For example the string "FOO"
// will be converted to "464F4F".
static std::string stringToHexString(const std::string& str) {
    std::string result(str.size() * 2, '0');
    for (size_t i = 0; i < str.size(); ++i) {
        int2hex(reinterpret_cast<uint8_t*>(&result[i * 2]), 2, str[i]);
    }
    return result;
}

void TagLengthValue::populateData(const char* tag,
        std::initializer_list<const TagLengthValue*> data) {
    populateData(tag, data.begin(), data.end());
}

template<class T>
void TagLengthValue::populateData(const char* tag,
        std::initializer_list<const T*> data) {
    populateData(tag, data.begin(), data.end());
}

template<class T>
void TagLengthValue::populateData(const char* tag, T begin, T end) {
    size_t payloadSize = 0;
    for (auto object = begin; object != end; ++object) {
        payloadSize += (*object)->size();
    }
    size_t tagLength = tag ? strlen(tag) : 0;
    // Use half the size because it's not the size in characters but in
    // bytes. Each byte is represented by two characters
    std::string sizeString = createSizeString(payloadSize / 2);
    mData.resize(tagLength + sizeString.size() + payloadSize);
    if (tag && tagLength > 0) {
        memcpy(&mData[0], tag, tagLength);
    }
    memcpy(&mData[tagLength], sizeString.c_str(), sizeString.size());
    size_t offset = tagLength + sizeString.size();

    for (auto object = begin; object != end; ++object) {
        size_t objectSize = (*object)->size();
        memcpy(&mData[offset], (*object)->c_str(), objectSize);
        offset += objectSize;
    }
}

std::string TagLengthValue::createSizeString(size_t size) {
    char sizeString[16];
    if (size <= kMaxSingleByteSize) {
        snprintf(sizeString, sizeof(sizeString), "%02x",
                 static_cast<uint32_t>(size));
        return sizeString;
    }
    // Sizes larger than 0x80 require size length specifier followed by size
    for (uint32_t numBytes = 1; numBytes <= 4; ++numBytes) {
        if (size < (1ULL << (numBytes * 8))) {
            // The size requires numBytes byte(s) of storage, each byte takes
            // two characters of space in a string in hex representation.
            snprintf(sizeString, sizeof(sizeString), "%02x%0*x",
                     kMaxSingleByteSize + numBytes, 2 * numBytes,
                     static_cast<uint32_t>(size));
            return sizeString;
        }
    }
    assert(false);
    sizeString[0] = '\0';
    return sizeString;
}

const char AidRefDo::kTag[] = "4F";
const char PkgRefDo::kTag[] = "CA";
const char DeviceAppIdRefDo::kTag[] = "C1";
const char RefDo::kTag[] = "E1";
const char ApduArDo::kTag[] = "D0";
const char NfcArDo::kTag[] = "D1";
const char PermArDo::kTag[] = "DB";
const char ArDo::kTag[] = "E3";
const char RefArDo::kTag[] = "E2";
const char AllRefArDo::kTag[] = "FF40";

PkgRefDo::PkgRefDo(const std::string& packageName) {
    std::string hexPackageName = stringToHexString(packageName);
    populateData(kTag, { &hexPackageName });
}

DeviceAppIdRefDo::DeviceAppIdRefDo(const std::string& stringData) {
    populateData(kTag, { &stringData });
}

RefDo::RefDo(const DeviceAppIdRefDo& deviceAppIdRefDo) {
    populateData(kTag, { &deviceAppIdRefDo });
}

RefDo::RefDo(const AidRefDo& aidRefDo,
             const DeviceAppIdRefDo& deviceAppIdRefDo) {
    populateData(kTag, { &aidRefDo, &deviceAppIdRefDo });
}

RefDo::RefDo(const DeviceAppIdRefDo& deviceAppIdRefDo,
             const PkgRefDo& pkgRefDo) {
    populateData(kTag, { &deviceAppIdRefDo, &pkgRefDo });
}

ApduArDo::ApduArDo(Allow rule) {
    // Allocate enough space for the data plus NULL, then resize down to 2 to
    // avoid modifying the terminating NULL (which is undefined behavior)
    std::string data(3, '0');
    snprintf(&data[0], data.size(), "%02x", static_cast<int>(rule));
    data.resize(2);
    populateData(kTag, { &data });
}

ApduArDo::ApduArDo(std::initializer_list<std::string> rules) {
    std::vector<const std::string*> data;
    for(const auto& rule : rules) {
        data.push_back(&rule);
    }
    populateData(kTag, data.begin(), data.end());
}

NfcArDo::NfcArDo(Allow rule) {
    // Allocate enough space for the data plus NULL, then resize down to 2 to
    // avoid modifying the terminating NULL (which is undefined behavior)
    std::string data(3, '0');
    snprintf(&data[0], data.size(), "%02x", static_cast<int>(rule));
    data.resize(2);
    populateData(kTag, { &data });
}

PermArDo::PermArDo(const std::string& stringData) {
    populateData(kTag, { &stringData });
}

ArDo::ArDo(const ApduArDo& apduArDo) {
    populateData(kTag, { &apduArDo });
}

ArDo::ArDo(const NfcArDo& nfcArDo) {
    populateData(kTag, { &nfcArDo });
}

ArDo::ArDo(const PermArDo& permArDo) {
    populateData(kTag, { &permArDo });
}

ArDo::ArDo(const ApduArDo& apduArDo, const NfcArDo& nfcArDo) {
    populateData(kTag, { &apduArDo, &nfcArDo });
}

RefArDo::RefArDo(const RefDo& refDo, const ArDo& arDo) {
    populateData(kTag, { &refDo, &arDo });
}

AllRefArDo::AllRefArDo(std::initializer_list<RefArDo> refArDos) {
    std::vector<const RefArDo*> data;
    for (const auto& object : refArDos) {
        data.push_back(&object);
    }
    populateData(kTag, data.begin(), data.end());
}

}  // namespace android
