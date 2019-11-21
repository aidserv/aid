#ifndef IOS_CRACKER_APPLE_AUTH_DEVICE_H_
#define IOS_CRACKER_APPLE_AUTH_DEVICE_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_cracker/atl_dll_main.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_cracker{
	class WIN_DLL_API AppleAuthorizeDevice
	{
	public:
		AppleAuthorizeDevice();
		virtual ~AppleAuthorizeDevice(void);
		void iTunesMobileDeviceComponentSetup();
		bool AuthorizeMobileDevice(const char* udid);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(AppleAuthorizeDevice);
	};
	typedef std::auto_ptr<ios_cracker::AppleAuthorizeDevice> AppleAuthorizeDeviceAutoPtr;
}
//////////////////////////////////////////////////////////////////////////
#endif