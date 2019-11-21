// Copyright 2015 The Android Open Source Project
// //
// // This software is licensed under the terms of the GNU General Public
// // License version 2, as published by the Free Software Foundation, and
// // may be copied, distributed, and modified under those terms.
// //
// // This program is distributed in the hope that it will be useful,
// // but WITHOUT ANY WARRANTY; without even the implied warranty of
// // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// // GNU General Public License for more details.

#include "android/utils/timezone.h"
#include "android/base/testing/Utils.h"

#include <gtest/gtest.h>
using android::base::System;
TEST(TimeZone, setTimeZoneToEST) {
    //this test relies on the windows registry and it won't work on wine
    SKIP_TEST_ON_WINE();
    // Try to set AVD's timezone for at most 3 times and if it succeeds, execute
    // the following tests.
    for (int i = 0; i < 3; i++) {
        if (!timezone_set("America/New_York")) {
            // utc time for 2017-10-01 00:00:00
            time_t current_time = 1506816000;
            EXPECT_EQ(-60 * 60 * 4, android_tzoffset_in_seconds(&current_time));
            struct tm local = *android_localtime(&current_time);
            EXPECT_EQ(2017 - 1900, local.tm_year);
            EXPECT_EQ(8, local.tm_mon);
            EXPECT_EQ(30, local.tm_mday);
            EXPECT_EQ(20, local.tm_hour);
            EXPECT_EQ(0, local.tm_min);
            EXPECT_EQ(0, local.tm_sec);
            EXPECT_EQ(1, local.tm_isdst);

            // change current time to utc time for 2016-12-10 00:00:00
            current_time = 1481328000;

            EXPECT_EQ(-60 * 60 * 5, android_tzoffset_in_seconds(&current_time));
            local = *android_localtime(&current_time);
            EXPECT_EQ(2016 - 1900, local.tm_year);
            EXPECT_EQ(11, local.tm_mon);
            EXPECT_EQ(9, local.tm_mday);
            EXPECT_EQ(19, local.tm_hour);
            EXPECT_EQ(0, local.tm_min);
            EXPECT_EQ(0, local.tm_sec);
            EXPECT_EQ(0, local.tm_isdst);
            break;
        }
    }
}

TEST(TimeZone, setTimeZoneToChinaStandardTime) {
    //this test relies on the windows registry and it won't work on wine
    SKIP_TEST_ON_WINE();
    // Try to set AVD's timezone for at most 3 times and if it succeeds, execute
    // the following tests.
    for (int i = 0; i < 3; i++) {
        if (!timezone_set("Asia/Shanghai")) {
            // utc time for 2017-10-01 00:00:00
            time_t current_time = 1506816000;
            EXPECT_EQ(60 * 60 * 8, android_tzoffset_in_seconds(&current_time));
            struct tm local = *android_localtime(&current_time);
            EXPECT_EQ(2017 - 1900, local.tm_year);
            EXPECT_EQ(9, local.tm_mon);
            EXPECT_EQ(1, local.tm_mday);
            EXPECT_EQ(8, local.tm_hour);
            EXPECT_EQ(0, local.tm_min);
            EXPECT_EQ(0, local.tm_sec);
            EXPECT_EQ(-1, local.tm_isdst);

            // change current time to utc time for 2016-12-10 00:00:00
            current_time = 1481328000;

            EXPECT_EQ(60 * 60 * 8, android_tzoffset_in_seconds(&current_time));
            local = *android_localtime(&current_time);
            EXPECT_EQ(2016 - 1900, local.tm_year);
            EXPECT_EQ(11, local.tm_mon);
            EXPECT_EQ(10, local.tm_mday);
            EXPECT_EQ(8, local.tm_hour);
            EXPECT_EQ(0, local.tm_min);
            EXPECT_EQ(0, local.tm_sec);
            EXPECT_EQ(-1, local.tm_isdst);
        }
    }
}

TEST(TimeZone, setTimeZoneToIndiaStandardTime) {
    //this test relies on the windows registry and it won't work on wine
    SKIP_TEST_ON_WINE();
    // Try to set AVD's timezone for at most 3 times and if it succeeds, execute
    // the following tests.
    for (int i = 0; i < 3; i++) {
        if (!timezone_set("Asia/Calcutta")) {
            // utc time for 2016-10-13 21:55:18
            time_t current_time = 1476395718;
            EXPECT_EQ(60 * 60 * 5 + 60 * 30,
                      android_tzoffset_in_seconds(&current_time));
            struct tm local = *android_localtime(&current_time);
            EXPECT_EQ(2016 - 1900, local.tm_year);
            EXPECT_EQ(9, local.tm_mon);
            EXPECT_EQ(14, local.tm_mday);
            EXPECT_EQ(3, local.tm_hour);
            EXPECT_EQ(25, local.tm_min);
            EXPECT_EQ(18, local.tm_sec);
            EXPECT_EQ(-1, local.tm_isdst);

            // change current time to utc time for 2016-12-10 00:00:00
            current_time = 1481328000;

            EXPECT_EQ(60 * 60 * 5 + 60 * 30,
                      android_tzoffset_in_seconds(&current_time));
            local = *android_localtime(&current_time);
            EXPECT_EQ(2016 - 1900, local.tm_year);
            EXPECT_EQ(11, local.tm_mon);
            EXPECT_EQ(10, local.tm_mday);
            EXPECT_EQ(5, local.tm_hour);
            EXPECT_EQ(30, local.tm_min);
            EXPECT_EQ(0, local.tm_sec);
            EXPECT_EQ(-1, local.tm_isdst);
        }
    }
}

