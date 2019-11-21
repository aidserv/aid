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

#include "android/utils/compiler.h"

#include <stdint.h>

// This is a C wrapper for a serial line abstraction, based
// on QEMU's character device (char.h)
//
// CSerialLine allows one to write data into the connection and supply a
// callback for reading. To send some data, call android_serialline_write()
// which returns the number of writes accepted by the serial line (which may
// be less than what is being sent if the line is not ready). It is possible
// to create a buffered serial line wrapper by calling
// android_serialline_buffer_open().
//
// When there's some data to read, the callbacks supplied in
// android_serialline_addhandlers() will be called in this order:
//    SLCanReadHandler(opaque)
//    SLReadHandler(opaque, data, len)
// |opaque| is the value passed in second argument of
// android_serialline_addhandlers() - some user state for the callbacks

// Note that CSerialLine instances can only be created/destroyed from QEMU
// glue code. During unit-testing, a special TestSerialLine implementation
// will be provided instead, but isn't part of AndroidEmu itself.

ANDROID_BEGIN_HEADER

// A handle to the serial line object
typedef struct CSerialLine {} CSerialLine;

// Read callbacks for the serial line

// return a number of bytes read handler can consume now.
typedef int SLCanReadHandler(void* opaque);
// read the data - |size| bytes from memory at |buf|
typedef void SLReadHandler(void* opaque, const uint8_t* buf, int size);

// sets read callbacks on a specific serial line object.
// |opaque| is some user state to be passed to the callbacks
extern void android_serialline_addhandlers(CSerialLine* sl, void* opaque,
                                           SLCanReadHandler* canReadCallback,
                                           SLReadHandler* readCallback);

// try to send |len| bytes of |data| through the serial line,
// returns the number of bytes that went through, or negative number on error
extern int android_serialline_write(CSerialLine* sl, const uint8_t* data, int len);

// Given an existing CSerialLine instance |sl|, create a new instance that
// implements a buffer for any data sent to |sl|. On the other hand,
// any can_read() / read() request performed by it will be passed
// directly to |sl|'s handlers.
// NOTE: This function is not implemented by AndroidEmu and must be provided
// by the QEMU glue code, or unit-test runtime.
CSerialLine* android_serialline_buffer_open(CSerialLine* sl);

// Create two new SerialLine instances that are connected through buffers
// as a pipe. On success, return 0 and sets |*pfirst| and |*psecond| to
// the instance pointers. On failure, return -1 and sets |*pfirst| and
// |*psecond| to NULL.
// NOTE: This function is not implemented by AndroidEmu and must be provided
// by the QEMU glue code, or unit-test runtime.
int android_serialline_pipe_open(CSerialLine** pfirst, CSerialLine** psecond);

ANDROID_END_HEADER
