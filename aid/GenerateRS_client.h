#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "ABI/ios_authorize/apple_mobile_device_ex.h"
#include "ProtoBuf/GenerateRS/GenerateRS.grpc.pb.h"
#include "ProtoBuf/GenerateRS/GenerateRS.pb.h"

using namespace ABI::internal;
using grpc::Channel;
using AppleRemoteAuth::aid;


class aidClient {
public:
	aidClient(std::shared_ptr<Channel> channel)
		: stub_(aid::NewStub(channel)) {}
	bool aidClient::RemoteGetGrappa(string udid, std::string& grappa, unsigned long& grappa_session_id);
	bool RemoteAuth(AppleMobileDeviceEx& mobile_device, unsigned char* rq, unsigned long length, unsigned char* sig, unsigned long sig_length, std::string &rs, std::string& rs_sig);
	static aidClient* Instance();
private:
	std::unique_ptr<aid::Stub> stub_;
};
