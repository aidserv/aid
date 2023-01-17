#include "apple_import.h"
#include "Sync.h"
#include <ABI/thirdparty/glog/scoped_ptr.h>
#include <ABI/thirdparty/openssl/evp.h>
#include <smartbot/passport/itunes_native_interface.h>
#include <smartbot/passport/itunes_internal_interface.h>
#include <smartbot/thirdparty/glog/logging.h>

char g_sLibraryID[] = "5AC547BA5322B210";  // Where is this come from?
bool g_startSync = false;
unsigned char* g_grappa = NULL;
unsigned long g_grappa_len = 0;
unsigned int g_GrappaID;

void SendSyncRequest(ATHRef ath, unsigned int flags, string grappa)
{
	char sComputerName[256];
	unsigned long len = 256;
	GetComputerNameA(sComputerName, &len);

	CFMutableArrayRef arrDataclasses = CFArrayCreateMutable(NULL, 0, pkCFTypeArrayCallBacks);

	if ((flags & SYNC_RINGTONE) != 0)
	{
		CFStringRef sRingtong = CFStringCreateWithCString(NULL, "Ringtong", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sRingtong);
		CFRelease(sRingtong);
	}
	if ((flags & SYNC_MEDIA) != 0)
	{
		CFStringRef sMedia = CFStringCreateWithCString(NULL, "Media", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sMedia);
		CFRelease(sMedia);
	}
	if ((flags & SYNC_VOICEMEMO) != 0)
	{
		CFStringRef sVoiceMemo = CFStringCreateWithCString(NULL, "VoiceMemo", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sVoiceMemo);
		CFRelease(sVoiceMemo);
	}
	if ((flags & SYNC_BOOK) != 0)
	{
		CFStringRef sBook = CFStringCreateWithCString(NULL, "Book", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sBook);
		CFRelease(sBook);
	}
	if ((flags & SYNC_PHOTO) != 0)
	{
		CFStringRef sPhoto = CFStringCreateWithCString(NULL, "Photo", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sPhoto);
		CFRelease(sPhoto);
	}
	if ((flags & SYNC_DATA) != 0)
	{
		CFStringRef sData = CFStringCreateWithCString(NULL, "Data", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sData);
		CFRelease(sData);
	}
	if ((flags & SYNC_KEYBAG) != 0)
	{
		CFStringRef sKeyBag = CFStringCreateWithCString(NULL, "Keybag", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sKeyBag);
		CFRelease(sKeyBag);
	}
	if ((flags & SYNC_APPLICATION) != 0)
	{
		CFStringRef sApplication = CFStringCreateWithCString(NULL, "Application", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrDataclasses, sApplication);
		CFRelease(sApplication);
	}

	CFMutableDictionaryRef dictDataclassAnchors = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);

	if ((flags & SYNC_RINGTONG_DICT) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Ringtong", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);

		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);

		CFRelease(sKey);
		CFRelease(sValue);
	}
	if ((flags & SYNC_MEDIA_DICT) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Media", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);

		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);

		CFRelease(sKey);
		CFRelease(sValue);
	}
	if ((flags & SYNC_VOICEMEMO_DICT) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "VoiceMemo", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);

		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);

		CFRelease(sKey);
		CFRelease(sValue);
	}

	CFMutableArrayRef arrSyncedDataclasses = CFArrayCreateMutable(NULL, 0, pkCFTypeArrayCallBacks);

	if ((flags & SYNC_RINGTONG_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Ringtong", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_MEDIA_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Media", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_VOICEMEMO_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "VoiceMemo", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_BOOK_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Book", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_PHOTO_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Photo", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_DATA_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Data", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}
	if ((flags & SYNC_APPLICATION_ARR2) != 0)
	{
		CFStringRef sValue = CFStringCreateWithCString(NULL, "Application", kCFStringEncodingUTF8);
		CFArrayAppendValue(arrSyncedDataclasses, sValue);
		CFRelease(sValue);
	}

	CFMutableDictionaryRef dictHostInfo = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);

	CFStringRef sKey = CFStringCreateWithCString(NULL, "Version", kCFStringEncodingUTF8);
	CFStringRef sValue = CFStringCreateWithCString(NULL, "12.1.0.71", kCFStringEncodingUTF8);
	CFDictionaryAddValue(dictHostInfo, sKey, sValue);
	CFRelease(sKey);
	CFRelease(sValue);

	sKey = CFStringCreateWithCString(NULL, "SyncedDataclasses", kCFStringEncodingUTF8);
	CFDictionaryAddValue(dictHostInfo, sKey, arrSyncedDataclasses);
	CFRelease(sKey);

	sKey = CFStringCreateWithCString(NULL, "SyncHostName", kCFStringEncodingUTF8);
	sValue = CFStringCreateWithCString(NULL, sComputerName, kCFStringEncodingUTF8);
	CFDictionaryAddValue(dictHostInfo, sKey, sValue);
	CFRelease(sKey);
	CFRelease(sValue);

	sKey = CFStringCreateWithCString(NULL, "LibraryID", kCFStringEncodingUTF8);
	sValue = CFStringCreateWithCString(NULL, g_sLibraryID, kCFStringEncodingUTF8);
	CFDictionaryAddValue(dictHostInfo, sKey, sValue);
	CFRelease(sKey);
	CFRelease(sValue);

	sKey = CFStringCreateWithCString(NULL, "Grappa", kCFStringEncodingUTF8);
	CFDataRef sValue_grappa = CFDataCreate(NULL, (unsigned char*)grappa.c_str(), grappa.length());
	CFDictionaryAddValue(dictHostInfo, sKey, sValue_grappa);
	CFRelease(sKey);
	CFRelease(sValue_grappa);


	CFMutableDictionaryRef dictRsp = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);
	
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
	auto rspMsg = ATCFMessageCreate(ATHostConnectionGetCurrentSessionNumber(ath), sKey, dictRsp);
	CFRelease(sKey);

	ATHostConnectionSendMessage(ath, rspMsg);

	CFRelease(arrDataclasses);
	CFRelease(arrSyncedDataclasses);
	CFRelease(dictDataclassAnchors);
	CFRelease(dictHostInfo);

	return;
}

void SendMetadataSyncFinished(ATHRef ath, unsigned int flags)
{
	CFMutableDictionaryRef dictDataclassAnchors = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);

	if ((flags & SYNC_FINISHED_BOOK) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Book", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);
	}

	if ((flags & SYNC_FINISHED_DATA) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Data", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);
	}

	if ((flags & SYNC_FINISHED_RINGTONE) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Ringtone", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);
	}

	if ((flags & SYNC_FINISHED_MEDIA) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Media", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);
	}

	if ((flags & SYNC_FINISHED_VOICEMEMO) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "VoiceMemo", kCFStringEncodingUTF8);
		CFStringRef sValue = CFStringCreateWithCString(NULL, "1", kCFStringEncodingUTF8);
		CFDictionaryAddValue(dictDataclassAnchors, sKey, sValue);
		CFRelease(sKey);
		CFRelease(sValue);
	}

	CFMutableDictionaryRef dictSyncTypes = CFDictionaryCreateMutable(NULL, 0, pkCFTypeDictionaryKeyCallBacks, pkCFTypeDictionaryValueCallBacks);

	if ((flags & SYNC_FINISHED_2_KEYBAG) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Keybag", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_APPLICATION) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Application", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_VOICEMEMO) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "VoiceMemo", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_RINGTONE) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Ringtone", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_MEDIA) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Media", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_BOOK) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Book", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	if ((flags & SYNC_FINISHED_2_DATA) != 0)
	{
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Data", kCFStringEncodingUTF8);
		int value = 1;
		CFNumberRef nValue = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
		CFDictionaryAddValue(dictSyncTypes, sKey, nValue);
		CFRelease(sKey);
	}

	ATHostConnectionSendMetadataSyncFinished(ath, dictSyncTypes, dictDataclassAnchors);

	CFRelease(dictDataclassAnchors);
	CFRelease(dictSyncTypes);
	return;
}

DWORD WINAPI ReceiveMessageThreadFunc(LPVOID lpParam)
{
	ATHRef ath = (ATHRef)lpParam;
	ATHostConnectionRetain(ath);
	for (;;) {
		CFDictionaryRef dict = ATHostConnectionReadMessage(ath);
		g_startSync = true;
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Command", kCFStringEncodingUTF8);
		CFStringRef sValue = CFDictionaryGetValue(dict, sKey);
		CFIndex len = CFStringGetLength(sValue);
		char* sCommand = (char*)malloc(len + 1);
		CFStringGetCString(sValue, sCommand, len + 1, kCFStringEncodingUTF8);
		CFRelease(sKey);
		CFRelease(sValue);
		if (strcmp("Ping", sCommand) == 0) {
			ATHostConnectionSendPing(ath);
		}
		if (strcmp("SyncFinished", sCommand) == 0) {
			break;
		}
		if (strcmp("ReadyForSync", sCommand) == 0) {


			CFStringRef sKey = CFStringCreateWithCString(NULL, "Params", kCFStringEncodingUTF8);
			CFDictionaryRef DictParams = CFDictionaryGetValue(dict, sKey);
			CFRelease(sKey);
			sKey = nullptr;
			sKey = CFStringCreateWithCString(NULL, "DeviceInfo", kCFStringEncodingUTF8);
			CFDictionaryRef DictDeviceInfo = CFDictionaryGetValue(DictParams, sKey);
			CFRelease(sKey);
			sKey = nullptr;
			sKey = CFStringCreateWithCString(NULL, "Grappa", kCFStringEncodingUTF8);
			CFDataRef DataGrappa = CFDictionaryGetValue(DictDeviceInfo, sKey);
			CFRelease(sKey);
			
			g_grappa_len = CFDataGetLength(DataGrappa);
			if(g_grappa != NULL) free(g_grappa);
			g_grappa = (unsigned char*)malloc(g_grappa_len + 1);
			CFDataGetBytes(DataGrappa, CFRangeMake(0, g_grappa_len), g_grappa);
			////unsigned char to base64
			//scoped_array<unsigned char> encode_info(new unsigned char[((g_grappa_len - 1) / 3 + 1) * 4 + 1]);
			//memset(encode_info.get(), 0, ((g_grappa_len - 1) / 3 + 1) * 4 + 1);
			//EVP_EncodeBlock(encode_info.get(), g_grappa, g_grappa_len);
			//std::string grappa_base64(reinterpret_cast<char*>(encode_info.get()));
			//std::cout << grappa_base64 << endl;
			
			CFRelease(DataGrappa);
			CFRelease(DictDeviceInfo);
			CFRelease(DictParams);
		}
	}
	ATHostConnectionRelease(ath);
	return 0;
}
