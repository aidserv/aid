#include "ios_helper/go_auth_device.h"
#include "ABI/base/file/path.h"
#include "ABI/base/windows_process.h"

namespace ios_helper{
	bool GoAuthDevice::WaitAuthDeviceAppUserCertOK(const char* udid){
		USES_CONVERSION;
		std::wstring process_name = ABI::base::AppendPath(L"ios_authorizer.exe").directory_name();
		std::wstring process_arg = A2W(udid);
		return (ABI::base::Create(process_name,process_arg,INFINITE)!=0);
	}
}