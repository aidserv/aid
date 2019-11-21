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

#include "android/base/Debug.h"
#include "android/base/StringFormat.h"
#include "android/base/threads/Thread.h"

#include <string>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace base {

namespace {

// The current log output.
testing::LogOutput* gLogOutput = NULL;

bool gDcheckLevel = false;
LogSeverity gMinLogLevel = LOG_INFO;

// Convert a severity level into a string.
const char* severityLevelToString(LogSeverity severity) {
    const char* kSeverityStrings[] = {
        "INFO", "WARNING", "ERROR", "FATAL",
    };
    if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
        return kSeverityStrings[severity];
    if (severity == -1) {
        return "VERBOSE";
    }
    return "UNKNOWN";
}

// Default log output function
void defaultLogMessage(const LogParams& params,
                       const char* message,
                       size_t messageLen) {
    // Obtaining current thread's id is expensive. Only do this for verbose
    // logs.
    std::string tidStr;
    if (params.severity >= LOG_VERBOSE) {
        tidStr = android::base::StringFormat("[%lu]:", getCurrentThreadId());
    }

    fprintf(stderr,
            "%s%s:%s:%d:%.*s\n",
            tidStr.c_str(),
            severityLevelToString(params.severity),
            params.file,
            params.lineno,
            int(messageLen),
            message);
    // Note: by default, stderr is non buffered, but the program might
    // have altered this setting, so always flush explicitly to ensure
    // that the log is displayed as soon as possible. This avoids log
    // messages being lost when a crash happens, and makes debugging
    // easier. On the other hand, it means lots of logging will impact
    // performance.
    fflush(stderr);

    if (params.severity >= LOG_FATAL) {
        if (IsDebuggerAttached()) {
            android::base::DebugBreak();
        }
        _exit(1);
    }
}

void logMessage(const LogParams& params,
                const char* message,
                size_t messageLen) {
    if (gLogOutput) {
        gLogOutput->logMessage(params, message, messageLen);
    } else {
        defaultLogMessage(params, message, messageLen);
    }
}

}  // namespace

// DCHECK level.

bool dcheckIsEnabled() {
    return gDcheckLevel;
}

bool setDcheckLevel(bool enabled) {
    bool ret = gDcheckLevel;
    gDcheckLevel = enabled;
    return ret;
}

// LogSeverity

LogSeverity getMinLogLevel() {
    return gMinLogLevel;
}

void setMinLogLevel(LogSeverity level) {
    gMinLogLevel = level;
}

// LogString

LogString::LogString(const char* fmt, ...) : mString(NULL) {
    size_t capacity = 100;
    char* message = reinterpret_cast<char*>(::malloc(capacity));
    for (;;) {
        va_list args;
        va_start(args, fmt);
        int ret = vsnprintf(message, capacity, fmt, args);
        va_end(args);
        if (ret >= 0 && size_t(ret) < capacity)
            break;
        capacity *= 2;
    }
    mString = message;
}

LogString::~LogString() {
    ::free(mString);
}

// LogStream

LogStream::LogStream(const char* file, int lineno, LogSeverity severity) :
        mParams(file, lineno, severity),
        mString(NULL),
        mSize(0),
        mCapacity(0) {}

LogStream::~LogStream() {
    mSize = 0;
    mCapacity = 0;
    ::free(mString);
}

LogStream& LogStream::operator<<(char ch) {
    if (ch >= 32 && ch < 127) {
        append(&ch, 1U);
    } else {
        char temp[5];
        snprintf(temp, sizeof temp, "\\x%02x", ch);
        append(temp, 4U);
    }
    return *this;
}

LogStream& LogStream::operator<<(const void* ptr) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%p", ptr);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(int v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%d", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(unsigned v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%u", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(long v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%ld", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%lu", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(long long v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%lld", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
    char temp[20];
    int ret = snprintf(temp, sizeof temp, "%llu", v);
    append(temp, static_cast<size_t>(ret));
    return *this;
}

LogStream& LogStream::operator<<(android::base::StringView v) {
    if (!v.empty()) {
        append(v.c_str(), v.size());
    }
    return *this;
}

void LogStream::append(const char* str) {
    if (str && str[0])
        append(str, strlen(str));
}

void LogStream::append(const char* str, size_t len) {
    if (!len || len > INT32_MAX)
        return;

    size_t newSize = mSize + len;
    if (newSize > mCapacity) {
        size_t newCapacity = mCapacity;
        while (newCapacity < newSize)
            newCapacity += (newCapacity >> 2) + 32;
        mString = reinterpret_cast<char*>(
                ::realloc(mString, newCapacity + 1));
        mCapacity = newCapacity;
    }
    ::memcpy(mString + mSize, str, len);
    mSize += len;
    mString[mSize] = '\0';
}

// LogMessage

LogMessage::LogMessage(const char* file, int line, LogSeverity severity) :
        mStream(new LogStream(file, line, severity)) {}

LogMessage::~LogMessage() {
    logMessage(mStream->params(),
               mStream->string(),
               mStream->size());
    delete mStream;
}

// ErrnoLogMessage

ErrnoLogMessage::ErrnoLogMessage(const char* file,
                                 int line,
                                 LogSeverity severity,
                                 int errnoCode) :
        mStream(NULL), mErrno(errnoCode) {
    mStream = new LogStream(file, line, severity);
}

ErrnoLogMessage::~ErrnoLogMessage() {
    (*mStream) << "Error message: " << strerror(mErrno);
    logMessage(mStream->params(),
               mStream->string(),
               mStream->size());
    delete mStream;
    // Restore the errno.
    errno = mErrno;
}

// LogOutput

namespace testing {

// static
LogOutput* LogOutput::setNewOutput(LogOutput* newOutput) {
    LogOutput* ret = gLogOutput;
    gLogOutput = newOutput;
    return ret;
}

}  // namespace testing

}  // naemspace base
}  // namespace android
