#include "abi/ios_authorize/apple_device_auth.h"
#include <cstdio>
#include <Windows.h>
#include "ABI/ios_authorize/iTunesInterface.h"
#include "ABI/ios_authorize/Sync.h"
#include "ABI/ios_authorize/Universal.h"
#include "ABI/ios_authorize/itunes_internal_abi.h"
#include "ABI/ios_authorize/rs_sig_gen.h"
#include "ABI/ios_authorize/rq_sig_gen.h"
#include "ABI/ios_authorize/rs_gen.h"
#include "ABI/ios_authorize/rq_gen.h"
#include "ABI/ios_authorize/apple_mobile_device_ex.h"
#include "ABI/ios_authorize/apple_remote_auth.h"
#include "ABI/base/windows_process.h"
#include "ABI/base/file/path.h"
#include "ABI/base/string/string_split.h"
#include "ABI/base/string/string_conv.h"
#include "ABI/thirdparty/glog/logging.h"
#include "ABI/thirdparty/openssl/evp.h"


extern char g_sLibraryID[];

namespace ABI {
	namespace internal {
		const char filename_afsync_rq[] = "/AirFair/sync/afsync.rq";
		const char filename_afsync_rq_sig[] = "/AirFair/sync/afsync.rq.sig";
		const char filename_afsync_rs[] = "/AirFair/sync/afsync.rs";
		const char filename_afsync_rs_sig[] = "/AirFair/sync/afsync.rs.sig";
		static char g_authorize_udid[MAX_PATH] = { 0 };
		bool RequestSignatureAuthGenerate(AppleMobileDeviceEx& mobile_device, unsigned char* rq, unsigned long length, unsigned char* sig, unsigned long sig_length) {
			//remote server generate rs and rs.sig follow:
			ABI::internal::AppleRemoteAuth auth;
			ABI::internal::RSKeyGen* rs = ABI::internal::RSKeyGen::GetInstance();
			ABI::internal::RSSigKeyGen* rs_sig = ABI::internal::RSSigKeyGen::GetInstance();
	
			std::string response_auth = auth.RemoteAuth(mobile_device, rq, length, sig, sig_length);
			rs->reset();
			scoped_array<unsigned char> decode_buffer(reinterpret_cast<unsigned char*>(new unsigned char[response_auth.size()]));
			rs->set_data(reinterpret_cast<unsigned char*>(malloc(response_auth.size())));
			//base64 decode auth data
			size_t decode_length = EVP_DecodeBlock(decode_buffer.get(), reinterpret_cast<const unsigned char*>(response_auth.c_str()), response_auth.size());
			decode_length -= ABI::base::EVPLength(response_auth);
			rs->set_length(decode_length);
			memmove(rs->data(), decode_buffer.get(), decode_length);
			//////////////////////////////////////////////////////////////////////////
			if (rs->data() == NULL || rs->length() == 0) {
				LOG(ERROR) << "genreate rs failed!" << std::endl;
				return false;
			}
			rs_sig->reset();
			rs_sig->GenSignature(mobile_device.grappa_session_id(), rs->data(), rs->length());
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
			switch (CallbackInfo->msgType)
			{
			case ADNCI_MSG_CONNECTED:
				message("Device %p connected\n", deviceHandle);
				if (deviceHandleConnected == NULL) {
					AMDeviceConnect(deviceHandle);
					ABI::internal::AppleMobileDeviceEx apple_device(deviceHandle);
					if (std::string(g_authorize_udid) != apple_device.udid()) {
						deviceHandleConnected = NULL;
					}
					else {
						deviceHandleConnected = deviceHandle;
					}
					AMDeviceDisconnect(deviceHandle);
				}
				break;

			case ADNCI_MSG_DISCONNECTED:
				message("Device %p disconnected\n", deviceHandle);
				deviceHandleConnected = NULL;
				break;

			case 3:
				message("Unsubscribed\n");
				break;

			default:
				message("Unknown message %d\n", CallbackInfo->msgType);
				break;
			}
		}
		bool __cdecl WaitDeviceUDID(const char* udid) {
			void* subscribe = nullptr;
			if (udid == NULL || !udid[0]) {
				return false;
			}
			strncpy(g_authorize_udid, udid, strlen(udid));
			AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0, &subscribe);
			int i = 0;
			for (;;) {
				Sleep(100);
				if (deviceHandleConnected != NULL) {
					memset(g_authorize_udid, 0, MAX_PATH);
					break;
				}
				if (i++ >= 300) return false;
			}
			return true;
		}
		bool __cdecl ConnectIOSDevice(AMDeviceRef device) {
			int ret;
			ret = AMDeviceConnect(device);
			if (ret) {
				message("AMDeviceConnect() failed.\n");
				return false;
			}
			AMDeviceIsPaired(device);
			ret = AMDeviceValidatePairing(device);
			if (ret == 0) {
				ret = AMDeviceStartSession(device);
				if (ret) {
					message("AMDeviceStartSession() failed.\n");
					return false;
				}
			}
			return true;
		}
		bool __cdecl CloseIOSDevice(AMDeviceRef device) {
			int ret;
			ret = AMDeviceStopSession(device);
			if (ret) {
				message("AMDeviceStopSession() failed.\n");
				return false;
			}
			ret = AMDeviceDisconnect(device);
			if (ret) {
				message("AMDeviceDisconnect() failed.\n");
				return false;
			}
			deviceHandleConnected = NULL;
			return true;
		}
		ErrorAuthorize __cdecl AuthorizeDevice(void* deviceHandle) {
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
			ATHRef ath = ATHostConnectionCreateWithLibrary(sLibrary, sUDID, 0);
			if (ath == NULL) {
				LOG(ERROR) << "ATHostConnectionCreateWithLibrary failed" << std::endl;
				return unknown_sync_error;
			}

			g_startSync = false;
			HANDLE hThread = CreateThread(NULL, 0, ReceiveMessageThreadFunc, (LPVOID)ath, 0, NULL);
			if (hThread == NULL) {
				LOG(ERROR) << "ReceiveMessageThreadFunc Thread failed" << std::endl;
				return unknown_sync_error;
			}
			else {
				WaitForSingleObject(hThread, 5000);
			}

			AFCRef afc = OpenIOSFileSystem(deviceHandle);
			if (afc == NULL) {
				LOG(ERROR) << "OpenIOSFileSystem failed" << std::endl;
				return unknown_sync_error;
			}
			else {
				AFCRemovePath(afc, filename_afsync_rq);
				while (!g_startSync) Sleep(100);
				SendSyncRequest(ath, SYNC_KEYBAG);
			}
			//ATHostConnectionReadMessage(ath);

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
				CFRelease(sUDID);
				CFRelease(sLibrary);
			}
			apple_device.set_grappa_sessionid(ath);
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
			return authorize_ok;
		}
	}
}