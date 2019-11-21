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

/* initialize */
extern void  android_hw_fingerprint_init( void );

/* touch fingerprint sensor with fingerid*/
void android_hw_fingerprint_touch(int fingerid);

/* take finger off the fingerprint sensor */
void android_hw_fingerprint_remove();
