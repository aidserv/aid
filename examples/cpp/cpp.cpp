#include <iostream>
#include <Windows.h>
#include "aid2/aid2.h"
#include <thread>
#include<string>
#include <fstream>

using namespace std;
string gudid;
string gipaPath;

const char rootcert_path[] = "certificate/ca.pem";
const char clientcert_path[] = "certificate/client2025.pem";
const char clientkey_path[] = "certificate/client.key";

static string get_file_contents(const char* fpath)
{
	std::ifstream finstream(fpath);
	std::string contents;
	contents.assign((std::istreambuf_iterator<char>(finstream)),
		std::istreambuf_iterator<char>());
	finstream.close();
	return contents;
}

// 插入连接事件
void Connecting(const char* udid, const char* DeviceName, const char* ProductType, const char* DeviceEnclosureColor, const char* MarketingName)
{
	gudid = udid;
	std::cout << "Connecting udid:" << udid << ',' << DeviceName << ',' << ProductType << ", " << DeviceEnclosureColor << ", " << MarketingName << std::endl;
	auto retDoPair = DoPair(gudid.c_str());
	switch (retDoPair)
	{
	case AuthorizeReturnStatus::AuthorizeDopairingLocking:
		std::cout << "，请打开密码锁定，进入ios主界面，在插拔手机。" << std::endl;
		return;
	case AuthorizeReturnStatus::AuthorizeDopairingTrust:
		std::cout << "，请在设备端按下“信任”按钮，在插拔手机。" << std::endl;
		return;
	case AuthorizeReturnStatus::AuthorizeDopairingNotTrust:
		std::cout << "，使用者按下了“不信任”按钮，请在设备端按下“信任”按钮，在插拔手机。" << std::endl;
		return;
	}
	auto ret = AuthorizeDevice(gudid.c_str());
	std::cout << "iOS设备，udid:" << gudid << (ret== AuthorizeReturnStatus::AuthorizeSuccess ? " 授权成功" : " 授权失败") << std::endl;
	auto retInstall = InstallApplication(gudid.c_str(), gipaPath.c_str());
	std::cout << "iOS设备，udid:" << gudid << " ipa包：" << gipaPath.c_str() << (retInstall ? " 安装成功" : " 安装失败") << std::endl;
}


// 断开事件
void Distinct(const char* udid)
{
	std::cout << "Distinct udid:" << udid << std::endl;
}


//安装事件通知 回调函数
void InstallApplicationInfo(const char* status, const int percent) {
	std::cout << "[" << percent << "%]," << status << "." << std::endl;
	if (strcmp(status, "success") == 0)
		MessageBox(NULL, L"安装ipa包成功", L"成功", MB_OK | MB_ICONASTERISK);
	if (strcmp(status, "fail") == 0)
		MessageBox(NULL, L"安装ipa包失败", L"失败", MB_OK | MB_ICONERROR);
}


int main(int argc, char* argv[], char* envp[])
{
	gipaPath = argv[1];
	auto rootcert = get_file_contents(rootcert_path);
	auto clientkey = get_file_contents(clientkey_path);
	auto clientcert = get_file_contents(clientcert_path);
	
	RegisterInstallCallback(InstallApplicationInfo);  //安装ipa 回调函数
	RegisterConnectCallback(Connecting);		// 连接回调函数
	RegisterDisconnectCallback(Distinct);		// 断开事件

	TransferCertificate(rootcert.c_str(), "certificate/client.key", "certificate/client2025.pem");  //设置ca根证书
	Setaidserv("https://aid.aidserv.cn:60080");    //设置调用aidserver
	StartListen();
	std::cout << "按回车键停止..." << std::endl;
	std::cin.get();  // 阻止主线程退出
	StopListen();
}