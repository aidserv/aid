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

#include "android/filesystems/internal/PartitionConfigBackend.h"

#include <gtest/gtest.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// A special file path used during unit-testing that corresponds to
// a non-existing file. See TestPartitionConfigBackend::pathExists() below.
#define DOESNT_EXIST_PREFIX "/DOES_NOT_EXISTS"

// A special file path to indicate a bad ramdisk.img path.
// Using this with extractRamdiskFile() below will return false.
static const char kBadRamdiskFile[] = "/BAD_RAMDISK.IMG";

// A special file path to indicate a ramdisk.img that contains an fstab
// file that will
static const char kYaffsFstabFile[] = "fstab.yaffs2";

// Fake fstab content corresponding to yaffs2 partitions.
static const char kYaffsFstabContent[] = "yaffs2";

// A special file path prefix that cannot be locked.
// This is a macro to use string concatenation at compile time,
// e.g. CANNOT_LOCK_PREFIX "2"
#define CANNOT_LOCK_PREFIX "/CANNOT_LOCK_FILE"

#define CANNOT_COPY_PREFIX "/CANNOT_COPY"

// A special file path prefix corresponding to an YAFFS2 partition.
#define YAFFS_PATH_PREFIX "/YAFFS_FILE"

static bool strStartsWith(const char* path, const char* prefix) {
    return !strncmp(path, prefix, strlen(prefix));
}

// Base unittest version of PartitionConfigBackend interface.
// All methods return success, and ext4 as the default partition type.
class TestPartitionConfigBackend
        : public android::internal::PartitionConfigBackend {
public:
    TestPartitionConfigBackend()
        : mPrevBackend(nullptr), mCommands(), mTempCounter(0) {
        // Save previous backend then inject self into the process.
        mPrevBackend = PartitionConfigBackend::setForTesting(this);
    }

    virtual ~TestPartitionConfigBackend() {
        // Restore previous backend.
        PartitionConfigBackend::setForTesting(mPrevBackend);
    }

    virtual bool pathExists(const char* path) override {
        if (strStartsWith(path, DOESNT_EXIST_PREFIX)) {
            return false;
        }
        return true;
    }

    virtual bool pathEmptyFile(const char* path) override {
        addCommand("EMPTY [%s]", path);
        return true;
    }

    virtual bool pathCopyFile(const char* dst, const char* src) override {
        if (strStartsWith(src, CANNOT_COPY_PREFIX)) {
            return false;
        }
        addCommand("COPY [%s] <- [%s]", dst, src);
        return true;
    }

    virtual bool pathLockFile(const char* path) override {
        if (strStartsWith(path, CANNOT_LOCK_PREFIX)) {
            errno = EINVAL;
            return false;
        }
        addCommand("LOCK [%s]", path);
        return true;
    }

    virtual bool pathCreateTempFile(std::string* path) override {
        char tmp[200];
        snprintf(tmp, sizeof(tmp), "/tmp/tempfile%d", ++mTempCounter);
        *path = tmp;
        addCommand("TEMPFILE [%s]", tmp);
        return true;
    }

    virtual AndroidPartitionType probePartitionFileType(
            const char* path) override {
        if (strStartsWith(path, YAFFS_PATH_PREFIX)) {
            return ANDROID_PARTITION_TYPE_YAFFS2;
        }
        return ANDROID_PARTITION_TYPE_EXT4;
    }

    virtual bool extractRamdiskFile(const char* ramdisk_path,
                                    const char* file_path,
                                    std::string* out) override {
        if (!strcmp(ramdisk_path, kBadRamdiskFile)) {
            return false;
        }
        if (!strcmp(file_path, kYaffsFstabFile)) {
            *out = kYaffsFstabContent;
            return true;
        }
        *out = "";
        return true;
    }

    virtual bool parsePartitionFormat(const std::string& fstab,
                                      const char* mountPath,
                                      std::string* partitionFormat) override {
        if (fstab == kYaffsFstabContent) {
            *partitionFormat = "yaffs2";
        } else {
            *partitionFormat = "ext4";
        }
        return true;
    }

    virtual bool makeEmptyPartition(AndroidPartitionType partitionType,
                                    uint64_t partitionSize,
                                    const char* partitionPath) override {
        addCommand("EMPTY_PARTITION format=%s size=%lld [%s]",
                   androidPartitionType_toString(partitionType),
                   static_cast<unsigned long long>(partitionSize),
                   partitionPath);
        return true;
    }

    // Resize an existing ext4 partition at |partitionPath| to a new
    // size in bytes given by |partitionSize|.
    virtual void resizeExt4Partition(const char* partitionPath,
                                     uint64_t partitionSize) override {
        addCommand("EXT4_RESIZE size=%lld [%s]",
                   static_cast<unsigned long long>(partitionSize),
                   partitionPath);
    }

    const std::string& commands() const { return mCommands; }

private:
    void addCommand(const char* fmt, ...) {
        char* str = NULL;
        va_list args;
        va_start(args, fmt);
        vasprintf(&str, fmt, args);
        va_end(args);
        mCommands += str;
        mCommands += "\n";
        free(str);
    }

    PartitionConfigBackend* mPrevBackend;
    std::string mCommands;
    int mTempCounter;
};

// A helper class used to collect the virtual NAND partition setup
// that is performed by android_partition_configuration_setup().
// Create a new instance, and use its |setupPartition| method address
// as the |setup_func| parameter, and |this| as |setup_opaque|.
class PartitionCollector {
public:
    // Virtual partition information after setup.
    struct Partition {
        std::string name;
        uint64_t size;
        std::string path;
        AndroidPartitionType format;
        bool readonly;
    };

    // All virtual partitions, as a simple public vector.
    std::vector<Partition> partitions;

    // A callback used as a |setup_func| parameter when calling
    // android_partition_configuration_setup(). Use the instance's |this|
    // value as |setup_opaque|.
    static void setupPartition(void* opaque,
                               const char* name,
                               uint64_t size,
                               const char* path,
                               AndroidPartitionType format,
                               bool readonly) {
        auto collector = static_cast<PartitionCollector*>(opaque);

        Partition part;
        part.name = name;
        part.size = size;
        part.path = path;
        part.format = format;
        part.readonly = readonly;

        collector->partitions.push_back(part);
    }
};

// Save a lot of typing.
typedef PartitionCollector::Partition Partition;

// Helper function to check the results of a given partition configuration.
// |config| is the input AndroidPartitionConfiguration instance.
// |expectedCommands| is the expected commands string.
// |expectedPartitions| is an array of expected Partition descriptors.
// |expectedPartitionsCount| is the number of items in the array.
static void checkConfig(const AndroidPartitionConfiguration* config,
                        const char* expectedCommands,
                        const Partition* expectedPartitions,
                        size_t expectedPartitionsCount) {
    TestPartitionConfigBackend testBackend;
    PartitionCollector collector;

    char* error_message = NULL;
    EXPECT_TRUE(android_partition_configuration_setup(
            config, PartitionCollector::setupPartition, &collector,
            &error_message));

    if (error_message) {
        EXPECT_STREQ("", error_message);
    } else {
        std::vector<Partition>& partitions = collector.partitions;

        EXPECT_EQ(expectedPartitionsCount, partitions.size());

        for (size_t n = 0; n < expectedPartitionsCount; ++n) {
            const Partition& expected = expectedPartitions[n];

            EXPECT_EQ(expected.name, partitions[n].name);
            EXPECT_EQ(expected.size, partitions[n].size);
            EXPECT_EQ(expected.path, partitions[n].path) << "#" << n;
            EXPECT_EQ(expected.format, partitions[n].format);
            EXPECT_EQ(expected.readonly, partitions[n].readonly);
        }
    }

    EXPECT_STREQ(expectedCommands, testBackend.commands().c_str());
}

static void checkErrorConfig(const AndroidPartitionConfiguration* config,
                             const char* expectedCommands,
                             const char* expectedError) {
    TestPartitionConfigBackend testBackend;
    PartitionCollector collector;

    char* error_message = NULL;
    EXPECT_FALSE(android_partition_configuration_setup(
            config, PartitionCollector::setupPartition, &collector,
            &error_message));
    if (!error_message) {
        EXPECT_TRUE(error_message);
    } else {
        EXPECT_STREQ(expectedError, error_message);
        free(error_message);
    }

    EXPECT_STREQ(expectedCommands, testBackend.commands().c_str());
}

TEST(PartitionConfig, normalSetup) {
    static const AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = nullptr,
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
            .writable_system = false,
    };

    static const char kExpectedCommands[] =
            "LOCK [/avd/userdata-qemu.img]\n"
            "LOCK [/avd/cache.img]\n";

    static const PartitionCollector::Partition kExpectedPartitions[] = {
            {"system", 123456ULL, "/images/system.img",
             ANDROID_PARTITION_TYPE_EXT4, true},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, wipeData) {
    AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = "/avd/system.img",
                            .init_path = nullptr,
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = "/images/userdata.img",
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = true,
            .writable_system = false,
    };

    static const char kExpectedCommands[] =
            "LOCK [/avd/userdata-qemu.img]\n"
            "COPY [/avd/userdata-qemu.img] <- [/images/userdata.img]\n"
            "EXT4_RESIZE size=400000 [/avd/userdata-qemu.img]\n"
            "LOCK [/avd/cache.img]\n"
            "EMPTY_PARTITION format=ext4 size=100000 [/avd/cache.img]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/avd/system.img",
             ANDROID_PARTITION_TYPE_EXT4, true},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, writableSystem) {
    AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = nullptr,
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
            .writable_system = true,
    };

    static const char kExpectedCommands[] =
            "TEMPFILE [/tmp/tempfile1]\n"
            "COPY [/tmp/tempfile1] <- [/images/system.img]\n"
            "LOCK [/avd/userdata-qemu.img]\n"
            "LOCK [/avd/cache.img]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/tmp/tempfile1", ANDROID_PARTITION_TYPE_EXT4,
             false},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, persistentWritableSystem) {
    AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = "/avd/system.img",
                            .init_path = nullptr,
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = "/images/userdata.img",
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = true,
            .writable_system = true,
    };

    static const char kExpectedCommands[] =
            "LOCK [/avd/system.img]\n"
            "LOCK [/avd/userdata-qemu.img]\n"
            "COPY [/avd/userdata-qemu.img] <- [/images/userdata.img]\n"
            "EXT4_RESIZE size=400000 [/avd/userdata-qemu.img]\n"
            "LOCK [/avd/cache.img]\n"
            "EMPTY_PARTITION format=ext4 size=100000 [/avd/cache.img]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/avd/system.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, persistentWritableSystemWithWipeData) {
    AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = "/avd/system.img",
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = "/images/userdata.img",
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = true,
            .writable_system = true,
    };

    static const char kExpectedCommands[] =
            "LOCK [/avd/system.img]\n"
            "COPY [/avd/system.img] <- [/images/system.img]\n"
            "LOCK [/avd/userdata-qemu.img]\n"
            "COPY [/avd/userdata-qemu.img] <- [/images/userdata.img]\n"
            "EXT4_RESIZE size=400000 [/avd/userdata-qemu.img]\n"
            "LOCK [/avd/cache.img]\n"
            "EMPTY_PARTITION format=ext4 size=100000 [/avd/cache.img]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/avd/system.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, LockedFiles) {
    static const char kLockedDataFile[] = CANNOT_LOCK_PREFIX "_data";
    static const char kLockedSystemFile[] = CANNOT_LOCK_PREFIX "_system";
    static const char kLockedCacheFile[] = CANNOT_LOCK_PREFIX "_cache";

    AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = kLockedSystemFile,
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = kLockedDataFile,
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = kLockedCacheFile,
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
    };

    static const char kExpectedCommands[] =
            "TEMPFILE [/tmp/tempfile1]\n"
            "EMPTY_PARTITION format=ext4 size=400000 [/tmp/tempfile1]\n"
            "TEMPFILE [/tmp/tempfile2]\n"
            "EMPTY_PARTITION format=ext4 size=100000 [/tmp/tempfile2]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, kLockedSystemFile,
             ANDROID_PARTITION_TYPE_EXT4, true},
            {"userdata", 400000ULL, "/tmp/tempfile1",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/tmp/tempfile2", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, MissingDataPartition) {
    static const char kMissingDataFile[] = DOESNT_EXIST_PREFIX "_data";

    static const AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = nullptr,
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = kMissingDataFile,
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
            .writable_system = false,
    };

    static const char kExpectedCommands[] =
            "LOCK [/DOES_NOT_EXISTS_data]\n"
            "EMPTY [/DOES_NOT_EXISTS_data]\n"
            "EMPTY_PARTITION format=ext4 size=400000 [/DOES_NOT_EXISTS_data]\n"
            "LOCK [/avd/cache.img]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/images/system.img",
             ANDROID_PARTITION_TYPE_EXT4, true},
            {"userdata", 400000ULL, kMissingDataFile,
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, "/avd/cache.img", ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, MissingSystemPartition) {
    static const char kMissingSystemFile[] = DOESNT_EXIST_PREFIX "_system";

    static const AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = kMissingSystemFile,
                            .init_path = nullptr,
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
            .writable_system = false,
    };

    static const char kExpectedCommands[] = "";

    static const char kExpectedError[] =
            "Missing read-only system partition image: /DOES_NOT_EXISTS_system";

    checkErrorConfig(&config, kExpectedCommands, kExpectedError);
}

TEST(PartitionConfig, MissingCacheFile) {
    static const char kMissingCacheFile[] = DOESNT_EXIST_PREFIX "_cache";

    static const AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = "fstab.unittest",
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = nullptr,
                            .init_path = "/images/system.img",
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = "/avd/userdata-qemu.img",
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = kMissingCacheFile,
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = false,
            .wipe_data = false,
            .writable_system = false,
    };

    static const char kExpectedCommands[] =
            "LOCK [/avd/userdata-qemu.img]\n"
            "LOCK [/DOES_NOT_EXISTS_cache]\n"
            "EMPTY [/DOES_NOT_EXISTS_cache]\n"
            "EMPTY_PARTITION format=ext4 size=100000 "
            "[/DOES_NOT_EXISTS_cache]\n";

    static const Partition kExpectedPartitions[3] = {
            {"system", 123456ULL, "/images/system.img",
             ANDROID_PARTITION_TYPE_EXT4, true},
            {"userdata", 400000ULL, "/avd/userdata-qemu.img",
             ANDROID_PARTITION_TYPE_EXT4, false},
            {"cache", 100000ULL, kMissingCacheFile, ANDROID_PARTITION_TYPE_EXT4,
             false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}

TEST(PartitionConfig, Yaffs2Partitions) {
    static const char kYaffsSystemImage[] = YAFFS_PATH_PREFIX "_system";
    static const char kYaffsDataImage[] = YAFFS_PATH_PREFIX "_data";

    static const AndroidPartitionConfiguration config = {
            .ramdisk_path = "/foo/ramdisk.img",
            .fstab_name = kYaffsFstabFile,
            .system_partition =
                    {
                            .size = 123456ULL,
                            .path = kYaffsSystemImage,
                            .init_path = nullptr,
                    },
            .data_partition =
                    {
                            .size = 400000ULL,
                            .path = kYaffsDataImage,
                            .init_path = nullptr,
                    },
            .cache_partition =
                    {
                            .size = 100000ULL,
                            .path = "/avd/cache.img",
                            .init_path = nullptr,
                    },
            .kernel_supports_yaffs2 = true,
            .wipe_data = false,
            .writable_system = false,
    };

    static const char kExpectedCommands[] =
            "LOCK [/YAFFS_FILE_data]\n"
            "LOCK [/avd/cache.img]\n"
            "EMPTY_PARTITION format=yaffs2 size=100000 [/avd/cache.img]\n";

    static const PartitionCollector::Partition kExpectedPartitions[] = {
            {"system", 123456ULL, kYaffsSystemImage,
             ANDROID_PARTITION_TYPE_YAFFS2, true},
            {"userdata", 400000ULL, kYaffsDataImage,
             ANDROID_PARTITION_TYPE_YAFFS2, false},
            {"cache", 100000ULL, "/avd/cache.img",
             ANDROID_PARTITION_TYPE_YAFFS2, false},
    };

    const size_t kExpectedPartitionsSize = ARRAY_SIZE(kExpectedPartitions);

    checkConfig(&config, kExpectedCommands, kExpectedPartitions,
                kExpectedPartitionsSize);
}
