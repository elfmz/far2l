#pragma once

/*
dialog.hpp

Класс диалога Dialog.

Предназначен для отображения модальных диалогов.
Является производным от класса Frame.
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
#include "plugin.hpp"
#include "vmenu.hpp"
#include "bitflags.hpp"
#include "CriticalSections.hpp"

class History;

// Флаги текущего режима диалога
enum DIALOG_MODES
{
	DMODE_INITOBJECTS           =0x00000001, // элементы инициализарованы?
	DMODE_CREATEOBJECTS         =0x00000002, // объекты (Edit,...) созданы?
	DMODE_WARNINGSTYLE          =0x00000004, // Warning Dialog Style?
	DMODE_DRAGGED               =0x00000008, // диалог двигается?
	DMODE_ISCANMOVE             =0x00000010, // можно ли двигать диалог?
	DMODE_ALTDRAGGED            =0x00000020, // диалог двигается по Alt-Стрелка?
	DMODE_SMALLDIALOG           =0x00000040, // "короткий диалог"
	DMODE_DRAWING               =0x00001000, // диалог рисуется?
	DMODE_KEY                   =0x00002000, // Идет посылка клавиш?
	DMODE_SHOW                  =0x00004000, // Диалог виден?
	DMODE_MOUSEEVENT            =0x00008000, // Нужно посылать MouseMove в обработчик?
	DMODE_RESIZED               =0x00010000, //
	DMODE_ENDLOOP               =0x00020000, // Конец цикла обработки диалога?
	DMODE_BEGINLOOP             =0x00040000, // Начало цикла обработки диалога?
	//DMODE_OWNSITEMS           =0x00080000, // если TRUE, Dialog освобождает список Item в деструкторе
	DMODE_NODRAWSHADOW          =0x00100000, // не рисовать тень?
	DMODE_NODRAWPANEL           =0x00200000, // не рисовать подложку?
	DMODE_FULLSHADOW            =0x00400000,
	DMODE_NOPLUGINS             =0x00800000,
	DMODE_KEEPCONSOLETITLE      =0x10000000, // не изменять заголовок консоли
	DMODE_CLICKOUTSIDE          =0x20000000, // было нажатие мыши вне диалога?
	DMODE_MSGINTERNAL           =0x40000000, // Внутренняя Message?
	DMODE_OLDSTYLE              =0x80000000, // Диалог в старом (до 1.70) стиле
};

//#define DIMODE_REDRAW       0x00000001 // требуется принудительная прорисовка итема?

#define MakeDialogItemsEx(Data,Item) \
	DialogItemEx Item[ARRAYSIZE(Data)]; \
	DataToItemEx(Data,Item,ARRAYSIZE(Data));

// Структура, описывающая автоматизацию для DIF_AUTOMATION
// на первом этапе - примитивная - выставление флагов у элементов для CheckBox
struct DialogItemAutomation
{
	WORD ID;                    // Для этого элемента...
	DWORD Flags[3][2];          // ...выставить вот эти флаги
	// [0] - Unchecked, [1] - Checked, [2] - 3Checked
	// [][0] - Set, [][1] - Skip
};

class DlgUserControl
{
	public:
		COORD CursorPos;
		bool CursorVisible;
		DWORD CursorSize;

	public:
		DlgUserControl():
			CursorVisible(false),
			CursorSize(static_cast<DWORD>(-1))
		{
			CursorPos.X=CursorPos.Y=-1;
		}
		~DlgUserControl() {};
};

/*
Описывает один элемент диалога - внутренне представление.
Для плагинов это FarDialogItem (за исключением ObjPtr)
*/
struct DialogItemEx
{
	int Type;
	int X1,Y1,X2,Y2;
	int Focus;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		FarList *ListItems;
		int  ListPos;
		CHAR_INFO *VBuf;
	};
	FARString strHistory;
	FARString strMask;
	DWORD Flags;
	int DefaultButton;

	FARString strData;
	size_t nMaxLength;

	WORD ID;
	BitFlags IFlags;
	unsigned AutoCount;   // Автоматизация
	DialogItemAutomation* AutoPtr;
	DWORD_PTR UserData; // ассоциированные данные

	// прочее
	void *ObjPtr;
	VMenu *ListPtr;
	DlgUserControl *UCData;

	int SelStart;
	int SelEnd;

	void Clear()
	{
		Type=0;
		X1=0;
		Y1=0;
		X2=0;
		Y2=0;
		Focus=0;
		Reserved=0;
		strHistory.Clear();
		strMask.Clear();
		Flags=0;
		DefaultButton=0;
		strData.Clear();
		nMaxLength=0;
		ID=0;
		IFlags.ClearAll();
		AutoCount=0;
		AutoPtr=nullptr;
		UserData=0;
		ObjPtr=nullptr;
		ListPtr=nullptr;
		UCData=nullptr;
		SelStart=0;
		SelEnd=0;
	}

	const DialogItemEx &operator=(const DialogItemEx &Other)
	{
		Type          = Other.Type;
		X1            = Other.X1;
		X2            = Other.X2;
		Y1            = Other.Y1;
		Y2            = Other.Y2;
		Focus         = Other.Focus;
		Reserved      = Other.Reserved;
		Flags         = Other.Flags;
		DefaultButton = Other.DefaultButton;
		strData       = Other.strData;
		nMaxLength    = Other.nMaxLength;
		ID            = Other.ID;
		IFlags        = Other.IFlags;
		AutoCount     = Other.AutoCount;
		AutoPtr       = Other.AutoPtr;
		UserData      = Other.UserData;
		ObjPtr        = Other.ObjPtr;
		ListPtr       = Other.ListPtr;
		UCData        = Other.UCData;
		SelStart      = Other.SelStart;
		SelEnd        = Other.SelEnd;
		return *this;
	}

	void Indent(int Delta)
	{
		X1 += Delta;
		X2 += Delta;
	}

	bool AddAutomation(int id,
		FarDialogItemFlags UncheckedSet,FarDialogItemFlags UncheckedSkip,
		FarDialogItemFlags CheckedSet,FarDialogItemFlags CheckedSkip,
		FarDialogItemFlags Checked3Set,FarDialogItemFlags Checked3Skip)
	{
		DialogItemAutomation *Auto;

		if ((Auto=(DialogItemAutomation*)xf_realloc(AutoPtr,sizeof(DialogItemAutomation)*(AutoCount+1))) )
		{
			AutoPtr=Auto;
			Auto=AutoPtr+AutoCount;
			Auto->ID=id;
			Auto->Flags[0][0]=UncheckedSet;
			Auto->Flags[0][1]=UncheckedSkip;
			Auto->Flags[1][0]=CheckedSet;
			Auto->Flags[1][1]=CheckedSkip;
			Auto->Flags[2][0]=Checked3Set;
			Auto->Flags[2][1]=Checked3Skip;
			AutoCount++;
			return true;
		}
		return false;
	}
};

/*
Описывает один элемент диалога - для сокращения объемов
Структура аналогичена структуре InitDialogItem (см. "Far PlugRinG
Russian Help Encyclopedia of Developer")
*/

struct DialogDataEx
{
	WORD  Type;
	short X1,Y1,X2,Y2;
	union
	{
		DWORD_PTR Reserved;
		unsigned int Selected;
		const wchar_t *History;
		const wchar_t *Mask;
		FarList *ListItems;
		int  ListPos;
		CHAR_INFO *VBuf;
	};
	DWORD Flags;
	const wchar_t *Data;
};

class DlgEdit;
class ConsoleTitle;

class Dialog: public Frame
{
		friend class DlgEdit;
		friend LONG_PTR WINAPI SendDlgMessage(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
		friend LONG_PTR WINAPI DefDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

	private:
		bool bInitOK;               // диалог был успешно инициализирован
		INT_PTR PluginNumber;       // Номер плагина, для формирования HelpTopic
		unsigned FocusPos;               // всегда известно какой элемент в фокусе
		unsigned PrevFocusPos;           // всегда известно какой элемент был в фокусе
		int IsEnableRedraw;         // Разрешена перерисовка диалога? ( 0 - разрешена)
		BitFlags DialogMode;        // Флаги текущего режима диалога

		LONG_PTR DataDialog;        // Данные, специфические для конкретного экземпляра диалога (первоначально здесь параметр, переданный в конструктор)

		DialogItemEx **Item; // массив элементов диалога
		DialogItemEx *pSaveItemEx; // пользовательский массив элементов диалога

		unsigned ItemCount;         // количество элементов диалога

		ConsoleTitle *OldTitle;     // предыдущий заголовок
		int PrevMacroMode;          // предыдущий режим макро

		FARWINDOWPROC RealDlgProc;      // функция обработки диалога

		// переменные для перемещения диалога
		int OldX1,OldX2,OldY1,OldY2;

		wchar_t *HelpTopic;

		volatile int DropDownOpened;// Содержит статус комбобокса и хистори: TRUE - открыт, FALSE - закрыт.

		CriticalSection CS;

		int RealWidth, RealHeight;

		GUID Id;
		bool IdExist;

	private:
		void Init(FARWINDOWPROC DlgProc,LONG_PTR InitParam);
		virtual void DisplayObject();
		void DeleteDialogObjects();
		int  LenStrItem(int ID, const wchar_t *lpwszStr = nullptr);

		void ShowDialog(unsigned ID=(unsigned)-1);  //    ID=-1 - отрисовать весь диалог

		LONG_PTR CtlColorDlgItem(int ItemPos,int Type,int Focus,int Default,DWORD Flags);
		/* $ 28.07.2000 SVS
		   + Изменяет фокус ввода между двумя элементами.
		     Вынесен отдельно для того, чтобы обработать DMSG_KILLFOCUS & DMSG_SETFOCUS
		*/
		void ChangeFocus2(unsigned SetFocusPos);

		unsigned ChangeFocus(unsigned FocusPos,int Step,int SkipGroup);
		BOOL SelectFromEditHistory(DialogItemEx *CurItem,DlgEdit *EditLine,const wchar_t *HistoryName,FARString &strStr);
		int SelectFromComboBox(DialogItemEx *CurItem,DlgEdit*EditLine,VMenu *List);
		int AddToEditHistory(const wchar_t *AddStr,const wchar_t *HistoryName);

		void ProcessLastHistory(DialogItemEx *CurItem, int MsgIndex);  // обработка DIF_USELASTHISTORY

		int ProcessHighlighting(int Key,unsigned FocusPos,int Translate);
		int CheckHighlights(WORD Chr,int StartPos=0);

		void SelectOnEntry(unsigned Pos,BOOL Selected);

		void CheckDialogCoord();
		BOOL GetItemRect(unsigned I,SMALL_RECT& Rect);
		bool ItemHasDropDownArrow(const DialogItemEx *Item);

		// возвращает заголовок диалога (текст первого текста или фрейма)
		const wchar_t *GetDialogTitle();

		BOOL SetItemRect(unsigned ID,SMALL_RECT *Rect);

		/* $ 23.06.2001 KM
		   + Функции программного открытия/закрытия комбобокса и хистори
		     и получения статуса открытости/закрытости комбобокса и хистори.
		*/
		volatile void SetDropDownOpened(int Status) { DropDownOpened=Status; }
		volatile int GetDropDownOpened() { return DropDownOpened; }

		void ProcessCenterGroup();
		unsigned ProcessRadioButton(unsigned);

		unsigned InitDialogObjects(unsigned ID=(unsigned)-1);

		int ProcessOpenComboBox(int Type,DialogItemEx *CurItem,unsigned CurFocusPos);
		int ProcessMoveDialog(DWORD Key);

		int Do_ProcessTab(int Next);
		int Do_ProcessNextCtrl(int Next,BOOL IsRedraw=TRUE);
		int Do_ProcessFirstCtrl();
		int Do_ProcessSpace();
		void SetComboBoxPos(DialogItemEx* Item=nullptr);

		LONG_PTR CallDlgProc(int nMsg, int nParam1, LONG_PTR nParam2);

		void ProcessKey(int Key, unsigned ItemPos);

	public:
		Dialog(DialogItemEx *SrcItem, unsigned SrcItemCount,
		       FARWINDOWPROC DlgProc=nullptr,LONG_PTR InitParam=0);
		Dialog(FarDialogItem *SrcItem, unsigned SrcItemCount,
		       FARWINDOWPROC DlgProc=nullptr,LONG_PTR InitParam=0);
		bool InitOK() {return bInitOK;}
		virtual ~Dialog();

	public:
		virtual int ProcessKey(int Key);
		virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
		virtual int64_t VMProcess(int OpCode,void *vParam=nullptr,int64_t iParam=0);
		virtual void Show();
		virtual void Hide();
		void FastShow() {ShowDialog();}

		void GetDialogObjectsData();

		void SetDialogMode(DWORD Flags) { DialogMode.Set(Flags); }
		bool CheckDialogMode(DWORD Flags) { return DialogMode.Check(Flags)!=FALSE; }

		// метод для перемещения диалога
		void AdjustEditPos(int dx,int dy);

		int IsMoving() {return DialogMode.Check(DMODE_DRAGGED);}
		void SetModeMoving(int IsMoving) { DialogMode.Change(DMODE_ISCANMOVE,IsMoving);}
		int  GetModeMoving() {return DialogMode.Check(DMODE_ISCANMOVE);}
		void SetDialogData(LONG_PTR NewDataDialog);
		LONG_PTR GetDialogData() {return DataDialog;};

		void InitDialog();
		void Process();
		void SetPluginNumber(INT_PTR NewPluginNumber) {PluginNumber=NewPluginNumber;}

		void SetHelp(const wchar_t *Topic);
		void ShowHelp();
		int Done() { return DialogMode.Check(DMODE_ENDLOOP); }
		void ClearDone();
		virtual void SetExitCode(int Code);

		void CloseDialog();

		virtual int GetTypeAndName(FARString &strType, FARString &strName);
		virtual int GetType() { return MODALTYPE_DIALOG; }
		virtual const wchar_t *GetTypeName() {return L"[Dialog]";};

		virtual int GetMacroMode();

		/* $ Введена для нужд CtrlAltShift OT */
		virtual int FastHide();
		virtual void ResizeConsole();
//    virtual void OnDestroy();

		// For MACRO
		const DialogItemEx **GetAllItem() {return (const DialogItemEx**)Item;};
		unsigned GetAllItemCount() {return ItemCount;};             // количество элементов диалога
		unsigned GetDlgFocusPos() {return FocusPos;};


		int SetAutomation(WORD IDParent,WORD id,
		                  FarDialogItemFlags UncheckedSet,FarDialogItemFlags UncheckedSkip,
		                  FarDialogItemFlags CheckedSet,FarDialogItemFlags CheckedSkip,
		                  FarDialogItemFlags Checked3Set=DIF_NONE,FarDialogItemFlags Checked3Skip=DIF_NONE);

		LONG_PTR WINAPI DlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

		virtual void SetPosition(int X1,int Y1,int X2,int Y2);

		BOOL IsInited();
		bool ProcessEvents();

		void SetId(const GUID& Id);

		friend class History;
};

typedef LONG_PTR(WINAPI *SENDDLGMESSAGE)(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

LONG_PTR WINAPI SendDlgMessage(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

LONG_PTR WINAPI DefDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

bool IsKeyHighlighted(const wchar_t *Str,int Key,int Translate,int AmpPos=-1);

void DataToItemEx(const DialogDataEx *Data,DialogItemEx *Item,int Count);

extern const wchar_t* fmtSavedDialogHistory;
