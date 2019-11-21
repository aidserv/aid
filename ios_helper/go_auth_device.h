#ifndef IOS_HELPER_GO_AUTH_DEVICE_H_
#define IOS_HELPER_GO_AUTH_DEVICE_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_helper/atl_dll_main.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_helper{
	class WIN_DLL_API GoAuthDevice
	{
	public:
		GoAuthDevice(void){}
		virtual ~GoAuthDevice(void){}
		bool WaitAuthDeviceAppUserCertOK(const char* udid);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(GoAuthDevice);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif