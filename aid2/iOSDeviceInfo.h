#pragma once
#include "iTunesApi/simpleApi.h"
#include<string>
#include "aid2.h"
namespace aid2 {
	using namespace std;

	//获取udid
	string getUdid(AMDeviceRef deviceHandle);

	class iOSDeviceInfo
	{
	public:
		iOSDeviceInfo(AMDeviceRef deviceHandle);
		~iOSDeviceInfo();

		string FairPlayCertificate();
		uint64_t FairPlayDeviceType();  // FairPlayDeviceType
		uint64_t KeyTypeSupportVersion();  // KeyTypeSupportVersion
		string DeviceName();   // 设备名称
		string ProductType();		// 产品类型，和机型一一对应
		string DeviceEnclosureColor(); // 外壳颜色
		string MarketingName();			//营销机型，如果iPhone 12等
		uint64_t TotalDiskCapacity();		//手机容量，如果128GB
		string udid();
		/******************************************************
		* 功能做配对，并读取需要信认的信息
		返回值：LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           弹出对话框,请在设备端按下“信任”按钮
				LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			使用者按下了“不信任”按钮。
				LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			请打开密码锁定，进入ios主界面
				LOCKDOWN_E_SUCCESS                                 = 0              成功
		***************************************************************/
		int DoPair();  //执行配对工作
	private:
		AMDeviceRef m_deviceHandle = nullptr;

		// 以下是手机获取到的信息
		string m_udid;  //存的udid 
		string m_FairPlayCertificate;   // domain: com.apple.mobile.iTunes key: FairPlayCertificate   存的bytes
		uint64_t m_FairPlayDeviceType = 0;   //domain: com.apple.mobile.iTunes  key: FairPlayDeviceType
		uint64_t m_KeyTypeSupportVersion = 0;  //domain: com.apple.mobile.iTunes key:KeyTypeSupportVersion
		string m_deviceName;
		string m_productType;
		string m_deviceEnclosureColor;
		string m_marketingName;
		uint64_t m_totalDiskCapacity;

		void initializeDevice();
		// 带有session信息
		void initializeDeviceEx();
	};
}

