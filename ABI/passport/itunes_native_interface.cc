#include "passport/itunes_native_interface.h"
#include <ImageHlp.h>
#pragma comment(lib,"Imagehlp.lib")
#include "passport/itunes_internal_interface.h"
#include "base/string/string_conv.h"
#include "ABI/ios_authorize/apple_import.h"
#pragma comment(lib,"Version.lib")

namespace passport{
	namespace internal{
		void AddEnvironmentVariable(const std::wstring& path){
			wchar_t env_path[4096] = {0};
			GetEnvironmentVariableW(L"PATH",env_path,4095);
			std::wstring new_env_path = (std::wstring(env_path) + std::wstring(L";")) + path;
			size_t posiltion = new_env_path.find_last_of('\\');
			if(posiltion!=std::wstring::npos){
				new_env_path[posiltion] = 0;
			}
			SetEnvironmentVariableW(L"PATH",new_env_path.c_str());
		}
		std::wstring GetSoftwareReleaseVersion(const wchar_t* full_path){
			VS_FIXEDFILEINFO *pVerInfo = NULL;
			DWORD dwTemp, dwSize;
			BYTE *pData = NULL;
			UINT uLen;
			dwSize = GetFileVersionInfoSizeW(full_path, &dwTemp);
			if(dwSize == 0){
				return L"";
			}
			pData = new BYTE[dwSize+1];
			if(pData == NULL){
				return L"";
			}
			if(!GetFileVersionInfoW(full_path, 0, dwSize, pData)){
				delete[] pData;
				return L"";
			}
			if(!VerQueryValueW(pData,L"\\",(void **)&pVerInfo,&uLen)){
				delete[] pData;
				return L"";
			}
			DWORD verMS = pVerInfo->dwFileVersionMS;
			DWORD verLS = pVerInfo->dwFileVersionLS;
			DWORD major = HIWORD(verMS);
			DWORD minor = LOWORD(verMS);
			DWORD build = HIWORD(verLS);
			DWORD revision = LOWORD(verLS);
			delete[] pData;
			wchar_t version[1024] = {0};
			_snwprintf(version,1024,L"%d.%d.%d.%d",major,minor,build,revision);
			return version;
		}
		std::wstring GetAppleMobileDeviceSupportDll(const std::wstring dll_name){
			void* hSetting = NULL;
			unsigned long length = 0;
			wchar_t* path = new wchar_t[MAX_PATH*sizeof(wchar_t)];
			if(path==NULL){
				return L"";
			}
			if(::RegCreateKeyW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support\\Shared",reinterpret_cast<PHKEY>(&hSetting))!=ERROR_SUCCESS){
				return L"";
			}
			if(::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),dll_name.c_str(), NULL, NULL, NULL, &length)!=ERROR_SUCCESS){
				return L"";
			}
			::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),dll_name.c_str(), NULL, NULL, (LPBYTE)path,&length);
			::RegCloseKey(reinterpret_cast<HKEY>(hSetting));
			std::wstring shared_dll(path);
			delete[] path;
			return shared_dll;
		}
		std::wstring GetAppleApplicationSupportDll(const std::wstring dll_name){
			void* hSetting = NULL;
			unsigned long length = 0;
			wchar_t* pCoreFoundationPath = new wchar_t[MAX_PATH*sizeof(wchar_t)];
			if(pCoreFoundationPath==NULL){
				return L"";
			}
			if(::RegCreateKeyW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Apple Inc.\\Apple Application Support",reinterpret_cast<PHKEY>(&hSetting))!=ERROR_SUCCESS){
				return L"";
			}
			if(::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),L"InstallDir", NULL, NULL, NULL, &length)!=ERROR_SUCCESS){
				return L"";
			}
			::RegQueryValueExW(reinterpret_cast<HKEY>(hSetting),L"InstallDir", NULL, NULL, (LPBYTE)pCoreFoundationPath, &length);
			::RegCloseKey(reinterpret_cast<HKEY>(hSetting));
			if(pCoreFoundationPath[wcslen(pCoreFoundationPath)-1] != '\\'){
				wcscat(pCoreFoundationPath,L"\\");
			}
			if(dll_name.length()>0){
				wcscat(pCoreFoundationPath,dll_name.c_str());
			}
			std::wstring support_dll(pCoreFoundationPath);
			delete[] pCoreFoundationPath;
			return support_dll;
		}
		std::wstring GetDirectory(){
			wchar_t buffer[MAX_PATH] = {0};
			wchar_t drive[_MAX_DRIVE] = {0};
			wchar_t dir[_MAX_DIR] = {0};
			wchar_t fname[_MAX_FNAME] = {0};
			wchar_t ext[_MAX_EXT] = {0};
			GetModuleFileNameW(NULL,buffer,MAX_PATH);
			_wsplitpath_s(buffer,drive,_MAX_DRIVE,dir,_MAX_DIR,fname,_MAX_FNAME,ext,_MAX_EXT);
			return (std::wstring(std::wstring(drive)+std::wstring(dir)));
		}
		std::wstring GetITunesInstallDll(const std::wstring dll_name){
			const std::wstring dll = GetDirectory().append(L"itunes.dll");
			//date:2015/09/06
			if(PathFileExistsW(dll.c_str()))
				return GetDirectory().append(dll_name);//load current directory itunes.dll
			wchar_t path[MAX_PATH] = { 0 };
			SHGetSpecialFolderPathW(NULL, path, CSIDL_PROGRAM_FILES, FALSE);
			return (std::wstring(path) + std::wstring(L"\\iTunes\\") + dll_name);
		}
	}
	iTunesNativeInterface* iTunesNativeInterface::GetInstance(){
		static iTunesNativeInterface* info;
		if(!info){
			iTunesNativeInterface* new_info = new iTunesNativeInterface();
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
				delete new_info;
			}
			const std::wstring corefp_dll = internal::GetDirectory().append(L"CoreFP.dll");
			const std::wstring itunesmobiledevice_dll = internal::GetDirectory().append(L"iTunesMobileDevice.dll");
			if (PathFileExistsW(corefp_dll.c_str()))
				new_info->HKLMCustomizeModule(CustomizeModule::kCoreFP, corefp_dll.c_str());
			if (PathFileExistsW(itunesmobiledevice_dll.c_str()))
				new_info->HKLMCustomizeModule(CustomizeModule::kiTunesMobileDeviceDLL, itunesmobiledevice_dll.c_str());
		}
		return info;
	}
	iTunesNativeInterface::iTunesNativeInterface(void){
		internal::AddEnvironmentVariable(internal::GetAppleApplicationSupportDll(L""));
		internal::AddEnvironmentVariable(internal::GetITunesInstallDll(L""));
		std::wstring directory = internal::GetAppleMobileDeviceSupportDll(L"AirTrafficHostDLL");
		if(directory.length()>0){
			directory[directory.find_last_of(L"\\")+1] = 0;
			internal::AddEnvironmentVariable(directory);
		}
	}
	iTunesNativeInterface::~iTunesNativeInterface(void){

	}
	bool iTunesNativeInterface::HKLMCustomizeModule(const CustomizeModule& customize_module, const wchar_t* module_name){
		std::wstring sub_key = L"";
		std::wstring sub_key_name = L"";
		if (customize_module == CustomizeModule::kCoreFP){
			sub_key = L"SOFTWARE\\Wow6432Node\\Apple Inc.\\CoreFP";
			sub_key_name = L"LibraryPath";
		}
		else if (customize_module == CustomizeModule::kiTunesMobileDeviceDLL){
			sub_key = L"SOFTWARE\\Wow6432Node\\Apple Inc.\\Apple Mobile Device Support\\Shared";
			sub_key_name = L"iTunesMobileDeviceDLL";
		}
		else{
			return true;
		}
		HKEY h_setting = NULL;
		bool is_success = true;
		if (::RegCreateKeyW(HKEY_LOCAL_MACHINE, sub_key.c_str(), reinterpret_cast<PHKEY>(&h_setting)) != ERROR_SUCCESS)
			return is_success;
		if (::RegSetValueExW(h_setting, 
			sub_key_name.c_str(),
			0, 
			REG_SZ, 
			(const BYTE*)module_name, 
			wcslen(module_name)*sizeof(wchar_t)) == ERROR_SUCCESS)
			is_success = false;
		RegCloseKey(h_setting);
		return is_success;
	}
	void iTunesNativeInterface::Init(){
		unsigned long Kbsync = 0;
		unsigned long CigHash = 0;
		unsigned long KbsyncID = 0;
		unsigned long WriteSIDD = 0;
		unsigned long WriteSIDB = 0;
		unsigned long DeAuthSIDB = 0;
		unsigned long CalcUnkP1 = 0;
		unsigned long SetAFSyncRQ = 0;
		unsigned long PreAuthByDSID = 0;
		unsigned long VerifyAFSyncRQ = 0;
		unsigned long GenerateAFSyncRS = 0;
		unsigned long SapInit = 0;
		unsigned long SapGetP1 = 0;
		unsigned long SapCalcBuffer = 0;
		unsigned long SapGetAS = 0;
		unsigned long SapGetASFD = 0;
		unsigned long SapGetASFD_a = 0;
		unsigned long GetCltDat = 0;
		unsigned long TranSetInf = 0;
		unsigned long UpdCDID = 0;
		unsigned long GetMD = 0;
		unsigned long InitHost = 0;
		unsigned long EstablishKey = 0;
		LoadDlls();
		const std::wstring itunes_dll = internal::GetITunesInstallDll(L"iTunes.dll");
		const std::wstring air_traffic_host_dll = internal::GetAppleMobileDeviceSupportDll(L"AirTrafficHostDLL");
		const HMODULE itunes_base = LoadLibraryW(itunes_dll.c_str());
		const HMODULE air_traffic_host_base = LoadLibraryW(air_traffic_host_dll.c_str());
		if(iTunesDllVersion(L"10.5.0.142")){
			Kbsync = 0xA42370u;
			KbsyncID = 0x9CF4D0u;
			CigHash = 0xA819C0u;
			WriteSIDD = 0x9FFCD0u;
			WriteSIDB = 0x9F9A40u;
			DeAuthSIDB = 0x9FCB60u;
			CalcUnkP1 = 0xA04CB0u;
			SetAFSyncRQ = 0xA09AB0u;
			PreAuthByDSID = 0xA0C820u;
			VerifyAFSyncRQ = 0xA83F50u;
			GenerateAFSyncRS = 0xA0E9E0u;
			const wchar_t* http_ua = L"iTunes/10.5 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.51.22";
		}
		else if(iTunesDllVersion(L"10.5.1.42")){
			Kbsync = 10875760;
			KbsyncID = 10405072;
			CigHash = 11135424;
			WriteSIDD = 10603728;
			WriteSIDB = 10578496;
			DeAuthSIDB = 10591072;
			CalcUnkP1 = 10624176;
			SetAFSyncRQ = 10644144;
			PreAuthByDSID = 10655776;
			VerifyAFSyncRQ = 11145040;
			GenerateAFSyncRS = 10664416;
			const wchar_t* http_ua = L"iTunes/10.5.1 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.51.22";
		}
		else if(iTunesDllVersion(L"10.5.2.11")){
			Kbsync = 10888048;
			KbsyncID = 10417360;
			CigHash = 11147712;
			WriteSIDD = 10616016;
			WriteSIDB = 10590784;
			DeAuthSIDB = 10603360;
			CalcUnkP1 = 10636464;
			SetAFSyncRQ = 10656432;
			PreAuthByDSID = 10668064;
			VerifyAFSyncRQ = 11157328;
			GenerateAFSyncRS = 10676704;
			const wchar_t* http_ua = L"iTunes/10.5.2 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.52.7";
		}
		else if(iTunesDllVersion(L"10.5.3.3")){
			Kbsync = 10892144;
			KbsyncID = 10421456;
			CigHash = 11151808;
			WriteSIDD = 10620112;
			WriteSIDB = 10594880;
			DeAuthSIDB = 10607456;
			CalcUnkP1 = 10640560;
			SetAFSyncRQ = 10660528;
			PreAuthByDSID = 10672160;
			VerifyAFSyncRQ = 11161424;
			GenerateAFSyncRS = 10680800;
			const wchar_t* http_ua = L"iTunes/10.5.3 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.52.7";
		}
		else if(iTunesDllVersion(L"10.6.0.40")){
			Kbsync = 10928896;
			KbsyncID = 10474704;
			CigHash = 11178624;
			WriteSIDD = 10666112;
			WriteSIDB = 10642064;
			DeAuthSIDB = 10653504;
			CalcUnkP1 = 10686160;
			SetAFSyncRQ = 10705648;
			PreAuthByDSID = 10716816;
			VerifyAFSyncRQ = 11188416;
			GenerateAFSyncRS = 10724688;
			const wchar_t* http_ua = L"iTunes/10.6 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.54.16";
		}
		else if(iTunesDllVersion(L"10.6.1.7")){
			Kbsync = 10928896;
			KbsyncID = 10474704;
			CigHash = 11178624;
			WriteSIDD = 10666112;
			WriteSIDB = 10642064;
			DeAuthSIDB = 10653504;
			CalcUnkP1 = 10686160;
			SetAFSyncRQ = 10705648;
			PreAuthByDSID = 10716816;
			VerifyAFSyncRQ = 11188416;
			GenerateAFSyncRS = 10724688;
			const wchar_t* http_ua = L"iTunes/10.6.1 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.54.16";
		}
		else if(iTunesDllVersion(L"10.6.3.25")){
			Kbsync = 10976432;
			KbsyncID = 10503376;
			CigHash = 11247136;
			WriteSIDD = 10703744;
			WriteSIDB = 10678496;
			DeAuthSIDB = 10690928;
			CalcUnkP1 = 10723984;
			SetAFSyncRQ = 10743552;
			PreAuthByDSID = 10756000;
			VerifyAFSyncRQ = 11257312;
			GenerateAFSyncRS = 10764240;
			const wchar_t* http_ua = L"iTunes/10.6.3 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/534.57.2";
		}
		else if(iTunesDllVersion(L"10.7.0.21")){
			Kbsync = 10995968;
			KbsyncID = 10507472;
			CigHash = 11267056;
			WriteSIDD = 10714416;
			WriteSIDB = 10689232;
			DeAuthSIDB = 10702080;
			CalcUnkP1 = 10734864;
			SetAFSyncRQ = 10754320;
			PreAuthByDSID = 10766608;
			VerifyAFSyncRQ = 11276512;
			GenerateAFSyncRS = 10774672;
			const wchar_t* http_ua = L"iTunes/10.7 (Windows; Microsoft Windows XP Professional Service Pack 3 (Build 2600)) AppleWebKit/536.26.9";
		}
		else if(iTunesDllVersion(L"11.0.0.163")){
			Kbsync = 14234208;
			KbsyncID = 14060768;
			CigHash = 13942656;
			WriteSIDD = 14076960;
			WriteSIDB = 14068240;
			DeAuthSIDB = 13940416;
			CalcUnkP1 = 14013904;
			SetAFSyncRQ = 14019264;
			PreAuthByDSID = 14024960;
			VerifyAFSyncRQ = 13950688;
			GenerateAFSyncRS = 14022688;
			const wchar_t* http_ua = L"iTunes/11.0 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.27.1";
		}
		else if(iTunesDllVersion(L"11.0.1.12")){
			Kbsync = 14279264;
			KbsyncID = 14105824;
			CigHash = 13987712;
			WriteSIDD = 14122016;
			WriteSIDB = 14113296;
			DeAuthSIDB = 13985472;
			CalcUnkP1 = 14058960;
			SetAFSyncRQ = 14064320;
			PreAuthByDSID = 14070016;
			VerifyAFSyncRQ = 13995744;
			GenerateAFSyncRS = 14067744;
			const wchar_t* http_ua = L"iTunes/11.0.1 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.27.1";
		}
		else if(iTunesDllVersion(L"11.0.2.25")){
			Kbsync = 14301600;
			KbsyncID = 14196416;
			CigHash = 14174096;
			WriteSIDD = 14288320;
			WriteSIDB = 14286000;
			DeAuthSIDB = 14196224;
			CalcUnkP1 = 14234512;
			SetAFSyncRQ = 14238736;
			PreAuthByDSID = 14243648;
			VerifyAFSyncRQ = 14178944;
			GenerateAFSyncRS = 14241536;
			const wchar_t* http_ua = L"iTunes/11.0.2 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.27.1";
		}
		else if(iTunesDllVersion(L"11.0.3.42")||iTunesDllVersion(L"11.0.4.4")){
			GenerateAFSyncRS = 14573520;
			VerifyAFSyncRQ = 14468208;
			PreAuthByDSID = 14576880;
			SetAFSyncRQ = 14571568;
			CalcUnkP1 = 14567216;
			DeAuthSIDB = 14646368;
			WriteSIDB = 14630048;
			WriteSIDD = 14549104;
			CigHash = 14463184;
			KbsyncID = 14610896;
			Kbsync = 14638080;
			const wchar_t* http_ua = L"iTunes/11.0.3 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.30.1";
			const wchar_t* http_ua_1 = L"iTunes/11.0.4 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.30.1";
		}
		else if(iTunesDllVersion(L"11.0.5.5")){
			Kbsync = 14642176;
			KbsyncID = 14614992;
			CigHash = 14467280;
			WriteSIDD = 14553200;
			WriteSIDB = 14634144;
			DeAuthSIDB = 14650464;
			CalcUnkP1 = 14571312;
			SetAFSyncRQ = 14575664;
			PreAuthByDSID = 14580976;
			VerifyAFSyncRQ = 14472304;
			GenerateAFSyncRS = 14577616;
			SapInit = 14621984;
			SapGetP1 = 14440000;
			SapCalcBuffer = 14443568;
			SapGetAS = 14455408;
			SapGetASFD = 14451616;
			const wchar_t* http_ua = L"iTunes/11.0.5 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.30.1";
		}
		else if(iTunesDllVersion(L"11.1.0.126")){
			Kbsync = 15982240;
			KbsyncID = 15896736;
			CigHash = 15885312;
			WriteSIDD = 15907264;
			WriteSIDB = 15901904;
			DeAuthSIDB = 15933952;
			CalcUnkP1 = 15945744;
			SetAFSyncRQ = 15948752;
			PreAuthByDSID = 15952096;
			VerifyAFSyncRQ = 15888160;
			GenerateAFSyncRS = 15950608;
			SapInit = 15900192;
			SapGetP1 = 15961696;
			SapCalcBuffer = 15962784;
			SapGetAS = 15900272;
			SapGetASFD = 15966256;
		}
		else if(iTunesDllVersion(L"11.1.1.11")){
			Kbsync = 15986336;
			KbsyncID = 15900832;
			CigHash = 15889408;
			WriteSIDD = 15911360;
			WriteSIDB = 15906000;
			DeAuthSIDB = 15938048;
			CalcUnkP1 = 15949840;
			SetAFSyncRQ = 15952848;
			PreAuthByDSID = 15956192;
			VerifyAFSyncRQ = 15892256;
			GenerateAFSyncRS = 15954704;
			SapInit = 15904288;
			SapGetP1 = 15965792;
			SapCalcBuffer = 15966880;
			SapGetAS = 15904368;
			SapGetASFD = 15970352;
		}
		else if(iTunesDllVersion(L"11.1.2.31")||iTunesDllVersion(L"11.1.2.32")||iTunesDllVersion(L"11.1.3.8")){
			Kbsync = 16084640;
			KbsyncID = 15999136;
			CigHash = 15987712;
			WriteSIDD = 16009664;
			WriteSIDB = 16004304;
			DeAuthSIDB = 16036352;
			CalcUnkP1 = 16048144;
			SetAFSyncRQ = 16051152;
			PreAuthByDSID = 16054496;
			VerifyAFSyncRQ = 15990560;
			GenerateAFSyncRS = 16053008;
			SapInit = 16002592;
			SapGetP1 = 16064096;
			SapCalcBuffer = 16065184;
			SapGetAS = 16002672;
			SapGetASFD = 16068656;
		}
		else if(iTunesDllVersion(L"11.1.4.62")||iTunesDllVersion(L"11.1.5.5")){
			Kbsync = 16125600;
			KbsyncID = 16040096;
			CigHash = 16028672;
			WriteSIDD = 16050624;
			WriteSIDB = 16045264;
			DeAuthSIDB = 16077312;
			CalcUnkP1 = 16089104;
			SetAFSyncRQ = 16092112;
			PreAuthByDSID = 16095456;
			VerifyAFSyncRQ = 16031520;
			GenerateAFSyncRS = 16093968;
			SapInit = 16043552;
			SapGetP1 = 16105056;
			SapCalcBuffer = 16106144;
			SapGetAS = 16043632;
			SapGetASFD = 16109616;
		}
		else if(iTunesDllVersion(L"11.2.0.114")||iTunesDllVersion(L"11.2.0.115")){
			Kbsync = 16617200;
			KbsyncID = 16536528;
			CigHash = 16525104;
			WriteSIDD = 16547072;
			WriteSIDB = 16541696;
			DeAuthSIDB = 16573760;
			CalcUnkP1 = 16585568;
			SetAFSyncRQ = 16588576;
			PreAuthByDSID = 16591920;
			VerifyAFSyncRQ = 16527952;
			GenerateAFSyncRS = 16590432;
			SapInit = 16539984;
			SapGetP1 = 16513040;
			SapCalcBuffer = 16514160;
			SapGetAS = 16540064;
			SapGetASFD = 16601216;
		}
		else if(iTunesDllVersion(L"11.2.2.3")){
			Kbsync = 16621296;
			KbsyncID = 16540624;
			CigHash = 16529200;
			WriteSIDD = 16551168;
			WriteSIDB = 16545792;
			DeAuthSIDB = 16577856;
			CalcUnkP1 = 16589664;
			SetAFSyncRQ = 16592672;
			PreAuthByDSID = 16596016;
			VerifyAFSyncRQ = 16532048;
			GenerateAFSyncRS = 16594528;
			SapInit = 16544080;
			SapGetP1 = 16517136;
			SapCalcBuffer = 16518256;
			SapGetAS = 16544160;
			SapGetASFD = 16605312;
		}
		else if(iTunesDllVersion(L"11.3.0.54")){
			Kbsync = 16690928;
			KbsyncID = 16610256;
			CigHash = 16598832;
			WriteSIDD = 16620800;
			WriteSIDB = 16615424;
			DeAuthSIDB = 16647488;
			CalcUnkP1 = 16659296;
			SetAFSyncRQ = 16662304;
			PreAuthByDSID = 16665648;
			VerifyAFSyncRQ = 16601680;
			GenerateAFSyncRS = 16664160;
			SapInit = 16613712;
			SapGetP1 = 16586768;
			SapCalcBuffer = 16587888;
			SapGetAS = 16613792;
			SapGetASFD = 16674944;
		}
		else if(iTunesDllVersion(L"11.3.1.2")){
			Kbsync = 16695024;
			KbsyncID = 16614352;
			CigHash = 16602928;
			WriteSIDD = 16624896;
			WriteSIDB = 16619520;
			DeAuthSIDB = 16651584;
			CalcUnkP1 = 16663392;
			SetAFSyncRQ = 16666400;
			PreAuthByDSID = 16669744;
			VerifyAFSyncRQ = 16605776;
			GenerateAFSyncRS = 16668256;
			SapInit = 16617808;
			SapGetP1 = 16590864;
			SapCalcBuffer = 16591984;
			SapGetAS = 16617888;
			SapGetASFD = 16679040;
			SapGetASFD_a = 16680320;
		}
		else if(iTunesDllVersion(L"11.4.0.18")){
			Kbsync = 16682736;
			KbsyncID = 16602064;
			CigHash = 16590640;
			WriteSIDD = 16612608;
			WriteSIDB = 16607232;
			DeAuthSIDB = 16639296;
			CalcUnkP1 = 16651104;
			SetAFSyncRQ = 16654112;
			PreAuthByDSID = 16657456;
			VerifyAFSyncRQ = 16593488;
			GenerateAFSyncRS = 16655968;
			SapInit = 16605520;
			SapGetP1 = 16578576;
			SapCalcBuffer = 16579696;
			SapGetAS = 16605600;
			SapGetASFD = 16666752;
			SapGetASFD_a = 16668032;
		}
		else if(iTunesDllVersion(L"12.0.1.26")){
			Kbsync = 186624;
			KbsyncID = 92288;
			CigHash = 67120;
			WriteSIDD = 112400;
			WriteSIDB = 100096;
			DeAuthSIDB = 270528;
			CalcUnkP1 = 209040;
			SetAFSyncRQ = 214112;
			PreAuthByDSID = 217712;
			VerifyAFSyncRQ = 70592;
			GenerateAFSyncRS = 32224;
			GetCltDat = 678256;
			TranSetInf = 681472;
			UpdCDID = 677472;
			GetMD = 683152;
			SapInit = 100048;
			SapGetP1 = 32128;
			SapCalcBuffer = 36048;
			SapGetAS = 175328;
			SapGetASFD = 238416;
			SapGetASFD_a = 241920;
		}
		else if(iTunesDllVersion(L"12.1.0.71") ||
			iTunesDllVersion(L"12.1.1.4") ||
			iTunesDllVersion(L"12.1.2.27")){
				if(!IsMachineAmd64(L"iTunes.dll",internal::GetITunesInstallDll(L"").c_str())){
					Kbsync = 0x2D900u;
					KbsyncID = 0x16880u;
					CigHash = 0x10630u;
					WriteSIDD = 0x1B710u;
					WriteSIDB = 0x18700u;
					DeAuthSIDB = 0x420C0u;
					CalcUnkP1 = 0x33090u;
					SetAFSyncRQ = 0x34460u;
					PreAuthByDSID = 0x35270u;
					VerifyAFSyncRQ = 0x113C0u;
					GenerateAFSyncRS = 0x7DE0u;
					GetCltDat = 0xA5970u;
					TranSetInf = 0xA6600u;
					UpdCDID = 0xA5660u;
					GetMD = 0xA6C90u;
					SapInit = 0x186D0u;
					SapGetP1 = 0x7D80u;
					SapCalcBuffer = 0x8CD0u;
					SapGetAS = 0x2ACE0u;
					SapGetASFD = 0x3A350u;
					SapGetASFD_a = 0x3B100u;
					const wchar_t* http_ua = L"iTunes/12.1 (Windows; Microsoft Windows 7 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/7600.1017.5000.21";
				}
				else{
					Kbsync = 0x6C940u;
					KbsyncID = 0x278E0u;
					CigHash = 0x1D950u;
					WriteSIDD = 0x66D40u;
					WriteSIDB = 0x42F30u;
					DeAuthSIDB = 0x6DD80u;
					CalcUnkP1 = 0x522A0u;
					SetAFSyncRQ = 0x53370u;
					PreAuthByDSID = 0x57A40u;
					VerifyAFSyncRQ = 0x10BD0u;
					GenerateAFSyncRS = 0x51210u;
					GetCltDat = 0x94DE0u;
					TranSetInf = 0x959C0u;
					UpdCDID = 0x94A10u;
					GetMD = 0x960E0u;
					SapInit = 0x449D0u;
					SapGetP1 = 0x14070u;
					SapCalcBuffer = 0x472B0u;
					SapGetAS = 0x5AE30u;
					SapGetASFD = 0x8ED20u;
					SapGetASFD_a = 0x11FF0u;
					const wchar_t* http_ua = L"iTunes/12.1 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601); x64) AppleWebKit/7600.1017.5000.21";
				}
		}
		else if (iTunesDllVersion(L"12.2.0.145") ||
			iTunesDllVersion(L"12.2.1.16") || 
			iTunesDllVersion(L"12.2.2.25")){
			Kbsync = 0x2BC50u;
			KbsyncID = 0x25370u;
			CigHash = 0x46890u;
			WriteSIDD = 0x73490u;
			WriteSIDB = 0x302E0u;
			DeAuthSIDB = 0x63EA0u;
			CalcUnkP1 = 0x6C710u;
			SetAFSyncRQ = 0x60A20u;
			PreAuthByDSID = 0x57160u;
			VerifyAFSyncRQ = 0x284C0u;
			GenerateAFSyncRS = 0x294F0u;
			GetCltDat = 0xB0970u;
			TranSetInf = 0xB1600u;
			UpdCDID = 0;
			GetMD = 0xB1C90u;
			SapInit = 0x20790u;
			SapGetP1 = 0x5DA20u;
			SapCalcBuffer = 0x6D850u;
			SapGetAS = 0x887E0u;
			SapGetASFD = 0x42AA0u;
			SapGetASFD_a = 0xE770u;
			const wchar_t* http_ua = L"iTunes/12.2 (Windows; Microsoft Windows 7 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/7600.1017.5000.21";
		}
		else if (iTunesDllVersion(L"12.3.0.44") || iTunesDllVersion(L"12.3.1.23")){
			Kbsync = 0x1CF80u;
			KbsyncID = 0xABA50u;
			CigHash = 0xACD0u;
			WriteSIDD = 0x35EA0u;
			WriteSIDB = 0x5F650u;
			DeAuthSIDB = 0x3F4C0u;
			CalcUnkP1 = 0x42AC0u;
			SetAFSyncRQ = 0x54170u;
			PreAuthByDSID = 0x7BC20u;
			VerifyAFSyncRQ = 0x43CB0u;
			GenerateAFSyncRS = 0x5350u;
			SapInit = 0x2FEE0u;
			SapGetP1 = 0x65FA0u;
			SapCalcBuffer = 0xA8920u;
			SapGetAS = 0x32F40u;
			SapGetASFD = 0x82FE0u;
			SapGetASFD_a = 0x7D620u;
			const wchar_t* http_ua = L"iTunes/12.3 (Windows; Microsoft Windows 7 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/7601.1056.1.1";
		}
		else{
			Kbsync = -1;
			KbsyncID = -1;
			CigHash = -1;
			WriteSIDD = -1;
			WriteSIDB = -1;
			DeAuthSIDB = -1;
			CalcUnkP1 = -1;
			SetAFSyncRQ = -1;
			PreAuthByDSID = -1;
			VerifyAFSyncRQ = -1;
			GenerateAFSyncRS = -1;
			GetCltDat = -1;
			TranSetInf = -1;
			UpdCDID = -1;
			GetMD = -1;
			SapInit = -1;
			SapGetP1 = -1;
			SapCalcBuffer = -1;
			SapGetAS = -1;
			SapGetASFD = -1;
			SapGetASFD_a = -1;
		}
		if(AirTrafficHostDllVersion(L"17.203.0.35")){
			InitHost = 0x41C0u;
			EstablishKey = 0x62B0u;
		}
		else if(AirTrafficHostDllVersion(L"17.203.0.40")){
			InitHost = 0x41C0u;
			EstablishKey = 0x62B0u;
		}
		else if(AirTrafficHostDllVersion(L"17.204.9.12")){
			InitHost = 0x41A0u;
			EstablishKey = 0x6290u;
		}
		else if(AirTrafficHostDllVersion(L"17.249.0.82")){
			InitHost = 17104;
			EstablishKey = 25536;
		}
		else if(AirTrafficHostDllVersion(L"17.282.0.1")){
			InitHost = 17248;
			EstablishKey = 25680;
		}
		else if(AirTrafficHostDllVersion(L"117.283.0.3")){
			InitHost = 17248;
			EstablishKey = 25680;
		}
		else if(AirTrafficHostDllVersion(L"17.287.0.14")){
			InitHost = 17248;
			EstablishKey = 25680;
		}
		else if(AirTrafficHostDllVersion(L"17.351.0.5")){
			InitHost = 0x4440u;
			EstablishKey = 0x6530u;
		}
		else if(AirTrafficHostDllVersion(L"17.351.6.39")){
			InitHost = 0x4570u;
			EstablishKey = 0x6660u;
		}
		else if(AirTrafficHostDllVersion(L"17.351.6.52")){
			InitHost = 0x4570u;
			EstablishKey = 0x6660u;
		}
		else if(AirTrafficHostDllVersion(L"17.448.0.41")){
			InitHost = 0x4570u;
			EstablishKey = 0x6660u;
		}
		else if(AirTrafficHostDllVersion(L"17.448.100.3")){
			InitHost = 0x5CA0u;
			EstablishKey = 0x7D90u;
		}
		else if(AirTrafficHostDllVersion(L"17.492.0.29")){
			if(!IsMachineAmd64(L"AirTrafficHost.dll",internal::GetAppleMobileDeviceSupportDll(L"").c_str())){
				InitHost = 0x6560u;
				EstablishKey = 0x8650u;
			}
			else{
				InitHost = 0x8F70u;
				EstablishKey = 0x21CA0u;
			}
		}
		else if (AirTrafficHostDllVersion(L"17.492.0.84")){
			InitHost = 0x6560u;
			EstablishKey = 0x8650u;
		}
		else if (AirTrafficHostDllVersion(L"17.578.2.5")){
			InitHost = 0x6560u;
			EstablishKey = 0x8650u;
		}
		else{
			InitHost = -1;
			EstablishKey = -1;
		}
		iTunesInternalInterface::Instance()->kb_seed = 0;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnKbsync) = reinterpret_cast<unsigned long>(itunes_base)+Kbsync;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnCigHash) = reinterpret_cast<unsigned long>(itunes_base)+CigHash;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnKbsyncID) = reinterpret_cast<unsigned long>(itunes_base)+KbsyncID;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnWriteSIDD) = reinterpret_cast<unsigned long>(itunes_base)+WriteSIDD;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnWriteSIDB) = reinterpret_cast<unsigned long>(itunes_base)+WriteSIDB;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnDeAuthSIDB) = reinterpret_cast<unsigned long>(itunes_base)+DeAuthSIDB;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnGenerateAFSyncRS) = reinterpret_cast<unsigned long>(itunes_base)+GenerateAFSyncRS;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnVerifyAFSyncRQ) = reinterpret_cast<unsigned long>(itunes_base)+VerifyAFSyncRQ;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSetAFSyncRQ) = reinterpret_cast<unsigned long>(itunes_base)+SetAFSyncRQ;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnCalcUnkP1) = reinterpret_cast<unsigned long>(itunes_base)+CalcUnkP1;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnPreAuthByDSID) = reinterpret_cast<unsigned long>(itunes_base)+PreAuthByDSID;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapInit) = reinterpret_cast<unsigned long>(itunes_base)+SapInit;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapGetP1) = reinterpret_cast<unsigned long>(itunes_base)+SapGetP1;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapCalcBuffer) = reinterpret_cast<unsigned long>(itunes_base)+SapCalcBuffer;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapGetAS) = reinterpret_cast<unsigned long>(itunes_base)+SapGetAS;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapGetASFD) = reinterpret_cast<unsigned long>(itunes_base)+SapGetASFD;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnSapGetASFD_a) = reinterpret_cast<unsigned long>(itunes_base)+SapGetASFD_a;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnGetCltDat) = reinterpret_cast<unsigned long>(itunes_base)+GetCltDat;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnTranSetInf) = reinterpret_cast<unsigned long>(itunes_base)+TranSetInf;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnUpdCDID) = reinterpret_cast<unsigned long>(itunes_base)+UpdCDID;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnGetMD) = reinterpret_cast<unsigned long>(itunes_base)+GetMD;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnInitHost) = reinterpret_cast<unsigned long>(air_traffic_host_base)+InitHost;
		*reinterpret_cast<unsigned long*>(&iTunesInternalInterface::Instance()->lpfnEstablishKey) = reinterpret_cast<unsigned long>(air_traffic_host_base)+EstablishKey;
	}
	bool iTunesNativeInterface::IsMachineAmd64(const wchar_t* file,const wchar_t* dir){
		bool result = false;
		PLOADED_IMAGE load_image = ImageLoad(ABI::base::UnicodeToAscii(file).c_str(),ABI::base::UnicodeToAscii(dir).c_str());
		if(load_image->FileHeader->FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64){
			result = true;
		}
		else if(load_image->FileHeader->FileHeader.Machine==IMAGE_FILE_MACHINE_I386){
			result = false;
		}
		ImageUnload(load_image);
		return result;
	}
	bool iTunesNativeInterface::iTunesDllVersion(const wchar_t* version){
		static wchar_t g_version[1024] = {0};
		if(!g_version[0]){
			const std::wstring itunes_dll = internal::GetITunesInstallDll(L"iTunes.dll");
			wcscpy_s(g_version,1023,internal::GetSoftwareReleaseVersion(itunes_dll.c_str()).c_str());
		}
		return (std::wstring(g_version)==version);
	}
	bool iTunesNativeInterface::AirTrafficHostDllVersion(const wchar_t* version){
		static wchar_t g_version_a[1024] = {0};
		if(!g_version_a[0]){
			const std::wstring air_traffic_host_dll = internal::GetAppleMobileDeviceSupportDll(L"AirTrafficHostDLL");
			wcscpy_s(g_version_a,1023,internal::GetSoftwareReleaseVersion(air_traffic_host_dll.c_str()).c_str());
		}
		return (std::wstring(g_version_a)==version);
	}
}