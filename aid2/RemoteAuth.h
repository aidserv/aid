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
		 * ����RemoteAuth
		 *
		 * @param url ��ʽΪ��schema://host:port
		 * @udid �����ֻ�udid
		 */
		RemoteAuth(const string& url, const std::string& udid);

		/**
		 * ����RemoteAuth
		 *
		 * @param url ��ʽΪ��schema://host:port
		 * @udid �����ֻ�udid
		 * @rootcert ��֤��
		 */
		RemoteAuth(const string& url, 
			const std::string& udid,
			const string& rootcert);
		
		/**
		 * ����RemoteAuth
		 *
		 * @param url ��ʽΪ��schema://host:port
		 * @udid �����ֻ�udid
		 * @rootcert ��֤��
		 * @client_cert_path client֤��·��
		 * @client_key_path client֤����Կ
		 */
		RemoteAuth(const string& url, 
			const std::string& udid,
			const string& rootcert, 
			const string& client_cert_path,
			const string& client_key_path);

		~RemoteAuth();

		/**
		 * ����Grappa
		 *
		 * @param grappa ���������grappa����
		 * @return ��ȡ��Ϊtrue,����Ϊflase
		 */
		bool GenerateGrappa(string& grappa);
		/**
		 * ����Grappa afsync.rs��afsync.rs.sig �ļ���Ϣ
		 *
		 * @param grappa ���������grappa����
		 * @return �ɹ�Ϊtrue,����Ϊflase
		 */
		bool GenerateRs(const string& grappa);
	private:
		std::string m_udid;
		unsigned long m_grappa_session_id = 0;  // grappa session id
		std::unique_ptr<Client> m_cli; // use smart pointer for RAII
	};
}
