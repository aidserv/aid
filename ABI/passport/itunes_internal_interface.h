#ifndef PASSPORT_ITUNES_INTERNAL_INTERFACE_H_
#define PASSPORT_ITUNES_INTERNAL_INTERFACE_H_
//////////////////////////////////////////////////////////////////////////
#include "passport/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	class WIN_DLL_API iTunesInternalInterface{
	public:
		static iTunesInternalInterface* Instance(){
			static iTunesInternalInterface* info;
			if(!info){
				iTunesInternalInterface* new_info = new iTunesInternalInterface;
				if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
					delete new_info;
				}
			}
			return info;
		}
		int (__cdecl *lpfnKbsync)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnCigHash)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnKbsyncID)(unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnWriteSIDD)(unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnWriteSIDB)(unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnDeAuthSIDB)(unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnGenerateAFSyncRS)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnVerifyAFSyncRQ)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnSetAFSyncRQ)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnCalcUnkP1)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnPreAuthByDSID)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnSapInit)(void);
		int (__cdecl *lpfnSapGetP1)(unsigned long, unsigned long);
		int (__cdecl *lpfnSapCalcBuffer)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnSapGetAS)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnSapGetASFD)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnSapGetASFD_a)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnGetCltDat)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnTranSetInf)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnUpdCDID)(unsigned long);
		int (__cdecl *lpfnGetMD)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnInitHost)(unsigned long, unsigned long, unsigned long, unsigned long);
		int (__cdecl *lpfnEstablishKey)(unsigned long, unsigned long, unsigned long);
		unsigned long kb_seed;
	private:
		iTunesInternalInterface(){}
		~iTunesInternalInterface(){}
		DISALLOW_EVIL_CONSTRUCTORS(iTunesInternalInterface);
	};
	template <typename T>
	unsigned long ToDword(const T* k){
		return reinterpret_cast<unsigned long>(k);
	}
	template <typename T>
	unsigned long ToDword(T* k){
		return reinterpret_cast<unsigned long>(k);
	}
	template <typename T>
	unsigned long ToDword(const T k){
		return reinterpret_cast<unsigned long>(k);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif