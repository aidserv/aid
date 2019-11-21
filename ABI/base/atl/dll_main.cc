#include "ABI/base/atl/dll_main.h"

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
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved){
	if(dwReason==DLL_PROCESS_ATTACH){
		AtlDllMain::Instance()->Initialize();
	}
	return AtlDllMain::Instance()->DllMain(dwReason,lpreserved);
}