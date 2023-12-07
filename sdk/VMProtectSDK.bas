' protection
Public Declare Sub VMProtectBegin Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectBeginVirtualization Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectBeginMutation Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectBeginUltra Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectBeginVirtualizationLockByKey Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectBeginUltraLockByKey Lib "VMProtectSDK32.dll" (ByVal Ptr As Long)
Public Declare Sub VMProtectEnd Lib "VMProtectSDK32.dll" ()

' utils
Public Declare Function VMProtectIsProtected Lib "VMProtectSDK32.dll" () As Boolean
Public Declare Function VMProtectIsDebuggerPresent Lib "VMProtectSDK32.dll" (ByVal Value As Boolean) As Boolean
Public Declare Function VMProtectIsVirtualMachinePresent Lib "VMProtectSDK32.dll" () As Boolean
Public Declare Function VMProtectIsValidImageCRC Lib "VMProtectSDK32.dll" () As Boolean
Public Declare Function VMProtectDecryptString Lib "VMProtectSDK32.dll" Alias "VMProtectDecryptStringW" (ByVal Ptr As Long) As Long
Public Declare Function VMProtectFreeString Lib "VMProtectSDK32.dll" (ByVal Ptr As Long) As Boolean

' licensing
Public Const SERIAL_STATE_FLAG_CORRUPTED = 1
Public Const SERIAL_STATE_FLAG_INVALID = 2
Public Const SERIAL_STATE_FLAG_BLACKLISTED = 4
Public Const SERIAL_STATE_FLAG_DATE_EXPIRED = 8
Public Const SERIAL_STATE_FLAG_RUNNING_TIME_OVER = 16
Public Const SERIAL_STATE_FLAG_BAD_HWID = 32
Public Const SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED = 64

Public Type VMProtectDate
    wYear As Integer
    bMonth As Byte
    bDay As Byte
End Type

Public Type VMProtectSerialNumberData
    nState As Long
    wUserName(1 To 256) As Integer
    wEMail(1 To 256) As Integer
    dtExpire As VMProtectDate
    dtMaxBuild As VMProtectDate
    bRunningTime As Long
    nUserDataLength As Byte
    bUserData(1 To 255) As Byte
End Type

Public Declare Function VMProtectSetSerialNumber Lib "VMProtectSDK32.dll" (ByVal Serial As String) As Long
Public Declare Function VMProtectGetSerialNumberState Lib "VMProtectSDK32.dll" () As Long
Public Declare Function VMProtectGetSerialNumberData Lib "VMProtectSDK32.dll" (ByRef Data As VMProtectSerialNumberData, ByVal Size As Long) As Boolean
Public Declare Function VMProtectGetCurrentHWID Lib "VMProtectSDK32.dll" (ByVal HWID As String, ByVal Size As Long) As Long

' activation
Public Const ACTIVATION_OK = 0
Public Const ACTIVATION_SMALL_BUFFER = 1
Public Const ACTIVATION_NO_CONNECTION = 2
Public Const ACTIVATION_BAD_REPLY = 3
Public Const ACTIVATION_BANNED = 4
Public Const ACTIVATION_CORRUPTED = 5
Public Const ACTIVATION_BAD_CODE = 6
Public Const ACTIVATION_ALREADY_USED = 7
Public Const ACTIVATION_SERIAL_UNKNOWN = 8
Public Const ACTIVATION_EXPIRED = 9
Public Const ACTIVATION_NOT_AVAILABLE = 10

Public Declare Function VMProtectActivateLicense Lib "VMProtectSDK32.dll" (ByVal Code As String, ByVal Serial As String, ByVal Size As Long) As Long
Public Declare Function VMProtectDeactivateLicense Lib "VMProtectSDK32.dll" (ByVal Serial As String) As Long
Public Declare Function VMProtectGetOfflineActivationString Lib "VMProtectSDK32.dll" (ByVal Code As String, ByVal Buf As String, ByVal Size As Long) As Long
Public Declare Function VMProtectGetOfflineDeactivationString Lib "VMProtectSDK32.dll" (ByVal Serial As String, ByVal Buf As String, ByVal Size As Long) As Long

' StrFromPtr
Private Declare Function StrLen Lib "kernel32.dll" Alias "lstrlenW" (ByVal Str As Long) As Long
Private Declare Sub CopyMemory Lib "kernel32.dll" Alias "RtlMoveMemory" (ByVal Dest As Long, ByVal Source As Long, ByVal Length As Long)
Public Function StrFromPtr(ByVal Ptr As Long) As String
    Dim res As String
    Dim chars As Long
    chars = StrLen(Ptr)
    res = String$(chars, 0)
    CopyMemory StrPtr(res), Ptr, chars * 2
    StrFromPtr = res
End Function
