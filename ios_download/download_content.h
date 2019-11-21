#ifndef IOS_DOWNLOAD_DOWNLOAD_CONTENT_H_
#define IOS_DOWNLOAD_DOWNLOAD_CONTENT_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_download{
	class DownloadContent
	{
	public:
		DownloadContent(void);
		~DownloadContent(void);
		bool CreateConnect(const wchar_t* domain,const char* port =NULL);
		bool AddDownloadHeader(const wchar_t* path,const wchar_t* header = NULL);
		std::string Download(const size_t try_count=1,const char* file = NULL);
	private:
		void DownloadDone();
	private:
		HINTERNET net_open_;
		HINTERNET net_connect_;
		HINTERNET net_request_;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif