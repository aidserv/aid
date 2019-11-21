#ifndef SMARTBOT_ACCOUNT_THIRD_MAILID_H_
#define SMARTBOT_ACCOUNT_THIRD_MAILID_H_
//////////////////////////////////////////////////////////////////////////
#include "smartbot/passport/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace smartbot{
	namespace account{
		class ThirdMailId
		{
		public:
			WIN_DLL_API ThirdMailId();
			WIN_DLL_API ~ThirdMailId();
			WIN_DLL_API void RegistedUser();
		private:
			DISALLOW_EVIL_CONSTRUCTORS(ThirdMailId);
		};
	}
}
#endif