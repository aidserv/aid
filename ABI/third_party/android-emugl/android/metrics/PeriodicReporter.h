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

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/Compiler.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/metrics/MetricsReporter.h"

#include <functional>
#include <list>
#include <map>
#include <memory>

namespace android {
namespace metrics {

// PeriodicReporter - a class that helps one to implement metrics reporting
// task that needs to run recurrently, with a specific timeout between the runs.
//
// This class optimizes the number of messages sent by combining all tasks added
// with the same reporting peroid into a single metrics message.
//
// To create such task, call the addTask() method, supplying the run period and
// the task callback.
// Task callback is invoked as a regular MetricsReporter callback; it may return
// |false| if there's no need to log anything at the current invocation.
//
// If your task needs to be able to stop reporting metrics at some point, use
// addCancelableTask() and destroy the returned token when the task isn't needed
// anymore.
//
// To actually start running the tasks, call start() method passing the metrics
// reporter instance and a looper to use for the recurring tasks. Before that,
// PeriodicReporter would be in a 'collect-only' mode where it collects all
// tasks added in addTask()/addCancelableTask(), but never runs those.
// stop() call stops and deletes all recrurring tasks and switches the
// PeriodicReporter back to 'collect-only' mode.
//

class PeriodicReporter final
        : public std::enable_shared_from_this<PeriodicReporter> {
    DISALLOW_COPY_ASSIGN_AND_MOVE(PeriodicReporter);

    using WeakPtr = std::weak_ptr<PeriodicReporter>;
    using InternalCallback = MetricsReporter::ConditionalCallback;
    // The container choice - list<> - here is to make sure the iterators are
    // preserved on all inserts or erases of other elements. We store the
    // iterator to the inserted element to be able to remove it later.
    using CallbackList = std::list<InternalCallback>;

    // A private guard class that removes the task when destroyed.
    class TaskGuard final {
        DISALLOW_COPY_ASSIGN_AND_MOVE(TaskGuard);
    public:
        using RemoverFunc = std::function<void()>;

        TaskGuard(RemoverFunc func) : taskRemover(std::move(func)) {}
        ~TaskGuard() { taskRemover(); }

    private:
        RemoverFunc taskRemover;
    };

public:
    // Forward-declare the singleton instance holder: this allows it to call
    // the private constructor.
    struct InstanceHolder;

    using TaskToken = std::shared_ptr<TaskGuard>;
    using Callback = InternalCallback;
    using Ptr = std::shared_ptr<PeriodicReporter>;

    // Start PeriodicReporter using the supplied |metricsReporter| for reporting
    // and |looper| to run periodic tasks.
    // You can add tasks even before this call; they just won't run until the
    // reporter is started.
    static void start(MetricsReporter* metricsReporter, base::Looper* looper);
    // Stop the currently running PeriodicReporter instance. This finalizes and
    // deletes all currently running tasks and reverts back to a mode where the
    // reporter collects all tasks but never runs them.
    static void stop();
    // Return the currently active instance.
    static PeriodicReporter& get();

    ~PeriodicReporter();

    // Add a new periodic reporting task that runs every |periodMs| milliseconds
    void addTask(base::System::Duration periodMs, Callback callback);
    // Same as addTask(), but the task can be canceled by destroying the
    // returned TaskToken.
    TaskToken addCancelableTask(base::System::Duration periodMs,
                                Callback callback);

private:
    // A set of callbacks added with the same period, and a recurrent task that
    // invokes them.
    struct PerPeriodData {
        base::Optional<base::RecurrentTask> task;
        CallbackList callbacks;
    };

    // Private constructor to make sure we only create a shared_ptr<>-owneed
    // instances (needed for the enable_shared_from_this<> base class).
    PeriodicReporter();

    // Update the internal MetricsReporter and Looper and start processing.
    void startImpl(MetricsReporter* metricsReporter, base::Looper* looper);

    void createPerPeriodTimerNoLock(PerPeriodData* dataPtr,
                                    base::System::Duration periodMs);

    void reportForPerPeriodData(PerPeriodData* data);

    // The implementation of task adding.
    // |mLock| has to be help during the call.
    CallbackList::iterator addTaskInternalNoLock(
            base::System::Duration periodMs,
            Callback callback);
    void removeTask(base::System::Duration periodMs,
                    CallbackList::iterator iter);

    MetricsReporter* mMetricsReporter = nullptr;
    base::Looper* mLooper = nullptr;

    base::Lock mLock;

    // A collection of tasks grouped by their run period.
    // Note: std::map<> is required here as it doesn't invalidate its iterators
    //  on all operations other than erasing the specific element for that
    //  iterator. We store the iterators (and, actually, addresses of stored
    //  PerPeriodData objects) and later use them, so they *must* be stable.
    std::map<base::System::Duration, PerPeriodData> mPeriodDataByPeriod;
};

}  // namespace metrics
}  // namespace android
