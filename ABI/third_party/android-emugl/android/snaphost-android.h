/* Copyright (C) 2012 The Android Open Source Project
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

#include "android/utils/ini.h"

/* Matches HW config saved for a VM snapshot against the current HW config.
 * Param:
 *  hw_ini - CIniFile instance containing the current HW config settings.
 *  name - Name of the snapshot for which the VM is loading.
 * Return:
 *  Boolean: 1 if HW configurations match, or 0 if they don't match.
 */
extern int snaphost_match_configs(CIniFile* hw_ini, const char* name);

/* Saves HW config settings for the current VM.
 * Param:
 *  name - Name of the snapshot for the current VM.
 */
extern void snaphost_save_config(const char* name);
