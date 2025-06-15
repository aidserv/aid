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
		 * 读取SyncAllowed状态
		 *
		 * @return 读取到为true,否则为flase
		 */
		bool SyncAllowed();
		/**
		 * 发起请求同步指令
		 *
		 * @param grappa 输入grappa参数
		 * @return 发送成功为true
		 */
		bool RequestingSync(const string& grappa);
		/**
		 * 读取ReadyForSync状态
		 *
		 * @param grappa输出参数 读取ReadyForSync到的时候grappa值
		 * @return 读取到为true,否则为flase
		 */
		bool ReadyForSync(string& grappa);
		/**
		 * 发起完成同步指令
		 *
		 * @return 发送成功为true
		 */
		bool FinishedSyncingMetadata();
		/**
		 * 读取SyncFinished状态
		 *
		 * @return 读取到为true,否则为flase
		 */
		bool SyncFinished();

	private:
		//以下是需要同步信息
		string m_LibraryID = "5AC547BA5322B210";
		string m_ComputerName = "pc";
		string m_Version = "12.6.0.100";
		ATHRef m_ath = nullptr;
	};
}
