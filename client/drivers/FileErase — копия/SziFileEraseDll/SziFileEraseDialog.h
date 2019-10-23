#pragma once

#include <list>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include <WinDef.h>
#include <Windows.h>
#include <tchar.h>
#include <WinIoCtl.h>
#include <Shellapi.h>
#include <Shobjidl.h>
#include <Shlobj.h>

#define DIALOG_TITLE			L"Удаление файла"
#define DIALOG_INIT_ERROR		L"Не удалось произвести безопасное удаление. Ошибка СЗИ «Кольчуга»"
#define DIALOG_WININIT_ERROR	L"Не удалось произвести безопасное удаление. Ошибка 0x%X"
#define DIALOG_ERROR_20			L"Не удалось произвести безопасное удаление. Ошибка: Процесс не может получить доступ к файлу, так как \"%s\" файл занят другим процессом."
#define DIALOG_QUESTION			L"После проведения операции безопасного удаления будет невозможно восстановить выбранный файл. Продолжить?"
#define DIALOG_ERROR			L"Не удалось произвести безопасное удаление. Ошибка драйвера системы. Код ошибки: 0x%X"

#define MENU_MAIN_TEXT			L"&Функции СЗИ «Кольчуга»"
#define MENU_SUBMENU_TEXT		L"Безопасно удалить"
#define MENU_VERBHELPTEXT		L"Безопасная очистка файла"

#define MAX_WINDOWSPATH			(MAX_PATH * sizeof(WCHAR))


namespace szi
{
	class SziFileEraseDialog 
	{
	public:
		typedef std::list<std::wstring> ListFiles;
	public:
		SziFileEraseDialog();
		~SziFileEraseDialog();

		bool Init();
		void Destroy();
		bool EraseFiles(const ListFiles&);
		bool EraseDir(const ListFiles&);
		std::wstring GetLastError() const;
	private:
		bool IsAccessWrite(const std::wstring&) const;
	private:
		IProgressDialog*	dialog_;
	mutable std::wstring	error_;
	};
	
}
