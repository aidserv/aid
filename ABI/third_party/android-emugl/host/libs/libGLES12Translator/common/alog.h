/* ARC MOD TRACK "third_party/android/system/core/include/log/log.h" */
/*
 * Copyright (C) 2005-2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
/* ARC MOD BEGIN */
/* Use the same defines as the Android headers to give theirs
 * precedence if both files are used.
 * For more info on Android logging, see:
 * http://elinux.org/Android_Logging_System
 */
#ifndef _COMMON_ALOG_H
#define _COMMON_ALOG_H

#include <common/log.h>
#include <stdarg.h>

extern "C" {

// TODO(khmel): Ideally include android/log.h here and use the ANDROID
// values instead. But recent private/libc_logging.h also defines ANDROID
// values. This causes compilation problems for some files which include both
// alog.h and libc_logging.h.
// These must match android/log.h values.
typedef enum arc_LogPriority {
  ARC_LOG_UNKNOWN = 0,
  ARC_LOG_DEFAULT,    /* only for SetMinPriority() */
  ARC_LOG_VERBOSE,
  ARC_LOG_DEBUG,
  ARC_LOG_INFO,
  ARC_LOG_WARN,
  ARC_LOG_ERROR,
  ARC_LOG_FATAL,
  ARC_LOG_SILENT     /* only for SetMinPriority(); must be last */
} arc_LogPriority;

/*
 * Log an assertion failure and SIGTRAP the process to have a chance
 * to inspect it, if a debugger is attached. This uses the FATAL priority.
 * It will now also submit the logging statement together with the crash log
 * (provided that the user has enabled crash reporting) as long as they signed
 * in using a tester email address.
 */
void __android_log_assert_with_source(const char* cond, const char *tag,
                                      const char* file, int line,
                                      const char* fmt, ...)
#if defined(__GNUC__)
  __attribute__ ((noreturn))
  __attribute__ ((format(printf, 5, 6)))
#endif
    ;

/*
 * Behaves the same as __android_log_assert_with_source. It also requires that
 * the user has enabled crash reporting, but does not perform the extra check to
 * see if they are using a tester email. Do not call this function unless you
 * can guarantee no privacy-sensitive data will be passed to it.
 */
void __android_log_assert_with_source_and_add_to_crash_report(
    const char* cond, const char *tag,
    const char* file, int line,
    const char* fmt, ...)
#if defined(__GNUC__)
  __attribute__ ((noreturn))
  __attribute__ ((format(printf, 5, 6)))
#endif
    ;

/*
 * Send a formatted string to the log, used like printf(fmt,...).
 * A version that uses va_list to list additional parameters.
 */
int __android_log_vprint(int prio, const char* tag,
                         const char* fmt, va_list args);

/*
 * A variant of __android_log_assert that takes a va_list to list
 * additional parameters.
 */
void __android_log_vassert(const char* cond, const char* tag,
                           const char* fmt, va_list args);

// These must match cutil/log.h values.
typedef enum {
  ARC_LOG_ID_MAIN = 0,
  ARC_LOG_ID_RADIO = 1,
  ARC_LOG_ID_EVENTS = 2,
  ARC_LOG_ID_SYSTEM = 3,
  ARC_LOG_ID_CRASH = 4,

  ARC_LOG_ID_MAX
} arc_log_id_t;

}

