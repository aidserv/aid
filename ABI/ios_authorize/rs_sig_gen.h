#ifndef ABI_IOS_AUTHORIZE_RS_SIG_GEN_H_
#define ABI_IOS_AUTHORIZE_RS_SIG_GEN_H_
//////////////////////////////////////////////////////////////////////////
#include "abi/ios_authorize/itunes_internal_abi.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		class RSSigKeyGen
		{
		public:
			explicit RSSigKeyGen(){
				set_data(0);
				set_length(0);
			}
			~RSSigKeyGen(){
				reset();
			}
			void reset(){
				if(data()!=0){
					free(data());
					set_data(0);
				}
				if(length()!=0){
					set_length(0);
				}
			}
			unsigned char* data() const{
				return data_;
			}
			unsigned long length() const{
				return length_;
			}
			void GenSignature(const unsigned long& session_id,const unsigned char* data_1,const unsigned long length_1){
				unsigned char *test = 0;
				unsigned long test_length = 0;
				int (__cdecl *gen_afsync_rs_sig)(unsigned long,const unsigned char*,const unsigned long,unsigned char*,unsigned long*);
				*reinterpret_cast<unsigned long*>(&gen_afsync_rs_sig) = ABI::internal::iTunesInternalABI::GetITunesAddress();
				gen_afsync_rs_sig(session_id,const_cast<unsigned char*>(data_1),length_1,reinterpret_cast<unsigned char*>(&test),&test_length);
				set_data(test);
				set_length(test_length);
			}
			static RSSigKeyGen* GetInstance(){
				static RSSigKeyGen* info;
				if(!info){
					RSSigKeyGen* new_info = new RSSigKeyGen();
					if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
						delete new_info;
					}
				}
				return info;
			}
		private:
			void set_data(unsigned char* auto_data){
				data_ = auto_data;
			}
			void set_length(unsigned long auto_length){
				length_ = auto_length;
			}
			unsigned char *data_;
			unsigned long length_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif