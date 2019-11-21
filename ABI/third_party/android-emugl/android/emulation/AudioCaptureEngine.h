// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/Log.h"

namespace android {
namespace emulation {

class AudioCapturer;

// A generic engine for audio capturing

// AudioCaptureEngine provides a generic inteface for audio capture.
// The specific vm engine must implements a subclass of AudioCaptureEngine.
//
// Usage:
//    1) At vm starts up, register this enigne by calling the AudioCaptureEngine::set() static method
//       bool qemu_android_emulation_early_setup() {
//          ...
//          AudioCaptureEngine::set(new YouAudioCaptureEngine());
//          ...
//       }
//    2) Create a new AudioCapturer instance, which implements onSample() method
//
//    3) Call AudioCaptureEngine::start() with the instance created in (1) to start capture.
//       After this call, audio byte stream will arrive in AudioCapturer::onSample() method
//
//    4) Call AudioCaptureEngine::stop() with the instance created in (1) to stop capture
//
class AudioCaptureEngine {
protected:
     AudioCaptureEngine() = default;
     virtual ~AudioCaptureEngine() = default;

public:
     // start the capturer to capture audio, subclass should implement this method
     virtual int start(AudioCapturer *capturer) = 0;

     // stop the capturer, subclass should implement this method
     virtual int stop(AudioCapturer *capturer) = 0;

     // set the audio capture engine
     // The set() needs to be called at the point where no races are possible
     // for example, during the period where vm setups
     static void set(AudioCaptureEngine* engine) {
         mInstance = engine;
     }

     static AudioCaptureEngine* get() {
         CHECK(mInstance);
         return mInstance;
     }

private:
    static AudioCaptureEngine *mInstance;

    DISALLOW_COPY_AND_ASSIGN(AudioCaptureEngine);
};

}  // namespace emulation
}  // namespace android
