// Copyright (C) 2014 The Android Open Source Project
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

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#else
#  include <pthread.h>
#endif

namespace android {
namespace base {

// A class to model storage of thread-specific values, that can be
// destroyed on thread exit.
//
// Note that on Windows, a thread must call OnThreadExit() explicitly
// here to ensure that the values are probably discarded. This is an
// unfortunate requirement of the Win32 API, which doesn't support
// destructors at all.
//
// There are various hacks on the web to try to achieve this automatically
// (e.g. [1]) but they rely on using the Microsoft build tools,
// which doesn't work for us.
//
// Note another important issue with ThreadStore instances: if you create
// one instance in a shared library, you need to make sure that it is
// always destroyed before the library is unloaded. Otherwise, future
// thread exit will likely crash, due to calling a destructor function
// that is no longer in the process' address space.
//
// Finally, destroying an instance does _not_ free the corresponding values,
// because doing so properly requires coordinating all participating threads,
// which is impossible to achieve in the most general case. Thus, consider
// that thread-local values are always leaked on library unload, or on
// program exit.
//
// [1] http://stackoverflow.com/questions/14538159/about-tls-callback-in-windows

// ThreadStoreBase is the base class used by all ThreadStore template
// instances, used to reduce bloat.
class ThreadStoreBase {
public:
    // Type of a function used to destroy a thread-specific value that
    // was previously assigned by calling set().
    typedef void (Destructor)(void* value);

    // Initialize instance so that is hold keys that must be destroyed
    // on thread exit by calling |destroy|.
    explicit ThreadStoreBase(Destructor* destroy);

    // NOTE: Destructor don't free the thread-local values, but are required
    // to avoid crashes (see note above).
    ~ThreadStoreBase();

    // Retrieve current thread-specific value from store.
#ifdef _WIN32
    void* get() const;
#else
    inline void* get() const {
        return pthread_getspecific(mKey);
    }
#endif

    // Set the new thread-specific value.
#ifdef _WIN32
    void set(void* value);
#else
    inline void set(void* value) {
        pthread_setspecific(mKey, value);
    }
#endif

    inline void* swap(void* value) {
        void* old = get();
        set(value);
        return old;
    }

#ifdef _WIN32
    // Each thread should call this function on exit to ensure that
    // all corresponding TLS values are properly freed.
    static void OnThreadExit();
#else
    // Nothing to do on Posix.
    static inline void OnThreadExit() {}
#endif

private:
    // Ensure you can't create an empty ThreadStore instance.
    ThreadStoreBase();

    DISALLOW_COPY_AND_ASSIGN(ThreadStoreBase);

#ifdef _WIN32
    int mKey;
#else
    pthread_key_t mKey;
#endif
};

// ThreadStore is a template class used to implement a thread-local store
// of objects of type |T|. Note that the store owns the objects, and these
// are destroyed when an android::base::Thread exits.
template <typename T>
class ThreadStore : public ThreadStoreBase {
public:
    // Create a new ThreadStore instance.
    ThreadStore() : ThreadStoreBase(myDestructor) {}

    // Retrieve the thread-specific object instance, or NULL if set()
    // was never called before in the current thread.
    T* get() {
        return static_cast<T*>(ThreadStoreBase::get());
    }

    // Set the current thread-specific objet instance for this thread.
    // |t| is the new object instance.
    // NOTE: Any previous object instance is deleted.
    void set(T* t) {
        T* old = static_cast<T*>(swap(t));
        delete old;
    }

    // Swap the current thread-specific object for this thread.
    // |t| is the new object instance.
    // Return the previous one. Transfers ownership to the caller.
    T* swap(T* t) {
        return static_cast<T*>(ThreadStoreBase::swap(t));
    }

private:
    static void myDestructor(void* opaque) {
        delete static_cast<T*>(opaque);
    }
};

}  // namespace base
}  // namespace android
