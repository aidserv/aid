/* Copyright (C) 2015 The Android Open Source Project
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

#ifdef __cplusplus
extern "C"
#endif
void* getNSWindow(void* ns_view);
void nsWindowHideWindowButtons(void* ns_window);
int numHeldMouseButtons();
void nsWindowAdopt(void *parent, void *child);
