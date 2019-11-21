#include "ABI/ios_authorize/iTunesInterface.h"
#include <ShlObj.h>
#include <cstdint>
#include <cassert>

DWORD Ios8 = 1;
//◊‘º∫µÿ÷∑    ”Œœ∑µÿ÷∑
bool str_cmp(unsigned char *ct1,unsigned char *ct2 ,int len)
{
	for(int i=0;i<len;i++)
	{ 
		if(ct1[i]!=ct2[i] && ct1[i]!= 0)
			return false;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//º∆À„ƒ⁄¥Êµÿ÷∑         “Ï≥£œ‘ æ            ƒ£øÈ√˚≥∆    Ãÿ’˜¬Î             Ãÿ’˜¬Î≥§∂»
DWORD GetTestAddress(WCHAR *Abnormal, WCHAR *name, unsigned char *Nes,int Nes_len)
{
	//µ√µΩª˘µÿ÷∑
	DWORD DllAddress = (DWORD)GetModuleHandle(name);
	unsigned char *t; 
	t=(unsigned char*)DllAddress;
	//ƒ£øÈ≥§∂»
	int len=0;
	if(DllAddress==0){
		::MessageBox(0,Abnormal,L"Ã· æ£°Œ¥∑¢œ÷÷∏∂®ƒ£øÈ",0);
		return 0;
	}
	IMAGE_DOS_HEADER *Dll_Header = (IMAGE_DOS_HEADER *)DllAddress;
	IMAGE_NT_HEADERS *DLL_Headers = (IMAGE_NT_HEADERS*)  (DllAddress+Dll_Header->e_lfanew);



	if(!DLL_Headers->OptionalHeader.SizeOfCode) // TMD
	{
		len = DLL_Headers->OptionalHeader.SizeOfImage;

	}
	else //Õ®”√
	{
		len = DLL_Headers->OptionalHeader.SizeOfCode;
		//DllAddress = DLL_Headers->OptionalHeader.BaseOfCode+DllAddress;
	} 

	for(int i=0;i<len;i++)
	{
		if(true== str_cmp(Nes,&t[i],Nes_len))  
			return (DWORD) (&t[i]);
	}
	::MessageBox(0,Abnormal,L"Ã· æ£°",0);
	//printf(Abnormal);
	//printf("\n");
	return 0;
}


DWORD VerifyAfsyncRq(DWORD dwGrappaSessionId, 
	unsigned char* pszRq, 
	DWORD dwSizeRq, 
	unsigned char* pszRqSig, 
	DWORD dwSizeRqSig)
{
	//56 53 51 50 52 e8 
	static unsigned char GOstr[]= "\x56\x53\x51\x50\x52\xe8";
	static DWORD ret=GetTestAddress(L"À——∞VerifyAfsyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push dwSizeRqSig
		push pszRqSig
		push dwSizeRq
		push pszRq
		push dwGrappaSessionId
		call dwCall
		add esp,0x14
	}
}

DWORD VerifyAfsyncRq_8(DWORD dwGrappaSessionId, 
	unsigned char* pszRq, 
	DWORD dwSizeRq, 
	unsigned char* pszRqSig, 
	DWORD dwSizeRqSig)
{
	//ff b5 c0 fb ff ff e8
	static unsigned char GOstr[]= "\xff\xb5\xc0\xfb\xff\xff\xe8";
	static DWORD ret=GetTestAddress(L"À——∞VerifyAfsyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+6+*(DWORD*)(ret+7)+5;

	_asm
	{
		push dwSizeRqSig
		push pszRqSig
		push dwSizeRq
		push pszRq
		push dwGrappaSessionId
		call dwCall
		add esp,0x14
	}
}

DWORD GeneratePrivateKey(DWORD dwKbsyncId, 
	unsigned char* pszFairPlayCertificate, 
	DWORD dwFairPlayCertificateSize, 
	PVOID lpFairPlayGUID, //0x14
	INT64* pdwFairPlayDeviceTypeAndKeyType,//0x65
	DWORD dwType,//1
	DWORD* pdwPrivateKey)
{
	//8d 55 e4 52 56 50 51 e8
	static unsigned char GOstr[]= "\x8d\x55\xe4\x52\x56\x50\x51\xe8";
	static DWORD ret=GetTestAddress(L"À——∞CalcAfSyncPrivateKey ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+7+*(DWORD*)(ret+8)+5;

	_asm
	{
		push pdwPrivateKey
		push dwType
		push pdwFairPlayDeviceTypeAndKeyType
		push lpFairPlayGUID
		push dwFairPlayCertificateSize
		push pszFairPlayCertificate
		push dwKbsyncId
		call dwCall
		add esp,0x1c
	}
}

DWORD GeneratePrivateKey_8(DWORD dwKbsyncId, 
	unsigned char* pszFairPlayCertificate, 
	DWORD dwFairPlayCertificateSize, 
	PVOID lpFairPlayGUID, //0x14
	INT64* pdwFairPlayDeviceTypeAndKeyType,//0x65
	DWORD dwType,//1
	DWORD* pdwPrivateKey)
{
	//57 52 50 51 e8
	static unsigned char GOstr[]= "\x57\x52\x50\x51\xe8";
	static DWORD ret=GetTestAddress(L"À——∞CalcAfSyncPrivateKey ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+4+*(DWORD*)(ret+5)+5;

	_asm
	{
		push pdwPrivateKey
		push dwType
		push pdwFairPlayDeviceTypeAndKeyType
		push lpFairPlayGUID
		push dwFairPlayCertificateSize
		push pszFairPlayCertificate
		push dwKbsyncId
		call dwCall
		add esp,0x1c
	}
}

DWORD SetAfSyncRq(DWORD dwPrivateKey, 
	const char* pszRqData, 
	DWORD dwRqSize, 
	const char* pszICInfoData,
	DWORD dwICInfoSize)
{
	//8b 02 56 51 50 e8
	static unsigned char GOstr[]= "\x8b\x02\x56\x51\x50\xe8";
	static DWORD ret=GetTestAddress(L"À——∞SetAfSyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;
	_asm
	{
		push dwICInfoSize
		push pszICInfoData
		push dwRqSize
		push pszRqData
		push dwPrivateKey
		call dwCall
		add esp,0x14
	}
}

DWORD SetAfSyncRq_8(DWORD dwPrivateKey, 
	const char* pszRqData, 
	DWORD dwRqSize, 
	const char* pszICInfoData,
	DWORD dwICInfoSize)
{
	//57 53 ff 75 f8 e8
	static unsigned char GOstr[]= "\x57\x53\xff\x75\xf8\xe8";
	static DWORD ret=GetTestAddress(L"À——∞SetAfSyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push dwICInfoSize
		push pszICInfoData
		push dwRqSize
		push pszRqData
		push dwPrivateKey
		call dwCall
		add esp,0x14
	}
}

DWORD ParseAfsyncRq(DWORD dwMod, //0
	PVOID lpFairPlayGUID,
	DWORD dwFairPlayDeviceType,
	const char* pszRqData, 
	DWORD dwRqSize,
	const char** ppszDisdList, 
	DWORD* pdwDisdCount)
{
	//8d 55 d8 52 53 e8
	static unsigned char GOstr[]= "\x8d\x55\xd8\x52\x53\xe8";
	static DWORD ret=GetTestAddress(L"À——∞ParseAfsyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push pdwDisdCount
		push ppszDisdList
		push dwRqSize
		push pszRqData
		push dwFairPlayDeviceType
		push lpFairPlayGUID
		push dwMod
		call dwCall
		add esp,0x1c
	}
}

DWORD ParseAfsyncRq_8(DWORD dwMod, //0
	PVOID lpFairPlayGUID,
	DWORD dwFairPlayDeviceType,
	const char* pszRqData, 
	DWORD dwRqSize,
	const char** ppszDisdList, 
	DWORD* pdwDisdCount)
{
	//50 8d 45 e4 50 57 e8
	static unsigned char GOstr[]= "\x50\x8d\x45\xe4\x50\x57\xe8";
	static DWORD ret=GetTestAddress(L"À——∞ParseAfsyncRq ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+6+*(DWORD*)(ret+7)+5;

	_asm
	{
		push pdwDisdCount
		push ppszDisdList
		push dwRqSize
		push pszRqData
		push dwFairPlayDeviceType
		push lpFairPlayGUID
		push dwMod
		call dwCall
		add esp,0x1c
	}
}

DWORD AuthKeyByDSID_8(DWORD dwPrivateKey, 
	DWORD dwDsidLow, 
	DWORD dwDsidHigh, 
	DWORD dwUnkVaule,
	DWORD dwUnkVaule2)
{
	//52 51 ff 75 f8 e8
	static unsigned char GOstr[]= "\x52\x51\xff\x75\xf8\xe8";
	static DWORD ret=GetTestAddress(L"À——∞PreAuthByDSID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push dwUnkVaule2
		push dwUnkVaule
		push dwDsidHigh
		push dwDsidLow
		push dwPrivateKey
		call dwCall
		add esp,0x14
	}
}

DWORD AuthKeyByDSID(DWORD dwPrivateKey, 
	DWORD dwDsidLow, 
	DWORD dwDsidHigh, 
	DWORD dwUnkVaule,
	DWORD dwUnkVaule2)
{
	//6a 00 51 50 8b 45 fc 50 e8
	static unsigned char GOstr[]= "\x6a\x00\x51\x50\x8b\x45\xfc\x50\xe8";
	static DWORD ret=GetTestAddress(L"À——∞PreAuthByDSID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+8+*(DWORD*)(ret+9)+5;

	_asm
	{
		push dwUnkVaule2
			push dwUnkVaule
			push dwDsidHigh
			push dwDsidLow
			push dwPrivateKey
			call dwCall
			add esp,0x14
	}
}

//…˙≥…RSŒƒº˛
DWORD GenerateAfSyncRs(DWORD dwPrivateKey, 
	const char** ppszRsData, 
	DWORD* pdwRsSize, 
	const char** ppszICInfoData, 
	DWORD* pdwICInfoSize)
{
	//51 52 56 50 57 e8
	static unsigned char GOstr[]= "\x51\x52\x56\x50\x57\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GenerateAfSyncRs ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push pdwICInfoSize
		push ppszICInfoData
		push pdwRsSize
		push ppszRsData
		push dwPrivateKey
		call dwCall
		add esp,0x14
	}
}

DWORD GenerateAfSyncRs_8(DWORD dwPrivateKey, 
	const char** ppszRsData, 
	DWORD* pdwRsSize, 
	const char** ppszICInfoData, 
	DWORD* pdwICInfoSize)
{
	//57 52 51 50 ff 75 08 e8
	static unsigned char GOstr[]= "\x57\x52\x51\x50\xff\x75\x08\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GenerateAfSyncRs ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+7+*(DWORD*)(ret+8)+5;

	_asm
	{
		push pdwICInfoSize
		push ppszICInfoData
		push pdwRsSize
		push ppszRsData
		push dwPrivateKey
		call dwCall
		add esp,0x14
	}
}

DWORD GetSCInfoPath(char* Path)
{
	//89 bd 34 f5 ff ff 89 3e e8
	static unsigned char GOstr[]= "\x89\xbd\x34\xf5\xff\xff\x89\x3e\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GetSCInfoPath ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+8+*(DWORD*)(ret+9)+5;

	_asm
	{
		push Path
		call dwCall
		add esp,4
	}
}

DWORD GetSCInfoPath_8(char* Path)
{
	//c7 85 2c f5 ff ff 00 00 00 00 c7 03 00 00 00 00 e8
	static unsigned char GOstr[]= "\xc7\x85\x2c\xf5\xff\xff\x00\x00\x00\x00\xc7\x03\x00\x00\x00\x00\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GetSCInfoPath ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+16+*(DWORD*)(ret+17)+5;

	_asm
	{
		push ecx
		mov ecx,Path
		call dwCall
		pop ecx
	}
}

DWORD GetKbsync_ID(char* path,DWORD UNKNOW1,DWORD UNKNOW2,DWORD UNKNOW3,DWORD* pdwKbsyncID)
{
	//8d 8d 50 f5 ff ff 57 51 e8
	static unsigned char GOstr[]= "\x89\xbd\x34\xf5\xff\xff\x89\x3e\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GetSCInfoPath ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+8+*(DWORD*)(ret+9)+5;

	_asm
	{
		push pdwKbsyncID
		push UNKNOW3
		push UNKNOW2
		push UNKNOW1
		push path
		call dwCall 
		add esp,0x14
	}
}

DWORD GetKbsyncID(char* path,DWORD* dwKbsyncID)
{
	/*
	//3b cf 75 08 8b 0d
	static unsigned char GOstr[]= "\x3b\xcf\x75\x08\x8b\x0d";
	static DWORD ret=GetTestAddress(L"À——∞GetKbsyncID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD* pKbsyncID = *(DWORD**)(ret+6);

	return *pKbsyncID;
	*/
	//8b 4d 08 8d 45 fc 50 e8
	static unsigned char GOstr[]= "\x8b\x4d\x08\x8d\x45\xfc\x50\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GetKbsyncID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+7+*(DWORD*)(ret+8)+5;

	_asm
	{
		push ecx
		mov ecx,path
		push dwKbsyncID
		call dwCall 
		add esp,0x4
		pop ecx
	}
}

DWORD GetKbsyncID_8(char* path,DWORD* pdwKbsyncID)
{
	//75 48 8d 55 fc e8
	static unsigned char GOstr[]= "\x75\x48\x8d\x55\xfc\xe8";
	static DWORD ret=GetTestAddress(L"À——∞GetKbsyncID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;

	_asm
	{
		push ecx
		push edx

		mov ecx,path
		mov edx,pdwKbsyncID
		call dwCall 

		pop edx
		pop ecx
	}
}

DWORD CalcHashSig(DWORD nGrappaSessionId,char* pInputBuf, DWORD nInputBufSize,
	              char** ppOutputBuf, DWORD* pOutputBufSize)
{
	//52 56 50 57 e8 ?? ?? ?? ?? 8b f0 83 c4 14
	static unsigned char GOstr[]= "\x52\x56\x50\x57\xE8\x00\x00\x00\x00\x8B\xF0\x83\xC4\x14";
	static DWORD ret=GetTestAddress(L"À——∞iTunesCalcHashSig ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+4+*(DWORD*)(ret+5)+5;

	_asm
	{
		push pOutputBufSize
		push ppOutputBuf
		push nInputBufSize
		push pInputBuf
		push nGrappaSessionId
		call dwCall
		add esp,0x14
	}
}
DWORD CalcHashSig_8(DWORD nGrappaSessionId,char* pInputBuf, DWORD nInputBufSize,
	char** ppOutputBuf, DWORD* pOutputBufSize)
{
	//e8 ?? ?? ?? ?? 50 56 e8
	static unsigned char GOstr[]= "\xe8\x00\x00\x00\x00\x50\x56\xe8";
	static DWORD ret=GetTestAddress(L"À——∞iTunesCalcHashSig ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
	static DWORD dwCall = ret+7+*(DWORD*)(ret+8)+5;

	_asm
	{
		push pOutputBufSize
		push ppOutputBuf
		push nInputBufSize
		push pInputBuf
		push nGrappaSessionId
		call dwCall
		add esp,0x14
	}
}


DWORD GenerateKbsyncID(DWORD lpfnKbsyncID, const char* all_pc_md5, const char* local_pc_md5)
{
	//75 48 8d 55 fc e8
// 	static unsigned char GOstr[]= "\x75\x48\x8d\x55\xfc\xe8";
// 	static DWORD ret=GetTestAddress(L"À——∞GetKbsyncID ß∞‹",L"iTunes.dll",GOstr,sizeof(GOstr));
// 	if(!ret){
// 		return -1;
// 	}
// 	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;
// 	static DWORD GetAllMachineInfoMD5Call = dwCall+0x3c+*(DWORD*)(dwCall+0x3d)+5;
// 	static DWORD GetEachMachineInfoMD5Call = dwCall+0x4f+*(DWORD*)(dwCall+0x50)+5;
// 	static DWORD KbsyncIDCall = dwCall+0xC5+*(DWORD*)(dwCall+0xC6)+5;

	char AllInfoMD5[0x18] = {0};
	char EachInfoMD5[0x18] = {0};
	char* sc_info = NULL;
	if(!sc_info){
		static char sc_info_path[MAX_PATH] = {0};
		SHGetSpecialFolderPathA(NULL,sc_info_path,CSIDL_COMMON_APPDATA,FALSE);
		lstrcatA(sc_info_path,"\\Apple Computer\\iTunes\\SC Info");
		sc_info = &sc_info_path[0];
	}
	DWORD KbsyncID;
	int(_cdecl* CalcKbsyncID)(const char*, const char*, const char*, DWORD*);
	if (lpfnKbsyncID&&all_pc_md5&&local_pc_md5){
		*(DWORD*)&CalcKbsyncID = lpfnKbsyncID;
		DWORD calc_kbsync_id_error = CalcKbsyncID(all_pc_md5, local_pc_md5, sc_info, &KbsyncID);
		assert(calc_kbsync_id_error == 0);
	}
// 	__asm
// 	{
// 		pushad
// 		lea ecx,AllInfoMD5
// 		call GetAllMachineInfoMD5Call
// 		lea ecx,EachInfoMD5
// 		call GetEachMachineInfoMD5Call
// 
// 		lea eax,KbsyncID
// 		push eax
// 		push sc_info
// 		lea eax,EachInfoMD5
// 		push eax
// 		lea eax,AllInfoMD5
// 		push eax
// 		call KbsyncIDCall
// 		add esp,0x10
// 		popad
// 	}
	return KbsyncID;
}