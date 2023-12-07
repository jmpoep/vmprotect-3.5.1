#include "context_menu.h"
#include "resource.h"

extern HINSTANCE instance;
extern long dll_ref_count;

#define IDM_PROTECT		0

/**
 * ContextMenuExt
 */

ContextMenuExt::ContextMenuExt(void) : ref_count_(1)
{
	InterlockedIncrement(&dll_ref_count);
}

ContextMenuExt::~ContextMenuExt(void)
{
	InterlockedDecrement(&dll_ref_count);
}

IFACEMETHODIMP ContextMenuExt::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(ContextMenuExt, IContextMenu), //-V221
		QITABENT(ContextMenuExt, IContextMenu2), //-V221
		QITABENT(ContextMenuExt, IContextMenu3), //-V221
		QITABENT(ContextMenuExt, IShellExtInit), //-V221
		{ 0 },
#pragma warning( push )
#pragma warning( disable: 4838 )
	};
#pragma warning( pop )
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ContextMenuExt::AddRef()
{
	return InterlockedIncrement(&ref_count_);
}

IFACEMETHODIMP_(ULONG) ContextMenuExt::Release()
{
	ULONG res = InterlockedDecrement(&ref_count_);
	if (!res)
		delete this;
	return res;
}

#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

std::wstring PathName(LPCITEMIDLIST parent, LPCITEMIDLIST child)
{
	IShellFolder *shellFolder = NULL;
	IShellFolder *parentFolder = NULL;
	STRRET name;
	TCHAR * szDisplayName = NULL;
	std::wstring ret;
	HRESULT hr;

	hr = ::SHGetDesktopFolder(&shellFolder);
	if (!SUCCEEDED(hr) || shellFolder == NULL)
		return ret;
	if (parent)
	{
		hr = shellFolder->BindToObject(parent, 0, IID_IShellFolder, (void**) &parentFolder);
		if (!SUCCEEDED(hr) || parentFolder == NULL)
			parentFolder = shellFolder;
	} 
	else 
	{
		parentFolder = shellFolder;
	}

	if ((parentFolder != 0)&&(child != 0))
	{
		hr = parentFolder->GetDisplayNameOf(child, SHGDN_NORMAL | SHGDN_FORPARSING, &name);
		if (!SUCCEEDED(hr))
		{
			parentFolder->Release();
			return ret;
		}
		hr = StrRetToStr (&name, child, &szDisplayName);
		if (!SUCCEEDED(hr))
			return ret;
	}
	parentFolder->Release();
	if (szDisplayName == NULL)
	{
		CoTaskMemFree(szDisplayName);
		return ret;
	}
	ret = szDisplayName;
	CoTaskMemFree(szDisplayName);
	return ret;
}

IFACEMETHODIMP ContextMenuExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	file_names_.clear();
	if (!pDataObj)
		return E_INVALIDARG;

	static int g_shellidlist = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

	// get selected files/folders
	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {(CLIPFORMAT)g_shellidlist,
			(DVTARGETDEVICE FAR *)NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			if (pidlFolder)
			{
				FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}


				HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
				if ( NULL == drop )
				{
					ReleaseStgMedium ( &stg );
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, (UINT)-1, NULL, 0);
				for (int i = 0; i < count; i++)
				{
					// find the path length in chars
					UINT len = DragQueryFile(drop, i, NULL, 0);
					if (len == 0)
						continue;
					TCHAR * szFileName = new TCHAR[len+1];
					if (0 == DragQueryFile(drop, i, szFileName, len+1))
					{
						delete [] szFileName;
						continue;
					}
					file_names_.push_back(szFileName);
					delete [] szFileName;
				} // for (int i = 0; i < count; i++)
				GlobalUnlock ( drop );
				ReleaseStgMedium ( &stg );
			}
			else
			{
				//Enumerate PIDLs which the user has selected
				CIDA* cida = (CIDA*)GlobalLock(medium.hGlobal);
				LPCITEMIDLIST parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				for (int i = 0; i < count; ++i)
				{
					LPCITEMIDLIST child (GetPIDLItem (cida, i));
					file_names_.push_back(PathName(parent, child));
				} // for (int i = 0; i < count; ++i)
				GlobalUnlock(medium.hGlobal);
			}

			ReleaseStgMedium ( &medium );
			if (medium.pUnkForRelease)
			{
				IUnknown* relInterface = (IUnknown*)medium.pUnkForRelease;
				relInterface->Release();
			}
		}
	}

	return file_names_.empty() ? E_FAIL : S_OK;
}

