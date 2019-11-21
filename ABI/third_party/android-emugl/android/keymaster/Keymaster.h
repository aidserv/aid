// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/keymaster/keymaster_common.h"

#include <vector>

namespace android{
namespace Keymaster {
    int generateKeypair(
            const keymaster_keypair_t keyType, const void* keyParams,
            std::vector<uint8_t>* keyBlob);
    int importKeypair(const uint8_t* key, const uint32_t keyLength,
            std::vector<uint8_t>* keyBlob);
    int getKeypairPublic(const uint8_t* keyBlob, const uint32_t keyBlobLength,
            std::vector<uint8_t>* x509Data);
    int signData(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* data, const uint32_t dataLength,
            std::vector<uint8_t>* signedData);
    int verifyData(const void* params,
            const uint8_t* keyBlob, const uint32_t keyBlobLength,
            const uint8_t* signedData, const uint32_t signedDataLength,
            const uint8_t* signature, const uint32_t signatureLength);
}
}

