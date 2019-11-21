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

#include "android/base/files/Stream.h"

/* Snapshot opengl renderer to |stream| */
void android_saveOpenglRenderer(android::base::Stream* stream);

/* Restore opengl renderer from |stream| */
int android_loadOpenglRenderer(android::base::Stream* stream, int version);
