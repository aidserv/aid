#ifndef IOS_HELPER_HOOK_OS_INFO_H_
#define IOS_HELPER_HOOK_OS_INFO_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_helper{
	class OsInfo
	{
	public:
		OsInfo(){}
		~OsInfo(){}
		bool GenerateHardwareSerialHDD(const std::string* drive_letter,std::string* str);
		void GenerateWindowsHardwareGUID(char* str);
		void PatchLocalComputerHardwareInfo();
		void HookOsInfoFilter(bool un_state);
		static OsInfo* Instance(){
			static OsInfo* info;
			if(!info){
				OsInfo* new_info = new OsInfo;
				if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
					delete new_info;
				}
			}
			return info;
		}
	private:
		DISALLOW_EVIL_CONSTRUCTORS(OsInfo);
		void OutputInfo(std::ostream* out,const void* src,const unsigned long length);
		void SetHKLM(const std::wstring* key,const std::wstring* name,const std::wstring* reset_value);
		void HookComputerName(bool un_state);
		void HookAdapterSerial(bool un_state);
		void HookVolumeSerial(bool un_state);
		void SetSystemBios();
		void SetProcessorName();
		void SetWindowsProductId();
		void SetHwProfile();
	};
	struct ProcessName{
	public:
		static bool iTunesProcessInternalDll(){
			wchar_t module_name[MAX_PATH] = {0};
			GetModuleFileNameW(NULL,module_name,MAX_PATH);
			std::wstring upper_name(module_name);
			std::transform(upper_name.begin(),upper_name.end(),upper_name.begin(),std::toupper);
			return (upper_name.find(L"ITUNES.EXE")!=std::wstring::npos);
		}
		static bool ProcessNameFilter(const wchar_t* upcase_name){
			wchar_t module_name[MAX_PATH] = {0};
			GetModuleFileNameW(NULL,module_name,MAX_PATH);
			std::wstring upper_name(module_name);
			std::transform(upper_name.begin(),upper_name.end(),upper_name.begin(),std::toupper);
			return (upper_name.find(upcase_name)!=std::wstring::npos);
		}
		static wchar_t* CoreProcessName(){
			return L"IPLUGINIPA.EXE";
		}
	private:
		DISALLOW_EVIL_CONSTRUCTORS(ProcessName);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif