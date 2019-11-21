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

#include "android/filesystems/fstab_parser.h"

#include <gtest/gtest.h>

TEST(FstabParser, ParsePartitionFormat) {
    static const char kFstab[] =
        "# Android fstab file.\n"
        "#<src>                                                  <mnt_point>         <type>    <mnt_flags and options>                              <fs_mgr_flags>\n"
        "# The filesystem that contains the filesystem checker binary (typically /system) cannot\n"
        "# specify MF_CHECK, and must come before any filesystems that do specify MF_CHECK\n"
        "/dev/block/mtdblock0                                    /system             ext4      ro,barrier=1                                         wait\n"
        "/dev/block/mtdblock1\t      \t                          /data\t             yaffs2    noatime,nosuid,nodev,barrier=1,nomblk_io_submit      wait,check\n"
        "/dev/block/mtdblock2\t/cache\tntfs\tnoatime,nosuid,nodev  wait,check\n"
        "/devices/platform/goldfish_mmc.0\t\t\tauto\tvfat      defaults                                             voldmanaged=sdcard:auto\n"
        ;
    static const size_t kFstabSize = sizeof(kFstab);

    char* out = NULL;
    EXPECT_TRUE(android_parseFstabPartitionFormat(kFstab, kFstabSize,
                                                  "/system", &out));
    EXPECT_STREQ("ext4", out);
    free(out);

    EXPECT_TRUE(android_parseFstabPartitionFormat(kFstab, kFstabSize,
                                                  "/data", &out));
    EXPECT_STREQ("yaffs2", out);
    free(out);

    EXPECT_TRUE(android_parseFstabPartitionFormat(kFstab, kFstabSize,
                                                  "/cache", &out));
    EXPECT_STREQ("ntfs", out);
    free(out);

    EXPECT_FALSE(android_parseFstabPartitionFormat(kFstab, kFstabSize,
                                                   "/unknown", &out));
}
