#ifndef ABI_IOS_AUTHORIZE_RQ_SIG_GEN_H_
#define ABI_IOS_AUTHORIZE_RQ_SIG_GEN_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		class RQSigReadBuf
		{
		public:
			RQSigReadBuf(){
				set_rq_sig_buf(NULL);
				set_rq_sig_size(0);
			}
			~RQSigReadBuf(){
				if(rq_sig_buf()){
					unsigned char* p = rq_sig_buf();
					free(p);
					set_rq_sig_buf(NULL);
				}
				if(rq_sig_buf()){
					set_rq_sig_size(0);
				}
			}
			bool RQSigGenBuf(ATHRef afc){
				AFCFileRef fileAfsyncRqSig = NULL;
				set_rq_sig_size(21);
				unsigned char *afsync_rq_buf = (unsigned char *)malloc(rq_sig_size());
				if(AFCFileRefOpen(afc,"/AirFair/sync/afsync.rq.sig", AFC_FILEMODE_READ, &fileAfsyncRqSig)!=0){
					return false;
				}
				unsigned long total = 0;
				while(total < rq_sig_size()){
					unsigned len = min(rq_sig_size() - total, rq_sig_size());
					if(AFCFileRefRead(afc,fileAfsyncRqSig,afsync_rq_buf,&len)!=0){
						return false;
					}
					if(!len){
						break;
					}
					total += len;
				}
				AFCFileRefClose(afc,fileAfsyncRqSig);
				set_rq_sig_buf(afsync_rq_buf);
				return true;
			}
			unsigned char* rq_sig_buf() const{
				return rq_sig_buf_;
			}
			unsigned long rq_sig_size() const{
				return rq_sig_size_;
			}
		private:
			void set_rq_sig_buf(unsigned char* auto_data){
				rq_sig_buf_ = auto_data;
			}
			void set_rq_sig_size(unsigned long auto_length){
				rq_sig_size_ = auto_length;
			}
			unsigned char* rq_sig_buf_;
			unsigned long rq_sig_size_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif