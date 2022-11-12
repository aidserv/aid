#ifndef ABI_IOS_AUTHORIZE_APPLE_REMOTE_AUTH_H_
#define ABI_IOS_AUTHORIZE_APPLE_REMOTE_AUTH_H_
//////////////////////////////////////////////////////////////////////////
#include <string>
#include "ABI/ios_authorize/apple_mobile_device_ex.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI {
	namespace internal {
		class AppleRemoteAuth
		{
		public:
			AppleRemoteAuth() {}
			~AppleRemoteAuth() {}
			std::string RemoteAuth(AppleMobileDeviceEx& mobile_device, unsigned char* rq,
				unsigned long rq_length, unsigned char* rq_sig, unsigned long rq_sig_length);
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif