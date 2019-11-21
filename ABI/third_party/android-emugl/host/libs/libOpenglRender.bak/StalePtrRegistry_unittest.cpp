// Copyright (C) 2017 The Android Open Source Project
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

#include "StalePtrRegistry.h"

#include <gtest/gtest.h>

TEST(StalePtrRegistry, Constructor) {
    StalePtrRegistry<void> reg;
}

TEST(StalePtrRegistry, Add) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    reg.addPtr(nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 1);
}

TEST(StalePtrRegistry, AddRemove) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    reg.addPtr(nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 1);
    reg.removePtr(nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 0);
}

TEST(StalePtrRegistry, AddMakeStale) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    reg.addPtr(nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.numStaleEntries(), 0);
    reg.makeCurrentPtrsStale();
    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);
}

TEST(StalePtrRegistry, Get) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    reg.addPtr(nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.getPtr(0), nullptr);
}

TEST(StalePtrRegistry, GetDefaultVal) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.getPtr(0), nullptr);
    EXPECT_EQ(reg.getPtr(0, (void*)0x1), (void*)0x1);
}

TEST(StalePtrRegistry, GetNonExisting) {
    StalePtrRegistry<void> reg;
    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.getPtr(0), nullptr);
    EXPECT_EQ(reg.numCurrEntries(), 0);
}

TEST(StalePtrRegistry, AddMakeStaleGetStale) {
    StalePtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_EQ(reg.numCurrEntries(), 0);

    reg.addPtr(ptr);

    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.numStaleEntries(), 0);

    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);

    EXPECT_EQ(reg.getPtr(handle), ptr);
}

TEST(StalePtrRegistry, AddMakeStaleGetStaleWithDelete) {
    StalePtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_EQ(reg.numCurrEntries(), 0);

    reg.addPtr(ptr);

    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.numStaleEntries(), 0);

    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);

    EXPECT_EQ(reg.getPtr(handle, nullptr, true), ptr);
    EXPECT_EQ(reg.getPtr(handle, nullptr, true), nullptr);
}

TEST(StalePtrRegistry, AddMakeStaleWithRemap) {
    StalePtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    void* remapped = (void*)0xbcdefa;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_EQ(reg.numCurrEntries(), 0);

    reg.addPtr(ptr);

    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.numStaleEntries(), 0);

    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);

    reg.remapStalePtr(handle, remapped);

    EXPECT_EQ(reg.getPtr(handle), remapped);
}

TEST(StalePtrRegistry, AddMakeStaleWithRemapNameCollision) {
    StalePtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    void* remapped = (void*)0xbcdefa;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_EQ(reg.numCurrEntries(), 0);

    reg.addPtr(ptr);

    EXPECT_EQ(reg.numCurrEntries(), 1);
    EXPECT_EQ(reg.numStaleEntries(), 0);

    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);

    reg.remapStalePtr(handle, remapped);

    EXPECT_EQ(reg.getPtr(handle), remapped);

    reg.addPtr(ptr);

    EXPECT_EQ(reg.getPtr(handle), ptr);
}

TEST(StalePtrRegistry, AddMakeStaleTwice) {
    StalePtrRegistry<void> reg;
    void* ptr1 = (void*)0xabcdef;
    uint64_t handle1 = (uint64_t)(uintptr_t)ptr1;
    void* ptr2 = (void*)0xbcdefa;
    uint64_t handle2 = (uint64_t)(uintptr_t)ptr2;

    reg.addPtr(ptr1);
    reg.makeCurrentPtrsStale();
    EXPECT_EQ(reg.numStaleEntries(), 1);
    reg.addPtr(ptr2);
    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 2);

    EXPECT_EQ(reg.getPtr(handle1), ptr1);
    EXPECT_EQ(reg.getPtr(handle2), ptr2);
}

TEST(StalePtrRegistry, AddMakeStaleTwiceWithCollision) {
    StalePtrRegistry<void> reg;
    void* ptr1 = (void*)0xabcdef;
    uint64_t handle1 = (uint64_t)(uintptr_t)ptr1;
    void* ptr2 = (void*)0xabcdef;
    uint64_t handle2 = (uint64_t)(uintptr_t)ptr2;

    reg.addPtr(ptr1);
    reg.makeCurrentPtrsStale();
    EXPECT_EQ(reg.numStaleEntries(), 1);
    reg.addPtr(ptr2);
    reg.makeCurrentPtrsStale();

    EXPECT_EQ(reg.numCurrEntries(), 0);
    EXPECT_EQ(reg.numStaleEntries(), 1);

    EXPECT_EQ(reg.getPtr(handle1), ptr1);
    EXPECT_EQ(reg.getPtr(handle2), ptr2);
}
