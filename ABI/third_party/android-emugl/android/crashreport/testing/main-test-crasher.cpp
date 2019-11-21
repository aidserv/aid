/* Copyright (C) 2011 The Android Open Source Project
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

/*
 * This is the source for a dummy crash program that starts the crash reporter
 * and then crashes due to an undefined symbol
 */

#include "android/crashreport/CrashReporter.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <cstdlib>

void crashme(int arg, bool nocrash, int delay_ms) {
    if (delay_ms) {
        printf("Delaying crash by %d ms\n", delay_ms);
        sleep_ms(delay_ms);
    }
    for (int i = 0; i < arg; i++) {
        if (!nocrash) {
            printf("Crash Me Now\n");
            asm(".byte 0");
        }
    }
}

/* Main routine */
int main(int argc, char** argv) {
    const char* pipe = nullptr;
    bool nocrash = false;
    bool noattach = false;
    int delay_ms = 0;
    int nn;
    for (nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];
        if (!strcmp(opt, "-pipe")) {
            if (nn + 1 < argc) {
                nn++;
                pipe = argv[nn];
            }
        } else if (!strcmp(opt, "-nocrash")) {
            nocrash = true;
        } else if (!strcmp(opt, "-noattach")) {
            noattach = true;
        } else if (!strcmp(opt, "-delay_ms")) {
            if (nn + 1 < argc) {
                nn++;
                delay_ms = atoi(argv[nn]);
            }
        }
    }

    if (!pipe) {
        derror("Usage: -pipe <pipe> <-nocrash> <-noattach> <-delay_ms xx>\n");
        return 1;
    }

    ::android::crashreport::CrashSystem::CrashPipe crashpipe(pipe);

    if (!noattach) {
        if (!::android::crashreport::CrashReporter::get()->attachCrashHandler(
                    crashpipe)) {
            derror("Couldn't attach to crash handler\n");
            return 1;
        }
    } else {
        printf("Not attaching to crash pipe\n");
    }

    crashme(argc, nocrash, delay_ms);
    printf("test crasher done\n");
    return 0;
}
