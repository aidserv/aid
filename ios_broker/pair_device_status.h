#ifndef IOS_BROKER_PAIR_DEVICE_STATUS_H_
#define IOS_BROKER_PAIR_DEVICE_STATUS_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceStatus
	{
	public:
		PairDeviceStatus():is_cracked_device_(false){}
		explicit PairDeviceStatus(const char* udid);
		~PairDeviceStatus(void);
		bool IsJailbrokenDevice() const;
		bool IsAlreadyActivationDevice(const char* udid);
		bool IsBackupHavePassword(const char* udid);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(PairDeviceStatus);
		bool IsPermissionEscapeDevice(const char* udid);
		bool IsExistPermissionEscapePath(const char* udid);
		bool is_cracked_device_;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif
