// Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/threads/internal/ParallelTaskBase.h"
#include "android/base/threads/Types.h"

#include <functional>
#include <memory>
#include <utility>

namespace android {
namespace base {

// A ParallelTask<Result> is an object that allows you to run a task in a
// separate thread, and take follow up action back on the current thread's event
// loop. Additionally, the |taskDoneFunction| function is called with a result
// of type |Result| returned from the launched thread.
//
// An example of a thread returning the typical int exitStatus:
//
//   class LifeUniverseAndEverythingInParallel {
//   public:
//       LifeUniverseAndEverythingInParallel(Looper* looper) :
//               mEarth(looper,
//                      std::bind(&LifeUniverseAndEverythingInParallel::compute,
//                                this, std::placeholders::_1),
//                      std::bind(
//                          &LifeUniverseAndEverythingInParallel::printAnswer,
//                          this, std::placeholders::_1) {}
//
//       // This is the entry point of computation.
//       bool startOfTime() {
//           return mEarth.start();
//       }
//       // Called by the parallel task.
//       void compute(int* outResult) {
//           *outResult = 42;
//       }
//
//       void printAnswer(const int& outResult) {
//           std::cout << "The mice say it's " << outResult << std::endl;
//       }
//
//   private:
//       ParallelTask<int> mEarth;
//   };
//
//   int main() {
//     LifeUniverseAndEverythingInParallel whatsTheAnswer;
//     whatsTheAnswer.startOfTime();
//     android::base::ThreadLooper::get()->runWithDeadline(
//             // 10 million years);
//     // ... Do other stuff while the mice run inside contraptions ...
//   }
//
//   If you just want to run a free function to compute something in parallel
//   and then take follow up action using another free function, a templatized
//   helper function is provided as well:
//
//   void computeStandalone(int* outResult) {
//       *outResult = 42;
//   }
//
//   void printAnswer(const int& outResult) {
//       std::cout << "The mice say it's " << outResult << std::endl;
//   }
//
//   runParallelTask<int>(
//           android::base::ThreadLooper::get(),
//           &computeStandalone,
//           &printAnswer);
//
template <class ResultType>
class ParallelTask final : public internal::ParallelTaskBase {
public:
    using TaskFunction = std::function<void(ResultType*)>;
    using TaskDoneFunction = std::function<void(const ResultType&)>;

    // Args:
    //     looper: A running looper for the current thread.
    //     taskFunction: The function you want to be called on a separate thread
    //             to perform the task.
    //     taskDoneFunction: The function you want to be called on the event
    //             loop in the current thread once the task is completed.
    //     checkTimeoutMs: The time in milliseconds between consecutive checks
    //             for thread termination. A bigger value possibly delayes the
    //             call to |taskDoneFunction|, but leads to fewer checks.
    //             Default: 1 second.
    //     flags: See android::base::Thread.
    ParallelTask(android::base::Looper* looper,
                 TaskFunction taskFunction,
                 TaskDoneFunction taskDoneFunction,
                 android::base::Looper::Duration checkTimeoutMs = 1 * 1000,
                 ThreadFlags flags = ThreadFlags::MaskSignals)
        : ParallelTaskBase(looper, checkTimeoutMs, flags),
        mTaskFunction(taskFunction), mTaskDoneFunction(taskDoneFunction) {}

    // Start the thread instance. Returns true on success, false otherwise.
    // (e.g. if the thread was already started or terminated).
    bool start() { return ParallelTaskBase::start(); }

    // Returns true if the task has been |start|'ed, but the call to
    // |taskDone| hasn't finished yet.
    // This function should be called from the same thread that called |start|.
    bool inFlight() const { return ParallelTaskBase::inFlight(); }

protected:
    void taskImpl() override final { mTaskFunction(&mResultBuffer); }

    void taskDoneImpl() override final { mTaskDoneFunction(mResultBuffer); }

private:
    ResultType mResultBuffer;
    TaskFunction mTaskFunction;
    TaskDoneFunction mTaskDoneFunction;

    DISALLOW_COPY_AND_ASSIGN(ParallelTask);
};

namespace internal {

template <class ResultType>
class SelfDeletingParallelTask {
public:
    using TaskFunction = typename ParallelTask<ResultType>::TaskFunction;
    using TaskDoneFunction =
            typename ParallelTask<ResultType>::TaskDoneFunction;

    SelfDeletingParallelTask(android::base::Looper* looper,
                             TaskFunction taskFunction,
                             TaskDoneFunction taskDoneFunction,
                             android::base::Looper::Duration checkTimeoutMs)
        : mTaskDoneFunction(taskDoneFunction),
          mParallelTask(looper,
                        taskFunction,
                        std::bind(&SelfDeletingParallelTask::taskDoneFunction,
                                  this,
                                  std::placeholders::_1),
                        checkTimeoutMs) {}

    bool start() { return mParallelTask.start(); };

    void taskDoneFunction(const ResultType& result) {
        mTaskDoneFunction(result);
        delete this;
    }

private:
    TaskDoneFunction mTaskDoneFunction;
    ParallelTask<ResultType> mParallelTask;

    DISALLOW_COPY_AND_ASSIGN(SelfDeletingParallelTask);
};

}  // namespace internal

template<class ResultType>
bool runParallelTask(android::base::Looper* looper,
                     std::function<void(ResultType*)> taskFunction,
                     std::function<void(const ResultType&)> taskDoneFunction,
                     android::base::Looper::Duration checkTimeoutMs = 1 * 1000) {
    auto flyaway = new internal::SelfDeletingParallelTask<ResultType>(
            looper, taskFunction, taskDoneFunction, checkTimeoutMs);
    return flyaway->start();

}

}  // namespace base
}  // namespace android
