#include "apple_import.h"
#include "abi/ios_authorize/itunes_module.h"
#include "ABI/ios_authorize/apple_device_auth.h"

am_device *deviceHandleConnected = NULL;
//struct afc_connection *iPodAFC;
//struct afc_connection *iPodConnection;
bool found_device = false;
char *device_id = NULL;
CFStringRef last_path = NULL;

void (*CFRelease)(CFTypeRef cf);
CFStringRef (*CFStringCreateWithCString)(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
CFIndex (*CFStringGetLength)(CFStringRef theString);
Boolean (*CFStringGetCString)(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding);
Boolean (*CFNumberGetValue)(CFNumberRef number, CFNumberType theType, void *valuePtr);
CFIndex (*CFDataGetLength)(CFDataRef theData);
void (*CFDataGetBytes)(CFDataRef theData, CFRange range, unsigned char *buffer);
CFMutableArrayRef (*CFArrayCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, const void *callBacks);
void (*CFArrayAppendValue)(CFMutableArrayRef theArray, const void *value);
CFMutableDictionaryRef (*CFDictionaryCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, const void *keyCallBacks, const void *valueCallBacks);
void (*CFDictionaryAddValue)(CFMutableDictionaryRef theDict, const void *key, const void *value);
const void *(*CFDictionaryGetValue)(CFDictionaryRef theDict, const void *key);
const void *(*CFDictionarySetValue)(CFDictionaryRef theDict, const void *key, const void *value);
CFTypeID (*CFGetTypeID)(CFTypeRef cf);
CFTypeID (*CFArrayGetTypeID)(void);
CFMutableArrayRef (*CFArrayCreateMutableCopy)(CFAllocatorRef allocator, CFIndex capacity, CFArrayRef theArray);
CFIndex (*CFArrayGetCount)(CFArrayRef theArray);
const void *(*CFArrayGetValueAtIndex)(CFArrayRef theArray, CFIndex idx);
CFTypeID (*CFStringGetTypeID)(void);
void (*CFArrayRemoveValueAtIndex)(CFMutableArrayRef theArray, CFIndex idx);
CFNumberRef (*CFNumberCreate)(CFAllocatorRef alloc, CFNumberType theType, const void *valuePtr);

CFTypeID (*CFDataGetTypeID)(void);
CFTypeID (*CFDictionaryGetTypeID)(void);
CFDataRef (*CFPropertyListCreateXMLData)(CFAllocatorRef allocator, CFPropertyListRef propertyList);
const unsigned char *(*CFDataGetBytePtr)(CFDataRef theData);

afc_error_t (*AFCDirectoryOpen)(AFCRef conn, char *path, struct afc_directory **dir);
afc_error_t (*AFCDirectoryRead)(AFCRef conn,struct afc_directory *dir,char **dirent);
afc_error_t (*AFCDirectoryClose)(AFCRef conn,struct afc_directory *dir);

CFURLRef (*CFURLCreateWithFileSystemPath)(CFAllocatorRef Allocator, CFStringRef Path, unsigned int a, Boolean Boole);
CFURLRef (*CFURLCopyAbsoluteURL)(CFURLRef url);
int (*CFEqual)(CFStringRef,CFStringRef);
int (*CFStringHasSuffix)(CFStringRef,CFStringRef);
CFStringRef (*CFStringCreateCopy)(int,CFStringRef);
CFStringRef(*CFDictionaryCreate)(int,CFStringRef*,CFStringRef*,int,void*,void*);
void (*CFRetain)(void *device);

int (*CFUUIDCreate)(int UN);
CFStringRef (*CFStringMakeConstantString)(char* str);
CFStringRef (*CFStringCreateWithFormatAndArguments)(int UN,int UN2,CFStringRef str,void* Key);
CFBooleanRef *pkCFBooleanTrue;
CFBooleanRef *pkCFBooleanFalse;
void *pkCFTypeArrayCallBacks;
void *pkCFTypeDictionaryKeyCallBacks;
void *pkCFTypeDictionaryValueCallBacks;

// iTunesMobileDevice functions
//
typedef void(*ptransfer_callback)(CFDictionaryRef dict, int arg);
typedef void(*pinstall_callback)(CFDictionaryRef dict, int arg);

int (*AMDeviceNotificationSubscribe)(void *, int , int , int, void **);
int (*AMDeviceConnect)(void *device);
int (*AMDeviceIsPaired)(void *device);
int (*AMDeviceValidatePairing)(void *device);
int (*AMDevicePair)(void *device);
int (*AMDevicePairWithOptions)(void *device, CFDictionaryRef dictOptions);
int (*AMDeviceStartSession)(void *device);
int (*AMDeviceStopSession)(void *device);
int (*AMDeviceDisconnect)(void *device);
//int (*AMDeviceSecureStartService)(void *device, CFStringRef serviceName, int, void **serviceHandle);
int (*AMDeviceStartService)(void *device, CFStringRef service_name, void **serviceHandle, unsigned int *unknown);
int (*AMDeviceTransferApplication)(void *serviceHandle,CFStringRef path,int,ptransfer_callback,int);
int (*AMDeviceInstallApplication)(void *serviceHandle,CFStringRef path,CFDictionaryRef options,pinstall_callback,int);
CFStringRef (*AMDeviceCopyDeviceIdentifier)(void *device);
CFStringRef (*AMDeviceCopyValue)(void *device, CFStringRef domain, CFStringRef name);
void (*AMDeviceSetValue)(void *device, CFStringRef domain, CFStringRef name, CFTypeRef value);
void (*AMDeviceRemoveValue)(void *device, CFStringRef domain, CFStringRef name);

unsigned int (*AFCConnectionOpen)(void *socketHandle, unsigned int io_timeout, AFCRef *afc);
unsigned int (*AFCConnectionClose)(AFCRef afc);
unsigned int (*AFCRemovePath)(AFCRef afc, const char *path);
unsigned int (*AFCFileInfoOpen)(AFCRef afc, const char *path, afc_dictionary **fileinfo);
unsigned int (*AFCKeyValueRead)(afc_dictionary *dict, char **key, char **value);
unsigned int (*AFCKeyValueClose)(afc_dictionary *dic);

unsigned int (*AFCFileRefOpen)(AFCRef afc, const char *path, unsigned __int64 mode, AFCFileRef *file);
unsigned int (*AFCFileRefClose)(AFCRef afc, AFCFileRef file);
unsigned int (*AFCFileRefRead)(AFCRef afc, AFCFileRef file, void *buf, unsigned int *plen);
unsigned int (*AFCFileRefWrite)(AFCRef afc, AFCFileRef file, void *buf, unsigned int len);

// AirTrafficHost.dll functions
//
ATHRef (*ATHostConnectionCreateWithLibrary)(CFStringRef library, CFStringRef udid, int);
void (*ATHostConnectionDestroy)(ATHRef);
int (*ATHostConnectionSendPowerAssertion)(ATHRef, CFBooleanRef enable);
void (*ATHostConnectionRetain)(ATHRef);
void (*ATHostConnectionRelease)(ATHRef);
CFDictionaryRef (*ATHostConnectionReadMessage)(ATHRef);
int (*ATHostConnectionSendSyncRequest)(ATHRef, CFArrayRef, CFDictionaryRef, CFDictionaryRef);
int (*ATHostConnectionSendPing)(ATHRef);
unsigned (*ATHostConnectionGetGrappaSessionId)(ATHRef);
int (*ATHostConnectionSendMetadataSyncFinished)(ATHRef, CFDictionaryRef, CFDictionaryRef);

CFRange CFRangeMake(CFIndex loc, CFIndex len)
{
	CFRange range;
	range.location = loc;
	range.length = len;
	return range;
}

void LoadFunction(HINSTANCE hDll, LPCSTR sFunc, FARPROC *func)
{
	*func = GetProcAddress(hDll, sFunc);
	if (!func){
		ABI::internal::message("Error in resolve function %s().\n", sFunc);
	}
}
am_device* GetConnectionHandle(){
	return deviceHandleConnected;
}
bool LoadDlls()
{
	ABI::iTunes::iTunesModule itunes_module;
	HINSTANCE hCoreFoundationDll = LoadLibrary(itunes_module.core_foundation_dll().c_str());
	if (hCoreFoundationDll == NULL){
		ABI::internal::message("Error in loading CoreFoundation.dll\n");
		return false;
	}

	HINSTANCE hITunesMobileDeviceDll = LoadLibrary(itunes_module.itunes_mobile_device_dll().c_str());
	if (hITunesMobileDeviceDll == NULL){
		ABI::internal::message("Error in loading iTunesMobileDevice.dll\n");
		return false;
	}

	HINSTANCE hAirTrafficHostDll = LoadLibrary(itunes_module.air_traffic_host_dll().c_str());
	if (hAirTrafficHostDll == NULL){
		ABI::internal::message("Error in loading AirTrafficHost.dll\n");
		return false;
	}
	
	HINSTANCE hITunesDll = LoadLibrary(itunes_module.iTunesDll(L"iTunes.dll").c_str());
	if (hITunesDll == NULL){
		ABI::internal::message("Error in loading ITunes.dll\n");
		return false;
	}

	SetDllDirectory(NULL);

	LoadFunction(hCoreFoundationDll, "CFRelease", (FARPROC *)&CFRelease);
	LoadFunction(hCoreFoundationDll, "CFStringCreateWithCString", (FARPROC *)&CFStringCreateWithCString);
	LoadFunction(hCoreFoundationDll, "CFStringGetLength", (FARPROC *)&CFStringGetLength);
	LoadFunction(hCoreFoundationDll, "CFStringGetCString", (FARPROC *)&CFStringGetCString);
	LoadFunction(hCoreFoundationDll, "CFNumberGetValue", (FARPROC *)&CFNumberGetValue);
	LoadFunction(hCoreFoundationDll, "CFDataGetLength", (FARPROC *)&CFDataGetLength);
	LoadFunction(hCoreFoundationDll, "CFDataGetBytes", (FARPROC *)&CFDataGetBytes);
	LoadFunction(hCoreFoundationDll, "CFArrayCreateMutable", (FARPROC *)&CFArrayCreateMutable);
	LoadFunction(hCoreFoundationDll, "CFArrayAppendValue", (FARPROC *)&CFArrayAppendValue);
	LoadFunction(hCoreFoundationDll, "CFDictionaryCreateMutable", (FARPROC *)&CFDictionaryCreateMutable);
	LoadFunction(hCoreFoundationDll, "CFDictionaryAddValue", (FARPROC *)&CFDictionaryAddValue);
	LoadFunction(hCoreFoundationDll, "CFDictionaryGetValue", (FARPROC *)&CFDictionaryGetValue);
	LoadFunction(hCoreFoundationDll, "CFDictionarySetValue", (FARPROC *)&CFDictionarySetValue);
	LoadFunction(hCoreFoundationDll, "CFGetTypeID", (FARPROC *)&CFGetTypeID);
	LoadFunction(hCoreFoundationDll, "CFArrayGetTypeID", (FARPROC *)&CFArrayGetTypeID);
	LoadFunction(hCoreFoundationDll, "CFArrayCreateMutableCopy", (FARPROC *)&CFArrayCreateMutableCopy);
	LoadFunction(hCoreFoundationDll, "CFArrayGetCount", (FARPROC *)&CFArrayGetCount);
	LoadFunction(hCoreFoundationDll, "CFArrayGetValueAtIndex", (FARPROC *)&CFArrayGetValueAtIndex);
	LoadFunction(hCoreFoundationDll, "CFStringGetTypeID", (FARPROC *)&CFStringGetTypeID);
	LoadFunction(hCoreFoundationDll, "CFArrayRemoveValueAtIndex", (FARPROC *)&CFArrayRemoveValueAtIndex);
	LoadFunction(hCoreFoundationDll, "CFNumberCreate", (FARPROC *)&CFNumberCreate);

	LoadFunction(hCoreFoundationDll, "CFDataGetTypeID", (FARPROC *)&CFDataGetTypeID);
	LoadFunction(hCoreFoundationDll, "CFDictionaryGetTypeID", (FARPROC *)&CFDictionaryGetTypeID);
	LoadFunction(hCoreFoundationDll, "CFPropertyListCreateXMLData", (FARPROC *)&CFPropertyListCreateXMLData);
	LoadFunction(hCoreFoundationDll, "CFDataGetBytePtr", (FARPROC *)&CFDataGetBytePtr);

	LoadFunction(hCoreFoundationDll, "kCFBooleanTrue", (FARPROC *)&pkCFBooleanTrue);
	LoadFunction(hCoreFoundationDll, "kCFBooleanFalse", (FARPROC *)&pkCFBooleanFalse);
	LoadFunction(hCoreFoundationDll, "kCFTypeArrayCallBacks", (FARPROC *)&pkCFTypeArrayCallBacks);
	LoadFunction(hCoreFoundationDll, "kCFTypeDictionaryKeyCallBacks", (FARPROC *)&pkCFTypeDictionaryKeyCallBacks);
	LoadFunction(hCoreFoundationDll, "kCFTypeDictionaryValueCallBacks", (FARPROC *)&pkCFTypeDictionaryValueCallBacks);

	LoadFunction(hCoreFoundationDll, "CFUUIDCreate", (FARPROC *)&CFUUIDCreate);
	LoadFunction(hCoreFoundationDll, "__CFStringMakeConstantString", (FARPROC *)&CFStringMakeConstantString);
	LoadFunction(hCoreFoundationDll, "CFStringCreateWithFormatAndArguments", (FARPROC *)&CFStringCreateWithFormatAndArguments);
	LoadFunction(hCoreFoundationDll, "CFURLCreateWithFileSystemPath", (FARPROC *)&CFURLCreateWithFileSystemPath);
	LoadFunction(hCoreFoundationDll, "CFURLCopyAbsoluteURL", (FARPROC *)&CFURLCopyAbsoluteURL);
	LoadFunction(hCoreFoundationDll, "CFEqual", (FARPROC *)&CFEqual);
	LoadFunction(hCoreFoundationDll, "CFStringHasSuffix", (FARPROC *)&CFStringHasSuffix);
	LoadFunction(hCoreFoundationDll, "CFStringCreateCopy", (FARPROC *)&CFStringCreateCopy);
	LoadFunction(hCoreFoundationDll, "CFDictionaryCreate", (FARPROC *)&CFDictionaryCreate);
	LoadFunction(hCoreFoundationDll, "CFRetain", (FARPROC *)&CFRetain);

	
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceNotificationSubscribe", (FARPROC *)&AMDeviceNotificationSubscribe);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceConnect", (FARPROC *)&AMDeviceConnect);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceIsPaired", (FARPROC *)&AMDeviceIsPaired);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceValidatePairing", (FARPROC *)&AMDeviceValidatePairing);
	LoadFunction(hITunesMobileDeviceDll, "AMDevicePair", (FARPROC *)&AMDevicePair);
	LoadFunction(hITunesMobileDeviceDll, "AMDevicePairWithOptions", (FARPROC *)&AMDevicePairWithOptions);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceStartSession", (FARPROC *)&AMDeviceStartSession);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceStopSession", (FARPROC *)&AMDeviceStopSession);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceDisconnect", (FARPROC *)&AMDeviceDisconnect);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceStartService", (FARPROC *)&AMDeviceStartService);

	LoadFunction(hITunesMobileDeviceDll, "AMDeviceCopyDeviceIdentifier", (FARPROC *)&AMDeviceCopyDeviceIdentifier);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceCopyValue", (FARPROC *)&AMDeviceCopyValue);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceSetValue", (FARPROC *)&AMDeviceSetValue);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceRemoveValue", (FARPROC *)&AMDeviceRemoveValue);

	LoadFunction(hITunesMobileDeviceDll, "AMDeviceTransferApplication", (FARPROC *)&AMDeviceTransferApplication);
	LoadFunction(hITunesMobileDeviceDll, "AMDeviceInstallApplication", (FARPROC *)&AMDeviceInstallApplication);

	LoadFunction(hITunesMobileDeviceDll, "AFCConnectionOpen", (FARPROC *)&AFCConnectionOpen);
	LoadFunction(hITunesMobileDeviceDll, "AFCConnectionClose", (FARPROC *)&AFCConnectionClose);
	LoadFunction(hITunesMobileDeviceDll, "AFCRemovePath", (FARPROC *)&AFCRemovePath);
	LoadFunction(hITunesMobileDeviceDll, "AFCFileInfoOpen", (FARPROC *)&AFCFileInfoOpen);
	LoadFunction(hITunesMobileDeviceDll, "AFCKeyValueRead", (FARPROC *)&AFCKeyValueRead);
	LoadFunction(hITunesMobileDeviceDll, "AFCKeyValueClose", (FARPROC *)&AFCKeyValueClose);

	LoadFunction(hITunesMobileDeviceDll, "AFCFileRefOpen", (FARPROC *)&AFCFileRefOpen);
	LoadFunction(hITunesMobileDeviceDll, "AFCFileRefClose", (FARPROC *)&AFCFileRefClose);
	LoadFunction(hITunesMobileDeviceDll, "AFCFileRefRead", (FARPROC *)&AFCFileRefRead);
	LoadFunction(hITunesMobileDeviceDll, "AFCFileRefWrite", (FARPROC *)&AFCFileRefWrite);

	//AFCDirectoryOpen
	LoadFunction(hITunesMobileDeviceDll, "AFCDirectoryOpen", (FARPROC *)&AFCDirectoryOpen);
	LoadFunction(hITunesMobileDeviceDll, "AFCDirectoryRead", (FARPROC *)&AFCDirectoryRead);
	LoadFunction(hITunesMobileDeviceDll, "AFCDirectoryClose", (FARPROC *)&AFCDirectoryClose);
	
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionCreateWithLibrary", (FARPROC *)&ATHostConnectionCreateWithLibrary);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionDestroy", (FARPROC *)&ATHostConnectionDestroy);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionSendPowerAssertion", (FARPROC *)&ATHostConnectionSendPowerAssertion);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionRetain", (FARPROC *)&ATHostConnectionRetain);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionRelease", (FARPROC *)&ATHostConnectionRelease);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionReadMessage", (FARPROC *)&ATHostConnectionReadMessage);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionSendSyncRequest", (FARPROC *)&ATHostConnectionSendSyncRequest);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionSendPing", (FARPROC *)&ATHostConnectionSendPing);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionGetGrappaSessionId", (FARPROC *)&ATHostConnectionGetGrappaSessionId);
	LoadFunction(hAirTrafficHostDll, "ATHostConnectionSendMetadataSyncFinished", (FARPROC *)&ATHostConnectionSendMetadataSyncFinished);
	return true;
}
