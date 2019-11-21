#ifndef IOS_CRACKER_APPLE_SSL_H_
#define IOS_CRACKER_APPLE_SSL_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_cracker{
	namespace internal{
		enum AppleProtocolType{
			apple_authenticate = 0,
			apple_signSapSetup
		};
		std::string SSLRecvMessage(const wchar_t* domain,const wchar_t* path);
		std::string SSLSendMessage(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,internal::AppleProtocolType apple_type,const wchar_t* app_header = NULL);
		std::string HTTPRecvMessage(const wchar_t* domain,const wchar_t* path);
		std::string HTTPSendMessage(const wchar_t* domain,const wchar_t* path,const void* src,const size_t length,const wchar_t* app_header = NULL);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif