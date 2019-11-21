#ifndef PASSPORT_ITUNES_HTTPS_CONFIGURE_H_
#define PASSPORT_ITUNES_HTTPS_CONFIGURE_H_
//////////////////////////////////////////////////////////////////////////
#include "passport/basictypes.h"
//#include "../ios_proxy_auth/ios_proxy.h"
#include "..\smartbot\passport\itunes_client_interface.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	namespace internal{
		void FreeConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* config);
		void FreeInfo(WINHTTP_PROXY_INFO* info);
		bool ConfigureSSL(HINTERNET internet);
		bool ApplyProxy(HINTERNET internet,const wchar_t* proxy_str,bool is_direct);
		bool ConfigureProxy(HINTERNET internet);
		bool ConfigureProxy(HINTERNET request);
	}
}
#endif