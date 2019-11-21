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

#include "android/utils/win32_cmdline_quote.h"

#include <gtest/gtest.h>

#include "android/utils/system.h"

// Helper class to hold a scoped heap-allocated string pointer that gets
// deleted on scope exit.
class ScopedCString {
public:
  ScopedCString(const char* str) : str_(str) {}
  const char* str() const { return str_; }
  ~ScopedCString() { AFREE((void*)str_); }
private:
  const char* str_;
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

TEST(win32_cmdline_quote,Test) {
  static const struct {
      const char* input;
      const char* expected;
  } kData[] = {
      { "foo", "foo" },
      { "foo bar", "\"foo bar\"" },
      { "foo\\bar", "foo\\bar" },
      { "foo\\\\bar", "foo\\\\bar" },
      { "foo\"bar", "\"foo\\\"bar\"" },
      { "foo\\\"bar", "\"foo\\\\\\\"bar\"" },
      { "foo\\bar zoo", "\"foo\\bar zoo\"" },
  };
  for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
      const char* input = kData[n].input;
      const char* expected = kData[n].expected;

      ScopedCString out(win32_cmdline_quote(input));
      EXPECT_STREQ(expected, out.str()) << "Quoting '" << input << "'";
  }
}
