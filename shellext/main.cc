#include "main.h"
#include "initguid.h"
#include "context_menu.h"
#include <shlwapi.h>

long dll_ref_count = 0;
HMODULE instance = NULL;

// {6416B534-5A38-47BA-A8DB-4253F49DC7D3}
DEFINE_GUID(CLSID_VMPContextMenu, 
0x6416B534, 0x5A38, 0x47BA, 0xA8, 0xDB, 0x42, 0x53, 0xF4, 0x9D, 0xC7, 0xD3);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		instance = hModule;
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

/**
 * export functions
 */

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
	if (IsEqualCLSID(CLSID_VMPContextMenu, rclsid)) {
		ClassFactory *class_factory;
		try {
			class_factory = new ClassFactory();
		} catch(...) {
			return E_OUTOFMEMORY;
		}

		if (class_factory) {
			HRESULT res = class_factory->QueryInterface(riid, ppv);
			class_factory->Release();
			return res;
		} else {
			return E_OUTOFMEMORY;
		}
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
	return dll_ref_count ? S_FALSE : S_OK;
}

static HRESULT SetRegistryKeyAndValue(HKEY root, PCWSTR sub_key, PCWSTR value_name, PCWSTR data)
{
	HKEY key = NULL;
    HRESULT res = HRESULT_FROM_WIN32(RegCreateKeyExW(root, sub_key, 0,  NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL));
    if (SUCCEEDED(res)) {
		if (data) {
			DWORD size = lstrlenW(data) * sizeof(*data);
			res = HRESULT_FROM_WIN32(RegSetValueExW(key, value_name, 0, REG_SZ, reinterpret_cast<const BYTE *>(data), size));
		}
		RegCloseKey(key);
	}
	return res;
}

static HRESULT GetRegistryKeyAndValue(HKEY root, PCWSTR sub_key, PCWSTR value_name, PWSTR data, DWORD size)
{
	HKEY key = NULL;
	HRESULT res = HRESULT_FROM_WIN32(RegOpenKeyExW(root, sub_key, 0, KEY_READ, &key));
	if (SUCCEEDED(res)){
		res = HRESULT_FROM_WIN32(RegQueryValueExW(key, value_name, NULL, NULL, reinterpret_cast<BYTE *>(data), &size));
		RegCloseKey(key);
	}
	return res;
}

static wchar_t *shell_ext_name = L"VMProtect Shell Extension";
static wchar_t *cls_id_mask = L"CLSID\\%s";
static wchar_t *cls_id_inproc_mask = L"CLSID\\%s\\InprocServer32";
static wchar_t *context_menu_key_name = L"*\\shellex\\ContextMenuHandlers\\VMProtect";

STDAPI DllRegisterServer(void)
{
	wchar_t file_name[MAX_PATH];
	if (GetModuleFileNameW(instance, file_name, _countof(file_name)) == 0)
		return HRESULT_FROM_WIN32(GetLastError());

	wchar_t cls_id[MAX_PATH];
	StringFromGUID2(CLSID_VMPContextMenu, cls_id, _countof(cls_id));

	wchar_t sub_key[MAX_PATH];
	wsprintfW(sub_key, cls_id_mask, cls_id);
	HRESULT res = SetRegistryKeyAndValue(HKEY_CLASSES_ROOT, sub_key, NULL, shell_ext_name);
	if (SUCCEEDED(res)) {
		wsprintfW(sub_key, cls_id_inproc_mask, cls_id);
		res = SetRegistryKeyAndValue(HKEY_CLASSES_ROOT, sub_key, NULL, file_name);
	}
	if (SUCCEEDED(res))
		res = SetRegistryKeyAndValue(HKEY_CLASSES_ROOT, sub_key, L"ThreadingModel", L"Apartment");
	if (SUCCEEDED(res))
		res = SetRegistryKeyAndValue(HKEY_CLASSES_ROOT, context_menu_key_name, NULL, cls_id);
	return res;
}

STDAPI DllUnregisterServer(void)
{
	wchar_t cls_id[MAX_PATH];
	StringFromGUID2(CLSID_VMPContextMenu, cls_id, _countof(cls_id));

	wchar_t sub_key[MAX_PATH];
	wsprintfW(sub_key, cls_id_inproc_mask, cls_id);
	HRESULT res = HRESULT_FROM_WIN32(RegDeleteKeyW(HKEY_CLASSES_ROOT, sub_key));
	if (SUCCEEDED(res)) {
		wsprintfW(sub_key, cls_id_mask, cls_id);
		res = HRESULT_FROM_WIN32(RegDeleteKeyW(HKEY_CLASSES_ROOT, sub_key));
	}
	if (SUCCEEDED(res))
		res = HRESULT_FROM_WIN32(RegDeleteKeyW(HKEY_CLASSES_ROOT, context_menu_key_name));
	return res;
}

/**
 * ClassFactory
 */

ClassFactory::ClassFactory() 
	: ref_count_(1)
{
	InterlockedIncrement(&dll_ref_count);
}

ClassFactory::~ClassFactory()
{
	InterlockedDecrement(&dll_ref_count);
}

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] = 
	{
		QITABENT(ClassFactory, IClassFactory), //-V221
		{ 0 },
#pragma warning( push )
#pragma warning( disable: 4838 )
	};
#pragma warning( pop )
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
	return InterlockedIncrement(&ref_count_);
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
	ULONG res = InterlockedDecrement(&ref_count_);
	if (!res)
		delete this;
	return res;
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;

	ContextMenuExt *ext;
	try {
		ext = new ContextMenuExt();
	} catch (...) {
		return E_OUTOFMEMORY;
	}

    if (ext) {
        HRESULT res = ext->QueryInterface(riid, ppv);
        ext->Release();
		return res;
    } else {
		return E_OUTOFMEMORY;
	}
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL lock)
{
	if (lock)
		InterlockedIncrement(&dll_ref_count);
	else
		InterlockedDecrement(&dll_ref_count);
	return S_OK;
}