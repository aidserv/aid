#ifndef IOS_BROKER_PAIR_DEVICE_APP_H_
#define IOS_BROKER_PAIR_DEVICE_APP_H_
//////////////////////////////////////////////////////////////////////////
#include "ios_broker/atl_dll_main.h"
//////////////////////////////////////////////////////////////////////////
namespace ios_broker{
	class WIN_DLL_API PairDeviceApp
	{
	public:
		PairDeviceApp();
		~PairDeviceApp();
		void SetDebugLavel();
		void set_list_apps_mode(int status);
		void set_install_mode(int status);
		void set_uninstall_mode(int status);
		void set_upgrade_mode(int status);
		void set_list_archives_mode(int status);
		void set_archive_mode(int status);
		void set_restore_mode(int status);
		void set_remove_archive_mode(int status);
		void InitializeBroker();
		int InstallBroker(const char* udid,const char* appid,const char* options);
		static PairDeviceApp* GetInstance();
	private:
		void message(const char* format,...);
		int list_apps_mode_;
		int install_mode_;
		int uninstall_mode_;
		int upgrade_mode_;
		int list_archives_mode_;
		int archive_mode_;
		int restore_mode_;
		int remove_archive_mode_;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif