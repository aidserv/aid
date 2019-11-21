#include "ios_broker/pair_device_snapshot.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/screenshotr.h>
#include "ios_broker/pair_device_trust.h"
#include "ios_broker/pair_device_status.h"
#include "ios_broker/pair_device_mounter.h"
#include "ios_broker/pair_connectioned.h"

namespace ios_broker{
	PairDeviceSnapshot::PairDeviceSnapshot(void):filename_(NULL){
	}
	PairDeviceSnapshot::PairDeviceSnapshot(const char* filename){
		if(filename!=NULL){
			AddSavePath(filename);
		}
	}
	PairDeviceSnapshot::~PairDeviceSnapshot(void){
		if(filename_!=NULL){
			delete[] filename_;
			filename_ = NULL;
		}
	}
	void PairDeviceSnapshot::AddSavePath(const char* filename){
		if(filename!=NULL){
			size_t length = strlen(filename);
			if(filename_!=NULL){
				delete[] filename_;
				filename_ = NULL;
			}
			filename_ = new char[length+sizeof(char)];
			memset(filename_,0,length+sizeof(char));
			strcpy(filename_,filename);
		}
	}
	bool PairDeviceSnapshot::SnapshotDevice(const char* udid,const char* mounted_developer_plugin,bool jailbroken_status){
		bool result = false;
		lockdownd_error_t ldret = LOCKDOWN_E_UNKNOWN_ERROR;
		screenshotr_client_t shotr = NULL;
		internal::PairConnectioned connected(udid,"PairDeviceSnapshot");
		const std::string developer_dmg = std::string(mounted_developer_plugin)+std::string("\\DeveloperDiskImage.dmg");
		const std::string developer_dmg_sig = std::string(mounted_developer_plugin)+std::string("\\DeveloperDiskImage.dmg.signature");
		if(!jailbroken_status){
			ios_broker::Mounter(false,&connected,developer_dmg.c_str(),developer_dmg_sig.c_str(),NULL);
		}
		lockdownd_service_descriptor_t service = NULL;
		if((lockdownd_start_service(connected.client_handle(),"com.apple.mobile.screenshotr",&service) != LOCKDOWN_E_SUCCESS) || !service->port){
			connected.EndConnectioned();
			return false;
		}
		if(screenshotr_client_new(connected.device_handle(),service,&shotr) != SCREENSHOTR_E_SUCCESS){
			lockdownd_service_descriptor_free(service);
			connected.EndConnectioned();
			return false;
		} 
		else{
			char *imgdata = NULL;
			uint64_t imgsize = 0;
			if(screenshotr_take_screenshot(shotr,&imgdata,&imgsize)==SCREENSHOTR_E_SUCCESS){
				FILE *f = fopen(filename_,"wb");
				if(f){
					if(fwrite(imgdata,1,(size_t)imgsize,f)==(size_t)imgsize){
						result = true;
					}
					fclose(f);
				}
			}
			if(imgdata!=NULL){
				free(imgdata);
			}
			screenshotr_client_free(shotr);
		}
		if(service){
			lockdownd_service_descriptor_free(service);
		}
		connected.EndConnectioned();
		return result;
	}
}