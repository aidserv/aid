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

#include "android/emulation/SetupParameters.h"

namespace android {

// static
void setupVirtualSerialPorts(ParameterList* kernelParams,
                             ParameterList* qemuParams,
                             int apiLevel,
                             const char* targetArch,
                             const char* kernelSerialPrefix,
                             bool isQemu2,
                             bool optionShowKernel,
                             bool hasShellConsole,
                             const char* shellSerial) {
    bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");

    bool isARMish = !strcmp(targetArch, "arm") || !strcmp(targetArch, "arm64");

    // Each virtual serial port device is connected to a QEMU 'chardev'
    // backend which can be 'null', 'stdio', 'con:', 'android-kmsg' or
    // 'android-qemud' in case of QEMU1. See docs/CHAR-DEVICES.TXT.
    //
    // CRITICAL: To properly support snapshotting, we need to ensure that the
    // set of virtual devices created for an AVD is consistent with its
    // hardware settings but does _not_ depend on command-line options. The
    // associated chardev can be different from one run to the other though.
    if (!isQemu2) {
        // Technical note: There are several important constraints when
        // setting up QEMU1 virtual ttys:
        //
        // - The first one if that for API level < 14, the system requires
        //   that the *second* virtual serial port (i.e. ttyS1) be associated
        //   with the android-qemud chardev, used to implement the legacy
        //   QEMUD protocol. Newer API levels use a pipe for this instead.
        //
        // - The second one is that the x86 and x86_64 virtual boards have
        //   a limited number of IRQs which makes it unable to setup more
        //   than two ttys at the same time.
        //
        // - Third, specifying -logcat implies -shell due to limitations in
        //   the guest system. I.e. the shell console will always receive
        //   logcat output, even if one uses -shell-serial to redirect its
        //   output to a pty or something similar.
        //
        // We thus consider the following cases:
        //
        // * For apiLevel >= 14:
        //      ttyS0 = android-kmsg (kernel messages)
        //      ttyS1 = <shell-serial> (logcat/shell).
        //
        // * For apiLevel < 14 && !x86ish:
        //      ttyS0 = android-kmsg (kernel messages)
        //      ttyS1 = android-qemud
        //      ttyS2 = <shell-serial> (logcat/shell)
        //
        // * For apiLevel < 14 && x86ish:
        //      ttyS0 = <shell-serial> (kernel messages/logcat/shell).
        //      ttyS1 = android-qemud
        //
        // Where <shell-serial> is the value of opts->shell_serial, which
        // by default will be the 'stdio' or 'con:' chardev (for Posix and
        // Windows, respectively).
        if (kernelParams) {
            if (optionShowKernel) {
                kernelParams->addFormat("console=%s0", kernelSerialPrefix);
            }
            int logcatSerial = 1;
            if (apiLevel < 14) {
                kernelParams->addFormat("android.qemud=%s1",
                                        kernelSerialPrefix);
                if (isX86ish) {
                    logcatSerial = 0;
                } else {
                    logcatSerial = 2;
                }
            } else {
                // The rild daemon, used for GSM emulation, checks for qemud,
                // just set it to a dummy value instead of a serial port.
                kernelParams->add("android.qemud=1");
            }

            if (hasShellConsole) {
                kernelParams->addFormat("androidboot.console=%s%d",
                                        kernelSerialPrefix, logcatSerial);
            }
        }

        if (qemuParams) {
            // First tty is used for kernel messages in all configurations, and
            // also for logcat/shell for (apiLevel < 14 && isX86ish).
            const char* kernelChardev;
            if (apiLevel < 14 && isX86ish) {
                kernelChardev = (hasShellConsole || optionShowKernel)
                                        ? shellSerial
                                        : "null";
            } else {
                kernelChardev = optionShowKernel ? "android-kmsg" : "null";
            }
            qemuParams->add2("-serial", kernelChardev);

            // For API level < 14, second tty is used for QEMUD.
            if (apiLevel < 14) {
                qemuParams->add2("-serial", "android-qemud");
            }

            // A third tty is required for apiLevel < 4 && !isX86ish.
            if (apiLevel >= 14 || !isX86ish) {
                qemuParams->add2("-serial",
                                 hasShellConsole ? shellSerial : "null");
            }
        }

    } else {  // isQemu2
        // A single virtual tty is used for -show-kernel, -shell and -logcat
        // and the -shell-serial option will affect all three of them.
        // TODO(digit): Ensure -shell-serial only works on -shell output.
        if (kernelParams) {
            if (optionShowKernel) {
                kernelParams->addFormat("console=%s0,38400",
                                        kernelSerialPrefix);
            }

            if (isARMish) {
                kernelParams->add("keep_bootcon");
                kernelParams->addFormat("earlyprintk=%s0", kernelSerialPrefix);
            }

            if (hasShellConsole) {
                kernelParams->addFormat("androidboot.console=%s0",
                                        kernelSerialPrefix);
            }

            // The rild daemon, used for GSM emulation, checks for qemud, just
            // set it to a dummy value instead of a serial port.
            kernelParams->add("android.qemud=1");
        }

        if (qemuParams) {
            bool useFirstSerial = optionShowKernel || hasShellConsole;
            qemuParams->add2("-serial", useFirstSerial ? shellSerial : "null");
        }
    }

    if (kernelParams && !optionShowKernel) {
        // Required to prevent kernel messages to be sent to framebuffer
        // (through 'vc0', i.e. virtual console 0).
        kernelParams->add("console=0");
    }
}

}  // namespace
