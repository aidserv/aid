#ifndef ABI_IOS_AUTHORIZE_MOBILEDEVICE_H_
#define ABI_IOS_AUTHORIZE_MOBILEDEVICE_H_

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	/*
#if defined(WIN32)
	#include <ConditionalMacros.h>
	#include <CoreFoundation.h>
	typedef unsigned int mach_error_t;
#elif defined(__APPLE__)
	#include <CoreFoundation/CoreFoundation.h>
	#include <mach/error.h>
#endif	

#include <Corefoundation.h>

/* Error codes */
#define MDERR_APPLE_MOBILE  (err_system(0x3a))
#define MDERR_IPHONE        (err_sub(0))

/* Apple Mobile (AM*) errors */
#define ERR_MOBILE_DEVICE		0	// (?? What is the purpose of this anyway ??)

#define MDERR_OK                0
#define MDERR_SYSCALL           0x01
#define MDERR_OUT_OF_MEMORY     0x03
#define MDERR_QUERY_FAILED      0x04
#define MDERR_INVALID_ARGUMENT  0x0b
#define MDERR_DICT_NOT_LOADED   0x25

/* Apple File Connection (AFC*) errors */
#define MDERR_AFC_OUT_OF_MEMORY	0x03
#define MDERR_AFC_NOT_FOUND		0x08
#define MDERR_AFC_ACCESS_DENIED	0x09

/* USBMux errors */
#define MDERR_USBMUX_ARG_NULL   0x16
#define MDERR_USBMUX_FAILED     0xffffffff

/* Messages passed to device notification callbacks: passed as part of
 * am_device_notification_callback_info. */
#define ADNCI_MSG_CONNECTED     1
#define ADNCI_MSG_DISCONNECTED  2
#define ADNCI_MSG_UNKNOWN       3

#define AMD_IPHONE_PRODUCT_ID   0x1290
//#define AMD_IPHONE_SERIAL       ""

/* Services, found in /System/Library/Lockdown/Services.plist */

#define CFSTR(a) CFStringCreateWithCString(0,a,0x08000100)//CFStringCreateWithCString(0,a,0x08000100)//CFStringMakeConstantString(a)//
#define AMSVC_AFC                   CFSTR("com.apple.afc")
#define AMSVC_AFC2                  CFSTR("com.apple.afc2")
#define AMSVC_BACKUP                CFSTR("com.apple.mobilebackup")
#define AMSVC_CRASH_REPORT_COPY     CFSTR("com.apple.crashreportcopy")
#define AMSVC_DEBUG_IMAGE_MOUNT     CFSTR("com.apple.mobile.debug_image_mount")
#define AMSVC_NOTIFICATION_PROXY    CFSTR("com.apple.mobile.notification_proxy")
#define AMSVC_PURPLE_TEST           CFSTR("com.apple.purpletestr")
#define AMSVC_SOFTWARE_UPDATE       CFSTR("com.apple.mobile.software_update")
#define AMSVC_SYNC                  CFSTR("com.apple.mobilesync")
#define AMSVC_SCREENSHOT            CFSTR("com.apple.screenshotr")
#define AMSVC_SYSLOG_RELAY          CFSTR("com.apple.syslog_relay")
#define AMSVC_SYSTEM_PROFILER       CFSTR("com.apple.mobile.system_profiler")
#define AMDEVICE_ID                 CFSTR("UniqueDeviceID")

typedef unsigned int afc_error_t;
typedef unsigned int usbmux_error_t;
typedef unsigned int mach_error_t;

/* mode 2 = read, mode 3 = write; unknown = 0 */
//#define AFC_FILEMODE_READ			1
//#define AFC_FILEMODE_WRITE			3

#define kCFURLPOSIXPathStyle        2

#define FILEREAD_BUFFER_SIZE	(1<<16)	// transfers split in 64 kB buffer chunks

#pragma pack(push, 1)

struct am_recovery_device;

struct am_device_notification_callback_info {
    struct am_device *dev;  /* 0    device */ 
    unsigned int msg;       /* 4    one of ADNCI_MSG_* */
};

/* The type of the device restore notification callback functions.
 * TODO: change to correct type. */
typedef void (*am_restore_device_notification_callback)(struct
    am_recovery_device *);

/* This is a CoreFoundation object of class AMRecoveryModeDevice. */
struct am_recovery_device {
    unsigned char unknown0[8];                          /* 0 */
    am_restore_device_notification_callback callback;   /* 8 */
    void *user_info;                                    /* 12 */
    unsigned char unknown1[12];                         /* 16 */
    unsigned int readwrite_pipe;                        /* 28 */
    unsigned char read_pipe;                            /* 32 */
    unsigned char write_ctrl_pipe;                      /* 33 */
    unsigned char read_unknown_pipe;                    /* 34 */
    unsigned char write_file_pipe;                      /* 35 */
    unsigned char write_input_pipe;                     /* 36 */
};

/* A CoreFoundation object of class AMRestoreModeDevice. */
struct am_restore_device {
    unsigned char unknown[32];
    int port;
};

/* The type of the device notification callback function. */
typedef void(*am_device_notification_callback)(struct
    am_device_notification_callback_info *);

/* The type of the _AMDDeviceAttached function.
 * TODO: change to correct type. */
typedef void *amd_device_attached_callback;

/* The type of the device restore notification callback functions.
 * TODO: change to correct type. */
typedef void (*am_restore_device_notification_callback)(struct
    am_recovery_device *);
 
struct am_device {
    unsigned char unknown0[16]; /* 0 - zero */
    unsigned int device_id;     /* 16 */
    unsigned int product_id;    /* 20 - set to AMD_IPHONE_PRODUCT_ID */
    char *serial;               /* 24 - set to AMD_IPHONE_SERIAL */
    unsigned int unknown1;      /* 28 */
    unsigned char unknown2[4];  /* 32 */
    unsigned int lockdown_conn; /* 36 */
    unsigned char unknown3[8];  /* 40 */
};

struct am_device_notification {
    unsigned int unknown0;                      /* 0 */
    unsigned int unknown1;                      /* 4 */
    unsigned int unknown2;                      /* 8 */
    am_device_notification_callback callback;   /* 12 */ 
    unsigned int unknown3;                      /* 16 */
};

struct afc_device_info {
    unsigned char unknown[12];  /* 0 */
};

struct afc_directory {
    unsigned char unknown[1];   /* size unknown */
};

struct afc_dictionary {
    unsigned char unknown[1];   /* size unknown */
};

typedef unsigned long long afc_file_ref;
typedef unsigned long AT_HANDLE;
struct usbmux_listener_1 {                  /* offset   value in iTunes */
    unsigned int unknown0;                  /* 0        1 */
    unsigned char *unknown1;                /* 4        ptr, maybe device? */
    amd_device_attached_callback callback;  /* 8        _AMDDeviceAttached */
    unsigned int unknown3;                  /* 12 */
    unsigned int unknown4;                  /* 16 */
    unsigned int unknown5;                  /* 20 */
};

struct usbmux_listener_2 {
    unsigned char unknown0[4144];
};

struct am_bootloader_control_packet {
    unsigned char opcode;       /* 0 */
    unsigned char length;       /* 1 */
    unsigned char magic[2];     /* 2: 0x34, 0x12 */
    //unsigned char payload[0];   /* 4 */
};

typedef struct {
	WIN32_FIND_DATA findData;
	afc_directory *pHandle;
	char *remotePath;
	unsigned int iPodSize;
	unsigned int iPodBlocks;
} t_iPodFileInfo;

#pragma pack(pop)


#ifdef __cplusplus
};
#endif

#endif