#pragma once
#include "iTunesApi/simpleApi.h"
#include <aid2/aid2.h>
#include<string>
namespace aid2 {
	using namespace std;
	class iOSApplication
	{
	public:
		iOSApplication(AMDeviceRef deviceHandle);
		bool Install(const string path);
		// 安装ipa回调函数指针
		static InstallApplicationFunc InstallCallback;
	private:
		AMDeviceRef m_deviceHandle = nullptr;

		// 安装应用回调处理函数
		static void install_callback(CFDictionaryRef dict, int arg);
		// copy ipa 文件到设备回调处理函数
		static void transfer_callback(CFDictionaryRef dict, int arg);
	};

}