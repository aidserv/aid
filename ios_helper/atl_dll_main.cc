#include "ios_helper/atl_dll_main.h"
#include "ios_helper/hook_os_info.h"
#include "ABI/exception/exception_dump.h"
#include "ABI/ios_authorize/apple_import.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libxml/parser.h>

class AtlDllMain:public ATL::CAtlDllModuleT<AtlDllMain>{
public:
	AtlDllMain()
		:CAtlDllModuleT< AtlDllMain >(),
		is_initialized_(false) {
	}
	~AtlDllMain() {
	}
	void Initialize() {
		if(is_initialized_) {
			return;
		}
		is_initialized_ = true;
	}
	static AtlDllMain* Instance(){
		static AtlDllMain* ref_instance;
		if(!ref_instance){
			AtlDllMain* new_instance = new AtlDllMain;
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&ref_instance),new_instance,NULL)){
				delete new_instance;
			}
		}
		return ref_instance;
	}
protected:
	bool is_initialized_;
};
BOOL WINAPI DllMain(HMODULE hDllHandle, DWORD dwReason, LPVOID lpreserved){
	if(dwReason==DLL_PROCESS_ATTACH){
		LoadDlls();
		libimobiledevice_initialize();
		if(ios_helper::ProcessName::iTunesProcessInternalDll()){
			ABI::carsh_exception::EnableExceptionHandler();
		}
		AtlDllMain::Instance()->Initialize();
	}
	else if(dwReason==DLL_PROCESS_DETACH){
		xmlCleanupParser();
		libimobiledevice_deinitialize();
		if(ios_helper::ProcessName::iTunesProcessInternalDll()){
			ABI::carsh_exception::DisableExceptionHandler();
		}
	}
	else{
		return DisableThreadLibraryCalls(hDllHandle);
	}
	return AtlDllMain::Instance()->DllMain(dwReason,lpreserved);
}