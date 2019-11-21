#include "ios_broker/atl_dll_main.h"
#include "ABI/ios_authorize/apple_import.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libxml/parser.h>
#include "ABI/exception/exception_dump.h"

class AtlDllMain:public ATL::CAtlDllModuleT<AtlDllMain>{
public:
	AtlDllMain()
		:CAtlDllModuleT< AtlDllMain >(),
		is_initialized_(false) {
	}
	~AtlDllMain() {
	}
	void Initialize(HMODULE module) {
		if(is_initialized_) {
			return;
		}
		is_initialized_ = true;
	}
	bool is_initialized()const{
		return is_initialized_;
	}
	static AtlDllMain* Instance(bool process_exit = false){
		static AtlDllMain* ref_instance;
		if(!ref_instance){
			AtlDllMain* new_instance = new AtlDllMain;
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&ref_instance),new_instance,NULL)){
				delete new_instance;
			}
		}
		if(process_exit){
			delete ref_instance;
			ref_instance = NULL;
		}
		return ref_instance;
	}
protected:
	bool is_initialized_;
};
BOOL WINAPI DllMain(HMODULE hDllHandle, DWORD dwReason, LPVOID lpreserved){
	DisableThreadLibraryCalls(hDllHandle);
	if(dwReason==DLL_PROCESS_ATTACH&&!AtlDllMain::Instance()->is_initialized()){
		libimobiledevice_initialize();
		ABI::carsh_exception::EnableExceptionHandler();
		AtlDllMain::Instance()->Initialize(hDllHandle);
	}
	if(dwReason==DLL_PROCESS_DETACH){
		xmlCleanupParser();
		libimobiledevice_deinitialize();
		ABI::carsh_exception::DisableExceptionHandler();
		AtlDllMain::Instance(true);
	}
	return TRUE;
}