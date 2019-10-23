#include "SziFileEraseMenu.h"
#include "SziFileEraseDialog.h"

#include <new>
#include <Shlwapi.h>
#include "resource.h"
#include <strsafe.h>


extern HINSTANCE	gInstance;
extern long			gDllRef;

#define IDM_ERASE             0

bool SziFileIsDirectory(const std::wstring& objectName);
bool MessageBoxQuestion(HWND hDesktop) ;

szi::ClassFactory::ClassFactory() : m_cRef(1) 
{
	InterlockedIncrement(&gDllRef);
}

szi::ClassFactory::~ClassFactory() 
{
	InterlockedDecrement(&gDllRef);
}

IFACEMETHODIMP szi::ClassFactory::QueryInterface(REFIID riid, void **ppv) 
{
	static const QITAB qit[] =
	{
		QITABENT(ClassFactory, IClassFactory),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) szi::ClassFactory::AddRef() 
{
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) szi::ClassFactory::Release() 
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if(0 == cRef)
	{
		delete this;
	}
	return cRef;
}

IFACEMETHODIMP szi::ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) 
{
	HRESULT hr = CLASS_E_NOAGGREGATION;
	
	if(pUnkOuter == NULL)
	{
		FileContextMenuExt * pExt = new (std::nothrow) FileContextMenuExt();
		if(pExt)
		{
			hr = pExt->QueryInterface(riid, ppv);
			pExt->Release();
		} else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

IFACEMETHODIMP szi::ClassFactory::LockServer(BOOL fLock) 
{
	if(fLock) 
	{
		InterlockedIncrement(&gDllRef);
	} else 
	{
		InterlockedDecrement(&gDllRef);
	}
	return S_OK;
}


szi::FileContextMenuExt::FileContextMenuExt(void) : 
	m_cRef(1),
	m_pszMenuText(MENU_MAIN_TEXT),
	m_pwszVerb(L"szifileerase"),
	m_pwszVerbHelpText(MENU_VERBHELPTEXT) 
{
	InterlockedIncrement(&gDllRef);

	m_hMenuBmp = LoadImage(gInstance, MAKEINTRESOURCE(IDB_MAINICON), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
}

szi::FileContextMenuExt::~FileContextMenuExt(void) 
{
	if(m_hMenuBmp) 
	{
		DeleteObject(m_hMenuBmp);
		m_hMenuBmp = NULL;
	}

	InterlockedDecrement(&gDllRef);
}


void szi::FileContextMenuExt::OnVerbDisplayFileName(HWND hWnd) 
{
	szi::SziFileEraseDialog		lFileEraser;
	std::list<std::wstring>		lFiles;
	std::list<std::wstring>		lDir;
	bool						lIsError = false;

	if(MessageBoxQuestion(hWnd)) 
	{
		for(auto xObject : m_SelectFiles)
		{
			if(!SziFileIsDirectory(xObject))
			{
				lFiles.push_back(xObject);
			} else 
			{
				lDir.push_back(xObject);
			}
		}

		if(lFileEraser.Init()) 
		{
			if(!lFileEraser.EraseFiles(lFiles))
			{
				lIsError = true;
			} else 
			{
				lIsError = !lFileEraser.EraseDir(lDir);
			}
			lFileEraser.Destroy();
		} else 
		{
			lIsError = true;
		}
	}

	if(lIsError) 
	{
		const std::wstring lError = lFileEraser.GetLastError();
		MessageBox(hWnd, lError.c_str(), DIALOG_TITLE, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	}
}


IFACEMETHODIMP szi::FileContextMenuExt::QueryInterface(REFIID riid, void **ppv) 
{
	static const QITAB qit[] =
	{
		QITABENT(FileContextMenuExt, IContextMenu),
		QITABENT(FileContextMenuExt, IShellExtInit),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) szi::FileContextMenuExt::AddRef() 
{
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) szi::FileContextMenuExt::Release() 
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if(0 == cRef) 
	{
		delete this;
	}
	return cRef;
}


IFACEMETHODIMP szi::FileContextMenuExt::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID) 
{	
	HRESULT		hr = E_FAIL;
	FORMATETC	fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM	stm;
	wchar_t		selectedFile[MAX_PATH] = L"\0";

	if(NULL == pDataObj) 
	{
		return E_INVALIDARG;
	}

	if(SUCCEEDED(pDataObj->GetData(&fe, &stm)))
	{
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if(hDrop != NULL) 
		{
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			for (UINT i = 0; i < nFiles; i++)
			{
				ZeroMemory(&selectedFile, ARRAYSIZE(selectedFile));
				if(0 != DragQueryFile(hDrop, i, selectedFile, ARRAYSIZE(selectedFile)))
				{
					m_SelectFiles.push_back(selectedFile);
					hr = S_OK;
				}
			}
			GlobalUnlock(stm.hGlobal);
		}
		ReleaseStgMedium(&stm);
	}
	return hr;
}

IFACEMETHODIMP szi::FileContextMenuExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) 
{
	// If uFlags include CMF_DEFAULTONLY then we should not do anything.
	if(CMF_DEFAULTONLY & uFlags) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
	}

	MENUITEMINFO mii = { sizeof(mii) };

	mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = m_pszMenuText;
	mii.fState = MFS_ENABLED;
	mii.hbmpItem = static_cast<HBITMAP>(m_hMenuBmp);
	mii.hSubMenu = CreatePopupMenu();

	if(!InsertMenu(mii.hSubMenu, 0, MF_BYPOSITION, idCmdFirst + IDM_ERASE, MENU_SUBMENU_TEXT))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if(!InsertMenuItem(hMenu, indexMenu, TRUE, &mii))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	MENUITEMINFO sep = { sizeof(sep) };
	sep.fMask = MIIM_TYPE;
	sep.fType = MFT_SEPARATOR;

	if(!InsertMenuItem(hMenu, indexMenu + 1, TRUE, &sep)) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_ERASE + 1));
}

IFACEMETHODIMP szi::FileContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici) 
{
	BOOL fUnicode = FALSE;

	if(pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) && (pici->fMask & CMIC_MASK_UNICODE)) 
	{
		fUnicode = TRUE;
	}

	if(fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW)) 
	{
		if(StrCmpIW(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW, m_pwszVerb) == 0) 
		{
			OnVerbDisplayFileName(pici->hwnd);
		} else 
		{
			return E_FAIL;
		}
	} else
	{
		if(LOWORD(pici->lpVerb) == IDM_ERASE) 
		{
			OnVerbDisplayFileName(pici->hwnd);
		} else 
		{
			return E_FAIL;
		}
	}
	return S_OK;
}

IFACEMETHODIMP szi::FileContextMenuExt::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *, LPSTR pszName, UINT cchMax) 
{
	HRESULT hr = E_INVALIDARG;

	if(idCommand == IDM_ERASE) 
	{
		switch(uFlags)
		{
			case GCS_HELPTEXTW:
				hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, m_pwszVerbHelpText);
				break;
			case GCS_VERBW:
				hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, m_pwszVerb);
				break;
			default:
				hr = S_OK;
		}
	}
	return hr;
}
