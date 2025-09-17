#include <future>
#include "DeviceManager.h"
#include "iOSDevice.h"
#include "iOSApplication.h"
#include "ATH.h"
#include "RemoteAuth.h"
#include "Logger.h"
#include "aid2.h"

using namespace std;
using namespace aid2;
static future<void> gfutListen;
static CFRunLoopRef grunLoop;
static ConnectCallbackFunc ConnectCallback = nullptr; // 连接回调指针函数指针变量
static DisconnectCallbackFunc  DisconnectCallback = nullptr; // 断开连接回调函数指针变量
static string strAidservUrl;
static string strRootCert;
static string strClientKey;
static string strClientCert;


void device_notification_callback(struct AMDeviceNotificationCallbackInformation* CallbackInfo)
{
	AMDeviceRef deviceHandle = CallbackInfo->deviceHandle;
	if ((ConnectMode)AMDeviceGetInterfaceType(deviceHandle) == ConnectMode::USB)
	{
		switch (CallbackInfo->msgType)
		{
			case ADNCI_MSG_CONNECTECD:
			{
				string udid = DeviceManager::get_udid(deviceHandle);
				DeviceManager::add_device(udid, deviceHandle);
				logger.log("Start Device.");
				logger.log("Device %p connected,udid:%s", deviceHandle, udid.c_str());
				// 连接回调
				if (ConnectCallback) { // 连接回调
					auto appleInfo = DeviceManager::get_device(deviceHandle);
					ConnectCallback(udid.c_str(),
						appleInfo->DeviceName().c_str(),
						appleInfo->ProductType().c_str(),
						appleInfo->DeviceEnclosureColor().c_str(),
						appleInfo->MarketingName().c_str()
					);
				}
				break; 
			}
			case ADNCI_MSG_DISCONNECTED:
			{
				string udid = DeviceManager::get_udid(deviceHandle);
				if (DisconnectCallback) {
					DisconnectCallback(udid.c_str());
				}
				DeviceManager::remove_device(udid);
				logger.log("Device %p disconnected,udid:%s", deviceHandle, udid.c_str());
				break;
			}
			case ADNCI_MSG_UNKNOWN:
				logger.log("Unsubscribed");
				break;
			default:
				logger.log("Unknown message %d\n", CallbackInfo->msgType);
				break;
		}
	}
	else
	{
		logger.log("不处理USB以外连接设备 %p 进行connect.", deviceHandle);
	}

}

void Setaidserv(const char* url)
{
	strAidservUrl = url;
}

void TransferCertificate(const char* rootcert, const char* clientkey, const char* clientcert)
{
	strRootCert = rootcert;
	strClientKey = clientkey;
	strClientCert = clientcert;
}

bool StartListen()
{
	gfutListen = async(launch::async, [] {
		void* subscribe = nullptr;
		int ret = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0, &subscribe);
		if (ret) {
			logger.log("StartListen failed." );
			return;
		}
		grunLoop = CFRunLoopGetCurrent();
		CFRunLoopRun();
		AMDeviceNotificationUnsubscribe(subscribe);
		CFRelease(subscribe);
		return;
		});
	logger.log("StartListen success.");
	return true;
}

bool StopListen()
{
	CFRunLoopStop(grunLoop);
	gfutListen.wait();
	logger.log("StopListen success.");
	return true;
}

AuthorizeReturnStatus AuthorizeDevice(const char* udid) {
	auto retDoPair = DoPair(udid);
	if (retDoPair) {
		if (retDoPair<0) logger.log("信认失败或没有通过。");
		return (AuthorizeReturnStatus)retDoPair;
	};

	std::shared_ptr<RemoteAuth> client;
	if (strRootCert.empty())
		client = std::make_shared<RemoteAuth>(strAidservUrl, udid);
	else if (strClientCert.empty())
		client = std::make_shared<RemoteAuth>(strAidservUrl, udid, strRootCert);
	else
		client = std::make_shared<RemoteAuth>(strAidservUrl, udid, strRootCert, strClientCert, strClientKey);
	string remote_grappa = string();
	string grappa = string();

	try
	{
		ATH ath = ATH(udid);  //new出同步实例

		if (!client->GenerateGrappa(remote_grappa))  //Grappa数据生成
		{
			logger.log("udid:%s,RemoteGetGrappa failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,RemoteGetGrappa success.", udid);

		if (!ath.SyncAllowed()) {  //允许同步
			logger.log("udid:%s,SyncAllowed message read failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncAllowed message read success.", udid);

		//请求生成同步信息
		if (!ath.RequestingSync(remote_grappa)) {
			logger.log("udid:%s,RequestingSync failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		};
		logger.log("udid:%s,RequestingSync success.", udid);
		// 获取请求同步状态数据
		if (!ath.ReadyForSync(grappa)) {
			logger.log("udid:%s,ReadyForSync message read failed.\n", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,ReadyForSync message read success.", udid);
		if (!client->GenerateRs(grappa))	//调用远程服务器指令生成afsync.rs和afsync.rs.sig文件
		{
			logger.log("udid:%s,GenerateRs failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,GenerateRs success.", udid);
		//模拟itunes完成同步指令
		if (!ath.FinishedSyncingMetadata())
		{
			logger.log("udid:%s,FinishedSyncingMetadata failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,FinishedSyncingMetadata success.", udid);
		//读取同步状态
		if (!ath.SyncFinished())
		{
			logger.log("udid:%s,SyncFinished message read SyncFailed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncFinished message read ok.", udid);
		return AuthorizeReturnStatus::AuthorizeSuccess;
	}
	catch (const char* e)
	{
		logger.log(e);
		return AuthorizeReturnStatus::AuthorizeFailed;
	}
}

int DoPair(const char* udid)
{
	auto appleInfo = DeviceManager::get_device(udid);
	if (appleInfo == nullptr) {
		logger.log("设备没有插入，初始化失败。");
		return AuthorizeReturnStatus::AuthorizeFailed;
	}
	return appleInfo->DoPair();
}

bool InstallApplication(const char* udid, const char* ipaPath) {
	auto retDoPair = DoPair(udid);
	if (retDoPair) {
		if (retDoPair < 0) logger.log("信认失败或没有通过。");
		return false;
	};
	iOSApplication iosapp = iOSApplication((AMDeviceRef)DeviceManager::get_handle(udid));
	auto retInstall = iosapp.Install(ipaPath);
	if (iOSApplication::InstallCallback)
		iOSApplication::InstallCallback(retInstall ? "success" : "fail", 100);
	return retInstall;
}


//注册安装回调函数，授权过程中需要配对信息和授权结果通过回调函数通知
void RegisterInstallCallback(InstallApplicationFunc callback) {
	iOSApplication::InstallCallback = callback;
}

void RegisterConnectCallback(ConnectCallbackFunc callback)
{
	ConnectCallback = callback;
}

void RegisterDisconnectCallback(DisconnectCallbackFunc callback)
{
	DisconnectCallback = callback;
}
