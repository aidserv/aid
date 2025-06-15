#pragma once
#include<string>
#include "iTunesApi/simpleApi.h"
namespace aid2 {
	using namespace std;
	class ATH
	{
	public:
		ATH(string udid);
		~ATH();



		/**
		 * ��ȡSyncAllowed״̬
		 *
		 * @return ��ȡ��Ϊtrue,����Ϊflase
		 */
		bool SyncAllowed();
		/**
		 * ��������ͬ��ָ��
		 *
		 * @param grappa ����grappa����
		 * @return ���ͳɹ�Ϊtrue
		 */
		bool RequestingSync(const string& grappa);
		/**
		 * ��ȡReadyForSync״̬
		 *
		 * @param grappa������� ��ȡReadyForSync����ʱ��grappaֵ
		 * @return ��ȡ��Ϊtrue,����Ϊflase
		 */
		bool ReadyForSync(string& grappa);
		/**
		 * �������ͬ��ָ��
		 *
		 * @return ���ͳɹ�Ϊtrue
		 */
		bool FinishedSyncingMetadata();
		/**
		 * ��ȡSyncFinished״̬
		 *
		 * @return ��ȡ��Ϊtrue,����Ϊflase
		 */
		bool SyncFinished();

	private:
		//��������Ҫͬ����Ϣ
		string m_LibraryID = "5AC547BA5322B210";
		string m_ComputerName = "pc";
		string m_Version = "12.6.0.100";
		ATHRef m_ath = nullptr;
	};
}
