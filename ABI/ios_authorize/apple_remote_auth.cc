#include "ABI/ios_authorize/apple_remote_auth.h"
#include "ABI/base/basetypes.h"
#include "ABI/base/windows_process.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/urls_conv.h"
#include "ABI/base/string/string_conv.h"
#include "ABI/thirdparty/openssl/evp.h"
#include "ABI/thirdparty/glog/scoped_ptr.h"
#include "ABI/thirdparty/glog/logging.h"
#include "ABI/ios_authorize/rs_gen.h"
#include <iostream>

namespace ABI {
	namespace internal {
		
		string AppleRemoteAuth::RemoteAuth(AppleMobileDeviceEx& mobile_device, unsigned char* rq,
			unsigned long rq_length, unsigned char* rq_sig, unsigned long rq_sig_length) {
			unsigned long private_key = 0;
			unsigned long length = sizeof(unsigned long) + rq_length;  //计算rq分配空间长度
			length += sizeof(unsigned long) + rq_sig_length;   //rq_sig
			length += sizeof(unsigned long);   //grappa_session_id
			length += sizeof(unsigned long) + mobile_device.key_fair_play_guid_length();   //key_fair_play_guid
			length += sizeof(unsigned long) + mobile_device.fair_play_certificate_len();   //fair_play_certificate_
			length += sizeof(__int64); //fair_play_device_type  
			length += sizeof(unsigned long); //private_key 
			unsigned long fair_play_guid_len = strlen(mobile_device.fair_play_guid());
			length += sizeof(unsigned long) + fair_play_guid_len;	//fair_play_guid

			scoped_array<unsigned char> buffer(new unsigned char[length + 1]);
			memset(buffer.get(), 0, length + 1);
			//base64 分配4/3+1空间
			scoped_array<unsigned char> encode_info(new unsigned char[((length - 1) / 3 + 1) * 4 + 1]);
			memset(encode_info.get(), 0, ((length - 1) / 3 + 1) * 4 + 1);

			//组装成buffer信息
			unsigned long position = 0;
			memmove(&buffer[position], (char*)&rq_length, sizeof(unsigned long));  //rq_length
			position += sizeof(unsigned long);
			memmove(&buffer[position], (char*)rq, rq_length);   //rq
			position += rq_length;
			memmove(&buffer[position], (char*)&rq_sig_length, sizeof(unsigned long));  //rq_sig_length
			position += sizeof(unsigned long);
			memmove(&buffer[position], (char*)rq_sig, rq_sig_length);   //rq_sig
			position += rq_sig_length;
			unsigned long grappa_session_id = mobile_device.grappa_session_id();
			memmove(&buffer[position], (char*)&grappa_session_id, sizeof(unsigned long));   //grappa_session_id
			position += sizeof(unsigned long);
			unsigned long key_fair_play_guid_length = mobile_device.key_fair_play_guid_length();
			memmove(&buffer[position], (char*)&key_fair_play_guid_length, sizeof(unsigned long));   //key_fair_play_guid_length
			position += sizeof(unsigned long);
			memmove(&buffer[position], (char*)mobile_device.key_fair_play_guid(), key_fair_play_guid_length);   //key_fair_play_guid
			position += key_fair_play_guid_length;
			unsigned long fair_play_certificate_len = mobile_device.fair_play_certificate_len();
			memmove(&buffer[position], (char*)&fair_play_certificate_len, sizeof(unsigned long));   //fair_play_certificate_len
			position += sizeof(unsigned long);
			memmove(&buffer[position], (char*)mobile_device.fair_play_certificate(), fair_play_certificate_len);   //fair_play_certificate
			position += fair_play_certificate_len;
			__int64 fair_play_device_type = mobile_device.fair_play_device_type();
			memmove(&buffer[position], (char*)&fair_play_device_type, sizeof(__int64));   //fair_play_device_type
			position += sizeof(__int64);
			memmove(&buffer[position], (char*)&private_key, sizeof(unsigned long));   //private_key
			position += sizeof(unsigned long);
			//fair_play_guid_len
			memmove(&buffer[position], (char*)&fair_play_guid_len, sizeof(unsigned long));
			position += sizeof(unsigned long);
			memmove(&buffer[position], (char*)mobile_device.fair_play_guid(), fair_play_guid_len);   //key_fair_play_guid
			position += fair_play_guid_len;
			//base64 编码
			EVP_EncodeBlock(encode_info.get(), buffer.get(), length);
			std::string encode_device_info = std::string(reinterpret_cast<char*>(encode_info.get()));
			std::string checksum = std::string(ABI::base::Md5(buffer.get(), length));

			char rs[2048] = { 0 };
			//GenerateRS((checksum + encode_device_info).c_str(), rs);
			return std::string(rs);
		}
	}
}