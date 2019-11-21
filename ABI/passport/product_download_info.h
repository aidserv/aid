#ifndef PASSPORT_PRODUCT_DOWNLOAD_INFO_H_
#define PASSPORT_PRODUCT_DOWNLOAD_INFO_H_
//////////////////////////////////////////////////////////////////////////
#include "passport/basictypes.h"
#include "memory/alloc_vtable.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	class ProductDownloadInfo:public memory::FreeManager
	{
	public:
		static ProductDownloadInfo* GetInterface(bool free_exit = false);
		virtual void Malloc();
		virtual void Free();
		void set_download_key(const char* key,size_t length);
		void set_download_url(const char* url,size_t length);
		void set_download_id(const char* id,size_t length);
		const char* download_key()const;
		const char* download_url()const;
		const char* download_id()const;
	private:
		ProductDownloadInfo():download_key_(NULL),
			download_url_(NULL),download_id_(NULL){

		}
		~ProductDownloadInfo(){

		}
		char* download_url_;
		char* download_key_;
		char* download_id_;
		DISALLOW_EVIL_CONSTRUCTORS(ProductDownloadInfo);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif