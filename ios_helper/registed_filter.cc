#include "ios_helper/registed_filter.h"
#include "ios_helper/hook_os_info.h"
#include "ABI/base/message_loop.h"
#include "ABI/base/message_hook.h"
#include "ABI/base/windows_version.h"
#include "ABI/base/process_token.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_conv.h"
#include "ABI/detours/detours.h"
#include "ABI/thirdparty/glog/scoped_ptr.h"
#include "ABI/ios_authorize/itunes_module.h"


namespace ios_helper{
	RegistedFilter::RegistedFilter(HINSTANCE hinstance){
		if(RequireProcessToken()){
			FixHardwareRegValue();
			RegistedHookFilter(hinstance);
		}
	}
	RegistedFilter::~RegistedFilter(){
		UnRegistedHookFilter();
	}
	bool RegistedFilter::RequireProcessToken(){
		ABI::base::ProcessToken process;
		ABI::base::WinVersion win_version;
		if(win_version.IsWinNT6()&&!process.IsRunAsAdmin()){
			process.RunProcessAdminToken();
			exit(0);
		}
		if(process.IsRunAsAdmin()){
			process.SetProcessDebugToken();
			return true;
		}
		else{
			return false;
		}
	}
	void RegistedFilter::FixHardwareRegValue(){
		ios_helper::OsInfo computer;
		scoped_array<char> guid(new char[1024]);
		computer.GenerateWindowsHardwareGUID(guid.get());
		computer.PatchLocalComputerHardwareInfo();
		computer.GenerateWindowsHardwareGUID(guid.get());
	}
	void RegistedFilter::RegistedHookFilter(HINSTANCE hinstance){
		if(ABI::base::MessageLoop(hinstance)){
			ABI::base::RegistedMessageFilter();
		}
		OsInfo::Instance()->HookOsInfoFilter(false);
	}
	void RegistedFilter::UnRegistedHookFilter(){
		OsInfo::Instance()->HookOsInfoFilter(true);
		ABI::base::UnRegistedFilter();
	}
	void RegistedFilter::RegistedProcessHookFilter(){
		STARTUPINFOW si = {sizeof(STARTUPINFOW),0};
		PROCESS_INFORMATION pi = {0};
		ABI::iTunes::iTunesModule itunes_module;
		const std::string dllname = ABI::base::UnicodeToAscii(ABI::base::AppendPath(L"ios_helper.dll").directory_name());
		const std::wstring exename = itunes_module.iTunesDll(L"iTunes.exe");
		DetourCreateProcessWithDllW(NULL,const_cast<LPWSTR>(exename.c_str()),NULL,NULL,FALSE,CREATE_DEFAULT_ERROR_MODE,NULL,NULL,&si,&pi,dllname.c_str(),NULL);
	}
	RegistedFilter* RegistedFilter::RegistedInstance(HINSTANCE hinstance){
		static RegistedFilter* info;
		if(!info){
			RegistedFilter* new_info = new RegistedFilter(hinstance);
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
				delete new_info;
			}
		}
		return info;
	}
}