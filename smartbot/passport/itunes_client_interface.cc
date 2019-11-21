#include "passport/itunes_client_interface.h"
#include <cassert>
#include "passport/itunes_internal_interface.h"
#include "passport/itunes_https.h"
#include "passport/itunes_native_interface.h"
#include "passport/itunes_cookie_interface.h"
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ssleay32.lib")
#include "base/string/string_split.h"
#include "base/string/string_conv.h"
#include <cookies/parsed_cookie.h>
#include <atlconv.h>
#include "thirdparty/glog/logging.h"
#include "thirdparty/glog/scoped_ptr.h"
#include "thirdparty/jsoncpp/json/value.h"
#include "thirdparty/jsoncpp/json/writer.h"
#include "ABI/ios_authorize/iTunesInterface.h"


namespace passport{
	const unsigned long kMaxCertLength = 1024*1024*8;
	namespace internal{
		unsigned long kSyncId = 0;
		unsigned long kMachineIdA[6] = {0x00000006,0x0B32EE8B,0x00004011,0x00000000};
		unsigned long kMachineIdB[6] = {0x00000006,0x3293AE72,0x0000D1C1,0x00000000};
		unsigned long GetKbSyncId(){
			return kSyncId;
		}
		std::string GetLoginText(const std::string& apple_id,const std::string& password,const char* machine_name,const char* machine_guid){
			ABI::base::HardwareInfo hardware;
			const std::string string_username = std::string("<string>")+apple_id+std::string("</string>");
			const std::string string_password = std::string("<string>")+password+std::string("</string>");
			const std::string string_guid = std::string("<string>")+machine_guid+std::string("</string>");
			const std::string string_machine_name = std::string("<string>")+machine_name+std::string("</string>");
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
			return plist;
		}
	
		std::string GetProductFormatInfo(const std::string& kbsync_data,
			const char* product_id,
			const std::string& credit,
			const char* machine_guid,
			const char* machine_name,bool expense = false){
			//reference:http://www.apple.com/itunes/affiliates/resources/documentation/itunes-store-web-service-search-api.html
			scoped_array<char> buffer(new char[kMaxCertLength]);
			std::string upload_info;
			upload_info.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
			upload_info.append("<plist version=\"1.0\"><dict>");
			//date:2015/09/06 upload_info.append("<key>appExtVrsId</key><string>812138967</string>");//http://www.oschina.net/code/snippet_184012_19659 keyword:appExtVrsId
			//date:2015/09/06
			if (expense)
				upload_info.append("<key>buyAndSkipHarvesting</key><string>true</string>");
			//date:2015/09/06
			if (expense)
				upload_info.append("<key>buyWithoutAuthorization</key><string>true</string>");
			if (!credit.empty())
				upload_info.append(std::string("<key>creditDisplay</key><string>\xC2\xA5")+std::string(&credit[2])+std::string("</string>"));//¥
			else
				upload_info.append("<key>creditDisplay</key><string></string>");//¥
			upload_info.append("<key>guid</key><string>%s</string>");//guid
			//date:2015/09/06
			if (expense)
				upload_info.append("<key>hasAskedToFulfillPreorder</key><string>true</string>");
			//date:2015/09/06
			if (expense){
				upload_info.append("<key>hasBeenAuthedForBuy</key><string>true</string>");
				upload_info.append("<key>hasDoneAgeCheck</key><string>true</string>");
			}
			upload_info.append("<key>kbsync</key><data>%s</data>");//kbsync
			upload_info.append("<key>machineName</key><string>%s</string>");//machine name
			upload_info.append("<key>needDiv</key><string>0</string>");//free?1:0??????
			upload_info.append("<key>origPage</key><string>Software</string>");
			upload_info.append("<key>origPage2</key><string>Genre-CN-Mobile Software Applications-29099</string>");
			upload_info.append("<key>origPageCh</key><string>Software Pages</string>");
			upload_info.append("<key>origPageCh2</key><string>Mobile Software Applications-main</string>");
			upload_info.append("<key>origPageLocation</key><string>Buy</string>");
			upload_info.append(ABI::base::GBKToUtf8("<key>origPageLocation2</key><string>Tab_iphone|Titledbox_热门付费 App|Lockup_8</string>"));
			//date:2015/09/07 original:<key>price</key><string>1000</string>
			upload_info.append("<key>price</key><string>0</string>");//https://itunes.apple.com/lookup?id=388624839 keyword:price
			upload_info.append("<key>pricingParameters</key><string>STDQ</string>");
			upload_info.append("<key>productType</key><string>C</string>");
			upload_info.append("<key>salableAdamId</key><string>%s</string>");
			if(expense)
				upload_info.append("<key>wasWarnedAboutFirstTimeBuy</key><string>true</string>");
			upload_info.append("</dict></plist>");
			_snprintf(buffer.get(),kMaxCertLength,upload_info.c_str(),machine_guid,kbsync_data.c_str(),machine_name,product_id);
			return std::string(buffer.get());
		}
		std::string GetFormat_pendingSongs(const char* guid,const char* kb_sync,const char* pc_name){
			scoped_array<char> buffer(new char[kMaxCertLength]);
			std::string upload_info;
			upload_info.append("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
			upload_info.append("<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">");
			upload_info.append("<plist version=\"1.0\"><dict>");
			upload_info.append("<key>auto-check</key><string>1</string>");
			upload_info.append("<key>guid</key><string>%s</string>");
			upload_info.append("<key>kbsync</key><data>%s</data>");
			upload_info.append("<key>machineName</key><string>%s</string>");
			upload_info.append("<key>needDiv</key><string>0</string>");
			upload_info.append("</dict></plist>");
			_snprintf(buffer.get(),kMaxCertLength,upload_info.c_str(),guid,kb_sync,pc_name);
			return std::string(ABI::base::UnicodeToUft8(ABI::base::AsciiToUnicode(buffer.get())));
		}
		std::string GetFormat_registerSuccess(const char* pc_name,const char* guid){
			scoped_array<char> buffer(new char[kMaxCertLength]);
			std::string upload_info;
			upload_info.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
			upload_info.append("<plist version=\"1.0\"><dict>");
			upload_info.append("<key>device-name</key><string>%s</string>");
			upload_info.append("<key>environment</key><string>production</string>");
			upload_info.append("<key>guid</key><string>%s</string>");
			upload_info.append("<key>serial-number</key><string>0</string>");
			upload_info.append("<key>token</key><data>dgH+NUHUu0XCrUcExIsa3xUzljya9Y99eJWtvelTzB4=</data>");
			upload_info.append("</dict></plist>");
			_snprintf(buffer.get(),kMaxCertLength,upload_info.c_str(),pc_name,guid);
			return std::string(ABI::base::UnicodeToUft8(ABI::base::AsciiToUnicode(buffer.get())));
		}
		std::string GetFormat_finishMachineProvisioning(const char* cli_data,const char* guid,const char* kb_sync,const char* machine_name){
			scoped_array<char> buffer(new char[kMaxCertLength]);
			std::string upload_info;
			upload_info.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">");
			upload_info.append("<plist version=\"1.0\"><dict>");
			upload_info.append("<key>clientData</key><string>%s</string>");
			upload_info.append("<key>guid</key><string>%s</string>");
			upload_info.append("<key>kbsync</key><data>%s</data>");
			upload_info.append("<key>machineName</key><string>%s</string>");
			upload_info.append("</dict></plist>");
			_snprintf(buffer.get(),kMaxCertLength,upload_info.c_str(),cli_data,guid,kb_sync,machine_name);
			return std::string(ABI::base::UnicodeToUft8(ABI::base::AsciiToUnicode(buffer.get())));
		}
		std::string GetSCInfoFolder(){
			char path[MAX_PATH] = {0};
			SHGetSpecialFolderPathA(NULL,path,CSIDL_COMMON_APPDATA,FALSE);
			lstrcatA(path,"\\Apple Computer\\iTunes\\SC Info");
			return (std::string(path));
		}
	
		std::string GetKeyValue(const std::string& key,const std::string h_table){
			const unsigned long key_position = h_table.find(key);
			if(key_position==std::string::npos){
				return "";
			}
			std::string key_text(&h_table[key_position]);
			const uint32 start = key_text.find("<string>");
			const uint32 end = key_text.find("</string>");
			key_text = key_text.substr(start,end-start);
			return std::string(key_text.substr(8,std::string::npos));
		}
		unsigned long GetInterMD5(const char* msg,const size_t len){
			MD5_CTX md5_ctx = {0};
			unsigned char digest[32] = {0};
			if(len){
				MD5_Init(&md5_ctx);
				MD5_Update(&md5_ctx,msg,len);
				MD5_Final(digest,&md5_ctx);
			}
			return *reinterpret_cast<unsigned long*>(&digest[0]);
		}
		void KbSyncIdParameter::Initialize(){
			ABI::base::HardwareInfo hardware;
			std::string tmp;
			MD5_CTX md5_ctx = { 0 };
			unsigned char digest[32] = { 0 };
			std::string full_calc("cache-controlEthernet");
			std::string hash_calc("cache-controlEthernet");
			hardware.GetVolumeSerial(tmp);
			full_calc.append(tmp);
			unsigned long inter_md5 = GetInterMD5(tmp.c_str(), tmp.length());
			hash_calc.append((const char*)&inter_md5, inter_md5 ? sizeof(unsigned long) : 0);
			tmp.resize(0);
			hardware.GetSystemBios(tmp);
			full_calc.append(tmp);
			inter_md5 = GetInterMD5(tmp.c_str(), tmp.length());
			hash_calc.append((const char*)&inter_md5, inter_md5 ? sizeof(unsigned long) : 0);
			tmp.resize(0);
			hardware.GetProcessorName(tmp);
			full_calc.append(tmp);
			inter_md5 = GetInterMD5(tmp.c_str(), tmp.length());
			hash_calc.append((const char*)&inter_md5, inter_md5 ? sizeof(unsigned long) : 0);
			tmp.resize(0);
			hardware.GetWinProductId(tmp);
			full_calc.append(tmp);
			inter_md5 = GetInterMD5(tmp.c_str(), tmp.length());
			hash_calc.append((const char*)&inter_md5, inter_md5 ? sizeof(unsigned long) : 0);
			tmp.resize(0);
			MD5_Init(&md5_ctx);
			MD5_Update(&md5_ctx, full_calc.c_str(), full_calc.length());
			MD5_Final(digest, &md5_ctx);
			memmove(&kMachineIdB[1], &digest[0], 6);
			unsigned long* tmp1 = &kMachineIdB[0];
			std::string hw_cookie = ABI::base::GetHardwareCookie();
			unsigned long hw_hex[kMaxCertLength / 1024 * 8] = { 0 };
			_snscanf(hw_cookie.c_str(), hw_cookie.length(), "%x.%x.%x.%x.%x.%x", &hw_hex[0], &hw_hex[1], &hw_hex[4], &hw_hex[3], &hw_hex[2], &hw_hex[5]);
			memset((void*)&md5_ctx, 0, sizeof(MD5_CTX));
			memset(digest, 0, 32);
			MD5_Init(&md5_ctx);
			MD5_Update(&md5_ctx, hash_calc.c_str(), hash_calc.length());
			MD5_Final(digest, &md5_ctx);
			memmove(&kMachineIdA[1], &digest[0], 6);
		}
		DWORD KbSyncIdParameter::GetKbsyncIDAddress(){
			return reinterpret_cast<DWORD>(iTunesInternalInterface::Instance()->lpfnKbsyncID);
		}
		const char* KbSyncIdParameter::AllPCMd5(){
			return (const char*)&kMachineIdB[0];
		}
		const char* KbSyncIdParameter::LocalPCMd5(){
			return (const char*)&kMachineIdA[0];
		}
		void KBSyncMachineIdInitialize(){
			KbSyncIdParameter::Initialize();
			iTunesInternalInterface::Instance()->kb_seed = GenerateKbsyncID(KbSyncIdParameter::GetKbsyncIDAddress(),
				KbSyncIdParameter::AllPCMd5(),
				KbSyncIdParameter::LocalPCMd5());
		}
		std::string GetAuthorizeMachine_kbsync(){
			unsigned char* kb_dsid = NULL;
			unsigned long kb_length = 0;
			if(iTunesInternalInterface::Instance()->lpfnKbsync==NULL){
				return "";
			}
			const uint64 dsid = static_cast<uint64>(atof(iTunesCookieInterface::GetInstance()->x_dsid().c_str()));
			const MakeLongLong h_dsid = {HIDWORD(dsid),LODWORD(dsid)};
			const unsigned long kb_seed = iTunesInternalInterface::Instance()->kb_seed;
			if(!iTunesInternalInterface::Instance()->lpfnKbsync(kb_seed,0xE8ED9F79,1,0,0xB,ToDword(&kb_dsid),ToDword(&kb_length))){
				scoped_array<unsigned char> kb_buffer(new unsigned char[kMaxCertLength]);
				if(EVP_EncodeBlock(kb_buffer.get(),kb_dsid,kb_length)!=-1){
					return (std::string((const char*)kb_buffer.get()));
				}
			}
			return "";
		}
		bool GetKbsyncToken(){
			unsigned char* kb_dsid = NULL;
			unsigned long kb_length = 0;
			if(iTunesInternalInterface::Instance()->lpfnKbsync==NULL){
// 				0E55F47C   4178B033
// 				0E55F480   E8ED9F79
// 				0E55F484   00000001
// 				0E55F488   00000000
// 				0E55F48C   0000000B
// 				0E55F490   0E55F4A8
// 				0E55F494   0E55F49C
				return false;
			}
			const uint64 dsid = static_cast<uint64>(atof(iTunesCookieInterface::GetInstance()->x_dsid().c_str()));
			const MakeLongLong h_dsid = {HIDWORD(dsid),LODWORD(dsid)};
			const unsigned long kb_seed = iTunesInternalInterface::Instance()->kb_seed;
			if(!iTunesInternalInterface::Instance()->lpfnKbsync(kb_seed,h_dsid.low,h_dsid.high,0,1,ToDword(&kb_dsid),ToDword(&kb_length))){
				scoped_array<unsigned char> kb_buffer(new unsigned char[kMaxCertLength]);
				if(EVP_EncodeBlock(kb_buffer.get(),kb_dsid,kb_length)!=-1){
					iTunesCookieInterface::GetInstance()->set_kbsync(reinterpret_cast<const char*>(kb_buffer.get()));
					return true;
				}
			}
			return false;
		}

		class ParsedAuthenticateCookie
		{
		public:
			explicit ParsedAuthenticateCookie(const std::string& http_header):cookie_token_(""){
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
			DISALLOW_EVIL_CONSTRUCTORS(ParsedAuthenticateCookie);
		};
		//fix:2015/3/17 add cookie flag "mz_mt0"
		char *ParsedAuthenticateCookie::kCookieName[] = {"mz_mt0","hsaccnt","mzf_in","Pod","itspod","X-Dsid","mz_at0-","mz_at_ssl-","wosid-lite","ns-mzf-inst",NULL};
	}
	class XAppleMDActionMessage
	{
	public:
		XAppleMDActionMessage(uint32_t dsid_low,uint32_t dsid_high):
			x_apple_md_(""),
			x_apple_md_m_(""){
			char* xa_md = nullptr;
			uint32_t xa_md_len = 0;
			char* xa_md_m = nullptr;
			uint32_t xa_md_m_len = 0;
			uint32_t (__cdecl *CalcMD)(uint32_t dsid_low,uint32_t dsid_high,char* x_apple_md_m,uint32_t* xa_md_m_len,char*xa_md,uint32_t* xa_md_len);
			*reinterpret_cast<uint32_t*>(&CalcMD) = (uint32_t)iTunesInternalInterface::Instance()->lpfnGetMD;
			CalcMD(dsid_low,dsid_high,(char*)&xa_md_m,&xa_md_m_len,(char*)&xa_md,&xa_md_len);
			if(xa_md_m_len){
				scoped_array<unsigned char> buffer(new unsigned char[kMaxCertLength]);
				memset(buffer.get(),0,kMaxCertLength);
				EVP_EncodeBlock(buffer.get(),(const unsigned char*)xa_md_m,xa_md_m_len);
				x_apple_md_m_.append((char*)buffer.get());
			}
			if(xa_md_len){
				scoped_array<unsigned char> buffer(new unsigned char[kMaxCertLength]);
				memset(buffer.get(),0,kMaxCertLength);
				EVP_EncodeBlock(buffer.get(),(const unsigned char*)xa_md,xa_md_len);
				x_apple_md_.append((char*)buffer.get());
			}
		}
		~XAppleMDActionMessage(){
			x_apple_md_.resize(0);
			x_apple_md_m_.resize(0);
		}
		std::string X_Apple_MD() const{
			return x_apple_md_;
		}
		std::string X_Apple_MD_M() const{
			return x_apple_md_m_;
		}
	private:
		std::string x_apple_md_;
		std::string x_apple_md_m_;
	};
	communicates* communicates::singleton(){
		static passport::communicates* itunes;
		if(!itunes){
			iTunesNativeInterface::GetInstance()->Init();
			communicates* new_info = new communicates();
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&itunes),new_info,NULL)){
				delete new_info;
			}
		}
		return itunes;
	}

	void communicates::DICallerAllocate(memory::DataInterfaceAllocate* fmgr){
		if(fmgr){
			fmgr->DIAllocate();
		}
	}

	void communicates::DICallerRelease(memory::DataInterfaceAllocate* fmgr){
		if(fmgr){
			fmgr->DIRelease();
		}
	}

	void communicates::ResetSapSetup(bool x_act_sig){
		Initialize();
		SapSetupInitialize(x_act_sig);
	}

	bool communicates::Authenticate(const char* username,const char* password,const char* os_name,const char* os_guid){
		internal::GenerateAuthenticateOsGUID os;
		std::string login_text = internal::GetLoginText(username,password,os.machine_name().c_str(),ABI::base::GetHardwareCookie().c_str());
		if(os_name!=NULL&&os_guid!=NULL){
			login_text = internal::GetLoginText(username,password,os_name,os_guid);
		}
		unsigned char* x_a_act_sig = NULL;
		unsigned long act_sig_len = 0;
		if(iTunesInternalInterface::Instance()->lpfnSapGetASFD==NULL){
			LOG(INFO)<<"itunes not support X-Apple-ActionSignature failed!"<<std::endl;
			return false;
		}
		iTunesInternalInterface::Instance()->lpfnSapGetASFD(internal::kSyncId,ToDword(login_text.c_str()),login_text.length(),ToDword(&x_a_act_sig),ToDword(&act_sig_len));
		if(act_sig_len&&x_a_act_sig){
			scoped_array<unsigned char> act_sig_from_server(new unsigned char[kMaxCertLength]);
			memset(act_sig_from_server.get(),0,kMaxCertLength);
			if(EVP_EncodeBlock(act_sig_from_server.get(),x_a_act_sig,act_sig_len)>0){
				USES_CONVERSION;
				iTunesCookieInterface::GetInstance()->set_login_cookie_flag(true);
				std::string message_header = std::string("X-Apple-ActionSignature: ")+std::string((const char*)act_sig_from_server.get())+std::string("\r\n");
				std::string login_message = internal::SendHTTPS(L"buy.itunes.apple.com",L"/WebObjects/MZFinance.woa/wa/authenticate",
					login_text.c_str(),login_text.length(),internal::apple_authenticate,A2W(message_header.c_str()), NULL, NULL);
				bool success = false;
				if(!iTunesCookieInterface::GetInstance()->login_cookie_flag()){
					iTunesCookieInterface::GetInstance()->set_x_apple_actionsignature(std::string((const char*)act_sig_from_server.get()));
					login_message = ABI::base::Utf8ToGBK(login_message);
					iTunesCookieInterface::GetInstance()->set_x_dsid(internal::GetKeyValue("dsPersonId",login_message));
					iTunesCookieInterface::GetInstance()->set_x_token(internal::GetKeyValue("passwordToken",login_message));
					iTunesCookieInterface::GetInstance()->set_credit_display(internal::GetKeyValue("creditDisplay",login_message));
					success = internal::GetKbsyncToken();
				}
				return success;
			}
			else{
				LOG(INFO)<<"EVP_EncodeBlock"<<__FUNCTION__<<__LINE__<<std::endl;
				return false;
			}
		}
		else{
			LOG(INFO)<<"X-Apple-ActionSignature calc error!"<<std::endl;
			return false;
		}
	}
	bool communicates::SendMessage_pendingSongs(){
		USES_CONVERSION;
		std::string message_header;
		message_header.resize(0);
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		//printf("SendMessage_pendingSongs_Cookie:%s\r\n",pass_token.Cookie().c_str());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		const std::string kb = iTunesCookieInterface::GetInstance()->kbsync();
		internal::GenerateAuthenticateOsGUID os_guid;
		const std::string product_info = internal::GetFormat_pendingSongs(os_guid.machine_guid().c_str(),
			kb.c_str(),
			os_guid.machine_name().c_str());
		std::string buy_result = internal::SendHTTPS(L"buy.itunes.apple.com",
			L"/WebObjects/MZFinance.woa/wa/pendingSongs",
			product_info.c_str(),
			product_info.length(),
			internal::apple_authenticate,
			AUniocde(message_header).c_str(),
			L"https://se.itunes.apple.com/WebObjects/MZStoreElements.woa/wa/purchases");
		printf("%s\r\n",ABI::base::Utf8ToGBK(buy_result).c_str());
		return true;
	}
	bool communicates::SendMessageLookupPurchasesAppIdList(){
		USES_CONVERSION;
		std::string message_header;
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		std::string buy_result = internal::ReadHTTPS(L"se.itunes.apple.com",
			L"/WebObjects/MZStoreElements.woa/wa/purchases?dataOnly=true&mt=8&restoreMode=false",
			A2W(message_header.c_str()),
			internal::apple_authenticate,
			L"https://se.itunes.apple.com/WebObjects/MZStoreElements.woa/wa/purchases");
		return true;
	}
	bool communicates::SendMessageLookupPurchasesAppInfo(const char* app_id){
		if (!app_id)
			return false;
		USES_CONVERSION;
		std::string message_header;
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("Origin: https://se.itunes.apple.com\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		//message_header.append("; xp_ci=3z35T4cZz4UIz4jWzCjHzwVwyAv1t; s_vi=[CS]v1|2AED3D870501340D-60000104000FD106[CE]");
		message_header.append("\r\n");
		std::string server_path = "/WebObjects/MZStorePlatform.woa/wa/lookup?version=1&id=";
		server_path.append(app_id);
		server_path.append("&p=item&caller=DI6&requestParameters=%5Bobject%20Object%5D");
		std::string app_info = internal::ReadHTTPS(L"client-api.itunes.apple.com",
			A2W(server_path.c_str()),
			A2W(message_header.c_str()),
			internal::apple_itunes,
			L"https://se.itunes.apple.com/WebObjects/MZStoreElements.woa/wa/purchases");
		if (app_info.empty()){
			//itunes-store-web-service-search-api 
			//reference:https://www.apple.com/itunes/affiliates/resources/documentation/itunes-store-web-service-search-api.html
			server_path.clear();
			server_path.append("/lookup?id=");
			server_path.append(app_id);
			app_info = internal::ReadHTTPS(L"itunes.apple.com",
				A2W(server_path.c_str()),
				A2W(message_header.c_str()),
				internal::apple_itunes,
				L"https://se.itunes.apple.com/WebObjects/MZStoreElements.woa/wa/purchases");
		}
		return true;
	}
	bool communicates::SendMessage_registerSuccess(){
		std::string message_header;
		message_header.resize(0);
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		const std::string kb = iTunesCookieInterface::GetInstance()->kbsync();
		internal::GenerateAuthenticateOsGUID os_guid;
		const std::string product_info = internal::GetFormat_registerSuccess(os_guid.machine_guid().c_str(),
			os_guid.machine_name().c_str());
		std::string buy_result = internal::SendHTTPS(L"buy.itunes.apple.com",
			L"/WebObjects/MZFinance.woa/wa/registerSuccess",
			product_info.c_str(),
			product_info.length(),
			internal::apple_authenticate,
			AUniocde(message_header).c_str(),
			L"https://itunes.apple.com/WebObjects/MZStore.woa/wa/viewGrouping?cc=cn&id=29099&mt=8");
		return true;
	}
	bool communicates::SendMessage_purchase_login(){
		internal::GenerateAuthenticateOsGUID os_guid;
		std::string message_header;
		message_header.resize(0);
		message_header.append("Client-Cloud-DAAP-Request-Reason: 1\r\n");
		message_header.append("Client-Cloud-Purchase-DAAP-Version: 1.1/iTunes-12.0.1.26\r\n");
		message_header.append("Client-DAAP-Version: 3.12\r\n");
		message_header.append("Client-iTunes-Sharing-Version: 3.11\r\n");
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Guid: ");
		message_header.append(os_guid.machine_guid().c_str());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		const std::string kb = iTunesCookieInterface::GetInstance()->kbsync();
		const std::string product_info = internal::GetFormat_registerSuccess(os_guid.machine_guid().c_str(),
			os_guid.machine_name().c_str());
		std::string buy_result = internal::SendHTTPS(L"pd-st.itunes.apple.com",
			L"/WebObjects/MZPurchaseDaap.woa/purchase/login",
			product_info.c_str(),
			product_info.length(),
			internal::apple_authenticate,
			AUniocde(message_header).c_str(),
			NULL);
		return true;
	}
	bool communicates::SendMessage_finishMachineProvisioning(const char* message){
		internal::GenerateAuthenticateOsGUID os_guid;
		std::string message_header;
		message_header.resize(0);
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		std::string buy_result = internal::SendHTTPS(L"buy.itunes.apple.com",
			L"/WebObjects/MZFinance.woa/wa/finishMachineProvisioning",
			message,
			strlen(message),
			internal::apple_authenticate,
			AUniocde(message_header).c_str(),
			L"http://itunes.apple.com/cn/");
		return true;
	}
	bool communicates::GenWebPassportAuthFile(const char* file,const char* os_name,const char* os_guid){
		Json::Value json_var;
		Json::FastWriter writer;
		if(os_name!=NULL&&os_guid!=NULL){
			json_var["machineName"] = os_name;
			json_var["guid"] = os_guid;
		}
		else{
			internal::GenerateAuthenticateOsGUID os;
			json_var["machineName"] = os.machine_name();
			json_var["guid"] = os.machine_guid();
		}
		json_var["X-Apple-ActionSignature"] = iTunesCookieInterface::GetInstance()->x_apple_actionsignature(); 
		json_var["X-Token"] = iTunesCookieInterface::GetInstance()->x_token(); 
		json_var["X-Dsid"] = iTunesCookieInterface::GetInstance()->x_dsid();
		json_var["X-Apple-Store-Front"] = iTunesCookieInterface::GetInstance()->x_apple_store_front();
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		json_var["Cookie"] = pass_token.Cookie();
		json_var["kbsync"] = iTunesCookieInterface::GetInstance()->kbsync();
		json_var["creditDisplay"] = iTunesCookieInterface::GetInstance()->credit_display();
		const std::string utf_json = ABI::base::UnicodeToUft8(ABI::base::AsciiToUnicode(writer.write(json_var)));
		std::ofstream out(file,std::ofstream::binary|std::ofstream::out);
		if(out.is_open()){
			out.write(utf_json.c_str(),utf_json.length());
			out.flush();
			out.close();
			out.clear();
			return true;
		}
		else{
			return false;
		}
	}
	bool communicates::SendAuthorizeMachineProtocol(){
// 		std::string ssss = internal::GetAuthorizeMachine_kbsync();
// 		if(ssss.length()){
// 		}
		return true;
	}
	bool communicates::SendMessage_buyProduct(const char* product_id,iTunesDownloadInfo* download_info,const int try_count,bool expense){
		std::string message_header;
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		const std::string user_dsid = iTunesCookieInterface::GetInstance()->x_dsid();
		if (user_dsid.length()){
			message_header.append("X-Dsid: ");
			message_header.append(user_dsid);
			message_header.append("\r\n");
			//date:2015/04/21 add XAppleMDActionMessage support
			const uint64 dsid = static_cast<uint64>(atof(user_dsid.c_str()));
			XAppleMDActionMessage x_applemd_action(LODWORD(dsid), HIDWORD(dsid));
			const std::string x_apple_md = x_applemd_action.X_Apple_MD();
			const std::string x_apple_md_m = x_applemd_action.X_Apple_MD_M();
			if (x_apple_md.length() && x_apple_md_m.length()){
				message_header.append("X-Apple-MD: ");
				message_header.append(x_apple_md);
				message_header.append("\r\n");
				message_header.append("X-Apple-MD-M: ");
				message_header.append(x_apple_md_m);
				message_header.append("\r\n");
			}
		}
		message_header.append("X-Apple-Tz: 28800\r\n");
		const std::string apple_store_front = iTunesCookieInterface::GetInstance()->x_apple_store_front();
		if (apple_store_front.length()){
			message_header.append("X-Apple-Store-Front: ");
			message_header.append(apple_store_front);
			message_header.append("\r\n");
		}
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		iTunesCookieInterface::GetInstance()->set_buy_product_state(iTunesCookieInterface::FIRST_BUY_BEGIN);
		const std::string kb_sync = iTunesCookieInterface::GetInstance()->kbsync();
		std::string buy_info;
		if (kb_sync.length()){
			internal::GenerateAuthenticateOsGUID os_guid;
			const std::string product_info = internal::GetProductFormatInfo(kb_sync,
				product_id,
				iTunesCookieInterface::GetInstance()->credit_display(),
				os_guid.machine_guid().c_str(),
				os_guid.machine_name().c_str(),
				expense);
			buy_info = internal::SendHTTPS(L"buy.itunes.apple.com",
				L"/WebObjects/MZBuy.woa/wa/buyProduct",
				product_info.c_str(),
				product_info.length(),
				internal::apple_authenticate,
				AUniocde(message_header).c_str(),
				L"https://itunes.apple.com/cn/");
		}
		if (buy_info.find("X-Apple-MD-Action_message") != std::string::npos || buy_info.find("trigger-download") != std::string::npos){
			if(iTunesCookieInterface::GetInstance()->buy_product_state()!=iTunesCookieInterface::FIRST_BUY_END){
				return false;
			}
			passport::communicates::singleton()->SendMessage_pendingSongs();
			return true;
		}
		else{
			if(try_count==1){
				SendMessage_buyProduct(product_id,download_info,try_count+1,true);
			}
			buy_info = ABI::base::Utf8ToGBK(buy_info);
			std::string value = internal::GetKeyValue("URL", buy_info);
			if(value.length()>=1){
				download_info->set_download_url(value.c_str(),value.length());
				value = internal::GetKeyValue("downloadKey", buy_info);
				download_info->set_download_key(value.c_str(),value.length());
				value = internal::GetKeyValue("download-id", buy_info);
				download_info->set_download_id(value.c_str(),value.length());
			}
			return (download_info->download_key()!=NULL&&download_info->download_url()!=NULL);
		}
	}

	bool communicates::SongDownloadDone(const char* product_id,iTunesDownloadInfo* download_info){
		USES_CONVERSION;
		std::string message_header;
		message_header.append("X-Token: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_token());
		message_header.append("\r\n");
		message_header.append("X-Dsid: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_dsid());
		message_header.append("\r\n");
		message_header.append("X-Apple-Tz: 28800\r\n");
		message_header.append("X-Apple-Store-Front: ");
		message_header.append(iTunesCookieInterface::GetInstance()->x_apple_store_front());
		message_header.append("\r\n");
		message_header.append("Cookie: ");
		internal::ParsedAuthenticateCookie pass_token(iTunesCookieInterface::GetInstance()->auth_response_header());
		message_header.append(pass_token.Cookie());
		message_header.append("\r\n");
		const std::wstring path = std::wstring(L"/WebObjects/MZFastFinance.woa/wa/songDownloadDone?")+
			std::wstring(L"songId=")+std::wstring(A2W(product_id))+
			std::wstring(L"&guid="+std::wstring(A2W(ABI::base::GetHardwareCookie().c_str())))+
			std::wstring(L"&download-id=")+std::wstring(A2W(download_info->download_id()));
		std::string buy_result = internal::ReadHTTPS(L"buy.itunes.apple.com",path.c_str(),A2W(message_header.c_str()),internal::apple_authenticate,L"http://itunes.apple.com/cn/");
		return (buy_result.length()>=1);
	}

	void communicates::Initialize(){
		internal::KBSyncMachineIdInitialize();
		unsigned long* ssss = &internal::kMachineIdA[0];
		LOG_IF(ERROR,iTunesInternalInterface::Instance()->lpfnSapInit())<<"Instance()->lpfnKbsyncID"<<std::endl;
		LOG_IF(ERROR,iTunesInternalInterface::Instance()->lpfnSapGetP1(
			ToDword(&internal::kSyncId),ToDword(&internal::kMachineIdA[0])))<<"Instance()->lpfnKbsyncID"<<std::endl;//fix:2015.04.01
	}

	void communicates::SapSetupInitialize(bool x_act_sig_flag){
		scoped_array<unsigned char> sap_setup(new unsigned char[kMaxCertLength]);
		unsigned long sign_length = 0;
		for(int i=0;i<3;i++){
			std::string signSapSetupCert = internal::ReadHTTPS(L"init.itunes.apple.com", L"/WebObjects/MZInit.woa/wa/signSapSetupCert", NULL, internal::apple_itunes, NULL, NULL);
			if(signSapSetupCert.length()){
				sap_setup.reset(new unsigned char[kMaxCertLength]);
				memset(sap_setup.get(),0,kMaxCertLength);
				signSapSetupCert = signSapSetupCert.substr(std::string("<plist>\n<dict>\n<key>sign-sap-setup-cert</key>\n<data>").length());
				signSapSetupCert = signSapSetupCert.substr(0,signSapSetupCert.length()-std::string("</data>\n</dict>\n</plist>\n").length());
				sign_length = EVP_DecodeBlock(sap_setup.get(),(const unsigned char*)signSapSetupCert.c_str(),signSapSetupCert.size());
				if(sign_length!=-1){
					break;
				}
			}
			Sleep(1000);
		}
		for(;;){
			unsigned long server_state = 0x601;
			unsigned char* cert_info = NULL;
			unsigned long cert_info_length = 0;
			if(iTunesInternalInterface::Instance()->lpfnSapCalcBuffer(x_act_sig_flag?200:210,//fix:2015.04.01 fix:2015.04.01//login appleid x_aa_sig?200:210,register x_aa_sig?210:200
				ToDword(&internal::kMachineIdA[0]),//fix:2015.04.01
				internal::kSyncId,
				ToDword(sap_setup.get()),
				sign_length,
				ToDword(&cert_info),
				ToDword(&cert_info_length),
				ToDword(&server_state))){
					break;
			}
			if(server_state==0x600){
				break;
			}
			scoped_array<unsigned char> cert_buffer(new unsigned char[kMaxCertLength]);
			memset(cert_buffer.get(),0,kMaxCertLength);
			unsigned long sign_sap_setup_length = EVP_EncodeBlock(cert_buffer.get(),cert_info,cert_info_length);
			if(sign_sap_setup_length==-1){
				LOG(INFO)<<"SapCalcBuffer calc failed!";
				return;
			}
			std::string message = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
				"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
				"<plist version=\"1.0\"><dict><key>sign-sap-setup-buffer</key><data>";
			message.append((const char*)cert_buffer.get());
			message.append("</data></dict></plist>");
			std::string sign_sap_setup_buffer;
			for(int i=0;i<3;i++){
				sign_sap_setup_buffer = internal::SendHTTPS(L"buy.itunes.apple.com",L"/WebObjects/MZPlay.woa/wa/signSapSetup",
					message.c_str(),message.length(),internal::apple_itunes,NULL, NULL, NULL);
				if(!sign_sap_setup_buffer.length()){
					Sleep(1000);
					continue;
				}
				sign_sap_setup_buffer = sign_sap_setup_buffer.substr(std::string("<plist>\n<dict>\n<key>sign-sap-setup-buffer</key>\n<data>").length());
				sign_sap_setup_buffer = sign_sap_setup_buffer.substr(0,sign_sap_setup_buffer.length()-std::string("</data>\n</dict>\n</plist>\n").length());
				memset(sap_setup.get(),0,kMaxCertLength);
				sign_length = EVP_DecodeBlock(sap_setup.get(),(const unsigned char*)sign_sap_setup_buffer.c_str(),sign_sap_setup_buffer.size());
				if(sign_length==-1){
					Sleep(1000);
					continue;
				}
				break;
			}
		}
	}

	communicates::communicates(void){

	}

	communicates::~communicates(void){

	}
	tdk_callback::tdk_callback(){

	}
	tdk_callback::~tdk_callback(){

	}
	void tdk_callback::Initialize(){
		iTunesNativeInterface::GetInstance()->Init();
		internal::KBSyncMachineIdInitialize();
		LOG_IF(ERROR,iTunesInternalInterface::Instance()->lpfnSapInit())<<"Instance()->lpfnKbsyncID";
		LOG_IF(ERROR,iTunesInternalInterface::Instance()->lpfnSapGetP1(
			ToDword(&internal::kSyncId),ToDword(&internal::kMachineIdA[0])))<<"Instance()->lpfnKbsyncID";//fix:2015.04.01
	}
	bool tdk_callback::SapSetupInitialize(const int x_aa_sig,const char* sign_cert,char* buffer,size_t length){
		scoped_array<unsigned char> sap_setup(new unsigned char[kMaxCertLength]);
		memset(sap_setup.get(),0,kMaxCertLength);
		unsigned long sap_length = EVP_DecodeBlock(sap_setup.get(),(const unsigned char*)sign_cert,strlen(sign_cert));
		if(sap_length==-1){
			LOG(INFO)<<"SapSetupInitialize failed!";
			return false;
		}
		unsigned long server_state = 0x601;
		unsigned char* cert_info = NULL;
		unsigned long cert_info_length = 0;
		if(iTunesInternalInterface::Instance()->lpfnSapCalcBuffer(x_aa_sig?210:200,//fix:2015.04.01//login appleid x_aa_sig?200:210,register x_aa_sig?210:200
			ToDword(&internal::kMachineIdA[0]),//fix:2015.04.01
			internal::kSyncId,
			ToDword(sap_setup.get()),
			sap_length,
			ToDword(&cert_info),
			ToDword(&cert_info_length),
			ToDword(&server_state))){
			LOG(INFO)<<"lpfnSapCalcBuffer calc failed!";
			return false;
		}
		if(server_state==0x600){
			LOG(INFO)<<"lpfnSapCalcBuffer OK";
			return true;
		}
		scoped_array<unsigned char> x_aa_sig_en(new unsigned char[kMaxCertLength]);
		int x_aa_sig_length = EVP_EncodeBlock(x_aa_sig_en.get(),cert_info,cert_info_length);
		if(cert_info_length==-1||x_aa_sig_length==-1){
			LOG(INFO)<<"SapCalcBuffer calc failed!";
			return false;
		}
		strncpy(buffer,(const char*)x_aa_sig_en.get(),x_aa_sig_length);
		return true;
	}
	bool tdk_callback::CalcXAppleActionSignature(char* buffer,size_t length){
		unsigned char* x_a_act_sig = nullptr;
		unsigned long act_sig_len = 0;
		const unsigned long kbsync_id = passport::internal::GetKbSyncId();
		passport::iTunesInternalInterface::Instance()->lpfnSapGetAS(kbsync_id,100,0,0,passport::ToDword(&x_a_act_sig),passport::ToDword(&act_sig_len));
		scoped_array<unsigned char> x_aa_sig(new unsigned char[kMaxCertLength]);
		if(x_a_act_sig&&act_sig_len){
			memset(x_aa_sig.get(),0,kMaxCertLength);
			int x_aa_sig_length = EVP_EncodeBlock(x_aa_sig.get(),x_a_act_sig,act_sig_len);
			if(x_aa_sig_length>0){
				strncpy(buffer,(const char*)x_aa_sig.get(),x_aa_sig_length);
			}
			return (x_aa_sig_length!=-1);
		}
		return false;
	}
	bool tdk_callback::CalcXAppleActionSignature(const char* x_aa_sig,const size_t length){
		if(x_aa_sig!=NULL&&length){
			const unsigned long kbsync_id = passport::internal::GetKbSyncId();
			scoped_array<unsigned char> calc_x_aa_sig(new unsigned char[kMaxCertLength]);
			memset(calc_x_aa_sig.get(),0,kMaxCertLength);
			size_t decode_len = EVP_DecodeBlock(calc_x_aa_sig.get(),(const unsigned char*)x_aa_sig,length);
			passport::iTunesInternalInterface::Instance()->lpfnSapGetASFD_a(kbsync_id,passport::ToDword(calc_x_aa_sig.get()),decode_len,0,0);
			return (decode_len!=-1);
		}
		return false;
	}
	bool tdk_callback::CalcXAppleActionSignature(const char* x_aa_sig,const size_t x_aa_sig_length,char* buffer,size_t length){
		if(x_aa_sig!=NULL&&x_aa_sig_length){
			unsigned char* x_a_act_sig = nullptr;
			unsigned long act_sig_len = 0;
			const unsigned long kbsync_id = passport::internal::GetKbSyncId();
			passport::iTunesInternalInterface::Instance()->lpfnSapGetASFD(kbsync_id,passport::ToDword(x_aa_sig),
				x_aa_sig_length,passport::ToDword(&x_a_act_sig),passport::ToDword(&act_sig_len));
			scoped_array<unsigned char> x_aa_sig_en(new unsigned char[kMaxCertLength]);
			if(x_a_act_sig!=NULL&&act_sig_len){
				memset(x_aa_sig_en.get(),0,kMaxCertLength);
				int encode_length = EVP_EncodeBlock(x_aa_sig_en.get(),x_a_act_sig,act_sig_len);
				if(encode_length>0){
					strncpy(buffer,(const char*)x_aa_sig_en.get(),encode_length!=-1?encode_length:0);
				}
				return (encode_length!=-1);
			}
			return false;
		}
		return false;
	}
}
