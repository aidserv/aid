#include "ios_cracker/atl_dll_main.h"
#include "ABI/ios_authorize/apple_import.h"
#include <openssl/des.h>
#include <openssl/evp.h>
#include <cstdint>
#include <libimobiledevice/libimobiledevice.h>
#include "libxml/parser.h"
#include "passport/itunes_native_interface.h"

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
		passport::iTunesNativeInterface::GetInstance();
		LoadDlls();
		//TODO CHECK!!
		//libimobiledevice_initialize();
		AtlDllMain::Instance()->Initialize(hDllHandle);
	}
	if(dwReason==DLL_PROCESS_DETACH){
		//xmlCleanupParser();
		//libimobiledevice_deinitialize();
		AtlDllMain::Instance(true);
	}
	return TRUE;
}