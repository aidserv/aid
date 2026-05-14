# aid2: 总览
Authorize ios device 实现功能和爱思助手，鲨漏验机，3utools一样安装app，使用aid2 授权之后 ，安装的IPA文件不需要输入Apple账户和密码。

V2.6 grpc编译的复杂度，改用了http接口调用，方便编译。

需要在linux,mac,安卓或者在树莓派等各类开发版上运行的商业应用请联系邮箱：2277695881@qq.com      qq:2277695881
# 目录

- [aid2: 总览](#aid2-总览)
- [目录](#目录)
- [配置编译环境](#配置编译环境)
  - [快速开始: Windows](#快速开始-windows)
- [编译](#编译)
  - [下载源码](#下载源码)
  - [使用Visual Studio 2022编译](#使用visual-studio-2022编译)
- [运行](#运行)
  - [调试](#调试)
  - [命令行运行](#命令行运行)
# 配置编译环境

首先，请阅读以下任一快速配置指南：
[Windows](#快速开始-windows)，


windows 平台支持windows 10之后版本，itunes 请安装本页提供连接的itunes版本，目前暂不支持从Microsoft Store安装itunes 版本。

编译环境需要用到 Vcpkg，openssl，nlohmann-json


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

安装itunes版本，普通安装包下载连接
* [下载 Windows 64位版 iTunes 12.13.9](https://secure-appldnld.apple.com/itunes12/042-62516-20231023-4B775F51-D1D0-4728-A168-77A5EFB3D51D/iTunes64Setup.exe)
* [下载 Windows 32位版 iTunes 12.13.9](https://secure-appldnld.apple.com/itunes12/042-62514-20231023-50B51FD0-68B9-4F27-989D-B226D7A42BEC/iTunesSetup.exe)
* [下载 Windows 64位版 iTunes 12.6.5.3（最后一个可下载ipa版本）](https://secure-appldnld.apple.com/itunes12/091-87819-20180912-69177170-B085-11E8-B6AB-C1D03409AD2A6/iTunes64Setup.exe)
* [下载 MacOS 64位版 iTunes 12.6.5.3（最后一个可下载ipa版本）](https://secure-appldnld.apple.com/itunes12/091-87821-20180912-69177170-B085-11E8-B6AB-C1D03409AD2A/iTunes12.6.5.dmg)


# 编译

## 下载源码

```cmd
> git clone --recursive https://github.com/aidserv/aid.git
```

## 使用Visual Studio 2022编译
使用 Visual Studio 2022 打开 aid2.sln 文件，平台选择x64,然后编译整个解决方案

# 运行
## 调试
    examples目录下面cpp项目为启动项目，就可以进入自动调试状态。当前项目安装设置的为h1.0.25.ipa，请更改你自己想要安装包文件名。

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
/******************************************************
* 执行配对信息，首次配对手机上会出现信任和不信任按钮
返回值：LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           弹出对话框,请在设备端按下“信任”按钮
		LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			使用者按下了“不信任”按钮。
		LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			请打开密码锁定，进入ios主界面
		LOCKDOWN_E_MUX_ERROR                               =  -8			USB数据线断开或者接触不良
															   0			配对成功
***************************************************************/
int DoPair(const char* udid);

/*******************************************************
根据udid授权
参数：udid
返回值：成功为AuthorizeReturnStatus::AuthorizeSuccess
*******************************************************/
AuthorizeReturnStatus AuthorizeDevice(const char * udid);

/*******************************************************
根据udid安装path 的ipa 包
参数：udid
      ipaPath ipa包的路径
返回值：成功为true
*******************************************************/
bool InstallApplication(const char* udid,const char* ipaPath);

```


