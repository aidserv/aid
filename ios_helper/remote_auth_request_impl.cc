#include "ios_helper/remote_auth_request_impl.h"
//follow console example:
// int main(int argc,char* argv[]){
// 	ABI::internal::AppleRemoteAuth auth;
// 	if(argc!=3){
// 		LOG(ERROR)<<"startup parameter count error!"<<std::endl;
// 		return 1;
// 	}
// 	LoadDlls();
// 	auth.GenerateAuthRS(auth.AnalysisDeviceParam(argv[1]),argv[2]);
// 	return 0;
// }
namespace iGenAuthRequest{
	void __cdecl GenAuthedResponseFormResuqest(const char* encode,const char* out_file){
		AuthRequestHandled<ABI::internal::AppleRemoteAuth> auth;
		auth.GenAuthedResponseFormResuqest(encode,out_file);
	}
}