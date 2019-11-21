/* Copyright (C) 2016 The Android Open Source Project
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

#include "android/telephony/TagLengthValue.h"

#include <gtest/gtest.h>

namespace android {

TEST(TagLengthValue, ConstructDeviceAppIdRefDo) {
    DeviceAppIdRefDo deviceAppIdRefDo("C0FFEE");

    // Ensure the correct tag is used and that the length and data are correct
    ASSERT_STREQ(/*tag*/ "C1" /*length*/ "03" /*payload*/ "C0FFEE",
                 deviceAppIdRefDo.c_str());
}

TEST(TagLengthValue, ConstructPkgRefDo) {
    PkgRefDo pkgRefDo("com.android.foo");

    ASSERT_STREQ(/*tag*/ "CA" /*length*/ "0f"
                 /*payload*/ "636f6d2e616e64726f69642e666f6f",
                 pkgRefDo.c_str());
}

TEST(TagLengthValue, ConstructRefDo) {
    RefDo refDo{DeviceAppIdRefDo("C0FFEE")};

    ASSERT_STREQ(/*tag*/ "E1" /*length*/ "05" /*payload*/ "C103C0FFEE",
                 refDo.c_str());
}

TEST(TagLengthValue, ConstructRefDoWithAidRefDo) {
    RefDo refDo{AidRefDo(), DeviceAppIdRefDo("C0FFEE")};

    // Note that the AID-REF-DO does not end up in the payload because we
    // construct an empty object.
    ASSERT_STREQ(/*tag*/ "E1" /*length*/ "05" /*payload*/ "C103C0FFEE",
                 refDo.c_str());
}

TEST(TagLengthValue, ConstructRefDoWithPkgRefDo) {
    RefDo refDo{DeviceAppIdRefDo("C0FFEE"), PkgRefDo("foo")};

    ASSERT_STREQ(/*tag*/ "E1" /*length*/ "0a"
                 /*payload*/ "C103C0FFEECA03666f6f",
                 refDo.c_str());
}

TEST(TagLengthValue, ConstructApduArDo) {
    ApduArDo globalDenyRule(ApduArDo::Allow::Never);
    ASSERT_STREQ(/*tag*/ "D0" /*length*/ "01" /*payload*/ "00",
                 globalDenyRule.c_str());

    ApduArDo globalAllowRule(ApduArDo::Allow::Always);
    ASSERT_STREQ(/*tag*/ "D0" /*length*/ "01" /*payload*/ "01",
                 globalAllowRule.c_str());

    // These rules are actually supposed to have a better format than this
    // but this will work for testing.
    ApduArDo specificRule( {"C0FFEE", "BEEF"} );
    ASSERT_STREQ(/*tag*/ "D0" /*length*/ "05" /*payload*/ "C0FFEEBEEF",
                 specificRule.c_str());
}

TEST(TagLengthValue, ConstructNfcArDo) {
    NfcArDo never(NfcArDo::Allow::Never);
    ASSERT_STREQ(/*tag*/ "D1" /*length*/ "01" /*payload*/ "00", never.c_str());

    NfcArDo always(NfcArDo::Allow::Always);
    ASSERT_STREQ(/*tag*/ "D1" /*length*/ "01" /*payload*/ "01", always.c_str());
}

TEST(TagLengthValue, ConstructPermArDo) {
    PermArDo permArDo("C0FFEE");

    ASSERT_STREQ(/*tag*/ "DB" /*length*/ "03" /*payload*/ "C0FFEE",
                 permArDo.c_str());
}

