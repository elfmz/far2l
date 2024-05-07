#pragma once

/*
help.hpp

Помощь
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

#include "frame.hpp"
#include "keybar.hpp"
#include "array.hpp"
#include "chgmmode.hpp"

class CallBackStack;

#define HelpBeginLink  L'<'
#define HelpEndLink    L'>'
#define HelpFormatLink L"<%ls/>%ls"

#define HELPMODE_CLICKOUTSIDE 0x20000000	// было нажатие мыши вне хелпа?

struct StackHelpData
{
	DWORD Flags;				// флаги
	int TopStr;					// номер верхней видимой строки темы
	int CurX, CurY;				// координаты (???)

	FARString strHelpMask;		// значение маски
	FARString strHelpPath;		// путь к хелпам
	FARString strHelpTopic;		// текущий топик
	FARString strSelTopic;		// выделенный топик (???)

	void Clear()
	{
		Flags = 0;
		TopStr = 0;
		CurX = CurY = 0;
		strHelpMask.Clear();
		strHelpPath.Clear();
		strHelpTopic.Clear();
		strSelTopic.Clear();
	}
};

enum HELPDOCUMENTSHELPTYPE
{
	HIDX_PLUGINS,		// Индекс плагинов
	HIDX_DOCUMS,		// Индекс документов
};

enum
{
	FHELPOBJ_ERRCANNOTOPENHELP = 0x80000000,
};

class HelpRecord
{
public:
	wchar_t *HelpStr;

	HelpRecord(const wchar_t *HStr = nullptr)
	{
		HelpStr = nullptr;
		if (HStr)
			HelpStr = wcsdup(HStr);
	}

	const HelpRecord &operator=(const HelpRecord &rhs)
	{
		if (this != &rhs) {
			free(HelpStr);
			HelpStr = wcsdup(rhs.HelpStr);
		}

		return *this;
	}

	bool operator==(const HelpRecord &rhs) const { return !StrCmpI(HelpStr, rhs.HelpStr); }

	int operator<(const HelpRecord &rhs) const { return StrCmpI(HelpStr, rhs.HelpStr) < 0; }

	~HelpRecord() { free(HelpStr); }
};

class Help : public Frame
{
private:
	ChangeMacroMode CMM;
	BOOL ErrorHelp;			// TRUE - ошибка! Например - нет такого топика
	SaveScreen *TopScreen;	// область сохранения под хелпом
	KeyBar HelpKeyBar;		// кейбар
	CallBackStack *Stack;	// стек возврата
	FARString strFullHelpPathName;

	StackHelpData StackData;
	TArray<HelpRecord> HelpList;	// "хелп" в памяти.

	int StrCount;					// количество строк в теме
	int FixCount;					// количество строк непрокручиваемой области
	int FixSize;					// Размер непрокручиваемой области
	int TopicFound;					// TRUE - топик найден
	int IsNewTopic;					// это новый топик?
	int MouseDown;

	FARString strCtrlColorChar;		// CtrlColorChar - опция! для спецсимвола-
	//   символа - для атрибутов
	uint64_t CurColor;						// CurColor - текущий цвет отрисовки
	int CtrlTabSize;					// CtrlTabSize - опция! размер табуляции

	FARString strCurPluginContents;		// помним PluginContents (для отображения в заголовке)

	DWORD LastStartPos;
	DWORD StartPos;

	FARString strCtrlStartPosChar;

	FARString strLastSearchStr;
	int LastSearchCase = 0, LastSearchWholeWords = 0, LastSearchRegexp = 0;

private:
	virtual void DisplayObject();
	int ReadHelp(const wchar_t *Mask = nullptr);
	void AddLine(const wchar_t *Line);
	void AddTitle(const wchar_t *Title);
	void HighlightsCorrection(FARString &strStr);
	void FastShow();
	void DrawWindowFrame();
	void OutString(const wchar_t *Str);
	int StringLen(const wchar_t *Str);
	void CorrectPosition();
	int IsReferencePresent();
	void MoveToReference(int Forward, int CurScreen);
	void ReadDocumentsHelp(int TypeIndex);
	void Search(FILE *HelpFile, uintptr_t nCodePage);
	int JumpTopic(const wchar_t *JumpTopic = nullptr);
	const HelpRecord *GetHelpItem(int Pos);

public:
	Help(const wchar_t *Topic, const wchar_t *Mask = nullptr, DWORD Flags = 0);
	virtual ~Help();

	static void Present(const wchar_t *Topic, const wchar_t *Mask = nullptr, DWORD Flags = 0);

public:
	virtual void Hide();
	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual void InitKeyBar();
	BOOL GetError() { return ErrorHelp; }
	virtual void SetScreenPosition();
	virtual void OnChangeFocus(int focus);	// вызывается при смене фокуса
	virtual void ResizeConsole();

	virtual int FastHide();		// Введена для нужд CtrlAltShift

	virtual const wchar_t *GetTypeName() { return L"[Help]"; }
	virtual int GetTypeAndName(FARString &strType, FARString &strName);
	virtual int GetType() { return MODALTYPE_HELP; }

	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam);

	static FARString &MkTopic(INT_PTR PluginNumber, const wchar_t *HelpTopic, FARString &strTopic);
};
