// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/threads/FunctorThread.h"

#include <gtest/gtest.h>

using android::base::FunctorThread;

static int intFunc() {
    return 43;
}

static intptr_t intptrFunc() {
    return intptr_t(&intFunc);
}

static int paramFunc(int i) {
    return 0;
}

TEST(FunctorThreadTest, TestCtor) {
    // this test actually just makes sure all constructor syntaxes compile fine
    FunctorThread t1{intFunc};
    FunctorThread t2{intptrFunc};
    FunctorThread t3{std::bind(paramFunc, 2)};
    FunctorThread t4{FunctorThread::Functor(intFunc)};
    FunctorThread t5{[](){ return -1;}};
}

TEST(FunctorThreadTest, TestRun) {
    bool ran = false;
    FunctorThread t([&ran]{ ran = true; return 0; });
    EXPECT_FALSE(ran);
    EXPECT_TRUE(t.start());
    EXPECT_TRUE(t.wait());
    EXPECT_TRUE(ran);
}