TEST(TagLengthValue, ConstructArDo) {
    const ApduArDo apduArDo( {"C0FFEE", "BEEF"} );
    const NfcArDo nfcArDo(NfcArDo::Allow::Always);
    const PermArDo permArDo("DECADE");

    ArDo arDoWithApdu(apduArDo);
    ASSERT_STREQ(/*tag*/ "E3" /*length*/ "07" /*payload*/ "D005C0FFEEBEEF",
                 arDoWithApdu.c_str());

    ArDo arDoWithNfc(nfcArDo);
    ASSERT_STREQ(/*tag*/ "E3" /*length*/ "03" /*payload*/ "D10101",
                 arDoWithNfc.c_str());

    ArDo arDoWithPerm(permArDo);
    ASSERT_STREQ(/*tag*/ "E3" /*length*/ "05" /*payload*/ "DB03DECADE",
                 arDoWithPerm.c_str());

    ArDo arDoWithApduAndNfc(apduArDo, nfcArDo);
    ASSERT_STREQ(/*tag*/ "E3" /*length*/ "0a"
                 /*payload*/ "D005C0FFEEBEEFD10101",
                 arDoWithApduAndNfc.c_str());
}

TEST(TagLengthValue, ConstructRefArDo) {
    RefDo refDo(AidRefDo(), DeviceAppIdRefDo("C0FFEE"));
    ArDo arDo{NfcArDo{NfcArDo::Allow::Always}};

    RefArDo refArDo(refDo, arDo);
    ASSERT_STREQ(/*tag*/ "E2" /*length*/ "0c"
                 /*payload*/ "E105C103C0FFEEE303D10101", refArDo.c_str());
}

TEST(TagLengthValue, ConstructAllRefArDo) {
    // Create an ALL-REF-AR-DO with two REF-AR-DOs in it
    AllRefArDo allRefArDo {
        RefArDo {
            RefDo {
                AidRefDo(),
                DeviceAppIdRefDo { "C0FFEE" }
            },
            ArDo {
                ApduArDo { ApduArDo::Allow::Never }
            }
        },
        RefArDo {
            RefDo {
                AidRefDo { },
                DeviceAppIdRefDo { "BEEF" }
            },
            ArDo {
                ApduArDo { ApduArDo::Allow::Always },
                NfcArDo { NfcArDo::Allow::Never }
            }
        }
    };

    ASSERT_STREQ(/*tag*/ "FF40" /*length*/ "1e"
                 /*1st REF-AR-DO*/ "E20cE105C103C0FFEEE303D00100"
                 /*2nd REF-AR-DO*/ "E20eE104C102BEEFE306D00101D10100",
                 allRefArDo.c_str());
}

TEST(TagLengthValue, TestLongLenths) {
    // Create a string that's longer than 128 bytes, this will force the length
    // part of the resulting string to span two bytes, see implementation for
    // details on why this is. Lengths are multiplied by two because each byte
    // is represented by two characters.
    std::string twoByteString(2 * 0xa0, 'x');
    DeviceAppIdRefDo twoByteTlv(twoByteString);
    ASSERT_EQ(twoByteTlv.c_str(), strstr(twoByteTlv.c_str(), "C181a0x"));

    // Longer than 255 bytes requires three byte length
    std::string threeByteString(2 * 0x123, 'x');
    DeviceAppIdRefDo threeByteTlv(threeByteString);
    ASSERT_EQ(threeByteTlv.c_str(), strstr(threeByteTlv.c_str(), "C1820123x"));

    // Longer than 65535 bytes requires four byte length
    std::string fourByteString(2 * 0x10312, 'x');
    DeviceAppIdRefDo fourByteTlv(fourByteString);
    ASSERT_EQ(fourByteTlv.c_str(), strstr(fourByteTlv.c_str(), "C183010312x"));

    /*
     This test is disabled because of excessive memory usage and slow
     performance. At the time of writing it works but since it's unlikely
     we will ever be returning data larger than 16 MB skipping this should
     be OK
    // Longer than 16 * 65535 requires five byte length
    std::string fiveByteString(2 * 0x1001002, 'x');
    DeviceAppIdRefDo fiveByteTlv(fiveByteString);
    ASSERT_EQ(fiveByteTlv.c_str(),
              strstr(fiveByteTlv.c_str(), "C18401001002x"));
    */
}

}  // namespace android

