/*
language.cpp

Работа с lng файлами
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

#include <limits>
#include "language.hpp"
#include "lang.hpp"
#include "scantree.hpp"
#include "vmenu.hpp"
#include "manager.hpp"
#include "message.hpp"
#include "config.hpp"
#include "strmix.hpp"
#include "pathmix.hpp"
#include "filestr.hpp"
#include "interf.hpp"

static const wchar_t LangFileMask[] = L"*.lng";

Language Lang;

static FILE *TryOpenLangFile(const wchar_t *Path, const wchar_t *Mask, const wchar_t *Language,
		FARString &strFileName, UINT &nCodePage, BOOL StrongLang, FARString *pstrLangName)
{
	strFileName.Clear();
	FILE *LangFile = nullptr;
	FARString strEngFileName;
	FARString strLangName;
	FAR_FIND_DATA_EX FindData;
	ScanTree ScTree(FALSE, FALSE);
	ScTree.SetFindPath(Path, Mask);

	while (ScTree.GetNextName(&FindData, strFileName)) {
		if (!Language)
			break;

		if (!(LangFile = fopen(strFileName.GetMB().c_str(), "rb"))) {
			fprintf(stderr, "OpenLangFile opfailed: %ls\n", strFileName.CPtr());

			strFileName.Clear();
		} else {
			DetectFileMagic(LangFile, nCodePage);

			if (GetLangParam(LangFile, L"Language", &strLangName, nullptr, nCodePage)
					&& !StrCmpI(strLangName, Language))
				break;

			fclose(LangFile);
			LangFile = nullptr;

			if (StrongLang) {
				strFileName.Clear();
				strEngFileName.Clear();
				break;
			}

			if (!StrCmpI(strLangName, L"English"))
				strEngFileName = std::move(strFileName);
		}
	}

	if (!LangFile) {
		if (!strEngFileName.IsEmpty()) {
			strFileName = std::move(strEngFileName);
			strLangName = L"English";
		}

		if (!strFileName.IsEmpty()) {
			LangFile = fopen(Wide2MB(strFileName).c_str(), FOPEN_READ);

			if (pstrLangName)
				*pstrLangName = std::move(strLangName);
		}
	}

	return (LangFile);
}

FILE *OpenLangFile(FARString strPath, const wchar_t *Mask, const wchar_t *Language, FARString &strFileName,
		UINT &nCodePage, BOOL StrongLang, FARString *pstrLangName)
{
	FILE *out = TryOpenLangFile(strPath, Mask, Language, strFileName, nCodePage, StrongLang, pstrLangName);
	if (!out) {
		if (TranslateFarString<TranslateInstallPath_Lib2Share>(strPath)) {
			out = TryOpenLangFile(strPath, Mask, Language, strFileName, nCodePage, StrongLang, pstrLangName);
		}
	}
	return out;
}

int GetLangParam(FILE *SrcFile, const wchar_t *ParamName, FARString *strParam1, FARString *strParam2,
		UINT nCodePage)
{
	wchar_t ReadStr[1024];
	FARString strFullParamName = L".";
	strFullParamName+= ParamName;
	int Length = (int)strFullParamName.GetLength();
	/*
		$ 29.11.2001 DJ
		не поганим позицию в файле; дальше @Contents не читаем
	*/
	BOOL Found = FALSE;
	long OldPos = ftell(SrcFile);

	StringReader SR;

	while (SR.Read(SrcFile, ReadStr, 1024, nCodePage)) {
		if (!StrCmpNI(ReadStr, strFullParamName, Length)) {
			wchar_t *Ptr = wcschr(ReadStr, L'=');

			if (Ptr) {
				*strParam1 = Ptr + 1;

				if (strParam2)
					strParam2->Clear();

				size_t pos;

				if (strParam1->Pos(pos, L',')) {
					if (strParam2) {
						*strParam2 = *strParam1;
						strParam2->LShift(pos + 1);
						RemoveTrailingSpaces(*strParam2);
					}

					strParam1->Truncate(pos);
				}

				RemoveTrailingSpaces(*strParam1);
				Found = TRUE;
				break;
			}
		} else if (!StrCmpNI(ReadStr, L"@Contents", 9))
			break;
	}

	fseek(SrcFile, OldPos, SEEK_SET);
	return (Found);
}

