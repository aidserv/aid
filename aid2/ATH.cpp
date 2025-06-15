#include "ATH.h"
#include "Logger.h"
namespace aid2 {

	ATH::ATH(string udid) {
		CFStringRef sUDID = CFStringCreateWithCString(NULL, udid.c_str(), kCFStringEncodingUTF8);
		CFStringRef sLibrary = CFStringCreateWithCString(NULL, m_LibraryID.c_str(), kCFStringEncodingUTF8);

		m_ath = ATHostConnectionCreateWithLibrary(sLibrary, sUDID, 0);
		if (m_ath == NULL) {
			CFRelease(sUDID);
			CFRelease(sLibrary);
			logger.log("new ATH error:ATHostConnectionCreateWithLibrary failed.");
			throw "new ATH error:ATHostConnectionCreateWithLibrary failed.";
		}
		CFRelease(sUDID);
		CFRelease(sLibrary);
	}

	aid2::ATH::~ATH()
	{
		ATHostConnectionDestroy(m_ath);
	}
	bool ATH::SyncAllowed()
	{
		bool bret = false;
		while (true)
		{
			CFDictionaryRef dict = ATHostConnectionReadMessage(m_ath);
			//读取命令行
			CFStringRef sKey = CFStringCreateWithCString(NULL, "Command", kCFStringEncodingUTF8);
			CFStringRef sValue = CFDictionaryGetValue(dict, sKey);
			CFIndex cmd_len = CFStringGetLength(sValue);
			string sCommand(cmd_len, '\0');
			CFStringGetCString(sValue, (char*)sCommand.data(), cmd_len + 1, kCFStringEncodingUTF8);
			CFRelease(sKey);
			CFRelease(sValue);

			logger.log("read SyncAllowed for ReceiveMessage:%s", sCommand.c_str());

			if (sCommand == "SyncAllowed") {
				bret = true;
				break;
			}
			if (sCommand == "Ping")   //超时
			{
				break;
			}
		}
		return bret;
	}
	bool ATH::RequestingSync(const string& grappa)
	{
		CFMutableArrayRef arrDataclasses = CFArrayCreateMutable(NULL, 0, kCFTypeArrayCallBacks);
		CFStringRef sKeyBag = CFStringCreateWithCString(NULL, "Keybag", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sKeyBag);
		CFRelease(sKeyBag);


		CFMutableDictionaryRef dictDataclassAnchors = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);
		CFMutableArrayRef arrSyncedDataclasses = CFArrayCreateMutable(NULL, 0, kCFTypeArrayCallBacks);
		CFMutableDictionaryRef dictHostInfo = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);

		CFStringRef sKey = CFStringCreateWithCString(NULL, "Version", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, m_Version.c_str(), kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictHostInfo, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);

		sKey = CFStringCreateWithCString(NULL, "SyncedDataclasses", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictHostInfo, sKey, arrSyncedDataclasses);
		CFRelease(sKey);

		sKey = CFStringCreateWithCString(NULL, "SyncHostName", kCFStringEncodingUTF8);
		sValue = CFStringCreateWithCString(NULL, m_ComputerName.c_str(), kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictHostInfo, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);

		sKey = CFStringCreateWithCString(NULL, "LibraryID", kCFStringEncodingUTF8);
		sValue = CFStringCreateWithCString(NULL, m_LibraryID.c_str(), kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictHostInfo, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);

		sKey = CFStringCreateWithCString(NULL, "Grappa", kCFStringEncodingUTF8);
		CFDataRef sValue_grappa = CFDataCreate(NULL, (unsigned char*)grappa.data(), grappa.length());
		CFDictionaryAddValue(dictHostInfo, sKey, sValue_grappa);
		CFRelease(sKey);
		CFRelease(sValue_grappa);


		CFMutableDictionaryRef dictRsp = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);

		sKey = CFStringCreateWithCString(NULL, "Dataclasses", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictRsp, sKey, arrDataclasses);
		CFRelease(sKey);

		sKey = CFStringCreateWithCString(NULL, "DataclassAnchors", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictRsp, sKey, dictDataclassAnchors);
		CFRelease(sKey);

		sKey = CFStringCreateWithCString(NULL, "HostInfo", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictRsp, sKey, dictHostInfo);
		CFRelease(sKey);

		sKey = CFStringCreateWithCString(NULL, "RequestingSync", kCFStringEncodingUTF8);
		auto rspMsg = ATCFMessageCreate(ATHostConnectionGetCurrentSessionNumber(m_ath), sKey, dictRsp);
		CFRelease(sKey);

		ATHostConnectionSendMessage(m_ath, rspMsg);

		CFRelease(arrDataclasses);
		CFRelease(arrSyncedDataclasses);
		CFRelease(dictDataclassAnchors);
		CFRelease(dictHostInfo);
		CFRelease(dictRsp);
		CFRelease(rspMsg);
		

		return true;
	}
	bool ATH::ReadyForSync(string& grappa)
	{
		bool bret = false;
		while (true)
		{
			CFDictionaryRef dict = ATHostConnectionReadMessage(m_ath);
			//读取命令行
			CFStringRef sKey = CFStringCreateWithCString(NULL, "Command", kCFStringEncodingUTF8);
			CFStringRef sValue = CFDictionaryGetValue(dict, sKey);
			CFIndex cmd_len = CFStringGetLength(sValue);
			string sCommand(cmd_len, '\0');
			CFStringGetCString(sValue, (char*)sCommand.data(), cmd_len + 1, kCFStringEncodingUTF8);
			CFRelease(sKey);
			CFRelease(sValue);
			if (sCommand == "ReadyForSync") {
				CFStringRef sKey = CFStringCreateWithCString(NULL, "Params", kCFStringEncodingUTF8);
				CFDictionaryRef DictParams = CFDictionaryGetValue(dict, sKey);
				CFRelease(sKey);
				sKey = CFStringCreateWithCString(NULL, "DeviceInfo", kCFStringEncodingUTF8);
				CFDictionaryRef DictDeviceInfo = CFDictionaryGetValue(DictParams, sKey);
				CFRelease(sKey);
				sKey = CFStringCreateWithCString(NULL, "Grappa", kCFStringEncodingUTF8);
				CFDataRef DataGrappa = CFDictionaryGetValue(DictDeviceInfo, sKey);
				CFRelease(sKey);
				auto grappa_len = CFDataGetLength(DataGrappa);
				grappa.resize(grappa_len);
				CFDataGetBytes(DataGrappa, CFRangeMake(0, grappa_len), (unsigned char*)grappa.data());
				bret = true;
				break;
			}
			if (sCommand == "Ping")   //超时
			{
				break;
			}
		}
		return bret;
		
	}
	bool ATH::FinishedSyncingMetadata()
	{
		CFMutableDictionaryRef dictDataclassAnchors = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);
		CFMutableDictionaryRef dictSyncTypes = CFDictionaryCreateMutable(NULL, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Keybag", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
		ATHostConnectionSendMetadataSyncFinished(m_ath, dictSyncTypes, dictDataclassAnchors);
		CFRelease(dictDataclassAnchors);
		CFRelease(dictSyncTypes);
		return true;
	}
	bool ATH::SyncFinished()
	{
		bool bret = false;
		while (true)
		{
			CFDictionaryRef dict = ATHostConnectionReadMessage(m_ath);
			//读取命令行
			CFStringRef sKey = CFStringCreateWithCString(NULL, "Command", kCFStringEncodingUTF8);
			CFStringRef sValue = CFDictionaryGetValue(dict, sKey);
			CFIndex cmd_len = CFStringGetLength(sValue);
			string sCommand(cmd_len, '\0');
			CFStringGetCString(sValue, (char*)sCommand.data(), cmd_len + 1, kCFStringEncodingUTF8);
			CFRelease(sKey);
			CFRelease(sValue);
			if (sCommand == "SyncFinished") {
				bret = true;
				break;
			}
			if (sCommand == "Ping" || sCommand =="SyncFailed")   //超时
			{
				break;
			}
		}
		return bret;
	}

}