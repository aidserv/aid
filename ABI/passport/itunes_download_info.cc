#include "passport/itunes_download_info.h"

namespace passport{
	iTunesDownloadInfo* iTunesDownloadInfo::GetInterface(bool free_exit){
		static iTunesDownloadInfo* ref_instance;
		if(!ref_instance){
			iTunesDownloadInfo* new_instance = new iTunesDownloadInfo;
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
	void iTunesDownloadInfo::DIAllocate(){
		DIRelease();
		download_key_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_key_,0,kMaxStackBufferLength+256);
		download_url_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_url_,0,kMaxStackBufferLength+256);
		download_id_ = reinterpret_cast<char*>(malloc(kMaxStackBufferLength+256));
		memset(download_id_,0,kMaxStackBufferLength+256);
	}
	void iTunesDownloadInfo::DIRelease(){
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
	void iTunesDownloadInfo::set_download_key(const char* key,size_t length){
		if(!length||key==NULL){
			return;
		}
		memset(download_key_,0,kMaxStackBufferLength+256);
		strncpy(download_key_,key,length);
	}
	void iTunesDownloadInfo::set_download_url(const char* url,size_t length){
		if(!length||url==NULL){
			return;
		}
		memset(download_url_,0,kMaxStackBufferLength+256);
		strncpy(download_url_,url,length);
	}
	void iTunesDownloadInfo::set_download_id(const char* id,size_t length){
		if(!length||id==NULL){
			return;
		}
		memset(download_id_,0,kMaxStackBufferLength+256);
		strncpy(download_id_,id,length);
	}
	const char* iTunesDownloadInfo::download_key()const{
		return download_key_;
	}
	const char* iTunesDownloadInfo::download_url()const{
		return download_url_;
	}
	const char* iTunesDownloadInfo::download_id()const{
		return download_id_;
	}
}