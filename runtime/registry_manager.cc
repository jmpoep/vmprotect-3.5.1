#ifdef WIN_DRIVER
#else
#include "common.h"
#include "utils.h"
#include "objects.h"

#include "crypto.h"
#include "core.h"
#include "file_manager.h"
#include "registry_manager.h"
#include "hook_manager.h"

// should be commented out in release builds
// #define CHECKED

#ifdef CHECKED
void XTrace0(LPCTSTR lpszText)
{
	::OutputDebugString(lpszText);
}

void XTrace(LPCTSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);
	int nBuf;
	TCHAR szBuffer[16384]; // get rid of this hard-coded buffer
	nBuf = _snwprintf_s(szBuffer, 16383, L"%p ", GetCurrentThreadId());
	nBuf += _vsnwprintf_s(szBuffer + nBuf, 16383 - nBuf, _TRUNCATE, lpszFormat, args);
	::OutputDebugString(szBuffer);
	va_end(args);
}
#endif

/**
 * hooked functions
 */

NTSTATUS WINAPI HookedNtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
}

NTSTATUS WINAPI HookedNtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtDeleteValueKey(KeyHandle, ValueName);
}

NTSTATUS WINAPI HookedNtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtCreateKey(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}

NTSTATUS WINAPI HookedNtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS WINAPI HookedNtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
}

NTSTATUS WINAPI HookedNtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtOpenKeyEx(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
}

NTSTATUS WINAPI HookedNtDeleteKey(HANDLE KeyHandle)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtDeleteKey(KeyHandle);
}

NTSTATUS WINAPI HookedNtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS WINAPI HookedNtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtEnumerateValueKey(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS WINAPI HookedNtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
	RegistryManager *registry_manager = Core::Instance()->registry_manager();
	return registry_manager->NtEnumerateKey(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

/**
 * RegistryValue
 */

RegistryValue::RegistryValue(const wchar_t *name)
	: name_(NULL), type_(REG_NONE), data_(NULL), size_(0)
{
	size_t size = wcslen(name) + 1;
	name_ = new wchar_t[size];
	memcpy(name_, name, size * sizeof(wchar_t));
}

RegistryValue::~RegistryValue()
{
	delete [] name_;
	delete [] data_;
}

void RegistryValue::SetValue(uint32_t type, void *data, uint32_t size)
{
	type_ = type;
	size_ = size;
	delete [] data_;
	if (size) {
		data_ = new uint8_t[size_];
		memcpy(data_, data, size_);
	} else {
		data_ = NULL;
	}
}

/**
 * RegistryKey
 */

RegistryKey::RegistryKey()
	: name_(NULL), owner_(NULL), is_real_(false), is_wow_(false), last_write_time_(0)
{

}

RegistryKey::RegistryKey(RegistryKey *owner, const wchar_t *name, bool is_real)
	: name_(NULL), owner_(owner), is_real_(is_real), is_wow_(false), last_write_time_(0)
{
	size_t size = wcslen(name) + 1;
	name_ = new wchar_t[size];
	memcpy(name_, name, size * sizeof(wchar_t));
}

RegistryKey::~RegistryKey()
{
	for (size_t i = 0; i < values_.size(); i++) {
		RegistryValue *value = values_[i];
		delete value;
	}
	values_.clear();

	for (size_t i = 0; i < keys_.size(); i++) {
		RegistryKey *key = keys_[i];
		delete key;
	}
	keys_.clear();

	delete [] name_;
}

RegistryValue *RegistryKey::GetValue(const wchar_t *value_name) const
{
	if (!value_name)
		value_name = L"";
	for (size_t i = 0; i < values_.size(); i++) {
		RegistryValue *value = values_[i];
		if (_wcsicmp(value_name, value->name()) == 0)
			return value;
	}
	return NULL;
}

RegistryValue *RegistryKey::AddValue(wchar_t *value_name)
{
	if (!value_name)
		value_name = L"";
	RegistryValue *value = new RegistryValue(value_name);
	values_.push_back(value);
	return value;
}

void RegistryKey::SetValue(wchar_t *value_name, uint32_t type, void *data, uint32_t size)
{
	RegistryValue *value = GetValue(value_name);
	if (!value)
		value = AddValue(value_name);
	value->SetValue(type, data, size);

	FILETIME system_time;
	GetSystemTimeAsFileTime(&system_time);
	last_write_time_ = static_cast<uint64_t>(system_time.dwHighDateTime) << 32 | static_cast<uint64_t>(system_time.dwLowDateTime);
}

bool RegistryKey::DeleteValue(wchar_t *value_name)
{
	if (!value_name)
		value_name = L"";
	for (size_t i = 0; i < values_.size(); i++) {
		RegistryValue *value = values_[i];
		if (_wcsicmp(value_name, value->name()) == 0) {
			delete value;
			values_.erase(i);
			return true;
		}
	}
	return false;
}

bool RegistryKey::is_wow_node(const wchar_t *name) const
{
	bool ret = (is_wow_ && _wcsicmp(L"Wow6432Node", name) == 0);
#ifdef CHECKED
	XTrace(L"is_wow_node: %s = %d\n", name, ret ? 1 : 0);
#endif
	return ret;
}

RegistryKey *RegistryKey::GetChild(const wchar_t *name) const
{
	if (!name || *name == 0)
		return NULL;

	if (is_wow_node(name))
		return const_cast<RegistryKey *>(this);

	for (size_t i = 0; i < keys_.size(); i++) {
		RegistryKey *key = keys_[i];
		if (_wcsicmp(name, key->name()) == 0)
			return key;
	}
	return NULL;
}

RegistryKey *RegistryKey::GetKey(const wchar_t *name) const
{
	if (!name || *name == 0)
		return NULL;

	if (*name == L'\\')
		name++;
	const wchar_t *sub_name = wcschr(name, L'\\');
	if (!sub_name)
		return GetChild(name);

	RegistryKey *key = GetChild(UnicodeString(name, sub_name - name).c_str());
	return key ? key->GetKey(sub_name) : NULL;
}

RegistryKey *RegistryKey::AddChild(const wchar_t *name, bool is_real, bool *is_exists)
{
	if (!name || *name == 0)
		return NULL;

	RegistryKey *key = is_wow_node(name) ? this : GetChild(name);
	if (is_exists)
		*is_exists = (key != NULL);
	if (!key) {
		key = new RegistryKey(this, name, is_real);
		keys_.push_back(key);
	}
	return key;
}

RegistryKey *RegistryKey::AddKey(const wchar_t *name, bool is_real, bool *is_exists)
{
	if (!name || *name == 0)
		return NULL;

	if (*name == L'\\')
		name++;
	const wchar_t *sub_name = wcschr(name, L'\\');
	if (!sub_name)
		return AddChild(name, is_real, is_exists);

	RegistryKey *key = AddChild(UnicodeString(name, sub_name - name).c_str(), is_real, is_exists);
	return key ? key->AddKey(sub_name, is_real, is_exists) : NULL;
}

bool RegistryKey::DeleteKey(RegistryKey *key)
{
	size_t index = keys_.find(key);
	if (index != -1) {
		keys_.erase(index);
		delete key;
		return true;
	}
	return false;
}

UnicodeString RegistryKey::full_name() const
{
	if (!owner_)
		return UnicodeString();
	return owner_->full_name() + L'\\' + name_;
}

/**
 * RegistryManager
 */

RegistryManager::RegistryManager(const uint8_t *data, HMODULE instance, const uint8_t *key, VirtualObjectList *objects)
	: instance_(instance)
	, data_(data)
	, objects_(objects)
	, nt_set_value_key_(NULL)
	, nt_delete_value_key_(NULL)
	, nt_create_key_(NULL)
	, nt_open_key_(NULL)
	, nt_open_key_ex_(NULL)
	, nt_query_value_key_(NULL)
	, nt_delete_key_(NULL)
	, nt_query_key_(NULL)
	, nt_enumerate_value_key_(NULL)
	, nt_enumerate_key_(NULL)
	, append_mode_(false)
{
	key_ = *(reinterpret_cast<const uint32_t *>(key));

//#ifndef _WIN64
	static const wchar_t *wow_keys[] = {
		L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes"
	};

	for (size_t i = 0; i < _countof(wow_keys); i++) {
		RegistryKey *key = cache_.AddKey(wow_keys[i], true, NULL);
		if (key)
			key->set_is_wow(true);
	}
//#endif
}

void RegistryManager::HookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	nt_set_value_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtSetValueKey"), &HookedNtSetValueKey);
	nt_delete_value_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtDeleteValueKey"), &HookedNtDeleteValueKey);
	nt_create_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtCreateKey"), &HookedNtCreateKey);
	nt_open_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtOpenKey"), &HookedNtOpenKey);
	nt_open_key_ex_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtOpenKeyEx"), &HookedNtOpenKeyEx, false);
	nt_query_value_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryValueKey"), &HookedNtQueryValueKey);
	nt_delete_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtDeleteKey"), &HookedNtDeleteKey);
	nt_query_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtQueryKey"), &HookedNtQueryKey);
	nt_enumerate_value_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtEnumerateValueKey"), &HookedNtEnumerateValueKey);
	nt_enumerate_key_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("NtEnumerateKey"), &HookedNtEnumerateKey);
	hook_manager.End();
}

void RegistryManager::UnhookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	hook_manager.UnhookAPI(nt_set_value_key_);
	hook_manager.UnhookAPI(nt_delete_value_key_);
	hook_manager.UnhookAPI(nt_create_key_);
	hook_manager.UnhookAPI(nt_open_key_);
	hook_manager.UnhookAPI(nt_open_key_ex_);
	hook_manager.UnhookAPI(nt_query_value_key_);
	hook_manager.UnhookAPI(nt_delete_key_);
	hook_manager.UnhookAPI(nt_query_key_);
	hook_manager.UnhookAPI(nt_enumerate_value_key_);
	hook_manager.UnhookAPI(nt_enumerate_key_);
	hook_manager.End();
}

REGISTRY_DIRECTORY RegistryManager::DecryptDirectory(const REGISTRY_DIRECTORY *directory_enc) const
{
	REGISTRY_DIRECTORY res;
	res.Reserved1 = directory_enc->Reserved1 ^ key_;
	res.Reserved2 = directory_enc->Reserved2 ^ key_;
	return res;
}

void RegistryManager::BeginRegisterServer()
{
	append_mode_ = true;
}

void RegistryManager::EndRegisterServer()
{
	append_mode_ = false;
}

NTSTATUS __forceinline RegistryManager::TrueNtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize)
{
	typedef NTSTATUS (WINAPI tNtSetValueKey)(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
	return reinterpret_cast<tNtSetValueKey *>(nt_set_value_key_)(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
}

NTSTATUS __forceinline RegistryManager::TrueNtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName)
{
	typedef NTSTATUS (WINAPI tNtDeleteValueKey)(HANDLE KeyHandle, PUNICODE_STRING ValueName);
	return reinterpret_cast<tNtDeleteValueKey *>(nt_delete_value_key_)(KeyHandle, ValueName);
}

NTSTATUS __forceinline RegistryManager::TrueNtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition)
{
	typedef NTSTATUS (WINAPI tNtCreateKey)(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition);
	return reinterpret_cast<tNtCreateKey *>(nt_create_key_)(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}

NTSTATUS __forceinline RegistryManager::TrueNtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtQueryValueKey)(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	return reinterpret_cast<tNtQueryValueKey *>(nt_query_value_key_)(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS __forceinline RegistryManager::TrueNtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
	typedef NTSTATUS (WINAPI tNtOpenKey)(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
	return reinterpret_cast<tNtOpenKey *>(nt_open_key_)(KeyHandle, DesiredAccess, ObjectAttributes);
}

NTSTATUS __forceinline RegistryManager::TrueNtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions)
{
	typedef NTSTATUS (WINAPI tNtOpenKeyEx)(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions);
	return reinterpret_cast<tNtOpenKeyEx *>(nt_open_key_ex_)(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
}

NTSTATUS __forceinline RegistryManager::TrueNtDeleteKey(HANDLE KeyHandle)
{
	typedef NTSTATUS (WINAPI tNtDeleteKey)(HANDLE KeyHandle);
	return reinterpret_cast<tNtDeleteKey *>(nt_delete_key_)(KeyHandle);
}

NTSTATUS __forceinline RegistryManager::TrueNtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtQueryKey)(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	return reinterpret_cast<tNtQueryKey *>(nt_query_key_)(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS __forceinline RegistryManager::TrueNtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtEnumerateValueKey)(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	return reinterpret_cast<tNtEnumerateValueKey *>(nt_enumerate_value_key_)(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS __forceinline RegistryManager::TrueNtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
	typedef NTSTATUS (WINAPI tNtEnumerateKey)(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	return reinterpret_cast<tNtEnumerateKey *>(nt_enumerate_key_)(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS __forceinline RegistryManager::TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength)
{
	return Core::Instance()->TrueNtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

NTSTATUS RegistryManager::NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize)
{
#ifdef CHECKED
	UnicodeString s(ValueName->Buffer, ValueName->Length / sizeof(wchar_t));
	XTrace(L"->NtSetValueKey %p %s\n", KeyHandle, s.c_str());
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			if ((object->access() & KEY_SET_VALUE) == 0)
			{
#ifdef CHECKED
				XTrace(L"NtSetValueKey STATUS_ACCESS_DENIED\n");
#endif
				return STATUS_ACCESS_DENIED;
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			try {
				key->SetValue(ValueName->Buffer, Type, Data, DataSize);
#ifdef CHECKED
				XTrace(L"NtSetValueKey STATUS_SUCCESS\n");
#endif
				return STATUS_SUCCESS;
			} catch(...) {
#ifdef CHECKED
				XTrace(L"NtSetValueKey STATUS_ACCESS_VIOLATION\n");
#endif
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	NTSTATUS ret = TrueNtSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
#ifdef CHECKED
	XTrace(L"TrueNtSetValueKey %p\n", ret);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName)
{
#ifdef CHECKED
	UnicodeString s(ValueName->Buffer, ValueName->Length / sizeof(wchar_t));
	XTrace(L"->NtDeleteValueKey %p %s\n", KeyHandle, s.c_str());
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			if ((object->access() & KEY_SET_VALUE) == 0)
			{
#ifdef CHECKED
				XTrace(L"NtDeleteValueKey STATUS_ACCESS_DENIED\n");
#endif
				return STATUS_ACCESS_DENIED;
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			try {
				NTSTATUS ret = key->DeleteValue(ValueName->Buffer) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND; 
#ifdef CHECKED
				XTrace(L"NtDeleteValueKey STATUS_SUCCESS\n");
#endif
				return ret;
			} catch(...) {
#ifdef CHECKED
				XTrace(L"NtSetValueKey STATUS_ACCESS_VIOLATION\n");
#endif
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	NTSTATUS ret = TrueNtDeleteValueKey(KeyHandle, ValueName);
#ifdef CHECKED
	XTrace(L"TrueNtDeleteValueKey %p\n", ret);
#endif
	return ret;
}

RegistryKey *RegistryManager::GetRootKey(HANDLE root, uint32_t *access, bool can_create)
{
	RegistryKey *res = NULL;
	if (root) {
		VirtualObject *object = objects_->GetKey(root);
		if (object && object->ref()) {
			// root is a virtual key
			res = static_cast<RegistryKey *>(object->ref());
			if (access)
				*access = object->access();
		} else {
			// root is a real key
			KEY_NAME_INFORMATION info;
			DWORD size;
			if (TrueNtQueryKey(root, KeyNameInformation, &info, sizeof(info), &size) == STATUS_BUFFER_OVERFLOW) {
				KEY_NAME_INFORMATION *data = reinterpret_cast<KEY_NAME_INFORMATION *>(new uint8_t[size]);
				if (NT_SUCCESS(TrueNtQueryKey(root, KeyNameInformation, data, size, &size))) {
					UnicodeString str(data->Name, data->NameLength / sizeof(wchar_t));
					res = can_create ? cache_.AddKey(str.c_str(), true, NULL) : cache_.GetKey(str.c_str());
					if (access) {
						if (object)
							*access = object->access();
						else {
							PUBLIC_OBJECT_BASIC_INFORMATION info;
							*access = (NT_SUCCESS(TrueNtQueryObject(root, ObjectBasicInformation, &info, sizeof(info), NULL))) ? info.GrantedAccess : 0;
						}
					}
				}
				delete [] data;
			}
		}
	} else {
		res = &cache_;
		if (access)
			*access = KEY_CREATE_SUB_KEY;
	}
	return res;
}

NTSTATUS RegistryManager::NtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition)
{
#ifdef CHECKED
	UnicodeString s(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t));
	XTrace(L"->NtCreateKey %p %s DesiredAccess=%p\n", ObjectAttributes->RootDirectory, s.c_str(), DesiredAccess);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		try {
			uint32_t root_access = 0;
			RegistryKey *root_key = GetRootKey(ObjectAttributes->RootDirectory, &root_access, append_mode_);
			if (root_key) {
				bool is_exists = true;
				RegistryKey *key;
				if (ObjectAttributes->ObjectName->Buffer) {
					UnicodeString str(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t));
					key = ((root_access & KEY_CREATE_SUB_KEY) != 0 && (append_mode_ || !root_key->is_real())) ? root_key->AddKey(str.c_str(), false, &is_exists) : root_key->GetKey(str.c_str());
				} else {
					key = root_key;
				}
				if (key) {
					if (!key->is_real()) {
						HANDLE newHandle = ::CreateEventA(NULL, false, false, NULL);
#ifdef CHECKED
						if(objects_->GetKey(newHandle))
						{
							XTrace(L"NtCreateKey HANDLE REUSE DETECTED\n");
						}
#endif
						VirtualObject *object = objects_->Add(OBJECT_KEY, key, newHandle, DesiredAccess);
						*KeyHandle = object->handle();
						if (Disposition)
							*Disposition = is_exists ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
#ifdef CHECKED
						XTrace(L"NtCreateKey STATUS_SUCCESS, is_exists=%d, h=%p\n", is_exists?1:0, *KeyHandle);
#endif
						return STATUS_SUCCESS;
					}
				} else if (ObjectAttributes->RootDirectory) {
					VirtualObject *object = objects_->GetKey(ObjectAttributes->RootDirectory);
					if (object && object->ref())
					{
						NTSTATUS ret = ((root_access & KEY_CREATE_SUB_KEY) == 0) ? STATUS_ACCESS_DENIED : STATUS_INVALID_PARAMETER;
#ifdef CHECKED
						XTrace(L"NtCreateKey re=%d\n", ret);
#endif
						return ret;
					}
				}
			}
		} catch(...) {
#ifdef CHECKED
			XTrace(L"NtCreateKey STATUS_ACCESS_VIOLATION\n");
#endif
			return STATUS_ACCESS_VIOLATION;
		}
	}

	NTSTATUS ret = TrueNtCreateKey(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
#ifdef CHECKED
	XTrace(L"TrueNtCreateKey %p h=%p\n", ret, *KeyHandle);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
#ifdef CHECKED
	UnicodeString s(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t));
	XTrace(L"->NtOpenKey %p %s DesiredAccess=%p\n", ObjectAttributes->RootDirectory, s.c_str(), DesiredAccess);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		try {
			RegistryKey *root_key = GetRootKey(ObjectAttributes->RootDirectory, NULL, false);
			if (root_key) {
				RegistryKey *key = ObjectAttributes->ObjectName->Buffer ? root_key->GetKey(UnicodeString(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str()) : root_key;
				if (key) {
					 if (!key->is_real()) {
						 HANDLE newHandle = ::CreateEventA(NULL, false, false, NULL);
#ifdef CHECKED
						if(objects_->GetKey(newHandle))
						{
							XTrace(L"NtOpenKey HANDLE REUSE DETECTED\n");
						}
#endif
						VirtualObject *object = objects_->Add(OBJECT_KEY, key, newHandle, DesiredAccess);
						*KeyHandle = object->handle();
#ifdef CHECKED
						XTrace(L"NtOpenKey STATUS_SUCCESS, h=%p\n", *KeyHandle);
#endif
						return STATUS_SUCCESS;
					 }
				} else if (ObjectAttributes->RootDirectory) {
					VirtualObject *object = objects_->GetKey(ObjectAttributes->RootDirectory);
					if (object && object->ref())
					{
#ifdef CHECKED
						XTrace(L"NtOpenKey STATUS_OBJECT_NAME_NOT_FOUND\n");
#endif
						return STATUS_OBJECT_NAME_NOT_FOUND;
					}
				}
			}
		} catch(...) {
#ifdef CHECKED
			XTrace(L"NtOpenKey STATUS_ACCESS_VIOLATION\n");
#endif
			return STATUS_ACCESS_VIOLATION;
		}

		if (append_mode_ && (DesiredAccess & (MAXIMUM_ALLOWED | KEY_SET_VALUE | KEY_CREATE_SUB_KEY))) {
			NTSTATUS status = TrueNtOpenKey(KeyHandle, KEY_READ | (DesiredAccess & KEY_WOW64_RES), ObjectAttributes);
#ifdef CHECKED
			XTrace(L"TrueNtOpenKey for MAXIMUM_ALLOWED %p h=%p\n", status, *KeyHandle);
#endif
			if (NT_SUCCESS(status)) {
				RegistryKey *root_key = GetRootKey(ObjectAttributes->RootDirectory, NULL, true);
				if (root_key) {
					bool is_exists = true;
					RegistryKey *key = ObjectAttributes->ObjectName->Buffer ? root_key->AddKey(UnicodeString(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str(), false, &is_exists) : root_key;
					if (key)
						objects_->Add(OBJECT_KEY, key, *KeyHandle, DesiredAccess);
				}
			}
			return status;
		}
	}

	NTSTATUS ret = TrueNtOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
#ifdef CHECKED
	XTrace(L"TrueNtOpenKey %p h=%p\n", ret, *KeyHandle);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions)
{
#ifdef CHECKED
	UnicodeString s(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t));
	XTrace(L"->NtOpenKeyEx %p %s DesiredAccess=%p\n", ObjectAttributes->RootDirectory, s.c_str(), DesiredAccess);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		try {
			RegistryKey *root_key = GetRootKey(ObjectAttributes->RootDirectory, NULL, false);
			if (root_key) {
				RegistryKey *key = ObjectAttributes->ObjectName->Buffer ? root_key->GetKey(UnicodeString(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str()) : root_key;
				if (key) {
					if (!key->is_real()) {
						VirtualObject *object = objects_->Add(OBJECT_KEY, key, ::CreateEventA(NULL, false, false, NULL), DesiredAccess);
						*KeyHandle = object->handle();
#ifdef CHECKED
						XTrace(L"NtOpenKeyEx STATUS_SUCCESS, h=%p\n", *KeyHandle);
#endif
						return STATUS_SUCCESS;
					}
				} else if (ObjectAttributes->RootDirectory) {
					VirtualObject *object = objects_->GetKey(ObjectAttributes->RootDirectory);
					if (object && object->ref())
					{
#ifdef CHECKED
						XTrace(L"NtOpenKeyEx STATUS_OBJECT_NAME_NOT_FOUND\n");
#endif
						return STATUS_OBJECT_NAME_NOT_FOUND;
					}
				}
			}
		} catch(...) {
#ifdef CHECKED
			XTrace(L"NtOpenKeyEx STATUS_ACCESS_VIOLATION\n");
#endif
			return STATUS_ACCESS_VIOLATION;
		}

		if (append_mode_ && (DesiredAccess & (MAXIMUM_ALLOWED | KEY_SET_VALUE | KEY_CREATE_SUB_KEY))) {
			NTSTATUS status = TrueNtOpenKey(KeyHandle, KEY_READ | (DesiredAccess & KEY_WOW64_RES), ObjectAttributes);
#ifdef CHECKED
			XTrace(L"TrueNtOpenKey for MAXIMUM_ALLOWED %p h=%p\n", status, *KeyHandle);
#endif
			if (NT_SUCCESS(status)) {
				RegistryKey *root_key = GetRootKey(ObjectAttributes->RootDirectory, NULL, true);
				if (root_key) {
					bool is_exists = true;
					RegistryKey *key = ObjectAttributes->ObjectName->Buffer ? root_key->AddKey(UnicodeString(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t)).c_str(), false, &is_exists) : root_key;
					if (key)
						objects_->Add(OBJECT_KEY, key, *KeyHandle, DesiredAccess);
				}
			}
			return status;
		}
	}

	NTSTATUS ret = TrueNtOpenKeyEx(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
#ifdef CHECKED
	XTrace(L"TrueNtOpenKeyEx %p h=%p\n", ret, *KeyHandle);
#endif
	return ret;
}

NTSTATUS RegistryManager::QueryValueKey(RegistryValue *value, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
	if (!value)
	{
#ifdef CHECKED
		XTrace(L"QueryValueKey STATUS_INVALID_PARAMETER\n");
#endif
		return STATUS_INVALID_PARAMETER;
	}

	switch (KeyValueInformationClass) {
	case KeyValueBasicInformation:
		{
			uint32_t name_size = static_cast<ULONG>(wcslen(value->name()) * sizeof(wchar_t));
			uint32_t min_size = offsetof(KEY_VALUE_BASIC_INFORMATION, Name);
			uint32_t result_size = min_size + name_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_VALUE_BASIC_INFORMATION *info = static_cast<KEY_VALUE_BASIC_INFORMATION *>(KeyValueInformation);
			info->TitleIndex = 0;
			info->Type = value->type();
			info->NameLength = name_size;

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;

			memcpy(info->Name, value->name(), name_size);
		}
		return STATUS_SUCCESS;

	case KeyValueFullInformation:
	case KeyValueFullInformationAlign64:
		{
			uint32_t align = 0;
			if (KeyValueInformationClass == KeyValueFullInformationAlign64) {
				align = 8 - ULONG(INT_PTR(KeyValueInformation)) % 8; //-V221
				KeyValueInformation = static_cast<uint8_t *>(KeyValueInformation) + align;
			}
			uint32_t name_size = static_cast<ULONG>(wcslen(value->name()) * sizeof(wchar_t));
			uint32_t data_size = value->size();
			uint32_t min_size = offsetof(KEY_VALUE_FULL_INFORMATION, Name) + align;
			uint32_t result_size = min_size + name_size + data_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_VALUE_FULL_INFORMATION *info = static_cast<KEY_VALUE_FULL_INFORMATION *>(KeyValueInformation);
			info->TitleIndex = 0;
			info->Type = value->type();
			info->NameLength = name_size;
			info->DataLength = data_size;
			info->DataOffset = min_size + name_size;

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;

			memcpy(info->Name, value->name(), name_size);
			memcpy(reinterpret_cast<uint8_t*>(info) + info->DataOffset, value->data(), data_size);
		}
		return STATUS_SUCCESS;

	case KeyValuePartialInformation:
	case KeyValuePartialInformationAlign64:
		{
			uint32_t align = 0;
			if (KeyValueInformationClass == KeyValuePartialInformationAlign64) {
				align = 8 - ULONG(INT_PTR(KeyValueInformation)) % 8; //-V221
				KeyValueInformation = static_cast<uint8_t *>(KeyValueInformation) + align;
			}
			uint32_t data_size = value->size();
			uint32_t min_size = offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data) + align;
			uint32_t result_size = min_size + data_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
			{
#ifdef CHECKED
				XTrace(L"QueryValueKey STATUS_BUFFER_TOO_SMALL\n");
#endif
				return STATUS_BUFFER_TOO_SMALL;
			}

			KEY_VALUE_PARTIAL_INFORMATION *info = static_cast<KEY_VALUE_PARTIAL_INFORMATION *>(KeyValueInformation);
			info->TitleIndex = 0;
			info->Type = value->type();
			info->DataLength = data_size;

			if (Length < result_size)
			{
#ifdef CHECKED
				XTrace(L"QueryValueKey STATUS_BUFFER_OVERFLOW\n");
#endif
				return STATUS_BUFFER_OVERFLOW;
			}

#ifdef CHECKED
			XTrace(L"QueryValueKey %s %d\n", value->data(), data_size);
#endif
			memcpy(info->Data, value->data(), data_size);
		}
		return STATUS_SUCCESS;

	default:
#ifdef CHECKED
		XTrace(L"QueryValueKey STATUS_INVALID_PARAMETER\n");
#endif
		return STATUS_INVALID_PARAMETER;
	}
}

NTSTATUS RegistryManager::NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
#ifdef CHECKED
	UnicodeString s(ValueName->Buffer, ValueName->Length / sizeof(wchar_t));
	XTrace(L"->NtQueryValueKey %p %s KeyValueInformationClass=%d Length=%d\n", KeyHandle, s.c_str(), KeyValueInformationClass, Length);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			if ((object->access() & KEY_QUERY_VALUE) == 0)
			{
#ifdef CHECKED
				XTrace(L"NtQueryValueKey STATUS_ACCESS_DENIED\n");
#endif
				return STATUS_ACCESS_DENIED;
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			RegistryValue *value = key->GetValue(ValueName->Buffer);
			if (value == NULL && append_mode_ == false) {
				// \Registry\Machine\Software\Classes\... to
				UnicodeString hklm = key->full_name();
				if (_wcsnicmp(hklm.c_str(), L"\\Registry\\Machine\\Software\\Classes\\", 35 /* SIZE */) == 0) {
					// \REGISTRY\USER\<SID>_Classes\...
					RegistryKey *ru = cache_.GetKey(L"\\REGISTRY\\USER");
					if (ru != NULL && ru->keys_size() == 1) {
						ru = ru->key(0)->GetKey(hklm.c_str() + 35 /* SIZE */);
					}
					if (ru) {
						value = ru->GetValue(ValueName->Buffer);
#ifdef CHECKED
						XTrace(L"NtQueryValueKey map to hkCU %p\n", value);
#endif
					}
				}
			}
			if (value) {
				try {
					NTSTATUS ret = QueryValueKey(value, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
#ifdef CHECKED
					XTrace(L"NtQueryValueKey ret=%p\n", ret);
#endif
					return ret;
				} catch(...) {
#ifdef CHECKED
					XTrace(L"NtQueryValueKey STATUS_ACCESS_VIOLATION\n");
#endif
					return STATUS_ACCESS_VIOLATION;
				}
			} else if (!key->is_real()) {
#ifdef CHECKED
				XTrace(L"NtQueryValueKey STATUS_OBJECT_NAME_NOT_FOUND\n");
#endif
				return STATUS_OBJECT_NAME_NOT_FOUND;
			}
		}
	}

	NTSTATUS ret = TrueNtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
#ifdef CHECKED
	XTrace(L"TrueNtQueryValueKey %p\n", ret);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtDeleteKey(HANDLE KeyHandle)
{
#ifdef CHECKED
	XTrace(L"->NtDeleteKey %p\n", KeyHandle);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			objects_->DeleteRef(key);
			if (key->owner()->DeleteKey(key))
				return STATUS_SUCCESS;
		}
	}

	if (append_mode_) {
		// FIXME
#ifdef CHECKED
		XTrace(L"TrueNtDeleteKey substed with STATUS_SUCCESS (append mode ON)\n");
#endif
		return STATUS_SUCCESS;
	}

	NTSTATUS ret = TrueNtDeleteKey(KeyHandle);
#ifdef CHECKED
	XTrace(L"TrueNtDeleteKey %p\n", ret);
#endif
	return ret;
}

NTSTATUS RegistryManager::QueryKey(RegistryKey *key, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
	if (!key)
		return STATUS_INVALID_PARAMETER;

	switch (KeyInformationClass) {
	case KeyBasicInformation:
		{
			UnicodeString name = key->full_name();
			uint32_t min_size = offsetof(KEY_BASIC_INFORMATION, Name);
			uint32_t name_size = static_cast<uint32_t>(name.size() * sizeof(wchar_t));
			uint32_t result_size = min_size + name_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_BASIC_INFORMATION *info = static_cast<KEY_BASIC_INFORMATION *>(KeyInformation);
			info->LastWriteTime.QuadPart = key->last_write_time();
			info->TitleIndex = 0;
			info->NameLength = name_size;

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;

			memcpy(info->Name, name.c_str(), name_size);
		}
		return STATUS_SUCCESS;

	case KeyNodeInformation:
		{
			UnicodeString name = key->full_name();
			uint32_t min_size = offsetof(KEY_NODE_INFORMATION, Name);
			uint32_t name_size = static_cast<uint32_t>(name.size() * sizeof(wchar_t));
			uint32_t result_size = min_size + name_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_NODE_INFORMATION *info = static_cast<KEY_NODE_INFORMATION *>(KeyInformation);
			info->LastWriteTime.QuadPart = key->last_write_time();
			info->TitleIndex = 0;
			info->ClassOffset = -1;
			info->ClassLength = 0;
			info->NameLength = name_size;

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;

			memcpy(info->Name, name.c_str(), name_size);
		}
		return STATUS_SUCCESS;

	case KeyFullInformation:
		{
			uint32_t min_size = offsetof(KEY_FULL_INFORMATION, Class);
			uint32_t result_size = min_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_FULL_INFORMATION *info = static_cast<KEY_FULL_INFORMATION *>(KeyInformation);
			info->LastWriteTime.QuadPart = key->last_write_time();
			info->TitleIndex = 0;
			info->ClassOffset = -1;
			info->ClassLength = 0;
			info->SubKeys = static_cast<uint32_t>(key->keys_size());
			info->MaxNameLen = 0;
			for (size_t i = 0; i < key->keys_size(); i++) {
				uint32_t name_size = static_cast<uint32_t>(wcslen(key->key(i)->name()) * sizeof(wchar_t));
				if (info->MaxNameLen < name_size)
					info->MaxNameLen = name_size;
			}
			info->MaxClassLen = 0;
			info->Values = static_cast<uint32_t>(key->values_size());
			info->MaxValueNameLen = 0;
			info->MaxValueDataLen = 0;
			for (size_t i = 0; i < key->values_size(); i++) {
				uint32_t name_size = static_cast<uint32_t>(wcslen(key->value(i)->name()) * sizeof(wchar_t));
				uint32_t data_size = key->value(i)->size();
				if (info->MaxValueNameLen < name_size)
					info->MaxValueNameLen = name_size;
				if (info->MaxValueDataLen < data_size)
					info->MaxValueDataLen = data_size;
			}

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;
		}
		return STATUS_SUCCESS;

	case KeyNameInformation:
		{
			UnicodeString name = key->full_name();
			uint32_t min_size = offsetof(KEY_NAME_INFORMATION, Name);
			uint32_t name_size = static_cast<uint32_t>(name.size() * sizeof(wchar_t));
			uint32_t result_size = min_size + name_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_NAME_INFORMATION *info = static_cast<KEY_NAME_INFORMATION *>(KeyInformation);
			info->NameLength = name_size;

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;

			memcpy(info->Name, name.c_str(), name_size);
		}
		return STATUS_SUCCESS;

	case KeyCachedInformation:
		{
			UnicodeString name = key->full_name();
			uint32_t min_size = sizeof(KEY_CACHED_INFORMATION);
			uint32_t result_size = min_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_CACHED_INFORMATION *info = static_cast<KEY_CACHED_INFORMATION *>(KeyInformation);
			info->LastWriteTime.QuadPart = key->last_write_time();
			info->TitleIndex = 0;
			info->SubKeys = static_cast<uint32_t>(key->keys_size());
			info->MaxNameLen = 0;
			for (size_t i = 0; i < key->keys_size(); i++) {
				ULONG name_size = static_cast<ULONG>(wcslen(key->key(i)->name()) * sizeof(wchar_t));
				if (info->MaxNameLen < name_size)
					info->MaxNameLen = name_size;
			}
			info->Values = static_cast<ULONG>(key->values_size());
			info->MaxValueNameLen = 0;
			info->MaxValueDataLen = 0;
			for (size_t i = 0; i < key->values_size(); i++) {
				uint32_t name_size = static_cast<uint32_t>(wcslen(key->value(i)->name()) * sizeof(wchar_t));
				uint32_t data_size = static_cast<uint32_t>(key->value(i)->size());
				if (info->MaxValueNameLen < name_size)
					info->MaxValueNameLen = name_size;
				if (info->MaxValueDataLen < data_size)
					info->MaxValueDataLen = data_size;
			}
			info->NameLength = static_cast<uint32_t>(name.size() * sizeof(wchar_t));

			if (Length < result_size)
				return STATUS_BUFFER_OVERFLOW;
		}
		return STATUS_SUCCESS;

	case KeyHandleTagsInformation:
		{
			uint32_t min_size = sizeof(KEY_HANDLE_TAGS_INFORMATION);
			uint32_t result_size = min_size;
			if (ResultLength)
				*ResultLength = result_size;
			if (Length < min_size)
				return STATUS_BUFFER_TOO_SMALL;

			KEY_HANDLE_TAGS_INFORMATION *info = static_cast<KEY_HANDLE_TAGS_INFORMATION *>(KeyInformation);
			info->HandleTags = 0;
		}
		return STATUS_SUCCESS;

	case KeyFlagsInformation:
	case KeyVirtualizationInformation:
		return STATUS_NOT_IMPLEMENTED;

	default:
		return STATUS_INVALID_PARAMETER;
	}
}

static void ApplyDirtyHack(PVOID KeyInformation)
{
	KEY_HANDLE_TAGS_INFORMATION *info = static_cast<KEY_HANDLE_TAGS_INFORMATION *>(KeyInformation);
	info->HandleTags = 0x601;
#ifdef CHECKED
	XTrace(L"ApplyDirtyHack info->HandleTags=%d\n", info->HandleTags);
#endif
}

NTSTATUS RegistryManager::NtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
#ifdef CHECKED
	XTrace(L"->NtQueryKey %p KeyInformationClass=%d\n", KeyHandle, KeyInformationClass);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			if ((object->access() & KEY_QUERY_VALUE) == 0)
			{
				 // Special case (взят из win2k/private/ntos/config/ntapi.c:NtQueryKey)
				if(KeyInformationClass != KeyNameInformation || object->access() == 0) {
#ifdef CHECKED
					XTrace(L"NtQueryKey STATUS_ACCESS_DENIED %p\n", object->access());
#endif
					return STATUS_ACCESS_DENIED;
				}
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			try {
				NTSTATUS st = QueryKey(key, KeyInformationClass, KeyInformation, Length, ResultLength);
#ifdef CHECKED
				XTrace(L"NtQueryKey st=%p, %d=%d\n", st, Length, ResultLength ? *ResultLength : 0);
#endif
				if (KeyInformationClass == KeyHandleTagsInformation && st == S_OK && (object->access() & KEY_WOW64_32KEY) && append_mode_ && KeyInformation)
					ApplyDirtyHack(KeyInformation);
				return st;
			} catch(...) {
#ifdef CHECKED
				XTrace(L"NtQueryKey STATUS_ACCESS_VIOLATION\n");
#endif
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	NTSTATUS ret = TrueNtQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
	if (KeyInformationClass == KeyHandleTagsInformation && ret == S_OK && append_mode_ && KeyInformation)
		ApplyDirtyHack(KeyInformation);
#ifdef CHECKED
	if(KeyInformationClass == 7)
	{
		KEY_HANDLE_TAGS_INFORMATION *info = static_cast<KEY_HANDLE_TAGS_INFORMATION *>(KeyInformation);
		if (info) 
		XTrace(L"TrueNtQueryKey %p HandleTags=%p\n", ret, info->HandleTags);
	} else if(KeyInformationClass == 3)
	{
		KEY_NAME_INFORMATION *info = static_cast<KEY_NAME_INFORMATION *>(KeyInformation);
		if (info) 
			XTrace(L"TrueNtQueryKey %p info->Name=%s\n", ret, (UnicodeString(info->Name, info->NameLength)).c_str());
	} else 
		XTrace(L"TrueNtQueryKey %p\n", ret);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
#ifdef CHECKED
	XTrace(L"->NtEnumerateValueKey %p\n", KeyHandle);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			if ((object->access() & KEY_QUERY_VALUE) == 0)
			{
#ifdef CHECKED
				XTrace(L"NtEnumerateValueKey STATUS_ACCESS_DENIED\n");
#endif
				return STATUS_ACCESS_DENIED;
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			if (Index >= key->values_size())
#ifdef CHECKED
				XTrace(L"NtEnumerateValueKey STATUS_NO_MORE_ENTRIES\n");
#endif
				return STATUS_NO_MORE_ENTRIES;
			try {
				NTSTATUS vret = QueryValueKey(key->value(Index), KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
#ifdef CHECKED
				XTrace(L"QueryValueKey %p\n", vret);
#endif
				return vret;
			} catch(...) {
#ifdef CHECKED
				XTrace(L"NtEnumerateValueKey STATUS_ACCESS_VIOLATION\n");
#endif
				return STATUS_ACCESS_VIOLATION;
			}
		}
	}

	NTSTATUS ret = TrueNtEnumerateValueKey(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
#ifdef CHECKED
	XTrace(L"TrueNtEnumerateValueKey %p\n", ret);
#endif
	return ret;
}

NTSTATUS RegistryManager::NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength)
{
#ifdef CHECKED
	XTrace(L"->NtEnumerateKey %p\n", KeyHandle);
#endif
	{
		CriticalSection	cs(objects_->critical_section());

		VirtualObject *object = objects_->GetKey(KeyHandle);
		if (object && object->ref()) {
			// a virtual key
			if ((object->access() & KEY_ENUMERATE_SUB_KEYS) == 0)
			{
#ifdef CHECKED
				XTrace(L"NtEnumerateKey STATUS_ACCESS_DENIED\n");
#endif
				return STATUS_ACCESS_DENIED;
			}

			RegistryKey *key = static_cast<RegistryKey *>(object->ref());
			if (Index >= key->keys_size())
#ifdef CHECKED
				XTrace(L"NtEnumerateKey STATUS_NO_MORE_ENTRIES\n");
#endif
				return STATUS_NO_MORE_ENTRIES;
			try {
				NTSTATUS vret = QueryKey(key->key(Index), KeyInformationClass, KeyInformation, Length, ResultLength);
#ifdef CHECKED
				XTrace(L"QueryKey %p\n", vret);
#endif
				return vret;
			} catch(...) {
#ifdef CHECKED
				XTrace(L"NtEnumerateKey STATUS_ACCESS_VIOLATION\n");
#endif
				return STATUS_ACCESS_VIOLATION;
			}
		} else {
			// a real key
#ifdef CHECKED
			XTrace(L"NtEnumerateKey real\n");
#endif
			uint32_t access;
			if (object)
				access = object->access();
			else {
				PUBLIC_OBJECT_BASIC_INFORMATION info;
				access = (NT_SUCCESS(TrueNtQueryObject(KeyHandle, ObjectBasicInformation, &info, sizeof(info), NULL))) ? info.GrantedAccess : 0;
#ifdef CHECKED
				XTrace(L"TrueNtQueryObject.Access = %p\n", access);
#endif
			}
			if (access & KEY_ENUMERATE_SUB_KEYS) {
				RegistryKey *key = NULL;
				{
					KEY_NAME_INFORMATION info;
					DWORD size;
					if (TrueNtQueryKey(KeyHandle, KeyNameInformation, &info, sizeof(info), &size) == STATUS_BUFFER_OVERFLOW) {
						KEY_NAME_INFORMATION *data = reinterpret_cast<KEY_NAME_INFORMATION *>(new uint8_t[size]);
						if (NT_SUCCESS(TrueNtQueryKey(KeyHandle, KeyNameInformation, data, size, &size)))
						{
							key = cache_.GetKey(UnicodeString(data->Name, data->NameLength / sizeof(wchar_t)).c_str());
#ifdef CHECKED
							XTrace(L"Cache used\n");
#endif
						}
#ifdef CHECKED
						else
						{
							XTrace(L"Cache NOT used\n");
						}
#endif
						delete [] data;
					}
				}
				if (key) {
					vector<RegistryKey *> children;
					for (size_t i = 0; i < key->keys_size(); i++) {
						RegistryKey *child = key->key(i);
						if (!child->is_real())
							children.push_back(child);
					}
					if (Index < children.size()) {
						try {
							NTSTATUS vret = QueryKey(children[Index], KeyInformationClass, KeyInformation, Length, ResultLength);
#ifdef CHECKED
							XTrace(L"QueryKey %p\n", vret);
#endif
							return vret;
						} catch(...) {
#ifdef CHECKED
							XTrace(L"NtEnumerateKey STATUS_ACCESS_VIOLATION\n");
#endif
							return STATUS_ACCESS_VIOLATION;
						}
					} else {
						Index -= static_cast<uint32_t>(children.size());
					}
				}
			}
		}
	}

	NTSTATUS ret = TrueNtEnumerateKey(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
#ifdef CHECKED
	XTrace(L"TrueNtEnumerateKey %p\n", ret);
#endif
	return ret;
}

#endif // WIN_DRIVER