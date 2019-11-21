#include "base/windows_hardware.h"
#include <algorithm>
#include <WinSock2.h>
#include <IPHlpApi.h>
#include "base/basetypes.h"
#include "base/windows_version.h"
#include "base/string/string_split.h"

namespace ABI{
	namespace base{
		HardwareInfo::HardwareInfo(){
				set_cookie("");
				std::string adapter_info = "";
				std::string volume_serial = "";
				std::string bois_info = "";
				std::string processor_name = "";
				std::string product_id = "";
				std::string computer_name = "";
				GetAdapterSerial(adapter_info);
				std::string guid_1 = base::Md5(adapter_info.c_str(),adapter_info.length(),8);
				GetVolumeSerial(volume_serial);
				std::string guid_2 = base::Md5(volume_serial.c_str(),volume_serial.length(),8);
				GetSystemBios(bois_info);
				std::string guid_3 = base::Md5(bois_info.c_str(),bois_info.length(),8);
				GetProcessorName(processor_name);
				std::string guid_4 = base::Md5(processor_name.c_str(),processor_name.length(),8);
				GetWinProductId(product_id);
				std::string guid_5 = base::Md5(product_id.c_str(),product_id.length(),8);
				GetWinComputerName(computer_name);
				std::string guid_6 = base::Md5(computer_name.c_str(),computer_name.length(),8);
				std::wstring hw_profile = HwProfile();
				std::string guid_7 = base::Md5(hw_profile.c_str(),hw_profile.length()*sizeof(wchar_t),8);
				std::string guid = guid_1+"."+guid_2+"."+guid_5+"."+guid_4+"."+guid_3+"."+guid_6+"."+guid_7;
				std::transform(guid.begin(),guid.end(),guid.begin(),::toupper);
				set_cookie(guid);
			}
			HardwareInfo::~HardwareInfo(){
				set_cookie("");
			}
			bool HardwareInfo::GetAdapterSerial(std::string& out){
				ULONG SizePointer;
				base::WinVersion version;
				if(!version.IsWin8()&&!version.IsWin8_1()){
					SizePointer = sizeof(IP_ADAPTER_INFO);
					PIP_ADAPTER_INFO dummy_info = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(sizeof(IP_ADAPTER_INFO)));
					PIP_ADAPTER_INFO adapter_info = dummy_info;
					memset(adapter_info,0,sizeof(IP_ADAPTER_INFO));
					if(!dummy_info){
						return false;
					}
					if(GetAdaptersInfo(dummy_info,&SizePointer)==ERROR_BUFFER_OVERFLOW){
						free(dummy_info);
						adapter_info = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(SizePointer));
						memset(adapter_info,0,SizePointer);
						if(!adapter_info){
							return false;
						}
					}
					GetAdaptersInfo(adapter_info,&SizePointer);
					for(unsigned int i=0;i<6;i++){
						if(i >= adapter_info->AddressLength){
							break;
						}
						out.append(1,adapter_info->Address[i]);
					}
					free(adapter_info);
					return true;
				}
				else{
					SizePointer = 0;
					if(GetAdaptersAddresses(0,GAA_FLAG_INCLUDE_ALL_INTERFACES,NULL,NULL,&SizePointer)!= ERROR_BUFFER_OVERFLOW){
						return false;
					}
					PIP_ADAPTER_ADDRESSES dummy_info = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(SizePointer));
					PIP_ADAPTER_ADDRESSES adapter_info = dummy_info;
					if(!dummy_info || GetAdaptersAddresses(0,GAA_FLAG_INCLUDE_ALL_INTERFACES,0,dummy_info,&SizePointer)){
						return false;
					}
					for(unsigned int i=0;i<6;i++){
						if(i >= adapter_info->PhysicalAddressLength){
							break;
						}
						out.append(1,adapter_info->PhysicalAddress[i++]);
					}
					free(adapter_info);
					return true;
				}
			}
			bool HardwareInfo::GetVolumeSerial(std::string& out){
				unsigned long VolumeSerialNumber = 0;
				GetVolumeInformationW(L"C:\\", 0, 0, &VolumeSerialNumber, 0, 0, 0, 0);
				out.append(reinterpret_cast<char*>(&VolumeSerialNumber),4);
				return true;
			}
			bool HardwareInfo::GetSystemBios(std::string& out){
				HKEY phkResult = NULL;
				DWORD cbData = 0;
				RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System",0,KEY_READ,&phkResult);
				if(!RegQueryValueExA(phkResult,"SystemBiosVersion",0,0,0,&cbData)){
					char* v7 = new char[cbData];
					if(!RegQueryValueExA(phkResult,"SystemBiosVersion",0,0,reinterpret_cast<unsigned char*>(v7),&cbData) ){
						out.append(v7,cbData);
					}
					delete []v7;
				}
				RegCloseKey(phkResult);
				return true;
			}
			bool HardwareInfo::GetProcessorName(std::string& out){
				HKEY phkResult = NULL;
				DWORD cbData = 0;
				RegOpenKeyExA(HKEY_LOCAL_MACHINE,"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",0,KEY_READ,&phkResult);
				if(!RegQueryValueExA(phkResult,"ProcessorNameString",0,0,0,&cbData)){
					char* v7 = new char[cbData];
					if(!RegQueryValueExA(phkResult,"ProcessorNameString",0,0,reinterpret_cast<unsigned char*>(v7),&cbData)){
						out.append(v7,cbData);
					}
					delete [] v7;
				}
				RegCloseKey(phkResult);
				return true;
			}
			bool HardwareInfo::GetWinProductId(std::string& out){
				HKEY phkResult = NULL;
				DWORD cbData = 0;
				char* reg_name = NULL;
				OSVERSIONINFOEXW version_information = {0};
				version_information.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
				version_information.dwMajorVersion = 6;
				version_information.dwMinorVersion = 0;
				unsigned long long condition_mask = VerSetConditionMask(VerSetConditionMask(0,VER_MAJORVERSION,VER_GREATER_EQUAL),VER_MINORVERSION,VER_GREATER_EQUAL);
				if(VerifyVersionInfoW(&version_information,VER_MAJORVERSION|VER_MINORVERSION,condition_mask)){
					reg_name = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
				}
				else{
					reg_name = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
				}
				RegOpenKeyExA(HKEY_LOCAL_MACHINE,reg_name,0,KEY_READ,&phkResult);
				if(!RegQueryValueExA(phkResult,"ProductId",0,0,0,&cbData)){
					char* v7 = new char[cbData];
					if(!RegQueryValueExA(phkResult,"ProductId",0,0,reinterpret_cast<unsigned char*>(v7),&cbData)){
						out.append(v7,cbData);
					}
					delete []v7;
				}
				RegCloseKey(phkResult);
				return true;
			}
			bool HardwareInfo::GetWinComputerName(std::string& out){
				wchar_t buffer[MAX_PATH] = {0};
				unsigned long length = MAX_PATH;
				GetComputerNameW(buffer,&length);
				out.resize(length);
				memmove(const_cast<char*>(out.c_str()),buffer,length);
				return true;
			}
			std::string HardwareInfo::GetMachineName(){
				std::string machine_name = "";
				GetWinComputerName(machine_name);
				return machine_name;
			}
			std::wstring HardwareInfo::HwProfile(){
 				/*regedit path HKLM\System\CurrentControlSet\Control\IDConfigDB\Hardware Profiles
 				reference:http://stackoverflow.com/questions/3263622/uniquely-identify-a-computer-by-its-hardware-profile-getcurrenthwprofile*/
				HW_PROFILE_INFOW hw_profile_info = {0};
				GetCurrentHwProfileW(reinterpret_cast<LPHW_PROFILE_INFOW>(&hw_profile_info));
				return (std::wstring(hw_profile_info.szHwProfileGuid));
			}
			std::string HardwareInfo::cookie() const{
				return cookie_;
			}
			void HardwareInfo::set_cookie(const std::string hardware_cookie){
				cookie_ = hardware_cookie;
			}
			std::string GetHardwareCookie(){
				HardwareInfo hardware;
				return hardware.cookie();
			}
	}
}