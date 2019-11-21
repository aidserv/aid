#include "ios_cracker/apple_auth_device.h"
#include "ios_cracker/apple_support.h"
#include "ios_cracker/apple_ssl.h"
#include "ABI/ios_authorize/itunes_module.h"
#include "ABI/ios_authorize/itunes_module_state.h"
#include "ABI/ios_authorize/apple_device_auth.h"
#include "ABI/itunes_reverse/itunes_abi.h"
#include "ABI/base/basetypes.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_conv.h"

namespace ios_cracker{
	AppleAuthorizeDevice::AppleAuthorizeDevice(){
	}
	AppleAuthorizeDevice::~AppleAuthorizeDevice(void){
		ABI::iTunes::iTunesModuleState module_state;
		if(!module_state.iTunesIsInstalled()){
			MessageBoxW(GetActiveWindow(),L"iTunes install failed!",L"",MB_ICONWARNING);
		}
		if(!module_state.AppleApplicationSupportIsInstalled()){
			MessageBoxW(GetActiveWindow(),L"AppleApplicationSupport install failed!",L"",MB_ICONWARNING);
		}
		if(!module_state.AppleMobileDeviceSupportIsInstalled()){
			MessageBoxW(GetActiveWindow(),L"AppleMobileDeviceSupport install failed!",L"",MB_ICONWARNING);
		}
	}
	void AppleAuthorizeDevice::iTunesMobileDeviceComponentSetup(){
		ios_cracker::internal::ThirdSupport third;
		ABI::iTunes::iTunesModuleState module_state;
		if(!module_state.AppleApplicationSupportIsInstalled()){
			third.RunSupportApp(third.AppleApplicationSupportMSI(),L"/qn");
		}
		if(!module_state.AppleMobileDeviceSupportIsInstalled()){
			third.RunSupportApp(third.AppleMobileDeviceSupportMSI(),L"/qn");
		}
		if(!module_state.iTunesIsInstalled()){
			third.RunSupportApp(third.iTunesSetupPackages(),L"/qn");
		}
	}
	bool AppleAuthorizeDevice::AuthorizeMobileDevice(const char* udid){
		bool state = true;
		ABI::internal::WaitDeviceUDID(udid);
		ABI::internal::ConnectIOSDevice(deviceHandleConnected);
		if(ABI::internal::AuthorizeDevice(deviceHandleConnected)!=ABI::internal::authorize_ok){
			state = false;
		}
		ABI::internal::CloseIOSDevice(deviceHandleConnected);
		return state;
	}
}