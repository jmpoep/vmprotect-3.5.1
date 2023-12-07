#ifndef CONTEXT_H
#define CONTEXT_H

#include <windows.h>
#pragma warning( disable : 4091 )
#include <shlobj.h>
#include <shlwapi.h>
#pragma warning( default : 4091 )
#include <string>
#include <vector>
#include "bitmap_utils.h"

#pragma comment(lib, "shlwapi.lib")

class ContextMenuExt : public IShellExtInit, public IContextMenu3
{
public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IShellExtInit
    IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

    // IContextMenu
    IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO command_info);
    IFACEMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);
    IFACEMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    IFACEMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	
    ContextMenuExt(void);

protected:
    ~ContextMenuExt(void);

private:
    long ref_count_;
	BitmapUtils utils_;
	std::vector<std::wstring> file_names_;
};

#endif