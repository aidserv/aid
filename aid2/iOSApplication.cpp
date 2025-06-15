#include "iOSApplication.h"
#include "Logger.h"
namespace aid2 {
	InstallApplicationFunc iOSApplication::InstallCallback = nullptr;

	void iOSApplication::transfer_callback(CFDictionaryRef dict, int arg) {
		int percent;
		CFStringRef status = CFDictionaryGetValue(dict, CFStringCreateWithCString(NULL, "Status", kCFStringEncodingUTF8));
		CFNumberGetValue(CFDictionaryGetValue(dict, CFStringCreateWithCString(NULL, "PercentComplete", kCFStringEncodingUTF8)), kCFNumberSInt32Type, &percent);
		auto len = CFStringGetLength(status);
		string strStatus;
		strStatus.resize(len);
		CFStringGetCString(status, (char*)strStatus.c_str(), len + 1, kCFStringEncodingUTF8);
		if (percent != 100) {
			if (InstallCallback) {
				InstallCallback(strStatus.c_str(), percent);
			}
		}
	}

	void iOSApplication::install_callback(CFDictionaryRef dict, int arg) {
		int percent;
		CFStringRef status = CFDictionaryGetValue(dict, CFStringCreateWithCString(NULL, "Status", kCFStringEncodingUTF8));
		CFNumberGetValue(CFDictionaryGetValue(dict, CFStringCreateWithCString(NULL, "PercentComplete", kCFStringEncodingUTF8)), kCFNumberSInt32Type, &percent);
		auto len = CFStringGetLength(status);
		string strStatus;
		strStatus.resize(len);
		CFStringGetCString(status, (char*)strStatus.c_str(), len + 1, kCFStringEncodingUTF8);
		if (InstallCallback) {
			InstallCallback(strStatus.c_str(), (percent <= 5 ? percent : (percent / 2)) + 50);
		}
	}


	iOSApplication::iOSApplication(AMDeviceRef deviceHandle)
	{
		m_deviceHandle = deviceHandle;
	}

	bool iOSApplication::Install(const string path)
	{
		void* serviceHandle;
		bool ret = true;
		AMDeviceConnect(m_deviceHandle);
		AMDeviceStartSession(m_deviceHandle);
		CFStringRef sServiceName = CFStringCreateWithCString(NULL, "com.apple.afc", kCFStringEncodingUTF8);
		if (AMDeviceStartService(m_deviceHandle, sServiceName, &serviceHandle, NULL) != 0) {
			ret = false;
		}
		CFRelease(sServiceName);
#ifdef WIN32
		CFStringRef app_path = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingGB_18030_2000);
#else
		CFStringRef app_path = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingUTF8);
#endif
		if (AMDeviceTransferApplication(serviceHandle, app_path, NULL, transfer_callback, NULL) != 0)
		{
			logger.log("AMDeviceTransferApplication ·µ»ØÊ§°Ü¡£");
			ret = false;
		}
		sServiceName = CFStringCreateWithCString(NULL, "com.apple.mobile.installation_proxy", kCFStringEncodingUTF8);
		if (AMDeviceStartService(m_deviceHandle, sServiceName, &serviceHandle, NULL) != 0) {
			ret = false;
		}

		CFStringRef keys[] = {
			CFStringCreateWithCString(NULL, "PackageType", kCFStringEncodingUTF8)
		};
		CFStringRef values[] = {
			CFStringCreateWithCString(NULL, "Developer", kCFStringEncodingUTF8)
		};
		CFDictionaryRef options = CFDictionaryCreate(NULL, keys, values, 1, NULL, NULL);
		if (AMDeviceInstallApplication(serviceHandle, app_path, options, install_callback, NULL) != 0) {
			logger.log("AMDeviceInstallApplication ·µ»ØÊ§°Ü,ipa:%s", path.c_str());
			ret = false;
		}
		CFRelease(sServiceName);
		CFRelease(options);
		AMDeviceStopSession(m_deviceHandle);
		AMDeviceDisconnect(m_deviceHandle);
		return ret;
	}
}