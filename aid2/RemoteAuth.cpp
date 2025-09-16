#include <httplib.h>
#include <nlohmann/json.hpp>
#include "cpp-base64/cpp-base64.h"
#include "RemoteAuth.h"
#include "Logger.h"
#include "Afsync.h"
#include "DeviceManager.h"  
#include "iOSDevice.h"

namespace aid2 {
	using namespace httplib;
	using namespace nlohmann;

    RemoteAuth::RemoteAuth(const string& url, const std::string& udid)
    {
        m_udid = udid;
        m_cli = new  Client(url);
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    RemoteAuth::RemoteAuth(const string& url, const std::string& udid, const string& rootcert)
    {
        m_udid = udid;
        m_cli = new  Client(url);
        m_cli->load_ca_cert_store(rootcert.c_str(), rootcert.size());
        m_cli->enable_server_certificate_verification(true); //sll证书验证
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    RemoteAuth::RemoteAuth(const string& url, const std::string& udid, const string& rootcert, const string& client_cert_path, const string& client_key_path)
    {
        m_udid = udid;
        m_cli = new  Client(url, client_cert_path, client_key_path);
        m_cli->load_ca_cert_store(rootcert.c_str(), rootcert.size());
        m_cli->enable_server_certificate_verification(true); //sll证书验证
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    RemoteAuth::~RemoteAuth()
    {
        delete m_cli;
    }
	bool RemoteAuth::GenerateGrappa(string& grappa)
	{
        bool ret = false;
        json jreq;
		jreq["udid"] = m_udid;

		auto res = m_cli->Post("/AppleRemoteAuth.aid/GenerateGrappa", jreq.dump(), "application/json");
        auto err = res.error();
		if (err == Error::Success && res->get_header_value("Content-Type")=="application/json") {
			json jres = json::parse(res->body);
			if (jres["ret"]) {
				m_grappa_session_id = jres["grappa_session_id"];
                grappa = base64_decode(jres["grappaData"]);
				ret = true;
			}
			else {
				logger.log("CreateHostGrappa error");
			}
		}
		else {
			logger.log("HTTP invocation error:Error message, %s",httplib::to_string(err).c_str());
		}
		return ret;
	}

	bool RemoteAuth::GenerateRs(const string& grappa)
	{
        //读取rq和rq_sid文件   
        bool ret = false;
        try
        {
			auto iosdevice = DeviceManager::get_device(m_udid);
            if (iosdevice->DoPair()) {
                logger.log("udid:%s DoPair Not successful!", m_udid.c_str());
                return ret;
            }
            Afsync afsync(DeviceManager::get_handle(m_udid));
            string rq_data = afsync.ReadRq();
            string rq_sig_data = afsync.ReadRqSig();

            // 通过远程服务器来生成afsync.rs和afsync.rs.sig 文件内容
            json jreq;
            jreq["rq_data"] = base64_encode(rq_data);
            jreq["rq_sig_data"] = base64_encode(rq_sig_data);
            jreq["grappa_session_id"] = m_grappa_session_id;
            jreq["fair_play_certificate"] = base64_encode(iosdevice->FairPlayCertificate());
            jreq["fair_device_type"] = iosdevice->FairPlayDeviceType();
            jreq["KeyTypeSupportVersion"] = iosdevice->KeyTypeSupportVersion();
            jreq["fair_play_guid"] = m_udid;
            jreq["grappa"] = base64_encode(grappa);
            jreq["dsid"] = 0;
            
            auto res = m_cli->Post("/AppleRemoteAuth.aid/GenerateRS", jreq.dump(), "application/json");
            auto err = res.error();
            if (err == Error::Success && res->get_header_value("Content-Type") == "application/json") {
                json jres = json::parse(res->body);
                if (jres["ret"]) {
                    string str_rsdata = jres["rs_data"];
                    string str_sigdata = jres["rs_sig_data"];
                    afsync.WriteRs(base64_decode(str_rsdata));
                    afsync.WriteRsSig(base64_decode(str_sigdata));
                    ret = true;
                }
                else {
                    logger.log("udid:%s genreate rs failed!", m_udid.c_str());
                }
            }
            else {
                logger.log("HTTP invocation error:Error message,%s", httplib::to_string(err).c_str());
            }
            return ret;
        }
        catch (const char* e)
        {
            logger.log(e);
            return ret;
        }
	}

}