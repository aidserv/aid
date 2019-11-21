#ifndef PASSPORT_ITUNES_DOWNLOAD_INFO_H_
#define PASSPORT_ITUNES_DOWNLOAD_INFO_H_
//////////////////////////////////////////////////////////////////////////
#include "smartbot/passport/basictypes.h"
#include "smartbot/memory/alloc_vtable.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	class iTunesDownloadInfo:public memory::DataInterfaceAllocate
	{
	public:
		static iTunesDownloadInfo* GetInterface(bool free_exit = false);
		virtual void DIAllocate();
		virtual void DIRelease();
		void set_download_key(const char* key,size_t length);
		void set_download_url(const char* url,size_t length);
		void set_download_id(const char* id,size_t length);
		const char* download_key()const;
		const char* download_url()const;
		const char* download_id()const;
	private:
		iTunesDownloadInfo():download_key_(NULL),
			download_url_(NULL),download_id_(NULL){

		}
		~iTunesDownloadInfo(){

		}
		char* download_url_;
		char* download_key_;
		char* download_id_;
		DISALLOW_EVIL_CONSTRUCTORS(iTunesDownloadInfo);
	};
}
//////////////////////////////////////////////////////////////////////////
#endif