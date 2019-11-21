// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/filesystems/ramdisk_extractor.h"

#include "android/base/Compiler.h"
#include "android/base/Log.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define DEBUG 0

#if DEBUG
#  define D(...)   printf(__VA_ARGS__), fflush(stdout)
#else
#  define D(...)   ((void)0)
#endif

// Ramdisk images are gzipped cpio archives using the new ASCII
// format as described at [1]. Hence this source file first implements
// a gzip-based input stream class, then 
//
// [1] http://people.freebsd.org/~kientzle/libarchive/man/cpio.5.txt

namespace {

// Helper class used to implement a gzip-based input stream.
// Usage is as follows:
//
//     GZipInputStream input(filePath);
//     if (input.error()) {
//        fprintf(stderr, "Could not open file: %s\n",
//                filePath, strerror(input.error()));
//     } else {
//        uint8_t header[32];
//        if (!doRead(&input, header, sizeof header)) {
//           ... error, could not read header.
//        }
//     }
//      .. stream is closed automatically on scope exit.
class GZipInputStream {
public:
    // Open a new input stream to read from file |filePath|.
    // The constructor can never fail, so call error() after
    // this to see if an error occured.
    explicit GZipInputStream(const char* filePath) {
        mFile = gzopen(filePath, "rb");
        if (mFile == Z_NULL) {
            mFile = NULL;
            mError = errno;
        } else {
            mError = 0;
        }
    }

    // Return the last error that occured on this stream,
    // or 0 if everything's well. Note that as soon as an
    // error occurs, the stream cannot be used anymore.
    int error() const { return mError; }

    // Close the stream, note that this is called automatically
    // from the destructor, but clients might want to do this
    // before.
    void close() {
        if (mFile) {
            gzclose(mFile);
            mFile = NULL;
        }
    }

    ~GZipInputStream() {
        close();
    }

    // Try to read up to |len| bytes of data from the input
    // stream into |buffer|. On success, return true and sets
    // |*readBytes| to the number of bytes that were read.
    // On failure, return false and set error().
    bool tryRead(void* buffer, size_t len, size_t* readBytes) {
        *readBytes = 0;

        if (mError) {
            return false;
        }

        if (len == 0) {
            return true;
        }

        // Warning, gzread() takes an unsigned int parameter for
        // the length, but will return an error if its value
        // exceeds INT_MAX anyway.
        char* buff = reinterpret_cast<char*>(buffer);

        while (len > 0) {
            size_t avail = len;
            if (avail > INT_MAX)
                avail = INT_MAX;

            int ret = gzread(mFile, buff, static_cast<unsigned int>(avail));
            if (ret < 0) {
                gzerror(mFile, &mError);
                break;
            }
            if (ret == 0) {
                if (gzeof(mFile)) {
                    break;
                }
                gzerror(mFile, &mError);
                break;
            }
            len -= ret;
            buff += ret;
            *readBytes += ret;
        }

        return (!mError && *readBytes > 0);
    }

    // Read exactly |len| bytes of data into |buffer|. On success,
    // return true, on failure, return false and set error().
    bool doRead(void* buffer, size_t len) {
        size_t readCount = 0;
        if (!tryRead(buffer, len, &readCount)) {
            return false;
        }
        if (readCount < len) {
            mError = EIO;
            return false;
        }
        return true;
    }

    bool doSkip(size_t len) {
        if (mError) {
            return false;
        }
        if (gzseek(mFile, len, SEEK_CUR) < 0) {
            gzerror(mFile, &mError);
            return false;
        }
        return true;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(GZipInputStream);

    gzFile mFile;
    int mError;
};

// Parse an hexadecimal string of 8 characters. On success,
// return true and sets |*value| to its value. On failure,
// return false.
bool parse_hex8(const char* input, uint32_t* value) {
    uint32_t result = 0;
    for (int n = 0; n < 8; ++n) {
        int c = input[n];
        unsigned d = static_cast<unsigned>(c - '0');
        if (d >= 10) {
            d = static_cast<unsigned>(c - 'a');
            if (d >= 6) {
                d = static_cast<unsigned>(c - 'A');
                if (d >= 6) {
                    return false;
                }
            }
            d += 10;
        }
        result = (result << 4) | d;
    }
    *value = result;
    return true;
}

}  // namespace

bool android_extractRamdiskFile(const char* ramdiskPath,
                                const char* fileName,
                                char** out,
                                size_t* outSize) {
    *out = NULL;
    *outSize = 0;

    GZipInputStream input(ramdiskPath);
    if (input.error()) {
        errno = input.error();
        return false;
    }

    // Type of cpio new ASCII header.
    struct cpio_newc_header {
        char c_magic[6];
        char c_ino[8];
        char c_mode[8];
        char c_uid[8];
        char c_gid[8];
        char c_nlink[8];
        char c_mtime[8];
        char c_filesize[8];
        char c_devmajor[8];
        char c_devminor[8];
        char c_rdevmajor[8];
        char c_rdevminor[8];
        char c_namesize[8];
        char c_check[8];
    };

    size_t fileNameLen = strlen(fileName);

    for (;;) {
        // Read the header then check it.
        cpio_newc_header header;
        if (!input.doRead(&header, sizeof header)) {
            // Assume end of input here.
            D("Could not find %s in ramdisk image at %s\n",
              fileName, ramdiskPath);
            return false;
        }

        D("HEADER %.6s\n", header.c_magic);
        if (memcmp(header.c_magic, "070701", 6) != 0) {
            D("Not a valid ramdisk image file: %s\n", ramdiskPath);
            errno = EINVAL;
            return false;
        }

        // Compare file names, note that files with a size of 0 are
        // hard links and should be ignored.
        uint32_t nameSize;
        uint32_t entrySize;
        if (!parse_hex8(header.c_namesize, &nameSize) ||
            !parse_hex8(header.c_filesize, &entrySize)) {
            D("Could not parse ramdisk file entry header!");
            break;
        }

        D("---- %d nameSize=%d entrySize=%d\n", __LINE__, nameSize, entrySize);

        // The header is followed by the name, followed by 4-byte padding
        // with NUL bytes. Compute the number of bytes to skip over the
        // name.
        size_t skipName =
                ((sizeof header + nameSize + 3) & ~3) - sizeof header;

        // The file data is 4-byte padded with NUL bytes too.
        size_t skipFile = (entrySize + 3) & ~3;
        size_t skipCount = skipName + skipFile;

        // Last record is named 'TRAILER!!!' and indicates end of archive.
        static const char kTrailer[] = "TRAILER!!!";
        static const size_t kTrailerSize = sizeof(kTrailer) - 1U;

        if ((entrySize == 0 || nameSize != fileNameLen + 1U) &&
            nameSize != kTrailerSize + 1U) {
            D("---- %d Skipping\n", __LINE__);
        } else {
            // Read the name and compare it.
            nameSize -= 1U;
            std::string entryName;
            entryName.resize(nameSize);
            if (!input.doRead(&entryName[0], nameSize)) {
                D("Could not read ramdisk file entry name!");
                break;
            }
            D("---- %d Name=[%s]\n", __LINE__, entryName.c_str());
            skipCount -= nameSize;

            // Check for last entry.
            if (nameSize == kTrailerSize &&
                !strcmp(entryName.c_str(), kTrailer)) {
                D("End of archive reached. Could not find %s in ramdisk image at %s",
                  fileName, ramdiskPath);
                return false;
            }

            // Check for the search file name.
            if (nameSize == entryName.size() &&
                !strcmp(entryName.c_str(), fileName)) {
                // Found it !! Skip over padding.
                if (!input.doSkip(skipName - nameSize)) {
                    D("Could not skip ramdisk name entry!");
                    break;
                }

                // Then read data into head-allocated buffer.
                *out = reinterpret_cast<char*>(malloc(entrySize));
                *outSize = entrySize;

                return input.doRead(*out, entrySize);
            }
        }
        if (!input.doSkip(skipCount)) {
            D("Could not skip ramdisk entry!");
            break;
        }
    }

    errno = input.error();
    return false;
}
