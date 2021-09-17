# aid
Authorize ios device 
任何安装了IOS的设备，安装app都不需要连接到appstore去下载和购买。


# 使用iTunes原装dll 开发苹果助手功能
iTunes 用的dll 和 libimobiledevice 开发有很大不同，特别里面针对权限，登录，授权等功能不是libimobiledevice所有实现的，只是详细记录一下这些功能会用到，以供查阅。
# 环境

操作系统：Windows 10
iTunes版本：12.5.3.16 (x64)
手机型号：iPhone 6 plus 金色
手机系统版本：10.1.1

## 目前需要的DLL
```
CoreFoundation.dll
iTunesMobileDevice.dll
AirTrafficHost.dll
AVFoundationCF.dll
iTunes.dll
```
# 加载DLL

```
// 设备状态（回调函数会用到）
struct ios_connect_info {
    HANDLE hDevice;
    unsigned int state;
};

//LoadFunctions
#define LoadLibraryModule(lib_hmodule, fun_pointer, fun_type, fun_name)  \
    fun_pointer = (fun_type)GetProcAddress(lib_hmodule, fun_name); \
    if(!fun_pointer) { break;}

//FreeLibrarys
#define FreeLibraryHandle(lib_handle)  \
    if(lib_handle) FreeLibrary(lib_handle);

#define itunes_AppleApplicationSupport_reg      _T("SOFTWARE\\Apple Inc.\\Apple Application Support")
#define itunes_AppleMobileDeviceSupport_reg     _T("SOFTWARE\\Apple Inc.\\Apple Mobile Device Support")
#define itunes_iTunes_reg                       _T("SOFTWARE\\Apple Computer, Inc.\\iTunes")


typedef     int(*func_AMDServiceConnectionSend)(HANDLE conn, char* data, UINT len);
typedef     int(*func_AMDServiceConnectionReceive)(HANDLE conn, void* data, UINT len);
typedef     void(*func_AMDServiceConnectionInvalidate)(HANDLE conn);
typedef     int(*func_AMDeviceGetInterfaceType)(HANDLE handle);
typedef     HANDLE(*func_AMDeviceRetain)(HANDLE handle);
typedef     int(*func_AMDeviceNotificationSubscribe)(void(*callback)(ios_connect_info* state), UINT uint1, UINT uint2, UINT uint3, HANDLE *notify);
typedef     int(*func_AMDeviceConnect)(HANDLE handle);
typedef     HANDLE(*func_AMDeviceCopyDeviceIdentifier)(HANDLE handle);
typedef     int(*func_AMDeviceDisconnect)(HANDLE handle);
typedef     int(*func_AMDeviceIsPaired)(HANDLE handle);
typedef     int(*func_AMDeviceValidatePairing)(HANDLE handle);
typedef     int(*func_AMDeviceStartSession)(HANDLE handle);
typedef     int(*func_AMDeviceStopSession)(HANDLE handle);
typedef     int(*func_AMDeviceSetValue)(HANDLE handle, HANDLE cfstring1, HANDLE cfstring2);
typedef     CFTypeRef(*func_AMDeviceCopyValue)(HANDLE handle, HANDLE cfstring1, HANDLE cfstring2);
//cf functions
typedef     CFStringRef(*func_CFStringMakeConstantString)(const char* data);

typedef     CFIndex(*func_CFGetTypeID)(CFTypeRef data);
typedef     CFIndex(*func_CFDictionaryGetTypeID)();
typedef     CFIndex(*func_CFDictionaryGetCount)(CFDictionaryRef theDict);
typedef     void(*func_CFDictionaryGetKeysAndValues)(CFDictionaryRef theDict, CFTypeRef *keys, CFTypeRef *values);
typedef     CFIndex(*func_CFArrayGetTypeID)();
typedef     CFIndex(*func_CFArrayGetCount)(CFArrayRef theArray);
typedef     CFTypeRef(*func_CFArrayGetValueAtIndex)(CFArrayRef theArray, CFIndex idx);
typedef     CFIndex(*func_CFStringGetTypeID)();
typedef     CFIndex(*func_CFStringGetLength)(CFStringRef data);
typedef     bool(*func_CFStringGetCString)(HANDLE cfstring, char* byteString, UINT len, UINT encode);
typedef     CFBooleanRef(*func_CFBooleanGetValue)(CFBooleanRef data);
typedef     CFIndex(*func_CFBooleanGetTypeID)();
typedef     CFIndex(*func_CFNumberGetTypeID)();
typedef     CFIndex(*func_CFNumberGetValue)(CFTypeRef data, int type, void *valuePtr);
typedef     CFNumberType(*func_CFNumberGetType)(CFTypeRef data);
typedef     CFIndex(*func_CFNumberGetByteSize)(CFTypeRef data);
typedef     CFIndex(*func_CFDataGetTypeID)();
typedef     CFIndex(*func_CFDataGetLength)(CFDataRef theData);
typedef     const uint8_t*              (*func_CFDataGetBytePtr)(CFDataRef theData);
typedef     void(*func_CFDataGetBytes)(CFDataRef theData, CFRange range, uint8_t *buffer);
typedef     CFIndex(*func_CFDateGetTypeID)();
typedef     CFDateRef(*func_CFDateCreate)(CFAllocatorRef allocator, CFAbsoluteTime at);
typedef     CFAbsoluteTime(*func_CFDateGetAbsoluteTime)(CFDateRef theDate);
typedef     CFTimeInterval(*func_CFDateGetTimeIntervalSinceDate)(CFDateRef theDate, CFDateRef otherDate);
typedef     CFComparisonResult(*func_CFDateCompare)(CFDateRef theDate, CFDateRef otherDate, void *context);

typedef     CFNumberRef(*func_CFNumberCreate)(CFAllocatorRef allocator, CFNumberType theType, const void *valuePtr);


typedef     CFDataRef(*func_CFDataCreate)(CFAllocatorRef allocator, const uint8_t *bytes, CFIndex length);
typedef     CFPropertyListRef(*func_CFPropertyListCreateFromXMLData)(CFAllocatorRef allocator, CFDataRef xmlData, CFOptionFlags mutabilityOption, CFStringRef *errorString);
typedef     CFWriteStreamRef(*func_CFWriteStreamCreateWithAllocatedBuffers)(CFAllocatorRef alloc, CFAllocatorRef bufferAllocator);
typedef     bool(*func_CFWriteStreamOpen)(CFWriteStreamRef stream);
typedef     CFIndex(*func_CFPropertyListWriteToStream)(CFPropertyListRef propertyList, CFWriteStreamRef stream, CFPropertyListFormat format, CFStringRef *errorString);
typedef     CFTypeRef(*func_CFWriteStreamCopyProperty)(CFWriteStreamRef stream, CFStringRef propertyName);
typedef     void(*func_CFWriteStreamClose)(CFWriteStreamRef stream);
typedef     void(*func_CFRelease)(CFTypeRef cf);

typedef     CFMutableArrayRef(*func_CFArrayCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, CFArrayCallBacks *callBacks);
typedef     void(*func_CFArrayAppendValue)(CFMutableArrayRef theArray, CFTypeRef value);
typedef     void(*func_CFArrayAppendArray)(CFMutableArrayRef theArray, CFArrayRef otherArray, CFRange otherRange);
typedef     CFMutableDictionaryRef(*func_CFDictionaryCreateMutable)(CFAllocatorRef allocator, CFIndex capacity, CFDictionaryKeyCallBacks *keyCallBacks, CFDictionaryValueCallBacks *valueCallBacks);
typedef     void(*func_CFDictionaryAddValue)(CFMutableDictionaryRef theDict, CFTypeRef key, CFTypeRef value);

//service
typedef     int(*func_AMDeviceSecureStartService)(HANDLE hDevice, HANDLE hCFServiceName, HANDLE hUnknow, HANDLE* pServiceHandle);
typedef     UINT(*func_AMDServiceConnectionGetSocket)(HANDLE pServiceHandle);
typedef     int(*func_AMDServiceConnectionGetSecureIOContext)(HANDLE pServiceHandle);
typedef     int(*func_AFCConnectionOpen)(UINT pServiceSocketHandle, unsigned int io_timeout, HANDLE* pAfcServiceHandle);
typedef     int(*func_AFCConnectionClose)(HANDLE pAfcServiceHandle);
typedef     int(*func_AFCFileInfoOpen)(HANDLE pAfcServiceHandle, const char* pszPath, HANDLE* pFileInfo);
typedef     int(*func_AFCKeyValueRead)(HANDLE pFileInfo, HANDLE* pKey, HANDLE* pValue);
typedef     int(*func_AFCKeyValueClose)(HANDLE pFileInfo);
typedef     int(*func_AFCFileRefOpen)(HANDLE pAfcServiceHandle, const char* pszPath, int mode, HANDLE& pFileRefHandle);
typedef     int(*func_AFCFileRefWrite)(HANDLE pAfcServiceHandle, HANDLE pFileRefHandle, char* pszData, LONG64 writeSize);
typedef     int(*func_AFCFileRefRead)(HANDLE pAfcServiceHandle, HANDLE pFileRefHandle, char* pszData, LONG64* readSize);
typedef     int(*func_AFCFileRefClose)(HANDLE pAfcServiceHandle, HANDLE pFileRefHandle);
typedef     int(*func_AFCDirectoryCreate)(HANDLE pAfcServiceHandle, const char *path);
typedef     int(*func_AFCDirectoryOpen)(HANDLE pAfcServiceHandle, const char* pszPath, HANDLE* pDirHandle);
typedef     int(*func_AFCDirectoryRead)(HANDLE pAfcServiceHandle, HANDLE pDirHandle, HANDLE* pBuffer);
typedef     int(*func_AFCDirectoryClose)(HANDLE pAfcServiceHandle, HANDLE pDirHandle);
typedef     int(*func_AFCRemovePath)(HANDLE pAfcServiceHandle, const char* pszPath);

//ATH Functions
typedef     ATHConnectionRef(*func_ATHostConnectionCreateWithLibrary)(CFStringRef cfGuid, CFStringRef cfLibraryID, HANDLE unkown);//yes
typedef     int(*func_ATHostConnectionRetain)(ATHConnectionRef athconn);//yes
typedef     CFTypeRef(*func_ATHostConnectionReadMessage)(ATHConnectionRef athconn);//yes
typedef     int(*func_ATHostConnectionSendPowerAssertion)(ATHConnectionRef athconn, CFBooleanRef unknown);//yes
typedef     int(*func_ATHostConnectionSendSyncRequest)(ATHConnectionRef athconn, CFTypeRef cfData1, CFTypeRef cfData2, CFTypeRef cfData3);//yes
typedef     int(*func_ATHostConnectionGetGrappaSessionId)(ATHConnectionRef athconn);//yes
typedef     int(*func_ATHostConnectionSendMetadataSyncFinished)(ATHConnectionRef athconn, CFTypeRef cfData1, CFTypeRef cfData2);//yes
typedef     int(*func_ATHostConnectionSendFileProgress)(ATHConnectionRef athconn, CFStringRef cfstring1, CFStringRef cfstring2);
typedef     int(*func_ATHostConnectionSendAssetCompleted)(ATHConnectionRef athconn, CFStringRef cfstring1, CFStringRef cfstring2, CFStringRef cfstring3);//yes
typedef     int(*func_ATHostConnectionDestroy)(ATHConnectionRef athconn);
typedef     int(*func_ATHostConnectionSendFileError)(ATHConnectionRef athconn, CFStringRef cfstring1, CFStringRef cfstring2);

//iTunes.dll
typedef     int(__cdecl *fun_GetHashCig)(unsigned int grappaSessionId, const char* plistdata, int plist_length, unsigned char** cigdata, int* ciglength);

//itunes environment
struct ituens_environment
{
    wchar_t sz_dll_AppleApplicationSupport_folder[ituens_max_path] = { 0 };
    wchar_t sz_dll_AppleMobileDeviceSupport_folder[ituens_max_path] = { 0 };
    wchar_t sz_dll_iTunes_folder[ituens_max_path] = { 0 };
    wchar_t sz_dll_itunesmobiledevice_path[ituens_max_path] = { 0 };
    wchar_t sz_dll_airtraffichost_path[ituens_max_path] = { 0 };
    wchar_t sz_dll_corefoundation_path[ituens_max_path] = { 0 };
    wchar_t sz_dll_avfoundationcf_path[ituens_max_path] = { 0 };
    wchar_t sz_dll_itunes_path[ituens_max_path] = { 0 };
    wchar_t sz_itunes_version[ituens_max_path] = { 0 };
    DWORD is64 = 0;

    HMODULE hAvFoundationCF = NULL;
    HMODULE hCoreFoundation = NULL;
    HMODULE hAirTrafficHost = NULL;
    HMODULE hiTunesMobileDevice = NULL;
    HMODULE hiTunes = NULL;
};

// 获取各个DLL的路径信息
iTunesApiError iTunes::get_itunes_environment(ituens_environment & env)
{
    iTunesApiError ituens_ret = iTunes_Ok;
    ituens_ret = iTunes_ReadReg_Failed;;
    do {
        HKEY regKey = nullptr;
        wchar_t regValue[ituens_max_path] = { 0 };
        unsigned long regSize = ituens_max_path * sizeof(wchar_t);
        DWORD dwType = REG_NONE;
        long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, itunes_AppleApplicationSupport_reg, 0, KEY_WOW64_64KEY | KEY_READ, &regKey);
        if (!ret) {
            ret = RegQueryValueEx(regKey, _T("InstallDir"), 0, &dwType, (LPBYTE)env.sz_dll_AppleApplicationSupport_folder, &regSize);
            if (ret != 0) break;
            swprintf(env.sz_dll_avfoundationcf_path, L"%sAVFoundationCF.dll", env.sz_dll_AppleApplicationSupport_folder);
            swprintf(env.sz_dll_corefoundation_path, L"%sCoreFoundation.dll", env.sz_dll_AppleApplicationSupport_folder);
        }
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, itunes_AppleMobileDeviceSupport_reg, 0, KEY_WOW64_64KEY | KEY_READ, &regKey);
        if (!ret) {
            ret = RegQueryValueEx(regKey, _T("InstallDir"), 0, &dwType, (LPBYTE)env.sz_dll_AppleMobileDeviceSupport_folder, &regSize);
            if (ret != 0) break;
            swprintf(env.sz_dll_airtraffichost_path, L"%sAirTrafficHost.dll", env.sz_dll_AppleMobileDeviceSupport_folder);
            swprintf(env.sz_dll_itunesmobiledevice_path, L"%siTunesMobileDevice.dll", env.sz_dll_AppleMobileDeviceSupport_folder);
        }
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, itunes_iTunes_reg, 0, KEY_WOW64_64KEY | KEY_READ, &regKey);
        if (!ret) {
            ret = RegQueryValueEx(regKey, _T("InstallDir"), 0, &dwType, (LPBYTE)env.sz_dll_iTunes_folder, &regSize);
            if (ret != 0) break;
            swprintf(env.sz_dll_itunes_path, L"%siTunes.dll", env.sz_dll_iTunes_folder);
            ret = RegQueryValueEx(regKey, _T("Version"), 0, &dwType, (LPBYTE)env.sz_itunes_version, &regSize);
            if (ret != 0) break;
            regSize = sizeof(DWORD);
            ret = RegQueryValueEx(regKey, _T("Installed64Bit"), 0, &dwType, (LPBYTE)(&env.is64), &regSize);
            if (ret != 0) break;
        }
        ituens_ret = iTunes_Ok;
    } while (0);
    return ituens_ret;
}

// 加载DLL和需要使用的函数以及常数值
iTunesApiError iTunes::load_itunes_library()
{
    iTunesApiError ituens_ret = iTunes_Ok;
    ituens_ret = get_itunes_environment(itunes_env);
    if (iTunes_Ok == ituens_ret) {
        wchar_t* lpsz_env = _wgetenv(L"path");
        wchar_t sz_env[ituens_max_path * 10] = { 0 };
        wsprintf(sz_env, _T("path=%s;%s;%s;%s"), 
            itunes_env.sz_dll_AppleApplicationSupport_folder, 
            itunes_env.sz_dll_AppleMobileDeviceSupport_folder, 
            itunes_env.sz_dll_iTunes_folder, 
            lpsz_env);
        _wputenv(sz_env);

        //load librarys
        SetCurrentDirectory(itunes_env.sz_dll_AppleApplicationSupport_folder);
        itunes_env.hCoreFoundation = LoadLibraryEx(itunes_env.sz_dll_corefoundation_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        itunes_env.hAvFoundationCF = LoadLibraryEx(itunes_env.sz_dll_avfoundationcf_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        SetCurrentDirectory(itunes_env.sz_dll_AppleMobileDeviceSupport_folder);
        itunes_env.hAirTrafficHost = LoadLibraryEx(itunes_env.sz_dll_airtraffichost_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        itunes_env.hiTunesMobileDevice = LoadLibraryEx(itunes_env.sz_dll_itunesmobiledevice_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        SetCurrentDirectory(itunes_env.sz_dll_iTunes_folder);
        itunes_env.hiTunes = LoadLibraryEx(itunes_env.sz_dll_itunes_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

        do
        {
            ituens_ret = iTunes_LoadLibrary_Failed;

            //am
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMRestoreRegisterForDeviceNotifications, func_AMRestoreRegisterForDeviceNotifications, "AMRestoreRegisterForDeviceNotifications");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDServiceConnectionSend, func_AMDServiceConnectionSend, "AMDServiceConnectionSend");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDServiceConnectionReceive, func_AMDServiceConnectionReceive, "AMDServiceConnectionReceive");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceGetInterfaceType, func_AMDeviceGetInterfaceType, "AMDeviceGetInterfaceType");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDServiceConnectionInvalidate, func_AMDServiceConnectionInvalidate, "AMDServiceConnectionInvalidate");
            //这个API函数这个版本好像不见了
            //LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceRetain, func_AMDeviceRetain, "AMDeviceRetain");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceNotificationSubscribe, func_AMDeviceNotificationSubscribe, "AMDeviceNotificationSubscribe");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceConnect, func_AMDeviceConnect, "AMDeviceConnect");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceCopyDeviceIdentifier, func_AMDeviceCopyDeviceIdentifier, "AMDeviceCopyDeviceIdentifier");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceDisconnect, func_AMDeviceDisconnect, "AMDeviceDisconnect");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceIsPaired, func_AMDeviceIsPaired, "AMDeviceIsPaired");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceValidatePairing, func_AMDeviceValidatePairing, "AMDeviceValidatePairing");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceStartSession, func_AMDeviceStartSession, "AMDeviceStartSession");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceStopSession, func_AMDeviceStopSession, "AMDeviceStopSession");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceSetValue, func_AMDeviceSetValue, "AMDeviceSetValue");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceCopyValue, func_AMDeviceCopyValue, "AMDeviceCopyValue");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceGetInterfaceType, func_AMDeviceGetInterfaceType, "AMDeviceGetInterfaceType");
            //cfstring
            LoadLibraryModule(itunes_env.hCoreFoundation, CFStringMakeConstantString, func_CFStringMakeConstantString, "__CFStringMakeConstantString");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFStringGetCString, func_CFStringGetCString, "CFStringGetCString");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFGetTypeID, func_CFGetTypeID, "CFGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFStringGetTypeID, func_CFStringGetTypeID, "CFStringGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFStringGetLength, func_CFStringGetLength, "CFStringGetLength");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFBooleanGetValue, func_CFBooleanGetValue, "CFBooleanGetValue");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFBooleanGetTypeID, func_CFBooleanGetTypeID, "CFBooleanGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFNumberGetTypeID, func_CFNumberGetTypeID, "CFNumberGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFNumberGetValue, func_CFNumberGetValue, "CFNumberGetValue");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFNumberGetType, func_CFNumberGetType, "CFNumberGetType");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFNumberGetByteSize, func_CFNumberGetByteSize, "CFNumberGetByteSize");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataCreate, func_CFDataCreate, "CFDataCreate");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFPropertyListCreateFromXMLData, func_CFPropertyListCreateFromXMLData, "CFPropertyListCreateFromXMLData");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFWriteStreamCreateWithAllocatedBuffers, func_CFWriteStreamCreateWithAllocatedBuffers, "CFWriteStreamCreateWithAllocatedBuffers");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFWriteStreamOpen, func_CFWriteStreamOpen, "CFWriteStreamOpen");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFPropertyListWriteToStream, func_CFPropertyListWriteToStream, "CFPropertyListWriteToStream");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFWriteStreamCopyProperty, func_CFWriteStreamCopyProperty, "CFWriteStreamCopyProperty");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetLength, func_CFDataGetLength, "CFDataGetLength");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetBytePtr, func_CFDataGetBytePtr, "CFDataGetBytePtr");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFWriteStreamClose, func_CFWriteStreamClose, "CFWriteStreamClose");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFRelease, func_CFRelease, "CFRelease");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayCreateMutable, func_CFArrayCreateMutable, "CFArrayCreateMutable");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayAppendValue, func_CFArrayAppendValue, "CFArrayAppendValue");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayAppendArray, func_CFArrayAppendArray, "CFArrayAppendArray");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDictionaryCreateMutable, func_CFDictionaryCreateMutable, "CFDictionaryCreateMutable");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDictionaryAddValue, func_CFDictionaryAddValue, "CFDictionaryAddValue");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDictionaryGetTypeID, func_CFDictionaryGetTypeID, "CFDictionaryGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDictionaryGetCount, func_CFDictionaryGetCount, "CFDictionaryGetCount");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDictionaryGetKeysAndValues, func_CFDictionaryGetKeysAndValues, "CFDictionaryGetKeysAndValues");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayGetTypeID, func_CFArrayGetTypeID, "CFArrayGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayGetCount, func_CFArrayGetCount, "CFArrayGetCount");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFArrayGetValueAtIndex, func_CFArrayGetValueAtIndex, "CFArrayGetValueAtIndex");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetTypeID, func_CFDataGetTypeID, "CFDataGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetLength, func_CFDataGetLength, "CFDataGetLength");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetBytePtr, func_CFDataGetBytePtr, "CFDataGetBytePtr");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDataGetBytes, func_CFDataGetBytes, "CFDataGetBytes");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDateGetTypeID, func_CFDateGetTypeID, "CFDateGetTypeID");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDateCreate, func_CFDateCreate, "CFDateCreate");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDateGetAbsoluteTime, func_CFDateGetAbsoluteTime, "CFDateGetAbsoluteTime");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDateGetTimeIntervalSinceDate, func_CFDateGetTimeIntervalSinceDate, "CFDateGetTimeIntervalSinceDate");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFDateCompare, func_CFDateCompare, "CFDateCompare");
            LoadLibraryModule(itunes_env.hCoreFoundation, CFNumberCreate, func_CFNumberCreate, "CFNumberCreate");

            //service
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDeviceSecureStartService, func_AMDeviceSecureStartService, "AMDeviceSecureStartService");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDServiceConnectionGetSocket, func_AMDServiceConnectionGetSocket, "AMDServiceConnectionGetSocket");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AMDServiceConnectionGetSecureIOContext, func_AMDServiceConnectionGetSecureIOContext, "AMDServiceConnectionGetSecureIOContext");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCConnectionOpen, func_AFCConnectionOpen, "AFCConnectionOpen");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCConnectionClose, func_AFCConnectionClose, "AFCConnectionClose");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCFileInfoOpen, func_AFCFileInfoOpen, "AFCFileInfoOpen");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCKeyValueRead, func_AFCKeyValueRead, "AFCKeyValueRead");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCKeyValueClose, func_AFCKeyValueClose, "AFCKeyValueClose");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCFileRefOpen, func_AFCFileRefOpen, "AFCFileRefOpen");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCFileRefWrite, func_AFCFileRefWrite, "AFCFileRefWrite");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCFileRefRead, func_AFCFileRefRead, "AFCFileRefRead");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCFileRefClose, func_AFCFileRefClose, "AFCFileRefClose");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCDirectoryCreate, func_AFCDirectoryCreate, "AFCDirectoryCreate");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCDirectoryOpen, func_AFCDirectoryOpen, "AFCDirectoryOpen");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCDirectoryRead, func_AFCDirectoryRead, "AFCDirectoryRead");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCDirectoryClose, func_AFCDirectoryClose, "AFCDirectoryClose");
            LoadLibraryModule(itunes_env.hiTunesMobileDevice, AFCRemovePath, func_AFCRemovePath, "AFCRemovePath");

            kCFBooleanTrue = *(CFBooleanRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFBooleanTrue");
            kCFBooleanFalse = *(CFBooleanRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFBooleanFalse");

            kCFStreamPropertyAppendToFile = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertyAppendToFile");
            kCFStreamPropertyFileCurrentOffset = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertyFileCurrentOffset");
            kCFStreamPropertyDataWritten = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertyDataWritten");
            kCFStreamPropertySocketNativeHandle = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertySocketNativeHandle");
            kCFStreamPropertySocketRemoteHostName = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertySocketRemoteHostName");
            kCFStreamPropertySocketRemotePortNumber = *(CFStringRef*)GetProcAddress(itunes_env.hCoreFoundation, "kCFStreamPropertySocketRemotePortNumber");
            kCFTypeArrayCallBacks = *(CFArrayCallBacks*)GetProcAddress(itunes_env.hCoreFoundation, "kCFTypeArrayCallBacks");
            kCFTypeDictionaryKeyCallBacks = *(CFDictionaryKeyCallBacks*)GetProcAddress(itunes_env.hCoreFoundation, "kCFTypeDictionaryKeyCallBacks");
            kCFTypeDictionaryValueCallBacks = *(CFDictionaryValueCallBacks*)GetProcAddress(itunes_env.hCoreFoundation, "kCFTypeDictionaryValueCallBacks");

            //ATH
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionCreateWithLibrary, func_ATHostConnectionCreateWithLibrary, "ATHostConnectionCreateWithLibrary");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionRetain, func_ATHostConnectionRetain, "ATHostConnectionRetain");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionReadMessage, func_ATHostConnectionReadMessage, "ATHostConnectionReadMessage");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendPowerAssertion, func_ATHostConnectionSendPowerAssertion, "ATHostConnectionSendPowerAssertion");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendSyncRequest, func_ATHostConnectionSendSyncRequest, "ATHostConnectionSendSyncRequest");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionGetGrappaSessionId, func_ATHostConnectionGetGrappaSessionId, "ATHostConnectionGetGrappaSessionId");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendMetadataSyncFinished, func_ATHostConnectionSendMetadataSyncFinished, "ATHostConnectionSendMetadataSyncFinished");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendFileProgress, func_ATHostConnectionSendFileProgress, "ATHostConnectionSendFileProgress");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendAssetCompleted, func_ATHostConnectionSendAssetCompleted, "ATHostConnectionSendAssetCompleted");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionDestroy, func_ATHostConnectionDestroy, "ATHostConnectionDestroy");
            LoadLibraryModule(itunes_env.hAirTrafficHost, ATHostConnectionSendFileError, func_ATHostConnectionSendFileError, "ATHostConnectionSendFileError");

            ituens_ret = iTunes_Ok;
        } while (0);
    }
    return ituens_ret;
}
```

