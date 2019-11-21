/*
 * Copyright (C) 2014 The Android Open Source Project
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
#ifndef GRAPHICS_TRANSLATION_GLES_MRU_CACHE_H_
#define GRAPHICS_TRANSLATION_GLES_MRU_CACHE_H_

#include <list>
#include <utility>

// A container of a limited number of objects that are managed in a most
// recently used manner and can be looked up by a Key.
template <typename Key, typename Value>
class MruCache {
 public:
  // Typedef for the function that can be used to clean up objects if/when
  // they get removed from the cache.
  typedef void (*DestroyFn)(Value* v);

  typedef std::list<std::pair<Key, Value> > Cache;

  explicit MruCache(size_t limit) : cache_(), destroy_(NULL), limit_(limit) {}

  MruCache(size_t limit, DestroyFn destroy)
      : cache_(), destroy_(destroy), limit_(limit) {}

  ~MruCache() {
    if (destroy_) {
      while (cache_.size() != 0) {
        destroy_(&cache_.back().second);
        cache_.pop_back();
      }
    }
  }

  Value* GetMostRecentlyUsed() {
    if (cache_.size() > 0) {
      return &cache_.front().second;
    }
    return NULL;
  }

  // Gets a pointer to the object referenced by the Key (or NULL if no such
  // object).  Will internally move the object to the "front" of the cache.
  Value* Get(const Key& key) {
    for (typename Cache::iterator i = cache_.begin(); i != cache_.end(); ++i) {
      if (i->first == key) {
        cache_.splice(cache_.begin(), cache_, i);
        return &i->second;
      }
    }
    return NULL;
  }

  // Push an object into the cache referenced by the Key.
  Value* Push(const Key& key, const Value& value) {
    if (cache_.size() == limit_) {
      if (destroy_) {
        destroy_(&cache_.back().second);
      }
      cache_.pop_back();
    }
    cache_.push_front(std::make_pair(key, value));
    return &cache_.front().second;
  }

 private:
  Cache cache_;
  DestroyFn destroy_;
  size_t limit_;

  MruCache(const MruCache&);
  MruCache& operator=(const MruCache&);
};

#endif  // GRAPHICS_TRANSLATION_GLES_MRU_CACHE_H_
