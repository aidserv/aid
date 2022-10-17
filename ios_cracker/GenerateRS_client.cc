#include "GenerateRS_client.h"
using grpc::ClientContext;
using grpc::Status;
using AppleRemoteAuth::RemoteDeviceInfo;
using AppleRemoteAuth::rsdata;



std::string aidClient::RemoteAuth(AppleMobileDeviceEx & mobile_device, unsigned char* rq, unsigned long length, unsigned char* sig, unsigned long sig_length) {
    // Data we are sending to the server.
    RemoteDeviceInfo request;
    request.set_rq_data((char*)rq, length);
    request.set_rq_sig_data((char*)sig, sig_length);
    request.set_grappa_session_id(mobile_device.grappa_session_id());
    request.set_key_fair_play_guid(mobile_device.key_fair_play_guid(), mobile_device.key_fair_play_guid_length());
    request.set_fair_play_certificate(mobile_device.fair_play_certificate(), mobile_device.fair_play_certificate_len());
    request.set_fair_device_type(mobile_device.fair_play_device_type());
    request.set_private_key(0);
    request.set_fair_play_guid(mobile_device.fair_play_guid(), strlen(mobile_device.fair_play_guid()));

    // Container for the data we expect from the server.
    rsdata reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->GenerateRS(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
        return reply.rs_data();
    }
    else {
        std::cout << status.error_code() << ": " << status.error_message()
            << std::endl;
        return "RPC failed";
    }
}
