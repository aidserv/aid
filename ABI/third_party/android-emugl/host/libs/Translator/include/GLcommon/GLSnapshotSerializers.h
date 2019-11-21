/*
* Copyright 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

template <class Container>
void loadContainer(android::base::Stream* stream, Container& c) {
    size_t size = (size_t)stream->getBe32();
    c.resize(size);
    for (auto& ite : c) {
        ite.onLoad(stream);
    }
}

template <class NameMap>
void loadNameMap(android::base::Stream* stream, NameMap& namemap) {
    assert(namemap.size() == 0);
    size_t size = static_cast<size_t>(stream->getBe32());
    for (size_t i = 0; i < size; i++) {
        typename NameMap::key_type name = stream->getBe32();
        namemap.emplace(name, stream);
    }
}

template <class Container>
void saveContainer(android::base::Stream* stream, const Container& c) {
    stream->putBe32(c.size());
    for (const auto& ite : c) {
        ite.onSave(stream);
    }
}

template <class NameMap>
void saveNameMap(android::base::Stream* stream, const NameMap& nameMap) {
    stream->putBe32(nameMap.size());
    for (const auto& ite : nameMap) {
        stream->putBe32(ite.first);
        ite.second.onSave(stream);
    }
}
