#ifndef BASE_WINDOWS_HARDWARE_H_
#define BASE_WINDOWS_HARDWARE_H_
//////////////////////////////////////////////////////////////////////////
#include <string>
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace base{
		class HardwareInfo
		{
		public:
			HardwareInfo();
			~HardwareInfo();
			bool GetAdapterSerial(std::string& out);
			bool GetVolumeSerial(std::string& out);
			bool GetSystemBios(std::string& out);
			bool GetProcessorName(std::string& out);
			bool GetWinProductId(std::string& out);
			bool GetWinComputerName(std::string& out);
			std::string GetMachineName();
			std::wstring HwProfile();
			std::string cookie() const;
		private:
			void set_cookie(const std::string hardware_cookie);
			std::string cookie_;
		};
		std::string GetHardwareCookie();
	}
}
//////////////////////////////////////////////////////////////////////////
#endif