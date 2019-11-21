// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/threads/ThreadStore.h"

namespace emugl {

// Non-templated ThreadStore in android::base is actually called
// ThreadStoreBase, while android::base::ThreadStore is a templated version
// which auto-casts and auto-deletes the stored data.
// That's why it's preferred to use android::base::ThreadStore<> directly over
// the emugl::ThreadStore.

// using ThreadStore = android::base::ThreadStoreBase;
typedef android::base::ThreadStoreBase ThreadStore;

}  // namespace emugl
