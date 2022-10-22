#ifndef IOS_CRACKER_APPLE_SUPPORT_H_
#define IOS_CRACKER_APPLE_SUPPORT_H_
//////////////////////////////////////////////////////////////////////////
#include "ABI/base/basetypes.h"
#include "ABI/base/file/path.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_cracker{
	namespace internal{
		class ThirdSupport
		{
		public:
			ThirdSupport();
			~ThirdSupport();
			std::wstring AppleApplicationSupportMSI();
			std::wstring AppleMobileDeviceSupportMSI();
			std::wstring iTunesMSI();
			std::wstring iTunesSetupPackages();
			void RunSupportApp(const std::wstring& name,const std::wstring& param);
		private:
			void set_path(ABI::base::Path* third_path,bool free_state = false);
			ABI::base::Path* path() const;
			ABI::base::Path* path_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif