#include "abi/ios_authorize/apple_device_auth.h"
#include <cstdio>
#include <Windows.h>
#include "ABI/ios_authorize/Sync.h"
#include "ABI/ios_authorize/Universal.h"
#include "ABI/ios_authorize/rs_sig_gen.h"
#include "ABI/ios_authorize/rq_sig_gen.h"
#include "ABI/ios_authorize/rs_gen.h"
#include "ABI/ios_authorize/rq_gen.h"
#include "ABI/ios_authorize/apple_mobile_device_ex.h"
#include "ABI/base/windows_process.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/string_conv.h"
#include "ABI/thirdparty/glog/logging.h"
#include "ABI/thirdparty/openssl/evp.h"
#include "GenerateRS_client.h"
#include <smartbot/passport/itunes_internal_interface.h>

extern char g_sLibraryID[];

namespace ABI {
	namespace internal {
		using namespace std;

		const char filename_afsync_rq[] = "/AirFair/sync/afsync.rq";
		const char filename_afsync_rq_sig[] = "/AirFair/sync/afsync.rq.sig";
		const char filename_afsync_rs[] = "/AirFair/sync/afsync.rs";
		const char filename_afsync_rs_sig[] = "/AirFair/sync/afsync.rs.sig";
		//static char g_authorize_udid[MAX_PATH] = { 0 };
		static map<string, am_device*> gudid;
		


		bool RequestSignatureAuthGenerate(AppleMobileDeviceEx& mobile_device, unsigned char* rq, unsigned long length, unsigned char* sig, unsigned long sig_length) {
			//remote server generate rs and rs.sig follow:

			std::string rs_data;
			std::string rs_sig_data;
			ABI::internal::RSKeyGen* rs = ABI::internal::RSKeyGen::GetInstance();
			ABI::internal::RSSigKeyGen* rs_sig = ABI::internal::RSSigKeyGen::GetInstance();
			//RemoteAuth
			aidClient* client = aidClient::Instance();
			bool ret = client->RemoteAuth(mobile_device, rq, length, sig, sig_length, rs_data, rs_sig_data);
			//rs->set_data
			rs->reset();
			rs->set_data(reinterpret_cast<unsigned char*>(malloc(rs_data.size())));
			rs->set_length(rs_data.size());
			memmove(rs->data(), rs_data.c_str(), rs_data.size());
			//////////////////////////////////////////////////////////////////////////
			if (rs->data() == NULL || rs->length() == 0) {
				LOG(ERROR) << "genreate rs failed!" << std::endl;
				return false;
			}
			//rs_sig->set_data
			rs_sig->reset();
			rs_sig->set_data(reinterpret_cast<unsigned char*>(malloc(rs_sig_data.size())));
			rs_sig->set_length(rs_sig_data.size());
			memmove(rs_sig->data(), rs_sig_data.c_str(), rs_sig_data.size());
			if (rs_sig->data() == NULL || rs_sig->length() == 0) {
				LOG(ERROR) << "genreate rs.sig failed!" << std::endl;
				return false;
			}
			return true;
		}
		AFCRef OpenIOSFileSystem(void* deviceHandle) {
			AFCRef afc;
			void* serviceHandle;
			CFStringRef sServiceName = CFStringCreateWithCString(NULL, "com.apple.afc", kCFStringEncodingUTF8);
			if (AMDeviceStartService(deviceHandle, sServiceName, &serviceHandle, NULL) != 0) {
				return NULL;
			}
			CFRelease(sServiceName);
			if (AFCConnectionOpen(serviceHandle, 0, &afc) != 0) {
				return NULL;
			}
			return afc;
		}
		void message(const char* format, ...) {
			char buffer[1024] = { 0 };
			va_list args;
			va_start(args, format);
			vsnprintf(buffer, 1024, format, args);
			LOG(INFO) << buffer << std::endl;
			va_end(args);
		}
		void CloseIOSFileSystem(AFCRef afc) {
			AFCConnectionClose(afc);
		}
		void device_notification_callback(struct AMDeviceNotificationCallbackInformation* CallbackInfo)
		{
			am_device* deviceHandle = CallbackInfo->deviceHandle;
			if (deviceHandle->connection_type == CONNECTION_TYPE_NETWORK)  //	CONNECTION_TYPE_NETWORK return
			{
				message("不处理网络连接设备 %p \n", deviceHandle);
				return;
			}
			switch (CallbackInfo->msgType)
			{
			case ADNCI_MSG_CONNECTED:
			{
				if (deviceHandle->connectid == 0) {
					int connret = AMDeviceConnect(deviceHandle);
				}
				ABI::internal::AppleMobileDeviceEx apple_device(deviceHandle);
				string udid = apple_device.udid();
				gudid[udid] = deviceHandle;
				message("Device %p connected\n", deviceHandle);
				break;
			}
			case ADNCI_MSG_DISCONNECTED:
			{
				string udid;
				for (auto it : gudid)
				{
					if (it.second == deviceHandle)
					{
						int iret = AMDeviceDisconnect(it.second);
						udid = it.first;
						break;
					}
				}
				gudid.erase(udid);
				message("Device %p disconnected\n", deviceHandle);
				break;
			}
			case 3:
				message("Unsubscribed\n");
				break;

			default:
				message("Unknown message %d\n", CallbackInfo->msgType);
				break;
			}
		}


		bool __cdecl WaitDeviceUDID() {
			void* subscribe = nullptr;
			int ret = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0, &subscribe);
			if (ret) {
				message("WaitDeviceUDID() failed.\n");
				return false;
			}
			return true;
		}



		bool __cdecl IOSDeviceStartSession(AMDeviceRef device) {
			AMDeviceIsPaired(device);
			int ret = AMDeviceValidatePairing(device);
			if (ret)
			{
				message("AMDeviceValidatePairing() failed.\n");
				return false;
			}
			else
			{
				ret = AMDeviceStartSession(device);
				if (ret) {
					message("AMDeviceStartSession() failed.\n");
					return false;
				}
			}
			return true;
		}

		bool __cdecl IOSDeviceStopSession(AMDeviceRef device) {
			int ret = AMDeviceStopSession(device);
			if (ret) {
				message("AMDeviceStopSession() failed.\n");
				return false;
			}
			return true;
		}


		ErrorAuthorize __cdecl AuthorizeDevice(const string udid) {
			int i = 0;
			for (;;) {
				Sleep(100);
				if (gudid[udid] != NULL) {
					break;
				}
				if (i++ >= 30) {
					gudid.erase(udid);   //remove udid
					LOG(ERROR) << "设备没有插入，初始化失败。" << std::endl;
					return device_init_fail;
				}
			}
			AMDeviceRef  deviceHandle = gudid.at(udid);
			if (deviceHandle == NULL) {
				LOG(ERROR) << "IOSDeviceStartSession failed" << std::endl;
				return start_session_fail;
			}

			if (!IOSDeviceStartSession(deviceHandle))
			{
				LOG(ERROR) << "IOSDeviceStartSession failed" << std::endl;
				return start_session_fail;
			}
			//读取设备信息
			AFCFileRef fileAfsyncRs = NULL;
			AFCFileRef fileAfsyncRsSig = NULL;
			ABI::internal::AppleMobileDeviceEx apple_device(deviceHandle);
			apple_device.GenFairDeviceInfo();
			CFStringRef sUDID = CFStringCreateWithCString(NULL, apple_device.unique_device_id(), kCFStringEncodingUTF8);
			CFStringRef sLibrary = CFStringCreateWithCString(NULL, g_sLibraryID, kCFStringEncodingUTF8);
			if (sUDID == NULL || sLibrary == NULL) {
				LOG(ERROR) << "device info read init failed" << std::endl;
				return device_info_init;
			}
			//ATHostConnectionCreateWithLibrary
			std:wstring wathpath = passport::internal::GetITunesInstallDll(L"").append(L"ATH.exe");
			//wstring to string
			std::string athpath;
			athpath.assign(wathpath.begin(), wathpath.end());
			CFStringRef sAthexe = CFStringCreateWithCString(NULL, athpath.c_str(), kCFStringEncodingUTF8);
			ATHRef ath = ATHostConnectionCreateWithLibrary(sLibrary, sUDID, sAthexe);
			if (ath == NULL) {
				LOG(ERROR) << "ATHostConnectionCreateWithLibrary failed" << std::endl;
				return unknown_sync_error;
			}
			CFRelease(sAthexe);
			CFRelease(sUDID);
			CFRelease(sLibrary);

			//RemoteGetGrappa
			aidClient* client = aidClient::Instance();
			string grappa;
			unsigned long grappa_session_id;
			auto rggret = client->RemoteGetGrappa(udid, grappa, grappa_session_id);
			if (!rggret)
			{
				LOG(ERROR) << "RemoteGetGrappa failed" << std::endl;
				return unknown_sync_error;
			}
			//CreateThread
			g_startSync = false;
			HANDLE hThread = CreateThread(NULL, 0, ReceiveMessageThreadFunc, (LPVOID)ath, 0, NULL);
			if (hThread == NULL) {
				LOG(ERROR) << "ReceiveMessageThreadFunc Thread failed" << std::endl;
				return unknown_sync_error;
			}
			else {
				WaitForSingleObject(hThread, 5000);
			}
			//SendSyncRequest
			AFCRef afc = OpenIOSFileSystem(deviceHandle);
			if (afc == NULL) {
				LOG(ERROR) << "OpenIOSFileSystem failed" << std::endl;
				return unknown_sync_error;
			}
			else {
				AFCRemovePath(afc, filename_afsync_rq);
				while (!g_startSync) Sleep(100);
				SendSyncRequest(ath, SYNC_KEYBAG, grappa);
			}

			ABI::internal::RQReadBuf rq_reader;
			rq_reader.RQGenSize(afc);
			if (!rq_reader.RQGenBuf(afc)) {
				LOG(ERROR) << "read_afsync_rq_fail" << std::endl;
				return read_afsync_rq_fail;
			}
			else {
				Sleep(1000);
			}
			ABI::internal::RQSigReadBuf rq_sig_reader;
			if (!rq_sig_reader.RQSigGenBuf(afc)) {
				LOG(ERROR) << "read_afsync_rq_sig_fail" << std::endl;
				return read_afsync_rq_sig_fail;
			}
			else {
				//CFRelease(sUDID);
				//CFRelease(sLibrary);
				//CFRelease(sAthexe);
			}
			apple_device.set_grappa_sessionid(grappa_session_id);
			apple_device.set_grappa_data(g_grappa);
			apple_device.set_grappa_data_len(g_grappa_len);
			if (!RequestSignatureAuthGenerate(apple_device, rq_reader.rq_buf(), rq_reader.rq_size(), rq_sig_reader.rq_sig_buf(), rq_sig_reader.rq_sig_size())) {
				LOG(ERROR) << "open with write rs file failed" << std::endl;
				return generate_rs_fail;
			}
			if (AFCFileRefOpen(afc, filename_afsync_rs, AFC_FILEMODE_WRITE, &fileAfsyncRs) != 0) {
				LOG(ERROR) << "open with write rs file failed" << std::endl;
				return open_afsync_rs_fail;
			}
			unsigned long ret = AFCFileRefWrite(afc, fileAfsyncRs, ABI::internal::RSKeyGen::GetInstance()->data(), ABI::internal::RSKeyGen::GetInstance()->length());
			if (ret != 0) {
				LOG(ERROR) << "write rs file failed" << std::endl;
				return write_afsync_rs_fail;
			}
			AFCFileRefClose(afc, fileAfsyncRs);
			ret = AFCFileRefOpen(afc, filename_afsync_rs_sig, AFC_FILEMODE_WRITE, &fileAfsyncRsSig);
			if (ret != 0) {
				LOG(ERROR) << "open with write rs.sig file failed" << std::endl;
				return open_afsync_rs_sig_fail;
			}
			ret = AFCFileRefWrite(afc, fileAfsyncRsSig, ABI::internal::RSSigKeyGen::GetInstance()->data(), ABI::internal::RSSigKeyGen::GetInstance()->length());
			if (ret != 0) {
				LOG(ERROR) << "write rs.sig file failed" << std::endl;
				return write_afsync_rs_sig_fail;
			}
			AFCFileRefClose(afc, fileAfsyncRsSig);
			SendMetadataSyncFinished(ath, SYNC_FINISHED_2_KEYBAG);

			if (WaitForSingleObject(hThread, INFINITE) != WAIT_OBJECT_0) {
				LOG(ERROR) << "WaitForSingleObject failed" << std::endl;
				return wait_object_fail;
			}
			CloseIOSFileSystem(afc);
			ATHostConnectionDestroy(ath);

			if (!IOSDeviceStopSession(deviceHandle))
			{
				LOG(ERROR) << "IOSDeviceStopSession failed" << std::endl;
			}
			return authorize_ok;
		}
	}
}