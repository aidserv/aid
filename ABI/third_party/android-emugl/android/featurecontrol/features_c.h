/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef enum {
#define FEATURE_CONTROL_ITEM(item) kFeature_##item,
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
    kFeature_n_items
} Feature;

ANDROID_END_HEADER
