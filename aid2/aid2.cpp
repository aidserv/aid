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
				string udid = DeviceManager::get_udid(deviceHandle);
				DeviceManager::add_device(udid, deviceHandle);
				logger.log("Start Device.");
				logger.log("Device %p connected,udid:%s", deviceHandle, udid.c_str());
				// ���ӻص�
				if (ConnectCallback) { // ���ӻص�
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
		logger.log("������USB���������豸 %p ����connect.", deviceHandle);
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
		if (retDoPair<0) logger.log("����ʧ�ܻ�û��ͨ����");
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
		ATH ath = ATH(udid);  //new��ͬ��ʵ��

		if (!client->GenerateGrappa(remote_grappa))  //Grappa��������
		{
			logger.log("udid:%s,RemoteGetGrappa failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,RemoteGetGrappa success.", udid);

		if (!ath.SyncAllowed()) {  //����ͬ��
			logger.log("udid:%s,SyncAllowed message read failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,SyncAllowed message read success.", udid);

		//��������ͬ����Ϣ
		if (!ath.RequestingSync(remote_grappa)) {
			logger.log("udid:%s,RequestingSync failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		};
		logger.log("udid:%s,RequestingSync success.", udid);
		// ��ȡ����ͬ��״̬����
		if (!ath.ReadyForSync(grappa)) {
			logger.log("udid:%s,ReadyForSync message read failed.\n", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,ReadyForSync message read success.", udid);
		if (!client->GenerateRs(grappa))	//����Զ�̷�����ָ������afsync.rs��afsync.rs.sig�ļ�
		{
			logger.log("udid:%s,GenerateRs failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,GenerateRs success.", udid);
		//ģ��itunes���ͬ��ָ��
		if (!ath.FinishedSyncingMetadata())
		{
			logger.log("udid:%s,FinishedSyncingMetadata failed.", udid);
			return AuthorizeReturnStatus::AuthorizeFailed;
		}
		logger.log("udid:%s,FinishedSyncingMetadata success.", udid);
		//��ȡͬ��״̬
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
		logger.log("�豸û�в��룬��ʼ��ʧ�ܡ�");
		return AuthorizeReturnStatus::AuthorizeFailed;
	}
	return appleInfo->DoPair();
}

bool InstallApplication(const char* udid, const char* ipaPath) {
	auto retDoPair = DoPair(udid);
	if (retDoPair) {
		if (retDoPair < 0) logger.log("����ʧ�ܻ�û��ͨ����");
		return false;
	};
	iOSApplication iosapp = iOSApplication((AMDeviceRef)DeviceManager::get_handle(udid));
	auto retInstall = iosapp.Install(ipaPath);
	if (iOSApplication::InstallCallback)
		iOSApplication::InstallCallback(retInstall ? "success" : "fail", 100);
	return retInstall;
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
