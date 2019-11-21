// Hook.h: interface for the Hook class.
//
//////////////////////////////////////////////////////////////////////
#include <Windows.h>

BOOL JumpEx(HANDLE hProcess,unsigned long ulAddress, void* Function, int ulNops );
BOOL Jump( PVOID pAddress, PVOID pFunction, DWORD dwNopSize );
void DebugString(LPCTSTR lpszFormat,...);
VOID Inject(HANDLE hProcess,char* name);
BOOL SafeCopyMemory( PVOID pDest, PVOID pSrc, DWORD dwSize );
VOID HexToString( BYTE byHex, char* pBuf );
VOID StringToHex( char* pBuf,BYTE* byHex);
VOID SendLogoinPacket( char* lpUserName, char* lpPwd );