/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common/alog.h"

#define LOG_BUF_SIZE 1024

extern "C" void
__android_log_assert_with_source(const char* cond, const char* tag,
                                 const char* file, int line,
                                 const char* fmt, ...) {
  va_list arguments;
  va_start(arguments, fmt);
  fprintf(stderr, "CONDITION %s WAS TRUE AT %s:%d\n", cond, file, line);
  vfprintf(stderr, fmt, arguments);
  fprintf(stderr, "\n");
  va_end(arguments);
  abort();
}

namespace arc {

int PrintLogBuf(int bufID, int prio, const char* tag, const char* fmt, ...) {
  va_list arguments;
  va_start(arguments, fmt);
  vfprintf(stderr, fmt, arguments);
  fprintf(stderr, "\n");
  va_end(arguments);
  return 0;
}

}  // namespace arc

int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    return arc::PrintLogBuf(0, prio, tag, buf);
}
