#include "android/utils/iolooper.h"

#include "android/base/sockets/SocketWaiter.h"

#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>

using namespace android::base;

namespace {

SocketWaiter* asWaiter(IoLooper* iol) {
    return reinterpret_cast<SocketWaiter*>(iol);
}

unsigned toWaiterEvents(int flags) {
    unsigned events = 0;
    if (flags & IOLOOPER_READ) {
        events |= SocketWaiter::kEventRead;
    }
    if (flags & IOLOOPER_WRITE) {
        events |= SocketWaiter::kEventWrite;
    }
    return events;
}

}  // namespace

IoLooper* iolooper_new(void) {
    return reinterpret_cast<IoLooper*>(SocketWaiter::create());
}

void iolooper_free(IoLooper*  iol) {
    delete asWaiter(iol);
}

void iolooper_reset(IoLooper*  iol) {
    asWaiter(iol)->reset();
}

void iolooper_modify(IoLooper* iol, int fd, int oldflags, int newflags) {
    asWaiter(iol)->update(fd, toWaiterEvents(newflags));
}

void iolooper_add_read(IoLooper* iol, int fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events | SocketWaiter::kEventRead);
}

void iolooper_add_write(IoLooper* iol, int fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events | SocketWaiter::kEventWrite);
}

void iolooper_del_read(IoLooper*  iol, int  fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events & ~SocketWaiter::kEventRead);
}

void iolooper_del_write(IoLooper*  iol, int  fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events & ~SocketWaiter::kEventWrite);
}

int iolooper_wait(IoLooper* iol, int64_t  duration) {
    return asWaiter(iol)->wait(duration);
}

int iolooper_is_read(IoLooper* iol, int fd) {
    return asWaiter(iol)->pendingEventsFor(fd) & SocketWaiter::kEventRead;
}

int iolooper_is_write(IoLooper* iol, int  fd) {
    return asWaiter(iol)->pendingEventsFor(fd) & SocketWaiter::kEventWrite;
}

int iolooper_has_operations(IoLooper* iol) {
    return asWaiter(iol)->hasFds();
}

int64_t iolooper_now(void) {
    struct timeval time_now;
    return gettimeofday(&time_now, NULL) ? -1 : (int64_t)time_now.tv_sec * 1000LL +
                                                time_now.tv_usec / 1000;
}

int iolooper_wait_absolute(IoLooper* iol, int64_t deadline) {
    int64_t timeout = deadline - iolooper_now();

    /* If the deadline has passed, set the timeout to 0, this allows us
     * to poll the file descriptor nonetheless */
    if (timeout < 0)
        timeout = 0;

    return iolooper_wait(iol, timeout);
}
