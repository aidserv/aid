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

// This file defines common callbacks used by the interfaces defined for
// AndroidEmu <--> QEMU interaction.

// A callback to consume a single line of output (including newline).
// |opque| is a handle to a context object. Functions that use this callback
// will also take a context handle that will be passed to the callback.
// |buff| contains the data to be consumed, of length |len|.
// The function should return the number of chars consumed successfully.
typedef int (*LineConsumerCallback)(void* opaque, const char* buff, int len);
