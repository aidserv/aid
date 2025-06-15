#ifndef AID2_H
#define AID2_H

#ifdef AID2_EXPORTS
#define AID2_API extern "C" __declspec(dllexport)
#else
#define AID2_API extern "C" __declspec(dllimport)
#endif

//��Ȩ����״̬��Ϣ
enum AuthorizeReturnStatus
{
	AuthorizeSuccess = 0,   //��Ȩ�ɹ�
	AuthorizeDopairingLocking = -17,  //ִ�������������ִ�ж�����⿪����
	AuthorizeFailed = 1,  //��Ȩʧ��
	AuthorizeDopairingTrust = -19,  //ִ���������Ҫ�����Σ�ִ�ж����밴����
	AuthorizeDopairingNotTrust = -18,  //ִ�������ʹ���߰��²�����
	AuthorizeDopairingUnplug = -8  //ʹ���߰ε����ֻ�
};

typedef void (*InstallApplicationFunc)(const char* status, const int percent);//��װ�����ص�����
typedef void (*ConnectCallbackFunc)(const char* udid, const char* DeviceName, const char* ProductType, const char* DeviceEnclosureColor, const char* MarketingName);//���Ӻ����ص�����
typedef void (*DisconnectCallbackFunc)(const char* udid);//�Ͽ��豸���ӻص�����


/*******************************************************
���÷����url
������url��ʽΪ�� IP:prot   ���� ����:�˿ں�
*******************************************************/
AID2_API void Setaidserv(const char* url);

/*******************************************************
���úͷ�����ͨ��֤��
������rootcert     ��֤��
	  clientkey    �ͻ���˽Կ
	  clientcert   �ͻ���֤��
*******************************************************/
AID2_API void TransferCertificate(const char* rootcert, const char* clientkey, const char* clientcert);

/*******************************************************
���̷߳�ʽ�������������������߳�
����ֵ���ɹ�Ϊtrue
**************************************************/
AID2_API bool StartListen();
/*******************************************************
ֹͣ�������رռ����߳�
����ֵ���ɹ�Ϊtrue
**************************************************/
AID2_API bool StopListen();

/*******************************************************
����udid��Ȩ
������udid
����ֵ���ɹ�ΪAuthorizeReturnStatus::AuthorizeSuccess
*******************************************************/
AID2_API AuthorizeReturnStatus AuthorizeDevice(const char * udid);

/*******************************************************
����deviceHandle��Ȩ
������deviceHandle
����ֵ���ɹ�ΪAuthorizeReturnStatus::AuthorizeSuccess
*******************************************************/
AID2_API AuthorizeReturnStatus AuthorizeDeviceEx(void* deviceHandle);

/******************************************************
* ִ�������Ϣ���״�����ֻ��ϻ�������κͲ����ΰ�ť
����ֵ��LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           �����Ի���,�����豸�˰��¡����Ρ���ť
		LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			ʹ���߰����ˡ������Ρ���ť��
		LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			�����������������ios������
		LOCKDOWN_E_MUX_ERROR                               =  -8			USB�����߶Ͽ����߽Ӵ�����
															   0			��Գɹ�
***************************************************************/
AID2_API int DoPair(const char* udid);

/******************************************************
* ִ�������Ϣ���״�����ֻ��ϻ�������κͲ����ΰ�ť
����ֵ��LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           �����Ի���,�����豸�˰��¡����Ρ���ť
		LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			ʹ���߰����ˡ������Ρ���ť��
		LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			�����������������ios������
		LOCKDOWN_E_MUX_ERROR                               =  -8			USB�����߶Ͽ����߽Ӵ�����
															 0			��Գɹ�
***************************************************************/
AID2_API int DoPairEx(void* deviceHandle);

/*******************************************************
����udid��װpath ��ipa ��
������deviceHandle
	  ipaPath ipa����·��
����ֵ���ɹ�Ϊtrue
*******************************************************/
AID2_API bool InstallApplicationEx(void* deviceHandle, const char* ipaPath);


/*******************************************************
����udid��װpath ��ipa ��
������udid
      ipaPath ipa����·��
����ֵ���ɹ�Ϊtrue
*******************************************************/
AID2_API bool InstallApplication(const char* udid,const char* ipaPath);

/*******************************************************
ע�ᰲװ�ص���������Ȩ��������Ҫ�����Ϣ����Ȩ���ͨ���ص�����֪ͨ
������callback �ص�����ָ��
**************************************************/
AID2_API void RegisterInstallCallback(InstallApplicationFunc callback);

/*******************************************************
�����豸����֪ͨ�Ļص�����
������callback �ص�����ָ��
**************************************************/
AID2_API void RegisterConnectCallback(ConnectCallbackFunc callback);

/*******************************************************
�Ͽ��豸����֪ͨ�Ļص�����
������callback �ص�����ָ��
**************************************************/
AID2_API void RegisterDisconnectCallback(DisconnectCallbackFunc callback);

#endif