unit VMProtectSDK;

interface

{$IF NOT DECLARED(PAnsiChar)}
type
  PAnsiChar = PUTF8Char;
{$IFEND}

// protection
  procedure VMProtectBegin(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectBeginVirtualization(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectBeginMutation(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectBeginUltra(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectBeginVirtualizationLockByKey(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectBeginUltraLockByKey(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  procedure VMProtectEnd; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};

// utils
  function VMProtectIsProtected: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectIsDebuggerPresent(CheckKernelMode: Boolean): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectIsVirtualMachinePresent: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectIsValidImageCRC: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectDecryptStringA(Value: PAnsiChar): PAnsiChar; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectDecryptStringW(Value: PWideChar): PWideChar; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectFreeString(Value: Pointer): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};

// licensing
type
  TVMProtectDate = packed record
   wYear: Word;
   bMonth: Byte;
   bDay: Byte;
  end;

  PVMProtectSerialNumberData = ^TVMProtectSerialNumberData;
  TVMProtectSerialNumberData = packed record
   nState: Longword;
   wUserName: array [0..255] of WideChar;
   wEMail: array [0..255] of WideChar;
   dtExpire: TVMProtectDate;
   dtMaxBuild: TVMProtectDate;
   bRunningTime: Longword;
   nUserDataLength: Byte;
   bUserData: array [0..254] of Byte;
  end;

const
  SERIAL_STATE_SUCCESS					= 0;
  SERIAL_STATE_FLAG_CORRUPTED			= $00000001;
  SERIAL_STATE_FLAG_INVALID				= $00000002;
  SERIAL_STATE_FLAG_BLACKLISTED			= $00000004;
  SERIAL_STATE_FLAG_DATE_EXPIRED		= $00000008;
  SERIAL_STATE_FLAG_RUNNING_TIME_OVER	= $00000010;
  SERIAL_STATE_FLAG_BAD_HWID			= $00000020;
  SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED	= $00000040;

  function VMProtectSetSerialNumber(SerialNumber: PAnsiChar): Longword; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectGetSerialNumberState: Longword; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectGetSerialNumberData(Data: PVMProtectSerialNumberData; DataSize: Integer): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectGetCurrentHWID(Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};

// activation
const
  ACTIVATION_OK             = 0;
  ACTIVATION_SMALL_BUFFER   = 1;
  ACTIVATION_NO_CONNECTION  = 2;
  ACTIVATION_BAD_REPLY      = 3;
  ACTIVATION_BANNED         = 4;
  ACTIVATION_CORRUPTED      = 5;
  ACTIVATION_BAD_CODE       = 6;
  ACTIVATION_ALREADY_USED   = 7;
  ACTIVATION_SERIAL_UNKNOWN = 8;
  ACTIVATION_EXPIRED        = 9;
  ACTIVATION_NOT_AVAILABLE  = 10;

  function VMProtectActivateLicense(ActivationCode: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectDeactivateLicense(SerialNumber: PAnsiChar): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectGetOfflineActivationString(ActivationCode: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};
  function VMProtectGetOfflineDeactivationString(SerialNumber: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF};

implementation

{$IFDEF WIN64}
  const VMProtectDLLName = 'VMProtectSDK64.dll';
{$ELSE}
{$IFDEF WIN32}
  const VMProtectDLLName = 'VMProtectSDK32.dll';
{$ELSE}
{$IFDEF DARWIN}
  {$LINKLIB libVMProtectSDK.dylib}
{$ELSE}
{$IFDEF MACOS}
  const VMProtectDLLName = 'libVMProtectSDK.dylib';
  {$IFNDEF MACOS64}
    {$DEFINE MACOS32}
  {$ENDIF}
{$ELSE}
{$IFDEF LINUX32}
  const VMProtectDLLName = 'libVMProtectSDK32.so';
{$ELSE}
{$IFDEF LINUX64}
  const VMProtectDLLName = 'libVMProtectSDK64.so';
{$ELSE}
  {$FATAL Unsupported OS!!!}
{$ENDIF}
{$ENDIF}
{$ENDIF}
{$ENDIF}
{$ENDIF}
{$ENDIF}

procedure VMProtectBegin(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBegin'{$ENDIF};
procedure VMProtectBeginVirtualization(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBeginVirtualization'{$ENDIF};
procedure VMProtectBeginMutation(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBeginMutation'{$ENDIF};
procedure VMProtectBeginUltra(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBeginUltra'{$ENDIF};
procedure VMProtectBeginVirtualizationLockByKey(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBeginVirtualizationLockByKey'{$ENDIF};
procedure VMProtectBeginUltraLockByKey(MarkerName: PAnsiChar); {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectBeginUltraLockByKey'{$ENDIF};
procedure VMProtectEnd; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectEnd'{$ENDIF};
function VMProtectIsProtected: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectIsProtected'{$ENDIF};
function VMProtectIsDebuggerPresent(CheckKernelMode: Boolean): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectIsDebuggerPresent'{$ENDIF};
function VMProtectIsVirtualMachinePresent: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectIsVirtualMachinePresent'{$ENDIF};
function VMProtectIsValidImageCRC: Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectIsValidImageCRC'{$ENDIF};
function VMProtectDecryptStringA(Value: PAnsiChar): PAnsiChar; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectDecryptStringA'{$ENDIF};
function VMProtectDecryptStringW(Value: PWideChar): PWideChar; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectDecryptStringW'{$ENDIF};
function VMProtectFreeString(Value: Pointer): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectFreeString'{$ENDIF};
function VMProtectSetSerialNumber(SerialNumber: PAnsiChar): Longword; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectSetSerialNumber'{$ENDIF};
function VMProtectGetSerialNumberState: Longword; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectGetSerialNumberState'{$ENDIF};
function VMProtectGetSerialNumberData(Data: PVMProtectSerialNumberData; DataSize: Integer): Boolean; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectGetSerialNumberData'{$ENDIF};
function VMProtectGetCurrentHWID(Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectGetCurrentHWID'{$ENDIF};
function VMProtectActivateLicense(ActivationCode: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectActivateLicense'{$ENDIF};
function VMProtectDeactivateLicense(SerialNumber: PAnsiChar): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectDeactivateLicense'{$ENDIF};
function VMProtectGetOfflineActivationString(ActivationCode: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectGetOfflineActivationString'{$ENDIF};
function VMProtectGetOfflineDeactivationString(SerialNumber: PAnsiChar; Buffer: PAnsiChar; BufferLen: Integer): Integer; {$IFDEF MSWINDOWS} stdcall {$ELSE} cdecl {$ENDIF}; external {$IFNDEF DARWIN}VMProtectDLLName{$ENDIF} {$IFDEF MACOS32} name '_VMProtectGetOfflineDeactivationString'{$ENDIF};

end.