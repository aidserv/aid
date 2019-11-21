// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/main-help.h"

// TODO: Re-implement android/help.c in C++ to get rid of stralloc.h
#include "android/help.h"
#include "android/utils/stralloc.h"

#include <stdio.h>
#include <string.h>

int emulator_parseHelpOption(const char* opt) {
    if (!strcmp(opt, "-help")) {
        STRALLOC_DEFINE(out);
        android_help_main(out);
        printf("%.*s", out->n, out->s);
        stralloc_reset(out);
        return 0;
    }

    if (!strncmp(opt, "-help-",6)) {
        STRALLOC_DEFINE(out);
        opt += 6;

        if (!strcmp(opt, "all")) {
            android_help_all(out);
        }
        else if (android_help_for_option(opt, out) == 0) {
            /* ok */
        }
        else if (android_help_for_topic(opt, out) == 0) {
            /* ok */
        }
        if (out->n > 0) {
            printf("\n%.*s", out->n, out->s);
            return 0;
        }

        fprintf(stderr, "unknown option: -help-%s\n", opt);
        fprintf(stderr, "please use -help for a list of valid topics\n");
        return 1;
    }
    return -1;
}
