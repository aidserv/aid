#ifndef IOS_BROKER_PAIR_DEVICE_MOUNTER_H_
#define IOS_BROKER_PAIR_DEVICE_MOUNTER_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceShutdown
	{
	public:
		PairDeviceShutdown(){}
		virtual ~PairDeviceShutdown(){}
		bool ShutdownDevice(const char* udid);
		bool RestartDevice(const char* udid);
		bool SleepDevice(const char* udid);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(PairDeviceShutdown);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif