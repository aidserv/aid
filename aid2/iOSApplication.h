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
		// ��װipa�ص�����ָ��
		static InstallApplicationFunc InstallCallback;
	private:
		AMDeviceRef m_deviceHandle = nullptr;

		// ��װӦ�ûص�������
		static void install_callback(CFDictionaryRef dict, int arg);
		// copy ipa �ļ����豸�ص�������
		static void transfer_callback(CFDictionaryRef dict, int arg);
	};

}