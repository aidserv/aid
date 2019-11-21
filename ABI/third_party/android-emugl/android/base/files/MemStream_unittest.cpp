// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/MemStream.h"

#include <gtest/gtest.h>

#include <stdint.h>

namespace android {
namespace base {

TEST(MemStream, write) {
    MemStream stream;
    int32_t val = 1;
    EXPECT_EQ(sizeof(val), stream.write(&val, sizeof(val)));
    EXPECT_EQ(sizeof(val), stream.writtenSize());
}

TEST(MemStream, read) {
    int32_t val = 1;
    MemStream::Buffer buffer(sizeof(val));
    memcpy(buffer.data(), &val, sizeof(val));
    MemStream stream(std::move(buffer));
    EXPECT_EQ(sizeof(val), stream.readSize());
    val = 0;
    EXPECT_EQ(sizeof(val), stream.read(&val, sizeof(val)));
    EXPECT_EQ(1, val);
    EXPECT_EQ(0, stream.readSize());
}

TEST(MemStream, sizes) {
    MemStream stream;
    EXPECT_EQ(0, stream.writtenSize());
    EXPECT_EQ(0, stream.readSize());

    stream.putBe64(1);
    EXPECT_EQ(8, stream.writtenSize());
    EXPECT_EQ(8, stream.readSize());

    stream.getBe16();
    EXPECT_EQ(8, stream.writtenSize());
    EXPECT_EQ(6, stream.readSize());
    EXPECT_EQ(2, stream.readPos());

    stream.putBe16(2);
    EXPECT_EQ(10, stream.writtenSize());
    EXPECT_EQ(8, stream.readSize());
    EXPECT_EQ(2, stream.readPos());

    stream.getBe64();
    EXPECT_EQ(10, stream.writtenSize());
    EXPECT_EQ(0, stream.readSize());
    EXPECT_EQ(10, stream.readPos());
}

TEST(MemStream, saveLoad) {
    MemStream stream;
    const int val = 1;
    const float fval = 2.;
    const StringView sval = "333 as string";
    stream.putBe32(val);
    stream.putFloat(fval);
    stream.putString(sval);

    MemStream saveStream;
    stream.save(&saveStream);

    MemStream stream2;
    stream2.load(&saveStream);
    EXPECT_EQ(val, stream2.getBe32());
    EXPECT_EQ(fval, stream2.getFloat());
    EXPECT_STREQ(sval.c_str(), stream2.getString().c_str());
}

}  // namespace base
}  // namespace android
