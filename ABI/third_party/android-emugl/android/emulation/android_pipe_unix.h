#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Initialize the unix: pipe facility. Must be called to allow the guest
// to connect to the pipe service.
void android_unix_pipes_init(void);

// Add a host Unix domain path to the white list that is checked when a guest
// connects to a unix: pipe. Any non-matching path will be rejected by
// force-closing the guest pipe fd.
void android_unix_pipes_add_allowed_path(const char* path);

ANDROID_END_HEADER
