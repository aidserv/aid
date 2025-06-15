# aid2: 总览
Authorize ios device 实现功能和爱思助手一样安装app，使用aid2 授权之后 ，安装的IPA文件不需要输入appid和密码，aid第二版本，可以支持windows和mac系统，目前mac系统还没有调通。

V2.1 版本在之前2.0功能上增加了安装ipa 功能。

V2.5 重新梳理了授权执行步骤，每台手机采用单线程对手机执行授权过程，降低了代码复杂度，提升了执行授权速度。

V2.6 grpc编译的复杂度，改用了http接口调用，方便编译。

需要在linux,mac,安卓或者商业应用的联系邮箱：2277695881@qq.com      qq:2277695881
# 目录

- [aid2: 总览](#aid2-总览)
- [目录](#目录)
- [入门](#入门)
  - [快速开始: Windows](#快速开始-windows)
  - [快速开始: Unix](#快速开始-unix)
  - [安装 macOS Developer Tools](#安装-macos-developer-tools)
- [编译](#编译)
  - [下载源码](#下载源码)
  - [使用Visual Studio 2022编译](#使用visual-studio-2022编译)
- [运行](#运行)
  - [调试](#调试)
  - [命令行运行](#命令行运行)
# 入门

首先，请阅读以下任一快速入门指南：
[Windows](#快速开始-windows) 或 [macOS ](#快速开始-unix)，
这取决于您使用的是什么平台。

windows 平台支持windows 10之后版本，itunes 安装最新版本或者说安装 itunes 最新版本里的AppleMobileDeviceSupport包，只做过x64平台做过大量测试，编写代码的时候也考虑到x86平台，但没有大量测试，请使用者自行测试或者拉起pull

编译环境需要用到 Vcpkg，grpc,protobuf,openssl

以下配置环境都需要用到科学上网，否则会失败。

## 快速开始: Windows

前置条件:
- Windows 10 或更新的版本
- [Git][getting-started:git]
- [Visual Studio][getting-started:visual-studio] 2022  或更新的版本（**包含英文语言包**）

首先，**请使用 `git clone vcpkg`** 并执行 bootstrap.bat 脚本。
您可以将 vcpkg 安装在任何地方，但是通常我们建议您使用 vcpkg 作为 CMake 项目的子模块，并将其全局安装到 Visual Studio 项目中。
我们建议您使用例如 `C:\src\vcpkg` 或 `C:\dev\vcpkg` 的安装目录，否则您可能遇到某些库构建系统的路径问题。

```cmd
> git clone https://github.com/microsoft/vcpkg
> .\vcpkg\bootstrap-vcpkg.bat
> .\vcpkg\vcpkg install openssl:x64-windows-static nlohmann-json:x64-windows-static
> .\vcpkg\vcpkg integrate install
```

安装itunes版本，windows 10下载连接
* [下载 Windows 10（64 位）版 iTunes 12.13.9](https://secure-appldnld.apple.com/itunes12/042-62516-20231023-4B775F51-D1D0-4728-A168-77A5EFB3D51D/iTunes64Setup.exe)
* [下载 Windows 10（32 位）版 iTunes 12.13.9](https://secure-appldnld.apple.com/itunes12/042-62514-20231023-50B51FD0-68B9-4F27-989D-B226D7A42BEC/iTunesSetup.exe)

## 快速开始: Unix

macOS 平台前置条件:
- [Apple Developer Tools][getting-started:macos-dev-tools]

首先，请下载 vcpkg 并执行 bootstrap.sh 脚本。
您可以将 vcpkg 安装在任何地方，但是通常我们建议您使用 vcpkg 作为 CMake 项目的子模块。

```sh
$ git clone https://github.com/microsoft/vcpkg
$ ./vcpkg/bootstrap-vcpkg.sh
$ ./vcpkg/vcpkg install grpc protobuf 
$ ./vcpkg/vcpkg integrate install
```

## 安装 macOS Developer Tools

在 macOS 中，您唯一需要做的是在终端中运行以下命令:

```sh
$ xcode-select --install
```

# 编译

## 下载源码

```cmd
> git clone --recursive https://github.com/aidserv/aid.git

```

## 使用Visual Studio 2022编译
使用 Visual Studio 2022 打开 aid2.sln 文件，平台选择x64,然后编译整个解决方案

然后到 https://github.com/aidserv/aid/releases 下载编译后版本，解压出aid2.zip压缩包,把certificate文件夹复制到编译输出目录下面就可以。


# 运行
## 调试
    examples目录下面cpp项目为启动项目，就可以进入自动调试状态。

## 命令行运行
   进入编译输出目录，x64\Debug或者x64\Release目录，安装app, 以下命令是安装换机助手，chatgpt，同步助手，由于自定app市场需要大量人力物力，如果需要安装指定的app,请联系上面邮箱或者QQ
```cmd
> cpp h1.0.25.ipa
> cpp ChatGPT 1.2023.21 1.ipa
> cpp tonbu11.1.0.ipa
```

运行以上命令输出：设备名:iPhone 授权成功 就自动授权成功，最后ipa安装成功会弹出安装成功界面，以上例子是每次安装app自动授权，如果需要一次性授权，请把授权和安装代码分开，目前提供两个外部dll函数

以上命令安装好，直接在手机屏幕上找到三个app 打开，检测是否需要输入apple id和密码，手机授权之后，需要在次测试，只能把把手机还原到初厂设置，具体操作是【设置】，【通用】，【传输或还原iPhone】，【抹掉所有内容和设置】。

# 主要函数
```

/*******************************************************
根据udid授权
参数：udid
返回值：成功为true
*******************************************************/
bool AuthorizeDevice(const char * udid);

/*******************************************************
根据udid安装path 的ipa 包
参数：udid
      ipaPath ipa包的路径
返回值：成功为true
*******************************************************/
bool InstallApplication(const char* udid,const char* ipaPath);

```


