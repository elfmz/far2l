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
#include "keys.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "ConfigRW.hpp"

#include "stdio.h"
#include "stdarg.h"

// Ключ где хранятся имена кодовых страниц
static const char *NamesOfCodePagesKey = "CodePages/Names";

const char *FavoriteCodePagesKey = "CodePages/Favorites";

// Стандартные кодовые страницы
enum StandardCodePages
{
	SearchAll = 1,
	Auto      = 2,
	KOI8      = 4,
	DOS       = 8,
	ANSI      = 16,
	UTF7      = 32,
	UTF8      = 64,
	UTF16LE   = 128,
	UTF16BE   = 256,
#if (__WCHAR_MAX__ > 0xffff)
	UTF32LE           = 512,
	UTF32BE           = 1024,
	AllUtfBiggerThan8 = UTF16BE | UTF16LE | UTF32BE | UTF32LE,
	AllStandard       = DOS | ANSI | KOI8 | UTF7 | UTF8 | UTF16BE | UTF16LE | UTF32BE | UTF32LE
#else
	AllUtfBiggerThan8 = UTF16BE | UTF16LE,
	AllStandard       = DOS | ANSI | KOI8 | UTF7 | UTF8 | UTF16BE | UTF16LE
#endif
};

// Источник вызова коллбака прохода по кодовым страницам
enum CodePagesCallbackCallSource
{
	CodePageSelect,
	CodePagesFill,
	CodePageCheck
};

// Номера контролов диалога редактирования имени кодовой страницы
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
// Идентификатор диалога
static UINT control;
// Меню
static VMenu *CodePages = nullptr;
// Текущая таблица символов
static UINT currentCodePage;
// Количество выбранных и обыкновенных таблиц символов
static int favoriteCodePages, normalCodePages;
// Признак необходимости отображать таблицы символов для поиска
static bool selectedCodePages;
// Источник вызова коллбака для функции EnumSystemCodePages
static CodePagesCallbackCallSource CallbackCallSource;
// Признак того, что кодовая страница поддерживается
// static bool CodePageSupported;

static std::unique_ptr<ConfigReader> s_cfg_reader;

static wchar_t *
FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length, bool &IsCodePageNameCustom);

#ifdef CP_DBG
static const char *levelNames[] = {"QUIET", "ERROR", "WARN", "TRACE", "INFO"};

static FILE *cplog = 0;

static void file_logger(int level, const char *cname, const char *msg, va_list v)
{

	int idx = 0;

	while (cplog == 0 && idx < 10) {
		char log_name[30];
#ifdef __unix__
		snprintf(log_name, sizeof(log_name), "/tmp/cp-trace%d.log", idx);
#else
		snprintf(log_name, sizeof(log_name), "c:/cp-trace%d.log", idx);
#endif
		cplog = fopen(log_name, "ab+");
		idx++;
	}

	fprintf(cplog, "[%s][%s] ", levelNames[level], cname);

	vfprintf(cplog, msg, v);

	// fprintf(cplog, "\n");

	fflush(cplog);
}
#endif	// CP_DBG

void cp_logger(int level, const char *cname, const char *msg, ...)
{
#ifdef CP_DBG
	va_list v;
	va_start(v, msg);
	file_logger(level, cname, msg, v);
	va_end(v);
#endif	// CP_DBG
	return;
}

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
	return position <= (UINT)CodePages->GetItemCount() - favoriteCodePages - (favoriteCodePages ? 1 : 0)
			- normalCodePages - (normalCodePages ? 1 : 0);
}

// Проверяем попадает или нет позиция в диапазон любимых кодовых страниц (правильность работы для разделителей не гарантируется)
inline bool IsPositionFavorite(UINT position)
{
	return position >= (UINT)CodePages->GetItemCount() - normalCodePages;
}

// Проверяем попадает или нет позиция в диапазон обыкновенных кодовых страниц (правильность работы для разделителей не гарантируется)
inline bool IsPositionNormal(UINT position)
{
	UINT ItemCount = CodePages->GetItemCount();
	return position >= ItemCount - normalCodePages - favoriteCodePages - (normalCodePages ? 1 : 0)
			&& position < ItemCount - normalCodePages;
}

// Формируем строку для визуального представления таблицы символов
static void FormatCodePageString(UINT CodePage, const wchar_t *CodePageName, FormatString &CodePageNameString,
		bool IsCodePageNameCustom)
{
	if (CodePage != CP_AUTODETECT) {
		CodePageNameString << fmt::Expand(5) << CodePage << BoxSymbols[BS_V1]
						<< (!IsCodePageNameCustom || CallbackCallSource == CodePagesFill ? L' ' : L'*');
	}
	CodePageNameString << CodePageName;
}

static int GetCodePageSelectType(UINT codePage)		// selectedCodePages, (selectType & CPST_FIND) != 0
{
	if (codePage == CP_AUTODETECT)
		return 0;
	// Получаем признак выбранности таблицы символов
	s_cfg_reader->SelectSection(FavoriteCodePagesKey);
	return s_cfg_reader->GetInt(ToDec(codePage), 0);
}

// Добавляем таблицу символов
static void AddCodePage(const wchar_t *codePageName, UINT codePage, int position, bool enabled, bool checked,
		bool IsCodePageNameCustom)
{
	if (CallbackCallSource == CodePagesFill) {
		// Вычисляем позицию вставляемого элемента
		if (position == -1) {
			FarListInfo info;
			SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
			position = info.ItemsNumber;
		}

		// Вставляем элемент
		FarListInsert item = {position, {}};

		FormatString name;
		FormatCodePageString(codePage, codePageName, name, IsCodePageNameCustom);
		item.Item.Text = name;

		if (selectedCodePages && checked) {
			item.Item.Flags|= MIF_CHECKED;
		}

		if (!enabled) {
			item.Item.Flags|= MIF_GRAYED;
		}

		SendDlgMessage(dialog, DM_LISTINSERT, control, (LONG_PTR)&item);
		// Устанавливаем данные для элемента
		FarListItemData data;
		data.Index = position;
		data.Data = (void *)(DWORD_PTR)codePage;
		data.DataSize = sizeof(UINT);
		SendDlgMessage(dialog, DM_LISTSETDATA, control, (LONG_PTR)&data);
	} else {
		// Создаём новый элемент меню
		MenuItemEx item;
		item.Clear();

		if (!enabled)
			item.Flags|= MIF_GRAYED;

		FormatString name;
		FormatCodePageString(codePage, codePageName, name, IsCodePageNameCustom);
		item.strName = name;

		item.UserData = (char *)(UINT_PTR)codePage;
		item.UserDataSize = sizeof(UINT);

		// Добавляем новый элемент в меню
		if (position >= 0)
			CodePages->AddItem(&item, position);
		else
			CodePages->AddItem(&item);

		// Если надо позиционируем курсор на добавленный элемент
		if (currentCodePage == codePage) {
			if ((CodePages->GetSelectPos() == -1 || GetMenuItemCodePage() != codePage)) {
				CodePages->SetSelectPos(position >= 0 ? position : CodePages->GetItemCount() - 1, 1);
			}
		}
	}
}

// Добавляем стандартную таблицу символов
static void
AddStandardCodePage(const wchar_t *codePageName, UINT codePage, int position = -1, bool enabled = true)
{
	bool checked = selectedCodePages && (GetCodePageSelectType(codePage) & CPST_FIND) != 0;
	AddCodePage(codePageName, codePage, position, enabled, checked, false);
}

// Добавляем разделитель
static void AddSeparator(LPCWSTR Label = nullptr, int position = -1)
{
	if (CallbackCallSource == CodePagesFill) {
		if (position == -1) {
			FarListInfo info;
			SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
			position = info.ItemsNumber;
		}

		FarListInsert item = {position, {}};
		item.Item.Text = Label;
		item.Item.Flags = LIF_SEPARATOR;
		SendDlgMessage(dialog, DM_LISTINSERT, control, (LONG_PTR)&item);
	} else {
		MenuItemEx item;
		item.Clear();
		item.strName = Label;
		item.Flags = MIF_SEPARATOR;

		if (position >= 0)
			CodePages->AddItem(&item, position);
		else
			CodePages->AddItem(&item);
	}
}

// Получаем количество элементов в списке
static int GetItemsCount()
{
	if (CallbackCallSource == CodePageSelect) {
		return CodePages->GetItemCount();
	} else {
		FarListInfo info;
		SendDlgMessage(dialog, DM_LISTINFO, control, (LONG_PTR)&info);
		return info.ItemsNumber;
	}
}

// Получаем позицию для вставки таблицы с учётом сортировки по номеру кодовой страницы
static int GetCodePageInsertPosition(UINT codePage, int start, int length)
{
	for (int position = start; position < start + length; position++) {
		UINT itemCodePage;

		if (CallbackCallSource == CodePageSelect)
			itemCodePage = GetMenuItemCodePage(position);
		else
			itemCodePage = GetListItemCodePage(position);

		if (itemCodePage >= codePage)
			return position;
	}

	return start + length;
}

// Получаем информацию о кодовой странице
static bool GetCodePageInfo(UINT CodePage, CPINFOEX &CodePageInfoEx)
{
	if (!WINPORT(GetCPInfoEx)(CodePage, 0, &CodePageInfoEx)) {
		// GetCPInfoEx возвращает ошибку для кодовых страниц без имени (например 1125), которые
		// сами по себе работают. Так что, прежде чем пропускать кодовую страницу из-за ошибки,
		// пробуем получить для неё информацию через GetCPInfo
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
static BOOL __stdcall EnumCodePagesProc(const wchar_t *lpwszCodePage)
{
	UINT codePage = _wtoi(lpwszCodePage);

	// Получаем информацию о кодовой странице. Если информацию по какой-либо причине получить не удалось, то
	// для списков продолжаем енумерацию, а для процедуры же проверки поддерживаемости кодовой страницы выходим
	CPINFOEX cpiex;
	if (!GetCodePageInfo(codePage, cpiex)) {
		return CallbackCallSource == CodePageCheck ? FALSE : TRUE;
	}

	if (IsStandardCodePage(codePage)) {
		return TRUE;	// continue
	}

	// Формируем имя таблиц символов
	bool IsCodePageNameCustom = false;
	wchar_t *codePageName = FormatCodePageName(_wtoi(lpwszCodePage), cpiex.CodePageName,
			sizeof(cpiex.CodePageName) / sizeof(wchar_t), IsCodePageNameCustom);

	int selectType = GetCodePageSelectType(codePage);
	bool checked = selectedCodePages && (selectType & CPST_FIND) != 0;
	// Добавляем таблицу символов либо в нормальные, либо в выбранные таблицы символов
	if (selectType & CPST_FAVORITE) {
		// Если надо добавляем разделитель между выбранными и нормальными таблицами символов
		if (!favoriteCodePages)
			AddSeparator(Msg::GetCodePageFavorites,
					GetItemsCount() - normalCodePages - (normalCodePages ? 1 : 0));

		// Добавляем таблицу символов в выбранные
		AddCodePage(codePageName, codePage,
				GetCodePageInsertPosition(codePage,
						GetItemsCount() - normalCodePages - favoriteCodePages - (normalCodePages ? 1 : 0),
						favoriteCodePages),
				true, checked, IsCodePageNameCustom);
		// Увеличиваем счётчик выбранных таблиц символов
		favoriteCodePages++;
	} else if (CallbackCallSource == CodePagesFill || !Opt.CPMenuMode) {
		// добавляем разделитель между стандартными и системными таблицами символов
		if (!favoriteCodePages && !normalCodePages)
			AddSeparator(Msg::GetCodePageOther);

		// Добавляем таблицу символов в нормальные
		AddCodePage(codePageName, codePage,
				GetCodePageInsertPosition(codePage, GetItemsCount() - normalCodePages, normalCodePages), true,
				checked, IsCodePageNameCustom);
		// Увеличиваем счётчик выбранных таблиц символов
		normalCodePages++;
	}

	return TRUE;
}

// Добавляем все необходимые таблицы символов
static void AddCodePages(DWORD codePages)
{
	// Добавляем стандартные таблицы символов
	AddStandardCodePage((codePages & ::SearchAll) ? Msg::FindFileAllCodePages : Msg::EditOpenAutoDetect,
			CP_AUTODETECT, -1, (codePages & ::SearchAll) || (codePages & ::Auto));
	AddSeparator(Msg::GetCodePageUnicode);

	AddStandardCodePage(L"UTF-8", CP_UTF8, -1, true);
	AddStandardCodePage(L"UTF-7", CP_UTF7, -1, true);
	AddStandardCodePage(L"UTF-16 (Little endian)", CP_UTF16LE, -1, true);
	AddStandardCodePage(L"UTF-16 (Big endian)", CP_UTF16BE, -1, true);
	if (sizeof(wchar_t) == 4) {
		AddStandardCodePage(L"UTF-32 (Little endian)", CP_UTF32LE, -1, true);
		AddStandardCodePage(L"UTF-32 (Big endian)", CP_UTF32BE, -1, true);
	}
	AddSeparator(Msg::GetCodePageSystem);
	AddStandardCodePage(L"ANSI", WINPORT(GetACP)(), -1, true);
	AddStandardCodePage(L"KOI8", CP_KOI8R, -1, true);
	AddStandardCodePage(L"OEM", WINPORT(GetOEMCP)(), -1, true);
	// Получаем таблицы символов установленные в системе
	WINPORT(EnumSystemCodePages)((CODEPAGE_ENUMPROCW)EnumCodePagesProc, 0);		// CP_INSTALLED
}

// Обработка добавления/удаления в/из список выбранных таблиц символов
static void ProcessSelected(bool select)
{
	if (Opt.CPMenuMode && select)
		return;

	UINT itemPosition = CodePages->GetSelectPos();
	UINT codePage = GetMenuItemCodePage();

	if ((select && IsPositionFavorite(itemPosition)) || (!select && IsPositionNormal(itemPosition))) {
		// Преобразуем номер таблицы символов в строку
		// FormatString strCPName;
		// strCPName<<codePage;
		// Получаем текущее состояние флага в реестре
		std::string strCPName = ToDec(codePage);
		s_cfg_reader->SelectSection(FavoriteCodePagesKey);
		int selectType = s_cfg_reader->GetInt(strCPName, 0);

		// Удаляем/добавляем в реестре информацию о выбранной кодовой странице
		{
			ConfigWriter cfg_writer;
			cfg_writer.SelectSection(FavoriteCodePagesKey);
			if (select)
				cfg_writer.SetInt(strCPName, CPST_FAVORITE | (selectType & CPST_FIND));
			else if (selectType & CPST_FIND)
				cfg_writer.SetInt(strCPName, CPST_FIND);
			else
				cfg_writer.RemoveKey(strCPName);
		}

		ConfigReaderScope::Update(s_cfg_reader);

		// Создаём новый элемент меню
		MenuItemEx newItem;
		newItem.Clear();
		newItem.strName = CodePages->GetItemPtr()->strName;
		newItem.UserData = (char *)(UINT_PTR)codePage;
		newItem.UserDataSize = sizeof(UINT);
		// Сохраняем позицию курсора
		int position = CodePages->GetSelectPos();
		// Удаляем старый пункт меню
		CodePages->DeleteItem(CodePages->GetSelectPos());

		// Добавляем пункт меню в новое место
		if (select) {
			// Добавляем разделитель, если выбранных кодовых страниц ещё не было
			// и после добавления останутся нормальные кодовые страницы
			if (!favoriteCodePages && normalCodePages > 1)
				AddSeparator(Msg::GetCodePageFavorites, CodePages->GetItemCount() - normalCodePages);

			// Ищем позицию, куда добавить элемент
			int newPosition = GetCodePageInsertPosition(codePage,
					CodePages->GetItemCount() - normalCodePages - favoriteCodePages, favoriteCodePages);
			// Добавляем кодовую страницу в выбранные
			CodePages->AddItem(&newItem, newPosition);

			// Удаляем разделитель, если нет обыкновенных кодовых страниц
			if (normalCodePages == 1)
				CodePages->DeleteItem(CodePages->GetItemCount() - 1);

			// Изменяем счётчики нормальных и выбранных кодовых страниц
			favoriteCodePages++;
			normalCodePages--;
			position++;
		} else {
			// Удаляем разделитеь, если после удаления не останнется ни одной
			// выбранной таблицы символов
			if (favoriteCodePages == 1 && normalCodePages > 0)
				CodePages->DeleteItem(CodePages->GetItemCount() - normalCodePages - 2);

			// Переносим элемент в нормальные таблицы, только если они показываются
			if (!Opt.CPMenuMode) {
				// Добавляем разделитель, если не было ни одной нормальной кодовой страницы
				if (!normalCodePages)
					AddSeparator(Msg::GetCodePageOther);

				// Добавляем кодовою страницу в нормальные
				CodePages->AddItem(&newItem,
						GetCodePageInsertPosition(codePage, CodePages->GetItemCount() - normalCodePages,
								normalCodePages));
				normalCodePages++;
			}
			// Если в режиме скрытия нормальных таблиц мы удалили последнюю выбранную таблицу, то удаляем и разделитель
			else if (favoriteCodePages == 1)
				CodePages->DeleteItem(CodePages->GetItemCount() - normalCodePages - 1);

			favoriteCodePages--;

			if (position == CodePages->GetItemCount() - normalCodePages - 1)
				position--;
		}

		// Устанавливаем позицию в меню
		CodePages->SetSelectPos(position >= CodePages->GetItemCount()
						? CodePages->GetItemCount() - 1
						: position,
				1);

		// Показываем меню
		if (Opt.CPMenuMode)
			CodePages->SetPosition(-1, -1, 0, 0);

		CodePages->Show();
	}
}

// Заполняем меню выбора таблиц символов
static void FillCodePagesVMenu(bool bShowUnicode, bool bShowUTF, bool bShowUTF7, bool bShowAuto)
{
	UINT codePage = currentCodePage;

	if (CodePages->GetSelectPos() != -1
			&& CodePages->GetSelectPos() < CodePages->GetItemCount() - normalCodePages)
		currentCodePage = GetMenuItemCodePage();

	// Очищаем меню
	favoriteCodePages = normalCodePages = 0;
	CodePages->DeleteItems();

	FARString title(Msg::GetCodePageTitle);
	if (Opt.CPMenuMode)
		title+= L" *";
	CodePages->SetTitle(title);

	// Добавляем таблицы символов
	// BUBUG: Когда добавится поддержка UTF7 параметр bShowUTF7 нужно убрать отовсюду
	AddCodePages(::DOS | ::ANSI | ::KOI8 | (bShowUTF ? ::UTF8 : 0) | (bShowUTF7 ? ::UTF7 : 0)
			| (bShowUnicode ? AllUtfBiggerThan8 : 0) | (bShowAuto ? ::Auto : 0));
	// Восстанавливаем оригинальную таблицу символов
	currentCodePage = codePage;
	// Позиционируем меню
	CodePages->SetPosition(-1, -1, 0, 0);
	// Показываем меню
	CodePages->Show();
}

// Форматируем имя таблицы символов
static wchar_t *FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length)
{
	bool IsCodePageNameCustom;
	return FormatCodePageName(CodePage, CodePageName, Length, IsCodePageNameCustom);
}

// Форматируем имя таблицы символов
static wchar_t *
FormatCodePageName(UINT CodePage, wchar_t *CodePageName, size_t Length, bool &IsCodePageNameCustom)
{
	if (!CodePageName || !Length)
		return CodePageName;

	// Пытаемся получить заданное пользоваталем имя таблицы символов
	// FormatString strCodePage;
	// strCodePage<<CodePage;
	s_cfg_reader->SelectSection(NamesOfCodePagesKey);
	FARString strCodePageName;
	const std::string &strCodePage = ToDec(CodePage);
	if (s_cfg_reader->GetString(strCodePageName, strCodePage, L"")) {
		Length = Min(Length - 1, strCodePageName.GetLength());
		IsCodePageNameCustom = true;
	} else
		IsCodePageNameCustom = false;

	if (*CodePageName) {
		// Под виндой на входе "XXXX (Name)", а, например, под wine просто "Name"
		wchar_t *Name = wcschr(CodePageName, L'(');
		if (Name && *(++Name)) {
			size_t NameLength = wcslen(Name) - 1;
			if (Name[NameLength] == L')') {
				Name[NameLength] = L'\0';
			}
		}
		if (IsCodePageNameCustom) {
			if (Name && strCodePageName == Name) {
				ConfigWriter(NamesOfCodePagesKey).RemoveKey(strCodePage);
				IsCodePageNameCustom = false;
				return Name ? Name : CodePageName;
			}
		} else
			return Name ? Name : CodePageName;
	}
	if (IsCodePageNameCustom) {
		wmemcpy(CodePageName, strCodePageName, Length);
		CodePageName[Length] = L'\0';
	}
	return CodePageName;
}

// Коллбак для диалога редактирования имени кодовой страницы
static LONG_PTR WINAPI EditDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_CLOSE) {
		if (Param1 == EDITCP_OK || Param1 == EDITCP_RESET) {
			FARString strCodePageName;
			UINT CodePage = GetMenuItemCodePage();
			// FormatString strCodePage;
			// strCodePage<<CodePage;
			const std::string &strCodePage = ToDec(CodePage);
			if (Param1 == EDITCP_OK) {
				wchar_t *CodePageName =
						strCodePageName.GetBuffer(SendDlgMessage(hDlg, DM_GETTEXTPTR, EDITCP_EDIT, 0) + 1);
				SendDlgMessage(hDlg, DM_GETTEXTPTR, EDITCP_EDIT, (LONG_PTR)CodePageName);
				strCodePageName.ReleaseBuffer();
			}
			{
				ConfigWriter cfg_writer;
				cfg_writer.SelectSection(NamesOfCodePagesKey);
				// Если имя кодовой страницы пустое, то считаем, что имя не задано
				if (!strCodePageName.GetLength())
					cfg_writer.RemoveKey(strCodePage);
				else
					cfg_writer.SetString(strCodePage, strCodePageName.CPtr());
			}
			ConfigReaderScope::Update(s_cfg_reader);
			// Получаем информацию о кодовой странице
			CPINFOEX cpiex;
			if (GetCodePageInfo(CodePage, cpiex)) {
				// Формируем имя таблиц символов
				bool IsCodePageNameCustom = false;
				wchar_t *CodePageName = FormatCodePageName(CodePage, cpiex.CodePageName,
						sizeof(cpiex.CodePageName) / sizeof(wchar_t), IsCodePageNameCustom);
				// Формируем строку представления
				FormatString strCodePageF;
				FormatCodePageString(CodePage, CodePageName, strCodePageF, IsCodePageNameCustom);
				// Обновляем имя кодовой страницы
				int Position = CodePages->GetSelectPos();
				CodePages->DeleteItem(Position);
				MenuItemEx NewItem;
				NewItem.Clear();
				NewItem.strName = strCodePageF;
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
static void EditCodePageName()
{
	UINT Position = CodePages->GetSelectPos();
	if (IsPositionStandard(Position))
		return;
	FARString CodePageName = CodePages->GetItemPtr(Position)->strName;
	size_t BoxPosition;
	if (!CodePageName.Pos(BoxPosition, BoxSymbols[BS_V1]))
		return;
	CodePageName.LShift(BoxPosition + 2);
	DialogDataEx EditDialogData[] = {
		{DI_DOUBLEBOX, 3, 1, 50, 5, {}, 0, Msg::GetCodePageEditCodePageName },
		{DI_EDIT,      5, 2, 48, 2, {(DWORD_PTR)L"CodePageName"}, DIF_FOCUS | DIF_HISTORY, CodePageName},
		{DI_TEXT,      0, 3, 0,  3, {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0, 4, 0,  3, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::Ok},
		{DI_BUTTON,    0, 4, 0,  3, {}, DIF_CENTERGROUP, Msg::Cancel},
		{DI_BUTTON,    0, 4, 0,  3, {}, DIF_CENTERGROUP, Msg::GetCodePageResetCodePageName}
	};
	MakeDialogItemsEx(EditDialogData, EditDialog);
	Dialog Dlg(EditDialog, ARRAYSIZE(EditDialog), EditDialogProc);
	Dlg.SetPosition(-1, -1, 54, 7);
	Dlg.SetHelp(L"EditCodePageNameDlg");
	Dlg.Process();
}

UINT SelectCodePage(UINT nCurrent, bool bShowUnicode, bool bShowUTF, bool bShowUTF7, bool bShowAuto)
{
	ConfigReaderScope grs(s_cfg_reader);
	CallbackCallSource = CodePageSelect;
	currentCodePage = nCurrent;
	// Создаём меню
	CodePages = new VMenu(L"", nullptr, 0, ScrY - 4);
	CodePages->SetBottomTitle(
			!Opt.CPMenuMode ? Msg::GetCodePageBottomTitle : Msg::GetCodePageBottomShortTitle);
	CodePages->SetFlags(VMENU_WRAPMODE | VMENU_AUTOHIGHLIGHT);
	CodePages->SetHelp(L"CodePagesMenu");
	// Добавляем таблицы символов
	FillCodePagesVMenu(bShowUnicode, bShowUTF, bShowUTF7, bShowAuto);
	// Показываем меню
	CodePages->Show();

	// Цикл обработки сообщений меню
	while (!CodePages->Done()) {
		switch (CodePages->ReadInput()) {
			// Обработка скрытия/показа системных таблиц символов
			case KEY_CTRLH:
				Opt.CPMenuMode = !Opt.CPMenuMode;
				CodePages->SetBottomTitle(
						!Opt.CPMenuMode ? Msg::GetCodePageBottomTitle : Msg::GetCodePageBottomShortTitle);
				FillCodePagesVMenu(bShowUnicode, bShowUTF, bShowUTF7, bShowAuto);
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
	ConfigReaderScope grs(s_cfg_reader);
	CallbackCallSource = CodePagesFill;
	// Устанавливаем переменные для доступа из коллбака
	dialog = dialogHandle;
	control = controlId;
	currentCodePage = codePage;
	favoriteCodePages = normalCodePages = 0;
	selectedCodePages = !allowAuto && allowAll;
	// Добавляем стндартные элементы в список
	AddCodePages((allowAuto ? ::Auto : 0) | (allowAll ? ::SearchAll : 0) | ::AllStandard);

	if (CallbackCallSource == CodePagesFill) {
		// Если надо выбираем элемент
		FarListInfo info;
		SendDlgMessage(dialogHandle, DM_LISTINFO, control, (LONG_PTR)&info);

		for (int i = 0; i < info.ItemsNumber; i++) {
			if (GetListItemCodePage(i) == codePage) {
				FarListGetItem Item = {i, {}};
				SendDlgMessage(dialog, DM_LISTGETITEM, control, reinterpret_cast<LONG_PTR>(&Item));
				SendDlgMessage(dialog, DM_SETTEXTPTR, control, reinterpret_cast<LONG_PTR>(Item.Item.Text));
				FarListPos Pos = {i, -1};
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

	CPINFOEX cpiex;

	return GetCodePageInfo(CodePage, cpiex);
}

void ShortReadableCodepageName(UINT cp, FARString &str_dest)
{
	if (cp == CP_UTF8)
		str_dest = L"UTF-8";
	/* in Linux ANSI & OEM is less informative than numeric
	else if (cp == WINPORT(GetACP)())
		str_dest = L"ANSI";
	else if (cp == WINPORT(GetOEMCP)())
		str_dest = L"OEM";*/
	else if (cp == CP_UTF7)
		str_dest = L"UTF-7";
	else if (cp == CP_UTF16LE)
		str_dest = L"U16LE";
	else if (cp == CP_UTF16BE)
		str_dest = L"U16BE";
	else if (cp == CP_UTF32LE)
		str_dest = L"U32LE";
	else if (cp == CP_UTF32BE)
		str_dest = L"U32BE";
	else if (cp == CP_KOI8R)
		str_dest = L"KOI8R";
	else
		str_dest.Format(L"%u",cp);
}
