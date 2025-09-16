#pragma once
#include <string>
#include <memory>
#include <map>  
#include "iTunesApi/simpleApi.h"
namespace aid2 {
    class DeviceManager {
    public:
		// ��ȡ�豸��ʵ��
        static std::shared_ptr<class iOSDevice> get_device(AMDeviceRef deviceHandle);
		// ����udid��ȡ�豸��ʵ��,���û���ҵ�����nullptr
        static std::shared_ptr<class iOSDevice> get_device(const std::string& udid);

        // ����������udid���豸�����ӳ��
        static void add_device(const std::string& udid, AMDeviceRef deviceHandle);
		// ɾ��������udidɾ��ӳ��
        static void remove_device(const std::string& udid);
		// ɾ��������deviceHandleɾ��ӳ��
        static void remove_device(AMDeviceRef deviceHandle);

		//����deviceHandle��map��ȡudid
        static std::string get_udid(AMDeviceRef deviceHandle);
        //����udid��map��ȡdeviceHandle
        static AMDeviceRef get_handle(const std::string& udid);
    private:
        static std::map<std::string, AMDeviceRef> m_udid;
    };
}