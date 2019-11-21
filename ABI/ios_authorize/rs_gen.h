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
			unsigned long GenPrivateKey(unsigned char* rq_data,unsigned long rq_length,unsigned char* rq_sig_data,unsigned long rq_sig_length){
				unsigned long private_key = 0;
				passport::internal::KbSyncIdParameter::Initialize();
				passport::iTunesNativeInterface::GetInstance()->Init();
				unsigned long kb_sync_id = GenerateKbsyncID(passport::internal::KbSyncIdParameter::GetKbsyncIDAddress(),
					passport::internal::KbSyncIdParameter::AllPCMd5(),
					passport::internal::KbSyncIdParameter::LocalPCMd5());
				__int64 device_type = fair_play_device_type();
				if(VerifyAfsyncRq_8(grappa_session_id(),rq_data,rq_length,rq_sig_data,rq_sig_length)!=0){
					return 0;
				}
				//GeneratePrivateKey_8(kb_sync_id,fair_play_certificate(),fair_play_certificate_len(),key_fair_play_guid(),&device_type,7,&private_key);
				return kb_sync_id;
			}
			void GenRS(Int64Make* dsid,size_t dsid_count,unsigned char* rq_data,unsigned long rq_length,
				unsigned char* rq_sig_data,unsigned long rq_sig_length,bool ignore_verify = true){
				DWORD PrivateKey = 0;
				char* pszDisdList = 0;
				DWORD dwDisdCount = 0;
				char* pszICInfoData = 0;
				DWORD dwICInfoSize = 0;
				passport::internal::KbSyncIdParameter::Initialize();
				passport::iTunesNativeInterface::GetInstance()->Init();
				//dsid ---> authorized computer
				DWORD dwKbsyncID = GenerateKbsyncID(passport::internal::KbSyncIdParameter::GetKbsyncIDAddress(),
					passport::internal::KbSyncIdParameter::AllPCMd5(),
					passport::internal::KbSyncIdParameter::LocalPCMd5());
				if(ignore_verify){
					unsigned long ret = VerifyAfsyncRq_8(grappa_session_id(),rq_data,rq_length,rq_sig_data,rq_sig_length);
					if(ret!=0){
						return;
					}
				}
				__int64 device_type = fair_play_device_type();
				GeneratePrivateKey_8(dwKbsyncID,fair_play_certificate(),fair_play_certificate_len(),key_fair_play_guid(),&device_type,7,&PrivateKey);
				unsigned long ret = SetAfSyncRq_8(PrivateKey,(const char*)rq_data,rq_length,0,0);
				ret = ParseAfsyncRq_8(0, key_fair_play_guid(), (DWORD)fair_play_device_type(), (const char*)rq_data, rq_length, (const char**)&pszDisdList, &dwDisdCount);
				for(size_t i=0;i<dsid_count;i++){
					ret = AuthKeyByDSID_8(PrivateKey,dsid[i].low,dsid[i].high,0,0);
				}
				unsigned char* afsync_rs = NULL;
				unsigned long afsync_rs_size = NULL;
				ret = ParseAfsyncRq_8(PrivateKey,key_fair_play_guid(),(DWORD)fair_play_device_type(),0,0,(const char**)&pszDisdList,&dwDisdCount);
				ret = GenerateAfSyncRs_8(PrivateKey,(const char**)&afsync_rs,&afsync_rs_size,(const char**)&pszICInfoData,&dwICInfoSize);
				set_data(afsync_rs);
				set_length(afsync_rs_size);
			}
			void GenRS(Int64Make* dsid,size_t dsid_count,unsigned char* rq_data,unsigned long rq_length,
				unsigned char* rq_sig_data,unsigned long rq_sig_length,unsigned long private_key,bool reload_flag){
				char* pszDisdList = 0;
				DWORD dwDisdCount = 0;
				char* pszICInfoData = 0;
				DWORD dwICInfoSize = 0;
				DWORD PrivateKey = 0;
				__int64 device_type = fair_play_device_type();
				GeneratePrivateKey_8(private_key,fair_play_certificate(),fair_play_certificate_len(),key_fair_play_guid(),&device_type,7,&PrivateKey);
				unsigned long ret = SetAfSyncRq_8(PrivateKey,(const char*)rq_data,rq_length,0,0);
				ret = ParseAfsyncRq_8(0,key_fair_play_guid(),(DWORD)fair_play_device_type(),(const char*)rq_data,rq_length,(const char**)&pszDisdList,&dwDisdCount);
				for(size_t i=0;i<dsid_count;i++){
					ret = AuthKeyByDSID_8(PrivateKey,dsid[i].low,dsid[i].high,0,0);
				}
				unsigned char* afsync_rs = NULL;
				unsigned long afsync_rs_size = NULL;
				ret = ParseAfsyncRq_8(PrivateKey,key_fair_play_guid(),(DWORD)fair_play_device_type(),0,0,(const char**)&pszDisdList,&dwDisdCount);
				ret = GenerateAfSyncRs_8(PrivateKey,(const char**)&afsync_rs,&afsync_rs_size,(const char**)&pszICInfoData,&dwICInfoSize);
				set_data(afsync_rs);
				set_length(afsync_rs_size);
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