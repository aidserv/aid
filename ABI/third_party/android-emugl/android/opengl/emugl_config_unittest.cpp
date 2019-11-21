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

#include "android/opengl/emugl_config.h"
#include "android/opengl/EmuglBackendList.h"
#include "android/opengl/gpuinfo.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

#if defined(_WIN32)
#  define LIB_NAME(x)  x ".dll"
#elif defined(__APPLE__)
#  define LIB_NAME(x)  "lib" x ".dylib"
#else
#  define LIB_NAME(x)  "lib" x ".so"
#endif

static std::string makeLibSubPath(const char* name) {
    return StringFormat("%s/%s/%s",
                        System::get()->getLauncherDirectory().c_str(),
                        System::kLibSubDir, name);
}

static void makeLibSubDir(TestTempDir* dir, const char* name) {
    dir->makeSubDir(makeLibSubPath(name).c_str());
}

static void makeLibSubFile(TestTempDir* dir, const char* name) {
    dir->makeSubFile(makeLibSubPath(name).c_str());
}

TEST(EmuglConfig, init) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    makeLibSubDir(myDir, "gles_vendor");
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("GLESv2"));

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, "host", NULL, 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "host", NULL, 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "mesa", NULL, 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "host", "off", 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "disable", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, "host", "on", 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, NULL, "on", 0, false, false, false,
                    WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "mesa", "enable", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }


    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "vendor", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode",
                     config.status);
        emuglConfig_setupEnv(&config);
        EXPECT_TRUE(
                strstr(System::get()->envGet("ANDROID_GLESv1_LIB").c_str(),
                       android::opengl::kGLES12TranslatorName) != NULL);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "vendor", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "vendor", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "guest", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "guest", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "guest", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_FALSE(config.enabled);
    }
}

TEST(EmuglConfig, initFromUISetting) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    makeLibSubDir(myDir, "gles_angle");
    makeLibSubFile(myDir, "gles_angle/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_angle/" LIB_NAME("GLESv2"));

    makeLibSubDir(myDir, "gles_angle9");
    makeLibSubFile(myDir, "gles_angle9/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_angle9/" LIB_NAME("GLESv2"));

    makeLibSubDir(myDir, "gles_swiftshader");
    makeLibSubFile(myDir, "gles_swiftshader/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_swiftshader/" LIB_NAME("GLESv2"));

    // If the gpu command line option is specified, the UI setting is overridden.
    for (int i = 0; i < 10; i++) {
        {
            EmuglConfig config;
            EXPECT_TRUE(emuglConfig_init(
                        &config, false, "host", "on", 0, false, false, false,
                        (enum WinsysPreferredGlesBackend)i));
            EXPECT_TRUE(config.enabled);
            EXPECT_STREQ("host", config.backend);
            EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
        }

        {
            EmuglConfig config;
            EXPECT_TRUE(emuglConfig_init(
                        &config, false, "guest", "auto", 0, false, false, false,
                        WINSYS_GLESBACKEND_PREFERENCE_AUTO));
            EXPECT_FALSE(config.enabled);
            EXPECT_STREQ("GPU emulation is disabled", config.status);
        }
    }

    // If the UI setting is not "auto", and there is no gpu command line option,
    // then use the UI setting, regardless of the AVD config.
    for (int i = 1; i < 5; i++) {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, "host", NULL, 0, false, false, false,
                    (enum WinsysPreferredGlesBackend)i));
        EXPECT_TRUE(config.enabled);
        switch (i) {
        case 0:
            EXPECT_STREQ("host", config.backend);
            break;
        case 1:
            EXPECT_STREQ("angle", config.backend);
            break;
        case 2:
            EXPECT_STREQ("angle9", config.backend);
            break;
        case 3:
            EXPECT_STREQ("swiftshader", config.backend);
            break;
        case 4:
            EXPECT_STREQ("host", config.backend);
            break;
        default:
            break;
        }
    }
}

TEST(EmuglConfig, initGLESv2Only) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_angle");

#ifdef _WIN32
    const char* kEglLibName = "libEGL.dll";
    const char* kGLESv1LibName = "libGLES_CM.dll";
    const char* kGLESv2LibName = "libGLESv2.dll";
#elif defined(__APPLE__)
    const char* kEglLibName = "libEGL.dylib";
    const char* kGLESv1LibName = "libGLES_CM.dylib";
    const char* kGLESv2LibName = "libGLESv2.dylib";
#else
    const char* kEglLibName = "libEGL.so";
    const char* kGLESv1LibName = "libGLES_CM.so";
    const char* kGLESv2LibName = "libGLESv2.so";
#endif

    std::string eglLibPath = StringFormat("gles_angle/%s", kEglLibName);
    std::string GLESv1LibPath = StringFormat("gles_angle/%s", kGLESv1LibName);
    std::string GLESv2LibPath = StringFormat("gles_angle/%s", kGLESv2LibName);

    makeLibSubFile(myDir, eglLibPath.c_str());
    makeLibSubFile(myDir, GLESv2LibPath.c_str());

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "angle", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("angle", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'angle' mode",
                     config.status);
        emuglConfig_setupEnv(&config);
        EXPECT_TRUE(
                strstr(System::get()->envGet("ANDROID_GLESv1_LIB").c_str(),
                       android::opengl::kGLES12TranslatorName) != NULL);
    }

    makeLibSubFile(myDir, GLESv1LibPath.c_str());

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "angle", "auto", 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("angle", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'angle' mode",
                     config.status);
        emuglConfig_setupEnv(&config);
        EXPECT_FALSE(
                strstr(System::get()->envGet("ANDROID_GLESv1_LIB").c_str(),
                       android::opengl::kGLES12TranslatorName) != NULL);
    }
}

TEST(EmuglConfig, initNxWithSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_swiftshader");
    makeLibSubFile(myDir, "gles_swiftshader/libEGL.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLES_CM.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLESv2.so");

    testSys.setRemoteSessionType("NX");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("swiftshader", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'swiftshader' mode", config.status);
}

TEST(EmuglConfig, initNxWithoutSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.setRemoteSessionType("NX");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under NX without Swiftshader", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_swiftshader");
    makeLibSubFile(myDir, "gles_swiftshader/libEGL.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLES_CM.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLESv2.so");

    testSys.setRemoteSessionType("Chrome Remote Desktop");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("swiftshader", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'swiftshader' mode", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithoutSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.setRemoteSessionType("Chrome Remote Desktop");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under Chrome Remote Desktop without Swiftshader", config.status);
}

TEST(EmuglConfig, initNoWindowWithSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_swiftshader");
    makeLibSubFile(myDir, "gles_swiftshader/libEGL.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLES_CM.so");
    makeLibSubFile(myDir, "gles_swiftshader/libGLESv2.so");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, true, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("swiftshader", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'swiftshader' mode", config.status);
}

TEST(EmuglConfig, initNoWindowWithoutSwiftshader) {
    TestSystem testSys("foo", System::kProgramBitness, "/");
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, true, false, false,
                WINSYS_GLESBACKEND_PREFERENCE_AUTO));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled (-no-window without Swiftshader)",
                 config.status);
}

TEST(EmuglConfig, setupEnv) {
}

TEST(EmuglConfig, hostGpuProps) {
    TestSystem testSys("/usr", 32);
    testSys.setLiveUnixTime(true);
    GpuInfoList* gpulist = const_cast<GpuInfoList*>(&globalGpuInfoList());
    gpulist->clear();
    EXPECT_TRUE(gpulist->infos.size() == 0);
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU0 MAKE";
    gpulist->currGpu().model = "TEST GPU0 MODEL";
    gpulist->currGpu().device_id = "TEST GPU0 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU0 REVISIONID";
    gpulist->currGpu().version = "TEST GPU0 VERSION";
    gpulist->currGpu().renderer = "TEST GPU0 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU1 MAKE";
    gpulist->currGpu().model = "TEST GPU1 MODEL";
    gpulist->currGpu().device_id = "TEST GPU1 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU1 REVISIONID";
    gpulist->currGpu().version = "TEST GPU1 VERSION";
    gpulist->currGpu().renderer = "TEST GPU1 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU2 MAKE";
    gpulist->currGpu().model = "TEST GPU2 MODEL";
    gpulist->currGpu().device_id = "TEST GPU2 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU2 REVISIONID";
    gpulist->currGpu().version = "TEST GPU2 VERSION";
    gpulist->currGpu().renderer = "TEST GPU2 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU3 MAKE";
    gpulist->currGpu().model = "TEST GPU3 MODEL";
    gpulist->currGpu().device_id = "TEST GPU3 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU3 REVISIONID";
    gpulist->currGpu().version = "TEST GPU3 VERSION";
    gpulist->currGpu().renderer = "TEST GPU3 RENDERER";

    emugl_host_gpu_prop_list gpu_props = emuglConfig_get_host_gpu_props();
    EXPECT_TRUE(gpu_props.num_gpus == 4);

    EXPECT_STREQ("TEST GPU0 MAKE", gpu_props.props[0].make);
    EXPECT_STREQ("TEST GPU1 MAKE", gpu_props.props[1].make);
    EXPECT_STREQ("TEST GPU2 MAKE", gpu_props.props[2].make);
    EXPECT_STREQ("TEST GPU3 MAKE", gpu_props.props[3].make);

    EXPECT_STREQ("TEST GPU0 MODEL", gpu_props.props[0].model);
    EXPECT_STREQ("TEST GPU1 MODEL", gpu_props.props[1].model);
    EXPECT_STREQ("TEST GPU2 MODEL", gpu_props.props[2].model);
    EXPECT_STREQ("TEST GPU3 MODEL", gpu_props.props[3].model);

    EXPECT_STREQ("TEST GPU0 DEVICEID", gpu_props.props[0].device_id);
    EXPECT_STREQ("TEST GPU1 DEVICEID", gpu_props.props[1].device_id);
    EXPECT_STREQ("TEST GPU2 DEVICEID", gpu_props.props[2].device_id);
    EXPECT_STREQ("TEST GPU3 DEVICEID", gpu_props.props[3].device_id);

    EXPECT_STREQ("TEST GPU0 REVISIONID", gpu_props.props[0].revision_id);
    EXPECT_STREQ("TEST GPU1 REVISIONID", gpu_props.props[1].revision_id);
    EXPECT_STREQ("TEST GPU2 REVISIONID", gpu_props.props[2].revision_id);
    EXPECT_STREQ("TEST GPU3 REVISIONID", gpu_props.props[3].revision_id);

    EXPECT_STREQ("TEST GPU0 VERSION", gpu_props.props[0].version);
    EXPECT_STREQ("TEST GPU1 VERSION", gpu_props.props[1].version);
    EXPECT_STREQ("TEST GPU2 VERSION", gpu_props.props[2].version);
    EXPECT_STREQ("TEST GPU3 VERSION", gpu_props.props[3].version);

    EXPECT_STREQ("TEST GPU0 RENDERER", gpu_props.props[0].renderer);
    EXPECT_STREQ("TEST GPU1 RENDERER", gpu_props.props[1].renderer);
    EXPECT_STREQ("TEST GPU2 RENDERER", gpu_props.props[2].renderer);
    EXPECT_STREQ("TEST GPU3 RENDERER", gpu_props.props[3].renderer);

    free_emugl_host_gpu_props(gpu_props);
}

TEST(EmuglConfig, hostGpuProps_empty) {
    TestSystem testSys("/usr", 32);
    testSys.setLiveUnixTime(true);
    GpuInfoList* gpulist = const_cast<GpuInfoList*>(&globalGpuInfoList());
    gpulist->clear();
    EXPECT_TRUE(gpulist->infos.size() == 0);

    emugl_host_gpu_prop_list gpu_props = emuglConfig_get_host_gpu_props();
    EXPECT_TRUE(gpu_props.num_gpus == 0);
    free_emugl_host_gpu_props(gpu_props);
}

}  // namespace base
}  // namespace android
