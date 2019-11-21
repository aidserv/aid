/* Copyright (C) 2007-2008 The Android Open Source Project
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

/** ANDROID CONFIGURATION FILE SUPPORT
 **
 ** A configuration file is loaded as a simplre tree of (key,value)
 ** pairs. keys and values are simple strings
 **/
typedef struct AConfig  AConfig;

struct AConfig
{
    AConfig*     next;
    AConfig*     first_child;
    AConfig*     last_child;
    const char*  name;
    const char*  value;
};

/* parse a text string into a config node tree */
extern void   aconfig_load(AConfig*  root, char*  data);

/* parse a file into a config node tree, return 0 in case of success, -1 otherwise */
extern int    aconfig_load_file(AConfig*  root, const char*  path);

/* save a config node tree into a file, return 0 in case of success, -1 otherwise */
extern int aconfig_save_file(const AConfig* root, const char* path);

/* create a single config node */
extern AConfig*  aconfig_node(const char *name, const char *value);

/* locate a named child of a config node */
extern const AConfig* aconfig_find_const(const AConfig* root, const char* name);

/* locate a named child of a config node. NULL if it doesn't exist. */
extern AConfig* aconfig_find(AConfig* root, const char* name);

/* add a named child to a config node (or modify it if it already exists) */
extern void aconfig_set(AConfig* root, const char* name, const char* value);

/* look up a child by name and return its value, eventually converted
 * into a boolean or integer */
extern int aconfig_bool(const AConfig* root, const char* name, int _default);
extern unsigned aconfig_unsigned(const AConfig* root,
                                 const char* name,
                                 unsigned _default);
extern int aconfig_int(const AConfig* root, const char* name, int _default);
extern const char* aconfig_str(const AConfig* root,
                               const char* name,
                               const char* _default);

/* Recusively destroy a node and its children. */
extern void aconfig_node_free(AConfig *root);

ANDROID_END_HEADER
