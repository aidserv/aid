#pragma once
#include "iTunesApi/simpleApi.h"
#include<string>
#include "aid2.h"
namespace aid2 {
	using namespace std;

	//��ȡudid
	string getUdid(AMDeviceRef deviceHandle);

	class iOSDeviceInfo
	{
	public:
		iOSDeviceInfo(AMDeviceRef deviceHandle);
		~iOSDeviceInfo();

		string FairPlayCertificate();
		uint64_t FairPlayDeviceType();  // FairPlayDeviceType
		uint64_t KeyTypeSupportVersion();  // KeyTypeSupportVersion
		string DeviceName();   // �豸����
		string ProductType();		// ��Ʒ���ͣ��ͻ���һһ��Ӧ
		string DeviceEnclosureColor(); // �����ɫ
		string MarketingName();			//Ӫ�����ͣ����iPhone 12��
		uint64_t TotalDiskCapacity();		//�ֻ����������128GB
		string udid();
		/******************************************************
		* ��������ԣ�����ȡ��Ҫ���ϵ���Ϣ
		����ֵ��LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           �����Ի���,�����豸�˰��¡����Ρ���ť
				LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			ʹ���߰����ˡ������Ρ���ť��
				LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			�����������������ios������
				LOCKDOWN_E_SUCCESS                                 = 0              �ɹ�
		***************************************************************/
		int DoPair();  //ִ����Թ���
	private:
		AMDeviceRef m_deviceHandle = nullptr;

		// �������ֻ���ȡ������Ϣ
		string m_udid;  //���udid 
		string m_FairPlayCertificate;   // domain: com.apple.mobile.iTunes key: FairPlayCertificate   ���bytes
		uint64_t m_FairPlayDeviceType = 0;   //domain: com.apple.mobile.iTunes  key: FairPlayDeviceType
		uint64_t m_KeyTypeSupportVersion = 0;  //domain: com.apple.mobile.iTunes key:KeyTypeSupportVersion
		string m_deviceName;
		string m_productType;
		string m_deviceEnclosureColor;
		string m_marketingName;
		uint64_t m_totalDiskCapacity;

		void initializeDevice();
		// ����session��Ϣ
		void initializeDeviceEx();
	};
}

