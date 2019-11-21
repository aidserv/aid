#ifndef BASE_WINDOWS_VERSION_H_
#define BASE_WINDOWS_VERSION_H_
//////////////////////////////////////////////////////////////////////////
#include "base/basetypes.h"
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class WinVersion
		{
		public:
			WinVersion(void);
			~WinVersion(void);
			bool IsAmd64() const;
			bool IsX64Os() const;
			bool IsWinXP() const;
			bool IsWin2003() const;
			bool IsWin2003_R2() const;
			bool IsVista() const;
			bool IsWin7() const;
			bool IsWin8() const;
			bool IsWin8_1() const;
			bool IsWinNT5() const;
			bool IsWinNT6() const;
		private:
			inline OSVERSIONINFOEXW os_version() const{
				const OSVERSIONINFOEXW c_os_version = os_version_;
				return c_os_version;
			}
			inline SYSTEM_INFO os_info() const{
				const SYSTEM_INFO os_info = os_info_;
				return os_info;
			}
			inline void set_os_version(const bool only_init=false){
				memset(&os_version_,0,sizeof(os_version_));
				if(!only_init){
					os_version_.dwOSVersionInfoSize = sizeof(os_version_);
					GetVersionExW(reinterpret_cast<LPOSVERSIONINFOW>(&os_version_));
				}
			}
			inline void set_os_info(const bool only_init=false){
				memset(&os_info_,0,sizeof(os_info_));
				if(!only_init){
					GetNativeSystemInfo(&os_info_);
				}
			}
			OSVERSIONINFOEXW os_version_;
			SYSTEM_INFO os_info_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif