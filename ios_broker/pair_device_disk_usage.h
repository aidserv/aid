#ifndef IOS_BROKER_PAIR_DEVICE_DISK_USAGE_H_
#define IOS_BROKER_PAIR_DEVICE_DISK_USAGE_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceDiskUsage
	{
	public:
		PairDeviceDiskUsage(void);
		~PairDeviceDiskUsage(void);
		void ToDiskUsageAnalyized(const char* udid);
		void ShowFormat(char* buf,size_t length,double bytes);
		double PercentTotalByte();
		double PercentFreeByte();
		double PercentAudioByte();
		double PercentVideoByte();
		double PercentCameraByte();
		double PercentApplicationByte();
		double PercentOtherByte();
	private:
		uint64 data_total_;
		uint64 data_free_;
		uint64 camera_usage_;
		uint64 app_usage_;
		uint64 other_usage_; 
		uint64 disk_total_;
		uint64 audio_usage_;
		uint64 video_usage_;
		DISALLOW_EVIL_CONSTRUCTORS(PairDeviceDiskUsage);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif
