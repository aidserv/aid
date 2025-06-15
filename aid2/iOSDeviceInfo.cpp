#include "iOSDeviceInfo.h"
#include "Logger.h"
namespace aid2 {
	//AuthorizeDeviceCallbackFunc iOSDeviceInfo::DoPairCallback = nullptr;

	//ªÒ»°udid
	string getUdid(AMDeviceRef deviceHandle)
	{
		string udid;
		CFStringRef found_device_id = AMDeviceCopyDeviceIdentifier(deviceHandle);
		auto len = CFStringGetLength(found_device_id);
		udid.resize(len);
		CFStringGetCString(found_device_id, (char*)udid.c_str(), len + 1, kCFStringEncodingUTF8);
		CFRelease(found_device_id);
		return udid;
	}

	iOSDeviceInfo::iOSDeviceInfo(AMDeviceRef deviceHandle)
	{
		m_deviceHandle = deviceHandle;
		AMDeviceConnect(m_deviceHandle);
		initializeDevice();
	}

	iOSDeviceInfo::~iOSDeviceInfo()
	{
		AMDeviceDisconnect(m_deviceHandle);
	}

	string iOSDeviceInfo::FairPlayCertificate()
	{
		return m_FairPlayCertificate;
	}

	uint64_t iOSDeviceInfo::FairPlayDeviceType()
	{
		return m_FairPlayDeviceType;
	}

	uint64_t iOSDeviceInfo::KeyTypeSupportVersion()
	{
		return m_KeyTypeSupportVersion;
	}

	string iOSDeviceInfo::DeviceName()
	{
		if (m_deviceName.empty()) {
			initializeDevice();
		}
		return m_deviceName;
	}

	string iOSDeviceInfo::ProductType()
	{
		if (m_productType.empty()) {
			initializeDevice();
		}
		return m_productType;
	}

	string iOSDeviceInfo::DeviceEnclosureColor()
	{
		if (m_deviceEnclosureColor.empty()) {
			initializeDevice();
		}
		return m_deviceEnclosureColor;
	}

	string iOSDeviceInfo::MarketingName()
	{
		if (m_marketingName.empty()) {
			initializeDevice();
		}
		return m_marketingName;
	}

	uint64_t iOSDeviceInfo::TotalDiskCapacity()
	{
		return m_totalDiskCapacity;
	}

	string iOSDeviceInfo::udid()
	{
		if (m_udid.empty()) {
			initializeDevice();
		}
		return m_udid;
	}

	void iOSDeviceInfo::initializeDevice()
	{
		CFStringRef found_device_id = AMDeviceCopyDeviceIdentifier(m_deviceHandle);
		auto ilen = CFStringGetLength(found_device_id);
		m_udid.resize(ilen);
		CFStringGetCString(found_device_id, (char*)m_udid.c_str(), ilen + 1, kCFStringEncodingUTF8);
		CFRelease(found_device_id);

		//DeviceName
		CFStringRef sKey = CFStringCreateWithCString(NULL, "DeviceName", kCFStringEncodingUTF8);
		CFStringRef sValue = AMDeviceCopyValue(m_deviceHandle, NULL, sKey);
		CFRelease(sKey);
		CFIndex len = CFStringGetLength(sValue);
		m_deviceName.resize(len);
		CFStringGetCString(sValue, (char*)m_deviceName.c_str(), len + 1, kCFStringEncodingUTF8);
		CFRelease(sValue);
		
		//ProductType
		sKey = CFStringCreateWithCString(NULL, "ProductType", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, NULL, sKey);
		CFRelease(sKey);
		len = CFStringGetLength(sValue);
		m_productType.resize(len);
		CFStringGetCString(sValue, (char*)m_productType.c_str(), len + 1, kCFStringEncodingUTF8);
		CFRelease(sValue);
	
		//DeviceEnclosureColor
		sKey = CFStringCreateWithCString(NULL, "DeviceEnclosureColor", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, NULL, sKey);
		CFRelease(sKey);
		len = CFStringGetLength(sValue);
		m_deviceEnclosureColor.resize(len);
		CFStringGetCString(sValue, (char*)m_deviceEnclosureColor.c_str(), len + 1, kCFStringEncodingUTF8);
		CFRelease(sValue);
		//MarketingName
		sKey = CFStringCreateWithCString(NULL, "MarketingName", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, NULL, sKey);
		CFRelease(sKey);
		len = CFStringGetLength(sValue);
		m_marketingName.resize(len);
		CFStringGetCString(sValue, (char*)m_marketingName.c_str(), len + 1, kCFStringEncodingUTF8);
		CFRelease(sValue);
	}



	void iOSDeviceInfo::initializeDeviceEx() {
		AMDeviceStartSession(m_deviceHandle);
		//FairPlayCertificate
		CFStringRef sDomain = CFStringCreateWithCString(NULL, "com.apple.mobile.iTunes", kCFStringEncodingUTF8);
		CFStringRef sKey = CFStringCreateWithCString(NULL, "FairPlayCertificate", kCFStringEncodingUTF8);
		CFStringRef sValue = AMDeviceCopyValue(m_deviceHandle, sDomain, sKey);
		CFRelease(sKey);
		CFIndex len = CFDataGetLength(sValue);
		m_FairPlayCertificate.resize(len);
		CFDataGetBytes(sValue, CFRangeMake(0, len), (unsigned char*)m_FairPlayCertificate.data());
		CFRelease(sValue);
		CFRelease(sDomain);
		//FairPlayDeviceType
		sDomain = CFStringCreateWithCString(NULL, "com.apple.mobile.iTunes", kCFStringEncodingUTF8);
		sKey = CFStringCreateWithCString(NULL, "FairPlayDeviceType", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, sDomain, sKey);
		CFRelease(sKey);
		CFNumberGetValue(sValue, kCFNumberSInt32Type, &m_FairPlayDeviceType);
		CFRelease(sValue);
		//KeyTypeSupportVersion
		sKey = CFStringCreateWithCString(NULL, "KeyTypeSupportVersion", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, sDomain, sKey);
		CFRelease(sKey);
		CFNumberGetValue(sValue, kCFNumberSInt32Type, &m_KeyTypeSupportVersion);
		CFRelease(sValue);
		CFRelease(sDomain);
		//TotalDiskCapacity
		sDomain = CFStringCreateWithCString(NULL, "com.apple.disk_usage", kCFStringEncodingUTF8);
		sKey = CFStringCreateWithCString(NULL, "TotalDiskCapacity", kCFStringEncodingUTF8);
		sValue = AMDeviceCopyValue(m_deviceHandle, sDomain, sKey);
		CFRelease(sKey);
		CFNumberGetValue(sValue, kCFNumberSInt64Type, &m_totalDiskCapacity);
		CFRelease(sValue);
		CFRelease(sDomain);
		AMDeviceStopSession(m_deviceHandle);
	}

	int iOSDeviceInfo::DoPair() {
		AMDeviceIsPaired(m_deviceHandle);
		int rc = AMDeviceValidatePairing(m_deviceHandle);
		if (rc == 0) {
			initializeDeviceEx();
			return rc;
		}
		// Do pairing
		CFDictionaryRef dictOptions = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);
		CFStringRef key = CFStringCreateWithCString(NULL, "ExtendedPairingErrors", kCFStringEncodingUTF8);
		CFDictionarySetValue(dictOptions, key, kCFBooleanTrue);
		rc = AMDevicePairWithOptions(m_deviceHandle, dictOptions);
		if (rc == 0) {
			initializeDeviceEx();
		}
        CFRelease(key);
        CFRelease(dictOptions);
        return rc;
	}
}