#include "base/string/string_conv.h"
#include "thirdparty/glog/scoped_ptr.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		std::wstring AsciiToUnicode(const std::string &str){
			if(!str.size()){
				return L"";
			}
			int size_needed = MultiByteToWideChar(CP_ACP,0,&str[0],(int)str.size(),NULL,0);
			std::wstring wstrTo(size_needed,0);
			MultiByteToWideChar(CP_ACP,0,&str[0],-1,&wstrTo[0],size_needed);
			return wstrTo;
		}
		std::string UnicodeToAscii(const std::wstring &wstr){
			if(!wstr.size()){
				return "";
			}
			int size_needed = WideCharToMultiByte(CP_ACP,0,&wstr[0],(int)wstr.size(),NULL,0,NULL,NULL);
			std::string strTo( size_needed, 0 );
			WideCharToMultiByte(CP_ACP,0,&wstr[0],-1,&strTo[0],size_needed,NULL,NULL);
			return strTo;
		}
		std::string UnicodeToUft8(const std::wstring& str){
			if(!str.size()){
				return "";
			}
			int n = WideCharToMultiByte(CP_UTF8,0,str.c_str(),-1,NULL,0,NULL,NULL);
			scoped_array<char> buf_1(new char[str.length()*4]);
			memset(buf_1.get(),0,str.length()*4);
			WideCharToMultiByte(CP_UTF8,0,str.c_str(),-1,buf_1.get(),n,NULL,NULL);  
			std::string strOutUTF8(buf_1.get());
			return strOutUTF8;
		}
		std::wstring Utf8ToUnicode(const std::string &str){
			int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL,str.c_str(),str.length(),NULL,0);
			wchar_t* wszString = new wchar_t[wcsLen + 1];
			::MultiByteToWideChar(CP_UTF8, NULL, str.c_str(), str.length(), wszString, wcsLen);
			wszString[wcsLen] = '\0';
			std::wstring unicodeText(wszString); 
			delete[] wszString;
			return unicodeText;
		}
		std::string GBKToUtf8(const std::string &str){
			if(!str.size()){
				return "";
			}
			int len_wchart = MultiByteToWideChar(CP_ACP,0,str.c_str(),-1,NULL,0);
			wchar_t * unicode= new wchar_t[len_wchart+10];
			if(!unicode){
				return "";
			}
			MultiByteToWideChar(CP_ACP,0,str.c_str(),-1,unicode,len_wchart);
			int len_utf8 = WideCharToMultiByte(CP_UTF8,0,unicode,-1,NULL,0,NULL,NULL);
			char* utf8str = new char[len_utf8+10];
			WideCharToMultiByte(CP_UTF8,0,unicode,-1,utf8str,len_utf8,NULL,NULL);
			std::string utf8_data(utf8str);
			delete[] utf8str;
			delete[] unicode;
			return utf8_data;
		}
		std::string Utf8ToGBK(const std::string &str){
			int len_wchart = MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,NULL,0);   
			wchar_t * unicode_2 = new wchar_t[len_wchart+10];
			if(!unicode_2){
				return "";
			}
			MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,unicode_2,len_wchart);
			int len_gbk = WideCharToMultiByte(CP_ACP,0,unicode_2,-1,NULL,0,NULL,NULL);  
			char * gbkstr = new char[len_gbk+10];
			WideCharToMultiByte(CP_ACP,0,unicode_2,-1,gbkstr,len_gbk,NULL,NULL);
			std::string gbk_data(gbkstr);
			delete[] gbkstr;
			delete[] unicode_2;
			return gbk_data;
		}
	}
}