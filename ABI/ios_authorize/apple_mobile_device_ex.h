#ifndef ABI_IOS_AUTHORIZE_DEVICE_EX_H_
#define ABI_IOS_AUTHORIZE_DEVICE_EX_H_
//////////////////////////////////////////////////////////////////////////
#include <fstream>
#include "ABI/ios_authorize/apple_import.h"
#include "ABI/ios_authorize/Universal.h"
#include "ABI/base/file/path.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		class AppleMobileDeviceEx
		{
		public:
			AppleMobileDeviceEx(void* handle):device_handle_(NULL){
				set_device_handle(handle);
				set_product_type(0);
				set_fair_play_certificate(NULL);
				set_fair_play_certificate_len(0);
				set_fair_play_guid(NULL);
				set_key_fair_play_guid(0);
				set_fair_play_device_type(0);
				set_unique_device_id(NULL);
				set_grappa_sessionid(NULL);
				set_key_fair_play_guid_length(0);
			}
			~AppleMobileDeviceEx(){
				set_device_handle(NULL);
				set_product_type(0);
				if(fair_play_certificate()!=NULL){
					unsigned char* fair = fair_play_certificate();
					free(fair);

				}
				set_fair_play_certificate(NULL);
				set_fair_play_certificate_len(0);
				if(fair_play_guid()!=NULL){
					char* fair = fair_play_guid();
					free(fair);
				}
				set_fair_play_guid(NULL);
				set_key_fair_play_guid(0);
				set_fair_play_device_type(0);
				if(unique_device_id()!=NULL){
					char* fair = unique_device_id();
					free(fair);
				}
				set_unique_device_id(NULL);
				set_grappa_sessionid(NULL);
				set_key_fair_play_guid_length(0);
			}
			void GenFairDeviceInfo(){
				__int64 fair_device_type = 0;
				CFIndex len = 0;
				CFStringRef sKey = CFStringCreateWithCString(NULL,"UniqueDeviceID",kCFStringEncodingUTF8);
				CFStringRef sValue = AMDeviceCopyValue(device_handle(),NULL,sKey);
				CFRelease(sKey);
				if(sValue){
					len = CFStringGetLength(sValue);
					set_unique_device_id((char *)malloc(len+1));
					CFStringGetCString(sValue,unique_device_id(),len+1,kCFStringEncodingUTF8);
					CFRelease(sValue);
				}
				CFStringRef sDomain = CFStringCreateWithCString(NULL,"com.apple.mobile.iTunes",kCFStringEncodingUTF8);
				sKey = nullptr;
				sKey = CFStringCreateWithCString(NULL,"FairPlayCertificate",kCFStringEncodingUTF8);
				sValue = nullptr;
				sValue = AMDeviceCopyValue(device_handle(),sDomain,sKey);
				CFRelease(sKey);
				set_fair_play_certificate_len(CFDataGetLength(sValue));
				set_fair_play_certificate((unsigned char *)malloc(fair_play_certificate_len()));
				CFDataGetBytes(sValue,CFRangeMake(0,fair_play_certificate_len()),fair_play_certificate());
				CFRelease(sValue);
				sKey = nullptr;
				sKey = CFStringCreateWithCString(NULL,"FairPlayGUID",kCFStringEncodingUTF8);
				sValue = nullptr;
				sValue = AMDeviceCopyValue(device_handle(),sDomain,sKey);
				CFRelease(sKey);
				len = CFStringGetLength(sValue);
				set_fair_play_guid((char *)malloc(len + 1));
				CFStringGetCString(sValue,fair_play_guid(),len+1,kCFStringEncodingUTF8);
				CFRelease(sValue);
				set_key_fair_play_guid_length(len/2 + 5);
				set_key_fair_play_guid((char *)malloc(key_fair_play_guid_length()));
				*(unsigned long*)key_fair_play_guid() = len/2;
				for(int i =0;i<len/2;i++){
					StringToHex(fair_play_guid()+i*2,(BYTE*)key_fair_play_guid()+4+i);
				}
				sKey = nullptr;
				sKey = CFStringCreateWithCString(NULL,"FairPlayDeviceType",kCFStringEncodingUTF8);
				sValue = nullptr;
				sValue = AMDeviceCopyValue(device_handle(),sDomain,sKey);
				CFRelease(sKey);
				if(sValue){
					CFNumberGetValue(sValue,kCFNumberSInt32Type,&fair_device_type);
					CFRelease(sValue);
					sKey = CFStringCreateWithCString(NULL,"KeyTypeSupportVersion",kCFStringEncodingUTF8);
					sValue = AMDeviceCopyValue(device_handle(),sDomain,sKey);
					CFRelease(sKey);
					CFNumberGetValue(sValue,kCFNumberSInt32Type,(PVOID)(((DWORD)&fair_device_type)+4));
					CFRelease(sValue);
					set_fair_play_device_type(fair_device_type);
				}
			}
			void* device_handle() const{
				return device_handle_;
			}
			unsigned long product_type() const{
				return product_type_;
			}
			unsigned char* fair_play_certificate() const{
				return fair_play_certificate_;
			}
			unsigned long fair_play_certificate_len() const{
				return fair_play_certificate_len_;
			}
			char* fair_play_guid() const{
				return fair_play_guid_;
			}
			void* key_fair_play_guid() const{
				return key_fair_play_guid_;
			}
			__int64 fair_play_device_type() const{
				return fair_play_device_type_;
			}
			char* unique_device_id() const{
				return unique_device_id_;
			}
			unsigned long key_fair_play_guid_length() const{
				return key_fair_play_guid_len_;
			}
			unsigned long grappa_session_id(){
				return grappa_sessionid_;
			}
			void set_grappa_sessionid(ATHRef ath){
				if(ath!=NULL){
					grappa_sessionid_ = ATHostConnectionGetGrappaSessionId(ath);
				}
				else{
					grappa_sessionid_ = NULL;
				}
			}
		private:
			void set_device_handle(void* handle){
				device_handle_ = handle;
			}
			void set_product_type(unsigned long type){
				product_type_ = type;
			}
			void set_fair_play_certificate(unsigned char* certificate){
				fair_play_certificate_ = certificate;
			}
			void set_fair_play_certificate_len(unsigned long certificate_len){
				fair_play_certificate_len_ = certificate_len;
			}
			void set_fair_play_guid(char* guid){
				fair_play_guid_ = guid;
			}
			void set_key_fair_play_guid(void* play_guid){
				key_fair_play_guid_ = play_guid;
			}
			void set_key_fair_play_guid_length(unsigned long length){
				key_fair_play_guid_len_ = length;
			}
			void set_fair_play_device_type(__int64 device_type){
				fair_play_device_type_ = device_type;
			}
			void set_unique_device_id(char* device_id){
				unique_device_id_ = device_id;
			}
			void* device_handle_;
			unsigned long product_type_;
			unsigned char* fair_play_certificate_;
			unsigned long fair_play_certificate_len_;
			char* fair_play_guid_;
			unsigned long key_fair_play_guid_len_;
			void* key_fair_play_guid_;
			__int64 fair_play_device_type_;
			char* unique_device_id_;
			unsigned long grappa_sessionid_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif