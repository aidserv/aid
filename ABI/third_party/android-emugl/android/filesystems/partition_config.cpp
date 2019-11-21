// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/filesystems/partition_config.h"

#include "android/base/system/System.h"
#include "android/filesystems/internal/PartitionConfigBackend.h"
#include "android/utils/debug.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using android::internal::PartitionConfigBackend;
using android::base::System;

// State structure shared by several functions.
typedef struct {
    char** error_message;
    AndroidPartitionSetupFunction setup_func;
    void* setup_opaque;
    PartitionConfigBackend* backend;
} PartitionConfigState;

// Helper function used to record an error message into the |state|,
// then return false. |format| is a typical printf()-like formatting
// string followed by optional arguments. Always return false to
// indicate an error.
static bool partition_config_error(PartitionConfigState* state,
                                   const char* format,
                                   ...) {
    va_list args;
    va_start(args, format);
    vasprintf(state->error_message, format, args);
    va_end(args);
    return false;
}

// Extract the partition type/format of a given partition image
// from the content of fstab.goldfish.
// |fstab| is the address of the fstab.goldfish data in memory.
// |fstabSize| is its size in bytes.
// |partitionName| is the name of the partition for debugging
// purposes (e.g. 'userdata').
// |partitionPath| is the partition path as it appears in the
// fstab file (e.g. '/data').
// On success, sets |*partitionType| to an appropriate value,
// on failure (i.e. |partitionPath| does not appear in the fstab
// file), leave the value untouched.
static void extractPartitionFormat(PartitionConfigState* state,
                                   const std::string& fstab,
                                   const char* partitionName,
                                   const char* partitionPath,
                                   AndroidPartitionType* partitionType) {
    std::string partFormat;
    if (!state->backend->parsePartitionFormat(fstab, partitionPath,
                                              &partFormat)) {
        VERBOSE_PRINT(init, "Could not extract format of %s partition!",
                      partitionName);
        return;
    }
    VERBOSE_PRINT(init, "Found format of %s partition: '%s'", partitionName,
                  partFormat.c_str());
    *partitionType = androidPartitionType_fromString(partFormat.c_str());
}

// List of value describing how to handle partition images in
// addNandImage() below, when no initial partition image
// file is provided.
//
// MUST_EXIST means that the partition image must exist, otherwise
// dump an error message and exit.
//
// CREATE_IF_NEEDED means that if the partition image doesn't exist, an
// empty partition file should be created on demand.
//
// MUST_WIPE means that the partition image should be wiped cleaned,
// even if it exists. This is useful to implement the -wipe-data option.
typedef enum {
    ANDROID_PARTITION_OPEN_MODE_MUST_EXIST,
    ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED,
    ANDROID_PARTITION_OPEN_MODE_MUST_WIPE,
} AndroidPartitionOpenMode;

// Add a NAND partition image to the hardware configuration.
//
// |part_name| is a string indicating the type of partition, i.e. "system",
// "userdata" or "cache".
// |part_type| is an enum describing the type of partition. If it is
// DISK_PARTITION_TYPE_PROBE, then try to auto-detect the type directly
// from the content of |part_file| or |part_init_file|.
// |part_mode| is an enum describing how to handle the partition image,
// see AndroidPartitionOpenMode for details.
// |part_size| is the partition size in bytes.
// |part_file| is the partition file path, can be NULL if |path_init_file|
// is not NULL.
// |part_init_file| is an optional path to the initialization partition file.
//
// The NAND partition will be backed by |part_file|, except in the following
// cases:
//    - |part_file| is NULL, or its value is "<temp>", indicating that a
//      new temporary image file must be used instead.
//
//    - |part_file| is not NULL, but the function fails to lock the file,
//      indicating it's already used by another instance. A warning should
//      be printed to warn the user, and a new temporary image should be
//      used.
//
// If |part_file| is not NULL and can be locked, if the partition image does
// not exit, then the file must be created as an empty partition.
//
// If |part_init_file| is not NULL, its content will be used to erase
// the content of the main partition image. This is automatically handled
// by the NAND code though.
//
// If |readonly| is true, then either the |part_file| or |part_init_file| will
// be mounted as read-only devices. This also prevents locking the partition
// image and creation of temporary copies.
//
static bool addNandImage(PartitionConfigState* state,
                         const char* part_name,
                         AndroidPartitionType part_type,
                         AndroidPartitionOpenMode part_mode,
                         uint64_t part_size,
                         const char* part_file,
                         const char* part_init_file,
                         bool readonly) {
    // Sanitize parameters, an empty string must be the same as NULL.
    if (part_file && !*part_file) {
        part_file = NULL;
    }
    if (part_init_file && !*part_init_file) {
        part_init_file = NULL;
    }

    // Sanity checks.
    if (part_size == 0) {
        return partition_config_error(
                state, "Invalid %s partition size 0x%" PRIx64,
                part_name, part_size);
    }

    if (part_init_file && !state->backend->pathExists(part_init_file)) {
        return partition_config_error(state, "Missing initial %s image: %s",
                                      part_name, part_init_file);
    }

    // In read-only mode, the initial partition image can be treated as
    // the main one.
    if (readonly && !part_file && part_init_file) {
        part_file = part_init_file;
        part_init_file = nullptr;
    }

    // As a special case, a |part_file| of '<temp>' means a temporary
    // partition is needed.
    if (part_file && !strcmp(part_file, "<temp>")) {
        part_file = NULL;
    }

    // Verify partition type, or probe it if needed.
    {
        // First determine which image file to probe.
        const char* image_file = NULL;
        if (part_file && state->backend->pathExists(part_file)) {
            image_file = part_file;
        } else if (part_init_file) {
            image_file = part_init_file;
        } else if (part_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            return partition_config_error(
                    state,
                    "Cannot determine type of %s partition: no image files!",
                    part_name);
        }

        if (part_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            VERBOSE_PRINT(init, "Probing %s image file for partition type: %s",
                          part_name, image_file);

            part_type = state->backend->probePartitionFileType(image_file);
        } else if (image_file) {
            // Probe the current image file to check that it is of the
            // right partition format.
            AndroidPartitionType image_type =
                    state->backend->probePartitionFileType(image_file);
            if (image_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
                return partition_config_error(
                        state, "Cannot determine %s partition type of: %s",
                        part_name, image_file);
            }

            if (image_type != part_type) {
                // The image file exists, but is not in the proper format!
                // This can happen in certain cases, e.g. a KitKat/x86 AVD
                // created with SDK 23.0.2 and started with the
                // corresponding emulator will create a cache.img in 'yaffs2'
                // format, while the system really expect it to be 'ext4',
                // as listed in the ramdisk.img.
                //
                // To work-around the problem, simply re-create the file
                // by wiping it when allowed.

                if (part_mode == ANDROID_PARTITION_OPEN_MODE_MUST_EXIST) {
                    return partition_config_error(
                            state,
                            "Invalid %s partition image type: %s (expected %s)",
                            part_name,
                            androidPartitionType_toString(image_type),
                            androidPartitionType_toString(part_type));
                }
                VERBOSE_PRINT(init,
                              "Image type mismatch for %s partition: "
                              "%s (expected %s)",
                              part_name,
                              androidPartitionType_toString(image_type),
                              androidPartitionType_toString(part_type));

                part_mode = ANDROID_PARTITION_OPEN_MODE_MUST_WIPE;
            }
        }
    }

    VERBOSE_PRINT(init, "%s partition format: %s", part_name,
                  androidPartitionType_toString(part_type));

    bool need_make_empty = (part_mode == ANDROID_PARTITION_OPEN_MODE_MUST_WIPE);

    // Must be here to avoid freeing the string too early.
    std::string tempFile;

    if (readonly) {
        if (!state->backend->pathExists(part_file)) {
            return partition_config_error(
                    state, "Missing read-only %s partition image: %s",
                    part_name, part_file);
        }
        need_make_empty = false;
    } else {
        bool need_temp_partition = true;

        if (part_init_file) {
            need_make_empty = true;
        }

        if (part_file) {
            if (!state->backend->pathLockFile(part_file)) {
                dwarning("%s image already in use, changes will not persist!\n",
                         part_name);
            } else {
                need_temp_partition = false;

                // If the partition image is missing, create it.
                if (!state->backend->pathExists(part_file)) {
                    if (part_mode == ANDROID_PARTITION_OPEN_MODE_MUST_EXIST) {
                        return partition_config_error(
                                state, "Missing %s partition image: %s",
                                part_name, part_file);
                    }
                    if (!state->backend->pathEmptyFile(part_file)) {
                        return partition_config_error(
                                state, "Cannot create %s image file at %s: %s",
                                part_name, part_file, strerror(errno));
                    }
                    need_make_empty = true;
                }
            }
        }

        // Do we need a temporary partition image ?
        if (need_temp_partition) {
            if (!state->backend->pathCreateTempFile(&tempFile)) {
                return partition_config_error(state,
                                              "Could not create temp file for "
                                              "%s partition image: %s\n",
                                              part_name, strerror(errno));
            }
            part_file = tempFile.c_str();
            VERBOSE_PRINT(init, "Mapping '%s' partition image to %s", part_name,
                          part_file);

            need_make_empty = true;
        }

        // Do we need to copy the initial partition file into the real one?
        if (part_init_file) {
            if (!state->backend->pathCopyFile(part_file, part_init_file)) {
                return partition_config_error(
                        state,
                        "Could not copy initial %s partition "
                        "image to real one: %s\n",
                        part_name, strerror(errno));
            }
            need_make_empty = false;
        }
    }

    // Do we need to make the partition image empty?
    if (need_make_empty) {
        VERBOSE_PRINT(init, "Creating empty %s partition image at: %s",
                      part_name, part_file);
        int ret = state->backend->makeEmptyPartition(part_type, part_size,
                                                     part_file);
        if (ret < 0) {
            return partition_config_error(
                    state, "Could not create %s image file at %s: %s",
                    part_name, part_file, strerror(-ret));
        }
    }

    (*state->setup_func)(state->setup_opaque, part_name, part_size, part_file,
                         part_type, readonly);

    return true;
}

bool android_partition_configuration_setup(
        const AndroidPartitionConfiguration* config,
        AndroidPartitionSetupFunction setup_func,
        void* setup_opaque,
        char** error_message) {
    // Setup shared state.
    PartitionConfigState state[1] = {{
            .error_message = error_message,
            .setup_func = setup_func,
            .setup_opaque = setup_opaque,
            .backend = PartitionConfigBackend::get(),
    }};

    // Determine format of all partition images, if possible.
    // Note that _UNKNOWN means the file, if it exists, will be probed.
    AndroidPartitionType system_partition_type = ANDROID_PARTITION_TYPE_UNKNOWN;
    AndroidPartitionType userdata_partition_type =
            ANDROID_PARTITION_TYPE_UNKNOWN;
    AndroidPartitionType cache_partition_type = ANDROID_PARTITION_TYPE_UNKNOWN;

    {
        // Starting with Android 4.4.x, the ramdisk.img contains
        // an fstab.goldfish file that lists the format of each partition.
        // If the file exists, parse it to get the appropriate values.
        std::string fstab;

        if (state->backend->extractRamdiskFile(config->ramdisk_path,
                                               config->fstab_name, &fstab)) {
            VERBOSE_PRINT(init, "Ramdisk image contains %s file",
                          config->fstab_name);

            extractPartitionFormat(state, fstab, "system", "/system",
                                   &system_partition_type);

            extractPartitionFormat(state, fstab, "userdata", "/data",
                                   &userdata_partition_type);

            extractPartitionFormat(state, fstab, "cache", "/cache",
                                   &cache_partition_type);
        } else {
            VERBOSE_PRINT(init, "No %s file in ramdisk image",
                          config->fstab_name);
        }
    }

    // Initialize system partition image.
    if (!addNandImage(
                state, "system", system_partition_type,
                ANDROID_PARTITION_OPEN_MODE_MUST_EXIST,
                config->system_partition.size, config->system_partition.path,
                config->system_partition.init_path, !config->writable_system)) {
        return false;
    }

    // Initialize data partition image.
    AndroidPartitionOpenMode userdata_partition_mode = config->wipe_data ?
            ANDROID_PARTITION_OPEN_MODE_MUST_WIPE :
            ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED;

    if (!addNandImage(state, "userdata", userdata_partition_type,
                      userdata_partition_mode, config->data_partition.size,
                      config->data_partition.path,
                      config->data_partition.init_path, false)) {
        return false;
    }

    // Extend the userdata-qemu.img to the desired size - resize2fs can only
    // extend partitions to fill available space.
    // Resize userdata-qemu.img if the size is smaller than what config.ini
    // says.
    // This can happen as user wants a larger data partition without wiping it.
    // b.android.com/196926
    System::FileSize current_data_size(config->data_partition.size);
    System::get()->pathFileSize(config->data_partition.path,
                                &current_data_size);
    if ((config->wipe_data ||
         current_data_size < config->data_partition.size) &&
        userdata_partition_type == ANDROID_PARTITION_TYPE_EXT4) {
        if (current_data_size < config->data_partition.size) {
            VERBOSE_PRINT(init,
                          "userdata partition is resized from %d M to %d M\n",
                          current_data_size / (1024 * 1024),
                          config->data_partition.size / (1024 * 1024));
        }
        state->backend->resizeExt4Partition(config->data_partition.path,
                                            config->data_partition.size);
    }

    // Initialize cache partition image, if any. Its type depends on the
    // kernel version. For anything >= 3.10, it must be EXT4, or
    // YAFFS2 otherwise.
    if (config->cache_partition.path) {
        if (cache_partition_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            cache_partition_type = config->kernel_supports_yaffs2
                                           ? ANDROID_PARTITION_TYPE_YAFFS2
                                           : ANDROID_PARTITION_TYPE_EXT4;
        }

        AndroidPartitionOpenMode cache_partition_mode =
                (config->wipe_data
                         ? ANDROID_PARTITION_OPEN_MODE_MUST_WIPE
                         : ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED);

        if (!addNandImage(state, "cache", cache_partition_type,
                          cache_partition_mode, config->cache_partition.size,
                          config->cache_partition.path, NULL, false)) {
            return false;
        }
    }

    return true;
}
