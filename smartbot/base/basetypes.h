#ifndef ABI_BASE_BASETYPES_H_
#define ABI_BASE_BASETYPES_H_
//////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
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
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ssleay32.lib")
#pragma comment(lib,"Winhttp.lib")
#endif
#include <memory>
#include <cassert>
#include <cctype>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
struct Int64Make
{
	unsigned long low;
	unsigned long high;
};
//////////////////////////////////////////////////////////////////////////
#endif