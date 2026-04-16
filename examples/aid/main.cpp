#include <iostream>
#include <Windows.h>
#include<string>
#include <fstream>
#include "aid2/aid2.h"


using namespace std;
string gudid;
string gipaPath;

//const char rootcert_path[] = "certificate/ca.pem";
//const char clientcert_path[] = "certificate/client2025.pem";
//const char clientkey_path[] = "certificate/client.key";

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
		std::cout << ",Please unlock the password, enter the iOS home screen, and then plug and unplug the phone." << std::endl;
		return;
	case AuthorizeReturnStatus::AuthorizeDopairingTrust:
		std::cout << ",Please press the \"Trust\" button on your device before plugging or unplugging your phone." << std::endl;
		return;
	case AuthorizeReturnStatus::AuthorizeDopairingNotTrust:
		std::cout << ",If the user presses the \"Untrust\" button, please press the \"Trust\" button on the device before plugging or unplugging the phone." << std::endl;
		return;
	}
	auto ret = AuthorizeDevice(gudid.c_str());
	std::cout << "iOS devices，udid:" << gudid << (ret== AuthorizeReturnStatus::AuthorizeSuccess ? " Authorization successful" : " Authorization failed") << std::endl;
	auto retInstall = InstallApplication(gudid.c_str(), gipaPath.c_str());
	std::cout << "iOS devices，udid:" << gudid << " IPA package:" << gipaPath.c_str() << (retInstall ? " Installation successful" : " Installation failed.") << std::endl;
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
		MessageBox(NULL, L"IPA package installed successfully.", L"success", MB_OK | MB_ICONASTERISK);
	if (strcmp(status, "fail") == 0)
		MessageBox(NULL, L"IPA package installation failed.", L"fail", MB_OK | MB_ICONERROR);
}


int main(int argc, char* argv[], char* envp[])
{
	gipaPath = argv[1];
	//auto rootcert = get_file_contents(rootcert_path);
	//auto clientkey = get_file_contents(clientkey_path);
	//auto clientcert = get_file_contents(clientcert_path);
	
	RegisterInstallCallback(InstallApplicationInfo);  //安装ipa 回调函数
	RegisterConnectCallback(Connecting);		// 连接回调函数
	RegisterDisconnectCallback(Distinct);		// 断开事件

	//TransferCertificate(rootcert.c_str(), "certificate/client.key", "certificate/client2025.pem");  //设置ca根证书
	Setaidserv("http://aid.aidserv.cn");    //设置调用aidserver
	StartListen();  //开始监听设备插入
	std::cout << "Press Enter to stop..." << std::endl;
	std::cin.get();  // 阻止主线程退出
	StopListen();	 //停止监听
}