# 设备连接以及获取设备基本信息
```
// 调用流程（按照顺序...）

// 注册回调
AMRestorableDeviceRegisterForNotifications // 注册监听手机状态的函数，比如进入DFU模式
AMDeviceNotificationSubscribe // 注册连接回调函数

// 在连接回调函数里
AMDeviceConnect // 正常：返回0
AMDeviceGetInterfaceType // 1：USB连接 2：WI-FI连接？（我这里只取返回为1的连接信息，2的直接放弃，设备插上的时候会走两次连接回调）
AMDeviceValidatePairing // 正常：返回0
AMDeviceStartSession // 启动会话，正常返回0

__CFStringMakeConstantString // 构造后续函数所需要的字符串类型的参数

// 获取设备信息
AMDeviceCopyValue // 重复调用，传入需要取出的属性值
// 以下取出我自己手机的信息，自行对号
//SerialNumber:F2MNPFCMG5QY
//UniqueDeviceID:b2275276e47c17897fdf68cd3879xxxxxxxxxxxx
//DeviceName:stiven's 6 plus
//ProductType:iPhone7,1  （不是7不是7不是7...）
//ProductVersion:10.1.1
//PhoneNumber:+86 153-1313-xxxx （不好意思...不能写出来）
//DeviceColor:#e1e4e3
//DeviceEnclosureColor:#e1ccb5
//BasebandVersion:5.26.00
//BasebandBootloaderVersion:??? （函数返回0，取不出来）
//ModelNumber:MGAK2
//RegionInfo:CH/A
//InternationalMobileEquipmentIdentity:354442064xxxxxx
//TimeZone:Asia/Shanghai
//BluetoothAddress:60:f8:1d:xx:xx:xx
//WiFiAddress:60:f8:1d:xx:xx:xx
//ActivationState:Activated
...

//还有很多...
```
# 所需要的服务
首先介绍一下这里的服务的概念:
需要设备完成某一个特定功能,首先需要使用相关API通知设备开启对应的服务,然后才能使用其他API来完成特定功能.
设备上有很多功能都需要开启服务,如:
- 操作文件 - AFC和AFC2服务
- 音乐和播放列表管理(增删改) - ATC服务
- 图片和相册管理(增删改) - ATC服务
- 联系人管理 - Mobilesync服务
- 应用程序管理(安装卸载等) - Springboard服务和Installation_proxy服务还有HouseArrest服务

