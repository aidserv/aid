#ifndef IAUTHEDCRACKER_APPLE_PROTOCOL_H_
#define IAUTHEDCRACKER_APPLE_PROTOCOL_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
#include "ABI/thirdparty/glog/basictypes.h"
//////////////////////////////////////////////////////////////////////////
namespace iAuthedCracker{
	namespace internal{
		class AppleProtocol
		{
		public:
			AppleProtocol(void);
			AppleProtocol(const std::string& apple_id,const std::string& password);
			~AppleProtocol(void);
			void GetLogin_plist(const std::string& apple_id,const std::string& password);
			void set_login_plist(std::string plist);
			void set_x_apple_actionsignature(std::string actionsignature);
			std::string XAppleActionSignature(std::string plist);
			std::string login_plist();
			std::string x_apple_actionsignature();
		private:
			void AddSignSapSetupCert();
			DISALLOW_EVIL_CONSTRUCTORS(AppleProtocol);
			std::string login_plist_;
			std::string x_apple_actionsignature_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif