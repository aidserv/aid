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

#include "android/opengl/gpuinfo.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

static const char osx_single_gpu[] =
"Graphics/Displays:\n"
"\n"
"    Intel Iris Pro:\n"
"\n"
"      Chipset Model: Intel Iris Pro\n"
"      Type: GPU\n"
"      Bus: Built-In\n"
"      VRAM (Dynamic, Max): 1536 MB\n"
"      Vendor: Intel (0x8086)\n"
"      Device ID: 0x0d26\n"
"      Revision ID: 0x0008\n"
"      Displays:\n"
"        Color LCD:\n"
"          Display Type: Retina LCD\n"
"          Resolution: 2880 x 1800 Retina\n"
"          Retina: Yes\n"
"          Pixel Depth: 32-Bit Color (ARGB8888)\n"
"          Main Display: Yes\n"
"          Mirror: Off\n"
"          Online: Yes\n"
"          Built-In: Yes\n";

static const char osx_dual_gpu[] =
"Intel HD Graphics 4000:\n"
"\n"
"     Chipset Model: Intel HD Graphics 4000\n"
"     Type: GPU\n"
"     Bus: Built-In\n"
"     VRAM (Total): 512 MB\n"
"     Vendor: Intel (0x8086)\n"
"     Device ID: 0x0166\n"
"     Revision ID: 0x0009\n"
"     gMux Version: 3.2.19 [3.2.8]\n"
"     Displays:\n"
"       Color LCD:\n"
"         Display Type: LCD\n"
"         Resolution: 2880 X 1800\n"
"         Retina: Yes\n"
"         Pixel Depth: 32-Bit Color (ARGB8888)\n"
"         Main Display: Yes\n"
"         Mirror: Off\n"
"         Online: Yes\n"
"         Built-In: Yes\n"
"         Connection Type: DisplayPort\n"
"\n"
"\n"
"   NVIDIA GeForce GT 650M:\n"
"\n"
"\n"
"     Chipset Model: NVIDIA GeForce GT 650M\n"
"     Type: GPU\n"
"     Bus: PCIe\n"
"     PCIe Lane Width: x8\n"
"     VRAM (Total): 1024 MB\n"
"     Vendor: NVIDIA (0x10de)\n"
"     Device ID: 0x0fd5\n"
"     Revision ID: 0x00a2\n"
"     ROM Revision: 3688\n"
"     gMux Version: 3.2.19 [3.2.8]\n";

static const char win_noinstalleddrivers[] = 
"\r\n"
"\r\n"
"AcceleratorCapabilities=\r\n"
"AdapterCompatibility=RealVNC\r\n"
"AdapterDACType=\r\n"
"AdapterRAM=\r\n"
"Availability=8\r\n"
"CapabilityDescriptions=\r\n"
"Caption=VNC Mirror Driver\r\n"
"ColorTableEntries=\r\n"
"ConfigManagerErrorCode=0\r\n"
"ConfigManagerUserConfig=FALSE\r\n"
"CreationClassName=Win32_VideoController\r\n"
"CurrentBitsPerPixel=\r\n"
"CurrentHorizontalResolution=\r\n"
"CurrentNumberOfColors=\r\n"
"CurrentNumberOfColumns=\r\n"
"CurrentNumberOfRows=\r\n"
"CurrentRefreshRate=\r\n"
"CurrentScanMode=\r\n"
"CurrentVerticalResolution=\r\n"
"Description=VNC Mirror Driver\r\n"
"DeviceID=VideoController1\r\n"
"DeviceSpecificPens=\r\n"
"DitherType=\r\n"
"DriverDate=20080314000000.000000-000\r\n"
"DriverVersion=1.8.0.0\r\n"
"ErrorCleared=\r\n"
"ErrorDescription=\r\n"
"ICMIntent=\r\n"
"ICMMethod=\r\n"
"InfFilename=oem24.inf\r\n"
"InfSection=vncmirror\r\n"
"InstallDate=\r\n"
"InstalledDisplayDrivers=\r\n"
"LastErrorCode=\r\n"
"MaxMemorySupported=\r\n"
"MaxNumberControlled=\r\n"
"MaxRefreshRate=\r\n"
"MinRefreshRate=\r\n"
"Monochrome=FALSE\r\n"
"Name=VNC Mirror Driver\r\n"
"NumberOfColorPlanes=\r\n"
"NumberOfVideoPages=\r\n"
"PNPDeviceID=ROOT\\DISPLAY\\0000\r\n"
"PowerManagementCapabilities=\r\n"
"PowerManagementSupported=\r\n"
"ProtocolSupported=\r\n"
"ReservedSystemPaletteEntries=\r\n"
"SpecificationVersion=\r\n"
"Status=Degraded\r\n"
"StatusInfo=\r\n"
"SystemCreationClassName=Win32_ComputerSystem\r\n"
"SystemName=ANDREW-PC\r\n"
"SystemPaletteEntries=\r\n"
"TimeOfLastReset=\r\n"
"VideoArchitecture=5\r\n"
"VideoMemoryType=2\r\n"
"VideoMode=\r\n"
"VideoModeDescription=\r\n"
"VideoProcessor=\r\n"
"\r\n"
"\r\n"
"AcceleratorCapabilities=\r\n"
"AdapterCompatibility=NVIDIA\r\n"
"AdapterDACType=Integrated RAMDAC\r\n"
"AdapterRAM=2147483648\r\n"
"Availability=3\r\n"
"CapabilityDescriptions=\r\n"
"Caption=NVIDIA GeForce GTX 660M \r\n"
"ColorTableEntries=\r\n"
"ConfigManagerErrorCode=0\r\n"
"ConfigManagerUserConfig=FALSE\r\n"
"CreationClassName=Win32_VideoController\r\n"
"CurrentBitsPerPixel=32\r\n"
"CurrentHorizontalResolution=1600\r\n"
"CurrentNumberOfColors=4294967296\r\n"
"CurrentNumberOfColumns=0\r\n"
"CurrentNumberOfRows=0\r\n"
"CurrentRefreshRate=60\r\n"
"CurrentScanMode=4\r\n"
"CurrentVerticalResolution=900\r\n"
"Description=NVIDIA GeForce GTX 660M \r\n"
"DeviceID=VideoController2\r\n"
"DeviceSpecificPens=\r\n"
"DitherType=0\r\n"
"DriverDate=20160321000000.000000-000\r\n"
"DriverVersion=10.18.13.6472\r\n"
"ErrorCleared=\r\n"
"ErrorDescription=\r\n"
"ICMIntent=\r\n"
"ICMMethod=\r\n"
"InfFilename=oem65.inf\r\n"
"InfSection=Section031\r\n"
"InstallDate=\r\n"
"InstalledDisplayDrivers=nvd3dumx.dll,nvwgf2umx.dll,nvwgf2umx.dll,nvwgf2umx.dll,nvd3dum,nvwgf2um,nvwgf2um,nvwgf2um\r\n"
"LastErrorCode=\r\n"
"MaxMemorySupported=\r\n"
"MaxNumberControlled=\r\n"
"MaxRefreshRate=60\r\n"
"MinRefreshRate=60\r\n"
"Monochrome=FALSE\r\n"
"Name=NVIDIA GeForce GTX 660M \r\n"
"NumberOfColorPlanes=\r\n"
"NumberOfVideoPages=\r\n"
"PNPDeviceID=PCI\\VEN_10DE&amp;DEV_0FD4&amp;SUBSYS_21151043&amp;REV_A1\\4&amp;169534F2&amp;0&amp;0008\r\n"
"PowerManagementCapabilities=\r\n"
"PowerManagementSupported=\r\n"
"ProtocolSupported=\r\n"
"ReservedSystemPaletteEntries=\r\n"
"SpecificationVersion=\r\n"
"Status=OK\r\n"
"StatusInfo=\r\n"
"SystemCreationClassName=Win32_ComputerSystem\r\n"
"SystemName=ANDREW-PC\r\n"
"SystemPaletteEntries=\r\n"
"TimeOfLastReset=\r\n"
"VideoArchitecture=5\r\n"
"VideoMemoryType=2\r\n"
"VideoMode=\r\n"
"VideoModeDescription=1600 x 900 x 4294967296 colors\r\n"
"VideoProcessor=GeForce GTX 660M\r\n";

static const char win_single_gpu[] = 
"\r\n"
"\r\n"
"AcceleratorCapabilities=\r\n"
"AdapterCompatibility=NVIDIA\r\n"
"AdapterDACType=Integrated RAMDAC\r\n"
"AdapterRAM=1073741824\r\n"
"Availability=3\r\n"
"CapabilityDescriptions=\r\n"
"Caption=NVIDIA Quadro K600\r\n"
"ColorTableEntries=\r\n"
"ConfigManagerErrorCode=0\r\n"
"ConfigManagerUserConfig=FALSE\r\n"
"CreationClassName=Win32_VideoController\r\n"
"CurrentBitsPerPixel=32\r\n"
"CurrentHorizontalResolution=2560\r\n"
"CurrentNumberOfColors=4294967296\r\n"
"CurrentNumberOfColumns=0\r\n"
"CurrentNumberOfRows=0\r\n"
"CurrentRefreshRate=59\r\n"
"CurrentScanMode=4\r\n"
"CurrentVerticalResolution=1600\r\n"
"Description=NVIDIA Quadro K600\r\n"
"DeviceID=VideoController1\r\n"
"DeviceSpecificPens=\r\n"
"DitherType=0\r\n"
"DriverDate=20130830000000.000000-000\r\n"
"DriverVersion=9.18.13.2086\r\n"
"ErrorCleared=\r\n"
"ErrorDescription=\r\n"
"ICMIntent=\r\n"
"ICMMethod=\r\n"
"InfFilename=oem39.inf\r\n"
"InfSection=Section078\r\n"
"InstallDate=\r\n"
"InstalledDisplayDrivers=nvd3dumx.dll,nvwgf2umx.dll,nvwgf2umx.dll,nvd3dum,nvwgf2um,nvwgf2um\r\n"
"LastErrorCode=\r\n"
"MaxMemorySupported=\r\n"
"MaxNumberControlled=\r\n"
"MaxRefreshRate=60\r\n"
"MinRefreshRate=50\r\n"
"Monochrome=FALSE\r\n"
"Name=NVIDIA Quadro K600\r\n"
"NumberOfColorPlanes=\r\n"
"NumberOfVideoPages=\r\n"
"PNPDeviceID=PCI\\VEN_10DE&amp;DEV_0FFA&amp;SUBSYS_094B103C&amp;REV_A1\\4&amp;16C9E9F9&amp;0&amp;0010\r\n"
"PowerManagementCapabilities=\r\n"
"PowerManagementSupported=\r\n"
"ProtocolSupported=\r\n"
"ReservedSystemPaletteEntries=\r\n"
"SpecificationVersion=\r\n"
"Status=OK\r\n"
"StatusInfo=\r\n"
"SystemCreationClassName=Win32_ComputerSystem\r\n"
"SystemPaletteEntries=\r\n"
"TimeOfLastReset=\r\n"
"VideoArchitecture=5\r\n"
"VideoMemoryType=2\r\n"
"VideoMode=\r\n"
"VideoModeDescription=2560 x 1600 x 4294967296 colors\r\n"
"VideoProcessor=Quadro K600\r\n";

static const char win_dual_gpu[] =
"AcceleratorCapabilities=\r\n"
"AdapterCompatibility=Advanced Micro Devices, Inc.\r\n"
"AdapterDACType=Internal DAC(400MHz)\r\n"
"AdapterRAM=4293918720\r\n"
"Availability=8\r\n"
"CapabilityDescriptions=\r\n"
"Caption=AMD Radeon (TM) R5 M335\r\n"
"ColorTableEntries=\r\n"
"ConfigManagerErrorCode=0\r\n"
"ConfigManagerUserConfig=FALSE\r\n"
"CreationClassName=Win32_VideoController\r\n"
"CurrentBitsPerPixel=\r\n"
"CurrentHorizontalResolution=\r\n"
"CurrentNumberOfColors=\r\n"
"CurrentNumberOfColumns=\r\n"
"CurrentNumberOfRows=\r\n"
"CurrentRefreshRate=\r\n"
"CurrentScanMode=\r\n"
"CurrentVerticalResolution=\r\n"
"Description=AMD Radeon (TM) R5 M335\r\n"
"DeviceID=VideoController1\r\n"
"DeviceSpecificPens=\r\n"
"DitherType=\r\n"
"DriverDate=20151204000000.000000-000\r\n"
"DriverVersion=15.300.1025.1001\r\n"
"ErrorCleared=\r\n"
"ErrorDescription=\r\n"
"ICMIntent=\r\n"
"ICMMethod=\r\n"
"InfFilename=oem27.inf\r\n"
"InfSection=ati2mtag_R503\r\n"
"InstallDate=\r\n"
"InstalledDisplayDrivers=aticfx64.dll,aticfx64.dll,aticfx64.dll,amdxc64.dll,aticfx32,aticfx32,aticfx32,amdxc32,atiumd64.dll,atidxx64.dll,atidxx64.dll,atiumdag,atidxx32,atidxx32,atiumdva,atiumd6a.cap,atitmm64.dll\r\n"
"LastErrorCode=\r\n"
"MaxMemorySupported=\r\n"
"MaxNumberControlled=\r\n"
"MaxRefreshRate=\r\n"
"MinRefreshRate=\r\n"
"Monochrome=FALSE\r\n"
"Name=AMD Radeon (TM) R5 M335\r\n"
"NumberOfColorPlanes=\r\n"
"NumberOfVideoPages=\r\n"
"PNPDeviceID=PCI\\VEN_1002&amp;DEV_6660&amp;SUBSYS_06B21028&amp;REV_81\\4&amp;2370267D&amp;0&amp;00E0\r\n"
"PowerManagementCapabilities=\r\n"
"PowerManagementSupported=\r\n"
"ProtocolSupported=\r\n"
"ReservedSystemPaletteEntries=\r\n"
"SpecificationVersion=\r\n"
"Status=OK\r\n"
"StatusInfo=\r\n"
"SystemCreationClassName=Win32_ComputerSystem\r\n"
"SystemName=DESKTOP-Q7BG33N\r\n"
"SystemPaletteEntries=\r\n"
"TimeOfLastReset=\r\n"
"VideoArchitecture=5\r\n"
"VideoMemoryType=2\r\n"
"VideoMode=\r\n"
"VideoModeDescription=\r\n"
"VideoProcessor=AMD Radeon Graphics Processor (0x6660)\r\n"
"\r\n"
"\r\n"
"AcceleratorCapabilities=\r\n"
"AdapterCompatibility=Intel Corporation\r\n"
"AdapterDACType=Internal\r\n"
"AdapterRAM=1073741824\r\n"
"Availability=3\r\n"
"CapabilityDescriptions=\r\n"
"Caption=Intel(R) HD Graphics 520\r\n"
"ColorTableEntries=\r\n"
"ConfigManagerErrorCode=0\r\n"
"ConfigManagerUserConfig=FALSE\r\n"
"CreationClassName=Win32_VideoController\r\n"
"CurrentBitsPerPixel=32\r\n"
"CurrentHorizontalResolution=1920\r\n"
"CurrentNumberOfColors=4294967296\r\n"
"CurrentNumberOfColumns=0\r\n"
"CurrentNumberOfRows=0\r\n"
"CurrentRefreshRate=59\r\n"
"CurrentScanMode=4\r\n"
"CurrentVerticalResolution=1080\r\n"
"Description=Intel(R) HD Graphics 520\r\n"
"DeviceID=VideoController2\r\n"
"DeviceSpecificPens=\r\n"
"DitherType=0\r\n"
"DriverDate=20150831000000.000000-000\r\n"
"DriverVersion=10.18.15.4281\r\n"
"ErrorCleared=\r\n"
"ErrorDescription=\r\n"
"ICMIntent=\r\n"
"ICMMethod=\r\n"
"InfFilename=oem6.inf\r\n"
"InfSection=iSKLD_w10\r\n"
"InstallDate=\r\n"
"InstalledDisplayDrivers=igdumdim64.dll,igd10iumd64.dll,igd10iumd64.dll,igd12umd64.dll,igdumdim32,igd10iumd32,igd10iumd32,igd12umd32\r\n"
"LastErrorCode=\r\n"
"MaxMemorySupported=\r\n"
"MaxNumberControlled=\r\n"
"MaxRefreshRate=59\r\n"
"MinRefreshRate=48\r\n"
"Monochrome=FALSE\r\n"
"Name=Intel(R) HD Graphics 520\r\n"
"NumberOfColorPlanes=\r\n"
"NumberOfVideoPages=\r\n"
"PNPDeviceID=PCI\\VEN_8086&amp;DEV_1916&amp;SUBSYS_06B21028&amp;REV_07\\3&amp;11583659&amp;0&amp;10\r\n"
"PowerManagementCapabilities=\r\n"
"PowerManagementSupported=\r\n"
"ProtocolSupported=\r\n"
"ReservedSystemPaletteEntries=\r\n"
"SpecificationVersion=\r\n"
"Status=OK\r\n"
"StatusInfo=\r\n"
"SystemCreationClassName=Win32_ComputerSystem\r\n"
"SystemPaletteEntries=\r\n"
"TimeOfLastReset=\r\n"
"VideoArchitecture=5\r\n"
"VideoMemoryType=2\r\n"
"VideoMode=\r\n"
"VideoModeDescription=1920 x 1080 x 4294967296 colors\r\n"
"VideoProcessor=Intel(R) HD Graphics Family\r\n";

static const char linux_single_gpu[] =
"Rev: 05\n"
"\n"
"Device: 04:00.0\n"
"Class:  VGA compatible controller [0300]\n"
"Vendor: NVIDIA Corporation [10de]\n"
"Device: GM107GL [Quadro K2200] [13ba]\n"
"SVendor:    Hewlett-Packard Company [103c]\n"
"SDevice:    Device [1097]\n"
"PhySlot:    2\n"
"Rev:    a2\n"
"\n"
"Device: 04:00.1\n"
"\n"
"\n"
"\n"
"\n"
"OpenGL version string: 4.4.0 NVIDIA 340.96\n"
"OpenGL shading language version string: 4.40 NVIDIA via Cg compiler\n"
"OpenGL context flags: (none)\n"
"\n";

static const char linux_mesadri[] =
"Rev: 05\n"
"\n"
"Device: 04:00.0\n"
"Class:  VGA compatible controller [0300]\n"
"Vendor: NVIDIA Corporation [10de]\n"
"Device: GM107GL [Quadro K2200] [13ba]\n"
"SVendor:    Hewlett-Packard Company [103c]\n"
"SDevice:    Device [1097]\n"
"PhySlot:    2\n"
"Rev:    a2\n"
"\n"
"Device: 04:00.1\n"
"\n"
"\n"
"\n"
"\n"
"OpenGL version string: 3.0 Mesa 10.4.2 (git-)\n"
"\n";

static const char linux_no_glxinfo[] =
"Rev: 05\n"
"\n"
"Device: 04:00.0\n"
"Class:  VGA compatible controller [0300]\n"
"Vendor: NVIDIA Corporation [10de]\n"
"Device: GM107GL [Quadro K2200] [13ba]\n"
"SVendor:    Hewlett-Packard Company [103c]\n"
"SDevice:    Device [1097]\n"
"PhySlot:    2\n"
"Rev:    a2\n"
"\n"
"Device: 04:00.1\n"
"\n"
"\n"
"\n"
"\n"
"\n";

static const char linux_no_lspci[] =
"\n"
"\n"
"OpenGL version string: 3.0 Mesa 10.4.2 (git-)\n"
"\n";

TEST(parse_gpu_info_list_osx, SingleGpu) {
    std::string contents(osx_single_gpu);

    GpuInfoList gpulist;
    parse_gpu_info_list_osx(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);

    GpuInfo info = gpulist.currGpu();

    EXPECT_TRUE(info.current_gpu);
    EXPECT_STREQ("Intel (0x8086)", info.make.c_str());
    EXPECT_STREQ("Intel Iris Pro", info.model.c_str());
    EXPECT_STREQ("0x0d26", info.device_id.c_str());
    EXPECT_STREQ("0x0008", info.revision_id.c_str());
    EXPECT_TRUE(info.version.empty());
    EXPECT_TRUE(info.renderer.empty());
    EXPECT_TRUE(info.dlls.empty());
}

TEST(parse_gpu_info_list_osx, DualGpu) {
    std::string contents(osx_dual_gpu);

    GpuInfoList gpulist;
    parse_gpu_info_list_osx(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 2);

    GpuInfo& intel_info = gpulist.infos[0];
    EXPECT_TRUE(intel_info.current_gpu);
    EXPECT_STREQ("Intel (0x8086)", intel_info.make.c_str());
    EXPECT_STREQ("Intel HD Graphics 4000", intel_info.model.c_str());
    EXPECT_STREQ("0x0166", intel_info.device_id.c_str());
    EXPECT_STREQ("0x0009", intel_info.revision_id.c_str());
    EXPECT_TRUE(intel_info.version.empty());
    EXPECT_TRUE(intel_info.renderer.empty());
    EXPECT_TRUE(intel_info.dlls.empty());

    GpuInfo& nvidia_info = gpulist.infos[1];
    EXPECT_FALSE(nvidia_info.current_gpu);
    EXPECT_STREQ("NVIDIA (0x10de)", nvidia_info.make.c_str());
    EXPECT_STREQ("NVIDIA GeForce GT 650M", nvidia_info.model.c_str());
    EXPECT_STREQ("0x0fd5", nvidia_info.device_id.c_str());
    EXPECT_STREQ("0x00a2", nvidia_info.revision_id.c_str());
    EXPECT_TRUE(nvidia_info.version.empty());
    EXPECT_TRUE(nvidia_info.renderer.empty());
    EXPECT_TRUE(nvidia_info.dlls.empty());
}

TEST(parse_gpu_info_list_windows, WinNoInstalledDriversContinueCase) {
    std::string contents(win_noinstalleddrivers);

    GpuInfoList gpulist;
    parse_gpu_info_list_windows(contents, &gpulist);

    EXPECT_EQ(2, (int)gpulist.infos.size());
}

TEST(parse_gpu_info_list_windows, SingleGpu) {
    std::string contents(win_single_gpu);

    GpuInfoList gpulist;
    parse_gpu_info_list_windows(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);

    GpuInfo& nvidia_info = gpulist.infos[0];

    EXPECT_FALSE(nvidia_info.current_gpu);
    EXPECT_STREQ("10de", nvidia_info.make.c_str());
    EXPECT_STREQ("NVIDIA Quadro K600", nvidia_info.model.c_str());
    EXPECT_STREQ("0ffa", nvidia_info.device_id.c_str());
    EXPECT_TRUE(nvidia_info.revision_id.empty());
    EXPECT_STREQ("9.18.13.2086", nvidia_info.version.c_str());
    EXPECT_TRUE(nvidia_info.renderer.empty());

    EXPECT_TRUE(nvidia_info.dlls.size() == 8);
    EXPECT_STREQ("nvd3dumx.dll", nvidia_info.dlls[0].c_str());
    EXPECT_STREQ("nvwgf2umx.dll", nvidia_info.dlls[1].c_str());
    EXPECT_STREQ("nvwgf2umx.dll", nvidia_info.dlls[2].c_str());
    EXPECT_STREQ("nvd3dum", nvidia_info.dlls[3].c_str());
    EXPECT_STREQ("nvwgf2um", nvidia_info.dlls[4].c_str());
    EXPECT_STREQ("nvwgf2um", nvidia_info.dlls[5].c_str());
    EXPECT_STREQ("nvoglv32.dll", nvidia_info.dlls[6].c_str());
    EXPECT_STREQ("nvoglv64.dll", nvidia_info.dlls[7].c_str());
}

TEST(parse_gpu_info_list_windows, DualGpu) {
    std::string contents(win_dual_gpu);

    GpuInfoList gpulist;
    parse_gpu_info_list_windows(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 2);

    GpuInfo& ati_info = gpulist.infos[0];
    EXPECT_FALSE(ati_info.current_gpu);
    EXPECT_STREQ("1002", ati_info.make.c_str());
    EXPECT_STREQ("AMD Radeon (TM) R5 M335", ati_info.model.c_str());
    EXPECT_STREQ("6660", ati_info.device_id.c_str());
    EXPECT_TRUE(ati_info.revision_id.empty());
    EXPECT_STREQ("15.300.1025.1001", ati_info.version.c_str());
    EXPECT_TRUE(ati_info.renderer.empty());

    EXPECT_TRUE(ati_info.dlls.size() == 19);
    EXPECT_STREQ("aticfx64.dll", ati_info.dlls[0].c_str());
    EXPECT_STREQ("aticfx64.dll", ati_info.dlls[1].c_str());
    EXPECT_STREQ("aticfx64.dll", ati_info.dlls[2].c_str());
    EXPECT_STREQ("amdxc64.dll", ati_info.dlls[3].c_str());
    EXPECT_STREQ("aticfx32", ati_info.dlls[4].c_str());
    EXPECT_STREQ("aticfx32", ati_info.dlls[5].c_str());
    EXPECT_STREQ("aticfx32", ati_info.dlls[6].c_str());
    EXPECT_STREQ("amdxc32", ati_info.dlls[7].c_str());
    EXPECT_STREQ("atiumd64.dll", ati_info.dlls[8].c_str());
    EXPECT_STREQ("atidxx64.dll", ati_info.dlls[9].c_str());
    EXPECT_STREQ("atidxx64.dll", ati_info.dlls[10].c_str());
    EXPECT_STREQ("atiumdag", ati_info.dlls[11].c_str());
    EXPECT_STREQ("atidxx32", ati_info.dlls[12].c_str());
    EXPECT_STREQ("atidxx32", ati_info.dlls[13].c_str());
    EXPECT_STREQ("atiumdva", ati_info.dlls[14].c_str());
    EXPECT_STREQ("atiumd6a.cap", ati_info.dlls[15].c_str());
    EXPECT_STREQ("atitmm64.dll", ati_info.dlls[16].c_str());
    EXPECT_STREQ("atioglxx.dll", ati_info.dlls[17].c_str());
    EXPECT_STREQ("atig6txx.dll", ati_info.dlls[18].c_str());

    GpuInfo& intel_info = gpulist.infos[1];
    EXPECT_FALSE(intel_info.current_gpu);
    EXPECT_STREQ("8086", intel_info.make.c_str());
    EXPECT_STREQ("Intel(R) HD Graphics 520", intel_info.model.c_str());
    EXPECT_STREQ("1916", intel_info.device_id.c_str());
    EXPECT_TRUE(intel_info.revision_id.empty());
    EXPECT_STREQ("10.18.15.4281", intel_info.version.c_str());
    EXPECT_TRUE(intel_info.renderer.empty());

    EXPECT_TRUE(intel_info.dlls.size() == 8);
    EXPECT_STREQ("igdumdim64.dll", intel_info.dlls[0].c_str());
    EXPECT_STREQ("igd10iumd64.dll", intel_info.dlls[1].c_str());
    EXPECT_STREQ("igd10iumd64.dll", intel_info.dlls[2].c_str());
    EXPECT_STREQ("igd12umd64.dll", intel_info.dlls[3].c_str());
    EXPECT_STREQ("igdumdim32", intel_info.dlls[4].c_str());
    EXPECT_STREQ("igd10iumd32", intel_info.dlls[5].c_str());
    EXPECT_STREQ("igd10iumd32", intel_info.dlls[6].c_str());
    EXPECT_STREQ("igd12umd32", intel_info.dlls[7].c_str());
}

TEST(parse_gpu_info_list_linux, EmptyStr) {
    std::string contents;

    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 0);
}

TEST(parse_gpu_info_list_linux, NoGlxInfo) {
    std::string contents(linux_no_glxinfo);

    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);
}

TEST(parse_gpu_info_list_linux, Nolspci) {
    std::string contents(linux_no_lspci);

    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);
}

TEST(parse_gpu_info_list_linux, SingleGpu) {
    std::string contents(linux_single_gpu);

    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);
    GpuInfo& nvidia_info = gpulist.infos[0];
    EXPECT_FALSE(nvidia_info.current_gpu);
    EXPECT_STREQ("10de", nvidia_info.make.c_str());
    EXPECT_TRUE(nvidia_info.model.empty());
    EXPECT_STREQ("13ba", nvidia_info.device_id.c_str());
    EXPECT_TRUE(nvidia_info.revision_id.empty());
    EXPECT_TRUE(nvidia_info.version.empty());
    EXPECT_STREQ("OpenGL version string: 4.4.0 NVIDIA 340.96",
            nvidia_info.renderer.c_str());
    EXPECT_TRUE(nvidia_info.dlls.empty());
}

TEST(parse_gpu_info_list_linux, MesaDRI) {
    std::string contents(linux_mesadri);

    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);

    EXPECT_TRUE(gpulist.infos.size() == 1);
    GpuInfo& nvidia_info = gpulist.infos[0];
    EXPECT_FALSE(nvidia_info.current_gpu);
    EXPECT_STREQ("10de", nvidia_info.make.c_str());
    EXPECT_TRUE(nvidia_info.model.empty());
    EXPECT_STREQ("13ba", nvidia_info.device_id.c_str());
    EXPECT_TRUE(nvidia_info.revision_id.empty());
    EXPECT_TRUE(nvidia_info.version.empty());
    EXPECT_STREQ("OpenGL version string: 3.0 Mesa 10.4.2 (git-)",
            nvidia_info.renderer.c_str());
    EXPECT_TRUE(nvidia_info.dlls.empty());
}

TEST(gpuinfo_query_blacklist, testBlacklist_Pos) {
    const BlacklistEntry test_list[] = {
        {"10de", nullptr, "13ba", nullptr, nullptr, nullptr, nullptr}
    };

    int test_list_len = sizeof(test_list) / sizeof(BlacklistEntry);

    std::string contents(linux_single_gpu);
    GpuInfoList gpulist;
    parse_gpu_info_list_linux(contents, &gpulist);
    bool on_blacklist = gpuinfo_query_blacklist(&gpulist,
                                                test_list,
                                                test_list_len);

    EXPECT_TRUE(on_blacklist);
}

TEST(gpuinfo_query_blacklist, testBlacklist_Neg) {
    const BlacklistEntry test_list[] = {
        {"10de", nullptr, "13ba", nullptr, nullptr, nullptr, nullptr},
        {"NVIDIA", "NVIDIA Quadro K600", nullptr, nullptr, nullptr, nullptr, nullptr},
        {"ASDF", "Intel Iris Pro", nullptr, nullptr, nullptr, nullptr, nullptr}
    };

    int test_list_len = sizeof(test_list) / sizeof(BlacklistEntry);

    std::string contents(osx_single_gpu);
    GpuInfoList gpulist;
    parse_gpu_info_list_osx(contents, &gpulist);
    bool on_blacklist = gpuinfo_query_blacklist(&gpulist,
                                                test_list,
                                                test_list_len);

    EXPECT_FALSE(on_blacklist);
}
