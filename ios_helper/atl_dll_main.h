#ifndef IOS_HELPER_ATL_DLL_MAIN_H_
#define IOS_HELPER_ATL_DLL_MAIN_H_
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
#include <Psapi.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <Dbt.h>
#include <IPHlpApi.h>
#include <ShellAPI.h>
#include <WinIoCtl.h>
#include <IPTypes.h>
#pragma comment(lib,"IPHLPAPI.lib")
#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"Ole32.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"Advapi32.lib")
#pragma comment(lib,"WinMM.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"Winhttp.lib")
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
#if defined(_WIN32)
typedef uint32 uint;
#elif (defined(_WIN64))
typedef uint64 uint;
#endif
//////////////////////////////////////////////////////////////////////////
#endif

