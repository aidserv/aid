#pragma once
#include <string>
#include <memory>
#include <map>  
#include "iTunesApi/simpleApi.h"
namespace aid2 {
    class DeviceManager {
    public:
		// 获取设备类实例
        static std::shared_ptr<class iOSDevice> get_device(AMDeviceRef deviceHandle);
		// 根据udid获取设备类实例,如果没有找到返回nullptr
        static std::shared_ptr<class iOSDevice> get_device(const std::string& udid);

        // 新增：管理udid和设备句柄的映射
        static void add_device(const std::string& udid, AMDeviceRef deviceHandle);
		// 删除：根据udid删除映射
        static void remove_device(const std::string& udid);
		// 删除：根据deviceHandle删除映射
        static void remove_device(AMDeviceRef deviceHandle);

		//根据deviceHandle从map获取udid
        static std::string get_udid(AMDeviceRef deviceHandle);
        //根据udid从map获取deviceHandle
        static AMDeviceRef get_handle(const std::string& udid);
    private:
        static std::map<std::string, AMDeviceRef> m_udid;
    };
}