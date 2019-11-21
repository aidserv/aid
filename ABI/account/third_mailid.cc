#include "smartbot/account/third_mailid.h"
#include "smartbot/passport/itunes_https.h"
#include "smartbot/passport/itunes_cookie_interface.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/string_conv.h"
#include "thirdparty/glog/logging.h"
#include "smartbot/cookies/cookie_constants.h"
#include "smartbot/cookies/parsed_cookie.h"
#include "nsPassword/strong_key.h"

namespace smartbot{
	namespace account{
		class setCookie
		{
		public:
			explicit setCookie(const std::string& http_header):cookie_token_(""){
				USES_CONVERSION;
				std::vector<std::wstring> multi_line_cookie = ABI::base::SplitMakePair(A2W(http_header.c_str()),L"Set-Cookie: ",L"\r\n");
				std::vector<std::wstring>::iterator it;
				for(it=multi_line_cookie.begin();it!=multi_line_cookie.end();it++){
					net::ParsedCookie cookie(W2A(it->c_str()));
					for(int index=0;kCookieName[index]!=NULL;index++){
						std::string pair_name = cookie.Name();
						if(!strnicmp(pair_name.c_str(),kCookieName[index],strlen(kCookieName[index]))&&!cookie.Value().empty()){
							if(cookie_token_.find(cookie.Value())==std::string::npos){
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
			std::wstring CookieLine(){
				return ABI::base::AsciiToUnicode(cookie_token_);
			}
		private:
			static char *kCookieName[];
			std::string cookie_token_;
			DISALLOW_EVIL_CONSTRUCTORS(setCookie);
		};
		char *setCookie::kCookieName[] = {"JSESSIONID","ser_adapter","mailsync",NULL};
		std::string GenRandom(const unsigned long len){
			char buffer[MAX_PATH] = {0};
			std::string alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
			std::string str;
			str.reserve(32);
			Sleep(1000);
			srand((unsigned)time(NULL));
			for(unsigned long i=0;i<len;++i){
				buffer[i] = alphabet[rand() % alphabet.length()];
			}
			return buffer;
		}
		static const char safe[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

		std::string Encode(const std::string &uri)  {  
			std::string ret;
			const unsigned char *ptr = (const unsigned char *)uri.c_str();  
			ret.reserve(uri.length());  
			for (; *ptr ; ++ptr)  {  
				if (!safe[*ptr]){  
					char buf[5];  
					memset(buf, 0, 5);  
#ifdef WIN32  
					_snprintf_s(buf, 5, "%%%X", (*ptr));  
#else  
					snprintf(buf, 5, "%%%X", (*ptr));  
#endif  
					ret.append(buf);      
				}  
				else if (*ptr==' '){  
					ret+='+';  
				}  
				else{  
					ret += *ptr;  
				}  
			}  
			return ret;  
		}
		ThirdMailId::ThirdMailId(){

		}
		ThirdMailId::~ThirdMailId(){

		}
		void ThirdMailId::RegistedUser(){
			//read cookie
			std::wstring msg_header(L"Cookie: OUTFOX_SEARCH_USER_ID=\"-586738825@192.168.194.23\"\r\n");
			msg_header.append(L"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2243.0 Safari/537.36");
			passport::iTunesCookieInterface::GetInstance()->set_signup_wizard_cookie_flag(true);
			std::string msg = passport::internal::ReadHTTPS(L"ssl.mail.163.com",L"/regall/unireg/call.do?cmd=register.entrance&flow=wap_main&from=EMPTY",
				msg_header.c_str(),passport::internal::apple_itunes,L"https://ssl.mail.163.com/regall/unireg/call.do?cmd=register.entrance&flow=wap_mobile");
			//read verify code
			msg_header.resize(0);
			msg_header.append(L"Cookie: ");
			if(!passport::iTunesCookieInterface::GetInstance()->signup_wizard_cookie_flag()){
				msg_header.append(setCookie(passport::iTunesCookieInterface::GetInstance()->auth_response_header()).CookieLine());
			}
			msg_header.append(L"\r\n");
			msg_header.append(L"Accept: image/webp,*/*;q=0.8\r\n");
			msg_header.append(L"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2243.0 Safari/537.36");
			msg = passport::internal::ReadHTTPS(L"ssl.mail.163.com",L"/regall/unireg/call.do?cmd=register.verifyCode&vt=en2",
				msg_header.c_str(),passport::internal::apple_itunes,L"https://ssl.mail.163.com/regall/unireg/call.do?cmd=register.entrance&flow=wap_main&from=EMPTY");
			//registed user
			std::ofstream outfile("verifyCode.img",std::ios::out|std::ios::binary);
			outfile.write(msg.c_str(),msg.length());
			outfile.close();
			msg_header.resize(0);
			msg_header.append(L"Cookie: ");
			if(!passport::iTunesCookieInterface::GetInstance()->signup_wizard_cookie_flag()){
				msg_header.append(setCookie(passport::iTunesCookieInterface::GetInstance()->auth_response_header()).CookieLine());
				msg_header.append(L"; OUTFOX_SEARCH_USER_ID=\"-586738825@192.168.194.23\"");
			}
			msg_header.append(L"\r\n");
			msg_header.append(L"DNT: 1\r\n");
			msg_header.append(L"Accept-Encoding: gzip, deflate\r\n");
			msg_header.append(L"Cache-Control: max-age=0\r\n");
			msg_header.append(L"Origin: https://ssl.mail.163.com\r\n");
			msg_header.append(L"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2243.0 Safari/537.36\r\n");
			msg_header.append(L"Content-Type: application/x-www-form-urlencoded\r\n");
			const std::string username(Encode(GenRandom(nsPassword::kPassword16Bit-1)));
			const std::string password(Encode(GenRandom(nsPassword::kPassword16Bit-1)));
			std::wstring message_body;
			message_body.append(L"cmd=register.start&flow=wap_main&from=EMPTY&errview=");
			message_body.append(AUniocde(Encode("register/wap_main/step1")));
			message_body.append(L"&name=");
			message_body.append(AUniocde(username));
			message_body.append(L"&domain=163.com&password=");
			message_body.append(AUniocde(password));
			message_body.append(L"&confirmPassword=");
			message_body.append(AUniocde(password));
			message_body.append(L"&vcode=AYVVR");//verify code????????
			const std::string send_body = ABI::base::UnicodeToAscii(message_body);
			std::string response = passport::internal::SendHTTPS(L"ssl.mail.163.com",L"/regall/unireg/call.do",send_body.c_str(),send_body.length(),
				passport::internal::apple_itunes,msg_header.c_str(),L"https://ssl.mail.163.com/regall/unireg/call.do?cmd=register.entrance&flow=wap_main&from=EMPTY",NULL);
		}
	}
}