#ifndef AID2_H
#define AID2_H

#ifdef AID2_EXPORTS
#define AID2_API extern "C" __declspec(dllexport)
#else
#define AID2_API extern "C" __declspec(dllimport)
#endif

//授权返回状态信息
enum AuthorizeReturnStatus
{
	AuthorizeSuccess = 0,   //授权成功
	AuthorizeDopairingLocking = -17,  //执行配对中锁屏，执行动作请解开锁屏
	AuthorizeFailed = 1,  //授权失败
	AuthorizeDopairingTrust = -19,  //执行配对中需要按信任，执行动作请按信任
	AuthorizeDopairingNotTrust = -18,  //执行配对中使用者按下不信任
	AuthorizeDopairingUnplug = -8  //使用者拔掉了手机
};

typedef void (*InstallApplicationFunc)(const char* status, const int percent);//安装函数回调定义
typedef void (*ConnectCallbackFunc)(const char* udid, const char* DeviceName, const char* ProductType, const char* DeviceEnclosureColor, const char* MarketingName);//连接函数回调定义
typedef void (*DisconnectCallbackFunc)(const char* udid);//断开设备连接回调定义


/*******************************************************
设置服务端url
参数：url格式为： IP:prot   或者 域名:端口号
*******************************************************/
AID2_API void Setaidserv(const char* url);

/*******************************************************
设置和服务器通信证书
参数：rootcert     根证书
	  clientkey    客户端私钥
	  clientcert   客户端证书
*******************************************************/
AID2_API void TransferCertificate(const char* rootcert, const char* clientkey, const char* clientcert);

/*******************************************************
以线程方式开户监听，不阻塞主线程
返回值：成功为true
**************************************************/
AID2_API bool StartListen();
/*******************************************************
停止监听，关闭监听线程
返回值：成功为true
**************************************************/
AID2_API bool StopListen();

/*******************************************************
根据udid授权
参数：udid
返回值：成功为AuthorizeReturnStatus::AuthorizeSuccess
*******************************************************/
AID2_API AuthorizeReturnStatus AuthorizeDevice(const char * udid);

/******************************************************
* 执行配对信息，首次配对手机上会出现信任和不信任按钮
返回值：LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING         = -19           弹出对话框,请在设备端按下“信任”按钮
		LOCKDOWN_E_USER_DENIED_PAIRING                     = -18			使用者按下了“不信任”按钮。
		LOCKDOWN_E_PASSWORD_PROTECTED                      = -17			请打开密码锁定，进入ios主界面
		LOCKDOWN_E_MUX_ERROR                               =  -8			USB数据线断开或者接触不良
															   0			配对成功
***************************************************************/
AID2_API int DoPair(const char* udid);

/*******************************************************
根据udid安装path 的ipa 包
参数：udid
      ipaPath ipa包的路径
返回值：成功为true
*******************************************************/
AID2_API bool InstallApplication(const char* udid,const char* ipaPath);

/*******************************************************
注册安装回调函数，授权过程中需要配对信息和授权结果通过回调函数通知
参数：callback 回调函数指针
**************************************************/
AID2_API void RegisterInstallCallback(InstallApplicationFunc callback);

/*******************************************************
插入设备连接通知的回调函数
参数：callback 回调函数指针
**************************************************/
AID2_API void RegisterConnectCallback(ConnectCallbackFunc callback);

/*******************************************************
断开设备连接通知的回调函数
参数：callback 回调函数指针
**************************************************/
AID2_API void RegisterDisconnectCallback(DisconnectCallbackFunc callback);

#endif