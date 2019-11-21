#include "ios_broker/pair_connectioned.h"
#include "ABI/base/string/string_conv.h"

namespace ios_broker{
	namespace internal{
		PairConnectioned::PairConnectioned(void):device_handle_(NULL),client_handle_(NULL){
			EndConnectioned();
		}
		PairConnectioned::PairConnectioned(const char* udid,const char* lable):device_handle_(NULL),client_handle_(NULL){
			if(udid!=NULL&&lable!=NULL){
				BeginConnectioned(udid,lable);
			}
		}
		PairConnectioned::~PairConnectioned(void){
			EndConnectioned();
		}
		bool PairConnectioned::BeginConnectioned(const char* udid,const char* lable){
			if(udid!=NULL){
				lockdownd_error_t ldret = LOCKDOWN_E_UNKNOWN_ERROR;
				if(IDEVICE_E_SUCCESS != idevice_new(&device_handle_, udid)){
					return false;
				}
				if(LOCKDOWN_E_SUCCESS != (ldret = lockdownd_client_new_with_handshake(device_handle_, &client_handle_,lable))){
					return false;
				}
				return true;
			}
			else{
				return false;
			}
		}
		void PairConnectioned::EndConnectioned(){
			if(client_handle_){
				lockdownd_client_free(client_handle_);
				client_handle_ = NULL;
			}
			if(device_handle_){
				idevice_free(device_handle_);
				device_handle_ = NULL;
			}
		}
		std::wstring PairConnectioned::GetName(){
			char *devname = NULL;
			std::wstring name = L"";
			if((LOCKDOWN_E_SUCCESS == lockdownd_get_device_name(client_handle(), &devname))&&devname){
				name = ABI::base::Utf8ToUnicode(devname);
			}
			if(devname){
				free(devname);
			}
			return name;
		}
		std::wstring PairConnectioned::GetProperty(const char* name,const char* domain,const PropertyVariantStringType& type){
			plist_t pver = NULL;
			char *result_string = NULL;
			lockdownd_get_value(client_handle(),domain,name,&pver);
			if (pver && plist_get_node_type(pver) == PLIST_STRING){
				plist_get_string_val(pver,&result_string);
			}
			plist_free(pver);
			if(result_string){
				std::wstring result = ABI::base::AsciiToUnicode(result_string);
				free(result_string);
				return result;
			}
			else{
				return L"";
			}
		}
		uint64 PairConnectioned::GetProperty(const char* name,const char* domain,const PropertyVariantUintType& type){
			plist_t pver = NULL;
			uint64 u_result = 0;
			lockdownd_get_value(client_handle(),domain,name,&pver);
			if (pver && plist_get_node_type(pver) == PLIST_UINT){
				plist_get_uint_val(pver,&u_result);
			}
			plist_free(pver);
			return u_result;
		}
		bool PairConnectioned::GetProperty(const char* name,const char* domain,const PropertyVariantBoolType& type){
			plist_t pver = NULL;
			uint8 b_result = 0;
			lockdownd_get_value(client_handle(),domain,name,&pver);
			if (pver && plist_get_node_type(pver) == PLIST_BOOLEAN){
				plist_get_bool_val(pver,&b_result);
			}
			plist_free(pver);
			return (b_result==1);
		}
		idevice_t PairConnectioned::device_handle(){
			return device_handle_;
		}
		lockdownd_client_t PairConnectioned::client_handle(){
			return client_handle_;
		}
	}
}