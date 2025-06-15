#pragma once
#ifndef WIN32
#include <CoreFoundation/CFRunLoop.h>
#else
#endif
#include <stdlib.h>
#include "unkonwStruct.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef void **PPV;
    typedef wchar_t *PWCHAR;

    ///////////////helper////////////
    void *MyPlistToCF(void *, int);
    void *MyCFToPlist(void *, CFIndex*);
    Boolean MyCFDictIsContainsKey(void *, void *);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CoreFoundation.dll
#ifdef WIN32
    void CFRunLoopRun();
    CFRunLoopRef CFRunLoopGetCurrent(); //not used in windows
    CFRunLoopRef CFRunLoopGetMain(); //not used in windows
    void CFRunLoopStop(CFRunLoopRef);   //not used in windows
    CFStringRef __CFStringMakeConstantString(const char *);
    void CFRelease(CFTypeRef);
    CFTypeRef CFRetain(CFTypeRef cf);
    extern void *kCFAllocatorDefault;
    extern void *kCFTypeArrayCallBacks;
    extern void *kCFBooleanTrue;
    extern void *kCFBooleanFalse;
    extern void *kCFNumberPositiveInfinity;
    extern void *kCFTypeDictionaryKeyCallBacks;
    extern void *kCFTypeDictionaryValueCallBacks;

    CFMutableDictionaryRef CFDictionaryCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const void* keyCallBacks, const void* valueCallBacks);
    CFMutableDictionaryRef CFDictionaryCreateMutableCopy(CFAllocatorRef, CFIndex, CFDictionaryRef);
    CFDictionaryRef CFDictionaryCreate(CFAllocatorRef, void *, void *, int, void *, void *);
    CFDataRef CFPropertyListCreateXMLData(CFAllocatorRef, CFPropertyListRef);
    CFIndex CFDataGetLength(CFDataRef);
    void *CFDataGetBytePtr(CFDataRef);
    void* CFDataGetBytes(CFDataRef theData, CFRange range, unsigned char* buffer);
    CFMutableArrayRef CFArrayCreateMutable(CFAllocatorRef, int, void *);
    CFMutableArrayRef CFArrayCreateMutableCopy(CFAllocatorRef, CFIndex, CFArrayRef);
    void CFArrayAppendArray(CFMutableArrayRef, CFArrayRef, CFRange);
    void CFDictionarySetValue(CFMutableDictionaryRef, const void *, const void *);
    void CFDictionaryAddValue(CFMutableDictionaryRef, const void *, const void *);
    void *CFDictionaryGetValue(CFDictionaryRef, CFStringRef);
    int CFArrayGetCount(CFArrayRef);
    void *CFArrayGetValueAtIndex(CFArrayRef, int);
    void CFArrayRemoveValueAtIndex(CFMutableArrayRef, int);
    void CFArrayRemoveAllValues(CFMutableArrayRef);
    CFNumberRef CFNumberCreate(CFAllocatorRef, CFNumberType, void *);
    void CFArrayAppendValue(CFMutableArrayRef, void *);
    CFDataRef CFDataCreate(CFAllocatorRef alloc, unsigned char* bytes, CFIndex length);
    Boolean CFDictionaryContainsKey(CFDictionaryRef, void *);
    CFStringRef CFStringCreateWithCString(CFAllocatorRef, const char *, int);
    CFStringRef CFStringCreateWithCharacters(CFAllocatorRef, const wchar_t *, int);
    CFStringRef CFStringCreateWithCharactersNoCopy(CFAllocatorRef, const wchar_t *, int, CFAllocatorRef);
    int CFStringGetLength(CFStringRef);
    CFURLRef CFURLCreateWithFileSystemPath(CFAllocatorRef, CFStringRef, CFURLPathStyle, int);
    CFReadStreamRef CFReadStreamCreateWithFile(CFAllocatorRef, CFURLRef);
    int CFReadStreamOpen(CFReadStreamRef);
    void CFReadStreamClose(CFReadStreamRef);
    CFPropertyListRef CFPropertyListCreateWithStream(CFAllocatorRef, CFReadStreamRef, CFIndex, int, void *, CFStringRef *);
    CFMutableDataRef CFDataCreateMutable(CFAllocatorRef, CFIndex);
    void CFDataAppendBytes(CFMutableDataRef, const uint8_t *, CFIndex);
    CFTypeID CFGetTypeID(CFTypeRef);
    CFPropertyListRef CFPropertyListCreateWithData(CFAllocatorRef, CFMutableDataRef, int, void *, CFStringRef *);
    CFPropertyListRef CFPropertyListCreateFromXMLData(CFAllocatorRef, CFDataRef, int, CFStringRef *);
    int CFDictionaryGetValueIfPresent(CFDictionaryRef, void *, void **);
    CFAbsoluteTime CFDateGetAbsoluteTime(CFDateRef);
    CFDateRef CFDateCreate(CFAllocatorRef, double);
    CFNumberType CFNumberGetType(CFNumberRef);
    int CFNumberGetValue(CFNumberRef, CFNumberType, void *);
    int CFStringGetSystemEncoding();
    const char *CFStringGetCStringPtr(CFStringRef, CFStringEncoding);
    int CFStringGetCString(CFStringRef, char *, CFIndex, CFStringEncoding);
    int CFStringGetBytes(CFStringRef, CFRange, CFStringEncoding, uint8_t, Boolean, uint8_t *, CFIndex, CFIndex *);
    CFDataRef CFPropertyListCreateData(CFAllocatorRef, CFPropertyListRef, int, int, void **);
    int CFURLWriteDataAndPropertiesToResource(CFURLRef, CFDataRef, CFDictionaryRef, int *);
    int CFDictionaryGetCount(CFDictionaryRef);
    void CFDictionaryGetKeysAndValues(CFDictionaryRef, void **, void **);
    CFTimeZoneRef CFTimeZoneCopyDefault();
    CFTimeZoneRef CFTimeZoneCopySystem();
    CFTimeZoneRef CFTimeZoneCreateWithName(CFAllocatorRef, CFStringRef, Boolean);
    CFTimeZoneRef CFTimeZoneCreateWithTimeIntervalFromGMT(CFAllocatorRef, CFTimeInterval ti);
    CFAbsoluteTime CFAbsoluteTimeGetCurrent();
    CFGregorianDate CFAbsoluteTimeGetGregorianDate(CFAbsoluteTime, CFTimeZoneRef);
    CFAbsoluteTime CFGregorianDateGetAbsoluteTime(CFGregorianDate, CFTimeZoneRef);
    CFTypeID CFStringGetTypeID();
    CFTypeID CFDictionaryGetTypeID();
    CFTypeID CFDataGetTypeID();
    CFTypeID CFNumberGetTypeID();
    CFTypeID CFAllocatorGetTypeID();
    CFTypeID CFURLGetTypeID();
    CFTypeID CFReadStreamGetTypeID();
    void CFDictionaryReplaceValue(CFDictionaryRef, void *, void *);
    CFTypeID CFArrayGetTypeID();
    CFTypeID CFDateGetTypeID();
    CFTypeID CFErrorGetTypeID();
    CFTypeID CFNullGetTypeID();
    CFTypeID CFBooleanGetTypeID();
    CFTypeID CFAttributedStringGetTypeID();
    CFTypeID CFBagGetTypeID();
    CFTypeID CFBitVectorGetTypeID();
    CFTypeID CFBundleGetTypeID();
    CFTypeID CFCalendarGetTypeID();
    CFTypeID CFCharacterSetGetTypeID();
    CFTypeID CFLocaleGetTypeID();
    CFTypeID CFRunArrayGetTypeID();
    CFTypeID CFSetGetTypeID();
    CFTypeID CFTimeZoneGetTypeID();
    CFTypeID CFTreeGetTypeID();
    CFTypeID CFUUIDGetTypeID();
    CFTypeID CFWriteStreamGetTypeID();
    CFTypeID CFXMLNodeGetTypeID();
    CFTypeID CFStorageGetTypeID();
    CFTypeID CFSocketGetTypeID();
    CFTypeID CFWindowsNamedPipeGetTypeID();
    CFTypeID CFPlugInGetTypeID();
    CFTypeID CFPlugInInstanceGetTypeID();
    CFTypeID CFBinaryHeapGetTypeID();
    CFTypeID CFDateFormatterGetTypeID();
    CFTypeID CFMessagePortGetTypeID();
    CFTypeID CFNotificationCenterGetTypeID();
    CFTypeID CFNumberFormatterGetTypeID();
    CFTypeID _CFKeyedArchiverUIDGetTypeID();
    int _CFKeyedArchiverUIDGetValue(void *);
    CFStringRef CFStringCreateWithFormat(CFAllocatorRef, CFDictionaryRef, CFStringRef, ...);
    CFBundleRef CFBundleGetMainBundle();
    CFURLRef CFBundleCopyBundleURL(CFBundleRef);
    CFURLRef CFURLCreateCopyDeletingLastPathComponent(CFAllocatorRef, CFURLRef);
    void *CFURLGetFileSystemRepresentation(CFURLRef, void *, uint8_t *, CFIndex);
    CFRange CFRangeMake(CFIndex loc, CFIndex len);

    Boolean CFEqual(CFTypeRef cf1, CFTypeRef cf2  );
    Boolean CFStringHasSuffix( CFStringRef theString, CFStringRef suffix );
    CFStringRef CFStringCreateCopy( CFAllocatorRef alloc, CFStringRef theString );
    CFDataRef CFStringCreateExternalRepresentation(CFAllocatorRef alloc, CFStringRef theString, CFStringEncoding encoding,UInt8 lossByte);
    void CFShow(CFTypeRef obj);
    void CFDictionaryApplyFunction(        CFDictionaryRef theDict,        void* applier,        void* context    );

#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //iTunesMobileDevice.dll
    int USBMuxConnectByPort(int, short, void *);
    int AMSUnregisterTarget(void *);
    int AMDServiceConnectionInvalidate(void *);

    int AFCConnectionClose(AFCRef afc);
    int AFCDeviceInfoOpen(void *, PPV);
    unsigned int AFCFileInfoOpen(AFCRef afc, const char* path, afc_dictionary** fileinfo);
    int AFCKeyValueRead(afc_dictionary* dict, char** key, char** value);
    int AFCKeyValueClose(afc_dictionary* dic);
    int AFCDirectoryOpen(void *, void *, PPV);
    int AFCDirectoryRead(void *, void *, PPV);
    int AFCDirectoryClose(void *, void *);
    int AFCDirectoryCreate(void *, void *);
    int AFCRemovePath(AFCRef afc, const char* path);
    int AFCRenamePath(AFCRef afc, const char* path, const char* new_path);
    int AFCFileRefOpen(AFCRef afc, const char* path, unsigned __int64 mode, AFCFileRef* file);
    int AFCFileRefRead(AFCRef afc, AFCFileRef file, void* buf, size_t * plen);
    int AFCFileRefWrite(AFCRef afc, AFCFileRef file, void* buf, size_t len);
    int AFCFileRefClose(AFCRef afc, AFCFileRef file);
    int AFCFileRefSeek(void *, unsigned long long, unsigned long long, unsigned long);
    int AFCFileRefTell(void *, unsigned long long, unsigned long long *);
    int AFCConnectionOpen(void* socketHandle, unsigned int io_timeout, AFCRef* afc);
    int AFCConnectionGetContext(void *);
    int AFCConnectionGetFSBlockSize(void *);
    int AFCConnectionGetIOTimeout(void *);
    int AFCConnectionGetSocketBlockSize(void *);

    int AMDeviceLookupApplications(void *, void *, PPV);
    int AMDeviceStartHouseArrestService(void *, void *, void *, void *, void *);
    int AMDeviceInstallApplication(void *, CFStringRef, void *, void *, void *);
    int AMDeviceUninstallApplication(void *, CFStringRef, void *, void *, void *);
    int AMDeviceTransferApplication(void * , CFStringRef path, CFDictionaryRef options, void * callbackj, void* user);

    int AMDeviceRemoveApplicationArchive(void *, CFStringRef, void *, void *, void *);
    int AMDeviceArchiveApplication(void *, CFStringRef, void *, void *, void *);
    int AMDeviceNotificationSubscribe(void *, int, int, int, PPV);
    int AMDeviceNotificationUnsubscribe(void *);
    int AMDeviceRelease(void *);
    int AMDeviceConnect(void *);
    int AMDeviceDisconnect(void *);
    int AMDeviceIsPaired(void *);
    int AMDeviceValidatePairing(void *);
    int AMDevicePair(void *);
    int AMDevicePairWithOptions(void* r1, CFDictionaryRef r2);
    int AMDeviceUnpair(void *);
    int AMDeviceStartSession(void *);
    int AMDeviceSecureStartService(void* device, CFStringRef serviceName, int, void** serviceHandle);
    int AMDeviceStartService(void* device, CFStringRef service_name, void** serviceHandle, unsigned int* unknown);
    int AMDeviceStopSession(void *);
    int AMDeviceSetValue(void *, void *, void *, void *);
    void *AMDeviceCopyDeviceIdentifier(void *);
    void *AMDeviceCopyValue(void *, void *, void *);
    int AMDeviceGetInterfaceType(void *);
    int AMDeviceGetConnectionID(void *);
    int AMDeviceEnterRecovery(void *);
    int AMDeviceRetain(void *);
    int AMDeviceDeactivate(void *);
    int AMDeviceActivate(void *, void *);
    int AMDeviceRemoveValue(void *, int, void *);
    int AMDeviceStartServiceWithOptions(void *, CFStringRef, void *, int *);

    int AMRestoreRegisterForDeviceNotifications(void *, void *, void *, void *, int, void *);
    int AMRestorePerformRecoveryModeRestore(void *, void *, void *, void *);
    int AMRestorePerformDFURestore(void *, void *, void *, void *);
    void AMRestoreUnregisterForDeviceNotifications();
    int AMSRestoreWithApplications(void *, void *, void *, void *, void *, void *, void *);
    int AMRestoreModeDeviceReboot(void *);
    int AMRestoreEnableFileLogging(char *);
    int AMRestoreDisableFileLogging();
    void *AMRestoreCreateDefaultOptions(void *);
    int AMRestorePerformRestoreModeRestore(void *, void *, void *, void *);
    void *AMRestoreModeDeviceCreate(int, int, int);
    int AMRestoreCreatePathsForBundle(void *, void *, void *, int, PPV, PPV, int, PPV);
    unsigned long AMRestoreModeDeviceGetTypeID(void *);
    void *AMRestoreModeDeviceCopySerialNumber(void *);

    int AMRecoveryModeDeviceSendFileToDevice(void *, CFStringRef);
    int AMRecoveryModeDeviceSendCommandToDevice(void *, CFStringRef);
    uint16_t AMRecoveryModeDeviceGetProductID(void *);
    unsigned long AMRecoveryModeDeviceGetProductType(void *);
    unsigned long AMRecoveryModeDeviceGetChipID(void *);
    uint64_t AMRecoveryModeDeviceGetECID(void *);
    unsigned long AMRecoveryModeDeviceGetLocationID(void *);
    unsigned long AMRecoveryModeDeviceGetBoardID(void *);
    unsigned char AMRecoveryModeDeviceGetProductionMode(void *);
    unsigned long AMRecoveryModeDeviceGetTypeID(void *);
    void *AMRecoveryModeGetSoftwareBuildVersion(void *);
    int AMRecoveryModeDeviceSetAutoBoot(void *, unsigned char);
    int AMRecoveryModeDeviceReboot(void *);
    void *AMRecoveryModeDeviceCopySerialNumber(void *);

    uint16_t AMDFUModeDeviceGetProductID(void *);
    unsigned long AMDFUModeDeviceGetProductType(void *);
    unsigned long AMDFUModeDeviceGetChipID(void *);
    uint64_t AMDFUModeDeviceGetECID(void *);
    unsigned long AMDFUModeDeviceGetLocationID(void *);
    unsigned long AMDFUModeDeviceGetBoardID(void *);
    unsigned char AMDFUModeDeviceGetProductionMode(void *);
    unsigned long AMDFUModeDeviceGetTypeID(void *);

    int AMRestorableDeviceRegisterForNotificationsForDevices(am_recovery_device_notification_callback, void *, unsigned int, void *, void *);
    int AMRestorableDeviceRestore(struct am_restore_device *, CFDictionaryRef, void *, void *);
    int AMRestorableDeviceGetState(void *);
    void *AMRestorableDeviceCopyDFUModeDevice(void *);
    void *AMRestorableDeviceCopyRecoveryModeDevice(void *);
    void *AMRestorableDeviceCopyAMDevice(void *);
    void *AMRestorableDeviceCreateFromAMDevice(void *);
    uint16_t AMRestorableDeviceGetProductID(void *);
    unsigned long AMRestorableDeviceGetProductType(void *);
    unsigned long AMRestorableDeviceGetChipID(void *);
    uint64_t AMRestorableDeviceGetECID(void *);
    unsigned long AMRestorableDeviceGetLocationID(void *);
    unsigned long AMRestorableDeviceGetBoardID(void *);
    void *AMRestorableDeviceCopySerialNumber(void *);

    int AMDShutdownNotificationProxy(void *);
    int USBMuxListenerCreate(void *, PPV);
    int USBMuxListenerHandleData(void *);
    int AMDObserveNotification(void *, void *);
    int AMSInitialize();
    int AMDListenForNotifications(void *, void *, void *);
    void *AMDServiceConnectionCreate(CFStringRef, void *, void *);
    void *AMDServiceConnectionGetSocket(void *);
    int AMDServiceConnectionGetSecureIOContext(void *);
    int AMDServiceConnectionReceive(void *, void *, int);
    int AMDServiceConnectionSend(void *, void *, int);
    int AMDServiceConnectionReceiveMessage(void *, CFDictionaryRef *, int *);
    int AMDServiceConnectionSendMessage(void *, CFDictionaryRef, int);
    int AMSChangeBackupPassword(CFStringRef, CFStringRef, CFStringRef, int *);
    int AMSBackupWithOptions(CFStringRef, CFStringRef, CFStringRef, CFDictionaryRef, BackupCallBack, int);
    int AMSCancelBackupRestore(void *);
    CFStringRef AMSGetErrorReasonForErrorCode(int);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //AirTrafficHost.dll
    void *ATCFMessageGetParam(void *, void *);
    int ATHostConnectionGetCurrentSessionNumber(ATHRef);
    void ATHostConnectionSendFileProgress(void *, void *, void *, double, int, int);
    CFDictionaryRef ATCFMessageCreate(uint32_t sesssion, CFStringRef messageType, CFDictionaryRef params);
    ATHRef ATHostConnectionCreateWithLibrary(CFStringRef library, CFStringRef udid, CFStringRef athexe);
    int ATHostConnectionCreate(void *);
    void ATHostConnectionSendPing(ATHRef);
    void ATHostConnectionSendAssetMetricsRequest(void *, int);
    void ATHostConnectionInvalidate(void *);
    void ATHostConnectionClose(ATHRef);
    void ATHostConnectionRelease(ATHRef);
    int ATHostConnectionSendPowerAssertion(ATHRef, CFBooleanRef enable);
    int ATHostConnectionRetain(ATHRef);
    int ATHostConnectionSendMetadataSyncFinished(ATHRef, CFDictionaryRef, CFDictionaryRef);
    void ATHostConnectionSendFileError(void *, void *, void *, int);
    int ATHostConnectionSendAssetCompleted(ATHRef, void *, void *, void *);
    CFStringRef ATCFMessageGetName(ATHRef);
    int ATHostConnectionSendHostInfo(ATHRef, CFDictionaryRef);
    int ATHostConnectionSendSyncRequest(ATHRef, CFArrayRef, CFDictionaryRef, CFDictionaryRef);
    int ATHostConnectionSendMessage(ATHRef, CFDictionaryRef);
    int ATHostConnectionGetGrappaSessionId(ATHRef);
    void *ATHostConnectionReadMessage(ATHRef);
    void ATHostConnectionDestroy(ATHRef);
#ifdef __cplusplus
};
#endif