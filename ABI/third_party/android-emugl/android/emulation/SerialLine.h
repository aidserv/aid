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

#include "android/emulation/serial_line.h"

// This is a C++ interface for a SerialLine abstraction, based
// on QEMU's character device (char.h)
// Currently it only provides a |write| call two callbacks for reading
// For the usage details see |android/emulation/serial_line.h|

namespace android {

class SerialLine : public CSerialLine {
public:
    virtual ~SerialLine() = default;

    typedef SLCanReadHandler* CanReadFunc;
    typedef SLReadHandler* ReadFunc;

    virtual void addHandlers(void* opaque,
                             CanReadFunc canReadFunc,
                             ReadFunc readFunc) = 0;

    virtual int write(const uint8_t* data, int len) = 0;

    // Small structure providing function pointers to create SerialLine
    // instances. Use Funcs::get() to get the current implementation. The
    // default implementation always fail to create anything, but an engine
    // or unit-test specific implementation can be injected by calling
    // Funcs::reset() at runtime.
    struct Funcs {
        // Given an existing SerialLine instance |sl|, create a new instance
        // that implements a buffer for any data sent to |sl|. On the other
        // hand, any CanReadFunc/ReadFunc request will be passed directly
        // to |sl|'s handlers.
        SerialLine* (*openBuffer)(SerialLine* sl);

        // Create two new SerialLine instances that are connected through
        // buffers as a pipe. On success, return true and sets |*pfirst|
        // and |*psecond| to the instance pointers.
        bool (*openPipe)(SerialLine** pfirsts, SerialLine** psecond);

        // Return current SerialLine::Funcs instance. This never returns
        // nullptr.
        static const Funcs* get();

        // Reset the current global SerialLine::Funcs implementation.
        // If |factory| is nullptr, the default implementation is used.
        // Returns the previous value.
        static const Funcs* reset(const Funcs* factory);
    };
};

}  // namespace android
