; protection
VMProtectBegin PROTO :DWORD
VMProtectBeginVirtualization PROTO :DWORD
VMProtectBeginMutation PROTO :DWORD
VMProtectBeginUltra PROTO :DWORD
VMProtectBeginVirtualizationLockByKey PROTO :DWORD
VMProtectBeginUltraLockByKey PROTO :DWORD
VMProtectEnd PROTO

; utils
VMProtectIsProtected PROTO
VMProtectIsDebuggerPresent PROTO :DWORD
VMProtectIsVirtualMachinePresent PROTO
VMProtectIsDebuggerPresent PROTO :DWORD
VMProtectDecryptStringA PROTO :DWORD
VMProtectDecryptStringW PROTO :DWORD
VMProtectFreeString PROTO :DWORD

; licensing
SERIAL_STATE_SUCCESS				equ 0
SERIAL_STATE_FLAG_CORRUPTED			equ	0x00000001
SERIAL_STATE_FLAG_INVALID			equ	0x00000002
SERIAL_STATE_FLAG_BLACKLISTED		equ	0x00000004
SERIAL_STATE_FLAG_DATE_EXPIRED		equ	0x00000008
SERIAL_STATE_FLAG_RUNNING_TIME_OVER	equ	0x00000010
SERIAL_STATE_FLAG_BAD_HWID			equ	0x00000020
SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED	equ	0x00000040
	
VMProtectDate STRUCT 	
	wYear			WORD	?
	bMonth			BYTE	?
	bDay			BYTE	?
VMProtectDate ENDS	

VMProtectSerialNumberData STRUCT
	nState			DWORD			?	; VMProtectSerialStateFlags
	wUserName		WCHAR[256]			; user name
	wEMail			WCHAR[256]			; email
	dtExpire		VMProtectDate	<>	; date of serial number expiration
	dtMaxBuild		VMProtectDate	<>	; max date of build, that will accept this key
	bRunningTime	DWORD			?	; running time in minutes
	nUserDataLength	BYTE			?	; length of user data in bUserData
	bUserData		BYTE[255]		?	; up to 255 bytes of user data
VMProtectSerialNumberData ENDS

VMProtectSetSerialNumber PROTO :DWORD
VMProtectGetSerialNumberState PROTO
VMProtectGetSerialNumberData PROTO :DWORD,:DWORD
VMProtectGetCurrentHWID PROTO :DWORD,:DWORD

; activation
ACTIVATION_OK				equ 0
ACTIVATION_SMALL_BUFFER		equ 1
ACTIVATION_NO_CONNECTION	equ 2
ACTIVATION_BAD_REPLY		equ 3
ACTIVATION_BANNED			equ 4
ACTIVATION_CORRUPTED		equ 5
ACTIVATION_BAD_CODE			equ 6
ACTIVATION_ALREADY_USED		equ 7
ACTIVATION_SERIAL_UNKNOWN	equ 8
ACTIVATION_NOT_AVAILABLE	equ 9	

VMProtectActivateLicense PROTO :DWORD,:DWORD,:DWORD
VMProtectDeactivateLicense PROTO :DWORD
VMProtectGetOfflineActivationString PROTO :DWORD,:DWORD,:DWORD
VMProtectGetOfflineDeactivationString PROTO :DWORD,:DWORD,:DWORD

; libs
includelib VMProtectSDK32.lib