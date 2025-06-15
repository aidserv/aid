#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#ifndef WIN32
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <stdint.h>
#pragma pack(push, 1)

/*Messages passed to device notification callbacks */
#define ADNCI_MSG_CONNECTECD 1
#define ADNCI_MSG_DISCONNECTED 2
#define ADNCI_MSG_UNKNOWN 3
#define AMD_IPHONE_PRODUCT_ID 0x1290
#define AFC_FILEMODE_READ 1
#define AFC_FILEMODE_WRITE 2

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	typedef void *CFStringRef;
	typedef void *CFTypeRef;
	typedef void *CFMutableDataRef;
	typedef void *CFPropertyListRef;
	typedef void *CFDictionaryRef;
	typedef void *CFMutableDictionaryRef;
	typedef void *CFDataRef;
	typedef void *CFNumberRef;
	typedef void *CFAllocatorRef;
	typedef void *CFURLRef;
	typedef void *CFReadStreamRef;
	typedef void *CFArrayRef;
	typedef void *CFDateRef;
	typedef void *CFErrorRef;
	typedef void *CFMutableArrayRef;
	typedef void *CFBooleanRef;
	typedef void *CFTimeZoneRef;
#ifdef _WIN64
	typedef signed long long CFIndex;
#else
	typedef signed long CFIndex;
#endif  // _WIN64
	typedef int CFTypeID;
	typedef int CFStringEncoding;
	typedef int CFURLPathStyle; // posix=0, hfs=1, windows=2
	typedef void * CFBundleRef;
	typedef void * CFRunLoopRef;
	typedef double CFAbsoluteTime;
	typedef double CFTimeInterval;
	typedef const void * AFCRef;
	typedef unsigned __int64 AFCFileRef;
	typedef const void* ATHRef;



#ifdef WIN32
	typedef unsigned char Boolean;
	typedef uint8_t UInt8;
#endif

	typedef struct _CFRange
	{
		CFIndex location;
		CFIndex length;
	} CFRange;

	typedef struct _CFGregorianDate
	{
		int32_t year;
		int8_t month;
		int8_t day;
		int8_t hour;
		int8_t minute;
		double second;
	} CFGregorianDate;

	typedef struct _CFGregorianUnits
	{
		int32_t years;
		int32_t months;
		int32_t days;
		int32_t hours;
		int32_t minutes;
		double seconds;
	} CFGregorianUnits;

	enum
	{
		kCFStringEncodingInvalidId = -1L,
		kCFStringEncodingMacRoman = 0L,
		kCFStringEncodingMacChineseSimp = 25,
		kCFStringEncodingWindowsLatin1 = 0x0500, /* ANSI codepage 1252 */
		kCFStringEncodingISOLatin1 = 0x0201,	 /* ISO 8850 1 */
		kCFStringEncodingNextStepLatin = 0x0B01, /* NextStep encoding*/
		kCFStringEncodingASCII = 0x0600,		 /* 0..127 */
		kCFStringEncodingGB_18030_2000 = 0x0632,	/* gb18030±àÂë */
		kCFStringEncodingUnicode = 0x0100,		 /* kTextEncodingUnicodeDefault  + kTextEncodingDefaultFormat (aka kUnicode16BitFormat) */
		kCFStringEncodingUTF8 = 0x08000100,		 /* kTextEncodingUnicodeDefault + kUnicodeUTF8Format */
		kCFStringEncodingNonLossyASCII = 0x0BFF	 /* 7bit Unicode variants used by YellowBox & Java */
	};

	enum
	{
		kCFPropertyListImmutable = 0,
		kCFPropertyListMutableContainers,
		kCFPropertyListMutableContainersAndLeaves
	};

	typedef enum _CFPropertyListFormat
	{
		kCFPropertyListOpenStepFormat = 1,
		kCFPropertyListXMLFormat_v1_0 = 100,
		kCFPropertyListBinaryFormat_v1_0 = 200
	} CFPropertyListFormat;

	typedef enum _CFNumberType
	{
		/* Types from MacTypes.h */
		kCFNumberSInt8Type = 1,
		kCFNumberSInt16Type = 2,
		kCFNumberSInt32Type = 3,
		kCFNumberSInt64Type = 4,
		kCFNumberFloat32Type = 5,
		kCFNumberFloat64Type = 6, /* 64-bit IEEE 754 */
		/* Basic C types */
		kCFNumberCharType = 7,
		kCFNumberShortType = 8,
		kCFNumberIntType = 9,
		kCFNumberLongType = 10,
		kCFNumberLongLongType = 11,
		kCFNumberFloatType = 12,
		kCFNumberDoubleType = 13, /* Other */
		kCFNumberCFIndexType = 14,
		kCFNumberNSIntegerType = 15,
		kCFNumberMaxType = 16
	} CFNumberType;
#endif

	struct am_recovery_device;
	struct am_dfu_device;
	struct am_restore_device;
	struct am_device_notification_callback_info;

	typedef void (*BackupCallBack)(CFStringRef targetID, int percentOrErrorCode, int cookie, CFStringRef backupPath, void *, void *, void *, void *, void *, void *, void *, void *);
	typedef void (*am_restore_device_notification_callback)(struct am_recovery_device *, void *);
	typedef void (*am_dfu_device_notification_callback)(struct am_dfu_device *, void *);
	typedef void (*am_recovery_device_notification_callback)(struct am_restore_device *, int);
	typedef void (*am_device_notification_callback)(struct am_device_notification_callback_info *);

	struct am_recovery_device
	{
		unsigned char unk[8];
		am_restore_device_notification_callback callback;
		void *user_info;
		unsigned char unk1[12];
		unsigned int readwrite_pipe;
		unsigned char read_pipe;
		unsigned char write_ctrl_pipe;
		unsigned char read_unk_pipe;
		unsigned char write_unk_pipe;
		unsigned char write_input_pipe;
	};

	struct am_restore_device
	{
		unsigned char unk[32];
		int port;
	};

	struct am_dfu_device
	{
		unsigned char unk[32];
		int port;
	};

	struct am_device
	{
		unsigned char unk0[16];
		unsigned int device_id;
		unsigned int product_id;
		char *serial;
		unsigned int unk1;
		unsigned char unk2[4];
		unsigned int lockdown_conn;
		unsigned char unk3[8];
	};

	struct am_device_notification
	{
		unsigned int unk0;
		unsigned int unk1;
		unsigned int unk2;
		am_device_notification_callback callback;
		unsigned int unk3;
	};

	struct AMDeviceNotificationCallbackInformation {
		am_device* deviceHandle;
		unsigned int msgType;
	};

	typedef struct am_device* AMDeviceRef;


	struct afc_device_info {
		unsigned char unknown[12];  /* 0 */
	};

	struct afc_directory {
		unsigned char unknown[1];   /* size unknown */
	};

	struct afc_dictionary {
		unsigned char unknown[1];   /* size unknown */
	};


	enum kAMDError
	{
		// Token: 0x040033C0 RID: 13248
		kAMDAlreadyArchivedError = -402653094,
		// Token: 0x040033C1 RID: 13249
		kAMDApplicationAlreadyInstalledError = -402653130,
		// Token: 0x040033C2 RID: 13250
		kAMDApplicationMoveFailedError,
		// Token: 0x040033C3 RID: 13251
		kAMDApplicationSandboxFailedError = -402653127,
		// Token: 0x040033C4 RID: 13252
		kAMDApplicationSINFCaptureFailedError = -402653128,
		// Token: 0x040033C5 RID: 13253
		kAMDApplicationVerificationFailedError = -402653126,
		// Token: 0x040033C6 RID: 13254
		kAMDArchiveDestructionFailedError,
		// Token: 0x040033C7 RID: 13255
		kAMDBadHeaderError = -402653182,
		// Token: 0x040033C8 RID: 13256
		kAMDBundleVerificationFailedError = -402653124,
		// Token: 0x040033C9 RID: 13257
		kAMDBusyError = -402653167,
		// Token: 0x040033CA RID: 13258
		kAMDCarrierBundleCopyFailedError = -402653123,
		// Token: 0x040033CB RID: 13259
		kAMDCarrierBundleDirectoryCreationFailedError,
		// Token: 0x040033CC RID: 13260
		kAMDCarrierBundleMissingSupportedSIMsError,
		// Token: 0x040033CD RID: 13261
		kAMDCheckinTimeoutError = -402653148,
		// Token: 0x040033CE RID: 13262
		kAMDCommCenterNotificationFailedError = -402653120,
		// Token: 0x040033CF RID: 13263
		kAMDContainerCreationFailedError,
		// Token: 0x040033D0 RID: 13264
		kAMDContainerP0wnFailedError,
		// Token: 0x040033D1 RID: 13265
		kAMDContainerRemovalFailedError,
		// Token: 0x040033D2 RID: 13266
		kAMDCryptoError = -402653166,
		// Token: 0x040033D3 RID: 13267
		kAMDDigestFailedError = -402653135,
		// Token: 0x040033D4 RID: 13268
		kAMDEmbeddedProfileInstallFailedError = -402653116,
		// Token: 0x040033D5 RID: 13269
		kAMDEOFError = -402653170,
		// Token: 0x040033D6 RID: 13270
		kAMDErrorError = -402653115,
		// Token: 0x040033D7 RID: 13271
		kAMDExecutableTwiddleFailedError,
		// Token: 0x040033D8 RID: 13272
		kAMDExistenceCheckFailedError,
		// Token: 0x040033D9 RID: 13273
		kAMDFileExistsError = -402653168,
		// Token: 0x040033DA RID: 13274
		kAMDGetProhibitedError = -402653162,
		// Token: 0x040033DB RID: 13275
		kAMDImmutableValueError = -402653159,
		// Token: 0x040033DC RID: 13276
		kAMDInstallMapUpdateFailedError = -402653112,
		// Token: 0x040033DD RID: 13277
		kAMDInvalidActivationRecordError = -402653146,
		// Token: 0x040033DE RID: 13278
		kAMDInvalidArgumentError = -402653177,
		// Token: 0x040033DF RID: 13279
		kAMDInvalidCheckinError = -402653149,
		// Token: 0x040033E0 RID: 13280
		kAMDInvalidDiskImageError = -402653133,
		// Token: 0x040033E1 RID: 13281
		kAMDInvalidHostIDError = -402653156,
		// Token: 0x040033E2 RID: 13282
		kAMDInvalidResponseError = -402653165,
		// Token: 0x040033E3 RID: 13283
		kAMDInvalidServiceError = -402653150,
		// Token: 0x040033E4 RID: 13284
		kAMDInvalidSessionIDError = -402653152,
		// Token: 0x040033E5 RID: 13285
		kAMDIsDirectoryError = -402653175,
		// Token: 0x040033E6 RID: 13286
		kAMDiTunesArtworkCaptureFailedError = -402653096,
		// Token: 0x040033E7 RID: 13287
		kAMDiTunesMetadataCaptureFailedError,
		// Token: 0x040033E8 RID: 13288
		kAMDManifestCaptureFailedError = -402653111,
		// Token: 0x040033E9 RID: 13289
		kAMDMapGenerationFailedError,
		// Token: 0x040033EA RID: 13290
		kAMDMissingActivationRecordError = -402653145,
		// Token: 0x040033EB RID: 13291
		kAMDMissingBundleExecutableError = -402653109,
		// Token: 0x040033EC RID: 13292
		kAMDMissingBundleIdentifierError,
		// Token: 0x040033ED RID: 13293
		kAMDMissingBundlePathError,
		// Token: 0x040033EE RID: 13294
		kAMDMissingContainerError,
		// Token: 0x040033EF RID: 13295
		kAMDMissingDigestError = -402653132,
		// Token: 0x040033F0 RID: 13296
		kAMDMissingHostIDError = -402653157,
		// Token: 0x040033F1 RID: 13297
		kAMDMissingImageTypeError = -402653136,
		// Token: 0x040033F2 RID: 13298
		kAMDMissingKeyError = -402653164,
		// Token: 0x040033F3 RID: 13299
		kAMDMissingOptionsError = -402653137,
		// Token: 0x040033F4 RID: 13300
		kAMDMissingPairRecordError = -402653147,
		// Token: 0x040033F5 RID: 13301
		kAMDMissingServiceError = -402653151,
		// Token: 0x040033F6 RID: 13302
		kAMDMissingSessionIDError = -402653153,
		// Token: 0x040033F7 RID: 13303
		kAMDMissingValueError = -402653163,
		// Token: 0x040033F8 RID: 13304
		kAMDMuxError = -402653131,
		// Token: 0x040033F9 RID: 13305
		kAMDNoResourcesError = -402653181,
		// Token: 0x040033FA RID: 13306
		kAMDNotConnectedError = -402653173,
		// Token: 0x040033FB RID: 13307
		kAMDNotFoundError = -402653176,
		// Token: 0x040033FC RID: 13308
		kAMDNotificationFailedError = -402653105,
		// Token: 0x040033FD RID: 13309
		kAMDOverrunError = -402653171,
		// Token: 0x040033FE RID: 13310
		kAMDPackageExtractionFailedError = -402653104,
		// Token: 0x040033FF RID: 13311
		kAMDPackageInspectionFailedError,
		// Token: 0x04003400 RID: 13312
		kAMDPackageMoveFailedError,
		// Token: 0x04003401 RID: 13313
		kAMDPasswordProtectedError = -402653158,
		// Token: 0x04003402 RID: 13314
		kAMDPathConversionFailedError = -402653101,
		// Token: 0x04003403 RID: 13315
		kAMDPermissionError = -402653174,
		// Token: 0x04003404 RID: 13316
		kAMDProvisioningProfileNotValid = -402653140,
		// Token: 0x04003405 RID: 13317
		kAMDReadError = -402653180,
		// Token: 0x04003406 RID: 13318
		kAMDReceiveMessageError = -402653138,
		// Token: 0x04003407 RID: 13319
		kAMDRemoveProhibitedError = -402653160,
		// Token: 0x04003408 RID: 13320
		kAMDRestoreContainerFailedError = -402653100,
		// Token: 0x04003409 RID: 13321
		kAMDSeatbeltProfileRemovalFailedError,
		// Token: 0x0400340A RID: 13322
		kAMDSendMessageError = -402653139,
		// Token: 0x0400340B RID: 13323
		kAMDSessionActiveError = -402653155,
		// Token: 0x0400340C RID: 13324
		kAMDSessionInactiveError,
		// Token: 0x0400340D RID: 13325
		kAMDSetProhibitedError = -402653161,
		// Token: 0x0400340E RID: 13326
		kAMDStageCreationFailedError = -402653098,
		// Token: 0x0400340F RID: 13327
		kAMDStartServiceError = -402653134,
		// Token: 0x04003410 RID: 13328
		kAMDOk = 0,
		// Token: 0x04003411 RID: 13329
		kAMDSUFirmwareError = -402653141,
		// Token: 0x04003412 RID: 13330
		kAMDSUPatchError = -402653142,
		// Token: 0x04003413 RID: 13331
		kAMDSUVerificationError = -402653143,
		// Token: 0x04003414 RID: 13332
		kAMDSymlinkFailedError = -402653097,
		// Token: 0x04003415 RID: 13333
		kAMDTimeOutError = -402653172,
		// Token: 0x04003416 RID: 13334
		kAMDUndefinedError = -402653183,
		// Token: 0x04003417 RID: 13335
		kAMDUnknownPacketError = -402653178,
		// Token: 0x04003418 RID: 13336
		kAMDUnsupportedError = -402653169,
		// Token: 0x04003419 RID: 13337
		kAMDWriteError = -402653179,
		// Token: 0x0400341A RID: 13338
		kAMDWrongDroidError = -402653144,
		// Token: 0x0400341B RID: 13339
		kAMDTrustComputerError = -402653034,
		// Token: 0x0400341C RID: 13340
		kAMDiTunesVersionTooLow = -1
	};

	enum ConnectMode
	{
		// Token: 0x04002D5D RID: 11613
		UNKNOWN,
		// Token: 0x04002D5E RID: 11614
		USB,
		// Token: 0x04002D5F RID: 11615
		WIFI,
		// Token: 0x04002D60 RID: 11616
		USB_Install_Driver,
		// Token: 0x04002D61 RID: 11617
		Connected_iOS_Driver,
		// Token: 0x04002D62 RID: 11618
		USB_None_Debug = -1,
		// Token: 0x04002D63 RID: 11619
		USB_None_Driver = -2,
		// Token: 0x04002D64 RID: 11620
		None_Open_Power = -3,
		// Token: 0x04002D65 RID: 11621
		DisconnectedDevice = -4,
		// Token: 0x04002D66 RID: 11622
		DisabledDevice = -5,
		// Token: 0x04002D67 RID: 11623
		NotTrustediOSDevice = -6,
		// Token: 0x04002D68 RID: 11624
		RepeatConnectedDevice = -7,
		// Token: 0x04002D69 RID: 11625
		USBConnectedADBDisconnected = -8,
		// Token: 0x04002D6A RID: 11626
		NotDaemonOfAndroid = -9,
		// Token: 0x04002D6B RID: 11627
		NotUnauthorizedOfAndroid = -10
	};


#pragma pack(pop)

#ifdef __cplusplus
};
#endif
