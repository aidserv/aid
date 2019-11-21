// Copyright 2016 The Android Open Source Project
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

#pragma once

#include "android/base/network/NetworkUtils.h"

#include <vector>

namespace android {
namespace base {

// A custom name resolver used for unit-testing.
// Usage:
//    1) Create instance in local scope.
//    2) Call addEntry() to add specific entries.
//    3) Perform any calls that might end up calling queryInterfaceName()
//    4) Automatically cleaned-up when exiting the scope.
class TestNetworkInterfaceNameResolver : public NetworkInterfaceNameResolver {
public:
    TestNetworkInterfaceNameResolver()
        : mOldResolver(netSetNetworkInterfaceNameResolverForTesting(this)) {}

    ~TestNetworkInterfaceNameResolver() {
        netSetNetworkInterfaceNameResolverForTesting(mOldResolver);
    }

    virtual int queryInterfaceName(const char* str) {
        for (const auto& item : mEntries) {
            if (item.first == str) {
                return item.second;
            }
        }
        return -1;
    }

    void addEntry(const char* name, int index) {
        mEntries.emplace_back(std::make_pair(std::string(name), index));
    }

private:
    NetworkInterfaceNameResolver* mOldResolver;
    std::vector<std::pair<std::string, int>> mEntries;
};

}  // namespace base
}  // namespace android
