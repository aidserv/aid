// Copyright (C) 2014-2015 The Android Open Source Project
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

#include <atomic>
#include <new>
#include <type_traits>

namespace android {
namespace base {
namespace internal {

// Older GCC stdlib uses deprecated naming scheme has_xxx instead of is_xxx
#if 0 // 移除
//#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 4
template <class T>
using is_trivially_default_constructible =
        std::has_trivial_default_constructor<T>;
//#else
#endif
template <class T>
using is_trivially_default_constructible =
        std::is_trivially_default_constructible<T>;
//#endif

// A LazyInstance is a helper template that can be used to perform
// thread-safe lazy initialization of static C++ objects without forcing
// the generation of C++ static constructors in the final executable.
//
// In a nutshell, you can replace a statement like:
//
//    static Foo gFoo;
//
// With:
//
//    static LazyInstance<Foo> gFoo = LAZY_INSTANCE_INIT;
//
// In the first case, a hidden static C++ constructor is embedded in the
// final executable, and executed at *load* *time* to call the Foo::Foo
// constructor on the gFoo object.
//
// On the second case, gFoo will only be initialized lazily, i.e. the first
// time any code actually tries to access the variable.
//
// Note that access is slightly different, i.e.:
//
//    gFoo.get() returns a reference to the lazy-initialized object.
//    gFoo.ptr() returns a pointer to it.
//    gFoo->Something() is equivalent to doing gFoo.ptr()->Something().
//
// 'gFoo' is stored in the .bss section and this doesn't use heap allocation.
// This class can only be used to perform lazy initialization through the
// class' default constructor. For more specialized cases, you will have
// to create a derived class, e.g.:
//
//    class FoorWithDefaultParams : public Foo {
//    public:
//       FooWithDefaultParams() : Foo(<default-parameters>) {}
//    };
//
//    LazyInstance<FooWithDefaultParams> gFoo = LAZY_INSTANCE_INIT;
//
// The implementation of LazyInstance relies on atomic operations and
// POD-struct class definitions, i.e. one that doesn't have any constructor,
// destructor, virtual members, or private ones, and that can be
// zero-initialized at link time.
//
// You can also use LazyInstance<> instances as static local variables,
// e.g.:
//
//     Foo*  getFooSingleton() {
//        static LazyInstance<Foo> sFoo = LAZY_INSTANCE_INIT;
//        return sFoo.ptr();
//     }
//
// This is useful on Windows which doesn't support thread-safe lazy
// initialization of static C++ local variables, or on other platforms
// when the code is compiled with -fno-threadsafe-statics.
//
// This class is heavily inspired by Chromium's implementation of the
// same-named class (see $CHROMIUM/src/base/lazy_instance.h).

// Atomic state variable type. Used to ensure to synchronize concurrent
// initialization and access without incurring the full cost of a mutex
// lock/unlock.
struct LazyInstanceState {
    enum class State : char {
        Init = 0,
        Constructing = 1,
        Done = 2,
    };

    bool inInitState() const;
    bool needConstruction();
    void doneConstructing();

    std::atomic<State> mState;
};

//static_assert(std::is_standard_layout<LazyInstanceState>::value,
//              "LazyInstanceState is not a standard layout type");
//static_assert(is_trivially_default_constructible<LazyInstanceState>::value,
//              "LazyInstanceState can't be trivially default constructed");

}  // namespace internal

// LazyInstance template definition, see comment above for usage
// instructions. It is crucial to make this a POD-struct compatible
// type [1].
//
// [1] http://en.wikipedia.org/wiki/Plain_Old_Data_Structures
//
template <class T>
struct LazyInstance {
    bool hasInstance() const { return !mState.inInitState(); }

    const T& get() const { return *ptr(); }
    T& get() { return *ptr(); }

    const T* operator->() const { return ptr(); }
    T* operator->() { return ptr(); }

    const T& operator*() const { return get(); }
    T& operator*() { return get(); }

    T* ptr() { return ptrInternal(); }
    const T* ptr() const { return ptrInternal(); }

private:
    T* ptrInternal() const;

    using StorageT =
            typename std::aligned_storage<sizeof(T),
                                          std::alignment_of<T>::value>::type;

    alignas(double) mutable internal::LazyInstanceState mState;
    mutable StorageT mStorage;
};

// Initialization value, must resolve to all-0 to ensure the object
// instance is actually placed in the .bss
#define LAZY_INSTANCE_INIT \
    {}

template <class T>
T* LazyInstance<T>::ptrInternal() const {
    // Make sure that LazyInstance<> instantiation remains static.
    // NB: this can't go in a class scope as class is still incomplete there.
    //static_assert(std::is_standard_layout<LazyInstance>::value,
    //              "LazyInstance<T> is not a standard layout type");
    //static_assert(
    //        internal::is_trivially_default_constructible<LazyInstance>::value,
    //        "LazyInstance<T> can't be trivially default constructed");

    if (mState.needConstruction()) {
        new (&mStorage) T();
        mState.doneConstructing();
    }
    return reinterpret_cast<T*>(&mStorage);
}

}  // namespace base
}  // namespace android
