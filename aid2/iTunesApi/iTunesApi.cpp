#include "iTunesApi.h"
#include <string>
#include <vector>

#ifdef WIN32
#define DLOPEN(path) LoadLibrary(path);
#define DLFREE(h) \
	if (h)        \
		FreeLibrary(h);
#define FUNC_LOAD(func) this->func = (PF_##func)GetProcAddress(m_hDll, #func);
#define VALUE_LOAD(func)                      \
	FUNC_LOAD(func);                          \
	{                                         \
		void **ppTmp = (void **)this->func;   \
		if (ppTmp)                            \
			this->func = (PF_##func) * ppTmp; \
	}
#define DIRECZT_LOAD(func) FUNC_LOAD(func)

// windows 下加载 DLL 时搜索  Apple Mobile Device Support 安装目录
BOOL addiTunesFrameworkDllDir() {
	void* hSetting = NULL;
	unsigned long length = 0;
	wchar_t* pCoreFoundationPath = new wchar_t[MAX_PATH * sizeof(wchar_t)];
	if (pCoreFoundationPath == NULL) {
		return 0;
	}
	if (::RegCreateKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support", reinterpret_cast<PHKEY>(&hSetting)) != ERROR_SUCCESS) {
		return 0;
	}
	if (::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting), L"InstallDir", NULL, NULL, NULL, &length) != ERROR_SUCCESS) {
		return 0;
	}
	::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting), L"InstallDir", NULL, NULL, (LPBYTE)pCoreFoundationPath, &length);
	::RegCloseKey(reinterpret_cast<HKEY>(hSetting));

	AddDllDirectory(pCoreFoundationPath);

	// add HKEY_LOCAL_MACHINE\SOFTWARE\Apple Inc.\Apple Application Support
	if (::RegCreateKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Apple Inc.\\Apple Application Support", reinterpret_cast<PHKEY>(&hSetting)) == ERROR_SUCCESS) {
		if (::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting), L"InstallDir", NULL, NULL, NULL, &length) == ERROR_SUCCESS) {
			::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting), L"InstallDir", NULL, NULL, (LPBYTE)pCoreFoundationPath, &length);
			::RegCloseKey(reinterpret_cast<HKEY>(hSetting));
			AddDllDirectory(pCoreFoundationPath);
		}
	}
	delete[] pCoreFoundationPath;
	return SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}


#else
CFBundleRef DLOPEN(const char *path)
{
	CFStringRef dllPath = ::CFStringCreateWithCString(NULL, path, ::kCFStringEncodingUTF8);
	CFURLRef pURLRef = ::CFURLCreateWithFileSystemPath(0, dllPath, kCFURLPOSIXPathStyle, 0);
	CFBundleRef hDll = ::CFBundleCreate(::kCFAllocatorDefault, pURLRef);
	return hDll;
}
#define DLFREE(h) \
	if (h)        \
		h = NULL;
#define DIRECZT_LOAD(func) this->func = (PF_##func)::func;
#define FUNC_LOAD(func) this->func = (PF_##func)::CFBundleGetFunctionPointerForName(m_hDll, ::CFStringCreateWithCString(NULL, #func, ::kCFStringEncodingUTF8));
#define VALUE_LOAD(func) this->func = (PF_##func)(&::func);
#endif

CCoreFoundation &CoreFoundation()
{
	static CCoreFoundation obj;
	return obj;
}

CMobileDevice &MobileDevice()
{
	static CMobileDevice obj;
	return obj;
}

CAirTrafficHost &AirTrafficHost()
{
	static CAirTrafficHost obj;
	return obj;
}

CCoreFoundation::CCoreFoundation(void)
{
#ifdef WIN32
	addiTunesFrameworkDllDir();
	m_hDll = DLOPEN(TEXT("CoreFoundation.dll"));

	FUNC_LOAD(kCFAllocatorDefault);
	FUNC_LOAD(kCFTypeArrayCallBacks);
	VALUE_LOAD(kCFBooleanTrue);
	VALUE_LOAD(kCFBooleanFalse);
	VALUE_LOAD(kCFNumberPositiveInfinity);
	FUNC_LOAD(kCFTypeDictionaryKeyCallBacks);
	FUNC_LOAD(kCFTypeDictionaryValueCallBacks);

	FUNC_LOAD(CFRunLoopRun);
	FUNC_LOAD(CFRunLoopGetCurrent);
	FUNC_LOAD(CFRunLoopGetMain);
	FUNC_LOAD(CFRunLoopStop);
	FUNC_LOAD(__CFStringMakeConstantString);
	FUNC_LOAD(CFRelease);
	FUNC_LOAD(CFRetain);
	FUNC_LOAD(CFDictionaryCreateMutable);
	FUNC_LOAD(CFDictionaryCreateMutableCopy);
	FUNC_LOAD(CFDictionaryCreate);
	FUNC_LOAD(CFPropertyListCreateXMLData);
	FUNC_LOAD(CFDataGetLength);
	FUNC_LOAD(CFDataGetBytes);
	FUNC_LOAD(CFDataGetBytePtr);
	FUNC_LOAD(CFArrayCreateMutable);
	FUNC_LOAD(CFArrayCreateMutableCopy);
	FUNC_LOAD(CFArrayAppendArray);
	FUNC_LOAD(CFDictionarySetValue);
	FUNC_LOAD(CFDictionaryAddValue);
	FUNC_LOAD(CFDictionaryGetValue);
	FUNC_LOAD(CFArrayGetCount);
	FUNC_LOAD(CFArrayGetValueAtIndex);
	FUNC_LOAD(CFArrayRemoveValueAtIndex);
	FUNC_LOAD(CFArrayRemoveAllValues);
	FUNC_LOAD(CFNumberCreate);
	FUNC_LOAD(CFArrayAppendValue);
	FUNC_LOAD(CFDataCreate);
	FUNC_LOAD(CFDictionaryContainsKey);
	FUNC_LOAD(CFStringCreateWithCString);
	FUNC_LOAD(CFStringCreateWithCharacters);
	FUNC_LOAD(CFStringCreateWithCharactersNoCopy);
	FUNC_LOAD(CFStringGetLength);
	FUNC_LOAD(CFURLCreateWithFileSystemPath);
	FUNC_LOAD(CFReadStreamCreateWithFile);
	FUNC_LOAD(CFReadStreamOpen);
	FUNC_LOAD(CFReadStreamClose);
	FUNC_LOAD(CFPropertyListCreateWithStream);
	FUNC_LOAD(CFDataCreateMutable);
	FUNC_LOAD(CFDataAppendBytes);
	FUNC_LOAD(CFGetTypeID);
	FUNC_LOAD(CFPropertyListCreateWithData);
	FUNC_LOAD(CFPropertyListCreateFromXMLData);
	FUNC_LOAD(CFDictionaryGetValueIfPresent);
	FUNC_LOAD(CFDateGetAbsoluteTime);
	FUNC_LOAD(CFDateCreate);
	FUNC_LOAD(CFNumberGetType);
	FUNC_LOAD(CFNumberGetValue);
	FUNC_LOAD(CFStringGetSystemEncoding);
	FUNC_LOAD(CFStringGetCStringPtr);
	FUNC_LOAD(CFStringGetCString);
	FUNC_LOAD(CFStringGetBytes);
	FUNC_LOAD(CFPropertyListCreateData);
	FUNC_LOAD(CFURLWriteDataAndPropertiesToResource);
	FUNC_LOAD(CFDictionaryGetCount);
	FUNC_LOAD(CFDictionaryGetKeysAndValues);
	FUNC_LOAD(CFTimeZoneCopyDefault);
	FUNC_LOAD(CFTimeZoneCopySystem);
	FUNC_LOAD(CFTimeZoneCreateWithName);
	FUNC_LOAD(CFTimeZoneCreateWithTimeIntervalFromGMT);
	FUNC_LOAD(CFAbsoluteTimeGetCurrent);
	FUNC_LOAD(CFAbsoluteTimeGetGregorianDate);
	FUNC_LOAD(CFGregorianDateGetAbsoluteTime);
	FUNC_LOAD(CFStringGetTypeID);
	FUNC_LOAD(CFDictionaryGetTypeID);
	FUNC_LOAD(CFDataGetTypeID);
	FUNC_LOAD(CFNumberGetTypeID);
	FUNC_LOAD(CFAllocatorGetTypeID);
	FUNC_LOAD(CFURLGetTypeID);
	FUNC_LOAD(CFReadStreamGetTypeID);
	FUNC_LOAD(CFDictionaryReplaceValue);
	FUNC_LOAD(CFArrayGetTypeID);
	FUNC_LOAD(CFDateGetTypeID);
	FUNC_LOAD(CFErrorGetTypeID);
	FUNC_LOAD(CFNullGetTypeID);
	FUNC_LOAD(CFBooleanGetTypeID);
	FUNC_LOAD(CFAttributedStringGetTypeID);
	FUNC_LOAD(CFBagGetTypeID);
	FUNC_LOAD(CFBitVectorGetTypeID);
	FUNC_LOAD(CFBundleGetTypeID);
	FUNC_LOAD(CFCalendarGetTypeID);
	FUNC_LOAD(CFCharacterSetGetTypeID);
	FUNC_LOAD(CFLocaleGetTypeID);
	FUNC_LOAD(CFRunArrayGetTypeID);
	FUNC_LOAD(CFSetGetTypeID);
	FUNC_LOAD(CFTimeZoneGetTypeID);
	FUNC_LOAD(CFTreeGetTypeID);
	FUNC_LOAD(CFUUIDGetTypeID);
	FUNC_LOAD(CFWriteStreamGetTypeID);
	FUNC_LOAD(CFXMLNodeGetTypeID);
	FUNC_LOAD(CFStorageGetTypeID);
	FUNC_LOAD(CFSocketGetTypeID);
	FUNC_LOAD(CFWindowsNamedPipeGetTypeID);
	FUNC_LOAD(CFPlugInGetTypeID);
	FUNC_LOAD(CFPlugInInstanceGetTypeID);
	FUNC_LOAD(CFBinaryHeapGetTypeID);
	FUNC_LOAD(CFDateFormatterGetTypeID);
	FUNC_LOAD(CFMessagePortGetTypeID);
	FUNC_LOAD(CFNotificationCenterGetTypeID);
	FUNC_LOAD(CFNumberFormatterGetTypeID);
	FUNC_LOAD(_CFKeyedArchiverUIDGetTypeID);
	FUNC_LOAD(_CFKeyedArchiverUIDGetValue);
	FUNC_LOAD(CFStringCreateWithFormat);
	FUNC_LOAD(CFEqual);
	FUNC_LOAD(CFStringHasSuffix);
	FUNC_LOAD(CFStringCreateCopy);
	FUNC_LOAD(CFStringCreateExternalRepresentation);
	FUNC_LOAD(CFShow);
	FUNC_LOAD(CFDictionaryApplyFunction);
	if (NULL == __CFStringMakeConstantString)
		MessageBoxA(NULL, "CoreFoundation.dll load fun fail", "error", 0);
#endif
}

CCoreFoundation::~CCoreFoundation(void){
	DLFREE(m_hDll)}

CMobileDevice::CMobileDevice()
{
#ifdef WIN32
	m_hDll = DLOPEN(TEXT("MobileDevice.dll"));
	if (m_hDll == NULL)
	{
		m_hDll = DLOPEN(TEXT("iTunesMobileDevice.dll"));
	}
	if (NULL == m_hDll)
		MessageBoxA(NULL, "MobileDevice.dll load fail", std::to_string(GetLastError()).c_str(), 0);
#else
	m_hDll = DLOPEN("/System/Library/PrivateFrameworks/MobileDevice.framework");
	if (NULL == m_hDll)
		printf("MobileDevice.dll load fail");
#endif
	FUNC_LOAD(AMDeviceLookupApplications);
	FUNC_LOAD(AMDeviceInstallApplication);
	FUNC_LOAD(AMDeviceRemoveApplicationArchive);
	FUNC_LOAD(AMDeviceUninstallApplication);
	FUNC_LOAD(AMDeviceArchiveApplication);
	FUNC_LOAD(AMDeviceTransferApplication);
	FUNC_LOAD(AMDeviceStartHouseArrestService);
	FUNC_LOAD(AFCConnectionOpen);
	FUNC_LOAD(AMDServiceConnectionInvalidate);
	FUNC_LOAD(AMDeviceNotificationSubscribe);
	FUNC_LOAD(AMDeviceNotificationUnsubscribe);
	FUNC_LOAD(AMDeviceRelease);
	FUNC_LOAD(AMDeviceConnect);
	FUNC_LOAD(AMDeviceDisconnect);
	FUNC_LOAD(AMDeviceIsPaired);
	FUNC_LOAD(AMDeviceValidatePairing);
	FUNC_LOAD(AMDevicePair);
	FUNC_LOAD(AMDevicePairWithOptions);
	FUNC_LOAD(AMDeviceUnpair);
	FUNC_LOAD(AMDeviceStartSession);
	FUNC_LOAD(AMDeviceSecureStartService);
	FUNC_LOAD(AMDeviceStartService);
	FUNC_LOAD(AMDeviceStopSession);
	FUNC_LOAD(AFCConnectionClose);
	FUNC_LOAD(AFCDeviceInfoOpen);
	FUNC_LOAD(AFCFileInfoOpen);
	FUNC_LOAD(AFCKeyValueRead);
	FUNC_LOAD(AFCKeyValueClose);
	FUNC_LOAD(AFCDirectoryOpen);
	FUNC_LOAD(AFCDirectoryRead);
	FUNC_LOAD(AFCDirectoryClose);
	FUNC_LOAD(AFCDirectoryCreate);
	FUNC_LOAD(AFCRemovePath);
	FUNC_LOAD(AFCRenamePath);
	FUNC_LOAD(AFCFileRefOpen);
	FUNC_LOAD(AFCFileRefRead);
	FUNC_LOAD(AFCFileRefWrite);
	FUNC_LOAD(AFCFileRefClose);
	FUNC_LOAD(AFCFileRefSeek);
	FUNC_LOAD(AFCFileRefTell);
	FUNC_LOAD(AMDeviceCopyDeviceIdentifier);
	FUNC_LOAD(AMDeviceCopyValue);
	FUNC_LOAD(AMDeviceGetInterfaceType);
	FUNC_LOAD(AMRestoreRegisterForDeviceNotifications);
	FUNC_LOAD(USBMuxConnectByPort);
	FUNC_LOAD(AMRestorePerformRecoveryModeRestore);
	FUNC_LOAD(AMRestorePerformDFURestore);
	FUNC_LOAD(AMRestorableDeviceRegisterForNotificationsForDevices);
	FUNC_LOAD(AMRestoreUnregisterForDeviceNotifications);
	FUNC_LOAD(AMRestorableDeviceRestore);
	FUNC_LOAD(AMSRestoreWithApplications);
	FUNC_LOAD(AMSUnregisterTarget);
	FUNC_LOAD(AMDeviceSetValue);
	FUNC_LOAD(AMRecoveryModeDeviceSendFileToDevice);
	FUNC_LOAD(AMRecoveryModeDeviceSendCommandToDevice);
	FUNC_LOAD(AMRecoveryModeDeviceGetProductID);
	FUNC_LOAD(AMRecoveryModeDeviceGetProductType);
	FUNC_LOAD(AMRecoveryModeDeviceGetChipID);
	FUNC_LOAD(AMRecoveryModeDeviceGetECID);
	FUNC_LOAD(AMRecoveryModeDeviceGetLocationID);
	FUNC_LOAD(AMRecoveryModeDeviceGetBoardID);
	FUNC_LOAD(AMRecoveryModeDeviceGetProductionMode);
	FUNC_LOAD(AMRecoveryModeDeviceGetTypeID);
	FUNC_LOAD(AMRecoveryModeGetSoftwareBuildVersion);
	FUNC_LOAD(AMDFUModeDeviceGetProductID);
	FUNC_LOAD(AMDFUModeDeviceGetProductType);
	FUNC_LOAD(AMDFUModeDeviceGetChipID);
	FUNC_LOAD(AMDFUModeDeviceGetECID);
	FUNC_LOAD(AMDFUModeDeviceGetLocationID);
	FUNC_LOAD(AMDFUModeDeviceGetBoardID);
	FUNC_LOAD(AMDFUModeDeviceGetProductionMode);
	FUNC_LOAD(AMDFUModeDeviceGetTypeID);
	FUNC_LOAD(AMRecoveryModeDeviceSetAutoBoot);
	FUNC_LOAD(AMRecoveryModeDeviceReboot);
	FUNC_LOAD(AMRestoreModeDeviceReboot);
	FUNC_LOAD(AMRestoreEnableFileLogging);
	FUNC_LOAD(AMRestoreDisableFileLogging);
	FUNC_LOAD(AMRestorableDeviceGetState);
	FUNC_LOAD(AMRestorableDeviceCopyDFUModeDevice);
	FUNC_LOAD(AMRestorableDeviceCopyRecoveryModeDevice);
	FUNC_LOAD(AMRestorableDeviceCopyAMDevice);
	FUNC_LOAD(AMRestorableDeviceCreateFromAMDevice);
	FUNC_LOAD(AMRestorableDeviceGetProductID);
	FUNC_LOAD(AMRestorableDeviceGetProductType);
	FUNC_LOAD(AMRestorableDeviceGetChipID);
	FUNC_LOAD(AMRestorableDeviceGetECID);
	FUNC_LOAD(AMRestorableDeviceGetLocationID);
	FUNC_LOAD(AMRestorableDeviceGetBoardID);
	FUNC_LOAD(AMRestoreModeDeviceGetTypeID);
	FUNC_LOAD(AMRestoreModeDeviceCopySerialNumber);
	FUNC_LOAD(AMRestorableDeviceCopySerialNumber);
	FUNC_LOAD(AMRecoveryModeDeviceCopySerialNumber);
	FUNC_LOAD(AFCConnectionGetContext);
	FUNC_LOAD(AFCConnectionGetFSBlockSize);
	FUNC_LOAD(AFCConnectionGetIOTimeout);
	FUNC_LOAD(AFCConnectionGetSocketBlockSize);
	FUNC_LOAD(AMRestoreCreateDefaultOptions);
	FUNC_LOAD(AMRestorePerformRestoreModeRestore);
	FUNC_LOAD(AMRestoreModeDeviceCreate);
	FUNC_LOAD(AMRestoreCreatePathsForBundle);
	FUNC_LOAD(AMDeviceGetConnectionID);
	FUNC_LOAD(AMDeviceEnterRecovery);
	FUNC_LOAD(AMDeviceRetain);
	FUNC_LOAD(AMDShutdownNotificationProxy);
	FUNC_LOAD(AMDeviceDeactivate);
	FUNC_LOAD(AMDeviceActivate);
	FUNC_LOAD(AMDeviceRemoveValue);
	FUNC_LOAD(USBMuxListenerCreate);
	FUNC_LOAD(USBMuxListenerHandleData);
	FUNC_LOAD(AMDObserveNotification);
	FUNC_LOAD(AMSInitialize);
	FUNC_LOAD(AMDListenForNotifications);
	FUNC_LOAD(AMDeviceStartServiceWithOptions);
	FUNC_LOAD(AMDServiceConnectionCreate);
	FUNC_LOAD(AMDServiceConnectionGetSocket);
	FUNC_LOAD(AMDServiceConnectionGetSecureIOContext);
	FUNC_LOAD(AMDServiceConnectionReceive);
	FUNC_LOAD(AMDServiceConnectionSend);
	FUNC_LOAD(AMDServiceConnectionReceiveMessage);
	FUNC_LOAD(AMDServiceConnectionSendMessage);
	FUNC_LOAD(AMSChangeBackupPassword);
	FUNC_LOAD(AMSBackupWithOptions);
	FUNC_LOAD(AMSCancelBackupRestore);
	FUNC_LOAD(AMSGetErrorReasonForErrorCode);

#ifdef WIN32
	if (NULL == AMDeviceNotificationSubscribe)
		MessageBoxA(NULL, "MobileDevice.dll load fun fail", "error", 0);
#else
	if (NULL == AMDeviceNotificationSubscribe)
		printf("MobileDevice.dll load fun fail");
#endif
}

CMobileDevice::~CMobileDevice(void){
	DLFREE(m_hDll)}

CAirTrafficHost::CAirTrafficHost(void)
{
#ifdef WIN32
	m_hDll = DLOPEN(L"AirTrafficHost.dll");
	if (NULL == m_hDll)
		MessageBoxA(NULL, "AirTrafficHost.dll load fail", "error", 0);
#else
	m_hDll = DLOPEN("/System/Library/PrivateFrameworks/AirTrafficHost.framework");
	if (NULL == m_hDll)
		printf("AirTrafficHost load fail");
#endif
	FUNC_LOAD(ATCFMessageGetParam);
	FUNC_LOAD(ATHostConnectionCreate);
	FUNC_LOAD(ATHostConnectionCreateWithLibrary);
	FUNC_LOAD(ATHostConnectionSendPing);
	FUNC_LOAD(ATHostConnectionSendAssetMetricsRequest);
	FUNC_LOAD(ATHostConnectionInvalidate);
	FUNC_LOAD(ATHostConnectionClose);
	FUNC_LOAD(ATHostConnectionGetCurrentSessionNumber);
	FUNC_LOAD(ATHostConnectionRelease);
	FUNC_LOAD(ATHostConnectionSendPowerAssertion);
	FUNC_LOAD(ATHostConnectionRetain);
	FUNC_LOAD(ATHostConnectionSendMetadataSyncFinished);
	FUNC_LOAD(ATHostConnectionSendFileError);
	FUNC_LOAD(ATCFMessageCreate);
	FUNC_LOAD(ATHostConnectionSendFileProgress);
	FUNC_LOAD(ATHostConnectionSendAssetCompleted);
	FUNC_LOAD(ATCFMessageGetName);
	FUNC_LOAD(ATHostConnectionSendHostInfo);
	FUNC_LOAD(ATHostConnectionSendSyncRequest);
	FUNC_LOAD(ATHostConnectionSendMessage);
	FUNC_LOAD(ATHostConnectionGetGrappaSessionId);
	FUNC_LOAD(ATHostConnectionReadMessage);
	FUNC_LOAD(ATHostConnectionDestroy);
#ifdef WIN32
	if (NULL == ATHostConnectionReadMessage)
		MessageBoxA(NULL, "AirTrafficHost.dll load fun fail", "error", 0);
#else
	if (NULL == ATHostConnectionReadMessage)
		printf("AirTrafficHost.dll load fun fail");
#endif
}

CAirTrafficHost::~CAirTrafficHost(void)
{
	DLFREE(m_hDll)
}
