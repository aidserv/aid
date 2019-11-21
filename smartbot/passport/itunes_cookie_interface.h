#ifndef PASSPORT_ITUNES_COOKIE_INTERFACE_H_
#define PASSPORT_ITUNES_COOKIE_INTERFACE_H_
//////////////////////////////////////////////////////////////////////////
#include "passport/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace passport{
	class iTunesCookieInterface
	{
	public:
		enum BuyProductStateTable{
			INIT,
			FIRST_BUY_BEGIN,
			FIRST_BUY_END
		};
		static iTunesCookieInterface* GetInstance();
		void set_login_cookie_flag(bool flag);
		bool login_cookie_flag() const;
		void set_signup_wizard_cookie_flag(bool flag);
		bool signup_wizard_cookie_flag() const;
		void set_x_apple_actionsignature(const std::string& text);
		std::string x_apple_actionsignature() const;
		void set_x_apple_store_front(const std::string& text);
		std::string x_apple_store_front() const;
		void set_x_dsid(const std::string& dsid);
		std::string x_dsid() const;
		void set_x_token(const std::string& token);
		std::string x_token() const;
		void set_credit_display(const std::string& credit);
		std::string credit_display() const;
		void set_kbsync(const std::string& kbsync_str);
		std::string kbsync() const;
		void set_x_apple_md_m(const std::string& k);
		std::string x_apple_md_m() const;
		void set_x_apple_md(const std::string& k);
		std::string x_apple_md() const;
		void set_auth_response_header(const std::string& k);
		std::string auth_response_header() const;
		void set_buy_product_state(BuyProductStateTable state);
		BuyProductStateTable buy_product_state() const;
		inline void set_x_apple_md_data(const std::string& buy_k){
			x_apple_md_data_ = buy_k;
		}
		inline std::string x_apple_md_data() const{
			return x_apple_md_data_;
		}
	private:
		iTunesCookieInterface(void);
		~iTunesCookieInterface(void);
		DISALLOW_EVIL_CONSTRUCTORS(iTunesCookieInterface);
		bool login_cookie_flag_;
		bool signup_wizard_cookie_flag_;
		BuyProductStateTable buy_product_state_;
		std::string x_apple_actionsignature_;
		std::string x_apple_store_front_;
		std::string x_dsid_;
		std::string x_token_;
		std::string credit_display_;
		std::string kbsync_;
		std::string x_apple_md_m_;
		std::string x_apple_md_;
		std::string auth_response_header_;
		std::string x_apple_md_data_;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif
