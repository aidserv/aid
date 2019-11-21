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

#include "android/emulation/SerialLine.h"

namespace android {

// Default implementation is provided to ensure the library builds
// as a stand-alone unit, but any testing requires a specific
// implementation injected through SerialLine::Funcs::reset()

static SerialLine* nullOpenBuffer(SerialLine* sl) {
    return nullptr;
}

static bool nullOpenPipe(SerialLine** pfirst, SerialLine** psecond) {
    *pfirst = nullptr;
    *psecond = nullptr;
    return false;
};

static const SerialLine::Funcs kNullFuncs = {
    .openBuffer = nullOpenBuffer,
    .openPipe = nullOpenPipe,
};

static const SerialLine::Funcs* sFuncs = &kNullFuncs;

// static
const SerialLine::Funcs* SerialLine::Funcs::get() {
    return sFuncs;
}

// static
const SerialLine::Funcs* SerialLine::Funcs::reset(
        const SerialLine::Funcs* funcs) {
    const SerialLine::Funcs* result = sFuncs;
    sFuncs = funcs ? funcs : &kNullFuncs;
    return result;
}

}  // namespace android
