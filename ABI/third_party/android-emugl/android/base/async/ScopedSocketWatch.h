// Copyright 2016 The Android Open Source Project
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

#include <memory>

namespace android {
namespace base {

// ScopedSocketWatch is a convenience class that implements a scoped
// Looper::FdWatch pointer that will automatically call socketClose()
// on destruction.

// Custom deleter from std::unique_ptr
struct SocketWatchDeleter {
    void operator()(android::base::Looper::FdWatch* watch) const;
};

using ScopedSocketWatch = std::unique_ptr<Looper::FdWatch, SocketWatchDeleter>;

}  // namespace base
}  // namespace android

