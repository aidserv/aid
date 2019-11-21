#include "smartbot/account/third_appleid.h"
#include "smartbot/passport/itunes_https.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/string_conv.h"
#include "thirdparty/glog/logging.h"
#include <cookies/parsed_cookie.h>
#include "passport/itunes_cookie_interface.h"

namespace smartbot{
	namespace account{
		class iToolsSetCookie
		{
		public:
			explicit iToolsSetCookie(const std::string& http_header):cookie_token_(""){
				const wchar_t* cookie_key[] = {L"Set-Cookie: ",L"set-cookie: ",NULL};
				USES_CONVERSION;
				for(int i=0;cookie_key[i]!=NULL;i++){
					std::vector<std::wstring> multi_line_cookie = ABI::base::SplitMakePair(A2W(http_header.c_str()),cookie_key[i],L"\r\n");
					std::vector<std::wstring>::iterator it;
					for(it=multi_line_cookie.begin();it!=multi_line_cookie.end();it++){
						net::ParsedCookie cookie(W2A(it->c_str()));
						for(int index=0;kCookieName[index]!=NULL;index++){
							std::string pair_name = cookie.Name();
							if(!strnicmp(pair_name.c_str(),kCookieName[index],strlen(kCookieName[index]))&&!cookie.Value().empty()){
								if(!cookie_token_.empty()){
									cookie_token_.append("; ");
								}
								net::ParsedCookie cookie_token("");
								cookie_token.SetName(cookie.Name());
								cookie_token.SetValue(cookie.Value());
								cookie_token_ += cookie_token.ToCookieLine();
								break;
							}
						}
					}
				}
			}
			std::string Cookie(){
				return cookie_token_;
			}
		private:
			static char *kCookieName[];
			std::string cookie_token_;
			DISALLOW_EVIL_CONSTRUCTORS(iToolsSetCookie);
		};
		//fix:2015/3/17 add cookie flag "mz_mt0"
		char *iToolsSetCookie::kCookieName[] = {"USID",NULL};
		ThirdAppleId::ThirdAppleId(){
			set_root_directory();
		}
		ThirdAppleId::~ThirdAppleId(){

		}
		void ThirdAppleId::iToolsAppleId(){
			std::string message_header;
/*			message_header.append("Cookie: PHPSESSID=b44s4nl9tv44nkv3b97e2ks8n6\r\n");*/
			message_header.append("Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n");
			message_header.append(ABI::base::GBKToUtf8("User-Agent: iTools 3.8 (iPhone; iPhone OS 8.1.3; zh_CN)\r\n"));
			std::string message_body;
			const std::string name(GenRandom(12));
			const std::string register_info("user="+name+"&password="+name+"&confirmPassword="+name);
			const std::wstring filename(ABI::base::AsciiToUnicode(name).c_str());
			InfoLogger(filename.c_str(),"",0);
			passport::iTunesCookieInterface::GetInstance()->set_login_cookie_flag(true);
			message_body.append(
				std::string("self_v=3.6&system_v=8.1.2&internal_v=3000&macaddr=02%3A00%3A00%3A00%3A00%3A00&model=iPhone%205s&ptype=0&channelid=Shunganglong&signer=Shunganglong&bid=me.itutu.soul&")+register_info+std::string("&device_id=F91F4C32-C5B9-4FD5-8288-8EEEAFA200A6&savePassword=true&device=phone&from=phone&mac=0f607264fc6318a92b9e13c65db7cd3c&adid=F91F4C32-C5B9-4FD5-8288-8EEEAFA200A6"));
			std::string r_msg = passport::internal::SendHTTPS(L"appuser.slooti.com",
				L"/index.php?r=user/register",message_body.c_str(),message_body.length(),
				passport::internal::apple_itunes,
				AUniocde(message_header).c_str(),NULL);
			if(r_msg.find("{\"status\":true")==std::string::npos){
				return;
			}
			const std::wstring rule_begin = L"\"sessionId\":";
			const std::wstring rule_end = L",";
			std::vector<std::wstring> result = ABI::base::SplitMakePair(AUniocde(r_msg),rule_begin,rule_end);
			if(result.size()!=1){
				return;
			}
			result = ABI::base::SplitMakePair(result[0],L"\"",L"\"");
			message_body.resize(0);
			message_body.append("sessionId=");
			message_body.append(ABI::base::UnicodeToAscii(result[0]));
			message_body.append("&device=phone");
			message_header.resize(0);
			message_header.append("Cookie: ");
			iToolsSetCookie pass_token(passport::iTunesCookieInterface::GetInstance()->auth_response_header());
			message_header.append(pass_token.Cookie());
			message_header.append("\r\n");
			message_header.append("Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n");
			message_header.append(ABI::base::GBKToUtf8("User-Agent: iTools 3.8 (iPhone; iPhone OS 8.1.3; zh_CN)\r\n"));
			r_msg = passport::internal::SendHTTPS(L"appuser.slooti.com",
				L"/?r=user/getAid",message_body.c_str(),message_body.length(),
				passport::internal::apple_itunes,
				AUniocde(message_header).c_str(),NULL);
			InfoLogger(filename.c_str(),r_msg.c_str(),r_msg.length());
		}
		void ThirdAppleId::InfoLogger(const wchar_t* filename,const char* msg,size_t length){
			if(msg&&length){
				std::wstring save_dir = root_directory()+std::wstring(L"appleid");
				if(!PathFileExistsW(save_dir.c_str())){
					CreateDirectoryW(save_dir.c_str(),FALSE);
				}
				save_dir.append(L"\\");
				save_dir.append(filename);
				std::ofstream outfile(save_dir,std::ios::out|std::ios::app);
				outfile.write(msg,length);
				outfile.close();
			}
		}
	}
}