#ifndef BASE_STRING_STRING_CONV_H_
#define BASE_STRING_STRING_CONV_H_
//////////////////////////////////////////////////////////////////////////
#include "base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		std::wstring AsciiToUnicode(const std::string &str);
		std::string UnicodeToAscii(const std::wstring &wstr);
		std::string UnicodeToUft8(const std::wstring& str);
		std::wstring Utf8ToUnicode(const std::string &str);
		std::string GBKToUtf8(const std::string &str);
		std::string Utf8ToGBK(const std::string &str);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif