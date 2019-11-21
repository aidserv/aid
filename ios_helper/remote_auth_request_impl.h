#ifndef IOS_HELPER_REMOTE_AUTH_REQUEST_IMPL_H_
#define IOS_HELPER_REMOTE_AUTH_REQUEST_IMPL_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
#include "ABI/base/atl/dll_main.h"
#include "ABI/thirdparty/glog/logging.h"
#include "ABI/ios_authorize/apple_remote_auth.h"
//////////////////////////////////////////////////////////////////////////
namespace iGenAuthRequest{
	template <typename T=ABI::internal::AppleRemoteAuth>
	class AuthRequestHandled
	{
	public:
		AuthRequestHandled(void):ptr_(NULL){
			ptr_.reset(new T);
		}
		void GenAuthedResponseFormResuqest(const char* encode,const char* out_file){
			LoadDlls();
			ptr_->GenerateAuthRS(ptr_->AnalysisDeviceParam(encode),out_file);
		}
		~AuthRequestHandled(void){
			ptr_.reset(NULL);
		}
	private:
		std::auto_ptr<T> ptr_;
	};
#ifdef __cplusplus
	extern "C"{
#endif
		WIN_DLL_API void __cdecl GenAuthedResponseFormResuqest(const char* encode,const char* out_file);
#ifdef __cplusplus
	};
#endif
}
//////////////////////////////////////////////////////////////////////////
#endif