#include "smartbot/account/register_new.h"
#include "smartbot/passport/itunes_client_interface.h"
#include "smartbot/passport/itunes_internal_interface.h"
#include "smartbot/passport/itunes_cookie_interface.h"
#include "smartbot/passport/itunes_https.h"
#include "ABI/base/string/string_case.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/string_conv.h"
#include <openssl/evp.h>
#include "smartbot/cookies/cookie_constants.h"
#include "smartbot/cookies/parsed_cookie.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/glog/scoped_ptr.h"

namespace smartbot{
	namespace account{
		class signupWizardCookie
		{
		public:
			explicit signupWizardCookie(const std::string& http_header):cookie_token_(""){
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
			DISALLOW_EVIL_CONSTRUCTORS(signupWizardCookie);
		};
		char *signupWizardCookie::kCookieName[] = {"hsaccnt","mzf_in","Pod","itsMetricsR","groupingPillToken","itspod","X-Dsid","mz_at0-","mz_at_ssl-","wosid-lite","ns-mzf-inst","session-store-id","s_vi","woinst","xp_ci","wosid","mzf_dr","ns-mzf-inst","ndcd",NULL};
		RegisterNew::RegisterNew(){
			Free();
			passport::communicates::singleton()->ResetSapSetup(true);
			passport::communicates::singleton()->SendAuthorizeMachineProtocol();
		}
		RegisterNew::~RegisterNew(){
			Free();
		}
		bool RegisterNew::signupWizard(){
			std::string message_header;
			const std::string boundary_name = "7E9391F731947C696914BAD02CE17785";
			message_header.append("Content-Type: multipart/form-data; boundary=");
			message_header.append(boundary_name);
			message_header.append("\r\n");
			message_header.append("X-Apple-Tz: 28800\r\n");
			message_header.append("X-Apple-Store-Front: 143465-19,28\r\n");
			passport::internal::GenerateAuthenticateOsGUID os_guid;
			std::string message_body;
			message_body.append("--");
			message_body.append(boundary_name);
			message_body.append("\r\n");
			message_body.append("Content-Disposition: form-data; name=\"guid\"\r\n\r\n");
			message_body.append(os_guid.machine_guid());
			message_body.append("\r\n");
			message_body.append("--");
			message_body.append(boundary_name);
			message_body.append("\r\n");
			message_body.append("Content-Disposition: form-data; name=\"machineName\"\r\n\r\n");
			message_body.append(os_guid.machine_name());
			message_body.append("\r\n");
			message_body.append("--");
			message_body.append(boundary_name);
			message_body.append("--\r\n");
			message_header.append("X-Apple-ActionSignature: ");
			message_header.append(CalcXAppleActionSignature());
			message_header.append("\r\n");
			passport::iTunesCookieInterface::GetInstance()->set_signup_wizard_cookie_flag(true);
			std::string buy_result = passport::internal::SendHTTPS(L"buy.itunes.apple.com",
				L"/WebObjects/MZFinance.woa/wa/signupWizard",message_body.c_str(),message_body.length(),
				passport::internal::apple_itunes,
				AUniocde(message_header).c_str(),
				L"https://buy.itunes.apple.com/WebObjects/MZFinance.woa/wa/signupWizard");
			CalcXAppleActionSignature(passport::iTunesCookieInterface::GetInstance()->auth_response_header());
			const std::wstring rule_begin = L"a role=\"button\" href=\"";
			const std::wstring rule_end = L"><button type=\"button\" class=\"emphasized\">";
			std::vector<std::wstring> result = ABI::base::SplitMakePair(AUniocde(buy_result),rule_begin,rule_end);
			if(result.size()!=1){
				return false;
			}
			const std::wstring keep_match = result[0];
			result = ABI::base::SplitMakePair(keep_match,L"href=\"",L"\"");
			if(result.size()!=1){
				return false;
			}
			return (signupWizardFlushOK(result[0]));
		}
		bool RegisterNew::signedApplePublicLicense(){
			std::string message_header = GetRequestHeader();
			passport::internal::GenerateAuthenticateOsGUID os_guid;
			message_header.append("Content-Type: application/x-www-form-urlencoded\r\n");
			std::wstring message_body = L"";
			message_body.append(agree_value_);
			message_body.append(L"=");
			message_body.append(agree_value_);
			message_body.append(L"&mzPageUUID=");
			message_body.append(mz_page_uuid_);
			message_body.append(L"&machineGUID=");
			message_body.append(ABI::base::AsciiToUnicode(os_guid.machine_guid()));
			message_body.append(L"&xAppleActionSignature=");
			std::string x_act_sig = ABI::base::UnicodeToAscii(mz_page_uuid_)+os_guid.machine_guid();
			message_body.append(ABI::base::AsciiToUnicode(CalcXAppleActionSignature(x_act_sig.c_str(),x_act_sig.length())));
			const std::string send_body = ABI::base::UnicodeToAscii(message_body);
			std::string response = passport::internal::SendHTTPS(L"buy.itunes.apple.com",agree_url_.c_str(),send_body.c_str(),send_body.length(),
				passport::internal::apple_itunes,AUniocde(message_header).c_str(),referer_url_.c_str(),NULL);
			const std::wstring rule_begin = L"secure-connection";
			const std::wstring rule_end = L"edit-account";
			std::vector<std::wstring> result = ABI::base::SplitMakePair(AUniocde(response),rule_begin,rule_end);
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"action=\"",L"\">");
			if(result.size()!=1){
				return false;
			}
			reg_info_url_ = result[0];
			result = ABI::base::SplitMakePair(AUniocde(response),L"mzPageUUID",L"/>");
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"'",L"'");
			reg_page_uuid_ = result[0];
			static const char* fill_id_name[] = {"emailAddress",
				"pass1",
				"pass2",
				"question1",
				"answer1",
				"question2",
				"answer2",
				"question3",
				"answer3",
				"recoveryEmail",
				"birthYear",
				"birthMonthPopup",
				"birthDayPopup",
				"newsletter",
				"marketing",
				"input class=\"continue\"",
				NULL
			};
			static const wchar_t* fill_id_value[] = {
				L"981902528@qq.com",
				L"QAZ2wsx3edc",
				L"QAZ2wsx3edc",
				L"0",
				L"12345",
				L"1",
				L"1234567",
				L"1",
				L"12345678",
				L"918831380@qq.com",
				L"1992",
				L"0",
				L"0",
				L"0",
				L"0",
				L"继续",
				NULL
			};
			for(int i=0;fill_id_name[i]!=NULL;i++){
				result = ABI::base::SplitMakePair(AUniocde(response),AUniocde(fill_id_name[i]),L"/>");
				result = ABI::base::SplitMakePair(result[0],L"name=\"",L"\"");
				if(std::string(fill_id_name[i])=="newsletter"||std::string(fill_id_name[i])=="marketing"){
					const std::wstring name = result[0];
					result = ABI::base::SplitMakePair(AUniocde(response),AUniocde(fill_id_name[i]),L"/>");
					result = ABI::base::SplitMakePair(result[0],L"value=\"",L"\"");
					reg_info_[AUniocde(fill_id_name[i])+L","+name] = result[0];
					if(!reg_msg_body_.empty()){
						reg_msg_body_.append(L"&");
					}
					reg_msg_body_.append(name);
					reg_msg_body_.append(L"=");
					reg_msg_body_.append(result[0]);
				}
				else{
					reg_info_[AUniocde(fill_id_name[i])+L","+result[0]] = ABI::base::AsciiToUnicode(ABI::base::UnicodeToUft8(fill_id_value[i]));
					if(!reg_msg_body_.empty()){
						reg_msg_body_.append(L"&");
					}
					reg_msg_body_.append(result[0]);
					reg_msg_body_.append(L"=");
					reg_msg_body_.append(ABI::base::AsciiToUnicode(ABI::base::UnicodeToUft8(fill_id_value[i])));
				}
				
			}
			reg_referer_ = std::wstring(L"https://buy.itunes.apple.com")+agree_url_;
			return true;
		}
		bool RegisterNew::paddingRegisteredInformation(const wchar_t* response_save){
			std::string message_header = GetRequestHeader();
			passport::internal::GenerateAuthenticateOsGUID os_guid;
			message_header.append("Content-Type: application/x-www-form-urlencoded\r\n");
			std::wstring message_body = reg_msg_body_;
			message_body.append(L"&mzPageUUID=");
			message_body.append(reg_page_uuid_);
			message_body.append(L"&machineGUID=");
			message_body.append(ABI::base::AsciiToUnicode(os_guid.machine_guid()));
			message_body.append(L"&xAppleActionSignature=");
			passport::iTunesCookieInterface::GetInstance()->set_signup_wizard_cookie_flag(true);
			std::string x_act_sig = ABI::base::UnicodeToAscii(reg_page_uuid_)+os_guid.machine_guid() + std::string("918831380@qq.com");
			message_body.append(ABI::base::AsciiToUnicode(CalcXAppleActionSignature(x_act_sig.c_str(),x_act_sig.length())));
			const std::string send_body = ABI::base::UnicodeToAscii(message_body);
			std::string response = passport::internal::SendHTTPS(L"buy.itunes.apple.com",reg_info_url_.c_str(),send_body.c_str(),send_body.length(),
				passport::internal::apple_itunes,AUniocde(message_header).c_str(),reg_referer_.c_str(),NULL);
			const std::wstring rule_begin = L"secure-connection";
			const std::wstring rule_end = L"edit-billing";
			std::vector<std::wstring> result = ABI::base::SplitMakePair(AUniocde(response),rule_begin,rule_end);
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"action=\"",L"\">");
			if(result.size()!=1){
				return false;
			}
			reg_bank_url_ = result[0];
			result = ABI::base::SplitMakePair(AUniocde(response),L"mzPageUUID",L"/>");
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"'",L"'");
			reg_bank_uuid_ = result[0];
			static const char* fill_id_name[] = {
				"id=\"country\"",
				"credit-card-type",
				"sp",
				"res",
				"id=\"lastFirstName\"",
				"id=\"firstName\"",
				"id=\"street1\"",
				"id=\"street2\"",
				"id=\"street3\"",
				"id=\"city\"",
				"id=\"postalcode\"",
				"id=\"state\"",
				"id=\"phone1Number\"",
				"input class=\"continue\"",
				NULL
			};
			static const char* fill_id_value[] = {
				"EN",
				NULL,
				NULL,
				NULL,
				"FirstName",//第一个姓
				"SeconeName",//第二个名
				"ChangPing",//街名和门牌号
				"",//楼号、单元号、房间号
				"",//街
				"BeiJing",//市级行政区
				"405200",//邮政编码
				"3", //选择省份
				"18315054311",
				"Create Apple ID",
				NULL
			};
			for(int i=0;fill_id_name[i]!=NULL;i++){
				result = ABI::base::SplitMakePair(AUniocde(response),AUniocde(fill_id_name[i]),L"/>");
				result = ABI::base::SplitMakePair(result[0],L"name=\"",L"\"");
				if(i==1||i==2||i==3){
					if(!reg_bank_body_.empty()){
						reg_bank_body_.append(L"&");
					}
					reg_bank_body_.append(AUniocde(fill_id_name[i]));
					reg_bank_body_.append(L"=");
					reg_bank_body_.append(L"");
				}
				else if(i==13){
 					reg_bank_body_.append(L"&ndpd-s=1.w-895561.1.2.9UUAJh1LJ1ZwKfSlBa9rbA%2C%2C.FCSRyCvthksHoCpVd6P9fiTYOlmgxzo5fndW5rsWKTa_nXFO8kBwRaV25OYSco1AejgYHjEWf2RXwdXnTGjhq-yccCuU8uJupcWNYjHr0wrJg9bNQcYhMvDdbGgGMlQ6qf7yPvm5irMJKsPIea04pli2Ng44HyOl_odl9lQX2fEYcGnxwFAR9u-HSDh-Fl8PI1xgdWdyK17CvuZEXj6gQq94CWsgqHeD7IPrM960F6lJ1aQKvNNHvUhm3QsBz6XAnqjIf8oqh9eqwJhblrtUIW0nssSR_Edq_HGCEBtsAFdQIQkJz1U26JdJV8iRdHbNRiMEwRjUFARIiTVN0YfTWg%2C%2C&ndpd-f=1.w-895561.1.2.LWVFNxLpb-FoV546YKuwwg%2C%2C.H22REESxoEMuzPYPeRvcNBOO7JbUe3HLTPm33Yj991pT4vb09JZ_NoocFU1xLBgSLr4017O01G8-1gdUAb92CWcrBIR_iA77dEMo55QhxLEBK0TuTxQcJTFqtMPzSYHX64F-wlMUJo9GU-Y0oNx5l77RPC55F74X5CEcUUPbg9Z2D0-jjjaC_OdGljbDw8k-2gvUNwwiOTg8tp2Hk9WEhU_g0L4RaHJ8KQ-G-FAuv9dqqJRsT3BOM2KzajrsLEJax7dTS-fHZo4gLkyZybAVGi2VIix22hgSC60d3olFQUmSsyr9s25NgUpz7nCygYJd4uy895HijAou5fP8mMurOg%2C%2C&ndpd-fm=1.w-895561.1.2.6SGOn-cSzBiGcYn3KsHbgA%2C%2C.ysjojYAX5van5Dt5e8we5X_C1xT6VzEzXfTivx8YnfYGHVrglAHO4Apa13JjIcgXzT2T9WfcRxQG4AH0DhBeowQrOZA9VS6X6WbxYHjTFK0nCKKjvDxSNaFFFmBqwYqxoRwnvH5kgcJ0Cnn61_ez6YZwJE3csgupZBdfCib_GS6xTQH_YE0CJlhpTJ03Fc8qjScEJB-mJ43rY-kl7I4aBN1FvEpX_exgWTg8ye_f3-yu65XzY0OVkkH3gELp5rHZLEpMJJJo9c_8tAqUONYtcjTjGIjpVKonM8pVgoIqx14-c2yJFIVvRPpwRmjz1ZxmYUufCodiQsiYGt2Vshx44Q%2C%2C&ndpd-w=1.w-895561.1.2.LclhcbCqVloRx7mRKB6r7w%2C%2C.IDSp259Vk78-NF7wk0RXOyQvEN98jxnYKW7KqJvdAuYxvVhgyQdoDur_1FMBltrQU6vILIY4GbTs6sq8ld3ae68eVMQOHySviooCrAybqlQN-HooO1O-i578t1YWGkHfPekciwIs86Pzl7U93achCza2QAjrPE9GDqniBfRkwlkUDz7TbdT3tdxSWEoyRERhaOM4W5y8hQpMsyYWx2lQrwKDsCUaH2pbhRtVpD77jmaFa1UHujLdbNsUNUaghbp0R6HDOWRWoPes3L-awVrBB-pyH3dBbrJCfSGbNziadfOLZemS_x7CBqD1IRSbye75jPNQEJfk9rGpFVXwCT1DsR2FtPCjtlGfhyv5Qrl76uUPonntmXvTL5hnK2PugexW&ndpd-ipr=ncip%2C0%2C550919da%2C1%2C1%3Bst%2C0%2ClastFirstName%2C4%2CfirstName%2C8%2Cstreet1%2C6%2Cstreet2%2C0%2Cstreet3%2C0%2Ccity%2C6%2Cpostalcode%2C6%2Cphone1Number%2C11%3Bmm%2C6f%2C10b%2C266%3Bmm%2C1180%2C209%2C2%3Bmm%2C26ef%2C16d%2C25d%3Bmc%2C631%2C3c5%2C2d1%2C2.0.1.1.3.0.7.11.3.9.1%3Bts%2C0%2C3f0f%3Bfs%2C4%2C0%2C0%2C%3B&ndpd-di=d1-e4a1c95fee11e36d&ndpd-bi=b1.1680x1050+1680x1010+24+24.-480.zh-CN&ndpd-wk=10205&ndpd-vk=7749&ndpd-wkr=663877&captchaMode=VIDEO");
					if(!reg_bank_body_.empty()){
						reg_bank_body_.append(L"&");
					}
					reg_bank_body_.append(result[0]);
					reg_bank_body_.append(L"=");
					reg_bank_body_.append(AUniocde(fill_id_value[i]));
				}
				else{
					if(!reg_bank_body_.empty()){
						reg_bank_body_.append(L"&");
					}
					reg_bank_body_.append(result[0]);
					reg_bank_body_.append(L"=");
					reg_bank_body_.append(AUniocde(fill_id_value[i]));
				}

			}
			reg_referer_ = std::wstring(L"https://buy.itunes.apple.com")+reg_info_url_;
			if(response_save!=NULL){
				std::ofstream out(response_save,std::ios::binary|std::ios::out);
				out.write(response.c_str(),response.length());
				out.close();
				out.clear();
			}
			return true;
		}
		bool RegisterNew::paddingPurchaseInformation(){
			std::string message_header = GetRequestHeader();
			passport::internal::GenerateAuthenticateOsGUID os_guid;
			message_header.append("Content-Type: application/x-www-form-urlencoded\r\n");
			std::wstring message_body = reg_bank_body_;
			message_body.append(L"&machineGUID=");
			message_body.append(ABI::base::AsciiToUnicode(os_guid.machine_guid()));
			message_body.append(L"&mzPageUUID=");
			message_body.append(reg_page_uuid_);
			message_body.append(L"&xAppleActionSignature=");
			std::string x_act_sig = ABI::base::UnicodeToAscii(reg_bank_uuid_)+os_guid.machine_guid();
			message_body.append(ABI::base::AsciiToUnicode(CalcXAppleActionSignature(x_act_sig.c_str(),x_act_sig.length())));
			message_body.append(L"&longName=Firstname,Seconename");
			const std::string send_body = ABI::base::UnicodeToAscii(message_body);
			std::string response = passport::internal::SendHTTPS(L"buy.itunes.apple.com",reg_bank_url_.c_str(),send_body.c_str(),send_body.length(),
				passport::internal::apple_itunes,AUniocde(message_header).c_str(),reg_referer_.c_str(),NULL);
			std::cout<<ABI::base::Utf8ToGBK(response.c_str()).c_str()<<std::endl;
			return true;
		}
		std::string RegisterNew::GetRequestHeader(){
			std::string message_header;
			message_header.append("Cookie: ");
			if(!passport::iTunesCookieInterface::GetInstance()->signup_wizard_cookie_flag()){
				message_header.append(signupWizardCookie(passport::iTunesCookieInterface::GetInstance()->auth_response_header()).Cookie());
			}
			message_header.append("\r\n");
			message_header.append("X-Apple-Tz: 28800\r\n");
			message_header.append("X-Apple-Store-Front: 143465-19,28\r\n");
			return message_header;
		}
		std::wstring RegisterNew::reg_bank_url(){
			return std::wstring(std::wstring(L"https://buy.itunes.apple.com")+reg_bank_url_);
		}
		bool RegisterNew::signupWizardFlushOK(const std::wstring url){
			std::string message_header = GetRequestHeader();
			const std::wstring ref_url = L"https://buy.itunes.apple.com/WebObjects/MZFinance.woa/wa/signupWizard";
			std::string response = passport::internal::ReadHTTPS(L"buy.itunes.apple.com",url.c_str(),
				AUniocde(message_header).c_str(),
				passport::internal::apple_itunes,
				ref_url.c_str(),
				NULL);
			const std::wstring rule_begin = L"accept-terms";
			const std::wstring rule_end = L"agree-checkbox";
			std::vector<std::wstring> result = ABI::base::SplitMakePair(AUniocde(response),rule_begin,rule_end);
			if(result.size()!=1){
				return false;
			}
			const std::wstring keep_match = result[0];
			result = ABI::base::SplitMakePair(keep_match,L"action=\"",L"\">");
			if(result.size()!=1){
				return false;
			}
			agree_url_ = result[0];
			result = ABI::base::SplitMakePair(AUniocde(response),L"mzPageUUID",L"/>");
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"'",L"'");
			mz_page_uuid_ = result[0];
			result = ABI::base::SplitMakePair(AUniocde(response),L"checkbox",L"pageUUID");
			if(result.size()!=1){
				return false;
			}
			result = ABI::base::SplitMakePair(result[0],L"value=\"",L"\"");
			if(result.size()!=1){
				return false;
			}
			agree_value_ = result[0];
			referer_url_ = std::wstring(L"https://buy.itunes.apple.com")+url;
			return true;
		}
		std::string RegisterNew::CalcXAppleActionSignature(){
			unsigned char* x_a_act_sig = nullptr;
			unsigned long act_sig_len = 0;
			const unsigned long kbsync_id = passport::internal::GetKbSyncId();
			passport::iTunesInternalInterface::Instance()->lpfnSapGetAS(kbsync_id,100,0,0,passport::ToDword(&x_a_act_sig),passport::ToDword(&act_sig_len));
			scoped_array<unsigned char> act_sig_from_server(new unsigned char[kMaxBufferLength]);
			int length = EVP_EncodeBlock(act_sig_from_server.get(),x_a_act_sig,act_sig_len);
			return std::string(reinterpret_cast<char*>(act_sig_from_server.get()));
		}
		void RegisterNew::CalcXAppleActionSignature(const std::string& httpheader){
			const wchar_t* cookie_key[] = {L"X-Apple-ActionSignature: ",L"x-apple-actionsignature: ",NULL};
			USES_CONVERSION;
			const std::wstring tmp = A2W(httpheader.c_str());
			for(int i=0;cookie_key[i]!=NULL;i++){
				std::vector<std::wstring> x_act_sig = ABI::base::SplitMakePair(tmp,cookie_key[i],L"\r\n");
				if(x_act_sig.size()){
					const unsigned long kbsync_id = passport::internal::GetKbSyncId();
					scoped_array<unsigned char> act_sig_from_server(new unsigned char[kMaxBufferLength]);
					const std::string x_act = W2A(x_act_sig[0].c_str());
					size_t decode_len = EVP_DecodeBlock(act_sig_from_server.get(),(const unsigned char*)x_act.c_str(),x_act.length());
					int err=passport::iTunesInternalInterface::Instance()->lpfnSapGetASFD_a(kbsync_id,passport::ToDword(act_sig_from_server.get()),decode_len,0,0);
					break;
				}
			}
		}
		std::string RegisterNew::CalcXAppleActionSignature(const void* src,size_t length){
			unsigned char* x_a_act_sig = nullptr;
			unsigned long act_sig_len = 0;
			const unsigned long kbsync_id = passport::internal::GetKbSyncId();
			passport::iTunesInternalInterface::Instance()->lpfnSapGetASFD(kbsync_id,passport::ToDword(src),length,passport::ToDword(&x_a_act_sig),passport::ToDword(&act_sig_len));
			scoped_array<unsigned char> act_sig_from_server(new unsigned char[kMaxBufferLength]);
			EVP_EncodeBlock(act_sig_from_server.get(),x_a_act_sig,act_sig_len);
			return std::string(reinterpret_cast<char*>(act_sig_from_server.get()));
		}
		void RegisterNew::Free(){
			agree_url_.resize(0);
			mz_page_uuid_.resize(0);
			agree_value_.resize(0);
			signup_wizard_response_.resize(0);
			referer_url_.resize(0);
			reg_info_url_.resize(0);
			reg_page_uuid_.resize(0);
			reg_referer_.resize(0);
			reg_info_.clear();
			reg_msg_body_.clear();
			reg_bank_body_.resize(0);
		}
	}
}