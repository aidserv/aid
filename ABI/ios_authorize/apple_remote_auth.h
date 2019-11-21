#ifndef ABI_IOS_AUTHORIZE_APPLE_REMOTE_AUTH_H_
#define ABI_IOS_AUTHORIZE_APPLE_REMOTE_AUTH_H_
//////////////////////////////////////////////////////////////////////////
#include <string>
#include "ABI/ios_authorize/apple_mobile_device_ex.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		struct RemoteDeviceInfo
		{
			unsigned long rq_size;
			unsigned char* rq_data;
			unsigned long rq_sig_size;
			unsigned char* rq_sig_data;
			unsigned long grappa_session_id;
			unsigned long key_fair_play_guid_length;
			unsigned char* key_fair_play_guid;
			unsigned long fair_play_certificate_length;
			unsigned char* fair_play_certificate;
			__int64 fair_device_type;
			unsigned long private_key;
		};
		class AppleRemoteAuth
		{
		public:
			AppleRemoteAuth(){}
			~AppleRemoteAuth(){}
			std::string RemoteAuthReady(AppleMobileDeviceEx& mobile_device,unsigned char *rq,unsigned long rq_length,
				unsigned char *rq_sig,unsigned long rq_sig_length,unsigned long private_key);
			std::string RemoteAuth(const std::string& device_info_file);
			RemoteDeviceInfo AnalysisDeviceParam(const std::string decode_info);
			void GenerateAuthRS(RemoteDeviceInfo info,const std::string& out);
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif