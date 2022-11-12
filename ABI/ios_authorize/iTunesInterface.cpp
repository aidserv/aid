#include "ABI/ios_authorize/iTunesInterface.h"
#include <ShlObj.h>
#include <cstdint>
#include <cassert>

//自己地址    游戏地址
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
//计算内存地址         异常显示            模块名称    特征码             特征码长度
DWORD GetTestAddress(WCHAR *Abnormal, WCHAR *name, unsigned char *Nes,int Nes_len)
{
	//得到基地址
	DWORD DllAddress = (DWORD)GetModuleHandle(name);
	unsigned char *t; 
	t=(unsigned char*)DllAddress;
	//模块长度
	int len=0;
	if(DllAddress==0){
		::MessageBox(0,Abnormal,L"提示！未发现指定模块",0);
		return 0;
	}
	IMAGE_DOS_HEADER *Dll_Header = (IMAGE_DOS_HEADER *)DllAddress;
	IMAGE_NT_HEADERS *DLL_Headers = (IMAGE_NT_HEADERS*)  (DllAddress+Dll_Header->e_lfanew);



	if(!DLL_Headers->OptionalHeader.SizeOfCode) // TMD
	{
		len = DLL_Headers->OptionalHeader.SizeOfImage;

	}
	else //通用
	{
		len = DLL_Headers->OptionalHeader.SizeOfCode;
		//DllAddress = DLL_Headers->OptionalHeader.BaseOfCode+DllAddress;
	} 

	for(int i=0;i<len;i++)
	{
		if(true== str_cmp(Nes,&t[i],Nes_len))  
			return (DWORD) (&t[i]);
	}
	::MessageBox(0,Abnormal,L"提示！",0);
	//printf(Abnormal);
	//printf("\n");
	return 0;
}


DWORD GenerateKbsyncID(DWORD lpfnKbsyncID, const char* all_pc_md5, const char* local_pc_md5)
{
	//75 48 8d 55 fc e8
// 	static unsigned char GOstr[]= "\x75\x48\x8d\x55\xfc\xe8";
// 	static DWORD ret=GetTestAddress(L"搜寻GetKbsyncID失败",L"iTunes.dll",GOstr,sizeof(GOstr));
// 	if(!ret){
// 		return -1;
// 	}
// 	static DWORD dwCall = ret+5+*(DWORD*)(ret+6)+5;
// 	static DWORD GetAllMachineInfoMD5Call = dwCall+0x3c+*(DWORD*)(dwCall+0x3d)+5;
// 	static DWORD GetEachMachineInfoMD5Call = dwCall+0x4f+*(DWORD*)(dwCall+0x50)+5;
// 	static DWORD KbsyncIDCall = dwCall+0xC5+*(DWORD*)(dwCall+0xC6)+5;

	//char AllInfoMD5[0x18] = {0};
	//char EachInfoMD5[0x18] = {0};
	//char* sc_info = NULL;
	//if(!sc_info){
	//	static char sc_info_path[MAX_PATH] = {0};
	//	SHGetSpecialFolderPathA(NULL,sc_info_path,CSIDL_COMMON_APPDATA,FALSE);
	//	lstrcatA(sc_info_path,"\\Apple Computer\\iTunes\\SC Info");
	//	sc_info = &sc_info_path[0];
	//}

	char* sc_info = NULL;
	if (!sc_info) {
		char buffer[MAX_PATH] = { 0 };
		char drive[_MAX_DRIVE] = { 0 };
		char dir[_MAX_DIR] = { 0 };
		char fname[_MAX_FNAME] = { 0 };
		char ext[_MAX_EXT] = { 0 };
		char sc_info_path[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		_splitpath(buffer, drive, dir, fname, ext);
		strcat(sc_info_path, drive);
		strcat(sc_info_path, dir);
		strcat(sc_info_path, "Apple Computer\\iTunes\\SC Info");
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