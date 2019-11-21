#include "ios_download/download_content.h"
#include <fstream>
#include "passport/itunes_https_configure.h"

namespace ios_download
{
    DownloadContent::DownloadContent(void): net_open_(NULL), net_connect_(NULL), net_request_(NULL){
		DownloadDone();
    }
    DownloadContent::~DownloadContent(void){
		DownloadDone();
    }
	bool DownloadContent::CreateConnect(const wchar_t* domain,const char* port){
		const wchar_t *user_agent = L"iTunes/12.2.2 (Windows; Microsoft Windows 8 x64 Business Edition (Build 9200); x64) AppleWebKit/7600.5017.0.22";
		net_open_ = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if(!net_open_){
			return false;
		} 
		if(!passport::internal::ConfigureSSL(net_open_)){
			return false;
		}
		net_connect_ = WinHttpConnect(net_open_, domain, port == NULL ? INTERNET_DEFAULT_HTTP_PORT : atoi(port), 0);
		if(!net_connect_)
		{
			return false;
		}
		return true;
	}
	bool DownloadContent::AddDownloadHeader(const wchar_t* path,const wchar_t* header){
		const wchar_t *types[50] = {{L"*/*"}, {0}};
		net_request_ = WinHttpOpenRequest(net_connect_, L"GET", path, NULL, WINHTTP_NO_REFERER, types, WINHTTP_FLAG_BYPASS_PROXY_CACHE);
		if(!net_request_)
		{
			return false;
		}
		if(header)
		{
			if(!WinHttpAddRequestHeaders(net_request_, header, wcslen(header), WINHTTP_ADDREQ_FLAG_ADD))
			{
				return false;
			}
		}
		std::wstring some_header(L"Cache-Control: no-cache\r\nAccept-Language: zh-cn,zh;q=0.5\r\n");
		if(!WinHttpAddRequestHeaders(net_request_, some_header.c_str(), some_header.length(), WINHTTP_ADDREQ_FLAG_ADD)){
			return false;
		}
		return true;
	}
    std::string DownloadContent::Download(const size_t try_count,const char* file){
        IStream *stream = NULL;
		std::ofstream out;
        std::string message = "";
        for(size_t count=try_count;count!=0;count--)
        {
            if(!WinHttpSendRequest(net_request_, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
            {
				message = "failed";
                break;
            }
            if(!WinHttpReceiveResponse(net_request_,0))
            {
				message = "failed";
                break;
            }
            wchar_t szContentLength[32] = {0};
            DWORD cch = 64;
            DWORD dwHeaderIndex = WINHTTP_NO_HEADER_INDEX;
            WinHttpQueryHeaders(net_request_, WINHTTP_QUERY_CONTENT_LENGTH, NULL, &szContentLength, &cch, &dwHeaderIndex);
			if(file==NULL){
				if(CreateStreamOnHGlobal(0,TRUE,&stream))
				{
					message = "failed";
					break;
				}
			}
			else{
				out.open(file,std::ofstream::out|std::ofstream::binary);
				if(!out.is_open()){
					message = "failed";
					break;
				}
			}
            DWORD dwReceivedTotal = 0;
            while(WinHttpQueryDataAvailable(net_request_, &cch) && cch){
				char p[4096] = {0};
                if(cch > 4096)
                {
                    cch = 4096;
                }
                dwReceivedTotal += cch;
                WinHttpReadData(net_request_, p, cch, &cch);
				if(stream!=NULL){
					stream->Write(p,cch, NULL);
				}
				else{
					out.write(p,cch);
					out.flush();
				}
            }
			if(stream!=NULL){
				char* p = NULL;
				stream->Write(&p, 1, NULL);
				HGLOBAL hgl;
				if(GetHGlobalFromStream(stream, &hgl))
				{
					message = "failed";
					break;
				}
				p = reinterpret_cast<char *>(GlobalLock(hgl));
				if(!p)
				{
					message = "failed";
					break;
				}
				message.resize(0);
				message.append(p, dwReceivedTotal);
				GlobalUnlock(hgl);
			}
        }
        if(stream){
            stream->Release();
        }
		if(out.is_open()){
			out.close();
			out.clear();
		}
        return message;
    }
	void DownloadContent::DownloadDone(){
		if(net_request_)
		{
			WinHttpCloseHandle(net_request_);
			net_request_ = NULL;
		}
		if(net_connect_)
		{
			WinHttpCloseHandle(net_connect_);
			net_connect_ = NULL;
		}
		if(net_open_)
		{
			WinHttpCloseHandle(net_open_);
			net_open_ = NULL;
		}
	}
}