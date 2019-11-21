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

#pragma once

#include <string>
#include <vector>

namespace android {
namespace opengl {

const char kGLES12TranslatorName[] = "GLES12Translator";

class EmuglBackendList {
public:
    // Parse the content of |execDir|/<lib>/ for Emugl backends.
    // |programBitness| can be 0 (autodetect), 32 or 64, and determines
    // the value of <lib> which will be 'lib' for 32-bit systems,
    // and 'lib64' for 64-bit ones.
    EmuglBackendList(const char* execDir, int programBitness);

    // Return the name of the default Emugl backend.
    const std::string& defaultName() const { return mDefaultName; }

    // Return the list of installed Emugl backends.
    const std::vector<std::string>& names() const { return mNames; }

    // Returns true if |name| is part of names().
    bool contains(const char* name) const;

    // Convert the name of an Emugl backend into the path of the
    // corresponding sub-directory, if it exits, or NULL otherwise.
    std::string getLibDirPath(const char* name);

    std::string getGLES12TranslatorLibName();

    // List of supported Emugl shared libraries.
    enum Library {
        LIBRARY_NONE,
        LIBRARY_EGL,
        LIBRARY_GLESv1,
        LIBRARY_GLESv2,
        LIBRARY_GLES12TR
    };

    // Probe the library directory for Emugl backend |name| and return
    // the path of one of the EmuGL shared libraries for it. The result
    // will be empty if there is no such library.
    // |library| is a library type. On success, return true and sets
    // |*libPath| to the library's path value. On failure, return false.
    bool getBackendLibPath(const char* name, Library library,
                           std::string* libPath);

private:
    std::string mDefaultName;
    std::vector<std::string> mNames;
    int mProgramBitness;
    std::string mExecDir;
};

}  // namespace opengl
}  // namespace android
