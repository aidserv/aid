// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QEvent>

#include <functional>

// Specialize std::hash for some Qt types.
namespace std {
template<> struct hash<QEvent::Type> {
    size_t operator()(QEvent::Type type) const {
        return hash<int>()(static_cast<int>(type));
    }
};

}

