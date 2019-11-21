#include "passport/product_download_info.h"

namespace passport{
	ProductDownloadInfo* ProductDownloadInfo::GetInterface(bool free_exit){
		static ProductDownloadInfo* ref_instance;
		if(!ref_instance){
			ProductDownloadInfo* new_instance = new ProductDownloadInfo;
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&ref_instance),new_instance,NULL)){
				delete new_instance;
			}
		}
		if(free_exit){
			delete ref_instance;
			ref_instance = NULL;
		}
		return ref_instance;
	}
	void ProductDownloadInfo::Malloc(){
		Free();
		download_key_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_key_,0,kMaxStackBufferLength+256);
		download_url_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_url_,0,kMaxStackBufferLength+256);
		download_id_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_id_,0,kMaxStackBufferLength+256);
	}
	void ProductDownloadInfo::Free(){
		if(download_key_){
			free(download_key_);
			download_key_ = NULL;
		}
		if(download_url_){
			free(download_url_);
			download_url_ = NULL;
		}
		if(download_id_){
			free(download_id_);
			download_id_ = NULL;
		}
	}
	void ProductDownloadInfo::set_download_key(const char* key,size_t length){
		if(!length||key==NULL){
			return;
		}
		memset(download_key_,0,kMaxStackBufferLength+256);
		strncpy(download_key_,key,length);
	}
	void ProductDownloadInfo::set_download_url(const char* url,size_t length){
		if(!length||url==NULL){
			return;
		}
		memset(download_url_,0,kMaxStackBufferLength+256);
		strncpy(download_url_,url,length);
	}
	void ProductDownloadInfo::set_download_id(const char* id,size_t length){
		if(!length||id==NULL){
			return;
		}
		memset(download_id_,0,kMaxStackBufferLength+256);
		strncpy(download_id_,id,length);
	}
	const char* ProductDownloadInfo::download_key()const{
		return download_key_;
	}
	const char* ProductDownloadInfo::download_url()const{
		return download_url_;
	}
	const char* ProductDownloadInfo::download_id()const{
		return download_id_;
	}
}