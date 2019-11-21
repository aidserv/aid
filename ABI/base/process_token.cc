#include "ABI/base/process_token.h"
#include "ABI/base/basetypes.h"

namespace ABI{
	namespace base{
		bool ProcessToken::IsUserInAdminGroup(){
			BOOL fInAdminGroup = FALSE;
			DWORD dwError = ERROR_SUCCESS;
			HANDLE hToken = NULL;
			HANDLE hTokenToCheck = NULL;
			DWORD cbSize = 0;
			OSVERSIONINFOW osver = { sizeof(osver),0};
			if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_DUPLICATE,&hToken)){
				dwError = GetLastError();
				goto Cleanup;
			}
			if(!GetVersionEx(&osver)){
				dwError = GetLastError();
				goto Cleanup;
			}

			if(osver.dwMajorVersion >= 6){
				TOKEN_ELEVATION_TYPE elevType;
				if(!GetTokenInformation(hToken,TokenElevationType,&elevType,sizeof(elevType),&cbSize)){
					dwError = GetLastError();
					goto Cleanup;
				}
				if(TokenElevationTypeLimited == elevType){
					if (!GetTokenInformation(hToken,TokenLinkedToken,&hTokenToCheck,sizeof(hTokenToCheck),&cbSize)){
						dwError = GetLastError();
						goto Cleanup;
					}
				}
			}
			if(!hTokenToCheck){
				if(!DuplicateToken(hToken,SecurityIdentification,&hTokenToCheck)){
					dwError = GetLastError();
					goto Cleanup;
				}
			}
			BYTE adminSID[SECURITY_MAX_SID_SIZE];
			cbSize = sizeof(adminSID);
			if(!CreateWellKnownSid(WinBuiltinAdministratorsSid,NULL,&adminSID,&cbSize)){
				dwError = GetLastError();
				goto Cleanup;
			}
			if(!CheckTokenMembership(hTokenToCheck,&adminSID,&fInAdminGroup)){
				dwError = GetLastError();
				goto Cleanup;
			}
Cleanup:
			if (hToken){
				CloseHandle(hToken);
				hToken = NULL;
			}
			if (hTokenToCheck){
				CloseHandle(hTokenToCheck);
				hTokenToCheck = NULL;
			}
			if(ERROR_SUCCESS!=dwError){
				fInAdminGroup = FALSE;
			}

			return fInAdminGroup==TRUE;
		}
		bool ProcessToken::IsRunAsAdmin(){
			BOOL fIsRunAsAdmin = FALSE;
			PSID pAdministratorsGroup = NULL;
			SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
			if(AllocateAndInitializeSid(&NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&pAdministratorsGroup))
			{
				if(!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin)){
					fIsRunAsAdmin = FALSE;
				}
			}
			if(pAdministratorsGroup){
				FreeSid(pAdministratorsGroup);
				pAdministratorsGroup = NULL;
			}
			return fIsRunAsAdmin==TRUE;
		}
		bool ProcessToken::IsProcessElevated(){
			BOOL is_elevated = FALSE;
			HANDLE hToken = NULL;
			if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken)){
				TOKEN_ELEVATION elevation = {0};
				DWORD dwSize;
				if(!GetTokenInformation(hToken,TokenElevation,&elevation,sizeof(elevation), &dwSize)){
					is_elevated = FALSE;
				}
				else{
					is_elevated = elevation.TokenIsElevated;
				}
				if(hToken){
					CloseHandle(hToken);
					hToken = NULL;
				}
			}
			return is_elevated==TRUE;
		}
		void ProcessToken::SetProcessDebugToken(){
			HANDLE hToken;
			TOKEN_PRIVILEGES TokenPrivileges, PreviousState;
			if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,&hToken)){
				if(LookupPrivilegeValueW(NULL,SE_DEBUG_NAME,&TokenPrivileges.Privileges[0].Luid)){
					DWORD dwReturnLength = 0;
					TokenPrivileges.PrivilegeCount = 1;
					TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
					AdjustTokenPrivileges(hToken,FALSE,&TokenPrivileges,sizeof(TOKEN_PRIVILEGES),&PreviousState,&dwReturnLength);
				}
			}
		}
		void ProcessToken::RunProcessAdminToken(){
			if(!IsRunAsAdmin()){
				wchar_t szPath[MAX_PATH] = {0};
				if (GetModuleFileName(NULL,szPath,ARRAYSIZE(szPath))){
					SHELLEXECUTEINFOW sei = { sizeof(sei),0};
					sei.lpVerb = L"runas";
					sei.lpFile = szPath;
					sei.hwnd = GetActiveWindow();
					sei.nShow = SW_NORMAL;
					ShellExecuteExW(&sei);
				}
			}
		}
	}
}