#include "passport/itunes_https.h"
#include <atlconv.h>
#include "passport/itunes_cookie_interface.h"
#include "passport/itunes_https_configure.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/glog/scoped_ptr.h"

namespace passport{
    namespace internal{
		const wchar_t* user_agent = L"iTunes/12.2.2 (Windows; Microsoft Windows 8 x64 Business Edition (Build 9200); x64) AppleWebKit/7600.5017.0.22";
		void PrintResponseHeader(HINTERNET hRequest){
			unsigned long header_length = 0;
			WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX,NULL,&header_length,WINHTTP_NO_HEADER_INDEX);
			if(GetLastError()==ERROR_INSUFFICIENT_BUFFER||header_length){
				scoped_array<wchar_t> buffer(new wchar_t[header_length/sizeof(wchar_t)]);
				WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_RAW_HEADERS_CRLF,WINHTTP_HEADER_NAME_BY_INDEX,buffer.get(),&header_length,WINHTTP_NO_HEADER_INDEX);
				if(iTunesCookieInterface::GetInstance()->login_cookie_flag()){
					wchar_t x_buffer[MAX_PATH] = {0};
					unsigned long buffer_length = MAX_PATH;
					BOOL upper_flag = WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_CUSTOM,L"X-Set-Apple-Store-Front",x_buffer,&buffer_length,WINHTTP_NO_HEADER_INDEX);
					if(!upper_flag||buffer_length==0){
						upper_flag = WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_CUSTOM,L"x-set-apple-store-front",x_buffer,&buffer_length,WINHTTP_NO_HEADER_INDEX);
					}
					if(upper_flag||buffer_length){
						USES_CONVERSION;
						iTunesCookieInterface::GetInstance()->set_auth_response_header(W2A(buffer.get()));
						iTunesCookieInterface::GetInstance()->set_x_apple_store_front(W2A(x_buffer));
						iTunesCookieInterface::GetInstance()->set_login_cookie_flag(false);
					}
				}
				else if(iTunesCookieInterface::GetInstance()->signup_wizard_cookie_flag()){
					USES_CONVERSION;
					iTunesCookieInterface::GetInstance()->set_auth_response_header(W2A(buffer.get()));
					iTunesCookieInterface::GetInstance()->set_signup_wizard_cookie_flag(false);
				}
				else if(iTunesCookieInterface::GetInstance()->buy_product_state()==iTunesCookieInterface::FIRST_BUY_BEGIN){
					wchar_t x_buffer[kMaxBufferLength] = {0};
					unsigned long buffer_length = kMaxBufferLength;
					if(WinHttpQueryHeaders(hRequest,WINHTTP_QUERY_CUSTOM,L"x-apple-md-data",x_buffer,&buffer_length,WINHTTP_NO_HEADER_INDEX)){
						USES_CONVERSION;
						iTunesCookieInterface::GetInstance()->set_x_apple_md_data(W2A(x_buffer));
					}
					iTunesCookieInterface::GetInstance()->set_buy_product_state(iTunesCookieInterface::FIRST_BUY_END);
				}
#ifdef	__DEBUG
				LOG(INFO)<<buffer.get()<<std::endl;
#endif
			}
		}

		std::string ReadHTTPS(const wchar_t* domain,const wchar_t* path,const wchar_t* header,iTunesExtHeader options,const wchar_t* referer,const char* port) {
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				unsigned long option_flag = SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|SECURITY_FLAG_IGNORE_UNKNOWN_CA;
				hOpen = WinHttpOpen(user_agent,WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen,domain,port==NULL?INTERNET_DEFAULT_HTTPS_PORT:atoi(port),0);
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"*/*"/*"text/html, application/xhtml+xml, application/xml;q=0.9, * / *;q=0.8"*/},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"GET",path,NULL,referer,types,(port==NULL||atoi(port)!=80)?WINHTTP_FLAG_SECURE:WINHTTP_FLAG_BYPASS_PROXY_CACHE);
				if(!hRequest){
					break;
				}
				
				ConfigureProxy(hRequest);
				if(port==NULL||atoi(port)!=80){
					ConfigureSSL(hRequest);
					WinHttpSetOption(hRequest,WINHTTP_OPTION_SECURITY_FLAGS,(LPVOID)(&option_flag),sizeof(unsigned long));
				}
				std::wstring some_header(L"Cache-Control: no-cache\r\nConnection: Keep-Alive\r\nAccept-Language: zh-cn, zh;q=0.75, en-us;q=0.50, en;q=0.25\r\n");
				if(options==internal::apple_authenticate){
					some_header.append(L"Content-Type: application/x-apple-plist\r\n");
				}
				if(header){
					some_header.append(header);
				}
				if(!WinHttpSendRequest(hRequest,some_header.c_str(),some_header.length(),WINHTTP_NO_REQUEST_DATA,0,0,WINHTTP_FLAG_SECURE)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				DWORD cch = 1;
				internal::PrintResponseHeader(hRequest);
				if(CreateStreamOnHGlobal(0, TRUE, &stream)){
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

		std::string SendHTTPS(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,iTunesExtHeader options,const wchar_t* header,const wchar_t* referer,const char* post){
			HINTERNET hOpen = 0;
			HINTERNET hConnect = 0;
			HINTERNET hRequest = 0;
			IStream *stream = NULL;
			std::string message = "";
			for(;;){
				unsigned long option_flag = SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|SECURITY_FLAG_IGNORE_UNKNOWN_CA;
				unsigned long write_length = 0;
				hOpen = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
				if(!hOpen){
					break;
				}
				hConnect = WinHttpConnect(hOpen,domain,post==NULL?INTERNET_DEFAULT_HTTPS_PORT:atoi(post),0);
				if(!hConnect){
					break;
				}
				const wchar_t* types[50] = {{L"text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8"},{0}};
				hRequest = WinHttpOpenRequest(hConnect,L"POST",path,NULL,referer,types,WINHTTP_FLAG_SECURE);
				if(!hRequest){
					break;
				}
				if(header){
					if(!WinHttpAddRequestHeaders(hRequest,header,wcslen(header),WINHTTP_ADDREQ_FLAG_ADD)){
						break;
					}
				}
				ConfigureProxy(hRequest);
				ConfigureSSL(hRequest);
// 				if(!WinHttpSetTimeouts(hRequest,20000,20000,20000,20000)){
// 					break;
// 				}
				WinHttpSetOption(hRequest,WINHTTP_OPTION_SECURITY_FLAGS,(LPVOID)(&option_flag),sizeof(unsigned long));
				
				std::wstring request_header = L"Accept-Language: zh-cn, zh;q=0.75, en-us;q=0.50, en;q=0.25\r\nConnection: Keep-Alive\r\nCache-Control: no-cache\r\n";
				if(options==internal::apple_authenticate){
					request_header.append(L"Content-Type: application/x-apple-plist\r\n");
				}
				if(!WinHttpSendRequest(hRequest,request_header.c_str(),request_header.length(),(LPVOID)src,length,length,WINHTTP_FLAG_SECURE)){
					break;
				}
				if(!WinHttpReceiveResponse(hRequest,0)){
					break;
				}
				internal::PrintResponseHeader(hRequest);
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
				hRequest = NULL;
			}
			if(hConnect){
				WinHttpCloseHandle(hConnect);
				hConnect = NULL;
			}
			if(hOpen){
				WinHttpCloseHandle(hOpen);
				hOpen = NULL;
			}
			return message;
		}
    }
}