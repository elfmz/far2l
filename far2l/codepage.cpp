/*
codepage.cpp

Работа с кодовыми страницами
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "codepage.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "savefpos.hpp"
#include "keys.hpp"
#include "registry.hpp"
#include "language.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "config.hpp"

// Ключ где хранятся имена кодовых страниц
const wchar_t *NamesOfCodePagesKey = L"CodePages/Names";

const wchar_t *FavoriteCodePagesKey = L"CodePages/Favorites";

// Стандартные кодовое страницы
enum StandardCodePages
{
	SearchAll = 1,
	Auto = 2,
	KOI8 = 4,
	DOS = 8,
	ANSI = 16,
	UTF7 = 32,
	UTF8 = 64,
	UTF16LE = 128,
	UTF16BE = 256,
#if (__WCHAR_MAX__ > 0xffff)	
	UTF32LE = 512,
	UTF32BE = 1024,
	AllUtfBiggerThan8 = UTF16BE | UTF16LE | UTF32BE | UTF32LE,
	AllStandard = DOS | ANSI | KOI8 | UTF7 | UTF8 | UTF16BE | UTF16LE | UTF32BE | UTF32LE
#else
	AllUtfBiggerThan8 = UTF16BE | UTF16LE,
	AllStandard = DOS | ANSI | KOI8 | UTF7 | UTF8 | UTF16BE | UTF16LE
#endif	
};

// Источник вызова каллбака прохода по кодовым страницам
enum CodePagesCallbackCallSource
{
	CodePageSelect,
	CodePagesFill,
	CodePageCheck
};

// Номера контролов диалога редактирования имени коловой страницы
enum
{
	EDITCP_BORDER,
	EDITCP_EDIT,
	EDITCP_SEPARATOR,
	EDITCP_OK,
	EDITCP_CANCEL,
	EDITCP_RESET,
};

// Диалог
static HANDLE dialog;
// Идентифкатор диалога
static UINT control;
// Меню
static VMenu *CodePages = nullptr;
// Текущая таблица символов
static UINT currentCodePage;
// Количество выбранных и обыкновенных таблиц символов
static int favoriteCodePages, normalCodePages;
// Признак необходимости отображать таблицы символов для поиска
static bool selectedCodePages;
// Источник вызова каллбака для функции EnumSystemCodePages
static CodePagesCallbackCallSource CallbackCallSource;
// Признак того, что кодовая страница поддерживается
static bool CodePageSupported;

wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length, bool &IsCodePageNameCustom);

// Получаем кодовую страницу для элемента в меню
inline UINT GetMenuItemCodePage(int Position = -1)
{
	return static_cast<UINT>(reinterpret_cast<UINT_PTR>(CodePages->GetUserData(nullptr, 0, Position)));
}

inline UINT GetListItemCodePage(int Position = -1)
{
	return static_cast<UINT>(SendDlgMessage(dialog, DM_LISTGETDATA, control, Position));
}

// Проверяем попадает или нет позиция в диапазон стандартных кодовых страниц (правильность работы для разделителей не гарантируется)
inline bool IsPositionStandard(UINT position)
{
	return position<=(UINT)CodePages->GetItemCount()-favoriteCodePages-(favoriteCodePages?1:0)-normalCodePages-(normalCodePages?1:0);
}

// Проверяем попадает или нет позиция в диапазон любимых кодовых страниц (правильность работы для разделителей не гарантируется)
inline bool IsPositionFavorite(UINT position)
{
	return position>=(UINT)CodePages->GetItemCount()-normalCodePages;
}

// Проверяем попадает или нет позиция в диапазон обыкновенных кодовых страниц (правильность работы для разделителей не гарантируется)
inline bool IsPositionNormal(UINT position)
{
	UINT ItemCount = CodePages->GetItemCount();
	return position>=ItemCount-normalCodePages-favoriteCodePages-(normalCodePages?1:0) && position<ItemCount-normalCodePages;
}

// Формируем строку для визуального представления таблицы символов
void FormatCodePageString(UINT CodePage, const wchar_t *CodePageName, FormatString &CodePageNameString, bool IsCodePageNameCustom)
{
	if (CodePage!=CP_AUTODETECT)
	{
		CodePageNameString<<fmt::Width(5)<<CodePage<<BoxSymbols[BS_V1]<<(!IsCodePageNameCustom||CallbackCallSource==CodePagesFill?L' ':L'*');
	}
	CodePageNameString<<CodePageName;
}

// Добавляем таблицу символов
void AddCodePage(const wchar_t *codePageName, UINT codePage, int position, bool enabled, bool checked, bool IsCodePageNameCustom)
{
	if (CallbackCallSource == CodePagesFill)
	{
		// Вычисляем позицию вставляемого элемента
		if (position==-1)
		{
			FarListInfo info;
			SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
			position = info.ItemsNumber;
		}

		// Вставляем элемент
		FarListInsert item = {position};

		FormatString name;
		FormatCodePageString(codePage, codePageName, name, IsCodePageNameCustom);
		item.Item.Text = name;

		if (selectedCodePages && checked)
		{
			item.Item.Flags |= MIF_CHECKED;
		}

		if (!enabled)
		{
			item.Item.Flags |= MIF_GRAYED;
		}

		SendDlgMessage(dialog, DM_LISTINSERT, control, (LONG_PTR)&item);
		// Устанавливаем данные для элемента
		FarListItemData data;
		data.Index = position;
		data.Data = (void*)(DWORD_PTR)codePage;
		data.DataSize = sizeof(UINT);
		SendDlgMessage(dialog, DM_LISTSETDATA, control, (LONG_PTR)&data);
	}
	else
	{
		// Создаём новый элемент меню
		MenuItemEx item;
		item.Clear();

		if (!enabled)
			item.Flags |= MIF_GRAYED;

		FormatString name;
		FormatCodePageString(codePage, codePageName, name, IsCodePageNameCustom);
		item.strName = name;

		item.UserData = (char *)(UINT_PTR)codePage;
		item.UserDataSize = sizeof(UINT);

		// Добавляем новый элемент в меню
		if (position>=0)
			CodePages->AddItem(&item, position);
		else
			CodePages->AddItem(&item);

		// Если надо позиционируем курсор на добавленный элемент
		if (currentCodePage==codePage)
		{
			if ((CodePages->GetSelectPos()==-1 || GetMenuItemCodePage()!=codePage))
			{
				CodePages->SetSelectPos(position>=0?position:CodePages->GetItemCount()-1, 1);
			}
		}
	}
}

// Добавляем стандартную таблицу символов
void AddStandardCodePage(const wchar_t *codePageName, UINT codePage, int position = -1, bool enabled = true)
{
	bool checked = false;

	if (selectedCodePages && codePage!=CP_AUTODETECT)
	{
		FormatString strCodePageName;
		strCodePageName<<codePage;
		int selectType = 0;
		GetRegKey(FavoriteCodePagesKey, strCodePageName, selectType, 0);

		if (selectType & CPST_FIND)
			checked = true;
	}

	AddCodePage(codePageName, codePage, position, enabled, checked, false);
}

// Добавляем разделитель
void AddSeparator(LPCWSTR Label=nullptr,int position = -1)
{
	if (CallbackCallSource == CodePagesFill)
	{
		if (position==-1)
		{
			FarListInfo info;
			SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
			position = info.ItemsNumber;
		}

		FarListInsert item = {position};
		item.Item.Text = Label;
		item.Item.Flags = LIF_SEPARATOR;
		SendDlgMessage(dialog, DM_LISTINSERT, control, (LONG_PTR)&item);
	}
	else
	{
		MenuItemEx item;
		item.Clear();
		item.strName = Label;
		item.Flags = MIF_SEPARATOR;

		if (position>=0)
			CodePages->AddItem(&item, position);
		else
			CodePages->AddItem(&item);
	}
}

// Получаем количество элементов в списке
int GetItemsCount()
{
	if (CallbackCallSource == CodePageSelect)
	{
		return CodePages->GetItemCount();
	}
	else
	{
		FarListInfo info;
		SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
		return info.ItemsNumber;
	}
}

// Получаем позицию для вставки таблицы с учётом сортировки по номеру кодовой страницы
int GetCodePageInsertPosition(UINT codePage, int start, int length)
{
	for (int position=start; position < start+length; position++)
	{
		UINT itemCodePage;

		if (CallbackCallSource == CodePageSelect)
			itemCodePage = GetMenuItemCodePage(position);
		else
			itemCodePage = GetListItemCodePage(position);

		if (itemCodePage >= codePage)
			return position;
	}

	return start+length;
}

// Получаем информацию о кодовой странице
bool GetCodePageInfo(UINT CodePage, CPINFOEX &CodePageInfoEx)
{
	if (!WINPORT(GetCPInfoEx)(CodePage, 0, &CodePageInfoEx))
	{
		// GetCPInfoEx возвращает ошибку для кодовых страниц без имени (например 1125), которые
		// сами по себе работают. Так что, прежде чем пропускать кодовую страницу из-за ошибки,
		// пробуем получить для неё информауию через GetCPInfo
		CPINFO CodePageInfo;

		if (!WINPORT(GetCPInfo)(CodePage, &CodePageInfo))
			return false;

		CodePageInfoEx.MaxCharSize = CodePageInfo.MaxCharSize;
		CodePageInfoEx.CodePageName[0] = L'\0';
	}
	
	// BUBUG: Пока не поддерживаем многобайтовые кодовые страницы
	if (CodePageInfoEx.MaxCharSize != 1)
		return false;

	return true;
}

// Callback-функция получения таблиц символов
BOOL __stdcall EnumCodePagesProc(const wchar_t *lpwszCodePage)
{
	UINT codePage = _wtoi(lpwszCodePage);

	// Для функции проверки нас не интересует информация о кодовых страницах отличных от проверяемой
	if (CallbackCallSource == CodePageCheck && codePage != currentCodePage)
		return TRUE;

	// Получаем информацию о кодовой странице. Если информацию по какой-либо причине получить не удалось, то
	// для списков прожолжаем енумерацию, а для процедуры же проверки поддерживаемости кодовой страницы выходим
	CPINFOEX cpiex;
	if (!GetCodePageInfo(codePage, cpiex))
		return CallbackCallSource == CodePageCheck ? FALSE : TRUE;

	// Для функции провепки поддерживаемости кодовый страницы мы прошли все проверки и можем выходить
	if (CallbackCallSource == CodePageCheck)
	{
		CodePageSupported = true;
		return FALSE;
	}

	// Формируем имя таблиц символов
	bool IsCodePageNameCustom = false;
	wchar_t *codePageName = FormatCodePageName(_wtoi(lpwszCodePage), cpiex.CodePageName, sizeof(cpiex.CodePageName)/sizeof(wchar_t), IsCodePageNameCustom);
	// Получаем признак выбранности таблицы символов
	int selectType = 0;
	GetRegKey(FavoriteCodePagesKey, lpwszCodePage, selectType, 0);

	// Добавляем таблицу символов либо в нормальные, либо в выбранные таблицы симовлов
	if (selectType & CPST_FAVORITE)
	{
		// Если надо добавляем разделитель между выбранными и нормальными таблицами симовлов
		if (!favoriteCodePages)
			AddSeparator(MSG(MGetCodePageFavorites),GetItemsCount()-normalCodePages-(normalCodePages?1:0));

		// Добавляем таблицу символов в выбранные
		AddCodePage(
		    codePageName,
		    codePage,
		    GetCodePageInsertPosition(
		        codePage,
		        GetItemsCount()-normalCodePages-favoriteCodePages-(normalCodePages?1:0),
		        favoriteCodePages
		    ),
		    true,
		    selectType & CPST_FIND ? true : false,
			IsCodePageNameCustom
		);
		// Увеличиваем счётчик выбранных таблиц символов
		favoriteCodePages++;
	}
	else if (CallbackCallSource == CodePagesFill || !Opt.CPMenuMode)
	{
		// добавляем разделитель между стандартными и системными таблицами символов
		if (!favoriteCodePages && !normalCodePages)
			AddSeparator(MSG(MGetCodePageOther));

		// Добавляем таблицу символов в нормальные
		AddCodePage(
		    codePageName,
		    codePage,
		    GetCodePageInsertPosition(
		        codePage,
		        GetItemsCount()-normalCodePages,
		        normalCodePages
		    ),
			true,
			false,
			IsCodePageNameCustom
		);
		// Увеличиваем счётчик выбранных таблиц символов
		normalCodePages++;
	}

	return TRUE;
}

// Добавляем все необходимые таблицы символов
void AddCodePages(DWORD codePages)
{
	// Добавляем стандартные таблицы символов
	AddStandardCodePage((codePages & ::SearchAll) ? MSG(MFindFileAllCodePages) : MSG(MEditOpenAutoDetect), CP_AUTODETECT, -1, (codePages & ::SearchAll) || (codePages & ::Auto));
	AddSeparator(MSG(MGetCodePageSystem));
	AddStandardCodePage(L"DOS", WINPORT(GetOEMCP)(), -1, true);
	AddStandardCodePage(L"ANSI", WINPORT(GetACP)(), -1, true);
	AddStandardCodePage(L"KOI8", CP_KOI8R, -1, true);
	AddSeparator(MSG(MGetCodePageUnicode));
	AddStandardCodePage(L"UTF-7", CP_UTF7, -1, true);
	AddStandardCodePage(L"UTF-8", CP_UTF8, -1, true);
	AddStandardCodePage(L"UTF-16 (Little endian)", CP_UTF16LE, -1, true);
	AddStandardCodePage(L"UTF-16 (Big endian)", CP_UTF16BE, -1, true);
	if (sizeof(wchar_t)==4) {
		AddStandardCodePage(L"UTF-32 (Little endian)", CP_UTF32LE, -1, true);
		AddStandardCodePage(L"UTF-32 (Big endian)", CP_UTF32BE, -1, true);
	}
	// Получаем таблицы символов установленные в системе
	//EnumSystemCodePages((CODEPAGE_ENUMPROCW)EnumCodePagesProc, CP_INSTALLED);
}

// Обработка добавления/удаления в/из список выбранных таблиц символов
void ProcessSelected(bool select)
{
	if (Opt.CPMenuMode && select)
		return;

	UINT itemPosition = CodePages->GetSelectPos();
	UINT codePage = GetMenuItemCodePage();

	if ((select && IsPositionFavorite(itemPosition)) || (!select && IsPositionNormal(itemPosition)))
	{
		// Преобразуем номер таблицы символов в строку
		FormatString strCPName;
		strCPName<<codePage;
		// Получаем текущее состояние флага в реестре
		int selectType = 0;
		GetRegKey(FavoriteCodePagesKey, strCPName, selectType, 0);

		// Удаляем/добавляем в ресестре информацию о выбранной кодовой странице
		if (select)
			SetRegKey(FavoriteCodePagesKey, strCPName, CPST_FAVORITE | (selectType & CPST_FIND ? CPST_FIND : 0));
		else if (selectType & CPST_FIND)
			SetRegKey(FavoriteCodePagesKey, strCPName, CPST_FIND);
		else
			DeleteRegValue(FavoriteCodePagesKey, strCPName);

		// Создаём новый элемент меню
		MenuItemEx newItem;
		newItem.Clear();
		newItem.strName = CodePages->GetItemPtr()->strName;
		newItem.UserData = (char *)(UINT_PTR)codePage;
		newItem.UserDataSize = sizeof(UINT);
		// Сохраняем позицию курсора
		int position=CodePages->GetSelectPos();
		// Удаляем старый пункт меню
		CodePages->DeleteItem(CodePages->GetSelectPos());

		// Добавляем пункт меню в новое место
		if (select)
		{
			// Добавляем разделитель, если выбранных кодовых страниц ещё не было
			// и после добавления останутся нормальные кодовые страницы
			if (!favoriteCodePages && normalCodePages>1)
				AddSeparator(MSG(MGetCodePageFavorites),CodePages->GetItemCount()-normalCodePages);

			// Ищем позицию, куда добавить элемент
			int newPosition = GetCodePageInsertPosition(
			                      codePage,
			                      CodePages->GetItemCount()-normalCodePages-favoriteCodePages,
			                      favoriteCodePages
			                  );
			// Добавляем кодовою страницу в выбранные
			CodePages->AddItem(&newItem, newPosition);

			// Удаляем разделитель, если нет обыкновынных кодовых страниц
			if (normalCodePages==1)
				CodePages->DeleteItem(CodePages->GetItemCount()-1);

			// Изменяем счётчики нормальных и выбранных кодовых страниц
			favoriteCodePages++;
			normalCodePages--;
			position++;
		}
		else
		{
			// Удаляем разделитеь, если после удаления не останнется ни одной
			// выбранной таблицы символов
			if (favoriteCodePages==1 && normalCodePages>0)
				CodePages->DeleteItem(CodePages->GetItemCount()-normalCodePages-2);

			// Переносим элемент в нормальные таблицы, только если они показываются
			if (!Opt.CPMenuMode)
			{
				// Добавляем разделитель, если не было ни одной нормальной кодовой страницы
				if (!normalCodePages)
					AddSeparator(MSG(MGetCodePageOther));

				// Добавляем кодовою страницу в нормальные
				CodePages->AddItem(
				    &newItem,
				    GetCodePageInsertPosition(
				        codePage,
				        CodePages->GetItemCount()-normalCodePages,
				        normalCodePages
				    )
				);
				normalCodePages++;
			}
			// Если в режиме скрытия нормальных таблиц мы удалили последнюю выбранную таблицу, то удаляем и разделитель
			else if (favoriteCodePages==1)
				CodePages->DeleteItem(CodePages->GetItemCount()-normalCodePages-1);

			favoriteCodePages--;

			if (position==CodePages->GetItemCount()-normalCodePages-1)
				position--;
		}

		// Устанавливаем позицию в меню
		CodePages->SetSelectPos(position>=CodePages->GetItemCount() ? CodePages->GetItemCount()-1 : position, 1);

		// Показываем меню
		if (Opt.CPMenuMode)
			CodePages->SetPosition(-1, -1, 0, 0);

		CodePages->Show();
	}
}

// Заполняем меню выбора таблиц символов
void FillCodePagesVMenu(bool bShowUnicode, bool bShowUTF, bool bShowUTF7)
{
	UINT codePage = currentCodePage;

	if (CodePages->GetSelectPos()!=-1 && CodePages->GetSelectPos()<CodePages->GetItemCount()-normalCodePages)
		currentCodePage = GetMenuItemCodePage();

	// Очищаем меню
	favoriteCodePages = normalCodePages = 0;
	CodePages->DeleteItems();

	UnicodeString title = MSG(MGetCodePageTitle);
	if (Opt.CPMenuMode)
		title += L" *";
	CodePages->SetTitle(title);

	// Добавляем таблицы символов
	// BUBUG: Когда добавится поддержка UTF7 параметр bShowUTF7 нужно убрать отовсюду
	AddCodePages(::DOS | ::ANSI | ::KOI8 | (bShowUTF ? ::UTF8 : 0) | (bShowUTF7 ? ::UTF7 : 0) | (bShowUnicode ? AllUtfBiggerThan8 : 0));
	// Восстанавливаем оригинальню таблицу символов
	currentCodePage = codePage;
	// Позиционируем меню
	CodePages->SetPosition(-1, -1, 0, 0);
	// Показываем меню
	CodePages->Show();
}

// Форматируем имя таблицы символов
wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length)
{
	bool IsCodePageNameCustom;
	return FormatCodePageName(CodePage, CodePageName, Length, IsCodePageNameCustom);
}

// Форматируем имя таблицы символов
wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length, bool &IsCodePageNameCustom)
{
	if (!CodePageName || !Length)
		return CodePageName;

	// Пытаемся получить заданное пользоваталем имя таблицы символов
	FormatString strCodePage;
	strCodePage<<CodePage;
	FARString strCodePageName;
	if (GetRegKey(NamesOfCodePagesKey, strCodePage, strCodePageName, L""))
	{
		Length = Min(Length-1, strCodePageName.GetLength());
		IsCodePageNameCustom = true;
	}
	else
		IsCodePageNameCustom = false;
	if (*CodePageName)
	{
		// Под виндой на входе "XXXX (Name)", а, например, под wine просто "Name"
		wchar_t *Name = wcschr(CodePageName, L'(');
		if (Name && *(++Name))
		{
			size_t NameLength = wcslen(Name)-1;
			if (Name[NameLength] == L')')
			{
				Name[NameLength] = L'\0';
			}
		}
		if (IsCodePageNameCustom)
		{
			if (strCodePageName==Name)
			{
				DeleteRegValue(NamesOfCodePagesKey, strCodePage);
				IsCodePageNameCustom = false;
				return Name;
			}
		}
		else
			return Name;
	}
	if (IsCodePageNameCustom)
	{
		wmemcpy(CodePageName, strCodePageName, Length);
		CodePageName[Length] = L'\0';
	}
	return CodePageName;
}

// Каллбак для диалога редактирования имени кодовой страницы
LONG_PTR WINAPI EditDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg==DN_CLOSE)
	{
		if (Param1==EDITCP_OK || Param1==EDITCP_RESET)
		{
			FARString strCodePageName;
			UINT CodePage = GetMenuItemCodePage();
			FormatString strCodePage;
			strCodePage<<CodePage;
			if (Param1==EDITCP_OK)
			{
				wchar_t *CodePageName = strCodePageName.GetBuffer(SendDlgMessage(hDlg, DM_GETTEXTPTR, EDITCP_EDIT, 0)+1);
				SendDlgMessage(hDlg, DM_GETTEXTPTR, EDITCP_EDIT, (LONG_PTR)CodePageName);
				strCodePageName.ReleaseBuffer();
			}
			// Если имя кодовой страницы пустое, то считаем, что имя не задано
			if (!strCodePageName.GetLength())
				DeleteRegValue(NamesOfCodePagesKey, strCodePage);
			else
				SetRegKey(NamesOfCodePagesKey, strCodePage, strCodePageName);
			// Получаем информацию о кодовой странице
			CPINFOEX cpiex;
			if (GetCodePageInfo(CodePage, cpiex))
			{
				// Формируем имя таблиц символов
				bool IsCodePageNameCustom = false;
				wchar_t *CodePageName = FormatCodePageName(CodePage, cpiex.CodePageName, sizeof(cpiex.CodePageName)/sizeof(wchar_t), IsCodePageNameCustom);
				// Формируем строку представления
				strCodePage.Clear();
				FormatCodePageString(CodePage, CodePageName, strCodePage, IsCodePageNameCustom);
				// Обновляем имя кодовой страницы
				int Position = CodePages->GetSelectPos();
				CodePages->DeleteItem(Position);
				MenuItemEx NewItem;
				NewItem.Clear();
				NewItem.strName = strCodePage;
				NewItem.UserData = (char *)(UINT_PTR)CodePage;
				NewItem.UserDataSize = sizeof(UINT);
				CodePages->AddItem(&NewItem, Position);
				CodePages->SetSelectPos(Position, 1);
			}
		}
	}
	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

// Вызов редактора имени кодовой страницы
void EditCodePageName()
{
	UINT Position = CodePages->GetSelectPos();
	if (IsPositionStandard(Position))
		return;
	FARString CodePageName = CodePages->GetItemPtr(Position)->strName;
	size_t BoxPosition;
	if (!CodePageName.Pos(BoxPosition, BoxSymbols[BS_V1]))
		return;
	CodePageName.LShift(BoxPosition+2);
	DialogDataEx EditDialogData[]=
		{
			DI_DOUBLEBOX, 3, 1, 50, 5, 0, 0, MSG(MGetCodePageEditCodePageName),
			DI_EDIT,      5, 2, 48, 2, (DWORD_PTR)L"CodePageName", DIF_FOCUS|DIF_HISTORY, CodePageName,
			DI_TEXT,      0, 3,  0, 3, 0, DIF_SEPARATOR, L"",
			DI_BUTTON,    0, 4,  0, 3, 0, DIF_DEFAULT|DIF_CENTERGROUP, MSG(MOk),
			DI_BUTTON,    0, 4,  0, 3, 0, DIF_CENTERGROUP, MSG(MCancel),
			DI_BUTTON,    0, 4,  0, 3, 0, DIF_CENTERGROUP, MSG(MGetCodePageResetCodePageName)
		};
	MakeDialogItemsEx(EditDialogData, EditDialog);
	Dialog Dlg(EditDialog, ARRAYSIZE(EditDialog), EditDialogProc);
	Dlg.SetPosition(-1, -1, 54, 7);
	Dlg.SetHelp(L"EditCodePageNameDlg");
	Dlg.Process();
}

UINT SelectCodePage(UINT nCurrent, bool bShowUnicode, bool bShowUTF, bool bShowUTF7)
{
	CallbackCallSource = CodePageSelect;
	currentCodePage = nCurrent;
	// Создаём меню
	CodePages = new VMenu(L"", nullptr, 0, ScrY-4);
	CodePages->SetBottomTitle(MSG(!Opt.CPMenuMode?MGetCodePageBottomTitle:MGetCodePageBottomShortTitle));
	CodePages->SetFlags(VMENU_WRAPMODE|VMENU_AUTOHIGHLIGHT);
	CodePages->SetHelp(L"CodePagesMenu");
	// Добавляем таблицы символов
	FillCodePagesVMenu(bShowUnicode, bShowUTF, bShowUTF7);
	// Показываем меню
	CodePages->Show();

	// Цикл обработки сообщений меню
	while (!CodePages->Done())
	{
		switch (CodePages->ReadInput())
		{
			// Обработка скрытия/показа системных таблиц символов
			case KEY_CTRLH:
				Opt.CPMenuMode = !Opt.CPMenuMode;
				CodePages->SetBottomTitle(MSG(!Opt.CPMenuMode?MGetCodePageBottomTitle:MGetCodePageBottomShortTitle));
				FillCodePagesVMenu(bShowUnicode, bShowUTF, bShowUTF7);
				break;
			// Обработка удаления таблицы символов из списка выбранных
			case KEY_DEL:
			case KEY_NUMDEL:
				ProcessSelected(false);
				break;
			// Обработка добавления таблицы символов в список выбранных
			case KEY_INS:
			case KEY_NUMPAD0:
				ProcessSelected(true);
				break;
			// Редактируем имя таблицы символов
			case KEY_F4:
				EditCodePageName();
				break;
			default:
				CodePages->ProcessInput();
				break;
		}
	}

	// Получаем выбранную таблицу символов
	UINT codePage = CodePages->Modal::GetExitCode() >= 0 ? GetMenuItemCodePage() : (UINT)-1;
	delete CodePages;
	CodePages = nullptr;
	return codePage;
}

// Заполняем список таблицами символов
UINT FillCodePagesList(HANDLE dialogHandle, UINT controlId, UINT codePage, bool allowAuto, bool allowAll)
{
	CallbackCallSource = CodePagesFill;
	// Устанавливаем переменные для доступа из каллбака
	dialog = dialogHandle;
	control = controlId;
	currentCodePage = codePage;
	favoriteCodePages = normalCodePages = 0;
	selectedCodePages = !allowAuto && allowAll;
	// Добавляем стндартные элементы в список
	AddCodePages((allowAuto ? ::Auto : 0) | (allowAll ? ::SearchAll : 0) | ::AllStandard);

	if (CallbackCallSource == CodePagesFill)
	{
		// Если надо выбираем элемент
		FarListInfo info;
		SendDlgMessage(dialogHandle, DM_LISTINFO, control, (LONG_PTR)&info);

		for (int i=0; i<info.ItemsNumber; i++)
		{
			if (GetListItemCodePage(i)==codePage)
			{
				FarListGetItem Item={i};
				SendDlgMessage(dialog, DM_LISTGETITEM, control, reinterpret_cast<LONG_PTR>(&Item));
				SendDlgMessage(dialog, DM_SETTEXTPTR, control, reinterpret_cast<LONG_PTR>(Item.Item.Text));
				FarListPos Pos={i,-1};
				SendDlgMessage(dialog, DM_LISTSETCURPOS, control, reinterpret_cast<LONG_PTR>(&Pos));
				break;
			}
		}
	}

	// Возвращаем число любимых таблиц символов
	return favoriteCodePages;
}

bool IsCodePageSupported(UINT CodePage)
{
	// Для стандартных кодовых страниц ничего проверять не надо
	// BUGBUG: мы не везде поддержиаем все стандартные кодовые страницы. Это не проверяется
	if (CodePage == CP_AUTODETECT || IsStandardCodePage(CodePage))
		return true;

	// Проходим по всем кодовым страницам системы и проверяем поддерживаем мы или нет её
	CallbackCallSource = CodePageCheck;
	currentCodePage = CodePage;
	CodePageSupported = false;
	WINPORT(EnumSystemCodePages)((CODEPAGE_ENUMPROCW)EnumCodePagesProc, 0);//CP_INSTALLED
	return CodePageSupported;
}
