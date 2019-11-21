#include "ABI/ios_authorize/itunes_module_state.h"
#include <Windows.h>
#include "ABI/ios_authorize/itunes_module.h"

namespace ABI{
	namespace iTunes{
		bool iTunesModuleState::iTunesIsInstalled(){
			ABI::iTunes::iTunesModule module;
			if(GetModuleHandleW(L"iTunes.dll")!=NULL){
				return true;
			}
			return (LoadLibraryW(module.iTunesDll(L"iTunes.dll").c_str())!=NULL);
		}
		bool iTunesModuleState::AppleApplicationSupportIsInstalled(){
			ABI::iTunes::iTunesModule module;
			if(GetModuleHandleW(L"CoreFoundation.dll")!=NULL){
				return true;
			}
			return (LoadLibraryW(module.core_foundation_dll().c_str())!=NULL);
		}
		bool iTunesModuleState::AppleMobileDeviceSupportIsInstalled(){
			ABI::iTunes::iTunesModule module;
			if(GetModuleHandleW(L"iTunesMobileDevice.dll")!=NULL){
				return true;
			}
			return (LoadLibraryW(module.itunes_mobile_device_dll().c_str())!=NULL);
		}
	}
}