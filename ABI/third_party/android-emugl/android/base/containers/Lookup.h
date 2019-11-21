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

#include "android/base/TypeTraits.h"

#include <initializer_list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// A set of convenience functions for map and set lookups. They allow a simpler
// syntax, e.g.
//   if (auto val = find(map, "key")) {
//       <process the value>
//   }
// ... or
//   auto value = find(funcThatReturnsMap(), "other_key");
//   if (!value) ...
//
// Note: these don't work for multimaps, as there's no single value
//  to return (and, more importantly, as those are completely useless).

namespace android {
namespace base {

// Helper predicates that check if the template argument is a map / set /
// a mutlikey collection of any kind.
// These are used as a constraints for the lookup functions to get better error
// messages if the arguments don't support the map interface.
template <class T>
using is_any_map = std::integral_constant<
        bool,
        is_template_instantiation_of<T, std::map>::value ||
                is_template_instantiation_of<T, std::unordered_map>::value>;

template <class T>
using is_any_set = std::integral_constant<
        bool,
        is_template_instantiation_of<T, std::set>::value ||
                is_template_instantiation_of<T, std::unordered_set>::value>;

template <class T>
using is_any_multikey = std::integral_constant<
        bool,
        is_template_instantiation_of<T, std::multimap>::value ||
                is_template_instantiation_of<T, std::unordered_multimap>::value ||
                is_template_instantiation_of<T, std::multiset>::value ||
                is_template_instantiation_of<T, std::unordered_multiset>::value>;

template <class T, class = enable_if<is_any_map<T>>>
const typename T::mapped_type* find(const T& map,
                                    const typename T::key_type& key) {
    const auto it = map.find(key);
    if (it == map.end()) {
        return nullptr;
    }

    return &it->second;
}

// Version that returns a modifiable value.
template <class T, class = enable_if<is_any_map<T>>>
typename T::mapped_type* find(T& map, const typename T::key_type& key) {
    auto it = map.find(key);
    if (it == map.end()) {
        return nullptr;
    }

    return &it->second;
}

// Version with a default, returns a _copy_ because of the possible fallback
// to a default - it might be destroyed after the call.
template <class T,
          class U = typename T::mapped_type,
          class = enable_if_c<
                  is_any_map<T>::value &&
                  std::is_convertible<U, typename T::mapped_type>::value>>
typename T::mapped_type findOrDefault(const T& map,
                                      const typename T::key_type& key,
                                      U&& defaultVal = {}) {
    if (auto valPtr = find(map, key)) {
        return *valPtr;
    }
    return std::forward<U>(defaultVal);
}

// Version that finds the first of the values passed in |keys| in the order they
// are passed. E.g., the following code finds '2' as the first value in |keys|:
//   set<int> s = {1, 2, 3};
//   auto val = findFirstOf(s, {2, 1});
//   EXPECT_EQ(2, *val);
template <class T, class = enable_if<is_any_map<T>>>
const typename T::mapped_type* findFirstOf(
        const T& map,
        std::initializer_list<typename T::key_type> keys) {
    for (const auto& key : keys) {
        if (const auto valPtr = find(map, key)) {
            return valPtr;
        }
    }
    return nullptr;
}

template <class T, class = enable_if<is_any_map<T>>>
typename T::mapped_type* findFirstOf(
        T& map,
        std::initializer_list<typename T::key_type> keys) {
    for (const auto& key : keys) {
        if (const auto valPtr = find(map, key)) {
            return valPtr;
        }
    }
    return nullptr;
}

// Version that finds first of the passed |key| values or returns the
// |defaultVal| if none were found.
template <class T,
          class U,
          class = enable_if_c<
                  is_any_map<T>::value &&
                  std::is_convertible<U, typename T::mapped_type>::value>>
typename T::mapped_type findFirstOfOrDefault(
        const T& map,
        std::initializer_list<typename T::key_type> keys,
        U&& defaultVal) {
    if (const auto valPtr = findFirstOf(map, keys)) {
        return *valPtr;
    }
    return std::forward<U>(defaultVal);
}

template <class T,
          class = enable_if_c<is_any_map<T>::value || is_any_set<T>::value ||
                              is_any_multikey<T>::value>>
bool contains(const T& c, const typename T::key_type& key) {
    const auto it = c.find(key);
    return it != c.end();
}

template <class T,
          class = enable_if_c<is_any_map<T>::value || is_any_set<T>::value ||
                              is_any_multikey<T>::value>>
bool containsAnyOf(const T& c,
                   std::initializer_list<typename T::key_type> keys) {
    for (const auto& key : keys) {
        if (contains(c, key)) {
            return true;
        }
    }
    return false;
}

}  // namespace base
}  // namespace android
