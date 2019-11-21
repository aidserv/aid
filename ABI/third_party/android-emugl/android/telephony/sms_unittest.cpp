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

#include <gtest/gtest.h>
#include "android/telephony/sms.h"

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

namespace sms {

TEST(Sms, AddrFromStr) {

    static const struct {
          const char*          input;
          const int            expectedRet;
          const int            expectedToa;
          const int            expectedLen;
          const unsigned char  expectedData[16];
      } kData[] = {
          { "(650) 123-4567",   0, 0x81, 10, { 0x56, 0x10, 0x32, 0x54, 0x76 } },
          { "+1.650 123/4567",  0, 0x91, 11, { 0x61, 0x05, 0x21, 0x43, 0x65, 0xf7 } },
          { "5",                0, 0x81,  1, { 0xf5 } },
          { "+()",             -1, 0,     0, { } }, // No digits
          { "",                -1, 0,     0, { } }, // No digits
          { "123 456.7890"
            "(12)34567890"
            "1234567890 12",    0, 0x81, 32, { 0x21, 0x43, 0x65, 0x87, 0x09,
                                               0x21, 0x43, 0x65, 0x87, 0x09,
                                               0x21, 0x43, 0x65, 0x87, 0x09,
                                               0x21                          } },
          { "123 456.7890"
            "(12)34567890"
            "1234567890 123",  -1, 0,     0, { } }, // Too long
      };

    SmsAddressRec smsAddrRec;

    for (size_t idx = 0; idx < ARRAY_SIZE(kData); ++idx) {
        const char* input = kData[idx].input;

        memset(smsAddrRec.data, 0, sizeof(smsAddrRec.data));

        int rVal = sms_address_from_str(&smsAddrRec, input, strlen(input));

        EXPECT_EQ(rVal, kData[idx].expectedRet);
        if (rVal == 0) {
            EXPECT_EQ(smsAddrRec.len, kData[idx].expectedLen);
            const char cmpResult = memcmp( smsAddrRec.data,
                                           kData[idx].expectedData,
                                           (kData[idx].expectedLen + 1) / 2 );
            EXPECT_FALSE(cmpResult);
        }
    }
}

TEST(Sms, Utf8FromStr) {

    static const struct {
          const unsigned char  input[32];
          const int            outputBuffSize;
          const int            expectedRet;
          const unsigned char  expectedUtf8[32];
      } kData[] = {
          { "A text message",                    32, 14, "A text message"},
          { "A text message",                    11, 14, "A text mess"},
          { "A two line\ntext message",          32, 23, "A two line\ntext message"},
          { "A two line\\ntext message",         32, 23, "A two line\ntext message"},
          { "My \\x41 \\u0042 Cs",               32,  9, "My A B Cs" },
          { "5 \\xe2\\x82\\u00aC each \\u12345", 32, 18,
            "5 \303\242\302\202\302\254 each \301\210\2645"},
      };

    unsigned char actualOutput[256];

    for (size_t idx = 0; idx < ARRAY_SIZE(kData); ++idx) {

        memset(actualOutput, 0, sizeof(actualOutput));

        const char *input = (char *)kData[idx].input;

        int nCharsOut = sms_utf8_from_message_str( input,
                                                   strlen(input),
                                                   actualOutput,
                                                   kData[idx].outputBuffSize );

        EXPECT_EQ(nCharsOut, kData[idx].expectedRet);
        if (nCharsOut > 0) {
            char *expectedOutput = (char *)kData[idx].expectedUtf8;

            const char cmpResult = strncmp( (const char *)actualOutput,
                                            expectedOutput,
                                            nCharsOut );
            EXPECT_FALSE(cmpResult);
        }
    }
}

}  // namespace sms