int Select(int HelpLanguage, VMenu **MenuPtr)
{
	const wchar_t *Title, *Mask;
	FARString *strDest;

	if (HelpLanguage) {
		Title = Msg::HelpLangTitle;
		Mask = HelpFileMask;
		strDest = &Opt.strHelpLanguage;
	} else {
		Title = Msg::LangTitle;
		Mask = LangFileMask;
		strDest = &Opt.strLanguage;
	}

	struct Less {
		bool operator()(const FARString& A, const FARString& B) const { return StrCmpI(A,B) < 0; }
	};
	std::map<FARString, FARString, Less> Map;

	VMenu *LangMenu = new VMenu(Title, nullptr, 0, ScrY - 4);
	*MenuPtr = LangMenu;
	LangMenu->SetFlags(VMENU_WRAPMODE);
	LangMenu->SetPosition(ScrX / 2 - 8 + 5 * HelpLanguage, ScrY / 2 - 4 + 2 * HelpLanguage, 0, 0);
	FARString strFullName;
	FAR_FIND_DATA_EX FindData;
	ScanTree ScTree(FALSE, FALSE);
	ScTree.SetFindPath(g_strFarPath, Mask);

	while (ScTree.GetNextName(&FindData, strFullName)) {
		FILE *LangFile = fopen(Wide2MB(strFullName).c_str(), FOPEN_READ);

		if (!LangFile)
			continue;

		UINT nCodePage = CP_UTF8;
		DetectFileMagic(LangFile, nCodePage);
		FARString strLangName, strLangDescr;

		if (GetLangParam(LangFile, L"Language", &strLangName, &strLangDescr, nCodePage)) {
			FARString strEntryName;

			if (!HelpLanguage
					|| (!GetLangParam(LangFile, L"PluginContents", &strEntryName, nullptr, nCodePage)
							&& !GetLangParam(LangFile, L"DocumentContents", &strEntryName, nullptr,
									nCodePage))) {
				/*
					$ 01.08.2001 SVS
					Не допускаем дубликатов!
					Если в каталог с ФАРом положить еще один HLF с одноименным
					языком, то... фигня получается при выборе языка.
				*/
				if (0 == Map.count(strLangName))
				{
					FARString strItemText;
					strItemText.Format(L"%.40ls", !strLangDescr.IsEmpty() ? strLangDescr.CPtr():strLangName.CPtr());
					Map[strLangName] = strItemText;
				}
			}
		}

		fclose(LangFile);
	}

	MenuItemEx LangMenuItem;
	for (auto it = Map.cbegin(); it != Map.cend(); ++it)
	{
		LangMenuItem.strName = it->second;
		LangMenuItem.SetSelect(!StrCmpI(*strDest, it->first));
		LangMenu->SetUserData(it->first.CPtr(), 0, LangMenu->AddItem(&LangMenuItem));
	}

	LangMenu->AssignHighlights(FALSE);
	LangMenu->Process();

	if (LangMenu->Modal::GetExitCode() < 0)
		return FALSE;

	wchar_t *lpwszDest = strDest->GetBuffer(LangMenu->GetUserDataSize() / sizeof(wchar_t) + 1);
	LangMenu->GetUserData(lpwszDest, LangMenu->GetUserDataSize());
	strDest->ReleaseBuffer();
	return (LangMenu->GetUserDataSize());
}

