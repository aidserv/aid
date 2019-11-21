#ifndef ABI_IOS_AUTHORIZE_ITUNES_MODULE_STATE_H_
#define ABI_IOS_AUTHORIZE_ITUNES_MODULE_STATE_H_
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace iTunes{
		class iTunesModuleState
		{
		public:
			iTunesModuleState(){}
			~iTunesModuleState(){}
			bool iTunesIsInstalled();
			bool AppleApplicationSupportIsInstalled();
			bool AppleMobileDeviceSupportIsInstalled();
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif