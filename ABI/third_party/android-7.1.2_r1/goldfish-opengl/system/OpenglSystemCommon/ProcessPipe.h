//
// Copyright (C) 2016 The Android Open Source Project
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
//

#pragma once

#include <stdint.h>

// The process pipe is used to notify the host about process exits,
// also associate all process-owned host GL resources with a process unique ID
// (puid). This is for the purpose that the host  can clean up process resources
// when a process is killed. It will fallback to the default path if the host
// does not support it. Processes are identified by acquiring a per-process
// 64bit unique ID (puid) from the host.
//
// Calling processPipeInit will associate the current render thread with its
// puid in the host.
//
// This is called when creating rcEncoder.

struct renderControl_client_context_t;

extern bool processPipeInit(renderControl_encoder_context_t *rcEnc);
