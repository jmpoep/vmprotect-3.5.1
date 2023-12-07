#pragma once

#define VMP_IMPORT __declspec(dllimport)
#define VMP_API __stdcall
#define VMP_WCHAR wchar_t
#ifdef _WIN64
	#pragma comment(lib, "VMProtectDDK64.lib")
#else
	#pragma comment(lib, "VMProtectDDK32.lib")
#endif // _WIN64

#ifdef __cplusplus
extern "C" {
#endif

// protection
VMP_IMPORT void VMP_API VMProtectBegin(const char *);
VMP_IMPORT void VMP_API VMProtectBeginVirtualization(const char *);
VMP_IMPORT void VMP_API VMProtectBeginMutation(const char *);
VMP_IMPORT void VMP_API VMProtectBeginUltra(const char *);
VMP_IMPORT void VMP_API VMProtectBeginVirtualizationLockByKey(const char *);
VMP_IMPORT void VMP_API VMProtectBeginUltraLockByKey(const char *);
VMP_IMPORT void VMP_API VMProtectEnd(void);

// utils
VMP_IMPORT BOOLEAN VMP_API VMProtectIsProtected();
VMP_IMPORT BOOLEAN VMP_API VMProtectIsDebuggerPresent(BOOLEAN); // IRQL = PASSIVE_LEVEL
VMP_IMPORT BOOLEAN VMP_API VMProtectIsVirtualMachinePresent(void); // IRQL = PASSIVE_LEVEL
VMP_IMPORT BOOLEAN VMP_API VMProtectIsValidImageCRC(void);
VMP_IMPORT const char * VMP_API VMProtectDecryptStringA(const char *value);
VMP_IMPORT const VMP_WCHAR * VMP_API VMProtectDecryptStringW(const VMP_WCHAR *value);
VMP_IMPORT BOOLEAN VMP_API VMProtectFreeString(const void *value);

// licensing
enum VMProtectSerialStateFlags
{
	SERIAL_STATE_SUCCESS				= 0,
	SERIAL_STATE_FLAG_CORRUPTED			= 0x00000001,
	SERIAL_STATE_FLAG_INVALID			= 0x00000002,
	SERIAL_STATE_FLAG_BLACKLISTED		= 0x00000004,
	SERIAL_STATE_FLAG_DATE_EXPIRED		= 0x00000008,
	SERIAL_STATE_FLAG_RUNNING_TIME_OVER	= 0x00000010,
	SERIAL_STATE_FLAG_BAD_HWID			= 0x00000020,
	SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED	= 0x00000040,
};

#pragma pack(push, 1)
typedef struct
{
	unsigned short	wYear;
	unsigned char	bMonth;
	unsigned char	bDay;
} VMProtectDate;

typedef struct
{
	int				nState;				// VMProtectSerialStateFlags
	VMP_WCHAR		wUserName[256];		// user name
	VMP_WCHAR		wEMail[256];		// email
	VMProtectDate	dtExpire;			// date of serial number expiration
	VMProtectDate	dtMaxBuild;			// max date of build, that will accept this key
	int				bRunningTime;		// running time in minutes
	unsigned char	nUserDataLength;	// length of user data in bUserData
	unsigned char	bUserData[255];		// up to 255 bytes of user data
} VMProtectSerialNumberData;
#pragma pack(pop)

VMP_IMPORT int VMP_API VMProtectSetSerialNumber(const char *serial);
VMP_IMPORT int VMP_API VMProtectGetSerialNumberState();
VMP_IMPORT BOOLEAN VMP_API VMProtectGetSerialNumberData(VMProtectSerialNumberData *data, int size);
VMP_IMPORT int VMP_API VMProtectGetCurrentHWID(char *hwid, int size);

// activation
enum VMProtectActivationFlags
{
	ACTIVATION_OK = 0,
	ACTIVATION_SMALL_BUFFER,
	ACTIVATION_NO_CONNECTION,
	ACTIVATION_BAD_REPLY,
	ACTIVATION_BANNED,
	ACTIVATION_CORRUPTED,
	ACTIVATION_BAD_CODE,
	ACTIVATION_ALREADY_USED,
	ACTIVATION_SERIAL_UNKNOWN,
	ACTIVATION_EXPIRED,
	ACTIVATION_NOT_AVAILABLE
};

VMP_IMPORT int VMP_API VMProtectActivateLicense(const char *code, char *serial, int size);
VMP_IMPORT int VMP_API VMProtectDeactivateLicense(const char *serial);
VMP_IMPORT int VMP_API VMProtectGetOfflineActivationString(const char *code, char *buf, int size);
VMP_IMPORT int VMP_API VMProtectGetOfflineDeactivationString(const char *serial, char *buf, int size);

#ifdef __cplusplus
}
#endif