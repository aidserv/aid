/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ChecksumCalculatorThreadInfo.h"

#include "emugl/common/crash_reporter.h"
// #include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

#include <atomic>
#include <string>

#include <assert.h>
#include <stdio.h>

namespace {

class ChecksumCalculatorThreadStore : public ::emugl::ThreadStore {
public:
    ChecksumCalculatorThreadStore() : ::emugl::ThreadStore(NULL) {}
};

#ifdef TRACE_CHECKSUMHELPER
std::atomic<size_t> sNumInstances(0);
// size_t sNumInstances = 0;
#endif  // TRACE_CHECKSUMHELPER
}

// static ::emugl::LazyInstance<ChecksumCalculatorThreadStore> s_tls = LAZY_INSTANCE_INIT;
ChecksumCalculatorThreadStore g_tls;
ChecksumCalculatorThreadStore *s_tls = &g_tls;

static ChecksumCalculatorThreadInfo* getChecksumCalculatorThreadInfo() {
    return static_cast<ChecksumCalculatorThreadInfo*>(s_tls->get());
}

ChecksumCalculatorThreadInfo::ChecksumCalculatorThreadInfo() {
    LOG_CHECKSUMHELPER("%s: Checksum thread created (%u instances)\n",
                       __FUNCTION__, (unsigned int)(++sNumInstances));
    s_tls->set(this);
}

ChecksumCalculatorThreadInfo::~ChecksumCalculatorThreadInfo() {
    LOG_CHECKSUMHELPER("%s: GLprotocol destroyed (%u instances)\n",
                       __FUNCTION__, (unsigned int)(--sNumInstances));
    s_tls->set(NULL);
}

ChecksumCalculator& ChecksumCalculatorThreadInfo::get() {
    return getChecksumCalculatorThreadInfo()->m_protocol;
}

bool ChecksumCalculatorThreadInfo::setVersion(uint32_t version) {
    return getChecksumCalculatorThreadInfo()->m_protocol.setVersion(version);
}

bool ChecksumCalculatorThreadInfo::writeChecksum(ChecksumCalculator* calc,
                                                 void* buf,
                                                 size_t bufLen,
                                                 void* outputChecksum,
                                                 size_t outputChecksumLen) {
    calc->addBuffer(buf, bufLen);
    return calc->writeChecksum(outputChecksum, outputChecksumLen);
}

bool ChecksumCalculatorThreadInfo::validate(ChecksumCalculator* calc,
                                            void* buf,
                                            size_t bufLen,
                                            void* checksum,
                                            size_t checksumLen) {
    calc->addBuffer(buf, bufLen);
    return calc->validate(checksum, checksumLen);
}

void ChecksumCalculatorThreadInfo::validOrDie(ChecksumCalculator* calc,
                                              void* buf,
                                              size_t bufLen,
                                              void* checksum,
                                              size_t checksumLen,
                                              const char* message) {
    if (!validate(calc, buf, bufLen, checksum, checksumLen)) {
        //TODO:SHENG remove emugl_crash_reporter(message);
		printf("ChecksumCalculatorThreadInfo::validOrDie\n");
        ;
    }
}
