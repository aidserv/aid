#pragma once
#include <httplib.h>
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
		 * @deviceHandle �����ֻ����
		 */
		RemoteAuth(const string& url, AMDeviceRef deviceHandle);

		/**
		 * ����RemoteAuth
		 *
		 * @param url ��ʽΪ��schema://host:port
		 * @deviceHandle �����ֻ����
		 * @rootcert ��֤��
		 */
		RemoteAuth(const string& url, 
			AMDeviceRef deviceHandle,
			const string& rootcert);
		
		/**
		 * ����RemoteAuth
		 *
		 * @param url ��ʽΪ��schema://host:port
		 * @deviceHandle �����ֻ����
		 * @rootcert ��֤��
		 * @client_cert_path client֤��·��
		 * @client_key_path client֤����Կ
		 */
		RemoteAuth(const string& url, 
			AMDeviceRef deviceHandle,
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
		AMDeviceRef m_deviceHandle;
		unsigned long m_grappa_session_id;  // grappa session id
		Client* m_cli;
	};
}
