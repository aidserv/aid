#include "ios_cracker/apple_ssl.h"
#include <Windows.h>
#include <winhttp.h>

namespace ios_cracker{
    namespace internal{
		const wchar_t* user_agent = L"iTunes/12.2.2 (Windows; Microsoft Windows 8 x64 Business Edition (Build 9200); x64) AppleWebKit/7600.5017.0.22";
		std::string SSLRecvMessage(const wchar_t* domain,const wchar_t* path){
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				hOpen = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen, domain, INTERNET_DEFAULT_HTTPS_PORT, 0);	
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"text/html, application/xhtml+xml, application/xml"},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"GET",path,NULL,WINHTTP_NO_REFERER,types,WINHTTP_FLAG_SECURE);
				if(!hRequest){
					break;
				}
				if(!WinHttpSendRequest(hRequest,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				wchar_t szContentLength[32] = {0};
				DWORD cch = 64;
				DWORD dwHeaderIndex = WINHTTP_NO_HEADER_INDEX;
				BOOL haveContentLength = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL,&szContentLength,&cch,&dwHeaderIndex);
				DWORD dwContentLength;
				if (haveContentLength) {
					dwContentLength = _wtoi(szContentLength);
				}
				if(CreateStreamOnHGlobal(0, true, &stream)){
					break;
				}
				char *p = new char[4096];
				if(!p){
					break;
				}
				DWORD dwReceivedTotal = 0;
				while(WinHttpQueryDataAvailable(hRequest, &cch) && cch){
					if(cch > 4096){
						cch = 4096;
					}
					dwReceivedTotal += cch;
					WinHttpReadData(hRequest,p,cch,&cch);
					stream->Write(p,cch,NULL);
				}
				delete[] p;
				p = NULL;
				stream->Write(&p, 1, NULL);
				HGLOBAL hgl;
				if(GetHGlobalFromStream(stream, &hgl)){
					break;
				}
				p = reinterpret_cast<char*>(GlobalLock(hgl));
				if(!p){
					break;
				}
				message.resize(0);
				message.append(p,dwReceivedTotal);
				GlobalUnlock(hgl);
				break;
			}
			if(stream){
				stream->Release();
			}
			if(hRequest){
				WinHttpCloseHandle(hRequest);
			}
			if(hConnect){
				WinHttpCloseHandle(hConnect);
			}
			if(hOpen){
				WinHttpCloseHandle(hOpen);
			}
			return message;
		}
		std::string SSLSendMessage(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,internal::AppleProtocolType apple_type,const wchar_t* app_header){
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				unsigned long write_length = 0;
				unsigned long option_flag = SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|SECURITY_FLAG_IGNORE_UNKNOWN_CA;
				hOpen = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen,domain,INTERNET_DEFAULT_HTTPS_PORT,0);
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"text/html, application/xhtml+xml, application/xml"},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"POST",path,NULL,WINHTTP_NO_REFERER,types,WINHTTP_FLAG_SECURE);
				if(!hRequest){
					break;
				}
				WinHttpSetOption(hRequest,WINHTTP_OPTION_SECURITY_FLAGS,(LPVOID)(&option_flag),sizeof(unsigned long));
				std::wstring externsion_header = L"Cache-Control: no-cache\r\nConnection: Keep-Alive\r\nAccept-Encoding: gzip\r\nCookie: Pod=58; itspod=58; X-Dsid=8005357461; mz_at0-8005357461=AwQAAAECAAGttQAAAABUZGys8Bkl7jccmMMhrqIt0zdtjuWpG8Y=; mz_at_ssl-8005357461=AwUAAAECAAGttQAAAABUZGysrck9tGhgA8RVZ0WkNHcEZNqFyWI=; mz_at0-8005358741=AwQAAAECAAGttQAAAABUZBzK9jo8ax0KlPjVvhgXFmj2Hkx+0/4=; mz_at_ssl-8005358741=AwUAAAECAAGttQAAAABUZBzKUigRkfa7WLW72Bh4ZgbsN6+afAo=";
				if(apple_type==internal::apple_authenticate){
					externsion_header.append(L"\r\nContent-Type: application/x-apple-plist");
					if(app_header!=NULL){
						externsion_header.append(L"\r\n");
						externsion_header.append(app_header);
					}
				}
				if(!WinHttpSendRequest(hRequest,externsion_header.c_str(),externsion_header.length(),(LPVOID)src,length,length,WINHTTP_FLAG_SECURE)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				if(CreateStreamOnHGlobal(0,TRUE,&stream)){
					break;
				}
				char *p = new char[4096];
				if(!p){
					break;
				}
				unsigned long dwReceivedTotal = 0;
				for(unsigned long cch = 4096;WinHttpReadData(hRequest,p,cch,&cch) && cch;cch = 4096){
					dwReceivedTotal += cch;
					stream->Write(p,cch,NULL);
				}
				delete[] p;
				p = NULL;
				stream->Write(&p, 1, NULL);
				HGLOBAL hgl;
				if(GetHGlobalFromStream(stream, &hgl)){
					break;
				}
				p = reinterpret_cast<char*>(GlobalLock(hgl));
				if(!p){
					break;
				}
				message.resize(0);
				message.append(p,dwReceivedTotal);
				GlobalUnlock(hgl);
				break;
			}
			if(stream){
				stream->Release();
			}
			if(hRequest){
				WinHttpCloseHandle(hRequest);
			}
			if(hConnect){
				WinHttpCloseHandle(hConnect);
			}
			if(hOpen){
				WinHttpCloseHandle(hOpen);
			}
			return message;
		}
		std::string HTTPRecvMessage(const wchar_t* domain,const wchar_t* path){
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				hOpen = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen, domain, INTERNET_DEFAULT_HTTP_PORT, 0);
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"text/html, application/xhtml+xml, application/xml"},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"GET",path,NULL,WINHTTP_NO_REFERER,types,WINHTTP_FLAG_BYPASS_PROXY_CACHE);
				if(!hRequest){
					break;
				}
				if(!WinHttpSendRequest(hRequest,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				wchar_t szContentLength[32] = {0};
				DWORD cch = 64;
				DWORD dwHeaderIndex = WINHTTP_NO_HEADER_INDEX;
				BOOL haveContentLength = WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_CONTENT_LENGTH,NULL,&szContentLength,&cch,&dwHeaderIndex);
				DWORD dwContentLength;
				if (haveContentLength) {
					dwContentLength = _wtoi(szContentLength);
				}
				if(CreateStreamOnHGlobal(0, true, &stream)){
					break;
				}
				char *p = new char[4096];
				if(!p){
					break;
				}
				DWORD dwReceivedTotal = 0;
				while(WinHttpQueryDataAvailable(hRequest, &cch) && cch){
					if(cch > 4096){
						cch = 4096;
					}
					dwReceivedTotal += cch;
					WinHttpReadData(hRequest,p,cch,&cch);
					stream->Write(p,cch,NULL);
				}
				delete[] p;
				p = NULL;
				stream->Write(&p, 1, NULL);
				HGLOBAL hgl;
				if(GetHGlobalFromStream(stream, &hgl)){
					break;
				}
				p = reinterpret_cast<char*>(GlobalLock(hgl));
				if(!p){
					break;
				}
				message.resize(0);
				message.append(p,dwReceivedTotal);
				GlobalUnlock(hgl);
				break;
			}
			if(stream){
				stream->Release();
			}
			if(hRequest){
				WinHttpCloseHandle(hRequest);
			}
			if(hConnect){
				WinHttpCloseHandle(hConnect);
			}
			if(hOpen){
				WinHttpCloseHandle(hOpen);
			}
			return message;
		}
		std::string HTTPSendMessage(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,const wchar_t* app_header){
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				unsigned long write_length = 0;
				hOpen = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen,domain,10000,0);//INTERNET_DEFAULT_HTTP_PORT
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"*/*"},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"POST",path,NULL,WINHTTP_NO_REFERER,types,WINHTTP_FLAG_BYPASS_PROXY_CACHE);
				if(!hRequest){
					break;
				}
				std::wstring externsion_header = L"Content-Type: application/x-www-form-urlencoded";
				if(!WinHttpSendRequest(hRequest,externsion_header.c_str(),externsion_header.length(),(LPVOID)src,length,length,WINHTTP_FLAG_BYPASS_PROXY_CACHE)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				if(CreateStreamOnHGlobal(0,TRUE,&stream)){
					break;
				}
				char *p = new char[4096];
				if(!p){
					break;
				}
				unsigned long dwReceivedTotal = 0;
				for(unsigned long cch = 4096;WinHttpReadData(hRequest,p,cch,&cch) && cch;cch = 4096){
					dwReceivedTotal += cch;
					stream->Write(p,cch,NULL);
				}
				delete[] p;
				p = NULL;
				stream->Write(&p, 1, NULL);
				HGLOBAL hgl;
				if(GetHGlobalFromStream(stream, &hgl)){
					break;
				}
				p = reinterpret_cast<char*>(GlobalLock(hgl));
				if(!p){
					break;
				}
				message.resize(0);
				message.append(p,dwReceivedTotal);
				GlobalUnlock(hgl);
				break;
			}
			if(stream){
				stream->Release();
			}
			if(hRequest){
				WinHttpCloseHandle(hRequest);
			}
			if(hConnect){
				WinHttpCloseHandle(hConnect);
			}
			if(hOpen){
				WinHttpCloseHandle(hOpen);
			}
			return message;
		}
    }
}