#include "base/string/string_case.h"

namespace ABI{
	namespace base{
		std::wstring ToUpper(const std::wstring& seque){
			if(!seque.size()){
				return L"";
			}
			std::wstring var_seque = seque;
			std::transform(var_seque.begin(),var_seque.end(),var_seque.begin(),::toupper);
			return var_seque;
		}
		std::wstring ToLower(const std::wstring& seque){
			if(!seque.size()){
				return L"";
			}
			std::wstring var_seque = seque;
			std::transform(var_seque.begin(),var_seque.end(),var_seque.begin(),::tolower);
			return var_seque;
		}
		std::string ToUpper(const std::string& seque){
			if(!seque.size()){
				return "";
			}
			std::string var_seque = seque;
			std::transform(var_seque.begin(),var_seque.end(),var_seque.begin(),::toupper);
			return var_seque;
		}
		std::string ToLower(const std::string& seque){
			if(!seque.size()){
				return "";
			}
			std::string var_seque = seque;
			std::transform(var_seque.begin(),var_seque.end(),var_seque.begin(),::tolower);
			return var_seque;
		}
	}
}