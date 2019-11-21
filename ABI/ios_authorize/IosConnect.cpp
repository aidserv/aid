#include "ABI/ios_authorize/ios_auth.h"
#include <cstdio>

void device_notification_callback(struct AMDeviceNotificationCallbackInformation *CallbackInfo)
{
	am_device *deviceHandle = CallbackInfo->deviceHandle;
	//printf("In device_notification_callback, msgType is %d\n", CallbackInfo->msgType);

	switch (CallbackInfo->msgType)
	{
	case ADNCI_MSG_CONNECTED:
		printf("Device %p connected\n", deviceHandle);
		if (deviceHandleConnected == NULL)
			deviceHandleConnected = deviceHandle;
		break;

	case ADNCI_MSG_DISCONNECTED:
		printf("Device %p disconnected\n", deviceHandle);
		break;

	case 3:
		printf("Unsubscribed\n");
		break;

	default:
		printf("Unknown message %d\n", CallbackInfo->msgType);
	}
}

void __cdecl ConnectIosDevice()
{
	void *subscribe;
	int rc = AMDeviceNotificationSubscribe(device_notification_callback, 0,0,0, &subscribe);
	if (rc < 0)
	{
		printf("AMDeviceNotificationSubscribe() failed.\n");
		exit(0);
	}

	printf("Waiting for USB connection...\n");
	while (true)
	{
		Sleep(100);
		if (deviceHandleConnected != NULL)
		{
			break;
		}
	}

	while (1)
	{
		// Physically connected
		rc = AMDeviceConnect(deviceHandleConnected);
		if (rc)
		{
			printf("AMDeviceConnect() failed.\n");
			exit(0);
		}

		AMDeviceIsPaired(deviceHandleConnected);
		rc = AMDeviceValidatePairing(deviceHandleConnected);
		if (rc == 0)
		{
			// Paired
			//
			rc = AMDeviceStartSession(deviceHandleConnected);
			if (rc)
			{
				printf("AMDeviceStartSession() failed.\n");
				exit(0);
			}
			break;
		}

		// Do pairing
		bool extstyle = true;
		if (!extstyle)
		{
			rc = AMDevicePair(deviceHandleConnected);
		}
		else
		{
			CFDictionaryRef dictOptions = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);
			CFStringRef key = CFStringCreateWithCString(NULL, "ExtendedPairingErrors", kCFStringEncodingUTF8);
			CFDictionarySetValue(dictOptions, key, *pkCFBooleanTrue);
			rc = AMDevicePairWithOptions(deviceHandleConnected, dictOptions);
		}

		if (rc == 0xe800001a)
		{
			printf("请打开密码锁定，进入ios主界面\n");
		}
		else if (rc == 0xe8000096)
		{
			// Waiting for user to press "trust" on ios device
			printf("请在设备端按下“信任”按钮\n");
		}
		else if (rc == 0xe8000095)
		{
			printf("用户按下了“不信任”按钮\n");
			Sleep(2000);
			exit(0);
		}
		else if (rc == 0)
		{
			break;
		}
		else
		{
			printf("AMDevicePair error code: %08X\n", rc);
		}

		rc = AMDeviceDisconnect(deviceHandleConnected);
		if (rc)
		{
			printf("AMDeviceDisconnect() failed.\n");
			exit(0);
		}

		Sleep(3000);
	}

	printf("Everything is OK. Device connected.\n");

	rc = AMDeviceStopSession(deviceHandleConnected);
	if (rc)
	{
		printf("AMDeviceStopSession() failed.\n");
		exit(0);
	}

	rc = AMDeviceDisconnect(deviceHandleConnected);
	if (rc)
	{
		printf("AMDeviceDisconnect() failed.\n");
		exit(0);
	}
	return; //
}


int __cdecl ConnectIOSDevice(AMDeviceRef device)
{
	int ret;
	ret = AMDeviceConnect(device);
	if (ret)
	{
		printf("AMDeviceConnect() failed.\n");
		return ret;
	}

	AMDeviceIsPaired(device);
	ret = AMDeviceValidatePairing(device);
	if (ret == 0)
	{
		ret = AMDeviceStartSession(device);
		if (ret)
		{
			printf("AMDeviceStartSession() failed.\n");
			return ret;
		}
	}

	return ret;
}

int __cdecl CloseIOSDevice(AMDeviceRef device)
{
	int ret;
	ret = AMDeviceStopSession(device);
	if (ret)
	{
		printf("AMDeviceStopSession() failed.\n");
		return ret;
	}
	ret = AMDeviceDisconnect(device);
	if (ret)
	{
		printf("AMDeviceDisconnect() failed.\n");
		return ret;
	}

	return ret;
}