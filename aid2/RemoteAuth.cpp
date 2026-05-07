#include <httplib.h>
#include <Windows.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "cpp-base64/cpp-base64.h"
#include "RemoteAuth.h"
#include "Logger.h"
#include "HardwareInfo.h"
#include "Afsync.h"
#include "DeviceManager.h"  
#include "iOSDevice.h"

namespace aid2 {
	using namespace httplib;
	using namespace nlohmann;

    // Constructor with url and udid
    RemoteAuth::RemoteAuth(const string& url, const std::string& udid)
        : m_udid(udid), m_cli(std::make_unique<Client>(url))
    {
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    // Constructor with url, udid, and rootcert
    RemoteAuth::RemoteAuth(const string& url, const std::string& udid, const string& rootcert)
        : m_udid(udid), m_cli(std::make_unique<Client>(url))
    {
        m_cli->load_ca_cert_store(rootcert.c_str(), rootcert.size());
        m_cli->enable_server_certificate_verification(true); // SSL certificate verification
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    // Constructor with url, udid, rootcert, client_cert_path, client_key_path
    RemoteAuth::RemoteAuth(const string& url, const std::string& udid, const string& rootcert, const string& client_cert_path, const string& client_key_path)
        : m_udid(udid), m_cli(std::make_unique<Client>(url, client_cert_path, client_key_path))
    {
        m_cli->load_ca_cert_store(rootcert.c_str(), rootcert.size());
        m_cli->enable_server_certificate_verification(true); // SSL certificate verification
        m_cli->set_connection_timeout(5);
        m_cli->set_read_timeout(5, 0);
        m_cli->set_write_timeout(5, 0);
    }

    RemoteAuth::~RemoteAuth()
    {
        // unique_ptr handles cleanup automatically
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

	bool RemoteAuth::GenerateRs(const string& grappa, long long dsid)
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
            jreq["dsid"] = dsid;

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

	bool RemoteAuth::PushAuthorizeInfo(long long dsid)
	{
        // 查找 SC Info 目录：先找传统版，再找 UWP 版
        auto findSCInfoDir = []() -> string {
            char buf[MAX_PATH];
            // 传统 iTunes：%PROGRAMDATA%\Apple Computer\iTunes\SC Info
            if (GetEnvironmentVariableA("PROGRAMDATA", buf, MAX_PATH)) {
                string path = string(buf) + "\\Apple Computer\\iTunes\\SC Info";
                if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return path;
            }
            // UWP iTunes：%LOCALAPPDATA%\Packages\AppleInc.iTunes_*\LocalCache\Roaming\Apple Computer\iTunes\SC Info
            if (GetEnvironmentVariableA("LOCALAPPDATA", buf, MAX_PATH)) {
                string packagesDir = string(buf) + "\\Packages\\";
                WIN32_FIND_DATAA fd;
                HANDLE hFind = FindFirstFileA((packagesDir + "AppleInc.iTunes_*").c_str(), &fd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            string path = packagesDir + fd.cFileName +
                                "\\LocalCache\\Roaming\\Apple Computer\\iTunes\\SC Info";
                            if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
                                FindClose(hFind);
                                return path;
                            }
                        }
                    } while (FindNextFileA(hFind, &fd));
                    FindClose(hFind);
                }
            }
            return "";
        };

        string scInfoDir = findSCInfoDir();
        if (scInfoDir.empty()) {
            logger.log("PushAuthorizeInfo: SC Info directory not found.");
            return false;
        }

        auto readFile = [](const string& path) -> string {
            std::ifstream f(path, std::ios::binary);
            if (!f) return "";
            return string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        };

        string sidbData = readFile(scInfoDir + "\\SC Info.sidb");
        string siddData = readFile(scInfoDir + "\\SC Info.sidd");
        string txtData  = readFile(scInfoDir + "\\SC Info.txt");

        if (sidbData.empty() || siddData.empty() || txtData.empty()) {
            logger.log("PushAuthorizeInfo: one or more SC Info files missing in %s.", scInfoDir.c_str());
            return false;
        }

        // SC Info.txt 作为 ProductId 使用，去除末尾换行/空白
        while (!txtData.empty() &&
               (txtData.back() == '\r' || txtData.back() == '\n' ||
                txtData.back() == ' '  || txtData.back() == '\t')) {
            txtData.pop_back();
        }

        HardwareInfo hwInfo;
        json jreq;
        jreq["udid"]          = m_udid;
        jreq["dsid"]          = dsid;
        jreq["ProductId"]     = txtData;
        jreq["sidb_data"]     = base64_encode(sidbData);
        jreq["sidd_data"]     = base64_encode(siddData);
        jreq["netserial"]     = hwInfo.NetAdapterSerial();
        jreq["volumeSerial"]  = hwInfo.VolumeSerial();
        jreq["biosVersion"]   = hwInfo.SystemBiosVersion();
        jreq["processorName"] = hwInfo.ProcessorName();
        jreq["computerName"]  = hwInfo.WinComputerName();
        jreq["hwProfile"]     = hwInfo.HwProfile();

        auto res = m_cli->Post("/AppleRemoteAuth.aid/PushAuthorizeInfo", jreq.dump(), "application/json");
        auto err = res.error();
        if (err != Error::Success) {
            logger.log("PushAuthorizeInfo HTTP error: %s", httplib::to_string(err).c_str());
            return false;
        }

        if (res->get_header_value("Content-Type") != "application/json") {
            logger.log("PushAuthorizeInfo unexpected response content-type: %s",
                       res->get_header_value("Content-Type").c_str());
            return false;
        }

        json jres = json::parse(res->body);
        bool ret = jres["ret"];
        if (!ret)
            logger.log("PushAuthorizeInfo failed: %s", jres.value("msg", "").c_str());
        return ret;
	}

}