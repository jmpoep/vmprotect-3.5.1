#include "common.h"
#include "objects.h"

#ifdef WIN_DRIVER
typedef BOOL (WINAPI *ENUMRESNAMEPROCA)(HMODULE hModule, LPCSTR lpType, LPSTR lpName, LONG_PTR lParam);
typedef BOOL (WINAPI *ENUMRESNAMEPROCW)(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
typedef BOOL (WINAPI *ENUMRESLANGPROCA)(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD  wLanguage, LONG_PTR lParam);
typedef BOOL (WINAPI *ENUMRESLANGPROCW)(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD  wLanguage, LONG_PTR lParam);
typedef BOOL (WINAPI *ENUMRESTYPEPROCA)(HMODULE hModule, LPSTR lpType, LONG_PTR lParam);
typedef BOOL (WINAPI *ENUMRESTYPEPROCW)(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam);
#else
#include "core.h"
#include "crypto.h"
#include "resource_manager.h"
#include "hook_manager.h"
#include "utils.h"
#endif

/**
 * exported functions
 */

HGLOBAL WINAPI ExportedLoadResource(HMODULE module, HRSRC res_info)
{
#ifdef WIN_DRIVER
	return NULL;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->LoadResource(module, res_info) : NULL;
#endif
}

HRSRC WINAPI ExportedFindResourceA(HMODULE module, LPCSTR name, LPCSTR type)
{
#ifdef WIN_DRIVER
	return NULL;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->FindResourceA(module, name, type) : NULL;
#endif
}

HRSRC WINAPI ExportedFindResourceExA(HMODULE module, LPCSTR type, LPCSTR name, WORD language)
{
#ifdef WIN_DRIVER
	return NULL;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->FindResourceExA(module, type, name, language) : NULL;
#endif
}

HRSRC WINAPI ExportedFindResourceW(HMODULE module, LPCWSTR name, LPCWSTR type)
{
#ifdef WIN_DRIVER
	return NULL;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->FindResourceExW(module, type, name, 0) : NULL;
#endif
}

HRSRC WINAPI ExportedFindResourceExW(HMODULE module, LPCWSTR type, LPCWSTR name, WORD language)
{
#ifdef WIN_DRIVER
	return NULL;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->FindResourceExW(module, type, name, language) : NULL;
#endif
}

int WINAPI ExportedLoadStringA(HMODULE module, UINT id, LPSTR buffer, int buffer_max)
{
#ifdef WIN_DRIVER
	return 0;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->LoadStringA(module, id, buffer, buffer_max) : 0;
#endif
}

int WINAPI ExportedLoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max)
{
#ifdef WIN_DRIVER
	return 0;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->LoadStringW(module, id, buffer, buffer_max) : 0;
#endif
}

BOOL WINAPI ExportedEnumResourceNamesA(HMODULE module, LPCSTR type, ENUMRESNAMEPROCA enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceNamesA(module, type, enum_func, param) : FALSE;
#endif
}

BOOL WINAPI ExportedEnumResourceNamesW(HMODULE module, LPCWSTR type, ENUMRESNAMEPROCW enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceNamesW(module, type, enum_func, param) : FALSE;
#endif
}

BOOL WINAPI ExportedEnumResourceLanguagesA(HMODULE module, LPCSTR type, LPCSTR name, ENUMRESLANGPROCA enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceLanguagesA(module, type, name, enum_func, param) : FALSE;
#endif
}

BOOL WINAPI ExportedEnumResourceLanguagesW(HMODULE module, LPCWSTR type, LPCWSTR name, ENUMRESLANGPROCW enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceLanguagesW(module, type, name, enum_func, param) : FALSE;
#endif
}

BOOL WINAPI ExportedEnumResourceTypesA(HMODULE module, ENUMRESTYPEPROCA enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceTypesA(module, enum_func, param) : FALSE;
#endif
}

BOOL WINAPI ExportedEnumResourceTypesW(HMODULE module, ENUMRESTYPEPROCW enum_func, LONG_PTR param)
{
#ifdef WIN_DRIVER
	return FALSE;
#else
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager ? resource_manager->EnumResourceTypesW(module, enum_func, param) : FALSE;
#endif
}

#ifdef WIN_DRIVER
#else
/**
 * hooked functions
 */

int WINAPI HookedLoadStringA(HMODULE module, UINT id, LPSTR buffer, int buffer_max)
{
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager->LoadStringA(module, id, buffer, buffer_max);
}

int WINAPI HookedLoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max)
{
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager->LoadStringW(module, id, buffer, buffer_max);
}

NTSTATUS WINAPI HookedLdrFindResource_U(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry)
{
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager->LdrFindResource_U(module, res_info, level, entry);
}

NTSTATUS WINAPI HookedLdrAccessResource(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *entry, void **res, ULONG *size)
{
	ResourceManager *resource_manager = Core::Instance()->resource_manager();
	return resource_manager->LdrAccessResource(module, entry, res, size);
}

/**
 * ResourceManager
 */

ResourceManager::ResourceManager(const uint8_t *data, HMODULE instance, const uint8_t *key)
	: instance_(instance)
	, data_(data)
	, load_resource_(NULL)
	, load_string_a_(NULL)
	, load_string_w_(NULL)
	, load_string_a_kernel_(NULL)
	, load_string_w_kernel_(NULL)
	, ldr_find_resource_u_(NULL)
	, ldr_access_resource_(NULL)
	, get_thread_ui_language_(NULL)
{
	CriticalSection::Init(critical_section_);
	key_ = *(reinterpret_cast<const uint32_t *>(key));
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("kernel32.dll"));
	if (dll)
		get_thread_ui_language_ = InternalGetProcAddress(dll, VMProtectDecryptStringA("GetThreadUILanguage"));
}

ResourceManager::~ResourceManager()
{
	CriticalSection::Free(critical_section_);
}

void ResourceManager::HookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	HMODULE dll = GetModuleHandleA(VMProtectDecryptStringA("ntdll.dll"));
	ldr_find_resource_u_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LdrFindResource_U"), &HookedLdrFindResource_U);
	ldr_access_resource_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LdrAccessResource"), &HookedLdrAccessResource);
	dll = GetModuleHandleA(VMProtectDecryptStringA("user32.dll"));
	load_string_a_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LoadStringA"), &HookedLoadStringA);
	load_string_w_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LoadStringW"), &HookedLoadStringW);
	dll = GetModuleHandleA(VMProtectDecryptStringA("kernelbase.dll"));
	if (dll) {
		load_string_a_kernel_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LoadStringA"), &HookedLoadStringA);
		load_string_w_kernel_ = hook_manager.HookAPI(dll, VMProtectDecryptStringA("LoadStringW"), &HookedLoadStringW);
	}
	hook_manager.End();
}

void ResourceManager::UnhookAPIs(HookManager &hook_manager)
{
	hook_manager.Begin();
	hook_manager.UnhookAPI(ldr_find_resource_u_);
	hook_manager.UnhookAPI(ldr_access_resource_);
	hook_manager.UnhookAPI(load_string_a_);
	hook_manager.UnhookAPI(load_string_w_);
	if (load_string_a_kernel_) {
		hook_manager.UnhookAPI(load_string_a_kernel_);
		hook_manager.UnhookAPI(load_string_w_kernel_);
	}
	hook_manager.End();
}

int __forceinline ResourceManager::TrueLoadStringA(HMODULE module, UINT id, LPSTR buffer, int buffer_max)
{
	typedef int (WINAPI tLoadStringA)(HMODULE module, UINT id, LPSTR buffer, int buffer_max);
	return reinterpret_cast<tLoadStringA *>(load_string_a_)(module, id, buffer, buffer_max);
}

int __forceinline ResourceManager::TrueLoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max)
{
	typedef int (WINAPI tLoadStringW)(HMODULE module, UINT id, LPWSTR buffer, int buffer_max);
	return reinterpret_cast<tLoadStringW *>(load_string_w_)(module, id, buffer, buffer_max);
}

NTSTATUS __forceinline ResourceManager::TrueLdrFindResource_U(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry)
{
	typedef int (WINAPI tLdrFindResource_U)(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry);
	return reinterpret_cast<tLdrFindResource_U *>(ldr_find_resource_u_)(module, res_info, level, entry);
}

NTSTATUS __forceinline ResourceManager::TrueLdrAccessResource(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *entry, void **res, ULONG *size)
{
	typedef int (WINAPI tLdrAccessResource)(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *pEntry, void **pRes, ULONG *pSize);
	return reinterpret_cast<tLdrAccessResource *>(ldr_access_resource_)(module, entry, res, size);
}

HGLOBAL ResourceManager::InternalLoadResource(HMODULE module, HRSRC res_info)
{
	if (!module)
		module = GetModuleHandle(NULL);

	if (module == instance_) { 
		CriticalSection	cs(critical_section_);

		size_t i = resources_.IndexByHandle(res_info);
		if (i != -1)
			return resources_[i]->Decrypt(instance_, key_);
	}
	return NULL;
}

HGLOBAL ResourceManager::LoadResource(HMODULE module, HRSRC res_info)
{
	HGLOBAL res = InternalLoadResource(module, res_info);
	if (res)
		return res;
	return ::LoadResource(module, res_info);
}

HRSRC ResourceManager::InternalFindResourceExW(HMODULE module, LPCWSTR type, LPCWSTR name, WORD language)
{
	if (!module)
		module = GetModuleHandle(NULL);

	if (module == instance_) { 
		const RESOURCE_DATA_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DATA_ENTRY *>(FindResourceEntry(type, name, language));
		if (entry_enc) {
			CriticalSection cs(critical_section_);

			size_t i = resources_.IndexByEntry(entry_enc);
			if (i != -1)
				return resources_[i]->handle();

			return resources_.Add(entry_enc, key_)->handle();
		}
	}
	return NULL;
}

BOOL CreateResourceName(LPCWSTR src, LPWSTR &dst)
{
	if (IS_INTRESOURCE(src)) {
		dst = MAKEINTRESOURCEW(LOWORD(src));
		return TRUE;
	}

	if (src[0] == L'#') {
		ULONG value = wcstoul(src + 1, NULL, 10);
		if (HIWORD(value))
			return FALSE;
		dst = MAKEINTRESOURCEW(value);
		return TRUE;
	}

	size_t len = wcslen(src);
	dst = new wchar_t[len + 1];
	wcscpy_s(dst, len + 1, src);
	CharUpperBuffW(dst, static_cast<DWORD>(len));
	return TRUE;
}

BOOL CreateResourceName(LPCSTR src, LPWSTR &dst)
{
	if (IS_INTRESOURCE(src)) {
		dst = MAKEINTRESOURCEW(LOWORD(src));
		return TRUE;
	}

	if (src[0] == '#') {
		ULONG value = strtoul(src + 1, NULL, 10);
		if (HIWORD(value))
			return FALSE;
		dst = MAKEINTRESOURCEW(value);
		return TRUE;
	}

    int size = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if (size == 0) 
		return FALSE;
    dst = new wchar_t[size];
    int size2 = MultiByteToWideChar(CP_ACP, 0, src, -1, dst, size);
	CharUpperBuffW(dst, static_cast<DWORD>(size2));
    return TRUE;
}

void FreeResourceName(LPCWSTR src)
{
	if (!IS_INTRESOURCE(src))
		delete [] src;
}

HRSRC ResourceManager::FindResourceExW(HMODULE module, LPCWSTR type, LPCWSTR name, WORD language)
{
	if (!module)
		module = GetModuleHandle(NULL);

	if (module == instance_)	{ 
		LPWSTR new_type = NULL;
		LPWSTR new_name = NULL;
		HRSRC res;

		if (CreateResourceName(type, new_type) && CreateResourceName(name, new_name)) {
			res = InternalFindResourceExW(module, new_type, new_name, language);
		} else {
			res = NULL;
		}

		FreeResourceName(new_type);
		FreeResourceName(new_name);

		if (res)
			return res;
	}
	return ::FindResourceExW(module, type, name, language);
}

HRSRC ResourceManager::FindResourceW(HMODULE module, LPCWSTR name, LPCWSTR type)
{
	return FindResourceExW(module, type, name, 0);
}

HRSRC ResourceManager::FindResourceExA(HMODULE module, LPCSTR type, LPCSTR name, WORD language)
{
	if (!module)
		module = GetModuleHandle(NULL);

	if (module == instance_)	{ 
		LPWSTR new_type = NULL;
		LPWSTR new_name = NULL;
		HRSRC res;

		if (CreateResourceName(type, new_type) && CreateResourceName(name, new_name)) {
			res = FindResourceExW(module, new_type, new_name, language);
		} else {
			res = NULL;
		}

		FreeResourceName(new_name);
		FreeResourceName(new_type);

		if (res)
			return res;
	}
	return ::FindResourceExA(module, type, name, language);
}

HRSRC ResourceManager::FindResourceA(HMODULE module, LPCSTR name, LPCSTR type)
{
	return FindResourceExA(module, type, name, 0);
}

RESOURCE_DIRECTORY ResourceManager::DecryptDirectory(const RESOURCE_DIRECTORY *directory_enc) const
{
	RESOURCE_DIRECTORY res;
	res.NumberOfNamedEntries = directory_enc->NumberOfNamedEntries ^ key_;
	res.NumberOfIdEntries = directory_enc->NumberOfIdEntries ^ key_;
	return res;
}

RESOURCE_DIRECTORY_ENTRY ResourceManager::DecryptDirectoryEntry(const RESOURCE_DIRECTORY_ENTRY *entry_enc) const
{
	RESOURCE_DIRECTORY_ENTRY res;
	res.Name = entry_enc->Name ^ key_;
	res.OffsetToData = entry_enc->OffsetToData ^ key_;
	return res;
}

const RESOURCE_DIRECTORY *ResourceManager::FindEntryById(const RESOURCE_DIRECTORY *directory_enc, WORD id, DWORD dir_type)
{
	const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);

	RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
	int min = directory.NumberOfNamedEntries;
	int max = min + directory.NumberOfIdEntries - 1;

	while (min <= max) {
		int i = (min + max) / 2;
		RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
		if (entry.Id == id) {
			if (entry.DataIsDirectory == dir_type)
				return reinterpret_cast<const RESOURCE_DIRECTORY *>(data_ + entry.OffsetToDirectory);
			break;
		}
		if (entry.Id > id) {
			max = i - 1;
		} else {
			min = i + 1;
		}
	}
	return NULL;
}

int ResourceManager::CompareStringEnc(LPCWSTR name, LPCWSTR str_enc) const
{
	for (size_t i = 0; ;i++) {
		wchar_t c = static_cast<wchar_t>(str_enc[i] ^ (_rotl32(key_, static_cast<int>(i)) + i));
		int res = name[i] - c;
		if (res)
			return (res < 0) ? -1 : 1;
		if (!name[i])
			break;
	}
	return 0;
}

LPWSTR ResourceManager::DecryptStringW(LPCWSTR str_enc) const
{
	size_t len = 0;
	while (true) {
		wchar_t c = static_cast<wchar_t>(str_enc[len] ^ (_rotl32(key_, static_cast<int>(len)) + len));
		len++;
		if (!c)
			break;
	}

	LPWSTR res = new wchar_t[len];
	for (size_t i = 0; i < len; i++) {
		res[i] = static_cast<wchar_t>(str_enc[i] ^ (_rotl32(key_, static_cast<int>(i)) + i));
	}
	return res;
}

LPSTR ResourceManager::DecryptStringA(LPCWSTR str_enc) const
{
	LPWSTR str = DecryptStringW(str_enc);
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	LPSTR res = new char[len];
	WideCharToMultiByte(CP_ACP, 0, str, -1, res, len, NULL, NULL);
	delete [] str;
	return res;
}

const RESOURCE_DIRECTORY *ResourceManager::FindEntryByName(const RESOURCE_DIRECTORY *directory_enc, LPCWSTR name, DWORD dir_type)
{
	const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
	RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);

	int min = 0;
	int max = directory.NumberOfNamedEntries - 1;

	while (min <= max) {
		int i = (min + max) / 2;
		RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
		int res = CompareStringEnc(name, reinterpret_cast<LPCWSTR>(data_ + entry.NameOffset));
		if (!res) {
			if (entry.DataIsDirectory == dir_type)
				return reinterpret_cast<const RESOURCE_DIRECTORY *>(data_ + entry.OffsetToDirectory);
			break;
		}
		if (res < 0) {
			max = i - 1;
		} else {
			min = i + 1;
		}
	}
	return NULL;
}

const RESOURCE_DIRECTORY *ResourceManager::FindEntry(const RESOURCE_DIRECTORY *directory_enc, LPCWSTR name, DWORD dir_type)
{
	if (IS_INTRESOURCE(name))
		return FindEntryById(directory_enc, LOWORD(name), dir_type);
	return FindEntryByName(directory_enc, name, dir_type);
}

const RESOURCE_DIRECTORY *ResourceManager::FindFirstEntry(const RESOURCE_DIRECTORY *directory_enc, DWORD dir_type)
{
	const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
	RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);

	for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++)	{
		RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
		if (entry.DataIsDirectory == dir_type)
			return reinterpret_cast<const RESOURCE_DIRECTORY *>(data_ + entry.OffsetToDirectory);
	}
	return NULL;
}

const RESOURCE_DIRECTORY *ResourceManager::FindResourceEntry(LPCWSTR type, LPCWSTR name, WORD language)
{
	if (!data_)
		return NULL;

	size_t i, j;

	const RESOURCE_DIRECTORY *directory = reinterpret_cast<const RESOURCE_DIRECTORY *>(data_);

	for (i = 0; i < 2; i++){
		LPCWSTR value = (!i) ? type : name;
		directory = FindEntry(directory, value, TRUE);
		if (!directory)
			return NULL;
	}

	LANGID lang_list[10];
	size_t lang_count = 0;

	/* specified language */
	lang_list[lang_count++] = language;

	/* specified language with neutral sublanguage */
	lang_list[lang_count++] = MAKELANGID(PRIMARYLANGID(language), SUBLANG_NEUTRAL);

	/* neutral language with neutral sublanguage */
	lang_list[lang_count++] = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	/* if no explicitly specified language, try some defaults */
	if (PRIMARYLANGID(language) == LANG_NEUTRAL) {
		/* user defaults, unless SYS_DEFAULT sublanguage specified */
		if (SUBLANGID(language) != SUBLANG_SYS_DEFAULT) {
			if (get_thread_ui_language_) {
				typedef LANGID(WINAPI tGetThreadUILanguage)();
				lang_list[lang_count++] = reinterpret_cast<tGetThreadUILanguage *>(get_thread_ui_language_)();
			}

			/* current thread locale language */
			lang_list[lang_count++] = LANGIDFROMLCID(GetThreadLocale());

			/* user locale language */
			lang_list[lang_count++] = LANGIDFROMLCID(GetUserDefaultLCID());

			/* user locale language with neutral sublanguage */
			lang_list[lang_count++] = MAKELANGID(PRIMARYLANGID(GetUserDefaultLCID()), SUBLANG_NEUTRAL);
		}

		/* now system defaults */

		/* system locale language */
		lang_list[lang_count++] = LANGIDFROMLCID(GetSystemDefaultLCID());

		/* system locale language with neutral sublanguage */
		lang_list[lang_count++] = MAKELANGID(PRIMARYLANGID(GetSystemDefaultLCID()), SUBLANG_NEUTRAL);

		/* English */
		lang_list[lang_count++] = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
	}

	for (i = 0; i < lang_count; i++) {
		LANGID lang = lang_list[i];
		bool is_processed = false;
		for (j = 0; j < i; j++) {
			if (lang_list[j] == lang) {
				is_processed = true;
				break;
			}
		}
		if (is_processed)
			continue;

		if (const RESOURCE_DIRECTORY *entry = FindEntryById(directory, lang, FALSE))
			return entry;
	}

	return FindFirstEntry(directory, FALSE);
}

LPWSTR ResourceManager::InternalFindStringResource(HMODULE module, UINT id)
{
	HRSRC res_info = InternalFindResourceExW(module, LPWSTR(RT_STRING), MAKEINTRESOURCEW((LOWORD(id) >> 4) + 1), 
		0);
	if (!res_info)
		return NULL;

	LPWSTR res = reinterpret_cast<LPWSTR>(InternalLoadResource(module, res_info));
	if (!res)
		return NULL;

	IMAGE_RESOURCE_DATA_ENTRY *entry = reinterpret_cast<IMAGE_RESOURCE_DATA_ENTRY *>(res_info);
	UINT res_size = entry->Size / sizeof(wchar_t);
	UINT size = 0;
	UINT str_len = 0;

	for (int i = id & 0x000f; i >= 0; i--) {
		str_len = res[size] + 1;
		size += str_len;
		if (size > res_size) 
			return NULL;
	}

	return (res + size - str_len);
}

int ResourceManager::InternalLoadStringW(LPCWSTR res, LPWSTR buffer, int buffer_max)
{
	if (!buffer || (buffer_max < 0))
		return 0;

	if (buffer_max == 0) {
		*(reinterpret_cast<LPCWSTR *>(buffer)) = res + 1;
		return *res;
	}

	int i = std::min<int>(buffer_max - 1, *res);
	if (i > 0) {
		memcpy(buffer, res + 1, i * sizeof (WCHAR));
		buffer[i] = 0;
		return i;
	} 
	if (buffer_max > 0)
		buffer[0] = 0;
	return 0;
}

int ResourceManager::InternalLoadStringA(LPCWSTR res, LPSTR buffer, int buffer_max)
{
	if (!buffer || (buffer_max <= 0))
		return 0;

	int i = WideCharToMultiByte(CP_ACP, 0, res + 1, *res, NULL, 0, NULL, NULL);
	if (i > 0) {
		if (i > buffer_max - 1) {
			LPSTR str = new char[i];
			i = std::min(buffer_max - 1, WideCharToMultiByte(CP_ACP, 0, res + 1, *res, str, i, NULL, NULL));
			memcpy(buffer, str, i * sizeof (CHAR));
			delete [] str;
		} else {
			i = WideCharToMultiByte(CP_ACP, 0, res + 1, *res, buffer, buffer_max - 1, NULL, NULL);
		}
		buffer[i] = 0;
		return i;
	}
	if (buffer_max > 0)
		buffer[0] = 0;
	return 0;
}

int ResourceManager::LoadStringA(HMODULE module, UINT uID, LPSTR buffer, int buffer_max)
{
	LPWSTR res = InternalFindStringResource(module, uID);
	if (res)
		return InternalLoadStringA(res, buffer, buffer_max);
	return TrueLoadStringA(module, uID, buffer, buffer_max);
}

int ResourceManager::LoadStringW(HMODULE module, UINT id, LPWSTR buffer, int buffer_max)
{
	LPWSTR res = InternalFindStringResource(module, id);
	if (res)
		return InternalLoadStringW(res, buffer, buffer_max);
	return TrueLoadStringW(module, id, buffer, buffer_max);
}

NTSTATUS ResourceManager::LdrFindResource_U(HMODULE module, const LDR_RESOURCE_INFO *res_info, ULONG level, const IMAGE_RESOURCE_DATA_ENTRY **entry)
{
	if (level == 3) {
		HRSRC res = InternalFindResourceExW(module, res_info->Type, res_info->Name, res_info->Language);
		if (res) {
			*entry = reinterpret_cast<IMAGE_RESOURCE_DATA_ENTRY *>(res);
			return 0;
		}
	}
	return TrueLdrFindResource_U(module, res_info, level, entry);
}

NTSTATUS ResourceManager::LdrAccessResource(HMODULE module, const IMAGE_RESOURCE_DATA_ENTRY *entry, void **res, ULONG *size)
{
	HGLOBAL h = InternalLoadResource(module, (HRSRC)entry);
	if (h) {
		if (res)
			*res = h;
		if (size)
			*size = entry->Size;
		return 0;
	}
	return TrueLdrAccessResource(module, entry, res, size);
}

BOOL ResourceManager::EnumResourceNamesA(HMODULE module, LPCSTR type, ENUMRESNAMEPROCA enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_) { 
		LPWSTR new_type = NULL;
		const RESOURCE_DIRECTORY *directory_enc;

		if (CreateResourceName(type, new_type)) {
			directory_enc = FindEntry(reinterpret_cast<const RESOURCE_DIRECTORY *>(data_), new_type, TRUE);
		} else {
			directory_enc = NULL;
		}
		FreeResourceName(new_type);

		if (directory_enc) {
			const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
			RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
			for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
				RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
				BOOL res;
				if (entry.NameIsString) {
					LPSTR name = DecryptStringA(reinterpret_cast<LPCWSTR>(data_ + entry.NameOffset));
					res = enum_func(module, type, name, param);
					delete [] name;
				} else {
					res = enum_func(module, type, MAKEINTRESOURCEA(entry.Id), param);
				}
				
				if (!res)
					break;
			}
			return TRUE;
		}
	}
	return ::EnumResourceNamesA(module, type, enum_func, param);
}

BOOL ResourceManager::EnumResourceNamesW(HMODULE module, LPCWSTR type, ENUMRESNAMEPROCW enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_) { 
		LPWSTR new_type = NULL;
		const RESOURCE_DIRECTORY *directory_enc;

		if (CreateResourceName(type, new_type)) {
			directory_enc = FindEntry(reinterpret_cast<const RESOURCE_DIRECTORY *>(data_), new_type, TRUE);
		} else {
			directory_enc = NULL;
		}
		FreeResourceName(new_type);

		if (directory_enc) {
			const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
			RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
			for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
				RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
				BOOL res;
				if (entry.NameIsString) {
					LPWSTR name = DecryptStringW(reinterpret_cast<LPCWSTR>(data_ + entry.NameOffset));
					res = enum_func(module, type, name, param);
					delete [] name;
				} else {
					res = enum_func(module, type, MAKEINTRESOURCEW(entry.Id), param);
				}

				if (!res)
					break;
			}
			return TRUE;
		}
	}
	return ::EnumResourceNamesW(module, type, enum_func, param);
}

BOOL ResourceManager::EnumResourceLanguagesA(HMODULE module, LPCSTR type, LPCSTR name, ENUMRESLANGPROCA enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_)	{ 
		const RESOURCE_DIRECTORY *directory_enc = reinterpret_cast<const RESOURCE_DIRECTORY *>(data_);

		for (size_t i = 0; i < 2; i++) {
			LPWSTR value = NULL;
			if (CreateResourceName((!i) ? type : name, value)) {
				directory_enc = FindEntry(directory_enc, value, TRUE);
			} else {
				directory_enc = NULL;
			}

			FreeResourceName(value);

			if (!directory_enc)
				break;
		}

		if (directory_enc) {
			const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
			RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
			for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
				RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
				if (!enum_func(module, type, name, entry.Id, param))
					break;
			}
			return TRUE;
		}
	}
	return ::EnumResourceLanguagesA(module, type, name, enum_func, param);
}

BOOL ResourceManager::EnumResourceLanguagesW(HMODULE module, LPCWSTR type, LPCWSTR name, ENUMRESLANGPROCW enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_)	{ 
		const RESOURCE_DIRECTORY *directory_enc = reinterpret_cast<const RESOURCE_DIRECTORY *>(data_);

		for (size_t i = 0; i < 2; i++) {
			LPWSTR value = NULL;
			if (CreateResourceName((!i) ? type : name, value)) {
				directory_enc = FindEntry(directory_enc, value, TRUE);
			} else {
				directory_enc = NULL;
			}

			FreeResourceName(value);

			if (!directory_enc)
				break;
		}

		if (directory_enc) {
			const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
			RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
			for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
				RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
				if (!enum_func(module, type, name, entry.Id, param))
					break;
			}
			return TRUE;
		}
	}
	return ::EnumResourceLanguagesW(module, type, name, enum_func, param);
}

BOOL ResourceManager::EnumResourceTypesA(HMODULE module, ENUMRESTYPEPROCA enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_) { 
		const RESOURCE_DIRECTORY *directory_enc = reinterpret_cast<const RESOURCE_DIRECTORY *>(data_);
		const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
		RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
		for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
			RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
			BOOL res;
			if (entry.NameIsString) {
				LPSTR type = DecryptStringA(reinterpret_cast<LPCWSTR>(data_ + entry.NameOffset));
				res = enum_func(module, type, param);
				delete [] type;
			} else {
				res = enum_func(module, MAKEINTRESOURCEA(entry.Id), param);
			}

			if (!res)
				break;
		}
	}
	return ::EnumResourceTypesA(module, enum_func, param);
}

BOOL ResourceManager::EnumResourceTypesW(HMODULE module, ENUMRESTYPEPROCW enum_func, LONG_PTR param)
{
	if (!module)
		module = GetModuleHandle(NULL); 

	if (module == instance_)	{ 
		const RESOURCE_DIRECTORY *directory_enc = reinterpret_cast<const RESOURCE_DIRECTORY *>(data_);
		const RESOURCE_DIRECTORY_ENTRY *entry_enc = reinterpret_cast<const RESOURCE_DIRECTORY_ENTRY *>(directory_enc + 1);
		RESOURCE_DIRECTORY directory = DecryptDirectory(directory_enc);
		for (size_t i = 0; i < directory.NumberOfNamedEntries + directory.NumberOfIdEntries; i++) {
			RESOURCE_DIRECTORY_ENTRY entry = DecryptDirectoryEntry(&entry_enc[i]);
			BOOL res;
			if (entry.NameIsString) {
				LPWSTR type = DecryptStringW(reinterpret_cast<LPCWSTR>(data_ + entry.NameOffset));
				res = enum_func(module, type, param);
				delete [] type;
			} else {
				res = enum_func(module, MAKEINTRESOURCEW(entry.Id), param);
			}

			if (!res)
				break;
		}
	}
	return ::EnumResourceTypesW(module, enum_func, param);
}

/**
 * VirtualResource
 */

VirtualResource::VirtualResource(const RESOURCE_DATA_ENTRY *entry_enc, uint32_t key) 
	: entry_(entry_enc), address_(NULL)
{
	RESOURCE_DATA_ENTRY entry;
	entry.OffsetToData = entry_enc->OffsetToData ^ key;
	entry.Size = entry_enc->Size ^ key;
	entry.CodePage = entry_enc->CodePage ^ key;
	entry.Reserved = entry_enc->Reserved ^ key;

	handle_.OffsetToData = entry.OffsetToData;
	handle_.CodePage = entry.CodePage;
	handle_.Size = entry.Size;
	handle_.Reserved = entry.Reserved;
}

VirtualResource::~VirtualResource()
{
	delete [] address_;
}

HGLOBAL VirtualResource::Decrypt(HMODULE instance, uint32_t key)
{
	if (!address_) {
		address_ = new uint8_t[handle_.Size];
		uint8_t *source = reinterpret_cast<uint8_t *>(instance) + handle_.OffsetToData;
		for (size_t i = 0; i < handle_.Size; i++) {
			address_[i] = static_cast<uint8_t>(source[i] ^ (_rotl32(key, static_cast<int>(i)) + i));
		}
	}
	return static_cast<HGLOBAL>(address_);
}

/**
 * VirtualResourceList
 */

VirtualResourceList::~VirtualResourceList()
{
	for (size_t i = 0; i < size(); i++) {
		VirtualResource *resource = v_[i];
		delete resource;
	}
	v_.clear();
}

VirtualResource *VirtualResourceList::Add(const RESOURCE_DATA_ENTRY *entry, uint32_t key)
{
	VirtualResource *resource = new VirtualResource(entry, key);
	v_.push_back(resource);
	return resource;
}

void VirtualResourceList::Delete(size_t index)
{
	VirtualResource *resource = v_[index];
	delete resource;
	v_.erase(index);
}	

size_t VirtualResourceList::IndexByEntry(const RESOURCE_DATA_ENTRY *entry) const
{
	for (size_t i = 0; i < size(); i++) {
		if (v_[i]->entry() == entry)
			return i;
	}
	return -1;
}

size_t VirtualResourceList::IndexByHandle(HRSRC handle) const
{
	for (size_t i = 0; i < size(); i++) {
		if (v_[i]->handle() == handle)
			return i;
	}
	return -1;
}

#endif