namespace arc {

enum CrashLogMessageKind {
  ReportableOnlyForTesters = 0,
  ReportableForAllUsers
};

typedef void (*AddCrashExtraInformationFunction)
    (CrashLogMessageKind crash_log_message_kind,
     const char* field_name, const char* message);
void RegisterCrashCallback(AddCrashExtraInformationFunction function);

int PrintLogBuf(int bufID, int prio, const char* tag, const char* fmt, ...)
#if defined(__GNUC__)
  __attribute__ ((format(printf, 4, 5)))
#endif
    ;

// Used to explicitly mark the return value of a function as unused. If you are
// really sure you don't want to do anything with the return value of a function
// that has been marked WARN_UNUSED_RESULT, wrap it with this. Example:
//
//   scoped_ptr<MyType> my_var = ...;
//   if (TakeOwnership(my_var.get()) == SUCCESS)
//     ignore_result(my_var.release());
//
// For ALOG's internal use only. Use the one in base/macros.h instead.
namespace alog_internal {
template <typename T>
inline void ignore_result(const T&) {}
}
}  // namespace arc
/* ARC MOD END */

// ---------------------------------------------------------------------

/*
 * Normally we strip ALOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV
#define __ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define ALOGV(...) do { if (0) { __ALOGV(__VA_ARGS__); } } while (0)
#else
#define ALOGV(...) __ALOGV(__VA_ARGS__)
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef ALOGV_IF
#if LOG_NDEBUG
#define ALOGV_IF(cond, ...)   ((void)0)
#else
#define ALOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

// ---------------------------------------------------------------------

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * verbose priority.
 */
#ifndef IF_ALOGV
#if LOG_NDEBUG
#define IF_ALOGV() if (false)
#else
#define IF_ALOGV() IF_ALOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * debug priority.
 */
#ifndef IF_ALOGD
#define IF_ALOGD() IF_ALOG(LOG_DEBUG, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * info priority.
 */
#ifndef IF_ALOGI
#define IF_ALOGI() IF_ALOG(LOG_INFO, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * warn priority.
 */
#ifndef IF_ALOGW
#define IF_ALOGW() IF_ALOG(LOG_WARN, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * error priority.
 */
#ifndef IF_ALOGE
#define IF_ALOGE() IF_ALOG(LOG_ERROR, LOG_TAG)
#endif


// ---------------------------------------------------------------------

/* ARC MOD BEGIN FORK */
/* Adding SLOG() macros in order to make SLOG*() functions only
 * evaluate their arguments when logging is going to happen.
 */
/*
 * Simplified macro to send a verbose system log message using the current LOG_TAG.
 */
#ifndef SLOGV
#if LOG_NDEBUG
#define SLOGV(...)   ((void)0)
#else
#define SLOGV(...) ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug system log message using the current LOG_TAG.
 */
#ifndef SLOGD
#define SLOGD(...) ((void)SLOG(ANDROID_LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an info system log message using the current LOG_TAG.
 */
#ifndef SLOGI
#define SLOGI(...) ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send a warning system log message using the current LOG_TAG.
 */
#ifndef SLOGW
#define SLOGW(...) ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an error system log message using the current LOG_TAG.
 */
#ifndef SLOGE
#define SLOGE(...) ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SLOG(ARC_LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
/* ARC MOD END FORK */

// ---------------------------------------------------------------------

/*
 * Log a fatal error.  If the given condition fails, this stops program
 * execution like a normal assertion, but also generating the given message.
 * It is NOT stripped from release builds.  Note that the condition test
 * is -inverted- from the normal assert() semantics.
 */