/*
	$ 01.09.2000 SVS
	+ Новый метод, для получения параметров для .Options
	.Options <KeyName>=<Value>
*/
int GetOptionsParam(FILE *SrcFile, const wchar_t *KeyName, FARString &strValue, UINT nCodePage)
{
	wchar_t ReadStr[1024];
	FARString strFullParamName;
	int Length = StrLength(L".Options");
	long CurFilePos = ftell(SrcFile);

	StringReader SR;
	while (SR.Read(SrcFile, ReadStr, 1024, nCodePage)) {
		if (!StrCmpNI(ReadStr, L".Options", Length)) {
			strFullParamName = RemoveExternalSpaces(ReadStr + Length);
			size_t pos;

			if (strFullParamName.RPos(pos, L'=')) {
				strValue = strFullParamName;
				strValue.LShift(pos + 1);
				RemoveExternalSpaces(strValue);
				strFullParamName.Truncate(pos);
				RemoveExternalSpaces(strFullParamName);

				if (!StrCmpI(strFullParamName, KeyName)) {
					fseek(SrcFile, CurFilePos, SEEK_SET);
					return TRUE;
				}
			}
		}
	}

	fseek(SrcFile, CurFilePos, SEEK_SET);
	return FALSE;
}

///////////////////////////////////////

class LanguageData
{
	std::vector<void *> _data;
	bool _shrinked = false;

	LanguageData(const LanguageData &) = delete;

public:
	LanguageData() = default;

	~LanguageData()
	{
		for (const auto &p : _data) {
			free(p);
		}
	}

	int Add(const void *ptr, size_t len)
	{
		_shrinked = false;
		_data.emplace_back();
		_data.back() = malloc(len);
		if (!_data.back()) {
			_data.pop_back();
			return -1;
		}

		memcpy(_data.back(), ptr, len);
		return _data.size() - 1;
	}

	template <class CharT>
	inline int AddChars(const CharT *chars, size_t cnt)
	{
		return Add(chars, cnt * sizeof(CharT));
	}

	template <class StrT>
	inline int AddStr(const StrT &str)
	{
		return AddChars(str.c_str(), str.size() + 1);
	}

	const void *Get(size_t ID)
	{
		if (!_shrinked) {
			_shrinked = true;
			_data.shrink_to_fit();
		}
		return (ID < _data.size()) ? _data[ID] : nullptr;
	}

	inline int Count() const { return _data.size(); }
};

///

static wchar_t *RefineLangString(wchar_t *str, size_t &len)
{
	while (IsSpace(*str)) {
		++str;
	}

	if (*str != L'\"') {
		return nullptr;
	}

	wchar_t *dst = ++str;

	for (const wchar_t *src = str; *src; ++dst) {
		switch (*src) {
			case L'\\':
				switch (src[1]) {
					case L'\\':
						*dst = L'\\';
						src+= 2;
						break;

					case L'\"':
						*dst = L'\"';
						src+= 2;
						break;

					case L'n':
						*dst = L'\n';
						src+= 2;
						break;

					case L'r':
						*dst = L'\r';
						src+= 2;
						break;

					case L'b':
						*dst = L'\b';
						src+= 2;
						break;

					case L't':
						*dst = L'\t';
						src+= 2;
						break;

					default:
						*dst = L'\\';
						src++;
						break;
				}
				break;

			case L'"':
				*dst = L'"';
				src+= (src[1] == L'"') ? 2 : 1;
				break;

			default:
				if (dst != src) {
					*dst = *src;
				}

				src++;
		}
	}

	for (;;) {
		*dst = 0;
		if (dst == str || !(IsSpace(*(dst - 1)) || IsEol(*(dst - 1)))) {
			break;
		}
		--dst;
	}

	if (dst != str && *(dst - 1) == L'"') {
		*(dst - 1) = 0;
	}

	len = dst - str;
	return str;
}

