#ifndef ACCOUNT_REGISTER_NEW_H_
#define ACCOUNT_REGISTER_NEW_H_
//////////////////////////////////////////////////////////////////////////
#include "smartbot/passport/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace smartbot{
	namespace account{
		class RegisterNew
		{
		public:
			WIN_DLL_API RegisterNew();
			WIN_DLL_API ~RegisterNew();
			WIN_DLL_API bool signupWizard();
			WIN_DLL_API bool signedApplePublicLicense();
			WIN_DLL_API bool paddingRegisteredInformation(const wchar_t* response_save);
			WIN_DLL_API bool paddingPurchaseInformation();
			std::string GetRequestHeader();
			std::wstring reg_bank_url();
		private:
			bool signupWizardFlushOK(const std::wstring url);
			std::string CalcXAppleActionSignature();
			void CalcXAppleActionSignature(const std::string& httpheader);
			std::string CalcXAppleActionSignature(const void* src,size_t length);
			void Free();
			std::string signup_wizard_response_;
			std::wstring agree_url_;
			std::wstring mz_page_uuid_;
			std::wstring agree_value_;
			std::wstring referer_url_;
			std::wstring reg_info_url_;
			std::wstring reg_referer_;
			std::wstring reg_page_uuid_;
			std::map<std::wstring,std::wstring> reg_info_;
			std::wstring reg_msg_body_;
			std::wstring reg_bank_url_;
			std::wstring reg_bank_uuid_;
			std::wstring reg_bank_body_;
			DISALLOW_EVIL_CONSTRUCTORS(RegisterNew);
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif