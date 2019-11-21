/* Copyright (C) 2007-2017 The Android Open Source Project
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

ANDROID_BEGIN_HEADER

enum proxy_errno {
    PROXY_ERR_OK,
    PROXY_ERR_BAD_FORMAT,
    PROXY_ERR_UNREACHABLE,
    PROXY_ERR_INTERNAL
};

ANDROID_END_HEADER
