#include "GenerateRS_client.h"
using grpc::ClientContext;
using grpc::Status;
using AppleRemoteAuth::RemoteDeviceInfo;
using AppleRemoteAuth::rsdata;
using AppleRemoteAuth::Grappa;
using AppleRemoteAuth::rqGeneGrappa;

const char rootcert_path[] = "certificate/ca.pem";
const char clientcert_path[] = "certificate/client.pem";
const char clientkey_path[] = "certificate/client.key";

static string get_file_contents(const char* fpath)
{
    std::ifstream finstream(fpath);
    std::string contents;
    contents.assign((std::istreambuf_iterator<char>(finstream)),
        std::istreambuf_iterator<char>());
    finstream.close();
    return contents;
}


bool aidClient::RemoteGetGrappa(string udid, std::string &grappa, unsigned long & grappa_session_id) {
    rqGeneGrappa request;
    request.set_udid(udid.c_str(), udid.length());
    Grappa reply;
    ClientContext context;

    // The actual RPC.
    Status status = stub_->GenerateGrappa(&context, request, &reply);

    if (status.ok()) {
        grappa = reply.grappadata();
        grappa_session_id = reply.grappa_session_id();
        return reply.ret();
    }
    else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return false;
    }
}

bool aidClient::RemoteAuth(AppleMobileDeviceEx & mobile_device, unsigned char* rq, unsigned long length, unsigned char* sig, unsigned long sig_length,std::string &rs,std::string &rs_sig) {
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
    request.set_grappa(mobile_device.grappa_data(), mobile_device.grappa_data_len());;
    // Container for the data we expect from the server.
    rsdata reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->GenerateRS(&context, request, &reply);

    if (status.ok()) {
        rs =  reply.rs_data();
        rs_sig = reply.rs_sig_data();
        return reply.ret();
    }
    else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return false;
    }
}

aidClient* aidClient::Instance() {
    {
        static aidClient* client;
        if (!client) {
            auto rootcert = get_file_contents(rootcert_path);
            auto clientkey = get_file_contents(clientkey_path);
            auto clientcert = get_file_contents(clientcert_path);
            grpc::SslCredentialsOptions ssl_opts;
            ssl_opts.pem_root_certs = rootcert;
            ssl_opts.pem_private_key = clientkey;
            ssl_opts.pem_cert_chain = clientcert;
            std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(ssl_opts);
            std::string rs_data;
            std::string rs_sig_data;

            aidClient* new_client = new aidClient(grpc::CreateChannel("aid.aidserv.cn:50051", creds));
            if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&client), new_client, NULL)) {
                delete new_client;
            }
        }
        return client;
    }
}


