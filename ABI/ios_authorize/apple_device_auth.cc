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
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

extern char g_sLibraryID[];

namespace ABI{
	namespace internal{
		const char filename_afsync_rq[] = "/AirFair/sync/afsync.rq";
		const char filename_afsync_rq_sig[] = "/AirFair/sync/afsync.rq.sig";
		const char filename_afsync_rs[] = "/AirFair/sync/afsync.rs";
		const char filename_afsync_rs_sig[] = "/AirFair/sync/afsync.rs.sig";
		static char g_authorize_udid[MAX_PATH] = {0};
		bool RequestSignatureAuthGenerate(AppleMobileDeviceEx& mobile_device,unsigned char *rq,unsigned long length,unsigned char *sig,unsigned long sig_length){
//remote server generate rs and rs.sig follow:
			ABI::internal::AppleRemoteAuth auth;
			ABI::internal::RSKeyGen* rs = ABI::internal::RSKeyGen::GetInstance();
			ABI::internal::RSSigKeyGen* rs_sig = ABI::internal::RSSigKeyGen::GetInstance();
			rs->reset();
			rs->set_grappa_session_id(mobile_device.grappa_session_id());
			rs->set_fair_play_certificate(mobile_device.fair_play_certificate());
			rs->set_fair_play_certificate_len(mobile_device.fair_play_certificate_len());
			rs->set_key_fair_play_guid(mobile_device.key_fair_play_guid());
			rs->set_fair_play_device_type(mobile_device.fair_play_device_type());
			unsigned long priv_k = rs->GenPrivateKey(rq,length,sig,sig_length);
			//form server remote generate auth data
			std::string response_auth = auth.RemoteAuth(auth.RemoteAuthReady(mobile_device,rq,length,sig,sig_length,priv_k));
			scoped_array<unsigned char> decode_buffer(reinterpret_cast<unsigned char*>(new unsigned char[response_auth.size()]));
			rs->set_data(reinterpret_cast<unsigned char*>(malloc(response_auth.size())));
			//base64 decode auth data
			size_t decode_length = EVP_DecodeBlock(decode_buffer.get(),reinterpret_cast<const unsigned char*>(response_auth.c_str()),response_auth.size());
			decode_length -= ABI::base::EVPLength(response_auth);
			rs->set_length(decode_length);
			memmove(rs->data(),decode_buffer.get(),decode_length);
			//////////////////////////////////////////////////////////////////////////
			if(rs->data()==NULL||rs->length()==0){
				LOG(ERROR)<<"genreate rs failed!"<<std::endl;
				return false;
			}
			rs_sig->reset();
			rs_sig->GenSignature(mobile_device.grappa_session_id(),rs->data(),rs->length());
			if(rs_sig->data()==NULL||rs_sig->length()==0){
				LOG(ERROR)<<"genreate rs.sig failed!"<<std::endl;
				return false;
			}
 			return true;
/*local generate rs and rs.sig follow:*/
// 			__int64 user_dsid[] = { 0x1E1A746E8 };
// 			ABI::internal::RSKeyGen::GetInstance()->set_grappa_session_id(mobile_device.grappa_session_id());
// 			ABI::internal::RSKeyGen::GetInstance()->set_fair_play_certificate(mobile_device.fair_play_certificate());
// 			ABI::internal::RSKeyGen::GetInstance()->set_fair_play_certificate_len(mobile_device.fair_play_certificate_len());
// 			ABI::internal::RSKeyGen::GetInstance()->set_key_fair_play_guid(mobile_device.key_fair_play_guid());
// 			ABI::internal::RSKeyGen::GetInstance()->set_fair_play_device_type(mobile_device.fair_play_device_type());
// 			ABI::internal::RSKeyGen::GetInstance()->GenRS(reinterpret_cast<Int64Make*>(&user_dsid[0]), sizeof(user_dsid) / sizeof(__int64), rq, length, sig, sig_length);
// 			if (ABI::internal::RSKeyGen::GetInstance()->data() == NULL || ABI::internal::RSKeyGen::GetInstance()->length() == 0){
// 				LOG(ERROR) << "genreate rs failed!" << std::endl;
// 				return false;
// 			}
// 			ABI::internal::RSSigKeyGen::GetInstance()->GenSignature(mobile_device.grappa_session_id(), ABI::internal::RSKeyGen::GetInstance()->data(), ABI::internal::RSKeyGen::GetInstance()->length());
// 			if (ABI::internal::RSSigKeyGen::GetInstance()->data() == NULL || ABI::internal::RSSigKeyGen::GetInstance()->length() == 0){
// 				LOG(ERROR) << "genreate rs.sig failed!" << std::endl;
// 				return false;
// 			}
//			return true;
		}
		AFCRef OpenIOSFileSystem(void *deviceHandle){
			AFCRef afc;
			void *serviceHandle;
			CFStringRef sServiceName = CFStringCreateWithCString(NULL,"com.apple.afc",kCFStringEncodingUTF8);
			if(AMDeviceStartService(deviceHandle,sServiceName,&serviceHandle,NULL) != 0){
				return NULL;
			}
			CFRelease(sServiceName);
			if(AFCConnectionOpen(serviceHandle,0,&afc)!=0){
				return NULL;
			}
			return afc;
		}
		void message(const char* format,...){
			char buffer[1024] = {0};
			va_list args;
			va_start(args,format);
			vsnprintf(buffer,1024,format,args);
			LOG(INFO)<<buffer<<std::endl;
			va_end (args);
		}
		void CloseIOSFileSystem(AFCRef afc){
			AFCConnectionClose(afc);
		}
		void device_notification_callback(struct AMDeviceNotificationCallbackInformation *CallbackInfo)
		{
			am_device *deviceHandle = CallbackInfo->deviceHandle;
			switch (CallbackInfo->msgType)
			{
			case ADNCI_MSG_CONNECTED:
				message("Device %p connected\n", deviceHandle);
				if(deviceHandleConnected == NULL){
					ConnectIOSDevice(deviceHandle);
					ABI::internal::AppleMobileDeviceEx apple_device(deviceHandle);
					apple_device.GenFairDeviceInfo();
					if(std::string(g_authorize_udid)!=apple_device.unique_device_id()){
						CloseIOSDevice(deviceHandleConnected);
						deviceHandleConnected = NULL;
					}
					else{
						deviceHandleConnected = deviceHandle;
					}
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
		bool __cdecl WaitDeviceUDID(const char* udid){
			void *subscribe = nullptr;
			if(udid==NULL||!udid[0]){
				return false;
			}
			strncpy(g_authorize_udid,udid,strlen(udid));
			AMDeviceNotificationSubscribe(device_notification_callback,0,0,0,&subscribe);
			for(;;){
				Sleep(100);
				if(deviceHandleConnected != NULL){
					memset(g_authorize_udid,0,MAX_PATH);
					break;
				}
			}
// 			int rc = 0;
// 			for(;;){
// 				rc = AMDeviceConnect(deviceHandleConnected);
// 				if(rc){
// 					LOG(ERROR)<<"AMDeviceConnect failed"<<std::endl;
// 					return false;
// 				}
// 				AMDeviceIsPaired(deviceHandleConnected);
// 				rc = AMDeviceValidatePairing(deviceHandleConnected);
// 				if(rc == 0){
// 					rc = AMDeviceStartSession(deviceHandleConnected);
// 					if (rc){
// 						LOG(ERROR)<<"AMDeviceStartSession failed"<<std::endl;
// 						return false;
// 					}
// 					break;
// 				}
// 				// Do pairing
// 				bool extstyle = true;
// 				if(!extstyle){
// 					rc = AMDevicePair(deviceHandleConnected);
// 				}
// 				else{
// 					CFDictionaryRef dictOptions = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);
// 					CFStringRef key = CFStringCreateWithCString(NULL, "ExtendedPairingErrors", kCFStringEncodingUTF8);
// 					CFDictionarySetValue(dictOptions, key, *pkCFBooleanTrue);
// 					rc = AMDevicePairWithOptions(deviceHandleConnected, dictOptions);
// 				}
// 				if (rc == 0xe800001a){
// 					message("请打开密码锁定，进入ios主界面\n");
// 				}
// 				else if (rc == 0xe8000096){
// 					message("请在设备端按下“信任”按钮\n");
// 				}
// 				else if (rc == 0xe8000095){
// 					message("用户按下了“不信任”按钮\n");
// 					Sleep(2000);
// 					return false;
// 				}
// 				else if (rc == 0){
// 					break;
// 				}
// 				rc = AMDeviceDisconnect(deviceHandleConnected);
// 				if (rc){
// 					LOG(ERROR)<<"AMDeviceDisconnect failed"<<std::endl;
// 					return false;
// 				}
// 				Sleep(3000);
// 			}
// 			rc = AMDeviceStopSession(deviceHandleConnected);
// 			if (rc){
// 				LOG(ERROR)<<"AMDeviceStopSession failed"<<std::endl;
// 				return false;
// 			}
// 			rc = AMDeviceDisconnect(deviceHandleConnected);
// 			if (rc){
// 				LOG(ERROR)<<"AMDeviceDisconnect failed"<<std::endl;
// 				return false;
// 			}
// 			return true ;
			return true;
		}
		bool __cdecl ConnectIOSDevice(AMDeviceRef device){
			int ret;
			ret = AMDeviceConnect(device);
			if(ret){
				message("AMDeviceConnect() failed.\n");
				return false;
			}
			AMDeviceIsPaired(device);
			ret = AMDeviceValidatePairing(device);
			if(ret == 0){
				ret = AMDeviceStartSession(device);
				if(ret){
					message("AMDeviceStartSession() failed.\n");
					return false;
				}
			}
			return true;
		}
		bool __cdecl CloseIOSDevice(AMDeviceRef device){
			int ret;
			ret = AMDeviceStopSession(device);
			if (ret){
				message("AMDeviceStopSession() failed.\n");
				return false;
			}
			ret = AMDeviceDisconnect(device);
			if(ret){
				message("AMDeviceDisconnect() failed.\n");
				return false;
			}
			return true;
		}
		ErrorAuthorize __cdecl AuthorizeDevice(void *deviceHandle){
			AFCFileRef fileAfsyncRs = NULL;
			AFCFileRef fileAfsyncRsSig = NULL;
			ABI::internal::AppleMobileDeviceEx apple_device(deviceHandle);
			apple_device.GenFairDeviceInfo();
			CFStringRef sUDID = CFStringCreateWithCString(NULL,apple_device.unique_device_id(),kCFStringEncodingUTF8);
			CFStringRef sLibrary = CFStringCreateWithCString(NULL,g_sLibraryID,kCFStringEncodingUTF8);
			if(sUDID==NULL||sLibrary==NULL){
				LOG(ERROR)<<"device info read init failed"<<std::endl;
				return device_info_init;
			}
			ATHRef ath = ATHostConnectionCreateWithLibrary(sLibrary,sUDID,0);
			if(ath==NULL){
				LOG(ERROR)<<"ATHostConnectionCreateWithLibrary failed"<<std::endl;
				return unknown_sync_error;
			}
			HANDLE hThread = CreateThread(NULL,0,ReceiveMessageThreadFunc,(LPVOID)ath,0,NULL);
			if(hThread==NULL){
				LOG(ERROR)<<"ReceiveMessageThreadFunc Thread failed"<<std::endl;
				return unknown_sync_error;
			}
			else{
				WaitForSingleObject(hThread,5000);
			}
			AFCRef afc = OpenIOSFileSystem(deviceHandle);
			if(afc==NULL){
				LOG(ERROR)<<"OpenIOSFileSystem failed"<<std::endl;
				return unknown_sync_error;
			}
			else{
				AFCRemovePath(afc, filename_afsync_rq);
				SendSyncRequest(ath,SYNC_KEYBAG|SYNC_APPLICATION);
			}
			ABI::internal::RQReadBuf rq_reader;
			rq_reader.RQGenSize(afc);
			if(!rq_reader.RQGenBuf(afc)){
				LOG(ERROR)<<"read_afsync_rq_fail"<<std::endl;
				return read_afsync_rq_fail;
			}
			else{
				Sleep(2000);
			}
			ABI::internal::RQSigReadBuf rq_sig_reader;
			if(!rq_sig_reader.RQSigGenBuf(afc)){
				LOG(ERROR)<<"read_afsync_rq_sig_fail"<<std::endl;
				return read_afsync_rq_sig_fail;
			}
			else{
				CFRelease(sUDID);
				CFRelease(sLibrary);
				Sleep(2000);
			}
			apple_device.set_grappa_sessionid(ath);
			if(!RequestSignatureAuthGenerate(apple_device,rq_reader.rq_buf(),rq_reader.rq_size(),rq_sig_reader.rq_sig_buf(),rq_sig_reader.rq_sig_size())){
				LOG(ERROR)<<"open with write rs file failed"<<std::endl;
				return generate_rs_fail;
			}
			if(AFCFileRefOpen(afc,filename_afsync_rs,AFC_FILEMODE_WRITE,&fileAfsyncRs)!= 0){
				LOG(ERROR)<<"open with write rs file failed"<<std::endl;
				return open_afsync_rs_fail;
			}
			unsigned long ret = AFCFileRefWrite(afc,fileAfsyncRs,ABI::internal::RSKeyGen::GetInstance()->data(),ABI::internal::RSKeyGen::GetInstance()->length());
			if(ret != 0){
				LOG(ERROR)<<"write rs file failed"<<std::endl;
				return write_afsync_rs_fail;
			}
			AFCFileRefClose(afc,fileAfsyncRs);
			ret = AFCFileRefOpen(afc,filename_afsync_rs_sig,AFC_FILEMODE_WRITE, &fileAfsyncRsSig);
			if(ret!=0){
				LOG(ERROR)<<"open with write rs.sig file failed"<<std::endl;
				return open_afsync_rs_sig_fail;
			}
			ret = AFCFileRefWrite(afc, fileAfsyncRsSig,ABI::internal::RSSigKeyGen::GetInstance()->data(),ABI::internal::RSSigKeyGen::GetInstance()->length());
			if(ret != 0){
				LOG(ERROR)<<"write rs.sig file failed"<<std::endl;
				return write_afsync_rs_sig_fail;
			}
			AFCFileRefClose(afc, fileAfsyncRsSig);
			SendMetadataSyncFinished(ath,SYNC_FINISHED_2_KEYBAG|SYNC_FINISHED_2_APPLICATION);
			if(WaitForSingleObject(hThread,INFINITE)!=WAIT_OBJECT_0){
				LOG(ERROR)<<"WaitForSingleObject failed"<<std::endl;
				return wait_object_fail;
			}
			CloseIOSFileSystem(afc);
			ATHostConnectionDestroy(ath);
			return authorize_ok;
		}
	}
}