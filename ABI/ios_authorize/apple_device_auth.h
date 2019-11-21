#ifndef ABI_IOS_AUTHORIZE_APPLE_DEVICE_AUTH_H_
#define ABI_IOS_AUTHORIZE_APPLE_DEVICE_AUTH_H_
//////////////////////////////////////////////////////////////////////////
#include "abi/ios_authorize/apple_import.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace internal{
		enum ErrorAuthorize{
			authorize_ok,
			open_afsync_rq_fail,
			read_afsync_rq_fail,
			open_afsync_rq_sig_fail,
			read_afsync_rq_sig_fail,
			open_afsync_rs_fail,
			write_afsync_rs_fail,
			open_afsync_rs_sig_fail,
			write_afsync_rs_sig_fail,
			wait_object_fail,
			device_info_init,
			generate_rs_fail,
			unknown_sync_error
		};
		void message(const char* format,...);
		bool __cdecl WaitDeviceUDID(const char* udid);
		bool __cdecl ConnectIOSDevice(AMDeviceRef device);
		bool __cdecl CloseIOSDevice(AMDeviceRef device);
		ErrorAuthorize __cdecl AuthorizeDevice(void *deviceHandle);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif