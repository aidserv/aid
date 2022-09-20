#ifndef ABI_IOS_AUTHORIZE_RS_GEN_H_
#define ABI_IOS_AUTHORIZE_RS_GEN_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
#include "ABI/ios_authorize/iTunesInterface.h"
#include "passport/itunes_client_interface.h"
#include "passport/itunes_native_interface.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		class RSKeyGen
		{
		public:
			explicit RSKeyGen(){
				set_data(0);
				set_length(0);
				set_grappa_session_id(0);
				set_fair_play_certificate(NULL);
				set_fair_play_certificate_len(0);
				set_key_fair_play_guid(NULL);
				set_fair_play_device_type(NULL);
			}
			~RSKeyGen(){
				reset();
			}
			void reset(){
				if(data()!=0){
					free(data());
					set_data(0);
				}
				set_length(0);
				set_grappa_session_id(0);
				set_fair_play_certificate(NULL);
				set_fair_play_certificate_len(0);
				set_key_fair_play_guid(NULL);
				set_fair_play_device_type(NULL);
			}
			unsigned char* data() const{
				return data_;
			}
			unsigned long length() const{
				return length_;
			}
			unsigned long grappa_session_id() const{
				return grappa_session_id_;
			}
			unsigned char *fair_play_certificate() const{
				return fair_play_certificate_;
			}
			unsigned long fair_play_certificate_len() const{
				return fair_play_certificate_len_;
			}
			void* key_fair_play_guid()const {
				return key_fair_play_guid_;
			}
			__int64 fair_play_device_type()const {
				return fair_play_device_type_;
			}
			void set_grappa_session_id(unsigned long session_id){
				grappa_session_id_ = session_id;
			}
			void set_fair_play_certificate(unsigned char* certificate){
				fair_play_certificate_ = certificate;
			}
			void set_fair_play_certificate_len(unsigned long certificate_len){
				fair_play_certificate_len_ = certificate_len;
			}
			void set_key_fair_play_guid(void* guid){
				key_fair_play_guid_ = guid;
			}
			void set_fair_play_device_type(__int64 device_type){
				fair_play_device_type_ = device_type;
			}
			void set_data(unsigned char* auto_data){
				data_ = auto_data;
			}
			void set_length(unsigned long auto_length){
				length_ = auto_length;
			}
			static RSKeyGen* GetInstance(){
				static RSKeyGen* info;
				if(!info){
					RSKeyGen* new_info = new RSKeyGen();
					if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
						delete new_info;
					}
				}
				return info;
			}
		private:
			unsigned char *data_;
			unsigned long length_;
			unsigned long grappa_session_id_;
			unsigned char *fair_play_certificate_;
			unsigned long fair_play_certificate_len_;
			void* key_fair_play_guid_;
			__int64 fair_play_device_type_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif