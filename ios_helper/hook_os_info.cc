#include <WinSock2.h>
#include "ios_helper/hook_os_info.h"
#include "ABI/base/lock_impl.h"
#include "ABI/base/lock_mutex.h"
#include "ABI/base/windows_hardware.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_conv.h"
#include "ABI/thirdparty/glog/logging.h"
#include "ABI/thirdparty/glog/scoped_ptr.h"
//#pragma comment(lib,"detours.lib")
#include "ABI/detours/detours.h"
#include "../ABI/thirdparty/mhook/mhook-lib/mhook.h"

namespace ios_helper{
	BOOL (WINAPI* GetVolumeInformationOriginateA)(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
	BOOL (WINAPI* GetVolumeInformationOriginateW)(LPCWSTR,LPWSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPWSTR,DWORD);
	DWORD (WINAPI* GetAdaptersInfoOriginate)(PIP_ADAPTER_INFO,PULONG);
	ULONG (WINAPI* GetAdaptersAddressesOriginate)(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);
	BOOL (WINAPI* GetComputerNameOriginateA)(LPSTR,LPDWORD);
	BOOL (WINAPI* GetComputerNameOriginateW)(LPWSTR,LPDWORD);

	BOOL WINAPI GetComputerNameFilterA(LPSTR lpBuffer,LPDWORD nSize){
		try{
			if(nSize==NULL){
				return FALSE;
			}
			if(*nSize<7){
				*nSize = 7;
				return FALSE;
			}
			if(lpBuffer!=NULL){
				BOOL state = GetComputerNameOriginateA(lpBuffer,nSize);
				//LOG(INFO)<<__FUNCTION__<<lpBuffer;
				strcpy(lpBuffer,"YYH-PC");
				*nSize = strlen(lpBuffer);
				return state;
			}
			return FALSE;
		}
		catch(...){
			LOG(WARNING)<<__FILE__<<__FUNCTION__;
			return FALSE;
		}
	}
	BOOL WINAPI GetComputerNameFilterW(LPWSTR lpBuffer,LPDWORD nSize){
		try{
			if(nSize==NULL){
				return FALSE;
			}
			if(*nSize<14){
				*nSize = 14;
				return FALSE;
			}
			if(lpBuffer!=NULL){
				BOOL state = GetComputerNameOriginateW(lpBuffer,nSize);
				//LOG(INFO)<<__FUNCTION__<<lpBuffer;
				wcscpy(lpBuffer,L"YYH-PC");
				*nSize = wcslen(lpBuffer);
				return state;
			}
			return FALSE;
		}
		catch(...){
			LOG(WARNING)<<__FILE__<<__FUNCTION__;
			return FALSE;
		}
	}
	DWORD WINAPI GetAdaptersInfoFilter(PIP_ADAPTER_INFO pAdapterInfo,PULONG pOutBufLen){
		try{
			DWORD error_code = GetAdaptersInfoOriginate(pAdapterInfo,pOutBufLen);
			if(error_code!=ERROR_BUFFER_OVERFLOW&&error_code==ERROR_SUCCESS){
				for(unsigned long i=0;i<=6;i++){
					pAdapterInfo->Address[i] = static_cast<BYTE>(i+1);
				}
				//LOG(INFO)<<__FUNCTION__<<pAdapterInfo->Address[0]<<pAdapterInfo->Address[1]<<pAdapterInfo->Address[2]<<pAdapterInfo->Address[3]<<pAdapterInfo->Address[4]<<pAdapterInfo->Address[5];
			}
			return error_code;
		}
		catch(...){
			LOG(WARNING)<<__FILE__<<__FUNCTION__;
			return ERROR_BUFFER_OVERFLOW;
		}
	}
	ULONG WINAPI GetAdaptersAddressesFilter(ULONG Family,ULONG Flags,PVOID Reserved,PIP_ADAPTER_ADDRESSES AdapterAddresses,PULONG SizePointer){
		DWORD error_code = ERROR_SUCCESS;
		try{
			error_code = GetAdaptersAddressesOriginate(Family,Flags,Reserved,AdapterAddresses,SizePointer);
			if(error_code!=ERROR_BUFFER_OVERFLOW&&error_code==ERROR_SUCCESS){
				for(unsigned long i=0;i<=6;i++){
					AdapterAddresses->PhysicalAddress[i] = static_cast<BYTE>(i+1);
				}
				//LOG(INFO)<<__FUNCTION__<<AdapterAddresses->PhysicalAddress[0]<<AdapterAddresses->PhysicalAddress[1]<<AdapterAddresses->PhysicalAddress[2]<<AdapterAddresses->PhysicalAddress[3]<<AdapterAddresses->PhysicalAddress[4]<<AdapterAddresses->PhysicalAddress[5];
			}
		}
		catch(...){
			LOG(WARNING)<<__FILE__<<__FUNCTION__;
			return ERROR_BUFFER_OVERFLOW;
		}
		return error_code;
	}
	BOOL WINAPI GetVolumeInformationFilterA(LPCSTR lpRootPathName,LPSTR lpVolumeNameBuffer,DWORD nVolumeNameSize,LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength,
		LPDWORD lpFileSystemFlags,LPSTR lpFileSystemNameBuffer,DWORD nFileSystemNameSize){
		try{
			BOOL state = GetVolumeInformationOriginateA(lpRootPathName,lpVolumeNameBuffer,nVolumeNameSize,lpVolumeSerialNumber,lpMaximumComponentLength,
				lpFileSystemFlags,lpFileSystemNameBuffer,nFileSystemNameSize);
			if(lpVolumeSerialNumber!=NULL){
				//LOG(INFO)<<__FUNCTION__<<lpVolumeSerialNumber[0];
				*lpVolumeSerialNumber = 0x12345678;
			}
			return state;
		}
		catch(...){
			LOG(WARNING)<<__FILE__<<__FUNCTION__;
			return FALSE;
		}
	}
	BOOL WINAPI GetVolumeInformationFilterW(LPCWSTR lpRootPathName,LPWSTR lpVolumeNameBuffer,DWORD nVolumeNameSize,LPDWORD lpVolumeSerialNumber,LPDWORD lpMaximumComponentLength,
		LPDWORD lpFileSystemFlags,LPWSTR lpFileSystemNameBuffer,DWORD nFileSystemNameSize){
			try{
				BOOL state = GetVolumeInformationOriginateW(lpRootPathName,lpVolumeNameBuffer,nVolumeNameSize,lpVolumeSerialNumber,lpMaximumComponentLength,
					lpFileSystemFlags,lpFileSystemNameBuffer,nFileSystemNameSize);
				if(lpVolumeSerialNumber!=NULL){
					//LOG(INFO)<<__FUNCTION__<<lpVolumeSerialNumber[0];
					*lpVolumeSerialNumber = 0x12345678;
				}
				return state;
			}
			catch(...){
				LOG(WARNING)<<__FILE__<<__FUNCTION__;
				return FALSE;
			}
	}
	bool OsInfo::GenerateHardwareSerialHDD(const std::string* drive_letter,std::string* str){
		char szPhysicalDrive[64];
		sprintf(szPhysicalDrive, "\\\\.\\%s:", drive_letter->c_str());
		HANDLE hPhysicalDriveIOCTL = CreateFileA(szPhysicalDrive,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if(hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE){
			DWORD dwBytesReturned = 0;
			VOLUME_DISK_EXTENTS vdExtents;
			ZeroMemory(&vdExtents, sizeof(vdExtents));
			if(!DeviceIoControl(hPhysicalDriveIOCTL,IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,NULL,0,&vdExtents,sizeof(vdExtents),&dwBytesReturned,NULL)){
				CloseHandle(hPhysicalDriveIOCTL);
				str->resize(0);
				return false;
			}
			GETVERSIONINPARAMS gvopVersionParams;
			ZeroMemory(&gvopVersionParams, sizeof(gvopVersionParams));
			if(!DeviceIoControl(hPhysicalDriveIOCTL,SMART_GET_VERSION,NULL,0,&gvopVersionParams,sizeof(gvopVersionParams),&dwBytesReturned,NULL)){
				CloseHandle(hPhysicalDriveIOCTL);
				str->resize(0);
				return false;
			}
			if(gvopVersionParams.bIDEDeviceMap >= 0){
				SENDCMDINPARAMS InParams = {
					IDENTIFY_BUFFER_SIZE, { 0, 1, 1, 0, 0, ((vdExtents.Extents[0].DiskNumber & 1) ? 0xB0 : 0xA0),
					((gvopVersionParams.bIDEDeviceMap >> vdExtents.Extents[0].DiskNumber & 0x10) ? ATAPI_ID_CMD : ID_CMD) },
					(BYTE)vdExtents.Extents[0].DiskNumber
				};
				DWORD dwBufSize = sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE;
				PSENDCMDOUTPARAMS pbtIDOutCmd = (PSENDCMDOUTPARAMS) new BYTE[dwBufSize];
				ZeroMemory(pbtIDOutCmd, dwBufSize);
				if(DeviceIoControl(hPhysicalDriveIOCTL, SMART_RCV_DRIVE_DATA, &InParams, sizeof(SENDCMDINPARAMS),pbtIDOutCmd, dwBufSize, &dwBytesReturned, NULL)){
					// Little Endian To Big Endian
					USHORT *pIDSector = (USHORT*)pbtIDOutCmd->bBuffer;
					for(int nShort = 10; nShort < 21; nShort++){
						pIDSector[nShort] = (((pIDSector[nShort] & 0x00FF) << 8) + ((pIDSector[nShort] & 0xFF00) >> 8));
					}
					char* lpszSerialNumber1 = new char[IDENTIFY_BUFFER_SIZE];
					ZeroMemory(lpszSerialNumber1, IDENTIFY_BUFFER_SIZE);
					RtlCopyMemory(lpszSerialNumber1, &pIDSector[10], IDENTIFY_BUFFER_SIZE);
					LPSTR lpszSerialNumber2 = lpszSerialNumber1;
					while(*lpszSerialNumber2 == ' ') lpszSerialNumber2++;
					str->resize(0);
					str->append(lpszSerialNumber2);
					delete[] lpszSerialNumber1;
				}
				delete[] pbtIDOutCmd;

			}
			else{
				CloseHandle(hPhysicalDriveIOCTL);
				str->resize(0);
				return false;
			}
			CloseHandle (hPhysicalDriveIOCTL);
			return true;
		}
		return false;
	}
	void OsInfo::GenerateWindowsHardwareGUID(char* str){
		if(str!=NULL){
			str[0] = 0x0;
			strcpy(str,ABI::base::GetHardwareCookie().c_str());
		}
	}
	void OsInfo::PatchLocalComputerHardwareInfo(){
		SetSystemBios();
		SetProcessorName();
		SetWindowsProductId();
		SetHwProfile();
	}
	void OsInfo::HookOsInfoFilter(bool un_state){
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		HookComputerName(un_state);
		HookAdapterSerial(un_state);
		HookVolumeSerial(un_state);
		DetourTransactionCommit();
	}
	void OsInfo::OutputInfo(std::ostream* out,const void* src,const unsigned long length){
		if(length&&src!=NULL){
			out->write(reinterpret_cast<const char*>(src),length);
		}
		else{
			unsigned long bytes_set = sizeof(unsigned long);
			scoped_array<unsigned char> bytes_buf(NULL);
			bytes_buf.reset(new unsigned char[bytes_set*2]);
			memset(bytes_buf.get(),0,bytes_set*2);
			memset(bytes_buf.get(),0xFF,bytes_set);
			out->write(reinterpret_cast<const char*>(bytes_buf.get()),bytes_set);
		}
	}
	void OsInfo::SetHKLM(const std::wstring* key,const std::wstring* name,const std::wstring* reset_value){
		HKEY phkResult = NULL;
		unsigned long rz_type = 0;
		unsigned long rz_length = 0;
		RegOpenKeyExW(HKEY_LOCAL_MACHINE,key->c_str(),0,KEY_ALL_ACCESS,&phkResult);
		RegQueryValueExW(phkResult,name->c_str(),0,&rz_type,NULL,&rz_length);
		RegSetValueExW(phkResult,name->c_str(),0,rz_type,reinterpret_cast<const BYTE*>(reset_value->c_str()),reset_value->length()*sizeof(wchar_t));
		RegCloseKey(phkResult);
	}
	void OsInfo::HookComputerName(bool un_state){
		if(!un_state){
			*reinterpret_cast<unsigned long*>(&GetComputerNameOriginateA) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Kernel32.dll"),"GetComputerNameA"));
			*reinterpret_cast<unsigned long*>(&GetComputerNameOriginateW) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Kernel32.dll"),"GetComputerNameW"));
			DetourAttach((PVOID*)&GetComputerNameOriginateA,GetComputerNameFilterA);
			DetourAttach((PVOID*)&GetComputerNameOriginateW,GetComputerNameFilterW);
// 			Mhook_SetHook((PVOID*)&GetComputerNameOriginateA,GetComputerNameFilterA);
// 			Mhook_SetHook((PVOID*)&GetComputerNameOriginateW,GetComputerNameFilterW);
		}
		else{
			DetourDetach((PVOID*)&GetComputerNameOriginateA,GetComputerNameFilterA);
			DetourDetach((PVOID*)&GetComputerNameOriginateW,GetComputerNameFilterW);
// 			Mhook_Unhook((PVOID*)&GetComputerNameOriginateA);
// 			Mhook_Unhook((PVOID*)&GetComputerNameOriginateW);
		}
	}
	void OsInfo::HookAdapterSerial(bool un_state){
		if(!un_state){
			*reinterpret_cast<unsigned long*>(&GetAdaptersInfoOriginate) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Iphlpapi.dll"),"GetAdaptersInfo"));
			*reinterpret_cast<unsigned long*>(&GetAdaptersAddressesOriginate) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Iphlpapi.dll"),"GetAdaptersAddresses"));
			DetourAttach((PVOID*)&GetAdaptersInfoOriginate,GetAdaptersInfoFilter);
			DetourAttach((PVOID*)&GetAdaptersAddressesOriginate,GetAdaptersAddressesFilter);
// 			Mhook_SetHook((PVOID*)&GetAdaptersInfoOriginate,GetAdaptersInfoFilter);
// 			Mhook_SetHook((PVOID*)&GetAdaptersAddressesOriginate,GetAdaptersAddressesFilter);
		}
		else{
			DetourDetach((PVOID*)&GetAdaptersInfoOriginate,GetAdaptersInfoFilter);
			DetourDetach((PVOID*)&GetAdaptersAddressesOriginate,GetAdaptersAddressesFilter);
// 			Mhook_Unhook((PVOID*)&GetAdaptersInfoOriginate);
// 			Mhook_Unhook((PVOID*)&GetAdaptersAddressesOriginate);
		}
	}
	void OsInfo::HookVolumeSerial(bool un_state){
		if(!un_state){
			*reinterpret_cast<unsigned long*>(&GetVolumeInformationOriginateA) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Kernel32.dll"),"GetVolumeInformationA"));
			*reinterpret_cast<unsigned long*>(&GetVolumeInformationOriginateW) = reinterpret_cast<unsigned long>(GetProcAddress(GetModuleHandleW(L"Kernel32.dll"),"GetVolumeInformationW"));
			DetourAttach((PVOID*)&GetVolumeInformationOriginateA,GetVolumeInformationFilterA);
			DetourAttach((PVOID*)&GetVolumeInformationOriginateW,GetVolumeInformationFilterW);
// 			Mhook_SetHook((PVOID*)&GetVolumeInformationOriginateA,GetVolumeInformationFilterA);
// 			Mhook_SetHook((PVOID*)&GetVolumeInformationOriginateW,GetVolumeInformationFilterW);
		}
		else{
			DetourDetach((PVOID*)&GetVolumeInformationOriginateA,GetVolumeInformationFilterA);
			DetourDetach((PVOID*)&GetVolumeInformationOriginateW,GetVolumeInformationFilterW);
// 			Mhook_Unhook((PVOID*)&GetVolumeInformationOriginateA);
// 			Mhook_Unhook((PVOID*)&GetVolumeInformationOriginateW);
		}
	}
	void OsInfo::SetSystemBios(){
		std::auto_ptr<std::wstring> key(new std::wstring(L"HARDWARE\\DESCRIPTION\\System"));
		std::auto_ptr<std::wstring> name(new std::wstring(L"SystemBiosVersion"));
		std::auto_ptr<std::wstring> value(new std::wstring(L"ACRSYS - 6222004"));
		SetHKLM(key.get(),name.get(),value.get());
	}
	void OsInfo::SetProcessorName(){
		std::auto_ptr<std::wstring> key(new std::wstring(L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"));
		std::auto_ptr<std::wstring> name(new std::wstring(L"ProcessorNameString"));
		std::auto_ptr<std::wstring> value(new std::wstring(L"Intel(R) Core(TM) i5-2500 CPU @ 3.30GHz"));
		SetHKLM(key.get(),name.get(),value.get());
	}
	void OsInfo::SetWindowsProductId(){
		wchar_t* reg_key = NULL;
		OSVERSIONINFOEXW version_information = {0};
		version_information.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
		version_information.dwMajorVersion = 6;
		version_information.dwMinorVersion = 0;
		unsigned long long condition_mask = VerSetConditionMask(VerSetConditionMask(0,VER_MAJORVERSION,VER_GREATER_EQUAL),VER_MINORVERSION,VER_GREATER_EQUAL);
		if(VerifyVersionInfoW(&version_information,VER_MAJORVERSION|VER_MINORVERSION,condition_mask)){
			reg_key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
		}
		else{
			reg_key = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
		}
		std::auto_ptr<std::wstring> key(new std::wstring(reg_key));
		std::auto_ptr<std::wstring> name(new std::wstring(L"ProductId"));
		std::auto_ptr<std::wstring> value(new std::wstring(L"69713-640-9722366-45198"));
		SetHKLM(key.get(),name.get(),value.get());
	}
	void OsInfo::SetHwProfile(){
		HKEY hKey = NULL;
		unsigned long current_config = 1;
		unsigned long config_size = sizeof(unsigned long);   
		wchar_t RegStr[MAX_PATH] = {0};
		if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"System\\CurrentControlSet\\Control\\IDConfigDB",0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS){
			return;
		}
		RegQueryValueEx(hKey,L"CurrentConfig",NULL,NULL,(LPBYTE)&current_config,&config_size);
		RegCloseKey(hKey);
		wsprintf(RegStr,L"System\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\%04u",current_config);
		std::auto_ptr<std::wstring> key(new std::wstring(RegStr));
		std::auto_ptr<std::wstring> name(new std::wstring(L"HwProfileGuid"));
		std::auto_ptr<std::wstring> value(new std::wstring(L"{846ee340-7039-11de-9d20-806e6f6e6963}"));
		SetHKLM(key.get(),name.get(),value.get());

	}
}