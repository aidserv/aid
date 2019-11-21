/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/utils/filelock.h"

#include "android/base/system/System.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/lock.h"
#include "android/utils/path.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#  include "android/base/memory/ScopedPtr.h"
#  include "android/base/system/Win32UnicodeString.h"
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <unistd.h>
#  include <signal.h>
#endif

using android::base::System;

// Set to 1 to enable debug traces here.
#if 0
#define  D(...)  printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

/** FILE LOCKS SUPPORT
 **
 ** A FileLock is useful to prevent several emulator instances from using
 ** the same writable file (e.g. the userdata.img disk images).
 **
 ** Create a FileLock object with filelock_create(), this function should
 ** return NULL only if the corresponding file path could not be locked.
 **
 ** All file locks are automatically released and destroyed when the program
 ** exits. The filelock_lock() function can also detect stale file locks
 ** that can linger when the emulator crashes unexpectedly, and will happily
 ** clean them for you
 **
 ** Here's how it works, three files are used:
 **     file  - the data file accessed by the emulator
 **     lock  - a lock file  (file + '.lock')
 **     temp  - a temporary file made unique with mkstemp
 **
 ** When locking:
 **      create 'temp' and store our pid in it
 **      attemp to link 'lock' to 'temp'
 **         if the link succeeds, we obtain the lock
 **      unlink 'temp'
 **
 ** When unlocking:
 **      unlink 'lock'
 **
 **/


/* Thread safety:
 *     _all_filelocks_tl: Hold this when accessing |_all_filelocks|.
 *                        This is locked at exit, so hold this for very, *very*
 *                        short.
 */
static CLock* _all_filelocks_tl;
/* This is set to true during exit.
 * All accesses of _all_filelocks should strictly follow this scheme:
 *   android_lock_acquire(_all_filelocks_tl);
 *   if (!_is_exiting) {
 *     // Safe to access _all_filelocks now.
 *   }
 *   android_lock_release(_all_filelocks_tl);
 */
static bool _is_exiting = false;

struct FileLock
{
  const char*  file;
  const char*  lock;
  char*        temp;
#ifdef _WIN32
  HANDLE       lock_handle;
#endif
  int          locked;
  FileLock*    next;
};

/* used to cleanup all locks at emulator exit */
static FileLock*   _all_filelocks;

#define  LOCK_NAME   ".lock"
#define  TEMP_NAME   ".tmp-XXXXXX"
#define  WIN_PIDFILE_NAME  "pid"

void
filelock_init() {
    _all_filelocks_tl = android_lock_new();
}

#ifdef _WIN32
template <class Func>
static bool retry(Func func, int tries = 4, int timeoutMs = 100) {
    for (;;) {
        if (func()) {
            return true;
        }
        if (--tries == 0) {
            return false;
        }
        System::get()->sleepMs(timeoutMs);
    }
}

static bool delete_file(const android::base::Win32UnicodeString& name) {
    return retry([&name]() {
        // Make sure the file isn't marked readonly, or deletion may fail.
        SetFileAttributesW(name.c_str(), FILE_ATTRIBUTE_NORMAL);
        return ::DeleteFileW(name.c_str()) != 0 ||
               ::GetLastError() == ERROR_FILE_NOT_FOUND;
    });
}

static bool delete_dir(const android::base::Win32UnicodeString& name) {
    return retry([&name]() {
        return ::RemoveDirectoryW(name.c_str()) != 0 ||
               ::GetLastError() == ERROR_FILE_NOT_FOUND;
    });
}

/* returns 0 on success, -1 on failure */
static int filelock_lock(FileLock* lock) {
    lock->lock_handle = nullptr;
    const auto unicodeDir = android::base::Win32UnicodeString(lock->lock);
    const auto unicodeName = android::base::Win32UnicodeString(lock->temp);

    HANDLE lockHandle = INVALID_HANDLE_VALUE;
    const bool createFileResult = retry(
        [&lockHandle, &unicodeDir, &unicodeName]() {
            if (!::CreateDirectoryW(unicodeDir.c_str(), nullptr) &&
                ::GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }

            // Open/create a lock file with a flags combination like:
            //  - open for writing only
            //  - allow only one process to open file for writing (our process)
            // Together, this guarantees that the file can only be opened when
            // our process is alive and keeps a handle to it.
            // As soon as our process ends or we close/delete it - other lock
            // can be acquired.
            // Note: FILE_FLAG_DELETE_ON_CLOSE doesn't work well here, as
            //  everyone else would need to open the file in FILE_SHARE_DELETE
            //  mode, and both Android Studio and cmd.exe don't use it. Fix
            //  at least the Android Studio part before trying to add this flag
            //  instead of the manual deletion code.
            lockHandle = ::CreateFileW(
                    unicodeName.c_str(),
                    GENERIC_WRITE,    // open only for writing
                    FILE_SHARE_READ,  // allow others to read the file, but not
                                      // to write to it or not to delete it
                    nullptr,          // no special security attributes
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                    nullptr);  // no template file
            if (lockHandle == INVALID_HANDLE_VALUE &&
                GetLastError() == ERROR_ACCESS_DENIED) {
                // Sometimes a previous file gets readonly attribute even when
                // its parent process has exited; that prevents us from opening
                // it for writing.
                SetFileAttributesW(unicodeName.c_str(), FILE_ATTRIBUTE_NORMAL);
            }
            return lockHandle != INVALID_HANDLE_VALUE;
        },
        5,      // tries
        200);   // sleep timeout between tries

    if (!createFileResult) {
        assert(lockHandle == INVALID_HANDLE_VALUE);
        return -1;
    }

    assert(lockHandle != INVALID_HANDLE_VALUE);

    // Make sure we kill the file/directory on any failure.
    auto fileDeleter = android::base::makeCustomScopedPtr(
            lockHandle, [&unicodeName, &unicodeDir](HANDLE h) {
                ::CloseHandle(h);
                delete_file(unicodeName);
                delete_dir(unicodeDir);
            });

    // We're good. Now write down the process ID as Android Studio relies on it.
    const DWORD pid = ::GetCurrentProcessId();
    char pidBuf[12];
    const int len = snprintf(pidBuf, sizeof(pidBuf), "%lu", pid);
    assert(len > 0 && len < (int)sizeof(pidBuf));
    DWORD bytesWritten;
    if (!::WriteFile(lockHandle, pidBuf, static_cast<DWORD>(len), &bytesWritten,
                     nullptr) ||
        bytesWritten != static_cast<DWORD>(len)) {
        D("Failed to write the current PID into the lock file");
        return -1;
    }
    lock->locked = 1;
    lock->lock_handle = fileDeleter.release();
    return 0;  // we're all good
}
#else
/* returns 0 on success, -1 on failure */
static int filelock_lock(FileLock* lock) {
    int    ret;
    int    temp_fd = -1;
    int    lock_fd = -1;
    int    rc, tries;
    FILE*  f = NULL;
    char   pid[8];
    struct stat  st_temp;
    int sleep_duration_ms = 0;

    temp_fd = mkstemp(lock->temp);

    if (temp_fd < 0) {
        D("Cannot create locking temp file '%s'", lock->temp);
        goto Fail;
    }

    snprintf(pid, sizeof pid, "%d", getpid());
    ret = HANDLE_EINTR(write(temp_fd, pid, strlen(pid) + 1));
    if (ret < 0) {
        D("Cannot write to locking temp file '%s'", lock->temp);
        goto Fail;
    }
    close(temp_fd);
    temp_fd = -1;

    rc = HANDLE_EINTR(lstat(lock->temp, &st_temp));
    if (rc < 0) {
        D("Can't properly stat our locking temp file '%s'", lock->temp);
        goto Fail;
    }

    /* now attempt to link the temp file to the lock file */
    for (tries = 4; tries > 0; tries--)
    {
        const int kSleepDurationMsMax = 2000;  // 2 seconds.
        const int kSleepDurationMsIncrement = 50;

        if (sleep_duration_ms > 0) {
            if (sleep_duration_ms > kSleepDurationMsMax) {
                D("Cannot acquire lock file '%s'", lock->lock);
                goto Fail;
            }
            System::get()->sleepMs(sleep_duration_ms);
        }
        sleep_duration_ms += kSleepDurationMsIncrement;

        // The return value of link() is buggy on NFS, so ignore it.
        // and use lstat() to look at the result.
        rc = HANDLE_EINTR(link(lock->temp, lock->lock));

        struct stat st_lock;
        rc = HANDLE_EINTR(lstat(lock->lock, &st_lock));
        if (rc != 0) {
            // Try again after sleeping a little.
            continue;
        }

        if (st_temp.st_rdev == st_lock.st_rdev &&
            st_temp.st_ino  == st_lock.st_ino  ) {
            /* The link() operation suceeded */
            lock->locked = 1;
            rc = HANDLE_EINTR(unlink(lock->temp));
            return 0;
        }

        if (S_ISDIR(st_lock.st_mode)) {
            char *win_pid;
            int win_pid_len;
            // The .lock file is a directory. This can only happen
            // when the AVD was previously used by a Win32 emulator
            // instance running under Wine on the same machine.
            fprintf(stderr,
                    "Stale Win32 lock file detected: %s\n",
                    lock->lock);

            /* Try deleting the pid file dropped in windows.
             * Ignore error -- try blowing away the directory anyway.
             */
            win_pid_len = strlen(lock->lock) + 1 + sizeof(WIN_PIDFILE_NAME);
            win_pid = (char *) malloc(win_pid_len);
            snprintf(win_pid, win_pid_len, "%s/" WIN_PIDFILE_NAME, lock->lock);
            HANDLE_EINTR(unlink(win_pid));
            free(win_pid);

            rc = HANDLE_EINTR(rmdir(lock->lock));
            if (rc != 0) {
                D("Removing stale Win32 lockfile '%s' failed (%s)",
                  lock->lock, strerror(errno));
            }

            goto Fail;
        }

        /* if we get there, it means that the link() call failed */
        /* check the lockfile to see if it is stale              */
        typedef enum {
            FRESHNESS_UNKNOWN = 0,
            FRESHNESS_FRESH,
            FRESHNESS_STALE,
        } Freshness;

        Freshness freshness = FRESHNESS_UNKNOWN;

        struct stat st;
        time_t now;
        rc = HANDLE_EINTR(time(&now));
        st.st_mtime = now - 120;

        int lockpid = 0;
        int lockfd = HANDLE_EINTR(open(lock->lock,O_RDONLY));
        if (lockfd >= 0) {
            char buf[16];
            int len = HANDLE_EINTR(read(lockfd, buf, sizeof(buf) - 1U));
            if (len < 0) {
                len = 0;
            }
            buf[len] = 0;
            lockpid = atoi(buf);

            rc = HANDLE_EINTR(fstat(lockfd, &st));
            if (rc == 0) {
                now = st.st_atime;
            }
            IGNORE_EINTR(close(lockfd));
        }
        /* if there is a PID, check that it is still alive */
        if (lockpid > 0) {
            rc = HANDLE_EINTR(kill(lockpid, 0));
            if (rc == 0 || errno == EPERM) {
                freshness = FRESHNESS_FRESH;
            } else if (rc < 0 && errno == ESRCH) {
                freshness = FRESHNESS_STALE;
            }
        }
        if (freshness == FRESHNESS_UNKNOWN) {
            /* no pid, stale if the file is older than 1 minute */
            freshness = (now >= st.st_mtime + 60) ?
                    FRESHNESS_STALE :
                    FRESHNESS_FRESH;
        }

        if (freshness == FRESHNESS_STALE) {
            D("Removing stale lockfile '%s'", lock->lock);
            rc = HANDLE_EINTR(unlink(lock->lock));
            sleep_duration_ms = 0;
            tries++;
        }
    }
    D("file '%s' is already in use by another process", lock->file);

Fail:
    if (f) {
        fclose(f);
    }

    if (temp_fd >= 0) {
        IGNORE_EINTR(close(temp_fd));
    }

    if (lock_fd >= 0) {
        IGNORE_EINTR(close(lock_fd));
    }

    HANDLE_EINTR(unlink(lock->temp));
    return -1;
}
#endif

void
filelock_release( FileLock*  lock )
{
    if (lock->locked) {
#ifdef _WIN32
        ::CloseHandle(lock->lock_handle);
        lock->lock_handle = nullptr;
        delete_file(android::base::Win32UnicodeString(lock->temp));
        delete_dir(android::base::Win32UnicodeString(lock->lock));
#else
        unlink( (char*)lock->lock );
#endif
        free((char*)lock->file);
        lock->file = lock->lock = lock->temp = nullptr;
        lock->locked = 0;
    }
}

static void
filelock_atexit( void )
{
  android_lock_acquire(_all_filelocks_tl);
  if (!_is_exiting) {
    for (FileLock* lock = _all_filelocks; lock != NULL; ) {
        filelock_release(lock);
        FileLock* const prev = lock;
        lock = lock->next;
        free(prev);
    }
  }
  _is_exiting = true;
  android_lock_release(_all_filelocks_tl);
  // We leak |_all_filelocks_tl| here. We can never guarantee that another
  // thread isn't trying to use filelock concurrently, so we can not safely
  // clean up the mutex. See b.android.com/209635
}

/* create a file lock */
FileLock*
filelock_create( const char*  file )
{
    int    file_len = strlen(file);
    int    lock_len = file_len + sizeof(LOCK_NAME);
#ifdef _WIN32
    int    temp_len = lock_len + 1 + sizeof(WIN_PIDFILE_NAME);
#else
    int    temp_len = file_len + sizeof(TEMP_NAME);
#endif
    const int paths_len = file_len + lock_len + temp_len + 3;

    FileLock* const lock = (FileLock*)malloc(sizeof(*lock));
    char* const paths = (char*)malloc(paths_len);

    lock->file = paths;
    memcpy( (char*)lock->file, file, file_len+1 );

    lock->lock = lock->file + file_len + 1;
    memcpy( (char*)lock->lock, file, file_len+1 );
    strcat( (char*)lock->lock, LOCK_NAME );

    lock->temp    = (char*)lock->lock + lock_len + 1;
#ifdef _WIN32
    snprintf( (char*)lock->temp, temp_len, "%s\\" WIN_PIDFILE_NAME,
              lock->lock );
#else
    snprintf((char*)lock->temp, temp_len, "%s%s", lock->file, TEMP_NAME);
#endif
    lock->locked = 0;

    if (filelock_lock(lock) < 0) {
        free(paths);
        free(lock);
        return NULL;
    }

    android_lock_acquire(_all_filelocks_tl);
    if (_is_exiting) {
        android_lock_release(_all_filelocks_tl);
        filelock_release(lock);
        free(lock);
        return NULL;
    }

    lock->next     = _all_filelocks;
    _all_filelocks = lock;
    android_lock_release(_all_filelocks_tl);

    if (lock->next == NULL)
        atexit( filelock_atexit );

    return lock;
}
