#ifndef ABI_IOS_AUTHORIZE_ITUNES_MODULE_H_
#define ABI_IOS_AUTHORIZE_ITUNES_MODULE_H_
//////////////////////////////////////////////////////////////////////////
#include <string>
//////////////////////////////////////////////////////////////////////////
namespace ABI{
	namespace iTunes{
		class iTunesModule
		{
		public:
			iTunesModule(void);
			~iTunesModule(void);
			std::wstring iTunesDll(const std::wstring& dll_name);
			std::wstring iTunesSCInfo();
			std::wstring core_foundation_dll() const{
				const std::wstring data = core_foundation_dll_;
				return data;
			}
			std::wstring air_traffic_host_dll() const{
				const std::wstring data = air_traffic_host_dll_;
				return data;
			}
			std::wstring asmapi_interface_dll() const{
				const std::wstring data = asmapi_interface_dll_;
				return data;
			}
			std::wstring itunes_mobile_device_dll() const{
				const std::wstring data = itunes_mobile_device_dll_;
				return data;
			}
			std::wstring mobile_device_dll() const{
				const std::wstring data = mobile_device_dll_;
				return data;
			}
		private:
			void iTunesPathInit();
			void AddEnvironmentVariable(const std::wstring& path);
			std::wstring iTunesGetSharedDll(const std::wstring dll_name);
			std::wstring iTunesFrameworkSupport(const std::wstring dll_name,bool env_able);
			std::wstring iTunesWinPath();
			std::wstring GetSpecialPath(const unsigned long& type_path,const std::wstring& name);
			void set_core_foundation_dll(std::wstring path){
				core_foundation_dll_.resize(0);
				core_foundation_dll_ = path;
			}
			void set_air_traffic_host_dll(std::wstring path){
				air_traffic_host_dll_.resize(0);
				air_traffic_host_dll_ = path;
			}
			void set_asmapi_interface_dll(std::wstring path){
				asmapi_interface_dll_.resize(0);
				asmapi_interface_dll_ = path;
			}
			void set_itunes_mobile_device_dll(std::wstring path){
				itunes_mobile_device_dll_.resize(0);
				itunes_mobile_device_dll_ = path;
			}
			void set_mobile_device_dll(std::wstring path){
				mobile_device_dll_.resize(0);
				mobile_device_dll_ = path;
			}
			std::wstring core_foundation_dll_;
			std::wstring air_traffic_host_dll_;
			std::wstring asmapi_interface_dll_;
			std::wstring itunes_mobile_device_dll_;
			std::wstring mobile_device_dll_;
		};
	}
}
//////////////////////////////////////////////////////////////////////////
#endif