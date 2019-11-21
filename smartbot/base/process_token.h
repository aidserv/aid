#ifndef ABI_BASE_PROCRSS_TOKEN_H_
#define ABI_BASE_PROCRSS_TOKEN_H_
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class ProcessToken
		{
		public:
			ProcessToken(){}
			~ProcessToken(){}
			bool IsUserInAdminGroup();
			bool IsRunAsAdmin();
			bool IsProcessElevated();
			void SetProcessDebugToken();
			void RunProcessAdminToken();
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif