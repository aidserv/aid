#include "passport/itunes_https_configure.h"


namespace passport{
	namespace internal{
		void FreeConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* config) {
			if (config->lpszAutoConfigUrl)
				GlobalFree(config->lpszAutoConfigUrl);
			if (config->lpszProxy)
				GlobalFree(config->lpszProxy);
			if (config->lpszProxyBypass)
				GlobalFree(config->lpszProxyBypass);
		}

		void FreeInfo(WINHTTP_PROXY_INFO* info) {
			if (info->lpszProxy)
				GlobalFree(info->lpszProxy);
			if (info->lpszProxyBypass)
				GlobalFree(info->lpszProxyBypass);
		}
		bool ConfigureSSL(HINTERNET internet){
			DWORD protocols = 0;
			protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_SSL2;
			protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_SSL3;
			protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1;
			protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1;
			protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
			BOOL rv = WinHttpSetOption(internet, WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols));
			return (rv==TRUE);
		}
		bool ApplyProxy(HINTERNET internet,const wchar_t* proxy_str,bool is_direct){
			WINHTTP_PROXY_INFO pi;
			if(is_direct){
				pi.dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
				pi.lpszProxy = WINHTTP_NO_PROXY_NAME;
				pi.lpszProxyBypass = WINHTTP_NO_PROXY_BYPASS;
			}
			else{
				pi.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
				pi.lpszProxy = const_cast<LPWSTR>(proxy_str);
				pi.lpszProxyBypass = WINHTTP_NO_PROXY_BYPASS;
			}
			BOOL rv = WinHttpSetOption(internet,WINHTTP_OPTION_PROXY,&pi,sizeof(pi));
			return (rv==TRUE);
		}

		bool ConfigureProxy(HINTERNET internet){
			WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = {0};
			if(!WinHttpGetIEProxyConfigForCurrentUser(&ie_config)){
				return false;
			}
			WINHTTP_AUTOPROXY_OPTIONS options = {0};
			options.fAutoLogonIfChallenged = TRUE;
			if(ie_config.fAutoDetect){
				options.lpszAutoConfigUrl = ie_config.lpszAutoConfigUrl;
				options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			}
			else{
				options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;  
				options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP|WINHTTP_AUTO_DETECT_TYPE_DNS_A;
			}
			std::wstring query_url(L"http://www.baidu.com");
			WINHTTP_PROXY_INFO info = {0};
			BOOL rv = WinHttpGetProxyForUrl(internet,query_url.c_str(),&options,&info);
			WINHTTP_PROXY_INFO pi;
			switch(info.dwAccessType){
			case WINHTTP_ACCESS_TYPE_NO_PROXY:
				ApplyProxy(internet,NULL,TRUE);
				break;
			case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
				ApplyProxy(internet,ie_config.lpszProxy,FALSE);
				break;
			default:
				pi.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
				pi.lpszProxy = ie_config.lpszProxy;
				pi.lpszProxyBypass = ie_config.lpszProxyBypass;
				rv = WinHttpSetOption(internet,WINHTTP_OPTION_PROXY,&pi,sizeof(pi));
				break;
			}
			FreeConfig(&ie_config);
			FreeInfo(&info);
			return (rv==TRUE);
		}

// 		bool ConfigureProxy(HINTERNET request)
// 		{
// 			WINHTTP_PROXY_INFO proxyInfo = {0};
// 			proxyInfo.lpszProxy = new WCHAR[PROXY_STR_MAX_LEN];
// 			proxyInfo.lpszProxyBypass = new WCHAR[PROXY_STR_MAX_LEN];
// 
// 			proxyInfo.dwAccessType = proxy.dwAccessType;
// 			swprintf_s(proxyInfo.lpszProxy, PROXY_STR_MAX_LEN, proxy.proxyStr);
// 			swprintf_s(proxyInfo.lpszProxyBypass, PROXY_STR_MAX_LEN, proxy.proxyPassBy); // *.local;<local>
// 
// 			BOOL rv = WinHttpSetOption(request,WINHTTP_OPTION_PROXY,&proxyInfo,sizeof(proxyInfo));
// 			delete[] proxyInfo.lpszProxy;
// 			delete[] proxyInfo.lpszProxyBypass;
// 
// 			//DWORD dw = sizeof(proxyInfo);
// 			//WinHttpQueryOption(request, WINHTTP_OPTION_PROXY, &proxyInfo, &dw);
// 			return rv;
// 		}

	}
}