#include "DeviceManager.h"  
#include "iOSDevice.h"  
#include <string>  

namespace aid2 {  
	std::map<std::string, am_device*> aid2::DeviceManager::m_udid;

   std::shared_ptr<class iOSDevice> DeviceManager::get_device(AMDeviceRef deviceHandle) {
       return std::make_shared<iOSDevice>(deviceHandle);  
   }  

   std::shared_ptr<class iOSDevice> DeviceManager::get_device(const std::string& udid) {
       auto iter = m_udid.find(udid);  
       if (iter == m_udid.end()) {  
           return nullptr;  
       } else {  
           return get_device(iter->second);  
       }  
   }  

   void DeviceManager::add_device(const std::string& udid, AMDeviceRef deviceHandle) {  
       m_udid[udid] = deviceHandle;  
   }  

   void DeviceManager::remove_device(const std::string& udid) {  
       m_udid.erase(udid);  
   }
   void DeviceManager::remove_device(AMDeviceRef deviceHandle)
   {
	   for (auto it = m_udid.begin(); it != m_udid.end(); ) {
		   if (it->second == deviceHandle) {
			   it = m_udid.erase(it);
		   }
		   else {
			   ++it;
		   }
	   }
   }

   std::string DeviceManager::get_udid(AMDeviceRef deviceHandle)
   {
	   for (auto it = m_udid.begin(); it != m_udid.end(); ) {
		   if (it->second == deviceHandle) {
			   //�ҵ���ͬ������udid
			   return it->first;
		   }
		   else {
			   ++it;
		   }
	   }
	   // δ�ҵ�ʱ���ؿ��ַ���
	   return std::string();
   }
   AMDeviceRef DeviceManager::get_handle(const std::string& udid)
   {
		//����udid ��ȡhandle
        auto iter = m_udid.find(udid);
        if (iter != m_udid.end()) {
            return iter->second;
        } else {
            return nullptr;
        }
   }
}
