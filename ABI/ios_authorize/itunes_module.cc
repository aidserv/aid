#include "abi/ios_authorize/itunes_module.h"
#include <Windows.h>
#include <shlobj.h>   
#include <Shlwapi.h>
#pragma	comment(lib,"shell32.lib")   
#include <exception>
#include "passport/itunes_native_interface.h"

namespace ABI{
	namespace iTunes{
		iTunesModule::iTunesModule(void){
			set_core_foundation_dll(L"");
			set_air_traffic_host_dll(L"");
			set_asmapi_interface_dll(L"");
			set_itunes_mobile_device_dll(L"");
			set_mobile_device_dll(L"");
			//AddEnvironmentVariable(iTunesWinPath());
			iTunesPathInit();
		}
		iTunesModule::~iTunesModule(void){
			set_core_foundation_dll(L"");
			set_air_traffic_host_dll(L"");
			set_asmapi_interface_dll(L"");
			set_itunes_mobile_device_dll(L"");
			set_mobile_device_dll(L"");
		}
		std::wstring iTunesModule::iTunesDll(const std::wstring& dll_name){
			return passport::internal::GetITunesInstallDll(dll_name);
		}

		void iTunesModule::iTunesPathInit(){
			set_core_foundation_dll(iTunesDll(L"CoreFoundation.dll"));
			set_air_traffic_host_dll(iTunesDll(L"AirTrafficHost.dll"));
			set_itunes_mobile_device_dll(iTunesDll(L"iTunesMobileDevice.dll"));
			AddEnvironmentVariable(itunes_mobile_device_dll());
		}
		void iTunesModule::AddEnvironmentVariable(const std::wstring& path){
			wchar_t env_path[4096] = {0};
			GetEnvironmentVariableW(L"PATH",env_path,4095);
			std::wstring new_env_path = (std::wstring(env_path) + std::wstring(L";")) + path;
			size_t posiltion = new_env_path.find_last_of('\\');
			if(posiltion!=std::wstring::npos){
				new_env_path[posiltion] = 0;
			}
			SetEnvironmentVariable(L"PATH",new_env_path.c_str());
		}
		std::wstring iTunesModule::iTunesGetSharedDll(const std::wstring dll_name){
			void* hSetting = NULL;
			unsigned long length = 0;
			wchar_t* path = new wchar_t[MAX_PATH*sizeof(wchar_t)];
			if(path==NULL){
				return L"";
			}
			if(::RegCreateKeyW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support\\Shared",reinterpret_cast<PHKEY>(&hSetting))!=ERROR_SUCCESS){
				return L"";
			}
			if(::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),dll_name.c_str(), NULL, NULL, NULL, &length)!=ERROR_SUCCESS){
				return L"";
			}
			::RegQueryValueEx(reinterpret_cast<HKEY>(hSetting),dll_name.c_str(), NULL, NULL, (LPBYTE)path,&length);
			::RegCloseKey(reinterpret_cast<HKEY>(hSetting));
			std::wstring shared_dll(path);
			delete[] path;
			return shared_dll;
		}
		std::wstring iTunesModule::iTunesFrameworkSupport(const std::wstring dll_name,bool env_able){

			void* hSetting = NULL;
			unsigned long length = 0;
			wchar_t* pCoreFoundationPath = new wchar_t[MAX_PATH*sizeof(wchar_t)];
			if(pCoreFoundationPath==NULL){
				return L"";
			}
			if(::RegCreateKeyW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Apple Inc.\\Apple Application Support",reinterpret_cast<PHKEY>(&hSetting))!=ERROR_SUCCESS){
				return L"";
			}
			if(::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),L"InstallDir", NULL, NULL, NULL, &length)!=ERROR_SUCCESS){
				return L"";
			}
			::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),L"InstallDir", NULL, NULL, (LPBYTE)pCoreFoundationPath, &length);
			::RegCloseKey(reinterpret_cast<HKEY>(hSetting));
			if(env_able){
				AddEnvironmentVariable(pCoreFoundationPath);
			}
			if(pCoreFoundationPath[wcslen(pCoreFoundationPath)-1] != '\\'){
				wcscat(pCoreFoundationPath,L"\\");
			}
			wcscat(pCoreFoundationPath,dll_name.c_str());
			std::wstring support_dll(pCoreFoundationPath);
			delete[] pCoreFoundationPath;
			return support_dll;
		}
		std::wstring iTunesModule::iTunesWinPath(){
			wchar_t path[MAX_PATH] = {0};
			SHGetSpecialFolderPathW(NULL,path,CSIDL_PROGRAM_FILES,FALSE);
			return (std::wstring(path)+std::wstring(L"\\iTunes\\"));
		}
		std::wstring iTunesModule::GetSpecialPath(const unsigned long& type_path,const std::wstring& name){
			wchar_t path[MAX_PATH] = {0};
			SHGetSpecialFolderPathW(NULL,path,type_path,FALSE);
			return (std::wstring(path)+name);
		}
	}
}