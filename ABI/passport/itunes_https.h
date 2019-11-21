#ifndef PASSPORT_ITUNES_HTTPS_H_
#define PASSPORT_ITUNES_HTTPS_H_
//////////////////////////////////////////////////////////////////////////
#include "passport/basictypes.h"
//#include "../ios_proxy_auth/ios_proxy.h"
#include "passport/itunes_client_interface.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	namespace internal{
		enum iTunesExtHeader{
			apple_itunes = -1,
			apple_authenticate,
			apple_signSapSetup
		};
		std::string ReadHTTPS(const wchar_t* domain,const wchar_t* path,const wchar_t* header,iTunesExtHeader options = apple_itunes,const wchar_t* referer=NULL,const char* port=NULL);
		std::string SendHTTPS(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,iTunesExtHeader options,const wchar_t* header,const wchar_t* referer=NULL,const char* post = NULL);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif