#include "apple_import.h"
#include "Sync.h"
char g_sLibraryID[] = "5AC547BA5322B210";  // Where is this come from?


void SendSyncRequest(ATHRef ath, unsigned int flags)
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
	CFStringRef sValue = CFStringCreateWithCString(NULL, "11.1.0.126", kCFStringEncodingUTF8);
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

	ATHostConnectionSendSyncRequest(ath, arrDataclasses, dictDataclassAnchors, dictHostInfo);

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
	for(;;){
		CFDictionaryRef dict = ATHostConnectionReadMessage(ath);
		CFStringRef sKey = CFStringCreateWithCString(NULL, "Command", kCFStringEncodingUTF8);
		CFStringRef sValue = CFDictionaryGetValue(dict, sKey);
		CFIndex len = CFStringGetLength(sValue);
		char *sCommand = (char *)malloc(len + 1);
		CFStringGetCString(sValue, sCommand, len + 1, kCFStringEncodingUTF8);
		CFRelease(sKey);
		CFRelease(sValue);
		if(strcmp("Ping",sCommand) == 0){
			ATHostConnectionSendPing(ath);
		}
		if(strcmp("SyncFinished", sCommand) == 0){
			break;
		}
	}
	ATHostConnectionRelease(ath);
	return 0;
}
