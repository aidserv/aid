#ifndef ABI_IOS_AUTHORIZE_APPLE_IMPORT_H_
#define ABI_IOS_AUTHORIZE_APPLE_IMPORT_H_

#include "ios_cracker/atl_dll_main.h"
#include "ABI/ios_authorize/MobileDevice.h"

#ifdef __cplusplus
extern "C"{
#endif
// Core foundation functions
//
typedef const void * CFTypeRef;
typedef const void * CFAllocatorRef;
typedef const void * CFBooleanRef;
typedef const void * CFNumberRef;
typedef const void * CFStringRef;
typedef const void * CFDataRef;
typedef const void * CFArrayRef;
typedef const void * CFMutableArrayRef;
typedef const void * CFDictionaryRef;
typedef const void * CFMutableDictionaryRef;
typedef const void * CFPropertyListRef;
typedef const void * CFURLRef;
typedef const void * AFCRef;
typedef unsigned __int64 AFCFileRef;
typedef const void * ATHRef;

typedef unsigned char Boolean;
typedef signed long CFIndex;
typedef CFIndex CFNumberType;
typedef unsigned int CFStringEncoding;
typedef unsigned long CFTypeID;

typedef struct am_device * AMDeviceRef;


#define kCFStringEncodingUTF8 0x08000100
#define kCFNumberSInt32Type 3
#define AFC_FILEMODE_READ 1
#define AFC_FILEMODE_WRITE 2


typedef struct{
	CFIndex location;
	CFIndex length;
} CFRange;

struct AMDeviceNotificationCallbackInformation {
	am_device *deviceHandle;
	unsigned int msgType;
} ;

extern am_device *deviceHandleConnected;
extern bool found_device;
extern char *device_id;
extern CFStringRef last_path;
WIN_DLL_API extern am_device* GetConnectionHandle();
WIN_DLL_API extern void (*CFRelease)(CFTypeRef cf);
WIN_DLL_API extern CFStringRef (*CFStringCreateWithCString)(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
WIN_DLL_API extern CFIndex (*CFStringGetLength)(CFStringRef theString);
WIN_DLL_API extern Boolean (*CFStringGetCString)(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding);
WIN_DLL_API extern Boolean (*CFNumberGetValue)(CFNumberRef number, CFNumberType theType, void *valuePtr);
WIN_DLL_API extern CFIndex (*CFDataGetLength)(CFDataRef theData);
WIN_DLL_API extern void (*CFDataGetBytes)(CFDataRef theData, CFRange range, unsigned char *buffer);
WIN_DLL_API extern CFMutableArrayRef (*CFArrayCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, const void *callBacks);
WIN_DLL_API extern void (*CFArrayAppendValue)(CFMutableArrayRef theArray, const void *value);
WIN_DLL_API extern CFMutableDictionaryRef (*CFDictionaryCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, const void *keyCallBacks, const void *valueCallBacks);
WIN_DLL_API extern void (*CFDictionaryAddValue)(CFMutableDictionaryRef theDict, const void *key, const void *value);
WIN_DLL_API extern const void *(*CFDictionaryGetValue)(CFDictionaryRef theDict, const void *key);
WIN_DLL_API extern const void *(*CFDictionarySetValue)(CFDictionaryRef theDict, const void *key, const void *value);
WIN_DLL_API extern CFTypeID (*CFGetTypeID)(CFTypeRef cf);
WIN_DLL_API extern CFTypeID (*CFArrayGetTypeID)(void);
WIN_DLL_API extern CFMutableArrayRef (*CFArrayCreateMutableCopy)(CFAllocatorRef allocator, CFIndex capacity, CFArrayRef theArray);
WIN_DLL_API extern CFIndex (*CFArrayGetCount)(CFArrayRef theArray);
WIN_DLL_API extern const void *(*CFArrayGetValueAtIndex)(CFArrayRef theArray, CFIndex idx);
WIN_DLL_API extern CFTypeID (*CFStringGetTypeID)(void);
WIN_DLL_API extern void (*CFArrayRemoveValueAtIndex)(CFMutableArrayRef theArray, CFIndex idx);
WIN_DLL_API extern CFNumberRef (*CFNumberCreate)(CFAllocatorRef alloc, CFNumberType theType, const void *valuePtr);

WIN_DLL_API extern CFTypeID (*CFDataGetTypeID)(void);
WIN_DLL_API extern CFTypeID (*CFDictionaryGetTypeID)(void);
WIN_DLL_API extern CFDataRef (*CFPropertyListCreateXMLData)(CFAllocatorRef allocator, CFPropertyListRef propertyList);
WIN_DLL_API extern const unsigned char *(*CFDataGetBytePtr)(CFDataRef theData);

WIN_DLL_API extern afc_error_t (*AFCDirectoryOpen)(AFCRef conn, char *path, struct afc_directory **dir);
WIN_DLL_API extern afc_error_t (*AFCDirectoryRead)(AFCRef conn,struct afc_directory *dir,char **dirent);
WIN_DLL_API extern afc_error_t (*AFCDirectoryClose)(AFCRef conn,struct afc_directory *dir);

WIN_DLL_API extern CFURLRef (*CFURLCreateWithFileSystemPath)(CFAllocatorRef Allocator, CFStringRef Path, unsigned int a, Boolean Boole);
WIN_DLL_API extern CFURLRef (*CFURLCopyAbsoluteURL)(CFURLRef url);
WIN_DLL_API extern int (*CFEqual)(CFStringRef,CFStringRef);
WIN_DLL_API extern int (*CFStringHasSuffix)(CFStringRef,CFStringRef);
WIN_DLL_API extern CFStringRef (*CFStringCreateCopy)(int,CFStringRef);
WIN_DLL_API extern CFStringRef(*CFDictionaryCreate)(int,CFStringRef*,CFStringRef*,int,void*,void*);
WIN_DLL_API extern void (*CFRetain)(void *device);

WIN_DLL_API extern int (*CFUUIDCreate)(int UN);
WIN_DLL_API extern CFStringRef (*CFStringMakeConstantString)(char* str);
WIN_DLL_API extern CFStringRef (*CFStringCreateWithFormatAndArguments)(int UN,int UN2,CFStringRef str,void* Key);
extern CFBooleanRef *pkCFBooleanTrue;
extern CFBooleanRef *pkCFBooleanFalse;
extern void *pkCFTypeArrayCallBacks;
extern void *pkCFTypeDictionaryKeyCallBacks;
extern void *pkCFTypeDictionaryValueCallBacks;

// iTunesMobileDevice functions
//
typedef void(*ptransfer_callback)(CFDictionaryRef dict, int arg);
typedef void(*pinstall_callback)(CFDictionaryRef dict, int arg);

WIN_DLL_API extern int (*AMDeviceNotificationSubscribe)(void *, int , int , int, void **);
WIN_DLL_API extern int (*AMDeviceConnect)(void *device);
WIN_DLL_API extern int (*AMDeviceIsPaired)(void *device);
WIN_DLL_API extern int (*AMDeviceValidatePairing)(void *device);
WIN_DLL_API extern int (*AMDevicePair)(void *device);
WIN_DLL_API extern int (*AMDevicePairWithOptions)(void *device, CFDictionaryRef dictOptions);
WIN_DLL_API extern int (*AMDeviceStartSession)(void *device);
WIN_DLL_API extern int (*AMDeviceStopSession)(void *device);
WIN_DLL_API extern int (*AMDeviceDisconnect)(void *device);
WIN_DLL_API extern int (*AMDeviceSecureStartService)(void *device, CFStringRef serviceName, int, void **serviceHandle);
WIN_DLL_API extern int (*AMDeviceStartService)(void *device, CFStringRef service_name, void **serviceHandle, unsigned int *unknown);
WIN_DLL_API extern int (*AMDeviceTransferApplication)(void *serviceHandle,CFStringRef path,int,ptransfer_callback,int);
WIN_DLL_API extern int (*AMDeviceInstallApplication)(void *serviceHandle,CFStringRef path,CFDictionaryRef options,pinstall_callback,int);
WIN_DLL_API extern CFStringRef (*AMDeviceCopyDeviceIdentifier)(void *device);
WIN_DLL_API extern CFStringRef (*AMDeviceCopyValue)(void *device, CFStringRef domain, CFStringRef name);
WIN_DLL_API extern void (*AMDeviceSetValue)(void *device, CFStringRef domain, CFStringRef name, CFTypeRef value);
WIN_DLL_API extern void (*AMDeviceRemoveValue)(void *device, CFStringRef domain, CFStringRef name);

WIN_DLL_API extern unsigned int (*AFCConnectionOpen)(void *socketHandle, unsigned int io_timeout, AFCRef *afc);
WIN_DLL_API extern unsigned int (*AFCConnectionClose)(AFCRef afc);
WIN_DLL_API extern unsigned int (*AFCRemovePath)(AFCRef afc, const char *path);
WIN_DLL_API extern unsigned int (*AFCFileInfoOpen)(AFCRef afc, const char *path, afc_dictionary **fileinfo);
WIN_DLL_API extern unsigned int (*AFCKeyValueRead)(afc_dictionary *dict, char **key, char **value);
WIN_DLL_API extern unsigned int (*AFCKeyValueClose)(afc_dictionary *dic);

WIN_DLL_API extern unsigned int (*AFCFileRefOpen)(AFCRef afc, const char *path, unsigned __int64 mode, AFCFileRef *file);
WIN_DLL_API extern unsigned int (*AFCFileRefClose)(AFCRef afc, AFCFileRef file);
WIN_DLL_API extern unsigned int (*AFCFileRefRead)(AFCRef afc, AFCFileRef file, void *buf, unsigned int *plen);
WIN_DLL_API extern unsigned int (*AFCFileRefWrite)(AFCRef afc, AFCFileRef file, void *buf, unsigned int len);

// AirTrafficHost.dll functions
//
WIN_DLL_API extern ATHRef (*ATHostConnectionCreateWithLibrary)(CFStringRef library, CFStringRef udid, int);
WIN_DLL_API extern void (*ATHostConnectionDestroy)(ATHRef);
WIN_DLL_API extern int (*ATHostConnectionSendPowerAssertion)(ATHRef, CFBooleanRef enable);
WIN_DLL_API extern void (*ATHostConnectionRetain)(ATHRef);
WIN_DLL_API extern void (*ATHostConnectionRelease)(ATHRef);
WIN_DLL_API extern CFDictionaryRef (*ATHostConnectionReadMessage)(ATHRef);
WIN_DLL_API extern int (*ATHostConnectionSendSyncRequest)(ATHRef, CFArrayRef, CFDictionaryRef, CFDictionaryRef);
WIN_DLL_API extern int (*ATHostConnectionSendPing)(ATHRef);
WIN_DLL_API extern unsigned (*ATHostConnectionGetGrappaSessionId)(ATHRef);
WIN_DLL_API extern int (*ATHostConnectionSendMetadataSyncFinished)(ATHRef, CFDictionaryRef, CFDictionaryRef);

CFRange CFRangeMake(CFIndex loc, CFIndex len);



bool LoadDlls();

#ifdef __cplusplus
};
#endif
#endif