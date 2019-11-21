#include "base/windows_version.h"

namespace ABI{
	namespace base{
		WinVersion::WinVersion(void){
			set_os_version();
			set_os_info();
		}
		WinVersion::~WinVersion(void){
			set_os_version(true);
			set_os_info(true);
		}
		bool WinVersion::IsAmd64() const{
			return (os_info().dwOemId == PROCESSOR_ARCHITECTURE_AMD64);
		}
		bool WinVersion::IsX64Os() const{
			return (os_info().dwOemId == PROCESSOR_ARCHITECTURE_AMD64 || os_info().dwOemId == PROCESSOR_ARCHITECTURE_IA64);
		}
		bool WinVersion::IsWinXP() const{
			return (os_version().dwMajorVersion==5&&os_version().dwMinorVersion==1);
		}
		bool WinVersion::IsWin2003() const{
			return (os_version().dwMajorVersion==5&&os_version().dwMinorVersion==2&&!GetSystemMetrics(SM_SERVERR2));
		}
		bool WinVersion::IsWin2003_R2() const{
			return (os_version().dwMajorVersion==5&&os_version().dwMinorVersion==2&&GetSystemMetrics(SM_SERVERR2));
		}
		bool WinVersion::IsVista() const{
			return (os_version().dwMajorVersion==6&&os_version().dwMinorVersion==0);
		}
		bool WinVersion::IsWin7() const{
			return (os_version().dwMajorVersion==6&&os_version().dwMinorVersion==1);
		}
		bool WinVersion::IsWin8() const{
			return (os_version().dwMajorVersion==6&&os_version().dwMinorVersion==2);
		}
		bool WinVersion::IsWin8_1() const{
			return (os_version().dwMajorVersion==6&&os_version().dwMinorVersion==3);
		}
		bool WinVersion::IsWinNT5() const{
			return (os_version().dwMajorVersion==5&&os_version().wProductType == VER_NT_WORKSTATION);
		}
		bool WinVersion::IsWinNT6() const{
			return (os_version().dwMajorVersion==6&&os_version().wProductType == VER_NT_WORKSTATION);
		}
	}
}
