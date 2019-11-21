/* Copyright 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include "android/utils/compiler.h"

#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// Shared type and constant declarations between android_pipe.h and
// android_pipe_host.h

// Buffer descriptor for android_pipe_guest_send() and
// android_pipe_guest_recv().
typedef struct AndroidPipeBuffer {
    uint8_t* data;
    size_t size;
} AndroidPipeBuffer;

/* List of bitflags returned in status of CMD_POLL command */
enum PipePollFlags {
    PIPE_POLL_IN  = 1 << 0,
    PIPE_POLL_OUT = 1 << 1,
    PIPE_POLL_HUP = 1 << 2
};

/* Possible status values used to signal errors - see goldfish_pipe_error_convert */
enum PipeErrors {
    PIPE_ERROR_INVAL  = -1,
    PIPE_ERROR_AGAIN  = -2,
    PIPE_ERROR_NOMEM  = -3,
    PIPE_ERROR_IO     = -4
};

/* Bit-flags used to signal events from the emulator */
enum PipeWakeFlags {
    PIPE_WAKE_CLOSED = 1 << 0,  /* emulator closed pipe */
    PIPE_WAKE_READ   = 1 << 1,  /* pipe can now be read from */
    PIPE_WAKE_WRITE  = 1 << 2,  /* pipe can now be written to */
    PIPE_WAKE_UNLOCK_DMA  = 1 << 3,  /* unlock this pipe's DMA buffer */
    PIPE_WAKE_UNLOCK_DMA_SHARED  = 1 << 4,  /* unlock DMA buffer of the pipe shared to this pipe */
};

/* Possible pipe closing reasons */
typedef enum PipeCloseReason {
    PIPE_CLOSE_GRACEFUL = 0,      /* guest sent a close command */
    PIPE_CLOSE_REBOOT   = 1,      /* guest rebooted, we're closing the pipes */
    PIPE_CLOSE_LOAD_SNAPSHOT = 2, /* close old pipes on snapshot load */
    PIPE_CLOSE_ERROR    = 3,      /* some unrecoverable error on the pipe */
} PipeCloseReason;

ANDROID_END_HEADER
