#include "ios_broker/pair_device_trust.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include "ABI/thirdparty/glog/logging.h"

namespace ios_broker{
	namespace internal{
		void message(const char* format,...){
			char buffer[1024] = {0};
			va_list args;
			va_start(args,format);
			vsnprintf(buffer,1024,format,args);
			LOG(INFO)<<buffer<<std::endl;
			va_end (args);
		}
		void print_error_message(const char* udid,lockdownd_error_t err){
			switch (err) {
			case LOCKDOWN_E_PASSWORD_PROTECTED:
				message("ERROR: Could not validate with device %s because a passcode is set. Please enter the passcode on the device and retry.\n", udid);
				break;
			case LOCKDOWN_E_INVALID_HOST_ID:
				message("ERROR: Device %s is not paired with this host\n", udid);
				break;
			case LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING:
				message("ERROR: Please accept the trust dialog on the screen of device %s, then attempt to pair again.\n", udid);
				break;
			case LOCKDOWN_E_USER_DENIED_PAIRING:
				message("ERROR: Device %s said that the user denied the trust dialog.\n", udid);
				break;
			default:
				message("ERROR: Device %s returned unhandled error code %d\n", udid, err);
				break;
			}
		}
	}
	PairDeviceTrust::PairDeviceTrust(void){
	}
	PairDeviceTrust::~PairDeviceTrust(void){
	}
	bool PairDeviceTrust::MobileDeviceOK(const char* udid){
		idevice_t phone = NULL;
		idevice_error_t ret = idevice_new(&phone, udid);
		if(ret != IDEVICE_E_SUCCESS) {
			return false;
		}
		idevice_free(phone);
		return true;
	}
	bool PairDeviceTrust::IsUnlockDevice(const char* udid){
		idevice_t phone = NULL;
		lockdownd_client_t client = NULL;
		idevice_error_t ret = idevice_new(&phone,udid);
		if(ret != IDEVICE_E_SUCCESS) {
			return false;
		}
		lockdownd_error_t devcie_status = lockdownd_client_new_with_handshake(phone, &client,"IsUnlockDevice");
		if(ret != LOCKDOWN_E_SUCCESS){
			if (devcie_status == LOCKDOWN_E_PASSWORD_PROTECTED){
				LOG(ERROR)<<"Please disable the password protection on your device and try again."<<std::endl;
			} 
			else if (devcie_status == LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING) {
				LOG(ERROR)<<"Please dismiss the trust dialog on your device and try again."<<std::endl;
			} 
			else {
				LOG(ERROR)<<"Failed to connect to lockdownd service on the device."<<std::endl;
			}
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		}
		if(client){
			lockdownd_client_free(client);
		}
		idevice_free(phone);
		return true;
	}
	bool PairDeviceTrust::PairComputer(const char* udid){
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
		lockdownd_error_t lerr;
		char *type = NULL;
		ret = idevice_new(&phone, udid);
		if(ret != IDEVICE_E_SUCCESS) {
			return false;
		}
		lerr = lockdownd_client_new_with_handshake(phone, &client, "PairComputer");
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		}
		lerr = lockdownd_query_type(client, &type);
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		} 
		else {
			if(type){
				free(type);
			}
			lerr = lockdownd_pair(client, NULL);
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			if (lerr == LOCKDOWN_E_SUCCESS) {
				return true;
			} 
			else{
				internal::print_error_message(udid,lerr);
				return false;
			}
		}
	}
	bool PairDeviceTrust::ValidateComputer(const char* udid){
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
		lockdownd_error_t lerr;
		char *type = NULL;
		ret = idevice_new(&phone, udid);
		if(ret != IDEVICE_E_SUCCESS) {
			return false;
		}
		lerr = lockdownd_client_new_with_handshake(phone,&client,"ValidateComputer");
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		}
		lerr = lockdownd_query_type(client, &type);
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		} 
		else {
			if(type){
				free(type);
			}
			lerr = lockdownd_validate_pair(client,NULL);
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			if (lerr == LOCKDOWN_E_SUCCESS) {
				return true;
			} 
			else {
				internal::print_error_message(udid,lerr);
				return false;
			}
		}
	}
	bool PairDeviceTrust::UnpairComputer(const char* udid){
		lockdownd_client_t client = NULL;
		idevice_t phone = NULL;
		idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
		lockdownd_error_t lerr;
		char *type = NULL;
		ret = idevice_new(&phone, udid);
		if(ret != IDEVICE_E_SUCCESS) {
			return false;
		}
		lerr = lockdownd_client_new_with_handshake(phone, &client, "UnpairComputer");
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		}
		lerr = lockdownd_query_type(client, &type);
		if (lerr != LOCKDOWN_E_SUCCESS) {
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			return false;
		} 
		else {
			if(type){
				free(type);
			}
			lerr = lockdownd_unpair(client, NULL);
			if(client){
				lockdownd_client_free(client);
			}
			idevice_free(phone);
			if (lerr == LOCKDOWN_E_SUCCESS) {
				return true;
			} 
			else {
				internal::print_error_message(udid,lerr);
				return false;
			}
		}
	}
	void PairDeviceTrust::GetPhysicalUDID(){
		char* device_name = nullptr;
		idevice_t device = NULL;
		lockdownd_client_t client = NULL;
		const char* udid = NULL;
		idevice_new(&device, udid);
		if (!device){
			return;
		}
		if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(device, &client, "idevice_id")) {
			idevice_free(device);
			return;
		}
		if ((LOCKDOWN_E_SUCCESS != lockdownd_get_device_name(client, &device_name)) || !device_name) {
			idevice_free(device);
			return;
		}
		lockdownd_client_free(client);
		idevice_free(device);
	}
}