#include "DeviceManager.h"  
#include "iOSDevice.h"  
#include <string>  

namespace aid2 {  
	std::map<std::string, am_device*> aid2::DeviceManager::m_udid;
    std::string DeviceManager::get_udid(AMDeviceRef deviceHandle)
    {
        string udid;
        CFStringRef found_device_id = AMDeviceCopyDeviceIdentifier(deviceHandle);
        auto len = CFStringGetLength(found_device_id);
        udid.resize(len);
        CFStringGetCString(found_device_id, (char*)udid.c_str(), len + 1, kCFStringEncodingUTF8);
        CFRelease(found_device_id);
        return udid;
    }
    ConnectMode DeviceManager::get_connect_mode(AMDeviceRef deviceHandle)
    {
		//根据deviceHandle 获取连接方式
        return ConnectMode(AMDeviceGetInterfaceType(deviceHandle));
    }
    std::shared_ptr<class iOSDevice> DeviceManager::get_device(AMDeviceRef deviceHandle) {
        return std::make_shared<iOSDevice>(deviceHandle);  
    }  

    void DeviceManager::add_device(const std::string& udid, AMDeviceRef deviceHandle) {  
        m_udid[udid] = deviceHandle;  
    }  

    void DeviceManager::remove_device(const std::string& udid) {  
        m_udid.erase(udid);  
    }
    AMDeviceRef DeviceManager::get_handle(const std::string& udid)
    {
		//根据udid 获取handle
        auto iter = m_udid.find(udid);
        if (iter != m_udid.end()) {
            return iter->second;
        } else {
            return nullptr;
        }
    }
    std::shared_ptr<class iOSDevice> DeviceManager::get_device(const std::string& udid) {
        auto iter = m_udid.find(udid);
        if (iter == m_udid.end()) {
            return nullptr;
        }
        else {
            return get_device(iter->second);
        }
    }
}
