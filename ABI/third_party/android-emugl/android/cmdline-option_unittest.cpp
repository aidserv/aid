// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/memory/ScopedPtr.h"
#include "android/cmdline-option.h"
#include "android/utils/debug.h"

#include <gtest/gtest.h>

TEST(CmdLineOptions, parsePort) {
    struct {
        const char* input;
        bool expectSuccess;
        int expectConsolePort;
        int expectAdbPort;
    } kData[] = {
        {"5554", true, 5554, 5555},
        {"5555", true, 5555, 5556},  // Odd port.
        {"5552", true, 5552, 5553},  // Port below lower bound of 5554.
        {"5553", true, 5553, 5554},  // Port below lower bound of 5554.
        {"6000", true, 6000, 6001},  // Port above upper bound.
        {"6001", true, 6001, 6002},  // Even port above range
        {"foo", false, 0, 0},
        {"5554trailingText", false, 0, 0},
        {"", false, 0, 0},
        {nullptr, false, 0, 0},
    };

    for (const auto& data : kData) {
        int consolePort = 0;
        int adbPort = 0;
        bool result = android_parse_port_option(data.input, &consolePort,
                                                &adbPort);
        EXPECT_EQ(data.expectSuccess, result)
                << "Failed on test case: (" << data.input << ")";
        EXPECT_EQ(data.expectConsolePort, consolePort)
                << "Failed on test case: (" << data.input << ")";
        EXPECT_EQ(data.expectAdbPort, adbPort)
                << "Failed on test case: (" << data.input << ")";
    }
}

TEST(CmdLineOptions, parsePorts) {
    struct {
        const char* input;
        bool expectSuccess;
        int expectConsolePort;
        int expectAdbPort;
    } kData[] = {
        {"5554,5555", true, 5554, 5555},
        {"1,200", true, 1, 200},           // No restrictions on ports
        {"65000,7", true, 65000, 7},
        {"-2,5555", false, 0, 0},          // No negative values
        {"5554,-42", false, 0, 0},         // No negative values
        {" 5554, 5555", true, 5554, 5555}, // Leading spaces are fine
        {"5554 ,5555", false, 0, 0},       // But not trailing spaces
        {"5554,5555 ", false, 0, 0},       // But not trailing spaces
        {"5554,5555trailingText", false, 0, 0},
        {"5554trailingtext,5555", false, 0, 0},
        {"345,", false, 0, 0},
        {",678", false, 0, 0},
        {"123", false, 0, 0},
        {"123 456", false, 0, 0},
        {"0,234", false, 0, 0},            // No ports below 1
        {"234,0", false, 0, 0},
        {"65536,234", false, 0, 0},        // No ports above 65535
        {"234,65536", false, 0, 0},
        {nullptr, false, 0, 0},
    };

    for (const auto& data : kData) {
        int consolePort = 0, adbPort = 0;
        bool result = android_parse_ports_option(data.input,
                                                 &consolePort,
                                                 &adbPort);
        EXPECT_EQ(data.expectSuccess, result)
                << "Failed on test case: (" << data.input << ")";
        EXPECT_EQ(data.expectConsolePort, consolePort)
                << "Failed on test case: (" << data.input << ")";
        EXPECT_EQ(data.expectAdbPort, adbPort)
                << "Failed on test case: (" << data.input << ")";
    }
}

TEST(CmdLineOptions, validatePorts) {
    struct {
        int consolePortInput;
        int adbPortInput;
        bool expectSuccess;
    } kData[] = {
        {5554, 5555, true},
        {5553, 5554, false},
        {5552, 5553, false},
        {5584, 5585, true},  // At least 16 port combinations are supported.
        {5555, 5556, false},  // Odd ports not allowed.
        {5552, 5553, false},  // Port below lower bound.
        {5553, 5554, false},  // Port below lower bound.
        {6000, 6001, false},  // Port above upper bound.
        {6001, 6002, false},  // Port above upper bound.
    };

    for (const auto& data : kData) {
        bool result = android_validate_ports(data.consolePortInput,
                                             data.adbPortInput);
        EXPECT_EQ(data.expectSuccess, result)
                << "Failed on test case: (" << data.consolePortInput
                << "," << data.adbPortInput << ")";
    }
}


TEST(CmdLineOptions, parseDebug) {
    struct {
        const char* option;
        bool parseAsSuffix;
        bool expectedResult;
        uint64_t initialFlags;
        uint64_t parsedFlags;
    } kData[] = {
        { NULL, false, false, 0, 0 },
        { NULL, true, false, 0, 0 },
        { "", false, 0, 0 },
        { "all", false, true, 0, -1ull },
        { "-all", false, true, 1, 0 },
        { "all", true, true, 0, -1ull },
        { "-all", true, false, 1, 1 },
        { "no-all", false, true, 1, 0 },
        { "noall", false, false, 1, 1 },
        { "init", false, true, 0, 1 },
        { "init", true, true, 0, 1 },
        { "-init", true, false, 1, 1 },
        { "all,no-init", false, true, 0, -1ull & ~1 },
        { "all,no-init", true, false, 0, 0 },
        { "init,blah,-foo,no-bar", false, true, 0, 1 },
        { "init,blah,-foo,no-bar", true, false, 0, 0 }
    };

    // Make sure the android_verbose debug flags are restored even if any test
    // case fails.
    const auto oldFlags = android_verbose;
    auto restoreFlags = android::base::makeCustomScopedPtr(nullptr,
        [oldFlags](std::nullptr_t*) { android_verbose = oldFlags; });

    for (const auto& data : kData) {
        android_verbose = data.initialFlags;
        bool result = android_parse_debug_tags_option(data.option,
                                                      data.parseAsSuffix);
        EXPECT_EQ(data.expectedResult, result)
                << "Failed on test case: ("
                << (data.option ? data.option : "null") << ")";
        EXPECT_EQ(data.parsedFlags, android_verbose)
                << "Failed on test case: ("
                << (data.option ? data.option : "null") << ")";
    }
}
