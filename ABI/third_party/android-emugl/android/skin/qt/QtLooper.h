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

#pragma once

#include "android/base/async/Looper.h"

namespace android {
namespace qt {

// Return a Looper instance for threads running Qt's event loop.
// There is one instance per QEventLoop, so calling this multiple times from the
// any thread will given you different instances of Looper that correspond to
// the same global state for that thread.
// TODO(pprabhu) Comment about starting the event loop?
android::base::Looper* createLooper();

}  // namespace qt
}  // namespace android
