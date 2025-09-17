#pragma once
#include <string>
#include <memory>
#include <map>  
#include "iTunesApi/simpleApi.h"
namespace aid2 {
    class DeviceManager {
    public:
        //����deviceHandle��ȡudid
        static std::string get_udid(AMDeviceRef deviceHandle);
        // ��ȡ�豸��ʵ��
        static std::shared_ptr<class iOSDevice> get_device(AMDeviceRef deviceHandle);
        /*�����ǹ���map�ķ���*/
        // ����������udid���豸�����ӳ��
        static void add_device(const std::string& udid, AMDeviceRef deviceHandle);
		// ɾ��������udidɾ��ӳ��
        static void remove_device(const std::string& udid);
        //����udid��map��ȡdeviceHandle
        static AMDeviceRef get_handle(const std::string& udid);
        // ����udid��map��ȡ�豸��ʵ��,���û���ҵ�����nullptr
        static std::shared_ptr<class iOSDevice> get_device(const std::string& udid);
    private:
        static std::map<std::string, AMDeviceRef> m_udid;
    };
}