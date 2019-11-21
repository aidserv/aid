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

#include "android/base/Log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <gtest/gtest.h>

namespace android {
namespace base {

// Create a severity level which is guaranteed to never generate a log
// message. See LogOnlyEvaluatesArgumentsIfNeeded for usage.
const LogSeverity LOG_INVISIBLE = static_cast<LogSeverity>(-10000);

class LogTest : public ::testing::Test, android::base::testing::LogOutput {
public:
    LogTest() : mFatal(false) {
        mSavedOutput = ::android::base::testing::LogOutput::setNewOutput(this);
        mExpected[0] = '\0';
        mBuffer[0] = '\x7f';
        mBuffer[1] = '\0';
    }

    ~LogTest() {
        ::android::base::testing::LogOutput::setNewOutput(mSavedOutput);
    }

    void setExpected(LogSeverity severity, int line, const char* suffix) {
        mExpectedParams.file = __FILE__;
        mExpectedParams.lineno = line;
        mExpectedParams.severity = severity;
        snprintf(mExpected, sizeof(mExpected), "%s", suffix);
    }

    // LogOutput override
    void logMessage(const LogParams& params,
                    const char* message,
                    size_t messageLen) {
        mParams = params;
        if (messageLen > sizeof(mBuffer) - 1)
            messageLen = sizeof(mBuffer) - 1;
        ::memcpy(mBuffer, message, messageLen);
        mBuffer[messageLen] = '\0';
        mFatal = (params.severity >= LOG_FATAL);
    }

protected:
    ::android::base::testing::LogOutput* mSavedOutput;
    LogParams mParams;
    LogParams mExpectedParams;
    char mExpected[1024];
    char mBuffer[1024];
    bool mFatal;
};

class CheckTest : public LogTest {
};

#if ENABLE_DCHECK != 0
class DCheckEnabledTest : public LogTest {
public:
    DCheckEnabledTest() : LogTest() {
        // Ensure DCHECKS() always run.
        mSavedLevel = setDcheckLevel(true);
    }

    ~DCheckEnabledTest() {
        setDcheckLevel(mSavedLevel);
    }
private:
    bool mSavedLevel;
};
#endif  // ENABLE_DCHECK == 0

#if ENABLE_DCHECK != 2
class DCheckDisabledTest : public LogTest {
public:
    DCheckDisabledTest() : LogTest() {
        mSavedLevel = setDcheckLevel(false);
    }

    ~DCheckDisabledTest() {
        setDcheckLevel(mSavedLevel);
    }
private:
    bool mSavedLevel;
};
#endif  // ENABLE_DCHECK != 2

class PLogTest : public LogTest {
public:
    PLogTest() : LogTest(), mForcedErrno(-1000) {}

    void setForcedErrno(int errnoCode) {
        mForcedErrno = errnoCode;
    }

    void setExpectedErrno(LogSeverity severity,
                          int line,
                          int errnoCode,
                          const char* suffix) {
        mExpectedParams.file = __FILE__;
        mExpectedParams.lineno = line;
        mExpectedParams.severity = severity;
        snprintf(mExpected,
                 sizeof(mExpected),
                 "%sError message: %s",
                 suffix,
                 strerror(errnoCode));
    }

