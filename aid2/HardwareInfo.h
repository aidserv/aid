#pragma once
#include <string>
namespace aid2 {
	class HardwareInfo
	{
	public:
		HardwareInfo() {};
		~HardwareInfo() {};
		// 获取网卡地址
		std::string NetAdapterSerial();
		// 获取系统盘卷序列号
		unsigned long VolumeSerial();
		// 获取系统BIOS版本
		std::string SystemBiosVersion();
		// 获取处理器名称
		std::string ProcessorName();
		// 获取计算机名称
		std::string WinComputerName();
		// 获取硬件配置文件
		std::string HwProfile();
	};
}
