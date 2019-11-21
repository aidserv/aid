#include "ios_broker/pair_device_disk_usage.h"
#include "ios_broker/pair_connectioned.h"

namespace ios_broker{
	PairDeviceDiskUsage::PairDeviceDiskUsage(void){
		data_total_ = 0;
		data_free_ = 0;
		camera_usage_ = 0;
		app_usage_ = 0;
		other_usage_ = 0; 
		disk_total_ = 0;
		audio_usage_ = 0;
		video_usage_ = 0;
	}
	PairDeviceDiskUsage::~PairDeviceDiskUsage(void){
		data_total_ = 0;
		data_free_ = 0;
		camera_usage_ = 0;
		app_usage_ = 0;
		other_usage_ = 0; 
		disk_total_ = 0;
		audio_usage_ = 0;
		video_usage_ = 0;
	}
	void PairDeviceDiskUsage::ToDiskUsageAnalyized(const char* udid){
		ios_broker::internal::PairConnectioned connected;
		connected.BeginConnectioned(udid,"disk_info");
		data_total_ = connected.GetProperty("TotalDataCapacity","com.apple.disk_usage",ios_broker::internal::PairConnectioned::kUintType);
		data_free_ = connected.GetProperty("TotalDataAvailable","com.apple.disk_usage",ios_broker::internal::PairConnectioned::kUintType);
		camera_usage_ = connected.GetProperty("CameraUsage","com.apple.disk_usage",ios_broker::internal::PairConnectioned::kUintType);
		app_usage_ = connected.GetProperty("MobileApplicationUsage","com.apple.disk_usage",ios_broker::internal::PairConnectioned::kUintType);
		disk_total_ = connected.GetProperty("TotalDiskCapacity","com.apple.disk_usage",ios_broker::internal::PairConnectioned::kUintType);
		other_usage_ = data_total_-data_free_-camera_usage_-app_usage_-audio_usage_-video_usage_;
		connected.EndConnectioned();
	}
	void PairDeviceDiskUsage::ShowFormat(char* buf,size_t length,double bytes){
		const char* suffixes[8] = {"B","KB","MB","GB","TB","PB","EB",0};
		uint64 div = 0;
		for(;bytes >= 1024 && div < (sizeof(suffixes) / sizeof(*suffixes));div++) {
			bytes /= 1024.00;
		}
		_snprintf(buf,MAX_PATH,"%.2f %s", bytes?(bytes*100+0.5)/100.0:bytes,suffixes[div]);
	}
	double PairDeviceDiskUsage::PercentTotalByte(){
		return ((double)data_total_);
	}
	double PairDeviceDiskUsage::PercentFreeByte(){
		return ((double)data_free_);
	}
	double PairDeviceDiskUsage::PercentAudioByte(){
		return ((double)audio_usage_);
	}
	double PairDeviceDiskUsage::PercentVideoByte(){
		return ((double)video_usage_);
	}
	double PairDeviceDiskUsage::PercentCameraByte(){
		return ((double)camera_usage_);
	}
	double PairDeviceDiskUsage::PercentApplicationByte(){
		return ((double)app_usage_);
	}
	double PairDeviceDiskUsage::PercentOtherByte(){
		return ((double)other_usage_);
	}
}