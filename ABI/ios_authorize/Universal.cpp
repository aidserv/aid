// Hook.cpp: implementation of the Hook class.
//
//////////////////////////////////////////////////////////////////////
#include "Universal.h"
#include <tchar.h>

#define jmpcall(frm, to) (int)(((int)to - (int)frm) - 5)
BOOL JumpEx(HANDLE hProcess,unsigned long ulAddress, void* Function, int ulNops )
{
	DWORD OldProtect, OldProtect2;
	DWORD NumberOfBytesWritten;
	char Buff[5];
	
	VirtualProtectEx( hProcess, ( void* )ulAddress, 5 + ulNops,
		PAGE_READWRITE, &OldProtect);
	
	
	*( unsigned char* )Buff = 0xE9;
	*( unsigned long* )( Buff + 1 ) = jmpcall( Buff, Function);
	memset( ( void* )( Buff + 5 ), 0x90, ulNops);
	
	WriteProcessMemory(hProcess,(PVOID)ulAddress,&Buff,5+ulNops,&NumberOfBytesWritten);
	
	VirtualProtectEx( hProcess, ( void* )ulAddress, 5 + ulNops,
		OldProtect, &OldProtect2);
	
	return TRUE;
}

BOOL Jump( PVOID pAddress, PVOID pFunction, DWORD dwNopSize )
{
	DWORD dwOldProtect1 = 0;
	DWORD i;
	DWORD dwRet;
    BOOL bRet = FALSE;
	
	__try
	{
		
		dwRet = VirtualProtectEx( GetCurrentProcess(), ( LPVOID )pAddress, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect1);
		if ( dwRet == 0 )
		{
			DebugString( ( _T( "FixGame-->: 内存属性修改失败.. !" ) ));
		}
		
		*( PBYTE )pAddress = 0xE9;
		
		*( PDWORD )( ( ( PBYTE )pAddress ) + 1 ) = jmpcall( pAddress, pFunction);
		
		
		for( i = 0; i < dwNopSize; i++ )
		{
			( ( ( PBYTE )pAddress ) + 5 )[i] = 0x90;
		}
		
		dwRet = VirtualProtectEx( GetCurrentProcess(), ( LPVOID )pAddress, 1,  dwOldProtect1, &dwOldProtect1);
		if ( dwRet == 0 )
		{
			DebugString ( ( _T( "FixGame-->: 内存属性还原失败.. !" ) ));
		}
		
		bRet = TRUE;
		
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
	}
    return bRet;
}

void DebugString(LPCTSTR lpszFormat,...)
{
	va_list args;
	TCHAR szText[1024];
	
	va_start(args, lpszFormat);
	wvsprintf(szText, lpszFormat, args);
	OutputDebugString(szText);
	va_end(args);
}


VOID Inject(HANDLE hProcess,char* name)
{
	PVOID pAddr;
	DWORD NumberOfBytesWritten;
	HANDLE hThread;
	
	pAddr = VirtualAllocEx(hProcess,NULL,strlen(name),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if (pAddr == NULL)
	{
		DebugString(L"Inject VirtualAllocEx Fail\r\n");
		return;
	}
	if(!WriteProcessMemory(hProcess,pAddr, (LPVOID)name, strlen(name),&NumberOfBytesWritten))
	{
		VirtualFreeEx(hProcess,NULL,strlen(name),MEM_RESERVE|MEM_COMMIT);
		DebugString(L"Inject WriteProcessMemory Fail\r\n");
		return;
	}
	hThread = CreateRemoteThread(hProcess,NULL,0, 
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32"),"LoadLibraryA"),
		pAddr,0,NULL);
	
	if (hThread==NULL)
	{
		VirtualFreeEx(hProcess,NULL,strlen(name),MEM_RESERVE|MEM_COMMIT);
		DebugString(L"Inject CreateRemoteThread Fail\r\n");
		return;
	}
	Sleep(1);
	VirtualFreeEx(hProcess,NULL,strlen(name),MEM_RESERVE|MEM_COMMIT);
	CloseHandle(hThread);
}


BOOL SafeCopyMemory( PVOID pDest, PVOID pSrc, DWORD dwSize )
{
	BOOL bRet;
	DWORD dwOldProtect;
	
	__try
	{
		
		bRet = VirtualProtectEx( GetCurrentProcess(), pDest, dwSize,
			PAGE_EXECUTE_READWRITE, &dwOldProtect);
		if ( bRet == FALSE )
		{
			//		KdPrint ( ( _T( "FixGame-->: SafeCopyMemory修改内存属性失败!" ) ));
			return FALSE;
		}
		
		__asm
		{
			mov	edi, pDest
				mov	esi, pSrc
				mov	ecx, dwSize
				rep	movsb
		}
		
		bRet = VirtualProtectEx( GetCurrentProcess(), pDest, dwSize, dwOldProtect,
			&dwOldProtect);
		if ( bRet == FALSE )
		{
			//		KdPrint ( ( _T( "FixGame-->: SafeCopyMemory还原内存属性失败!" ) ));
			return FALSE;
		}
		
		bRet = TRUE;
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bRet = FALSE;
    }
	
	return bRet;
}

VOID HexToString( BYTE byHex, char* pBuf ) 
{
	BYTE byMod;
	
	byMod = byHex / 16;
	
	if ( byMod >= 10 ) {
		byMod += 'A' - 10;
	} else {
		byMod += '0';
	}
	pBuf[0] = byMod;
	
	byMod = byHex % 16;
	
	if ( byMod >= 10 ) {
		byMod += 'A' - 10;
	} else {
		byMod += '0';
	}
	pBuf[1] = byMod;
	pBuf[2] = 0;
	
}

VOID StringToHex( char* pBuf,BYTE* byHex)
{
	BYTE bMod;
	bMod = *pBuf;
	
	if (bMod >= 'A' && bMod < 'a')
	{
		bMod -= 'A';
		bMod += 10;
	}
	else if (bMod >= 'a')
	{
		bMod -= 'a';
		bMod += 10;
	}
	else
	{
		bMod -= '0';
	}
	
	*byHex = bMod*16;
	bMod = *(pBuf+1);
	
	if (bMod >= 'A' && bMod < 'a')
	{
		bMod -= 'A';
		bMod += 10;
	}
	else if (bMod >= 'a')
	{
		bMod -= 'a';
		bMod += 10;
	}
	else
	{
		bMod -= '0';
	}
	*byHex += bMod;
}

//VOID HexToStringEx( BYTE byHex, char* pBuf) 