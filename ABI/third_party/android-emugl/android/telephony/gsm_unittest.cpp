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

#include "android/telephony/gsm.h"

#include <gtest/gtest.h>

namespace gsm {

TEST(Gsm, Utf8CheckGsm7) {
    char utf8In[] = "01234567890123456789012345678901234567890123456789"  // [  0] .. [ 49]
                    "0123456789012345678901234***8901234567890123456789"  // [ 50] .. [ 99]
                    "01234567890123456789012345678901234567890123456789"  // [100] .. [149]
                    "01234567890123456789012345678901234567890123456789"; // [150] .. [199]

    utf8In[75] = 0xE2; // Insert a UTF-8 'Euro currency' symbol
    utf8In[76] = 0x82;
    utf8In[77] = 0xAC;

    // The Euro symbol is a three-byte UTF-8 symbol and a
    // two-byte GSM-7 symbol.

    cbytes_t start = (cbytes_t)&utf8In[0];

    int isGsm7 = utf8_check_gsm7(start, strlen(utf8In));
    EXPECT_TRUE(isGsm7);

    cbytes_t end     = (cbytes_t)(start + strlen(utf8In));
    int      gsm7len = 154;

    // The "Euro" generates 2 output GSM-7 bytes from 3 input
    // bytes.
    // The other ("regular") input bytes give 1 GSM-7 byte
    // each, so 152 input bytes are be needed to generate 152
    // output bytes.
    // In total, this test case needs 155 input bytes to
    // generate 154 output bytes.
    cbytes_t result = utf8_skip_gsm7(start, end, gsm7len);
    EXPECT_EQ((result - start), 155);
}

TEST(Gsm, Utf8CheckUcs2) {
    char utf8In[] = "01234567890123456789012345678901234567890123456789"  // [  0] .. [ 49]
                    "0**34567890123456789012345678901234567890123456789"  // [ 50] .. [ 99]
                    "01234567890123456789012345678901234567890123456789"  // [100] .. [149]
                    "01234567890123456789012345678901234567890123456789"; // [150] .. [199]

    utf8In[51] = 0xC2; // Insert a UTF-8 'cent' symbol
    utf8In[52] = 0xA2;

    // The cent symbol is not in GSM-7. Its presence forces
    // the SMS message to use UCS-2.

    cbytes_t start   = (cbytes_t)&utf8In[0];

    int isGsm7 = utf8_check_gsm7(start, strlen(utf8In));
    EXPECT_FALSE(isGsm7);

    cbytes_t end     = (cbytes_t)(start + strlen(utf8In));
    int      ucs2len = 124;

    // The "cent" generates 2 output UCS-2 bytes from 2 input
    // bytes.
    // The other ("regular") input bytes give 2 UCS-2 bytes
    // each, so 61 input bytes are be needed to generate 122
    // output bytes.
    // In total, this test case needs 63 input bytes to
    // generate 124 output bytes.
    cbytes_t result = utf8_skip_ucs2(start, end, ucs2len);
    EXPECT_EQ((result - start), 63);
}

}  // namespace gsm
