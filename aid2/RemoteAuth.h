#pragma once
#include <httplib.h>
#include <memory> // for std::unique_ptr
#include "iTunesApi/simpleApi.h"
namespace aid2 {
	using namespace std;
	using namespace httplib;
	class RemoteAuth
	{
	public:
		/**
		 * 构造RemoteAuth
		 *
		 * @param url 格式为：schema://host:port
		 * @udid 连接手机udid
		 */
		RemoteAuth(const string& url, const std::string& udid);

		/**
		 * 构造RemoteAuth
		 *
		 * @param url 格式为：schema://host:port
		 * @udid 连接手机udid
		 * @rootcert 根证书
		 */
		RemoteAuth(const string& url, 
			const std::string& udid,
			const string& rootcert);
		
		/**
		 * 构造RemoteAuth
		 *
		 * @param url 格式为：schema://host:port
		 * @udid 连接手机udid
		 * @rootcert 根证书
		 * @client_cert_path client证书路径
		 * @client_key_path client证书密钥
		 */
		RemoteAuth(const string& url, 
			const std::string& udid,
			const string& rootcert, 
			const string& client_cert_path,
			const string& client_key_path);

		~RemoteAuth();

		/**
		 * 生成Grappa
		 *
		 * @param grappa 输出参数，grappa参数
		 * @return 读取到为true,否则为flase
		 */
		bool GenerateGrappa(string& grappa);
		/**
		 * 生成Grappa afsync.rs和afsync.rs.sig 文件信息
		 *
		 * @param grappa 输入参数，grappa参数
		 * @return 成功为true,否则为flase
		 */
		bool GenerateRs(const string& grappa);
	private:
		std::string m_udid;
		unsigned long m_grappa_session_id = 0;  // grappa session id
		std::unique_ptr<Client> m_cli; // use smart pointer for RAII
	};
}