当然还有一些其他的服务,这里就不多说了，其中有些服务是需要一直开启的,有些服务则在使用完之后需要马上关闭
```
// 启动服务

__CFStringMakeConstantString // 构造后续函数所需要的字符串类型的参数

AMDeviceSecureStartService(hDevice, CFStringRef, NULL, &hServiceHandle)
AMDServiceConnectionGetSocket(hServiceHandle) //socket number
AMDServiceConnectionGetSecureIOContext(hServiceHandle) // 正常返回0
__CFStringMakeConstantString("com.apple.afc");//文件系统服务
__CFStringMakeConstantString("com.apple.afc2");//越狱文件系统服务
__CFStringMakeConstantString("com.apple.mobile.installation_proxy");//应用服务
__CFStringMakeConstantString("com.apple.mobile.house_arrest");//应用内文件服务
__CFStringMakeConstantString("com.apple.mobile.notification_proxy");//通知服务
__CFStringMakeConstantString("com.apple.atc");//同步服务
__CFStringMakeConstantString("com.apple.springboardservices");
__CFStringMakeConstantString("com.apple.mobile.diagnostics_relay");
// 服务通信
AMDServiceConnectionSend // 发送正文内容的长度，&len，4个字节，正常返回发送字节成功数量
AMDServiceConnectionSend // 发送正文，推荐4096个字节一组一组的发
AMDServiceConnectionReceive // 接收服务的返回，4个字节，内容是即将收到正文的长度（需要memcpy转化一下）
AMDServiceConnectionReceive // 接收服务返回正文，推荐4096个字节一组一组的收

```
# 获取设备文件信息 AFCFileInfoOpen函数和AFCKeyValueRead函数
涵数原型说明如下
```
int AFCFileInfoOpen(HANDLE conn,char* path, HANDLE* data);
参数1：设备连接句柄；（不清楚的同志请参阅前面的章节内容）
参数2：设备文件路径参照  "/DCIM/100APPLE/IMG_0006.JPG";（是设备上的文件路径）
参数3：输出参数，返回数据的句柄，下一个函数需要用到；
返回值：0表示成功；
```

```
int AFCKeyValueRead(HANDLE data,HANDLE* key, HANDLE* value);
参数1：上个函数中的参数3；
参数2：输出参数；
参数3：输出参数；
返回值：0表示成功；
```

获取的文件信息是一个一个键值对，如 st_time = 198263546165;
所以我们需要一个链表结构保存文件信息：
```
typedef struct FileInfo{
	char key[MAX_PATH];
	char value[MAX_PATH];
	FileInfo* next;
} FI,*PFI;
```
体获取设备文件信息函数：
```
void GetFileInfomation(IOSFI* fi,char* path)
{
	FI *fi1,*fi2;
	fi = NULL;
	fi1=fi2=(IOSFI *)malloc(sizeof(IOSFI));
	HANDLE data;
	int ret = AFCFileInfoOpen(AFC,path,&data);
	if (ret ==0 && data!=NULL)
	{
		HANDLE hKey,hValue;
		hKey = hValue = NULL;
		while (AFCKeyValueRead(data,&hKey,&hValue)==0 &&hKey!=NULL && hValue!=NULL)
		{
			fi1=(FI *)malloc(sizeof(FI));
			strcpy(fi->key,(char *)(hKey));
			strcpy(fi->value,(char*)(hValue));
			if (fi == NULL)
				fi = fi1;
			else
				fi2->next = fi1;
			fi2 = fi1;
		}
	}
}
```
这样就获取到了设备上的文件基本信息，如创建时间，文件大小等等。

# 操作文件和文件夹所需要开启的服务:AFC服务和AFC2服务

开启AFC服务：
```
BOOL IOSAFCService::Open(){
	if (iTunesApi::AMDeviceSecureStartService(m_hDevice, iTunesApi::CFStringMakeConstantString("com.apple.afc"), NULL, &serviceHandle) != 0)
		return FALSE;
	socketHandle = iTunesApi::AMDServiceConnectionGetSocket(serviceHandle);
	if(iTunesApi::AMDServiceConnectionGetSecureIOContext(serviceHandle)!=0)
		return FALSE;
 
	if (iTunesApi::AFCConnectionOpen((UINT)socketHandle, 0, &afcHandle) != 0)
		return FALSE;
	return TRUE;
}
```
解释:

m_hDevice是之前的设备句柄,不清楚的请开前面几章的内容;

serviceHandle,socketHandle和afcHandle都是HANDLE类型;

其中afcHandle则是最终我们需要的AFC服务的句柄

需要用到的API函数有：

AMDeviceSecureStartService

AMDServiceConnectionGetSocket

AMDServiceConnectionGetSecureIOContext

AFCConnectionOpen

CFStringMakeConstantString 构造设备所需要的字符串类型，在CoreFundation.dll里面

 

至于函数原型，直接看上面的代码就能看出来，就不多说了。

 

至于AFC2服务只需要改一个地方：

iTunesApi::CFStringMakeConstantString("com.apple.afc") 改成iTunesApi::CFStringMakeConstantString("com.apple.afc2")即可

 

这两个服务有什么区别呢？

其中AFC服务是标准的文件系统，AFC2可以算是越狱文件系统了，AFC2服务更高级，

AFC服务所提供的根目录实际上是AFC2服务/var/mobile/Media目录

当然实际上我们一般只用得上AFC服务就足够了