bool Language::Init(const wchar_t *path, bool wide, int expected_max_id)
{
	// fprintf(stderr, "Language::Init(%ls, %u, %d)\n", path, wide, expected_max_id);
	if (_loaded && _data && _data->Count())
		return true;

	ErrnoSaver gle;
	UINT codepage = CP_UTF8;
	// fprintf(stderr, "Opt.strLanguage=%ls\n", Opt.strLanguage.CPtr());
	FARString lang_name = Opt.strLanguage;

	FILE *lang_file =
			OpenLangFile(path, LangFileMask, Opt.strLanguage, _message_file, codepage, FALSE, &lang_name);

	if (!lang_file) {
		_last_error = LERROR_FILE_NOT_FOUND;
		return false;
	}

	_last_error = LERROR_BAD_FILE;
	std::unique_ptr<LanguageData> new_data;
	try {
		new_data.reset(new LanguageData);

		wchar_t ReadStr[1024];
		StringReader sr;
		std::string tmp;
		while (sr.Read(lang_file, ReadStr, ARRAYSIZE(ReadStr), codepage)) {
			size_t refined_len;
			const wchar_t *refined_str = RefineLangString(ReadStr, refined_len);
			if (!refined_str)
				continue;

			if (wide) {
				new_data->AddChars(refined_str, refined_len + 1);

			} else {
				Wide2MB(refined_str, refined_len, tmp);
				new_data->AddStr(tmp);
			}
		}

		// Проведем проверку на количество строк в LNG-файлах
		if (expected_max_id == -1 || expected_max_id + 1 == new_data->Count()) {
			_last_error = LERROR_SUCCESS;
		} else {
			fprintf(stderr, "%s: count expected=%d but actual=%d\n", __FUNCTION__, expected_max_id + 1,
					new_data->Count());
		}

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}

	fclose(lang_file);

	if (_last_error != LERROR_SUCCESS)
		return false;

	if (this == &Lang && StrCmpI(Opt.strLanguage, lang_name))
		Opt.strLanguage = lang_name;

	_data = std::move(new_data);
	_wide = wide;
	_loaded = true;
	return true;
}

Language::Language() {}

Language::~Language() {}

void Language::Close()
{
	if (this != &Lang) {
		_data.reset();
		_wide = true;
	}

	_loaded = false;
}

const void *Language::GetMsg(FarLangMsgID id) const
{
	if (_data && (_loaded || this == &Lang)) {
		const void *out = _data->Get(id);
		if (out) {
			return out;
		}
	}

	fprintf(stderr, "Language::GetMsg(%d) Count=%d _wide=%d _loaded=%d\n", id, _data ? _data->Count() : -1,
			_wide, _loaded);

	/*
		$ 26.03.2002 DJ
		если менеджер уже в дауне - сообщение не выводим
	*/
	if (!FrameManager->ManagerIsDown()) {
		/*
			$ 03.09.2000 IS
			! Нормальное сообщение об отсутствии строки в языковом файле
			(раньше имя файла обрезалось справа и приходилось иногда гадать - в
			каком же файле ошибка)
		*/
		/* IS $ */
		FormatString line1, line2;
		line1 << L"Incorrect or damaged " << _message_file;
		line2 << L"Message " << id << L" not found";
		if (Message(MSG_WARNING, 2, L"Error", line1, line2, L"Ok", L"Quit") == 1)
			exit(0);
	}

	return nullptr;
}

const wchar_t *Language::GetMsgWide(FarLangMsgID id) const
{
	if (!_wide) {
		fprintf(stderr, "Language::GetMsgWide(%d): but language is MULTIBYTE\n", id);
		return L"";
	}

	return (const wchar_t *)GetMsg(id);
}

const char *Language::GetMsgMB(FarLangMsgID id) const
{
	if (_wide) {
		fprintf(stderr, "Language::GetMsgMB(%d): but language is WIDE\n", id);
		return "";
	}

	return (const char *)GetMsg(id);
}

FarLangMsgID Language::InternMsg(const wchar_t *str)
{
	if (!_data)
		return -1;

	const size_t len = wcslen(str);
	if (_wide)
		return _data->AddChars(str, len + 1);

	std::string tmp;
	Wide2MB(str, len, tmp);
	return _data->AddStr(tmp);
}

FarLangMsgID Language::InternMsg(const char *str)
{
	if (!_data)
		return -1;

	const size_t len = strlen(str);
	if (!_wide)
		return _data->AddChars(str, len + 1);

	std::wstring tmp;
	MB2Wide(str, len, tmp);
	return _data->AddStr(tmp);
}

//////////
const wchar_t *FarLangMsg::GetMsg(FarLangMsgID id)
{
	return ::Lang.GetMsgWide(id);
}
