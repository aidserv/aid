#ifndef PASSPORT_BASICTYPES_H_
#define PASSPORT_BASICTYPES_H_
//////////////////////////////////////////////////////////////////////////
#define _ATL_NOFORCE_MANIFEST
#define _STL_NOFORCE_MANIFEST
#define _CRT_NOFORCE_MANIFEST
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_ALL_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <Dbt.h>
#include <IPHlpApi.h>
#include <ShellAPI.h>
#include <WinIoCtl.h>
#include <IPTypes.h>
#include <atlcomcli.h>
#include <atlbase.h>
#include <atlwin.h>
#pragma comment(lib,"IPHLPAPI.lib")
#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"Ole32.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"Advapi32.lib")
#pragma comment(lib,"WinMM.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winhttp.lib")
#pragma comment(lib,"comctl32.lib")
#pragma warning(disable:4702)
#pragma warning(disable:4201)
#include <atlbase.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <cassert>
#include <algorithm>
#include <iterator>
#include <set>
using namespace std;
#pragma warning(default:4201)
#pragma warning(disable:4200)
#if defined(_WINDLL)||defined(_USRDLL)
#define WIN_DLL_API __declspec(dllexport)
#else
#define WIN_DLL_API __declspec(dllimport)
#endif
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef unsigned long	ulong;
typedef long long	llong;
typedef unsigned long long	ullong;
#if !defined(uint64)
typedef unsigned __int64 uint64;
#endif
#if !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#define _X86_
#endif
#if !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif
#if defined(_X86_)
	typedef uint32 uint;
#elif (defined(_AMD64_))
	typedef uint64 uint;
#endif

#ifndef HIDWORD
#define	HIDWORD(a) ((DWORD)((UINT64)(a) >> 32))
#define	LODWORD(a) ((DWORD)((UINT64)(a) & 0xffffffff))
#endif
	struct MakeLongLong{
		unsigned long high;
		unsigned long low;
	};
	static std::wstring AUniocde(const std::string& str){
		USES_CONVERSION;
		std::wstring dst = A2W(str.c_str());
		return dst;
	}
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
	TypeName(const TypeName&);                    \
	void operator=(const TypeName&)
const unsigned long kMaxStackBufferLength = 1024;
const unsigned long kMaxBufferLength = 4096;
static const char kHTTPUserAgent[] = "iTunes/12.2.2 (Windows; Microsoft Windows 8 x64 Business Edition (Build 9200); x64) AppleWebKit/7600.5017.0.22";
//////////////////////////////////////////////////////////////////////////
#endif

