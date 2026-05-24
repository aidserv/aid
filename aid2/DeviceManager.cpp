#include "DeviceManager.h"  
#include "iOSDevice.h"  
#include <string>  
#include <mutex>
#include <map>

namespace aid2 {  
    std::map<std::string, AMDeviceRef> DeviceManager::m_udid;
    std::mutex DeviceManager::m_mutex;
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
        std::lock_guard<std::mutex> lk(m_mutex);
        m_udid[udid] = deviceHandle;  
    }  

    void DeviceManager::remove_device(const std::string& udid) {  
        std::lock_guard<std::mutex> lk(m_mutex);
        m_udid.erase(udid);  
    }
    AMDeviceRef DeviceManager::get_handle(const std::string& udid)
    {
		//根据udid 获取handle
        std::lock_guard<std::mutex> lk(m_mutex);
        auto iter = m_udid.find(udid);
        if (iter != m_udid.end()) {
            return iter->second;
        } else {
            return nullptr;
        }
    }
    std::shared_ptr<class iOSDevice> DeviceManager::get_device(const std::string& udid) {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto iter = m_udid.find(udid);
        if (iter == m_udid.end()) {
            return nullptr;
        }
        else {
            return get_device(iter->second);
        }
    }
}
