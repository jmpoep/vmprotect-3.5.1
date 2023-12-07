#ifndef UTILS_H
#define UTILS_H

void ShowMessage(const VMP_CHAR *message);

#ifdef VMP_GNU
#elif defined(WIN_DRIVER)
HMODULE GetModuleHandleA(const char *name);
#else
void *InternalGetProcAddress(HMODULE module, const char *proc_name);

__forceinline void InitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString)
{
	if (SourceString)
		DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(UNICODE_NULL);
	else
		DestinationString->MaximumLength = DestinationString->Length = 0;

	DestinationString->Buffer = (PWCH)SourceString;
}
#endif

#endif