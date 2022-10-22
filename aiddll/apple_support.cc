#include "ios_cracker/apple_support.h"
#include "ABI/base/file/path.h"
#include "ABI/base/windows_version.h"
#include "ABI/thirdparty/glog/scoped_ptr.h"

namespace ios_cracker{
	namespace internal{
		ThirdSupport::ThirdSupport():path_(NULL){
			set_path(new ABI::base::Path);
		}
		ThirdSupport::~ThirdSupport(){
			if(path()!=NULL){
				set_path(NULL,true);
			}
		}
		std::wstring ThirdSupport::AppleApplicationSupportMSI(){
			if(ABI::base::WinVersion().IsX64Os()){
				return (ABI::base::AppendPath(L"tools\\AppleApplicationSupport64.msi").directory_name());
			}
			else{
				return (ABI::base::AppendPath(L"tools\\AppleApplicationSupport32.msi").directory_name());
			}
		}
		std::wstring ThirdSupport::AppleMobileDeviceSupportMSI(){
			if(ABI::base::WinVersion().IsX64Os()){
				return (ABI::base::AppendPath(L"tools\\AppleMobileDeviceSupport64.msi").directory_name());
			}
			else{
				return (ABI::base::AppendPath(L"tools\\AppleMobileDeviceSupport32.msi").directory_name());
			}
		}
		std::wstring ThirdSupport::iTunesMSI(){
			return (ABI::base::AppendPath(L"tools\\iTunes.msi").directory_name());
		}
		std::wstring ThirdSupport::iTunesSetupPackages(){
			if(ABI::base::WinVersion().IsX64Os()){
				return (ABI::base::AppendPath(L"tools\\iTunes64.exe").directory_name());
			}
			else{
				return (ABI::base::AppendPath(L"tools\\iTunes32.exe").directory_name());
			}
		}
		void ThirdSupport::RunSupportApp(const std::wstring& name,const std::wstring& param){
			if(PathFileExistsW(name.c_str())){
				scoped_array<wchar_t> qn_param(new wchar_t[name.length()*5]);
				wcsnset(qn_param.get(),0,name.length()*5);
				SHELLEXECUTEINFOW ShExecInfo = {0};
				memset(&ShExecInfo, 0, sizeof(ShExecInfo));
				ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
				ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				if(ABI::base::WinVersion().IsWinXP()){
					ShExecInfo.lpVerb = NULL;
					ShExecInfo.hwnd = NULL;
				}
				else{
					ShExecInfo.hwnd = GetActiveWindow();
					ShExecInfo.lpVerb = L"runas";
				}
				ShExecInfo.lpFile = name.c_str();
				ShExecInfo.lpParameters = param.c_str();
				ShExecInfo.lpDirectory = NULL;
				ShExecInfo.nShow = SW_NORMAL;
				ShExecInfo.hInstApp = NULL;
// 				if(name.find(L".msi")!=std::string::npos){
// 					ShExecInfo.lpFile = L"msiexec.exe";
// 					wcscpy(qn_param.get(),std::wstring(std::wstring(L"-i ")+name+std::wstring(L" ")+param).c_str());
// 					ShExecInfo.lpParameters = qn_param.get();
// 				}
				BOOL bResult = ShellExecuteEx(&ShExecInfo);
				if(!bResult){
					return;
				}
				WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
				CloseHandle(ShExecInfo.hProcess);
			}
			return;
		}
		void ThirdSupport::set_path(ABI::base::Path* third_path,bool free_state){
			if(free_state){
				ABI::base::Path* third = path();
				if(third!=NULL){
					delete third;
				}
				set_path(third_path);
			}
			else{
				path_ = third_path;
			}
		}
		ABI::base::Path* ThirdSupport::path() const{
			return path_;
		}
	}
}