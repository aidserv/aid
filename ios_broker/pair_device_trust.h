#ifndef IOS_BROKER_PAIR_DEVICE_TRUST_H_
#define IOS_BROKER_PAIR_DEVICE_TRUST_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceTrust
	{
	public:
		PairDeviceTrust(void);
		~PairDeviceTrust(void);
		bool MobileDeviceOK(const char* udid);
		bool IsUnlockDevice(const char* udid);
		bool PairComputer(const char* udid);
		bool ValidateComputer(const char* udid);
		bool UnpairComputer(const char* udid);
		void GetPhysicalUDID();
	private:
		DISALLOW_EVIL_CONSTRUCTORS(PairDeviceTrust);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif
