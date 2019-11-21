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

#include "android/main-common.h"
#include "android/main-qemu-parameters.h"

#include "android/base/StringFormat.h"
#include "android/emulation/ParameterList.h"
#include "android/emulation/SetupParameters.h"
#include "android/utils/debug.h"
#include "android/utils/file_data.h"
#include "android/utils/property_file.h"
#include "android/utils/system.h"

#include <memory>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct QemuParameters {
    android::ParameterList params;
};

size_t qemu_parameters_size(const QemuParameters* params) {
    return params->params.size();
}

char** qemu_parameters_array(const QemuParameters* params) {
    return params->params.array();
}

void qemu_parameters_free(QemuParameters* params) {
    delete params;
}

QemuParameters* qemu_parameters_create(const char* argv0,
                                       int qemuArgc,
                                       const char* const* qemuArgv,
                                       AndroidOptions* opts,
                                       const AvdInfo* avd,
                                       const char* initialKernelParameters,
                                       const char* androidHwIniPath,
                                       bool isQemu2,
                                       const char* targetArch) {
    std::unique_ptr<QemuParameters> result(new QemuParameters);
    android::ParameterList& params = result->params;

    params.add(argv0);

    // net.shared_net_ip boot property value.
    char boot_prop_ip[64] = {};
    if (opts->shared_net_id) {
        char* end;
        long shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            derror("option -shared-net-id must be an integer between 1 and 255\n");
            return NULL;
        }
        snprintf(boot_prop_ip, sizeof(boot_prop_ip),
                 "net.shared_net_ip=10.1.2.%ld", shared_net_id);

        params.add2If("-boot-property", boot_prop_ip);
    }

    params.add2If("-tcpdump", opts->tcpdump);

#ifdef CONFIG_NAND_LIMITS
    params.add2If("-nand-limits", opts->nand_limits);
#endif

    params.add2If("-timezone", opts->timezone);
    params.add2If( "-audio", opts->audio);
    params.add2If("-cpu-delay", opts->cpu_delay);
    params.add2If("-dns-server", opts->dns_server);
    params.addIf("-skip-adb-auth", opts->skip_adb_auth);

    if (opts->snapstorage) {
        // We still use QEMU command-line options for the following since
        // they can change from one invokation to the next and don't really
        // correspond to the hardware configuration itself.
        if (!opts->no_snapshot_load) {
            params.add2("-loadvm", opts->snapshot);
        }

        if (!opts->no_snapshot_save) {
            params.add2("-savevm-on-exit", opts->snapshot);
        }

        if (opts->no_snapshot_update_time) {
            params.add("-snapshot-no-time-update");
        }
    }

    // Virtual tty setup. Note: this must be synced with the code in
    // android/main-kernel-parameters.cpp. See technical note in this file.
    int apiLevel = avdInfo_getApiLevel(avd);
    android::setupVirtualSerialPorts(nullptr, &params, apiLevel, targetArch,
                                     "",  // kernelSerialPrefix
                                     isQemu2, opts->show_kernel,
                                     opts->logcat || opts->shell,
                                     opts->shell_serial);

    params.add2If("-radio", opts->radio);
    params.add2If("-gps", opts->gps);
    params.add2If("-code-profile", opts->code_profile);

    /* Pass boot properties to the core. First, those from boot.prop,
     * then those from the command-line */
    const FileData* bootProperties = avdInfo_getBootProperties(avd);
    if (!fileData_isEmpty(bootProperties)) {
        PropertyFileIterator iter[1];
        propertyFileIterator_init(iter,
                                  bootProperties->data,
                                  bootProperties->size);
        while (propertyFileIterator_next(iter)) {
            char temp[MAX_PROPERTY_NAME_LEN + MAX_PROPERTY_VALUE_LEN + 2];
            snprintf(temp, sizeof temp, "%s=%s", iter->name, iter->value);
            params.add2("-boot-property", temp);
        }
    }

    if (opts->prop != NULL) {
        ParamList*  pl = opts->prop;
        for ( ; pl != NULL; pl = pl->next ) {
            params.add2("-boot-property", pl->param);
        }
    }

    // TODO(digit): Add kernel parameters here.

    params.add2If("-android-ports", opts->ports);
    params.add2If("-android-port", opts->port);
    params.add2If("-android-report-console", opts->report_console);
    params.add2If("-http-proxy", opts->http_proxy);

    /* Set up the interfaces for inter-emulator networking */
    if (opts->shared_net_id) {
        unsigned int shared_net_id = atoi(opts->shared_net_id);
        char nic[37];

        params.add2("-net", "nic,vlan=0");
        params.add2("-net", "user,vlan=0");

        snprintf(nic, sizeof nic, "nic,vlan=1,macaddr=52:54:00:12:34:%02x", shared_net_id);
        params.add2("-net", nic);

        params.add2("-net", "socket,vlan=1,mcast=230.0.0.10:1234");
    }

    if (targetArch &&
        (!strcmp(targetArch, "x86") ||
         !strcmp(targetArch, "x86_64"))) {
        char* accel_status = NULL;
        CpuAccelMode accel_mode = ACCEL_AUTO;
        bool accel_ok = handleCpuAcceleration(opts, avd, &accel_mode, &accel_status);

        VERBOSE_PRINT(init, "checking cpu acceleration from main qemu params");
        // CPU acceleration only works for x86 and x86_64 system images.
        if (accel_mode == ACCEL_OFF && accel_ok) {
            params.add(kDisableAccelerator);
        } else if (accel_mode == ACCEL_ON) {
            if (!accel_ok) {
                derror("CPU acceleration not supported on this machine!");
                derror("Reason: %s", accel_status);
                return NULL;
            }
            params.add(kEnableAccelerator);
        } else {
            params.add(accel_ok ? kEnableAccelerator : kDisableAccelerator);
        }

        AFREE(accel_status);
    } else {
        if (VERBOSE_CHECK(init)) {
            dwarning("CPU acceleration only works with x86/x86_64 "
                    "system images.");
        }
    }

    params.add2("-android-hw", androidHwIniPath);

    for (int n = 0; n < qemuArgc; ++n) {
        params.add(qemuArgv[n]);
    }

    // Handling kernel parameters. They are passed to QEMU through the
    // -append flag.
    {
        std::string kernelParams;
        if (initialKernelParameters) {
            kernelParams += initialKernelParameters;
        }
        for (int n = 0; n < qemuArgc; ++n) {
            if (!strcmp(qemuArgv[n], "-append") && n + 1 < qemuArgc) {
                android::base::StringAppendFormat(&kernelParams, " %s",
                                                  qemuArgv[n + 1]);
                n++;
            }
        }
        if (!kernelParams.empty()) {
            params.add("-append");
            params.add(std::move(kernelParams));
        }
    }

    if(VERBOSE_CHECK(init)) {
        printf("QEMU options list:\n");
        for(size_t i = 0; i < params.size(); i++) {
            printf("emulator: argv[%02d] = \"%s\"\n", static_cast<int>(i),
                   params[i].c_str());
        }
        /* Dump final command-line option to make debugging the core easier */
        printf("Concatenated QEMU options:\n");
        for (size_t i = 0; i < params.size(); i++) {
            /* To make it easier to copy-paste the output to a command-line,
             * quote anything that contains spaces.
             */
            if (::strchr(params[i].c_str(), ' ') != nullptr) {
                printf(" '%s'", params[i].c_str());
            } else {
                printf(" %s", params[i].c_str());
            }
        }
        printf("\n");
    }

    return result.release();
}