#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (CONDITION(cond)) \
/* ARC MOD BEGIN FORK */ \
    ? ((void)android_printAssertArc(#cond, LOG_TAG, ## __VA_ARGS__)) \
/* ARC MOD END FORK */ \
    : (void)0 )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
/* ARC MOD BEGIN FORK */ \
    ( ((void)android_printAssertArc(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif

// WARNING: The log message printed by these macros is also sent to the crash
// server, provided the user has agreed to do so. Do not use these macros unless
// you are absolutely sure that the message will not have any privacy-sensitive
// data. Otherwise, use LOG_ALWAYS_FATAL, which checks if the user has signed in
// using a tester account.
#ifndef LOG_ALWAYS_FATAL_AND_ADD_TO_CRASH_REPORT
#define LOG_ALWAYS_FATAL_AND_ADD_TO_CRASH_REPORT(...) \
    ( ((void)android_printAssertArcAddToCrashReport(NULL, LOG_TAG, \
                                                    ## __VA_ARGS__)) )
#endif

#ifndef LOG_ALWAYS_FATAL_AND_ADD_TO_CRASH_REPORT_IF
#define LOG_ALWAYS_FATAL_AND_ADD_TO_CRASH_REPORT_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ( ((void)android_printAssertArcAddToCrashReport(NULL, LOG_TAG, \
                                                     ## __VA_ARGS__)) ) \
    : (void)0 )
#endif
/* ARC MOD END FORK */
/*
 * Versions of LOG_ALWAYS_FATAL_IF and LOG_ALWAYS_FATAL that
 * are stripped out of release builds.
 */
#if LOG_NDEBUG

#ifndef LOG_FATAL_IF
/* ARC MOD BEGIN */
// Call ignore_result to reduce the number of ALLOW_UNUSED macros. See
// the comment for ignore_result above for detail.
#define LOG_FATAL_IF(cond, ...) \
    (::arc::alog_internal::ignore_result(false && (cond)))
/* ARC MOD END */
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

/*
 * Assertion that generates a log message when the assertion fails.
 * Stripped out of release builds.  Uses the current LOG_TAG.
 */
#ifndef ALOG_ASSERT
#define ALOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
//#define ALOG_ASSERT(cond) LOG_FATAL_IF(!(cond), "Assertion failed: " #cond)
#endif

// ---------------------------------------------------------------------

/*
 * Basic log message macro.
 *
 * Example:
 *  ALOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
/* ARC MOD BEGIN */
#ifndef ALOG
#define ALOG(priority, tag, ...) \
   arc::PrintLogBuf(ARC_LOG_ID_MAIN, ARC_##priority, \
                      tag, __VA_ARGS__)
#endif

#ifndef SLOG
#define SLOG(buf_id, priority, tag, ...) \
   arc::PrintLogBuf(buf_id, priority, tag, __VA_ARGS__)
#endif
/* ARC MOD END */

/*
 * ===========================================================================
 *
 * The stuff in the rest of this file should not be used directly.
 */

#define android_printLog(prio, tag, fmt...) \
    __android_log_print(prio, tag, fmt)

#define android_vprintLog(prio, cond, tag, fmt...) \
    __android_log_vprint(prio, tag, fmt)

/* XXX Macros to work around syntax errors in places where format string
 * arg is not passed to ALOG_ASSERT, LOG_ALWAYS_FATAL or LOG_ALWAYS_FATAL_IF
 * (happens only in debug builds).
 */

/* Returns 2nd arg.  Used to substitute default value if caller's vararg list
 * is empty.
 */
#define __android_second(dummy, second, ...)     second

/* If passed multiple args, returns ',' followed by all but 1st arg, otherwise
 * returns nothing.
 */
#define __android_rest(first, ...)               , ## __VA_ARGS__

/* ARC MOD BEGIN */
#define android_printAssertArc(cond, tag, fmt...) \
    __android_log_assert_with_source(cond, tag, __FILE__, __LINE__, \
        __android_second(0, ## fmt, NULL) __android_rest(fmt))

// Sends the last log message to the plugin just before aborting. It was decided
// to not send this during the crash reporting callback since the process is not
// guaranteed to be in a good state to allocate the memory needed to send the
// message. Also, there is no upstream support to add arbitrary data to the
// minidump.
// By sending this separately only in the cases when we know is safe, we
// maximize the probability of reporting the crash successfully.
#define android_printAssertArcAddToCrashReport(cond, tag, fmt...) \
    __android_log_assert_with_source_and_add_to_crash_report(cond, tag, \
        __FILE__, __LINE__, \
        __android_second(0, ## fmt, NULL) __android_rest(fmt))

#endif  // _COMMON_ALOG_H
/* ARC MOD END */