    void logMessage(const LogParams& params,
                    const char* message,
                    size_t messageLen) {
        LogTest::logMessage(params, message, messageLen);

        if (mForcedErrno != -1000)
            errno = mForcedErrno;
    }

protected:
    int mForcedErrno;
};

#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

#define EXPECTED_STRING_PREFIX(prefix, line) \
  prefix ":" __FILE__ ":" STRINGIFY(line) ": "

#define CHECK_EXPECTATIONS() \
    EXPECT_STREQ(mExpectedParams.file, mParams.file); \
    EXPECT_EQ(mExpectedParams.lineno, mParams.lineno); \
    EXPECT_EQ(mExpectedParams.severity, mParams.severity); \
    EXPECT_STREQ(mExpected, mBuffer)

// Helper function used to set a boolean |flag|, then return |string|.
static const char* setFlag(bool* flag, const char* string) {
    *flag = true;
    return string;
}

TEST(LogString, EmptyString) {
    LogString ls("");
    EXPECT_STREQ("", ls.string());
}

TEST(LogString, SimpleString) {
    LogString ls("Hello");
    EXPECT_STREQ("Hello", ls.string());
}

TEST(LogString, FormattedString) {
    LogString ls("%d plus %d equals %d", 12, 23, 35);
    EXPECT_STREQ("12 plus 23 equals 35", ls.string());
}

TEST_F(LogTest, LogInfoEmpty) {
    setExpected(LOG_INFO, __LINE__ + 1, "");
    LOG(INFO);
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithString) {
    static const char kString[] = "Hello World!";
    setExpected(LOG_INFO, __LINE__ + 1, kString);
    LOG(INFO) << kString;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithTwoStrings) {
    setExpected(LOG_INFO, __LINE__ + 1, "Hello Globe!");
    LOG(INFO) << "Hello " << "Globe!";
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithLogString) {
    LogString ls("Hello You!");
    setExpected(LOG_INFO, __LINE__ + 1, ls.string());
    LOG(INFO) << ls;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogWarning) {
    static const char kWarning[] = "Elvis has left the building!";
    setExpected(LOG_WARNING, __LINE__ + 1, kWarning);
    LOG(WARNING) << kWarning;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogError) {
    static const char kError[] = "Bad Bad Robot!";
    setExpected(LOG_ERROR, __LINE__ + 1, kError);
    LOG(ERROR) << kError;

    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogFatal) {
    static const char kFatalMessage[] = "I'm dying";
    setExpected(LOG_FATAL, __LINE__ + 1, kFatalMessage);
    LOG(FATAL) << kFatalMessage;
    CHECK_EXPECTATIONS();
    EXPECT_TRUE(mFatal);
}

TEST_F(LogTest, LogEvaluatesArgumentsIfNeeded) {
    // Use LOG_FATAL since it is always active.
    bool flag = false;
    setExpected(LOG_FATAL, __LINE__ + 1, "PANIC: Flag was set!");
    LOG(FATAL) << "PANIC: " << setFlag(&flag, "Flag was set!");
    CHECK_EXPECTATIONS();
    EXPECT_TRUE(mFatal);
    EXPECT_TRUE(flag);
}

TEST_F(LogTest, LogOnlyEvaluatesArgumentsIfNeeded) {
    bool flag = false;
    LOG(INVISIBLE) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
}


// TODO(digit): Convert this to a real death test when this is supported
// by our version of GTest.
TEST_F(CheckTest, CheckFalse) {
    setExpected(LOG_FATAL, __LINE__ + 1, "Check failed: false. ");
    CHECK(false);
    CHECK_EXPECTATIONS();
}

TEST_F(CheckTest, CheckFalseEvaluatesArguments) {
    bool flag = false;
    setExpected(LOG_FATAL, __LINE__ + 1, "Check failed: false. Flag was set!");
    CHECK(false) << setFlag(&flag, "Flag was set!");
    EXPECT_TRUE(flag);
    CHECK_EXPECTATIONS();
}

TEST_F(CheckTest, CheckTrue) {
    CHECK(true);
    EXPECT_FALSE(mFatal);
}

TEST_F(CheckTest, CheckTrueDoesNotEvaluateArguments) {
    bool flag = false;
    CHECK(true) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}

#if ENABLE_DCHECK != 0
TEST_F(DCheckEnabledTest, DCheckIsOnReturnsTrue) {
    EXPECT_TRUE(DCHECK_IS_ON());
}

TEST_F(DCheckEnabledTest, DCheckFalse) {
    bool flag = false;
    setExpected(LOG_FATAL, __LINE__ + 1, "Check failed: false. Flag was set!");
    DCHECK(false) << setFlag(&flag, "Flag was set!");
    CHECK_EXPECTATIONS();
}

TEST_F(DCheckEnabledTest, DCheckTrue) {
    bool flag = false;
    DCHECK(true) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}
#endif  // ENABLE_DCHECK != 0

#if ENABLE_DCHECK != 2
TEST_F(DCheckDisabledTest, DCheckIsOnReturnsFalse) {
    EXPECT_FALSE(DCHECK_IS_ON());
}

TEST_F(DCheckDisabledTest, DCheckFalse) {
    bool flag = false;
    DCHECK(false) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}

TEST_F(DCheckDisabledTest, DCheckTrue) {
    DCHECK(true);
}
#endif  // ENABLE_DCHECK != 2

TEST_F(PLogTest, PLogInfoEmpty) {
    setExpectedErrno(LOG_INFO, __LINE__ + 2, EINVAL, "");
    errno = EINVAL;
    PLOG(INFO);
    CHECK_EXPECTATIONS();
}

TEST_F(PLogTest, PLogInfoPreservesErrno) {
    // Select a value that is unlikely to ever be raised by the logging
    // machinery.
    const int kErrnoCode = ENOEXEC;
    setForcedErrno(EINVAL);
    setExpectedErrno(LOG_INFO, __LINE__ + 2, kErrnoCode, "Hi");
    errno = kErrnoCode;
    PLOG(INFO) << "Hi";
    EXPECT_EQ(kErrnoCode, errno);
    CHECK_EXPECTATIONS();
}

#if ENABLE_DLOG
TEST_F(PLogTest, DPlogInfoEmpty) {
    setExpectedErrno(LOG_INFO, __LINE__ + 2, EINVAL, "");
    errno = EINVAL;
    DPLOG(INFO);
    CHECK_EXPECTATIONS();
}

TEST_F(PLogTest, DPlogInfoPreservesErrno) {
    // Select a value that is unlikely to ever be raised by the logging
    // machinery.
    const int kErrnoCode = ENOEXEC;
    setForcedErrno(EINVAL);
    setExpectedErrno(LOG_INFO, __LINE__ + 2, kErrnoCode, "Hi");
    errno = kErrnoCode;
    DPLOG(INFO) << "Hi";
    EXPECT_EQ(kErrnoCode, errno);
    CHECK_EXPECTATIONS();
}

#endif

}  // namespace base
}  // namespace android