IFACEMETHODIMP ContextMenuExt::QueryContextMenu(
	HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	if (CMF_DEFAULTONLY & uFlags)
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));

	MENUITEMINFOW mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
	mii.wID = idCmdFirst + IDM_PROTECT;
	mii.fType = MFT_STRING;
	mii.dwTypeData = L"Protect with VMProtect";
	mii.fState = MFS_ENABLED;
	mii.hbmpItem = utils_.IconToBitmapPARGB32(instance, IDI_ICON);
	if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
		return HRESULT_FROM_WIN32(GetLastError());

	MENUITEMINFOW sep = {};
	sep.cbSize = sizeof(sep);
	sep.fMask = MIIM_TYPE;
	sep.fType = MFT_SEPARATOR;
	if (!InsertMenuItemW(hMenu, indexMenu + 1, TRUE, &sep))
		return HRESULT_FROM_WIN32(GetLastError());

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_PROTECT + 1));
}

IFACEMETHODIMP ContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO command_info)
{
	if (HIWORD(command_info->lpVerb))
		return E_INVALIDARG;

	switch (LOWORD(command_info->lpVerb)) {
	case IDM_PROTECT:
		{
			wchar_t module_name[MAX_PATH];
			if (GetModuleFileNameW(instance, module_name, _countof(module_name)) != 0) {
				wchar_t *name = NULL;
				wchar_t *ptr = module_name;
				while (*ptr) {
					if (*ptr == '/' || *ptr == '\\')
						name = ptr + 1;
					ptr++;
				}
				if (name) {
					wcscpy_s(name, module_name + MAX_PATH - name - 1, L"VMProtect.exe"); //-V782
					for (size_t i = 0; i < file_names_.size(); i++) {
						wchar_t file_name[MAX_PATH * 2 + 2];
						wnsprintf(file_name, MAX_PATH * 2, L"\"%s\" \"%s\"", module_name, file_names_[i].c_str());
						PROCESS_INFORMATION pi = PROCESS_INFORMATION();
						STARTUPINFO si = STARTUPINFO();
						if (!CreateProcessW(module_name, file_name, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
							MessageBoxW(command_info->hwnd, L"Error executing VMProtect", L"Error", MB_ICONERROR | MB_OK);
							break;
						}
						CloseHandle(pi.hProcess); 
						CloseHandle(pi.hThread); 
					}
				}
			}
		} 
		break;
	}
	return S_OK;
}

IFACEMETHODIMP ContextMenuExt::GetCommandString(UINT_PTR idCommand, 
	UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
	if (idCommand == IDM_PROTECT) {
		switch (uFlags) {
		case GCS_HELPTEXTA:
			strncpy_s(reinterpret_cast<PSTR>(pszName), cchMax, "Protect with VMProtect", cchMax);
			return S_OK;
		case GCS_HELPTEXTW:
			wcsncpy_s(reinterpret_cast<PWSTR>(pszName), cchMax, L"Protect with VMProtect", cchMax);
			return S_OK;
		default:
			return E_NOTIMPL;
		}
	}
	return E_INVALIDARG;
}

IFACEMETHODIMP ContextMenuExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res;
	return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

IFACEMETHODIMP ContextMenuExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LRESULT res;
	if (!pResult)
		pResult = &res;
	*pResult = FALSE;

	switch (uMsg)
	{
	case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*)lParam;
			if (lpmis==NULL)
				break;
			lpmis->itemWidth += 2;
			if(lpmis->itemHeight < 16)
				lpmis->itemHeight = 16;
			*pResult = TRUE;
		}
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
			if (!lpdis || (lpdis->CtlType != ODT_MENU))
				break; //not for a menu
			HICON icon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
			if (!icon)
				break;
			DrawIconEx(lpdis->hDC,
				1,
				lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - 16) / 2,
				icon, 16, 16,
				0, 0, DI_NORMAL);
			DestroyIcon(icon);
			*pResult = TRUE;
		}
		break;
	default:
		return NOERROR;
	}

	return NOERROR;
}
