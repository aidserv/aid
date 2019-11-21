#ifndef BASE_STRING_STRING_SPLIT_H_
#define BASE_STRING_STRING_SPLIT_H_
//////////////////////////////////////////////////////////////////////////
#include "base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		std::vector<std::wstring> SplitMakePair(const std::wstring & str,const std::wstring& s1,const std::wstring& s2);
		std::wstring SplitDetail(const std::wstring& str,const std::wstring& detail);
		std::vector<std::wstring> SplitArray(const std::wstring & str,const std::wstring & delimiters);
		std::string HexStrFormByteArray(unsigned char *data, int len);
		std::string Md5(const void* str,size_t length,size_t block_length=0);
		std::string StringReplace(std::string& str, const std::string& from, const std::string& to);
		size_t EVPLength(const std::string& str);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif