#ifndef IOS_CRACKER_APPLE_CONNECTION_DEVICE_H_
#define IOS_CRACKER_APPLE_CONNECTION_DEVICE_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_cracker/atl_dll_main.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_cracker{
	class WIN_DLL_API ConnectionDevice
	{
	public:
		ConnectionDevice(const char* udid);
		virtual ~ConnectionDevice();
	private:
		DISALLOW_EVIL_CONSTRUCTORS(ConnectionDevice);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif