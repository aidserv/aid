#include "ios_cracker/atl_dll_main.h"
#include "ios_broker/pair_device_trust.h"
#pragma comment(lib,"ios_broker.lib")
#pragma comment(lib,"ios_cracker.lib")
#include "ios_helper/remote_auth_request_impl.h"

int main(int argc, char* argv[]){
	std::ifstream ifile(argv[1],std::ios::binary|std::ios::in);
	ifile.seekg(0, ifile.end);
	int length = ifile.tellg();
	ifile.seekg(0, ifile.beg);
	char * buffer = new char[length];
	ifile.read(buffer, length);
	ifile.close();
	iGenAuthRequest::GenAuthedResponseFormResuqest(buffer, "ios.authorized");
	return 0;
}