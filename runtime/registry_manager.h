#ifndef REGISTRY_MANAGER_H
#define REGISTRY_MANAGER_H

struct REGISTRY_DIRECTORY {
	uint32_t Reserved1;
	uint32_t Reserved2;
};

typedef struct _KEY_VALUE_BASIC_INFORMATION {
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   NameLength;
	WCHAR   Name[1];            // Variable size
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   DataOffset;
	ULONG   DataLength;
	ULONG   NameLength;
	WCHAR   Name[1];            // Variable size
//          Data[1];            // Variable size data not declared
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   DataLength;
	UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
	ULONG   Type;
	ULONG   DataLength;
	UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64,
	MaxKeyValueInfoClass  // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG   TitleIndex;
	ULONG   NameLength;
	WCHAR   Name[1];            // Variable length string
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG   TitleIndex;
	ULONG   ClassOffset;
	ULONG   ClassLength;
	ULONG   NameLength;
	WCHAR   Name[1];            // Variable length string
//          Class[1];           // Variable length string not declared
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG   TitleIndex;
	ULONG   ClassOffset;
	ULONG   ClassLength;
	ULONG   SubKeys;
	ULONG   MaxNameLen;
	ULONG   MaxClassLen;
	ULONG   Values;
	ULONG   MaxValueNameLen;
	ULONG   MaxValueDataLen;
	WCHAR   Class[1];           // Variable length
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
	ULONG NameLength;
	WCHAR Name[1];             // Variable length
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG         TitleIndex;
	ULONG         SubKeys;
	ULONG         MaxNameLen;
	ULONG         Values;
	ULONG         MaxValueNameLen;
	ULONG         MaxValueDataLen;
	ULONG         NameLength;
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_HANDLE_TAGS_INFORMATION {
	ULONG HandleTags;
} KEY_HANDLE_TAGS_INFORMATION, *PKEY_HANDLE_TAGS_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyNameInformation,
	KeyCachedInformation,
	KeyFlagsInformation,
	KeyVirtualizationInformation,
	KeyHandleTagsInformation,
	MaxKeyInfoClass  // MaxKeyInfoClass should always be the last enum
} KEY_INFORMATION_CLASS;

class RegistryValue
{
public:
	RegistryValue(const wchar_t *name);
	~RegistryValue();
	const wchar_t *name() const { return name_; }
	uint32_t type() const { return type_; }
	uint32_t size() const { return size_; }
	void *data() const { return data_; }
	void SetValue(uint32_t type, void *data, uint32_t size);
private:
	wchar_t *name_;
	uint32_t type_;
	uint8_t *data_;
	uint32_t size_;
	
	// no copy ctr or assignment op
	RegistryValue(const RegistryValue &);
	RegistryValue &operator =(const RegistryValue &);
};

class UnicodeString
{
public:
	UnicodeString()
		: data_(NULL), size_(0)
	{
		data_ = new wchar_t[size_ + 1];
		data_[size_] = 0;
	}
	UnicodeString(const wchar_t *str)
		: data_(NULL), size_(wcslen(str))
	{
		data_ = new wchar_t[size_ + 1];
		memcpy(data_, str, size_ * sizeof(wchar_t));
		data_[size_] = 0;
	}
	UnicodeString(const wchar_t *str, size_t size)
		: data_(NULL), size_(size)
	{
		data_ = new wchar_t[size_ + 1];
		memcpy(data_, str, size_ * sizeof(wchar_t));
		data_[size_] = 0;
	}
	UnicodeString(const UnicodeString &str)
		: data_(NULL), size_(str.size_)
	{
		data_ = new wchar_t[size_ + 1];
		memcpy(data_, str.data_, size_ * sizeof(wchar_t));
		data_[size_] = 0;
	}
	UnicodeString &operator=(const UnicodeString &src)
	{
		if (&src != this)
		{
			delete [] data_;
			size_ = src.size();
			data_ = new wchar_t[size_ + 1];
			memcpy(data_, src.c_str(), size_ * sizeof(wchar_t));
			data_[size_] = 0;
		}
		return *this;
	}
	~UnicodeString()
	{
		delete [] data_;
	}
	size_t size() const { return size_; }
	const wchar_t *c_str() const { return data_; }
	UnicodeString &append(const wchar_t *str) { return append (str, wcslen(str)); }
	UnicodeString &append(const wchar_t *str, size_t str_size)
	{
		size_t new_size = size_ + str_size;
		wchar_t *new_data = new wchar_t[new_size + 1];
		memcpy(new_data, data_, size_ * sizeof(wchar_t));
		memcpy(new_data + size_, str, str_size * sizeof(wchar_t));
		new_data[new_size] = 0;
		delete [] data_;
		data_ = new_data;
		size_ = new_size;
		return *this;
	}
	UnicodeString &operator + (wchar_t c) { return append(&c, 1); }
	UnicodeString &operator + (const wchar_t *str) { return append(str, wcslen(str)); }
	bool operator == (const wchar_t *str) const { return wcscmp(c_str(), str) == 0; }
private:
	wchar_t *data_;
	size_t size_;
};

class RegistryKey
{
public:
	RegistryKey();
	RegistryKey(RegistryKey *owner, const wchar_t *name, bool is_real);
	~RegistryKey();
	RegistryKey *GetKey(const wchar_t *name) const;
	RegistryKey *AddKey(const wchar_t *name, bool is_real, bool *is_exists);
	bool DeleteKey(RegistryKey *key);
	const wchar_t *name() const { return name_; }
	UnicodeString full_name() const;
	void SetValue(wchar_t *name, uint32_t type, void *data, uint32_t size);
	bool DeleteValue(wchar_t *name);
	RegistryValue *GetValue(const wchar_t *name) const;
	bool is_real() const { return is_real_; }
	void set_is_wow(bool value) { is_wow_ = value; }
	RegistryKey *owner() const { return owner_; }
	RegistryKey *key(size_t index) const { return keys_[index]; }
	size_t keys_size() const { return keys_.size(); }
	RegistryValue *value(size_t index) const { return values_[index]; }
	size_t values_size() const { return values_.size(); }
	uint64_t last_write_time() const { return last_write_time_; }
private:
	RegistryKey *GetChild(const wchar_t *name) const;
	RegistryKey *AddChild(const wchar_t *name, bool is_real, bool *is_exists);
	RegistryValue *AddValue(wchar_t *value_name);
	bool is_wow_node(const wchar_t *name) const;
	wchar_t *name_;
	RegistryKey *owner_;
	bool is_real_;
	bool is_wow_;
	vector<RegistryKey *> keys_;
	vector<RegistryValue *> values_;
	uint64_t last_write_time_;
	
	// no copy ctr or assignment op
	RegistryKey(const RegistryKey &);
	RegistryKey &operator =(const RegistryKey &);
};

class VirtualObjectList;
class RegistryManager
{
public:
	RegistryManager(const uint8_t *data, HMODULE instance, const uint8_t *key, VirtualObjectList *objects);
	void HookAPIs(HookManager &hook_manager);
	void UnhookAPIs(HookManager &hook_manager);
	void BeginRegisterServer();
	void EndRegisterServer();
	NTSTATUS NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
	NTSTATUS NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName);
	NTSTATUS NtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition);
	NTSTATUS NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
	NTSTATUS NtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions);
	NTSTATUS NtDeleteKey(HANDLE KeyHandle);
	NTSTATUS NtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
private:
	NTSTATUS TrueNtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
	NTSTATUS TrueNtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName);
	NTSTATUS TrueNtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition);
	NTSTATUS TrueNtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS TrueNtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
	NTSTATUS TrueNtOpenKeyEx(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG OpenOptions);
	NTSTATUS TrueNtDeleteKey(HANDLE KeyHandle);
	NTSTATUS TrueNtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS TrueNtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS TrueNtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS TrueNtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
	REGISTRY_DIRECTORY DecryptDirectory(const REGISTRY_DIRECTORY *directory_enc) const;
	RegistryKey *GetRootKey(HANDLE root, uint32_t *access, bool can_create);
	NTSTATUS QueryValueKey(RegistryValue *value, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	NTSTATUS QueryKey(RegistryKey *key, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);

	HMODULE instance_;
	const uint8_t *data_;
	uint32_t key_;
	void *nt_set_value_key_;
	void *nt_delete_value_key_;
	void *nt_create_key_;
	void *nt_open_key_;
	void *nt_open_key_ex_;
	void *nt_query_value_key_;
	void *nt_delete_key_;
	void *nt_query_key_;
	void *nt_enumerate_value_key_;
	void *nt_enumerate_key_;
	RegistryKey cache_;
	VirtualObjectList *objects_;
	bool append_mode_;

	// no copy ctr or assignment op
	RegistryManager(const RegistryManager &);
	RegistryManager &operator =(const RegistryManager &);
};

#endif