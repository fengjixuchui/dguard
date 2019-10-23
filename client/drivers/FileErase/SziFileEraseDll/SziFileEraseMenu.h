#pragma once

#include <unknwn.h>     // For IClassFactory
#include <windows.h>
#include <shlobj.h>     // For IShellExtInit and IContextMenu
#include <list>
#include <string>

namespace szi 
{
	class ClassFactory : public IClassFactory {
	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
		IFACEMETHODIMP_(ULONG) AddRef();
		IFACEMETHODIMP_(ULONG) Release();

		// IClassFactory
		IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
		IFACEMETHODIMP LockServer(BOOL fLock);

		ClassFactory();

	protected:
		~ClassFactory();

	private:
		long m_cRef;
	};

	class FileContextMenuExt : public IShellExtInit, public IContextMenu {
	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
		IFACEMETHODIMP_(ULONG) AddRef();
		IFACEMETHODIMP_(ULONG) Release();

		// IShellExtInit
		IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

		// IContextMenu
		IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
		IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
		IFACEMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

		FileContextMenuExt(void);

	protected:
		~FileContextMenuExt(void);

	private:

		void OnVerbDisplayFileName(HWND hWnd);
	private:
		long		m_cRef;
		std::list<std::wstring> m_SelectFiles;
		PWSTR		m_pszMenuText;
		HANDLE		m_hMenuBmp;
		PCWSTR		m_pwszVerb;
		PCWSTR		m_pwszVerbHelpText;
	};
}
