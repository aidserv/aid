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
static map<string, AMDeviceRef> gudid; //udid���豸���ӳ��
static future<void> gfutListen;
static CFRunLoopRef grunLoop;
static ConnectCallbackFunc ConnectCallback = nullptr; // ���ӻص�ָ�뺯��ָ�����
static DisconnectCallbackFunc  DisconnectCallback = nullptr; // �Ͽ����ӻص�����ָ�����
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
				// ���ӻص�
				if (ConnectCallback) { // ���ӻص�
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
		logger.log("������USB���������豸 %p ����connect.", deviceHandle);
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
				logger.log("�豸û�в��룬��ʼ��ʧ�ܡ�");
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
		logger.log("����ʧ�ܻ�û��ͨ����");
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
		ATH ath = ATH(udid);  //new��ͬ��ʵ��
		
		if (!client->GenerateGrappa(remote_grappa))  //Grappa��������
		{
			logger.log("udid:%s,RemoteGetGrappa failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,RemoteGetGrappa success.", udid.c_str());

		if (!ath.SyncAllowed()) {  //����ͬ��
			logger.log("udid:%s,SyncAllowed message read failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncAllowed message read success.", udid.c_str());
		
		//��������ͬ����Ϣ
		if (!ath.RequestingSync(remote_grappa)) {
			logger.log("udid:%s,RequestingSync failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		};
		logger.log("udid:%s,RequestingSync success.", udid.c_str());
		// ��ȡ����ͬ��״̬����
		if (!ath.ReadyForSync(grappa)) {
			logger.log("udid:%s,ReadyForSync message read failed.\n", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,ReadyForSync message read success.", udid.c_str());
		if (!client->GenerateRs(grappa))	//����Զ�̷�����ָ������afsync.rs��afsync.rs.sig�ļ�
		{
			logger.log("udid:%s,GenerateRs failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,GenerateRs success.", udid.c_str());
		//ģ��itunes���ͬ��ָ��
		if (!ath.FinishedSyncingMetadata())
		{
			logger.log("udid:%s,FinishedSyncingMetadata failed.", udid.c_str());
			delete client;
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,FinishedSyncingMetadata success.", udid.c_str());
		//��ȡͬ��״̬
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
				logger.log("�豸û�в��룬��ʼ��ʧ�ܡ�");
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

	if (rc == 0xe800001a) { //�����������������ios������
		return -17;
	}
	else if (rc == 0xe8000096) { //�����豸�˰��¡����Ρ���ť
		return -19;
	}
	else if (rc == 0xe8000095) { //ʹ���߰����ˡ������Ρ���ť
		return -18;
	}
	return rc;
}

bool InstallApplicationEx(void* deviceHandle, const char* ipaPath)
{
	iOSDeviceInfo appleInfo((AMDeviceRef)deviceHandle);

	if (appleInfo.DoPair()) {
		if (iOSApplication::InstallCallback) iOSApplication::InstallCallback("fail", 100);
		logger.log("����ʧ�ܻ�û��ͨ����");
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
		logger.log("�豸û�в��룬��ʼ��ʧ�ܡ�");
		return false;
	}
	auto deviceHandle = iter->second;
	return InstallApplicationEx(deviceHandle, ipaPath);
}


//ע�ᰲװ�ص���������Ȩ��������Ҫ�����Ϣ����Ȩ���ͨ���ص�����֪ͨ
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
