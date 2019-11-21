// Copyright 2014 The Android Open Source Project
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
namespace base {

// Convenience class used to implement per-thread Looper instances.
// This allows one to call ThreadLooper::get() from any place to retrieve
// a thread-local Looper instance.
class ThreadLooper {
public:
    // Retrieve the current thread's Looper instance.
    //
    // If setLooper() was called previously on this thread, return the
    // corresponding value. Otherwise, on first call, use Looper:create()
    // and store the new instsance in thread-local storage. Note that this
    // new instance will be freed automatically when the thread exits.
    static Looper* get();

    // Sets the looper for the current thread. Must be called before get()
    // for the current thread. |looper| is a Looper instance that cannot be
    // NULL, and will be returned by future calls to get().
    //
    // Note that |looper| will be stored in thread-local storage but will
    // not be destroyed when the thread exits (unless |own| is true).
    static void setLooper(Looper* looper, bool own = false);
};

}  // namespace base
}  // namespace android
