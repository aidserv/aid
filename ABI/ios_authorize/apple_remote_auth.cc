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
#include "ios_cracker/apple_ssl.h"

namespace ABI{
	namespace internal{
		__int64 dsid[] = {0x1DD281495};
		std::string AppleRemoteAuth::RemoteAuthReady(AppleMobileDeviceEx& mobile_device,unsigned char *rq,
			unsigned long rq_length,unsigned char *rq_sig,unsigned long rq_sig_length,unsigned long private_key){
			unsigned long length = 0;
			ABI::base::Path paths;
			std::string apple_device_path(base::UnicodeToAscii(paths.GetCurrentPath())+std::string("ios.authorized"));
			std::ofstream ofile(apple_device_path,std::ios::binary|std::ios::out);
			ofile.write((char*)&rq_length,sizeof(unsigned long));
			ofile.write((char*)rq,rq_length);
			ofile.write((char*)&rq_sig_length,sizeof(unsigned long));
			ofile.write((char*)rq_sig,rq_sig_length);
			length = mobile_device.grappa_session_id();
			ofile.write((char*)&length,sizeof(unsigned long));
			length = mobile_device.key_fair_play_guid_length();
			ofile.write((char*)&length,sizeof(unsigned long));
			ofile.write((char*)mobile_device.key_fair_play_guid(),length);
			ofile.flush();
			length = mobile_device.fair_play_certificate_len();
			ofile.write((char*)&length,sizeof(unsigned long));
			ofile.write((char*)mobile_device.fair_play_certificate(),length);
			ofile.flush();
			__int64 fair_device_type = mobile_device.fair_play_device_type();
			ofile.write((char*)&fair_device_type,sizeof(__int64));
			ofile.write((char*)&private_key,sizeof(unsigned long));
			ofile.flush();
			ofile.close();
			ofile.clear();
//date:2016/01/07
//			ExitProcess(0);
			return apple_device_path;
		}
		std::string AppleRemoteAuth::RemoteAuth(const std::string& device_info_file){
			size_t length = 0;
			std::ifstream ifile(device_info_file,std::ifstream::binary|std::ifstream::in);
			if(ifile.is_open()){
				ABI::base::Path paths;
				ifile.seekg(0,ifile.end);
				length = static_cast<size_t>(ifile.tellg());
				ifile.seekg(0,ifile.beg);
				scoped_array<unsigned char> file_info(new unsigned char[length+1]);
				memset(file_info.get(),0,length+1);
				scoped_array<unsigned char> encode_info(new unsigned char[length*4]);
				memset(encode_info.get(),0,length*4);
				ifile.read(reinterpret_cast<char*>(file_info.get()),length);
				ifile.close();
				ifile.clear();
				EVP_EncodeBlock(encode_info.get(),file_info.get(),length);
				std::wstring encode_device_info = base::AsciiToUnicode(reinterpret_cast<char*>(encode_info.get()));
				std::wstring checksum = base::AsciiToUnicode(ABI::base::Md5(file_info.get(),length));
				DeleteFileA(device_info_file.c_str());
				//local generate
				//thirdparty process generate auth data
// 				Process::Create(base::AppendPath(L"iAuthRS.exe").directory_name(),
// 					std::wstring(checksum+encode_device_info+std::wstring(L" "))+base::AppendPath(L"test.info").directory_name(),5);
				//current process generate auth data
// 				GenerateAuthRS(AnalysisDeviceParam(base::UnicodeToAscii(std::wstring(checksum+encode_device_info))),
// 					base::UnicodeToAscii(base::AppendPath(L"test.info").directory_name()));
				//
// 				std::ifstream test_info(base::AppendPath(L"test.info").directory_name(),std::ifstream::binary);
// 				test_info.seekg(0,test_info.end);
// 				length = static_cast<size_t>(test_info.tellg());
// 				test_info.seekg(0,test_info.beg);
// 				file_info.reset(new unsigned char[length+1]);
// 				memset(file_info.get(),0,length+1);
// 				test_info.read(reinterpret_cast<char*>(file_info.get()),length);
// 				test_info.close();
// 				std::string data(reinterpret_cast<char*>(file_info.get()));
// 				return data;
				//remote generate
				std::string remote_encode(std::string("deviceParam=")+base::UrlEncode(base::UnicodeToAscii(std::wstring(checksum+encode_device_info))));
				return ios_cracker::internal::HTTPSendMessage(L"10.18.0.76",L"/",remote_encode.c_str(),remote_encode.length(),NULL);
			}
			else{
				return "";
			}
		}
		RemoteDeviceInfo AppleRemoteAuth::AnalysisDeviceParam(const std::string decode_info){
			RemoteDeviceInfo device_info = {0};
			std::string real_device_info = &decode_info[32];
			scoped_array<unsigned char> file_info(new unsigned char[decode_info.length()]);
			size_t decode_length = EVP_DecodeBlock(file_info.get(),reinterpret_cast<const unsigned char*>(real_device_info.c_str()),real_device_info.length());
			unsigned long length = 0;
			unsigned long position = 0;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			device_info.rq_size = length;
			device_info.rq_data = new unsigned char[length];
			position += sizeof(unsigned long);
			memmove((char*)device_info.rq_data,&file_info[position],length);
			position += device_info.rq_size;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			position += sizeof(unsigned long);
			device_info.rq_sig_size = length;
			device_info.rq_sig_data = new unsigned char[length];
			memmove((char*)device_info.rq_sig_data,&file_info[position],length);
			position += device_info.rq_sig_size;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			position += sizeof(unsigned long);
			device_info.grappa_session_id = length;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			position += sizeof(unsigned long);
			device_info.key_fair_play_guid_length = length;
			device_info.key_fair_play_guid = new unsigned char[length];
			memmove((char*)device_info.key_fair_play_guid,&file_info[position],length);
			position += length;
			length = 0;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			position += sizeof(unsigned long);
			device_info.fair_play_certificate_length = length;
			device_info.fair_play_certificate = new unsigned char[length];
			memmove((char*)device_info.fair_play_certificate,&file_info[position],length);
			position += length;
			__int64 fair_device_type = 0;
			memmove((char*)&fair_device_type,&file_info[position],sizeof(__int64));
			position += sizeof(__int64);
			device_info.fair_device_type = fair_device_type;
			memmove((char*)&length,&file_info[position],sizeof(unsigned long));
			device_info.private_key = length;
			position += sizeof(unsigned long);
			std::string checksum = ABI::base::Md5(file_info.get(),position);
			if(decode_info.compare(0,32,checksum.c_str())!=0){
				LOG(INFO)<<"device info hash failed"<<std::endl;
			}
			return device_info;
		}
		void AppleRemoteAuth::GenerateAuthRS(RemoteDeviceInfo info,const std::string& out){
			ABI::internal::RSKeyGen* rs = ABI::internal::RSKeyGen::GetInstance();

			rs->set_grappa_session_id(info.grappa_session_id);
			rs->set_fair_play_certificate(info.fair_play_certificate);
			rs->set_fair_play_certificate_len(info.fair_play_certificate_length);
			rs->set_key_fair_play_guid(info.key_fair_play_guid);
			rs->set_fair_play_device_type(info.fair_device_type);

			rs->GenRS(reinterpret_cast<Int64Make*>(&dsid[0]),sizeof(dsid)/sizeof(__int64),
				info.rq_data,info.rq_size,info.rq_sig_data,info.rq_sig_size,false);

			if(rs->data()==NULL||rs->length()==0){
				LOG(ERROR)<<"genreate rs failed!"<<std::endl;
				return;
			}

			scoped_array<unsigned char> encode_info(new unsigned char[rs->length()*4]);
			memset(encode_info.get(),0,rs->length()*4);
			std::ofstream ofile(out,std::ios::binary|std::ios::out);
			EVP_EncodeBlock(encode_info.get(),rs->data(),rs->length());
			std::string outinfo(reinterpret_cast<char*>(encode_info.get()));
			ofile.write(outinfo.c_str(),outinfo.length());
			ofile.close();
		}
	}
}