#pragma once
#ifdef WIN32
#include <Windows.h>
#else
#include <CoreFoundation/CoreFoundation.h>
typedef CFBundleRef HMODULE;
#endif
#include "unkonwStruct.h"
#include <string>

#define FUNC_DEF(TRet, name, ...)           \
	typedef TRet (*PF_##name)(__VA_ARGS__); \
	PF_##name name = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CoreFoundation.dll
class CCoreFoundation
{
public:
	CCoreFoundation(void);
	~CCoreFoundation(void);

public:
#ifdef WIN32
	FUNC_DEF(void, CFRunLoopRun);
	FUNC_DEF(CFRunLoopRef, CFRunLoopGetCurrent);
	FUNC_DEF(CFRunLoopRef, CFRunLoopGetMain);
	FUNC_DEF(void, CFRunLoopStop, CFRunLoopRef);
	FUNC_DEF(CFStringRef, __CFStringMakeConstantString, const char *);
	FUNC_DEF(void, CFRelease, CFTypeRef);
	FUNC_DEF(CFTypeRef, CFRetain, CFTypeRef);
	FUNC_DEF(void, kCFAllocatorDefault);
	FUNC_DEF(void, kCFTypeArrayCallBacks);
	FUNC_DEF(void, kCFBooleanTrue);
	FUNC_DEF(void, kCFBooleanFalse);
	FUNC_DEF(void, kCFNumberPositiveInfinity);
	FUNC_DEF(void, kCFTypeDictionaryKeyCallBacks);
	FUNC_DEF(void, kCFTypeDictionaryValueCallBacks);
	FUNC_DEF(CFMutableDictionaryRef, CFDictionaryCreateMutable, CFAllocatorRef allocator, CFIndex capacity, const void* keyCallBacks, const void* valueCallBacks);
	FUNC_DEF(CFMutableDictionaryRef, CFDictionaryCreateMutableCopy, CFAllocatorRef, CFIndex, CFDictionaryRef);
	FUNC_DEF(CFDictionaryRef, CFDictionaryCreate, CFAllocatorRef, void *, void *, int, void *, void *);
	FUNC_DEF(CFDataRef, CFPropertyListCreateXMLData, CFAllocatorRef, CFPropertyListRef);
	FUNC_DEF(CFIndex, CFDataGetLength, CFDataRef);
	FUNC_DEF(void *, CFDataGetBytePtr, CFDataRef);
	FUNC_DEF(void*, CFDataGetBytes, CFDataRef, CFRange, unsigned char* buffer);
	FUNC_DEF(CFMutableArrayRef, CFArrayCreateMutable, CFAllocatorRef, int, void *);
	FUNC_DEF(CFMutableArrayRef, CFArrayCreateMutableCopy, CFAllocatorRef, CFIndex, CFArrayRef);
	FUNC_DEF(void, CFArrayAppendArray, CFMutableArrayRef, CFArrayRef, CFRange);
	FUNC_DEF(void, CFDictionarySetValue, CFMutableDictionaryRef, const void *, const void *);
	FUNC_DEF(void, CFDictionaryAddValue, CFMutableDictionaryRef, const void *, const void *);
	FUNC_DEF(void *, CFDictionaryGetValue, CFDictionaryRef, CFStringRef);
	FUNC_DEF(int, CFArrayGetCount, CFArrayRef);
	FUNC_DEF(void *, CFArrayGetValueAtIndex, CFArrayRef, int);
	FUNC_DEF(void, CFArrayRemoveValueAtIndex, CFMutableArrayRef, int);
	FUNC_DEF(void, CFArrayRemoveAllValues, CFMutableArrayRef);
	FUNC_DEF(CFNumberRef, CFNumberCreate, CFAllocatorRef, CFNumberType, void *);
	FUNC_DEF(void, CFArrayAppendValue, CFMutableArrayRef, void *);
	FUNC_DEF(CFDataRef, CFDataCreate, CFAllocatorRef, unsigned char*, CFIndex);
	FUNC_DEF(Boolean, CFDictionaryContainsKey, CFDictionaryRef, void *);
	FUNC_DEF(CFStringRef, CFStringCreateWithCString, CFAllocatorRef, const char *, int);
	FUNC_DEF(CFStringRef, CFStringCreateWithCharacters, CFAllocatorRef, const wchar_t *, int);
	FUNC_DEF(CFStringRef, CFStringCreateWithCharactersNoCopy, CFAllocatorRef, const wchar_t *, int, CFAllocatorRef);
	FUNC_DEF(int, CFStringGetLength, CFStringRef);
	FUNC_DEF(CFURLRef, CFURLCreateWithFileSystemPath, CFAllocatorRef, CFStringRef, CFURLPathStyle, int);
	FUNC_DEF(CFReadStreamRef, CFReadStreamCreateWithFile, CFAllocatorRef, CFURLRef);
	FUNC_DEF(int, CFReadStreamOpen, CFReadStreamRef);
	FUNC_DEF(void, CFReadStreamClose, CFReadStreamRef);
	FUNC_DEF(CFPropertyListRef, CFPropertyListCreateWithStream, CFAllocatorRef, CFReadStreamRef, CFIndex, int, void *, CFStringRef *);
	FUNC_DEF(CFMutableDataRef, CFDataCreateMutable, CFAllocatorRef, CFIndex);
	FUNC_DEF(void, CFDataAppendBytes, CFMutableDataRef, const uint8_t *, CFIndex);
	FUNC_DEF(CFTypeID, CFGetTypeID, CFTypeRef);
	FUNC_DEF(CFPropertyListRef, CFPropertyListCreateWithData, CFAllocatorRef, CFMutableDataRef, int, void *, CFStringRef *);
	FUNC_DEF(CFPropertyListRef, CFPropertyListCreateFromXMLData, CFAllocatorRef, CFDataRef, int, CFStringRef *);
	FUNC_DEF(int, CFDictionaryGetValueIfPresent, CFDictionaryRef, void *, void **);
	FUNC_DEF(CFAbsoluteTime, CFDateGetAbsoluteTime, CFDateRef);
	FUNC_DEF(CFDateRef, CFDateCreate, CFAllocatorRef, double);
	FUNC_DEF(CFNumberType, CFNumberGetType, CFNumberRef);
	FUNC_DEF(int, CFNumberGetValue, CFNumberRef, CFNumberType, void *);
	FUNC_DEF(int, CFStringGetSystemEncoding);
	FUNC_DEF(const char *, CFStringGetCStringPtr, CFStringRef, CFStringEncoding);
	FUNC_DEF(int, CFStringGetCString, CFStringRef, char *, CFIndex, CFStringEncoding);
	FUNC_DEF(int, CFStringGetBytes, CFStringRef, CFRange, CFStringEncoding, uint8_t, Boolean, uint8_t *, CFIndex, CFIndex *);
	FUNC_DEF(CFDataRef, CFPropertyListCreateData, CFAllocatorRef, CFPropertyListRef, int, int, void **);
	FUNC_DEF(int, CFURLWriteDataAndPropertiesToResource, CFURLRef, CFDataRef, CFDictionaryRef, int *);
	FUNC_DEF(int, CFDictionaryGetCount, CFDictionaryRef);
	FUNC_DEF(void, CFDictionaryGetKeysAndValues, CFDictionaryRef, void **, void **);
	FUNC_DEF(CFTimeZoneRef, CFTimeZoneCopyDefault);
	FUNC_DEF(CFTimeZoneRef, CFTimeZoneCopySystem);
	FUNC_DEF(CFTimeZoneRef, CFTimeZoneCreateWithName, CFAllocatorRef, CFStringRef, Boolean);
	FUNC_DEF(CFTimeZoneRef, CFTimeZoneCreateWithTimeIntervalFromGMT, CFAllocatorRef, CFTimeInterval ti);
	FUNC_DEF(CFAbsoluteTime, CFAbsoluteTimeGetCurrent);
	FUNC_DEF(CFGregorianDate, CFAbsoluteTimeGetGregorianDate, CFAbsoluteTime, CFTimeZoneRef);
	FUNC_DEF(CFAbsoluteTime, CFGregorianDateGetAbsoluteTime, CFGregorianDate, CFTimeZoneRef);
	FUNC_DEF(CFTypeID, CFStringGetTypeID);
	FUNC_DEF(CFTypeID, CFDictionaryGetTypeID);
	FUNC_DEF(CFTypeID, CFDataGetTypeID);
	FUNC_DEF(CFTypeID, CFNumberGetTypeID);
	FUNC_DEF(CFTypeID, CFAllocatorGetTypeID);
	FUNC_DEF(CFTypeID, CFURLGetTypeID);
	FUNC_DEF(CFTypeID, CFReadStreamGetTypeID);
	FUNC_DEF(void, CFDictionaryReplaceValue, CFDictionaryRef, void *, void *);
	FUNC_DEF(CFTypeID, CFArrayGetTypeID);
	FUNC_DEF(CFTypeID, CFDateGetTypeID);
	FUNC_DEF(CFTypeID, CFErrorGetTypeID);
	FUNC_DEF(CFTypeID, CFNullGetTypeID);
	FUNC_DEF(CFTypeID, CFBooleanGetTypeID);
	FUNC_DEF(CFTypeID, CFAttributedStringGetTypeID);
	FUNC_DEF(CFTypeID, CFBagGetTypeID);
	FUNC_DEF(CFTypeID, CFBitVectorGetTypeID);
	FUNC_DEF(CFTypeID, CFBundleGetTypeID);
	FUNC_DEF(CFTypeID, CFCalendarGetTypeID);
	FUNC_DEF(CFTypeID, CFCharacterSetGetTypeID);
	FUNC_DEF(CFTypeID, CFLocaleGetTypeID);
	FUNC_DEF(CFTypeID, CFRunArrayGetTypeID);
	FUNC_DEF(CFTypeID, CFSetGetTypeID);
	FUNC_DEF(CFTypeID, CFTimeZoneGetTypeID);
	FUNC_DEF(CFTypeID, CFTreeGetTypeID);
	FUNC_DEF(CFTypeID, CFUUIDGetTypeID);
	FUNC_DEF(CFTypeID, CFWriteStreamGetTypeID);
	FUNC_DEF(CFTypeID, CFXMLNodeGetTypeID);
	FUNC_DEF(CFTypeID, CFStorageGetTypeID);
	FUNC_DEF(CFTypeID, CFSocketGetTypeID);
	FUNC_DEF(CFTypeID, CFWindowsNamedPipeGetTypeID);
	FUNC_DEF(CFTypeID, CFPlugInGetTypeID);
	FUNC_DEF(CFTypeID, CFPlugInInstanceGetTypeID);
	FUNC_DEF(CFTypeID, CFBinaryHeapGetTypeID);
	FUNC_DEF(CFTypeID, CFDateFormatterGetTypeID);
	FUNC_DEF(CFTypeID, CFMessagePortGetTypeID);
	FUNC_DEF(CFTypeID, CFNotificationCenterGetTypeID);
	FUNC_DEF(CFTypeID, CFNumberFormatterGetTypeID);
	FUNC_DEF(CFTypeID, _CFKeyedArchiverUIDGetTypeID);
	FUNC_DEF(int, _CFKeyedArchiverUIDGetValue, void *);
	FUNC_DEF(CFStringRef, CFStringCreateWithFormat, CFAllocatorRef, CFDictionaryRef, CFStringRef, ...);
	FUNC_DEF(CFBundleRef, CFBundleGetMainBundle);
	FUNC_DEF(CFURLRef, CFBundleCopyBundleURL, CFBundleRef);
	FUNC_DEF(CFURLRef, CFURLCreateCopyDeletingLastPathComponent, CFAllocatorRef, CFURLRef);
	FUNC_DEF(void *, CFURLGetFileSystemRepresentation, CFURLRef, void *, uint8_t *, CFIndex);
	FUNC_DEF(Boolean, CFEqual, CFTypeRef, CFTypeRef);
	FUNC_DEF(Boolean, CFStringHasSuffix, CFTypeRef, CFTypeRef);
	FUNC_DEF(CFStringRef, CFStringCreateCopy, CFAllocatorRef, CFStringRef);
	FUNC_DEF(CFDataRef, CFStringCreateExternalRepresentation, CFAllocatorRef, CFStringRef, CFStringEncoding, UInt8);
	FUNC_DEF(void, CFShow, CFTypeRef);
	FUNC_DEF(void, CFDictionaryApplyFunction,CFDictionaryRef, void*, void* context);
#endif
private:
	HMODULE m_hDll = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MobileDevice.dll
class CMobileDevice
{
public:
	CMobileDevice();
	~CMobileDevice(void);

public:
	FUNC_DEF(int, AMDeviceLookupApplications, void *, void *, void **);
	FUNC_DEF(int, AMDeviceStartHouseArrestService, void *, void *, void *, void *, void *);
	FUNC_DEF(int, AMDeviceInstallApplication, void *, CFStringRef, void *, void *, void *);
	FUNC_DEF(int, AMDeviceUninstallApplication, void *, CFStringRef, void *, void *, void *);
	FUNC_DEF(int, AMDeviceRemoveApplicationArchive, void *, CFStringRef, void *, void *, void *);
	FUNC_DEF(int, AMDeviceArchiveApplication, void *, CFStringRef, void *, void *, void *);
	FUNC_DEF(int, AMDeviceTransferApplication, void*, CFStringRef, CFDictionaryRef, void*, void*);
	FUNC_DEF(int, AFCConnectionOpen, void* socketHandle, unsigned int io_timeout, AFCRef* afc);
	FUNC_DEF(int, AMDServiceConnectionInvalidate, void *);
	FUNC_DEF(int, AMDeviceNotificationSubscribe, void *, int, int, int, void **);
	FUNC_DEF(int, AMDeviceNotificationUnsubscribe, void *);
	FUNC_DEF(int, AMDeviceRelease, void *);
	FUNC_DEF(int, AMDeviceConnect, void *);
	FUNC_DEF(int, AMDeviceDisconnect, void *);
	FUNC_DEF(int, AMDeviceIsPaired, void *);
	FUNC_DEF(int, AMDeviceValidatePairing, void *);
	FUNC_DEF(int, AMDevicePair, void *);
	FUNC_DEF(int, AMDevicePairWithOptions, void*, CFDictionaryRef);
	FUNC_DEF(int, AMDeviceUnpair, void *);
	FUNC_DEF(int, AMDeviceStartSession, void *);
	FUNC_DEF(int, AMDeviceSecureStartService, void* device, CFStringRef serviceName, int, void** serviceHandle);
	FUNC_DEF(int, AMDeviceStartService, void* device, CFStringRef service_name, void** serviceHandle, unsigned int* unknown);
	FUNC_DEF(int, AMDeviceStopSession, void *);
	FUNC_DEF(int, AFCConnectionClose, AFCRef);
	FUNC_DEF(int, AFCDeviceInfoOpen, void *, void **);
	FUNC_DEF(unsigned int, AFCFileInfoOpen, AFCRef afc, const char* path, afc_dictionary** fileinfo);
	FUNC_DEF(int, AFCKeyValueRead, afc_dictionary* dict, char** key, char** value);
	FUNC_DEF(int, AFCKeyValueClose, afc_dictionary* dic);
	FUNC_DEF(int, AFCDirectoryOpen, void *, void *, void **);
	FUNC_DEF(int, AFCDirectoryRead, void *, void *, void **);
	FUNC_DEF(int, AFCDirectoryClose, void *, void *);
	FUNC_DEF(int, AFCDirectoryCreate, void *, void *);
	FUNC_DEF(int, AFCRemovePath, AFCRef afc, const char* path);
	FUNC_DEF(int, AFCRenamePath, AFCRef afc, const char* path, const char* new_path);
	FUNC_DEF(int, AFCFileRefOpen, AFCRef afc, const char* path, unsigned __int64 mode, AFCFileRef* file);
	FUNC_DEF(int, AFCFileRefRead, AFCRef afc, AFCFileRef file, void* buf, size_t * plen);
	FUNC_DEF(int, AFCFileRefWrite, AFCRef afc, AFCFileRef file, void* buf, size_t len);
	FUNC_DEF(int, AFCFileRefClose, AFCRef afc, AFCFileRef file);
	FUNC_DEF(int, AFCFileRefSeek, void *, unsigned long long, unsigned long long, unsigned long);
	FUNC_DEF(int, AFCFileRefTell, void *, unsigned long long, unsigned long long *);
	FUNC_DEF(void *, AMDeviceCopyDeviceIdentifier, void *);
	FUNC_DEF(void *, AMDeviceCopyValue, void *, void *, void *);
	FUNC_DEF(int, AMDeviceGetInterfaceType, void *);
	FUNC_DEF(int, AMRestoreRegisterForDeviceNotifications, void *, void *, void *, void *, int, void *);
	FUNC_DEF(int, USBMuxConnectByPort, int, short, void *);
	FUNC_DEF(int, AMRestorePerformRecoveryModeRestore, void *, void *, void *, void *);
	FUNC_DEF(int, AMRestorePerformDFURestore, void *, void *, void *, void *);
	FUNC_DEF(int, AMRestorableDeviceRegisterForNotificationsForDevices, am_recovery_device_notification_callback, void *, unsigned int, void *, void *);
	FUNC_DEF(void, AMRestoreUnregisterForDeviceNotifications);
	FUNC_DEF(int, AMRestorableDeviceRestore, am_restore_device *, CFDictionaryRef, void *, void *);
	FUNC_DEF(int, AMSRestoreWithApplications, void *, void *, void *, void *, void *, void *, void *);
	FUNC_DEF(int, AMSUnregisterTarget, void *);
	FUNC_DEF(int, AMDeviceSetValue, void *, void *, void *, void *);
	FUNC_DEF(int, AMRecoveryModeDeviceSendFileToDevice, void *, CFStringRef);
	FUNC_DEF(int, AMRecoveryModeDeviceSendCommandToDevice, void *, CFStringRef);
	FUNC_DEF(uint16_t, AMRecoveryModeDeviceGetProductID, void *);
	FUNC_DEF(unsigned long, AMRecoveryModeDeviceGetProductType, void *);
	FUNC_DEF(unsigned long, AMRecoveryModeDeviceGetChipID, void *);
	FUNC_DEF(uint64_t, AMRecoveryModeDeviceGetECID, void *);
	FUNC_DEF(unsigned long, AMRecoveryModeDeviceGetLocationID, void *);
	FUNC_DEF(unsigned long, AMRecoveryModeDeviceGetBoardID, void *);
	FUNC_DEF(unsigned char, AMRecoveryModeDeviceGetProductionMode, void *);
	FUNC_DEF(unsigned long, AMRecoveryModeDeviceGetTypeID, void *);
	FUNC_DEF(void *, AMRecoveryModeGetSoftwareBuildVersion, void *);
	FUNC_DEF(uint16_t, AMDFUModeDeviceGetProductID, void *);
	FUNC_DEF(unsigned long, AMDFUModeDeviceGetProductType, void *);
	FUNC_DEF(unsigned long, AMDFUModeDeviceGetChipID, void *);
	FUNC_DEF(uint64_t, AMDFUModeDeviceGetECID, void *);
	FUNC_DEF(unsigned long, AMDFUModeDeviceGetLocationID, void *);
	FUNC_DEF(unsigned long, AMDFUModeDeviceGetBoardID, void *);
	FUNC_DEF(unsigned char, AMDFUModeDeviceGetProductionMode, void *);
	FUNC_DEF(unsigned long, AMDFUModeDeviceGetTypeID, void *);
	FUNC_DEF(int, AMRecoveryModeDeviceSetAutoBoot, void *, unsigned char);
	FUNC_DEF(int, AMRecoveryModeDeviceReboot, void *);
	FUNC_DEF(int, AMRestoreModeDeviceReboot, void *);
	FUNC_DEF(int, AMRestoreEnableFileLogging, char *);
	FUNC_DEF(int, AMRestoreDisableFileLogging);
	FUNC_DEF(int, AMRestorableDeviceGetState, void *);
	FUNC_DEF(void *, AMRestorableDeviceCopyDFUModeDevice, void *);
	FUNC_DEF(void *, AMRestorableDeviceCopyRecoveryModeDevice, void *);
	FUNC_DEF(void *, AMRestorableDeviceCopyAMDevice, void *);
	FUNC_DEF(void *, AMRestorableDeviceCreateFromAMDevice, void *);
	FUNC_DEF(uint16_t, AMRestorableDeviceGetProductID, void *);
	FUNC_DEF(unsigned long, AMRestorableDeviceGetProductType, void *);
	FUNC_DEF(unsigned long, AMRestorableDeviceGetChipID, void *);
	FUNC_DEF(uint64_t, AMRestorableDeviceGetECID, void *);
	FUNC_DEF(unsigned long, AMRestorableDeviceGetLocationID, void *);
	FUNC_DEF(unsigned long, AMRestorableDeviceGetBoardID, void *);
	FUNC_DEF(unsigned long, AMRestoreModeDeviceGetTypeID, void *);
	FUNC_DEF(void *, AMRestoreModeDeviceCopySerialNumber, void *);
	FUNC_DEF(void *, AMRestorableDeviceCopySerialNumber, void *);
	FUNC_DEF(void *, AMRecoveryModeDeviceCopySerialNumber, void *);
	FUNC_DEF(int, AFCConnectionGetContext, void *);
	FUNC_DEF(int, AFCConnectionGetFSBlockSize, void *);
	FUNC_DEF(int, AFCConnectionGetIOTimeout, void *);
	FUNC_DEF(int, AFCConnectionGetSocketBlockSize, void *);
	FUNC_DEF(void *, AMRestoreCreateDefaultOptions, void *);
	FUNC_DEF(int, AMRestorePerformRestoreModeRestore, void *, void *, void *, void *);
	FUNC_DEF(void *, AMRestoreModeDeviceCreate, int, int, int);
	FUNC_DEF(int, AMRestoreCreatePathsForBundle, void *, void *, void *, int, void **, void **, int, void **);
	FUNC_DEF(int, AMDeviceGetConnectionID, void *);
	FUNC_DEF(int, AMDeviceEnterRecovery, void *);
	FUNC_DEF(int, AMDeviceRetain, void *);
	FUNC_DEF(int, AMDShutdownNotificationProxy, void *);
	FUNC_DEF(int, AMDeviceDeactivate, void *);
	FUNC_DEF(int, AMDeviceActivate, void *, void *);
	FUNC_DEF(int, AMDeviceRemoveValue, void *, int, void *);
	FUNC_DEF(int, USBMuxListenerCreate, void *, void **);
	FUNC_DEF(int, USBMuxListenerHandleData, void *);
	FUNC_DEF(int, AMDObserveNotification, void *, void *);
	FUNC_DEF(int, AMSInitialize);
	FUNC_DEF(int, AMDListenForNotifications, void *, void *, void *);
	FUNC_DEF(int, AMDeviceStartServiceWithOptions, void *, CFStringRef, void *, int *);
	FUNC_DEF(void *, AMDServiceConnectionCreate, CFStringRef, void *, void *);
	FUNC_DEF(void *, AMDServiceConnectionGetSocket, void *);
	FUNC_DEF(int, AMDServiceConnectionGetSecureIOContext, void *);
	FUNC_DEF(int, AMDServiceConnectionReceive, void *, void *, int);
	FUNC_DEF(int, AMDServiceConnectionSend, void *, void *, int);
	FUNC_DEF(int, AMDServiceConnectionReceiveMessage, void *, CFDictionaryRef *, int *);
	FUNC_DEF(int, AMDServiceConnectionSendMessage, void *, CFDictionaryRef, int);
	FUNC_DEF(int, AMSChangeBackupPassword, CFStringRef, CFStringRef, CFStringRef, int *);
	FUNC_DEF(int, AMSBackupWithOptions, CFStringRef, CFStringRef, CFStringRef, CFDictionaryRef, BackupCallBack, int);
	FUNC_DEF(int, AMSCancelBackupRestore, void *);
	FUNC_DEF(CFStringRef, AMSGetErrorReasonForErrorCode, int);



private:
	HMODULE m_hDll = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AirTrafficHost.dll
class CAirTrafficHost
{
public:
	CAirTrafficHost(void);
	~CAirTrafficHost(void);

public:
	FUNC_DEF(void *, ATCFMessageGetParam, void *, void *);
	FUNC_DEF(int, ATHostConnectionGetCurrentSessionNumber, ATHRef);
	FUNC_DEF(void, ATHostConnectionSendFileProgress, void *, void *, void *, double, int, int);
	FUNC_DEF(CFDictionaryRef, ATCFMessageCreate, uint32_t sesssion, CFStringRef messageType, CFDictionaryRef params);
	FUNC_DEF(ATHRef, ATHostConnectionCreateWithLibrary, CFStringRef library, CFStringRef udid, CFStringRef athexe);
	FUNC_DEF(int, ATHostConnectionCreate, void *);
	FUNC_DEF(void, ATHostConnectionSendPing, ATHRef);
	FUNC_DEF(void, ATHostConnectionSendAssetMetricsRequest, void *, int);
	FUNC_DEF(void, ATHostConnectionInvalidate, void *);
	FUNC_DEF(void, ATHostConnectionClose, ATHRef);
	FUNC_DEF(void, ATHostConnectionRelease, ATHRef);
	FUNC_DEF(int, ATHostConnectionSendPowerAssertion, ATHRef, CFBooleanRef enable);
	FUNC_DEF(int, ATHostConnectionRetain, ATHRef);
	FUNC_DEF(int, ATHostConnectionSendMetadataSyncFinished, ATHRef, CFDictionaryRef, CFDictionaryRef);
	FUNC_DEF(void, ATHostConnectionSendFileError, ATHRef, void *, void *, int);
	FUNC_DEF(int, ATHostConnectionSendAssetCompleted, ATHRef, void *, void *, void *);
	FUNC_DEF(CFStringRef, ATCFMessageGetName, ATHRef);
	FUNC_DEF(int, ATHostConnectionSendHostInfo, ATHRef, CFDictionaryRef);
	FUNC_DEF(int, ATHostConnectionSendSyncRequest, ATHRef, CFArrayRef, CFDictionaryRef, CFDictionaryRef);
	FUNC_DEF(int, ATHostConnectionSendMessage, ATHRef, CFDictionaryRef);
	FUNC_DEF(int, ATHostConnectionGetGrappaSessionId, ATHRef);
	FUNC_DEF(void *, ATHostConnectionReadMessage, ATHRef);
	FUNC_DEF(void, ATHostConnectionDestroy, ATHRef);
	
private:
	HMODULE m_hDll = 0;
};

CCoreFoundation &CoreFoundation();
CMobileDevice &MobileDevice();
CAirTrafficHost &AirTrafficHost();