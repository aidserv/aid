#include "ABI/base/message_hook.h"

#pragma data_seg("Shared")
HHOOK hkKey = NULL;				
HINSTANCE hInstHookDll=NULL;
#pragma data_seg()
#pragma comment(linker,"/section:Shared,rws")

namespace ABI{
	namespace base{
		LRESULT CALLBACK NextHookEx(int nCode,WPARAM wParam, LPARAM lParam){
			return CallNextHookEx(hkKey,nCode,wParam,lParam);
		}
		bool SetFilterModule(HINSTANCE instance){
			hInstHookDll = instance;
			return (hInstHookDll!=NULL);
		}
		bool RegistedMessageFilter(){
			if(hkKey==NULL){
				hkKey = SetWindowsHookEx(WH_GETMESSAGE,NextHookEx,hInstHookDll,0);
			}
			return (hkKey!=NULL);
		}
		bool UnRegistedFilter(){
			if(hkKey!=NULL){
				UnhookWindowsHookEx(hkKey);
				hkKey = NULL;
				return true;
			}
			return false;
		}
	}
}