#include <stdio.h>
#include <iostream>
#include<map>
#include <future>
#include "iOSDeviceInfo.h"
#include "iOSApplication.h"
#include "ATH.h"
#include <httplib.h>
#include "RemoteAuth.h"
#include "Logger.h"
#include "aid2.h"

using namespace std;
using namespace aid2;
static map<string, AMDeviceRef> gudid; //udid和设备句柄映射
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
				string udid = getUdid(deviceHandle);
				gudid[udid] = deviceHandle;
				auto appleInfo = iOSDeviceInfo(deviceHandle);
				// 连接回调
				if (ConnectCallback) { // 连接回调
					ConnectCallback(udid.c_str(),
						appleInfo.DeviceName().c_str(),
						appleInfo.ProductType().c_str(),
						appleInfo.DeviceEnclosureColor().c_str(),
						appleInfo.MarketingName().c_str()
					);
				}
				logger.log("Start Device.");
				logger.log("Device %p connected,udid:%s", deviceHandle, udid.c_str());
				break; 
			}
			case ADNCI_MSG_DISCONNECTED:
			{
				string udid;
				for (auto it : gudid)
				{
					if (it.second == deviceHandle)
					{
						udid = it.first;
						break;
					}
				}
				gudid.erase(udid);
				if (DisconnectCallback) {
					DisconnectCallback(udid.c_str());
				}
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

AID2_API void Setaidserv(const char* url)
{
	strAidservUrl = url;
}

AID2_API void TransferCertificate(const char* rootcert, const char* clientkey, const char* clientcert)
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
	int i = 0;
	map<string, AMDeviceRef>::iterator iter;
	for (;;) {
		this_thread::sleep_for(chrono::milliseconds(2));
		iter = gudid.find(udid);
		if (iter == gudid.end()) {
			if (i++ >= 30) {
				logger.log("设备没有插入，初始化失败。");
				return AuthorizeReturnStatus::AuthorizeFailed;
			}
		}
		else {
			break;
		}
	}
	auto deviceHandle = iter->second;
	return AuthorizeDeviceEx(deviceHandle);
}

AuthorizeReturnStatus AuthorizeDeviceEx(void* deviceHandle)
{
	iOSDeviceInfo appleInfo((AMDeviceRef)deviceHandle);
	auto retDoPair = appleInfo.DoPair();
	if (retDoPair) {
		logger.log("信认失败或没有通过。");
		return (AuthorizeReturnStatus)retDoPair;
	};

	RemoteAuth* client;
	if (strRootCert.empty())
		client = new RemoteAuth(strAidservUrl, (AMDeviceRef)deviceHandle);
	else if (strClientCert.empty())
		client = new RemoteAuth(strAidservUrl, (AMDeviceRef)deviceHandle, strRootCert);
	else
		client = new RemoteAuth(strAidservUrl, (AMDeviceRef)deviceHandle, strRootCert, strClientCert, strClientKey);
	string remote_grappa = string();
	string grappa = string();

	try
	{
		string udid = appleInfo.udid();
		ATH ath = ATH(udid);  //new出同步实例
		
		if (!client->GenerateGrappa(remote_grappa))  //Grappa数据生成
		{
			logger.log("udid:%s,RemoteGetGrappa failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,RemoteGetGrappa success.", udid.c_str());

		if (!ath.SyncAllowed()) {  //允许同步
			logger.log("udid:%s,SyncAllowed message read failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncAllowed message read success.", udid.c_str());
		
		//请求生成同步信息
		if (!ath.RequestingSync(remote_grappa)) {
			logger.log("udid:%s,RequestingSync failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		};
		logger.log("udid:%s,RequestingSync success.", udid.c_str());
		// 获取请求同步状态数据
		if (!ath.ReadyForSync(grappa)) {
			logger.log("udid:%s,ReadyForSync message read failed.\n", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,ReadyForSync message read success.", udid.c_str());
		if (!client->GenerateRs(grappa))	//调用远程服务器指令生成afsync.rs和afsync.rs.sig文件
		{
			logger.log("udid:%s,GenerateRs failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,GenerateRs success.", udid.c_str());
		//模拟itunes完成同步指令
		if (!ath.FinishedSyncingMetadata())
		{
			logger.log("udid:%s,FinishedSyncingMetadata failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,FinishedSyncingMetadata success.", udid.c_str());
		//读取同步状态
		if (!ath.SyncFinished())
		{
			logger.log("udid:%s,SyncFinished message read SyncFailed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncFinished message read ok.", udid.c_str());
		delete client;
		return AuthorizeReturnStatus::AuthorizeSuccess;
	}
	catch (const char* e)
	{
		logger.log(e);
		delete client;
		return AuthorizeReturnStatus::AuthorizeFailed;
	}
}


int DoPair(const char* udid)
{
	int i = 0;
	map<string, AMDeviceRef>::iterator iter;
	for (;;) {
		this_thread::sleep_for(chrono::milliseconds(2));
		iter = gudid.find(udid);
		if (iter == gudid.end()) {
			if (i++ >= 30) {
				logger.log("设备没有插入，初始化失败。");
				return -1;
			}
		}
		else {
			break;
		}
	}
	auto deviceHandle = iter->second;
	return DoPairEx(deviceHandle);
}


int DoPairEx(void* deviceHandle)
{
	iOSDeviceInfo iosdevice((AMDeviceRef)deviceHandle);
	auto rc = iosdevice.DoPair();

	if (rc == 0xe800001a) { //请打开密码锁定，进入ios主界面
		return -17;
	}
	else if (rc == 0xe8000096) { //请在设备端按下“信任”按钮
		return -19;
	}
	else if (rc == 0xe8000095) { //使用者按下了“不信任”按钮
		return -18;
	}
	return rc;
}

bool InstallApplicationEx(void* deviceHandle, const char* ipaPath)
{
	iOSDeviceInfo appleInfo((AMDeviceRef)deviceHandle);

	if (appleInfo.DoPair()) {
		if (iOSApplication::InstallCallback) iOSApplication::InstallCallback("fail", 100);
		logger.log("信认失败或没有通过。");
		return false;
	};

	iOSApplication iosapp = iOSApplication((AMDeviceRef)deviceHandle);

	auto retInstall = iosapp.Install(ipaPath);
	if (iOSApplication::InstallCallback)
		iOSApplication::InstallCallback(retInstall ? "success" : "fail", 100);
	return retInstall;
}

bool InstallApplication(const char* udid, const char* ipaPath) {
	auto iter = gudid.find(udid);
	if (iter == gudid.end())
	{
		logger.log("设备没有插入，初始化失败。");
		return false;
	}
	auto deviceHandle = iter->second;
	return InstallApplicationEx(deviceHandle, ipaPath);
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
