#include "ABI/itunes_reverse/itunes_abi.h"
#include <algorithm>
#include "ABI/base/string/string_split.h"
#include "ABI/ios_authorize/itunes_module.h"

namespace ABI{
	namespace ReverseEngineering{
		//08D05A20-089B0000-55 8B EC 81 EC 28 02 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 8B F2 8A D9 85 F6
		//089DD900-089B0000-55 89 E5 83 E4 F0 53 57 56 81 EC B4 01 00 00 0F 31 89 C1 8D 74 24 30 89 34 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 BA AB AA AA AA
		//08D04900-089B0000-55 8B EC 83 EC 5C A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 57 8B F9 85 FF 75 13 32 C0
		//089C0630-089B0000-55 89 E5 83 E4 F0 53 57 56 81 EC A4 01 00 00 8D 74 24 38 89 34 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 95 0A FF FF A1 ?? ?? ?? ?? 89 84 24 20 01 00 00
		//089B7D80-089B0000-55 89 E5 83 E4 F8 56 81 EC 84 00 00 00 A1 ?? ?? ?? ?? 89 44 24 08 8B 45 08
		//089C86D0-089B0000-55 89 E5 83 EC 10 8D 45 F4 8B 0D ?? ?? ?? ?? 29 C1
		//089B8CD0-089B0000-55 89 E5 83 E4 F0 53 57 56 81 EC B4 01 00 00 8D 44 24 78 89 04 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 F5 83 FF FF A1 ?? ?? ?? ?? 89 84 24 F8 00 00 00
		//089EA350-089B0000-55 89 E5 83 E4 F0 53 57 56 81 EC 04 02 00 00 0F 31 89 C6 8D 5C 24 38 89 1C 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 71 6D FC FF A1 ?? ?? ?? ?? 89 84 24 D8 01 00 00
		unsigned long GetModuleLength(wchar_t *name){
			unsigned char *dll_base = reinterpret_cast<unsigned char*>(GetModuleHandle(name));
			IMAGE_DOS_HEADER *Dll_Header = (IMAGE_DOS_HEADER *)reinterpret_cast<DWORD>(dll_base);
			IMAGE_NT_HEADERS *DLL_Headers = (IMAGE_NT_HEADERS*)(reinterpret_cast<DWORD>(dll_base)+Dll_Header->e_lfanew);
			unsigned long dll_length = DLL_Headers->OptionalHeader.SizeOfCode;
			if(!dll_length){
				dll_length = DLL_Headers->OptionalHeader.SizeOfImage;
			}
			return dll_length;
		}
		std::string AddressCodeOptimize(const char* code){
			std::string signature = code;
			return ABI::base::StringReplace(signature,"00","??");
		}
		unsigned long GetAddress(wchar_t *Abnormal,wchar_t *name,const std::string& sign){
			std::string signature = AddressCodeOptimize(sign.c_str());
			signature.erase(std::remove(signature.begin(),signature.end(),' '),signature.end());
			unsigned char *dll_base = reinterpret_cast<unsigned char*>(GetModuleHandle(name));
			unsigned long dll_length = GetModuleLength(name);
			for(unsigned int i=0;i<dll_length;i++){
				unsigned char* begin_sig = &dll_base[i];
				for(unsigned int j=0,index=0;j<signature.length();j+=2,index++){
					unsigned long sig_hex = 0;
					sscanf(&signature[j],"%02x",&sig_hex);
					if(signature[j]!='?'&&sig_hex!=begin_sig[index]){
						begin_sig = NULL;
						break;
					}
				}
				if(begin_sig!=NULL){
					return reinterpret_cast<unsigned long>(&dll_base[i]);
				}
			}
			return 0;
		}
		// 			const std::string abi_sub_8D05A20 = "55 8B EC 81 EC 28 02 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 8B F2 8A D9 85 F6";
		// 			const std::string func_name2 = "55 89 E5 83 E4 F0 53 57 56 81 EC B4 01 00 00 0F 31 89 C1 8D 74 24 30 89 34 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 BA AB AA AA AA";
		// 			const std::string func_name3 = "55 8B EC 83 EC 5C A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 57 8B F9 85 FF 75 13 32 C0";
		// 			const std::string func_name4 = "55 89 E5 83 E4 F0 53 57 56 81 EC A4 01 00 00 8D 74 24 38 89 34 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 95 0A FF FF A1 ?? ?? ?? ?? 89 84 24 20 01 00 00";
		// 			const std::string func_name5 = "55 89 E5 83 E4 F8 56 81 EC 84 00 00 00 A1 ?? ?? ?? ?? 89 44 24 08 8B 45 08";
		// 			const std::string func_name6 = "55 89 E5 83 EC 10 8D 45 F4 8B 0D ?? ?? ?? ?? 29 C1";
		// 			const std::string func_name7 = "55 89 E5 83 E4 F0 53 57 56 81 EC B4 01 00 00 8D 44 24 78 89 04 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 F5 83 FF FF A1 ?? ?? ?? ?? 89 84 24 F8 00 00 00";
		// 			const std::string func_name8 = "55 89 E5 83 E4 F0 53 57 56 81 EC 04 02 00 00 0F 31 89 C6 8D 5C 24 38 89 1C 24 C7 44 24 08 80 00 00 00 C7 44 24 04 00 00 00 00 E8 71 6D FC FF A1 ?? ?? ?? ?? 89 84 24 D8 01 00 00";
		// 			unsigned long test = GetAddress(NULL,L"iTunes.dll",abi_sub_8D05A20);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name2);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name3);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name4);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name5);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name6);
		// 			test = GetAddress(NULL,L"iTunes.dll",func_name7);
	}
}