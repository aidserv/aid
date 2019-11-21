#ifndef IOS_BROKER_PAIR_DEVICE_SNAPSHOT_H_
#define IOS_BROKER_PAIR_DEVICE_SNAPSHOT_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceSnapshot{
	public:
		PairDeviceSnapshot(void);
		explicit PairDeviceSnapshot(const char* filename);
		virtual ~PairDeviceSnapshot(void);
		void AddSavePath(const char* filename);
		bool SnapshotDevice(const char* udid,const char* mounted_developer_plugin,bool jailbroken_status);
	private:
		DISALLOW_EVIL_CONSTRUCTORS(PairDeviceSnapshot);
		char* filename_;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif