# aid2: Overview

aid2 authorizes iOS devices and installs apps in a way similar to AiSi Assistant, ShaLou device inspector, and 3uTools. After a device has been authorized with aid2, installing an IPA file does not require an Apple account or password.

In V2.6, gRPC was replaced with HTTP APIs to simplify compilation and reduce build complexity.

For commercial applications that need to run on Linux, macOS, Android, Raspberry Pi, and other development boards, please contact:

- Email: 2277695881@qq.com
- QQ: 2277695881

## Table of Contents

- [aid2: Overview](#aid2-overview)
- [Table of Contents](#table-of-contents)
- [Setting Up the Build Environment](#setting-up-the-build-environment)
  - [Quick Start: Windows](#quick-start-windows)
- [Build](#build)
  - [Download the Source Code](#download-the-source-code)
  - [Build with Visual Studio 2022](#build-with-visual-studio-2022)
- [Run](#run)
  - [Debugging](#debugging)
  - [Command-Line Usage](#command-line-usage)

# Setting Up the Build Environment

First, follow the quick setup guide below:

[Windows](#quick-start-windows)

aid2 supports Windows 10 and later. Install the version of iTunes linked on this page. Versions installed from the Microsoft Store are not currently supported.

The build environment requires Vcpkg, OpenSSL, and nlohmann-json.

## Quick Start: Windows

Prerequisites:

- Windows 10 or later
- [Git](https://git-scm.com/downloads)
- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) or later (**with the English language pack**)

First, **clone Vcpkg with `git clone`** and run the `bootstrap.bat` script.
You can install Vcpkg in any location. However, we generally recommend using Vcpkg as a submodule of a CMake project and integrating it globally with Visual Studio projects.
We recommend an installation directory such as `C:\src\vcpkg` or `C:\dev\vcpkg`; other locations may cause path-related issues when building some libraries.

```cmd
> git clone https://github.com/microsoft/vcpkg
> .\vcpkg\bootstrap-vcpkg.bat
> .\vcpkg\vcpkg install openssl:x64-windows-static nlohmann-json:x64-windows-static
> .\vcpkg\vcpkg integrate install
```

Download links for standard iTunes installers:

- [Download iTunes 12.13.9 for 64-bit Windows](https://secure-appldnld.apple.com/itunes12/042-62516-20231023-4B775F51-D1D0-4728-A168-77A5EFB3D51D/iTunes64Setup.exe)
- [Download iTunes 12.13.9 for 32-bit Windows](https://secure-appldnld.apple.com/itunes12/042-62514-20231023-50B51FD0-68B9-4F27-989D-B226D7A42BEC/iTunesSetup.exe)
- [Download iTunes 12.6.5.3 for 64-bit Windows (the last version that can download IPA files)](https://secure-appldnld.apple.com/itunes12/091-87819-20180912-69177170-B085-11E8-B6AB-C1D03409AD2A6/iTunes64Setup.exe)
- [Download iTunes 12.6.5.3 for 64-bit macOS (the last version that can download IPA files)](https://secure-appldnld.apple.com/itunes12/091-87821-20180912-69177170-B085-11E8-B6AB-C1D03409AD2A6/iTunes12.6.5.dmg)

# Build

## Download the Source Code

```cmd
> git clone --recursive https://github.com/aidserv/aid.git
```

## Build with Visual Studio 2022

Open `aid2.sln` in Visual Studio 2022, select `x64` as the platform, and build the entire solution.

# Run

## Debugging

Set the C++ project under the `examples` directory as the startup project to start debugging automatically. The current project is configured to install `h1.0.25.ipa`; change this to the filename of the package you want to install.

## Command-Line Usage

Go to the build output directory, either `x64\Debug` or `x64\Release`, to install apps. The following commands install Phone Transfer Assistant, ChatGPT, and Sync Assistant. Building a custom app marketplace requires substantial resources; if you need to install a specific app, please contact us using the email address or QQ number above.

```cmd
> cpp h1.0.25.ipa
> cpp ChatGPT 1.2023.21 1.ipa
> cpp tonbu11.1.0.ipa
```

The commands above report `Device name: iPhone` and `Authorization succeeded` when authorization succeeds, then show an installation-success dialog after the IPA is installed. The examples authorize the device for each installation. To authorize only once, separate the authorization and installation code. Two external DLL functions are currently provided.

After the installation is complete, open the three apps on the phone and check whether they prompt for an Apple ID and password. To retest authorization, restore the phone to factory settings. To do this, go to **Settings** > **General** > **Transfer or Reset iPhone** > **Erase All Content and Settings**.

# Main Functions

```cpp
/******************************************************
* Perform pairing. On first-time pairing, the phone displays Trust and Don't Trust buttons.
* Return values:
*   LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING = -19  A dialog is displayed; press "Trust" on the device.
*   LOCKDOWN_E_USER_DENIED_PAIRING            = -18  The user pressed "Don't Trust".
*   LOCKDOWN_E_PASSWORD_PROTECTED             = -17  Unlock the device with its passcode and open the iOS home screen.
*   LOCKDOWN_E_MUX_ERROR                      =  -8  The USB cable is disconnected or has a poor connection.
*                                              0      Pairing succeeded.
***************************************************************/
int DoPair(const char* udid);

/*******************************************************
* Authorize a device by UDID.
* Parameter: udid
* Return value: AuthorizeReturnStatus::AuthorizeSuccess on success.
*******************************************************/
AuthorizeReturnStatus AuthorizeDevice(const char* udid);

/*******************************************************
* Install the IPA package at ipaPath on the device with the given UDID.
* Parameters:
*   udid    The device UDID.
*   ipaPath The path to the IPA package.
* Return value: true on success.
*******************************************************/
bool InstallApplication(const char* udid, const char* ipaPath);
```
