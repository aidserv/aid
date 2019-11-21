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

#pragma once

#include "android/emulation/control/callbacks.h"
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// C interface to expose Qemu implementations of common VM related operations.
typedef struct QAndroidVmOperations {
    bool (*vmStop)(void);
    bool (*vmStart)(void);
    bool (*vmIsRunning)(void);

    // Snapshot-related VM operations.
    // |outConsuer| and |errConsumer| are used to report output / error
    // respectively. Each line of output is newline terminated and results in
    // one call to the callback. |opaque| is passed to the callbacks as context.
    // Returns true on success, false on failure.
    bool (*snapshotList)(void* opaque,
                         LineConsumerCallback outConsumer,
                         LineConsumerCallback errConsumer);
    bool (*snapshotSave)(const char* name,
                         void* opaque,
                         LineConsumerCallback errConsumer);
    bool (*snapshotLoad)(const char* name,
                         void* opaque,
                         LineConsumerCallback errConsumer);
    bool (*snapshotDelete)(const char* name,
                           void* opaque,
                           LineConsumerCallback errConsumer);
} QAndroidVmOperations;

ANDROID_END_HEADER
