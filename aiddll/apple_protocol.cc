#include "iAuthedCracker/apple_protocol.h"
#include "iAuthedCracker/apple_ssl.h"
#include "iAuthedCracker/apple_url_list.h"
#include "ABI/base/windows_hardware.h"
#include "ABI/base/string/string_split.h"
#include "ABI/itunes_reverse/itunes_abi.h"
#include "ABI/thirdparty/openssl/md5.h"
#include "ABI/thirdparty/openssl/evp.h"

namespace iAuthedCracker{
	namespace internal{
		unsigned __int64 apple_crypto_iv = 0;
		unsigned long apple_key_id[6] = {0x00000006, 0xC66B454C, 0x0000DC5C, 0x00000000, 0x00000000, 0x00000000};
		unsigned long apple_sc_info_k = 0;
		unsigned char apple_cert[0x10000] = {0};
		void GenerateKbsync(){
			int sc_info_this[256] = {0};
			unsigned long exchange_id = 0;
			const std::string sig_interface_a = "55 8B EC 81 EC 28 06 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 8B D9 56 85 DB 75 13 8D 41 CE";
			unsigned long (__fastcall* GenerateKbsyncInit)(unsigned long *p);
			const std::string sig_interface_b = "55 8B EC 81 EC 38 08 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 56 8B F1 57 8B FA 85 F6 75 15 B8 CE FF FF FF";
			unsigned long (__fastcall* GenerateKbsyncId)(unsigned long *p,unsigned long *exchange);
			*reinterpret_cast<unsigned long*>(&GenerateKbsyncInit) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",sig_interface_a);
			*reinterpret_cast<unsigned long*>(&GenerateKbsyncId) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",sig_interface_b);
			GenerateKbsyncInit(reinterpret_cast<unsigned long*>(&sc_info_this));
			GenerateKbsyncId(reinterpret_cast<unsigned long*>(&sc_info_this),&exchange_id);
		}
		void GenerateAppleWebCert(std::string message){
			struct message_signature_cert{
				unsigned long magic;
				unsigned long thread_id;
				unsigned long inter_locked;
				CRITICAL_SECTION section_locked;
				unsigned long iv_magic;
			};
			const std::string sig_interface = "55 8B EC 56 8B F1 85 F6 74 70 81 3E 6B 63 6F 6C 75 68 53 57 FF 15 ?? ?? ?? ?? 8D 5E 0C 8B F8 53"
				"FF 15 ?? ?? ?? ?? 83 7E 08 00 75 03 89 7E 04 FF 75 14 FF 46 08 FF 75 10 FF 75 0C FF 75 08 FF 76"
				"24 E8 0A DB 82 FF 83 C4 14 8B F8 81 3E 6B 63 6F 6C 75 20 FF 15 ?? ?? ?? ?? 39 46 04 75 13 FF 4E"
				"08 75 07 C7 46 04 FF FF FF FF 53 FF 15 ?? ?? ?? ?? 8B C7 5F 5B 5E 5D C2 10 00";
			const std::string request_sig = "55 8B EC 51 57 8B 7D 10 85 FF 75 0A 8D 47 CE 5F 8B E5 5D C2 0C 00 8D 45 FC C7 45 10 00 00 00 00 50 8D 45 10 C7 45 FC 00 00 00 00 50 FF 75 0C FF 75 08 E8 39 FF FF FF";
			void (__thiscall* GetMessageCert)(struct message_signature_cert *p,const char *data, int length, char *signature,unsigned long *signature_length);
			void (__thiscall* CalcXAppleActionSignature)(struct message_signature_cert *p,const char *request,int length,unsigned long *signature);
			*reinterpret_cast<unsigned long*>(&CalcXAppleActionSignature) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",request_sig);
			message_signature_cert cert_sig = {0};
			unsigned long cert[256] = {0};
			unsigned long length_cert = 0;
			cert_sig.magic = 0x6C6F636B;
			cert_sig.thread_id = -1;
			cert_sig.inter_locked = 0;
			InitializeCriticalSection(&cert_sig.section_locked);
			cert_sig.iv_magic = apple_crypto_iv;
			unsigned long signature[256] = {0};
			CalcXAppleActionSignature(&cert_sig,message.c_str(),message.length(),reinterpret_cast<unsigned long*>(&signature));
			//GetMessageCert(&cert_sig,message.c_str(),message.length(),reinterpret_cast<char*>(&signature),&length_cert);
			DeleteCriticalSection(&cert_sig.section_locked);
		}
		AppleProtocol::AppleProtocol(void){
			set_login_plist("");
			set_x_apple_actionsignature("");
			GenerateKbsync();
			AddSignSapSetupCert();
		}
		AppleProtocol::AppleProtocol(const std::string& apple_id,const std::string& password){
			set_login_plist("");
			set_x_apple_actionsignature("");
			GenerateKbsync();
			AddSignSapSetupCert();
			GetLogin_plist(apple_id,password);
		}
		AppleProtocol::~AppleProtocol(void){
			set_login_plist("");
			set_x_apple_actionsignature("");
		}
		void AppleProtocol::GetLogin_plist(const std::string& apple_id,const std::string& password){
			ABI::base::HardwareInfo hardware;
			const std::string string_username = std::string("<string>")+apple_id+std::string("</string>");
			const std::string string_password = std::string("<string>")+password+std::string("</string>");
			const std::string string_guid = std::string("<string>")+ABI::base::GetHardwareCookie()+std::string("</string>");
			const std::string string_machine_name = std::string("<string>")+hardware.GetMachineName()+std::string("</string>");
			std::string plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">";
			plist.append("<plist version=\"1.0\">");
			plist.append("<dict>");
			plist.append("<key>appleId</key>");
			plist.append(string_username);
			plist.append("<key>attempt</key>");
			plist.append("<integer>1</integer>");
			plist.append("<key>guid</key>");
			plist.append(string_guid);
			plist.append("<key>machineName</key>");
			plist.append(string_machine_name);
			plist.append("<key>password</key>");
			plist.append(string_password);
			plist.append("<key>why</key>");
			plist.append("<string>purchase</string>");
			plist.append("</dict>");
			plist.append("</plist>");
			set_login_plist(plist);
		}
		void AppleProtocol::set_login_plist(std::string plist){
			login_plist_ = plist;
		}
		void AppleProtocol::set_x_apple_actionsignature(std::string actionsignature){
			x_apple_actionsignature_ = actionsignature;
		}
		std::string AppleProtocol::XAppleActionSignature(std::string plist){
			unsigned long signature[256] = {0};
			unsigned long signature_length = 0;
			int (__cdecl *itunes_0x089EA350_x_apple_actionsignature)(unsigned long inthex_1D701E98,const char*,unsigned long,char* out_ptr,unsigned long* length);
			const std::string func_name8 = "55 89 E5 83 E4 F0 53 57 56 81 EC 04 02 00 00 0F 31 89 C6 8D 5C 24 38 89 1C 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 71 6D FC FF A1 ?? ?? ?? ?? 89 84 24 D8 01 00 00";
			*reinterpret_cast<unsigned long*>(&itunes_0x089EA350_x_apple_actionsignature) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",func_name8);
			GenerateAppleWebCert(plist);
			itunes_0x089EA350_x_apple_actionsignature(reinterpret_cast<unsigned long>(&apple_crypto_iv),plist.c_str(),plist.length(),reinterpret_cast<char*>(&signature[0]),&signature_length);
			return "";
		}
		std::string AppleProtocol::login_plist(){
			return login_plist_;
		}
		void AppleProtocol::AddSignSapSetupCert(){
			//ds:[0368DD50]=589886D0-58970000 (iTunes.589886D0)
			const std::string func_589886D0 = "55 89 E5 83 EC 10 8D 45 F4 8B 0D ?? ?? ?? ?? 29 C1 89 4D F8 8D 4D F6 89 4D FC 89 04 24 E8 4E 16 05 00 8B 45 F4 83 C4 10 5D C3";
			int (__stdcall *dword_589886D0_init)();
			*reinterpret_cast<unsigned long*>(&dword_589886D0_init) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",func_589886D0);
			bool (__thiscall* sub_8D046A0)(unsigned long *p);
			const std::string func_58CC46A0 = "55 8B EC 83 EC 5C A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 57 8B F9 85 FF 75 13 32 C0";
			*reinterpret_cast<unsigned long*>(&sub_8D046A0) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",func_58CC46A0);
			sub_8D046A0(reinterpret_cast<unsigned long*>(&apple_key_id[0]));
			//ds:[0368DD54]=58977D80-58970000 (iTunes.58977D80)
			const std::string func_58977D80 = "55 89 E5 83 E4 F8 56 81 EC 84 00 00 00 A1 ?? ?? ?? ?? 89 44 24 08 8B 45 08 89 44 24 70 8B 45 0C 89 44 24 40 8D 44 24 08 89 44 24 7C 8D 74 24 78";
			int (__cdecl *dword_58977D80_get_iv_password)(unsigned long*,unsigned long* iv_password);
			*reinterpret_cast<unsigned long*>(&dword_58977D80_get_iv_password) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",func_58977D80);
			dword_589886D0_init();
			dword_58977D80_get_iv_password(reinterpret_cast<unsigned long*>(&apple_crypto_iv),&apple_key_id[0]);
			//ds:[0368DD58]=58978CD0-58970000 (iTunes.58978CD0)//
			const std::string func_58978CD0 = "55 89 E5 83 E4 F0 53 57 56 81 EC B4 01 00 00 8D 44 24 78 89 04 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 F5 83 FF FF A1 ?? ?? ?? ?? 89 84 24 F8 00 00 00 C7 84 24 FC 00 00 00 01 00 00 00 C7 84 24 00 01 00 00 06 00 00 00 C7 84 24 04 01 00 00 0F 00 00 00";
			int (__cdecl *dword_58978CD0_add_apple_cert)(unsigned long, unsigned long*, unsigned long,const unsigned char*, unsigned long,unsigned char*, unsigned long*, unsigned long*);
			*reinterpret_cast<unsigned long*>(&dword_58978CD0_add_apple_cert) = ABI::ReverseEngineering::GetAddress(NULL,L"iTunes.dll",func_58978CD0);
			unsigned char *cert = {0};
			unsigned long cert_length = 0;
			unsigned long cert_type = 1;
			std::wstring domain = L"";
			std::wstring relative = L"";
			GetAppleUrlInfo(AppleUrlInfo::signSapSetupCert,domain,relative);
			std::string message = SSLRecvMessage(domain.c_str(),relative.c_str());
			message = message.substr(std::string("<plist>\n<dict>\n<key>sign-sap-setup-cert</key>\n<data>").length());
			message = message.substr(0,message.length()-std::string("</data>\n</dict>\n</plist>\n").length());
			unsigned long apple_cert_length = EVP_DecodeBlock((unsigned char*)&apple_cert[0],(const unsigned char*)message.c_str(),message.size());
			dword_58978CD0_add_apple_cert(0xC8,&apple_key_id[0],static_cast<unsigned long>(apple_crypto_iv),reinterpret_cast<unsigned char*>(&apple_cert[0]),apple_cert_length,reinterpret_cast<unsigned char*>(&cert),&cert_length,&cert_type);
			memset(apple_cert,0,0x10000);
			EVP_EncodeBlock(reinterpret_cast<unsigned char*>(apple_cert),cert,cert_length);
			std::string send_message = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict><key>sign-sap-setup-buffer</key><data>""")+std::string(reinterpret_cast<char*>(apple_cert))+std::string("</data></dict></plist>");
			GetAppleUrlInfo(AppleUrlInfo::signSapSetup,domain,relative);
			message = iAuthedCracker::internal::SSLSendMessage(domain.c_str(),relative.c_str(),send_message.c_str(),send_message.length(),internal::apple_signSapSetup);
			EVP_DecodeBlock((unsigned char*)&apple_cert[0],(const unsigned char*)message.c_str(),message.size());
			set_x_apple_actionsignature(std::string(reinterpret_cast<char*>(apple_cert)));
		}
		std::string AppleProtocol::x_apple_actionsignature(){
			return x_apple_actionsignature_;
		}
	}
}