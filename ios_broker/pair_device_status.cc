#include "ios_broker/pair_device_status.h"
#include "ios_broker/pair_device_app.h"
#include "PhoneCore/interface/unque_pointer.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/notification_proxy.h>

namespace ios_broker{
	namespace internal{
		class PermissionEscape:public sdk::ShowApp
		{
		public:
			PermissionEscape():is_escape_device_(false){}
			virtual void AppDetail(const char* udid, const char* appid, const char* detail, uint64_t dynamic_disk_usage, uint64_t static_disk_usage){
				std::string app_com_id = appid;
				if(app_com_id=="com.saurik.Cydia"||app_com_id=="com.pangu.ipa1"){
					is_escape_device_ = true;
				}
			}
			virtual void AppTotal(size_t count){

			}
			bool is_escape_device() const{
				return is_escape_device_;
			}
		private:
			bool is_escape_device_;
		};
	}
	PairDeviceStatus::PairDeviceStatus(const char* udid):is_cracked_device_(false){
		is_cracked_device_ = IsPermissionEscapeDevice(udid);
	}
	PairDeviceStatus::~PairDeviceStatus(void){
		is_cracked_device_ = false;
	}
	bool PairDeviceStatus::IsJailbrokenDevice() const{
		return is_cracked_device_;
	}
	bool PairDeviceStatus::IsAlreadyActivationDevice(const char* udid){
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		if (IDEVICE_E_SUCCESS != idevice_new(&phone, udid)) {
			return false;
		}
		if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(phone, &client, "ideviceclient")) {
			idevice_free(phone);
			return false;
		}
		plist_t pl = NULL;
		bool device_not_activated = false;
		lockdownd_get_value(client, NULL, "ActivationState", &pl);
		if (pl && plist_get_node_type(pl) == PLIST_STRING) { 
			char *as = NULL;
			plist_get_string_val(pl, &as);
			plist_free(pl);
			if (as){
				if(strcmp(as,"Unactivated") == 0) {
					device_not_activated = true;
				}
				free(as);
			}
		}
		lockdownd_client_free(client);
		idevice_free(phone);
		return device_not_activated;
	}
	bool PairDeviceStatus::IsBackupHavePassword(const char* udid){
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		if (IDEVICE_E_SUCCESS != idevice_new(&phone, udid)) {
			return false;
		}
		if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(phone, &client, "ideviceclient")) {
			idevice_free(phone);
			return false;
		}
		plist_t pl = NULL;
		bool backup_password_exist = false;
		lockdownd_get_value(client, "com.apple.mobile.backup", "WillEncrypt", &pl);
		if (pl && plist_get_node_type(pl) == PLIST_STRING) { 
			unsigned char as = 0;
			plist_get_bool_val(pl, &as);
			plist_free(pl);
			if (as) {
				backup_password_exist = true;
			}
		}
		lockdownd_client_free(client);
		idevice_free(phone);
		return backup_password_exist;
	}
	bool PairDeviceStatus::IsPermissionEscapeDevice(const char* udid){
		/*
			follow example can ignone:
			internal::PermissionEscape permission;
			ios_broker::PairDeviceApp::GetInstance()->InitializeBroker();
			ios_broker::PairDeviceApp::GetInstance()->set_list_apps_mode(1);
			sdk::ShowApp* orig_api = sdk::pointer::GetInstance()->ibroker_nterface();
			sdk::pointer::GetInstance()->set_ibroker_nterface(&permission);
			ios_broker::PairDeviceApp::GetInstance()->InstallBroker(udid,NULL,",list_all");
			if(permission.is_escape_device()){
			sdk::pointer::GetInstance()->set_ibroker_nterface(orig_api);
			return true;
			}
			sdk::pointer::GetInstance()->set_ibroker_nterface(orig_api);
		*/
		return IsExistPermissionEscapePath(udid);
	}
	bool PairDeviceStatus::IsExistPermissionEscapePath(const char* udid){
		unsigned int bytes = 0;
		lockdownd_service_descriptor_t port = 0, i = 0;
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		bool jailbroken_status = false;
		if (IDEVICE_E_SUCCESS != idevice_new(&phone, udid)) {
			return false;
		}
		if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(phone, &client, "ideviceclient")) {
			idevice_free(phone);
			return false;
		}
		for(bool once_1 = false;!once_1;once_1=true){
			lockdownd_start_service(client, "com.apple.afc2", &port);
			if (port) {
				jailbroken_status = true;
				afc_client_t afc = NULL;
				afc_client_new(phone, port, &afc);
				if (afc) {
					char **fileinfo = NULL;
					afc_get_file_info(afc, "/Applications", &fileinfo);
					if (fileinfo){
						int i;
						for (i = 0; fileinfo[i]; i += 2) {
							if (!strcmp(fileinfo[i], "st_ifmt")) {
								if (strcmp(fileinfo[i + 1], "S_IFLNK") == 0) {
									jailbroken_status = true;//jailbroken_device
									break;
								}
							}
						}
						if (fileinfo) {
							int i = 0;
							while (fileinfo[i]) {
								free(fileinfo[i]);
								i++;
							}
							free(fileinfo);
							fileinfo = NULL;
						}
					}
					else{
						afc_get_file_info(afc, "/private/etc/launchd.conf", &fileinfo);
						if(fileinfo) {
							free(fileinfo);
							jailbroken_status = true;//jailbroken_device
						}
					}
					afc_client_free(afc);
				}
			}
			break;
		}
		if(port){
			lockdownd_service_descriptor_free(port);
		}
		lockdownd_client_free(client);
		idevice_free(phone);
		return jailbroken_status;
	}
}