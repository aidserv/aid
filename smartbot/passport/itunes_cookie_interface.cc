#include "passport/itunes_cookie_interface.h"

namespace passport{
	iTunesCookieInterface* iTunesCookieInterface::GetInstance(){
		static iTunesCookieInterface* info;
		if(!info){
			iTunesCookieInterface* new_info = new iTunesCookieInterface();
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
				delete new_info;
			}
		}
		return info;
	}
	void iTunesCookieInterface::set_login_cookie_flag(bool flag){
		login_cookie_flag_ = flag;
	}
	bool iTunesCookieInterface::login_cookie_flag() const{
		return login_cookie_flag_;
	}
	void iTunesCookieInterface::set_signup_wizard_cookie_flag(bool flag){
		signup_wizard_cookie_flag_ = flag;
	}
	bool iTunesCookieInterface::signup_wizard_cookie_flag() const{
		return signup_wizard_cookie_flag_;
	}
	void iTunesCookieInterface::set_x_apple_actionsignature(const std::string& text){
		x_apple_actionsignature_ = text;
	}
	std::string iTunesCookieInterface::x_apple_actionsignature() const{
		return x_apple_actionsignature_;
	}
	void iTunesCookieInterface::set_x_apple_store_front(const std::string& text){
		x_apple_store_front_ = text;
	}
	std::string iTunesCookieInterface::x_apple_store_front() const{
		return x_apple_store_front_;
	}
	void iTunesCookieInterface::set_x_dsid(const std::string& dsid){
		x_dsid_ = dsid;
	}
	std::string iTunesCookieInterface::x_dsid() const{
		return x_dsid_;
	}
	void iTunesCookieInterface::set_x_token(const std::string& token){
		x_token_ = token;
	}
	std::string iTunesCookieInterface::x_token() const{
		return x_token_;
	}
	void iTunesCookieInterface::set_credit_display(const std::string& credit){
		credit_display_ = credit;
	}
	std::string iTunesCookieInterface::credit_display() const{
		return credit_display_;
	}
	void iTunesCookieInterface::set_kbsync(const std::string& kbsync_str){
		kbsync_ = kbsync_str;
	}
	std::string iTunesCookieInterface::kbsync() const{
		return kbsync_;
	}
	void iTunesCookieInterface::set_x_apple_md_m(const std::string& k){
		x_apple_md_m_ = k;
	}
	std::string iTunesCookieInterface::x_apple_md_m() const{
		return x_apple_md_m_;
	}
	void iTunesCookieInterface::set_x_apple_md(const std::string& k){
		x_apple_md_ = k;
	}
	std::string iTunesCookieInterface::x_apple_md() const{
		return x_apple_md_;
	}
	void iTunesCookieInterface::set_auth_response_header(const std::string& k){
		auth_response_header_ = k;
	}
	std::string iTunesCookieInterface::auth_response_header() const{
		return auth_response_header_;
	}
	void iTunesCookieInterface::set_buy_product_state(BuyProductStateTable state){
		buy_product_state_ = state;
	}
	iTunesCookieInterface::BuyProductStateTable iTunesCookieInterface::buy_product_state() const{
		return buy_product_state_;
	}
	iTunesCookieInterface::iTunesCookieInterface(void){
		set_login_cookie_flag(false);
		set_signup_wizard_cookie_flag(false);
		set_x_apple_actionsignature("");
		set_x_apple_store_front("");
		set_x_dsid("");
		set_x_token("");
		set_credit_display("");
		set_kbsync("");
		set_x_apple_md_m("");
		set_x_apple_md("");
		set_auth_response_header("");
		set_buy_product_state(iTunesCookieInterface::INIT);
		set_x_apple_md_data("");
	}
	iTunesCookieInterface::~iTunesCookieInterface(void){
		set_login_cookie_flag(false);
		set_signup_wizard_cookie_flag(false);
		set_x_apple_actionsignature("");
		set_x_apple_store_front("");
		set_x_dsid("");
		set_x_token("");
		set_credit_display("");
		set_kbsync("");
		set_x_apple_md_m("");
		set_x_apple_md("");
		set_auth_response_header("");
		set_buy_product_state(iTunesCookieInterface::INIT);
		set_x_apple_md_data("");
	}
}
