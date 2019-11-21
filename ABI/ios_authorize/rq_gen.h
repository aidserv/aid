#ifndef ABI_IOS_AUTHORIZE_RQ_GEN_H_
#define ABI_IOS_AUTHORIZE_RQ_GEN_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		class RQReadBuf
		{
		public:
			RQReadBuf(){
				set_rq_buf(NULL);
				set_rq_size(0);
			}
			~RQReadBuf(){
				if(rq_buf()){
					unsigned char* p = rq_buf();
					free(p);
					set_rq_buf(NULL);
				}
				if(rq_size()){
					set_rq_size(0);
				}
			}
			void RQGenSize(ATHRef afc){
				afc_dictionary *fileinfo;
				while(true){
					if(AFCFileInfoOpen(afc,"/AirFair/sync/afsync.rq",&fileinfo) == 0){
						break;
					}
					Sleep(500);
				}
				char *pKey, *pValue;
				AFCKeyValueRead(fileinfo, &pKey, &pValue);
				while(pKey||pValue){
					if (pKey == NULL || pValue == NULL)
						break;
					if(!_stricmp(pKey, "st_size")){
						set_rq_size(atoi(pValue));
						break;
					}
					AFCKeyValueRead(fileinfo, &pKey, &pValue);
				}
				AFCKeyValueClose(fileinfo);
			}
			bool RQGenBuf(ATHRef afc){
				unsigned long total = 0;
				unsigned char *afsync_rq_buf = (unsigned char *)malloc(rq_size());
				AFCFileRef fileAfsyncRq = NULL;
				unsigned int ret = AFCFileRefOpen(afc,"/AirFair/sync/afsync.rq",AFC_FILEMODE_READ,&fileAfsyncRq);
				if(ret != 0){
					return false;
				}
				while(total < rq_size()){
					unsigned len = min(rq_size()-total,rq_size());
					ret = AFCFileRefRead(afc,fileAfsyncRq,afsync_rq_buf,&len);
					if(ret != 0){
						return false;
					}
					if(!len){
						break;
					}
					total += len;
				}
				AFCFileRefClose(afc,fileAfsyncRq);
				set_rq_buf(afsync_rq_buf);
				return true;
			}
			unsigned char* rq_buf() const{
				return rq_buf_;
			}
			unsigned long rq_size() const{
				return rq_size_;
			}
		private:
			void set_rq_buf(unsigned char* auto_data){
				rq_buf_ = auto_data;
			}
			void set_rq_size(unsigned long auto_length){
				rq_size_ = auto_length;
			}
			unsigned char* rq_buf_;
			unsigned long rq_size_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif