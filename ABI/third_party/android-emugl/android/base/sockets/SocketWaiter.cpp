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

#include "android/base/sockets/SocketWaiter.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketErrors.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#  include <sys/types.h>
#  include <sys/select.h>
#endif


#include <errno.h>
#include <string.h>

namespace android {
namespace base {

namespace {

class SelectSocketWaiter : public SocketWaiter {
public:
    SelectSocketWaiter() : SocketWaiter() {
        reset();
    }

    virtual ~SelectSocketWaiter() {}

    virtual void reset() {
        FD_ZERO(mReads);
        FD_ZERO(mWrites);
        FD_ZERO(mReadsResult);
        FD_ZERO(mWritesResult);
        mMaxFd = -1;
        mMaxFdValid = true;
        mPendingFd = 0;
    }

    virtual unsigned wantedEventsFor(int fd) const {
        if (!isValidFd(fd)) {
            return 0U;
        }
        // Compute current flags for fd.
        unsigned events = 0;
        if (FD_ISSET(fd, mReads)) {
            events |= kEventRead;
        }
        if (FD_ISSET(fd, mWrites)) {
            events |= kEventWrite;
        }
        return events;
    }

    virtual unsigned pendingEventsFor(int fd) const {
        if (!isValidFd(fd)) {
            return 0U;
        }
        // Compute current flags for fd.
        unsigned events = 0;
        if (FD_ISSET(fd, mReadsResult)) {
            events |= kEventRead;
        }
        if (FD_ISSET(fd, mWritesResult)) {
            events |= kEventWrite;
        }
        return events;
    }

    virtual bool hasFds() const {
        return getFdCount() > 0;
    }

    virtual void update(int fd, unsigned events) {
        DCHECK(isValidFd(fd)) << "fd " << fd << " max " << FD_SETSIZE;

        // Compute current flags for fd.
        unsigned oldEvents = wantedEventsFor(fd);
        if (events == oldEvents) {
            return;
        }

        // Compare changed flags.
        unsigned changed = (events ^ oldEvents);
        if ((changed & kEventRead) != 0) {
            if ((events & kEventRead) != 0) {
                setFdInSet(fd, mReads);
            } else {
                clearFdInSet(fd, mReads);
            }
        }
        if ((changed & kEventWrite) != 0) {
            if ((events & kEventWrite) != 0) {
                setFdInSet(fd, mWrites);
            } else {
                clearFdInSet(fd, mWrites);
            }
        }

        // Update mMaxFd / mMaxFdValid
        if (oldEvents == 0 && mMaxFdValid && fd > mMaxFd) {
            mMaxFd = fd;
        } else if (events == 0 && mMaxFdValid && fd == mMaxFd) {
            mMaxFdValid = false;
        }
    }

    virtual int wait(int64_t timeout_ms) {
        int count = getFdCount();

        mPendingFd = 0;

        // Nothing to wait on.
        if (count <= 0) {
            return 0;
        }

       // The Mac version of select() will return EINVAL on timeouts larger
       // than 100000000, so clamp it instead since it's unlikely we're
       // going to wait this long (27.777. hours).
#if _DARWIN_C_SOURCE
        if (timeout_ms != INT64_MAX && timeout_ms > 100000000000LL) {
            timeout_ms = 100000000000LL;
        }
#endif

        // Compute timeout pointer.
        struct timeval tm0, *tm;
        if (timeout_ms < 0 || timeout_ms == INT64_MAX) {
            tm = NULL;
        } else {
            tm0.tv_sec = timeout_ms / 1000;
            tm0.tv_usec = (timeout_ms - 1000 * tm0.tv_sec) * 1000;
            tm = &tm0;
        }

        int ret;
        do {
            fd_set errs;
            FD_ZERO(&errs);

            mReadsResult[0] = mReads[0];
            mWritesResult[0] = mWrites[0];

            ret = ::select(count, mReadsResult, mWritesResult, &errs, tm);
            if (ret == 0) {
                errno = ETIMEDOUT;
            }
        } while (ret < 0 && errno == EINTR);

        if (ret < 0) {
            LOG(ERROR) << LogString("Error: %s\n", strerror(errno));
        }
        return ret;
    }

    virtual int nextPendingFd(unsigned *fdEvents) {
        int count = getFdCount();
        int fd = mPendingFd;

        while (fd < count) {
            ++fd;
            unsigned events = 0;
            if (FD_ISSET(fd, mReadsResult)) {
                events |= kEventRead;
            }
            if (FD_ISSET(fd, mWritesResult)) {
                events |= kEventWrite;
            }
            if (events) {
                *fdEvents = events;
                mPendingFd = fd;
                return fd;
            }
        }

        *fdEvents = 0;
        return -1;
    }

    // Return true iff |fd| is a valid file descriptor.
    bool isValidFd(int fd) const {
#ifdef _WIN32
        return (unsigned)fd < FD_SETSIZE;
#else
        return fd >= 0 && fd < FD_SETSIZE;
#endif
    }

    void setFdInSet(int fd, fd_set* set) {
#ifdef _WIN32
        FD_SET((unsigned)fd, set);
#else
        FD_SET(fd, set);
#endif
    }

    void clearFdInSet(int fd, fd_set* set) {
#ifdef _WIN32
        FD_CLR((unsigned)fd, set);
#else
        FD_CLR(fd, set);
#endif
    }

    // Return the number of file descriptors to wait on during select().
    int getFdCount() const {
        int maxFd = mMaxFd;

        if (mMaxFdValid) {
            return maxFd + 1;
        }

        // Recompute the maximum file descriptor.
        maxFd = -1;
        for (int fd = 0; fd < FD_SETSIZE; ++fd) {
            if (!FD_ISSET(fd, mReads) && !FD_ISSET(fd, mWrites)) {
                continue;
            }
            maxFd = fd;
        }

        mMaxFd = maxFd;
        mMaxFdValid = true;

        return maxFd + 1;
    }

private:
    fd_set mReads[1];
    fd_set mWrites[1];
    fd_set mReadsResult[1];
    fd_set mWritesResult[1];
    mutable int mMaxFd;
    mutable bool mMaxFdValid;
    int mPendingFd;
};

}  // namespace

// static
SocketWaiter* SocketWaiter::create() {
    return new SelectSocketWaiter();
}

}  // namespace base
}  // namespace android
