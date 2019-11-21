#ifndef ABI_BASE_STRING_URLS_CONV_H_
#define ABI_BASE_STRING_URLS_CONV_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		inline unsigned char to_hex( unsigned char x );
		const std::string UrlEncode(const std::string& s);
		inline unsigned char from_hex(unsigned char ch);
		const std::string UrlDecode(const std::string& str);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif