#pragma once

/*
macro.hpp

Ìàêðîñû
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

#include "syntax.hpp"
#include "tvar.hpp"
#include "macroopcode.hpp"

enum MACRODISABLEONLOAD
{
	MDOL_ALL            = 0x80000000, // äèñàáëèì âñå ìàêðîñû ïðè çàãðóçêå
	MDOL_AUTOSTART      = 0x00000001, // äèñàáëèì àâòîñòàðòóþùèå ìàêðîñû
};

// îáëàñòè äåéñòâèÿ ìàêðîñîâ (íà÷àëî èñïîëíåíèÿ) -  ÍÅ ÁÎËÅÅ 0xFF îáëàñòåé!
enum MACROMODEAREA
{
	MACRO_FUNCS                =  -3,
	MACRO_CONSTS               =  -2,
	MACRO_VARS                 =  -1,

	// see also plugin.hpp # FARMACROAREA
	MACRO_OTHER                =   0, // Ðåæèì êîïèðîâàíèÿ òåêñòà ñ ýêðàíà, âåðòèêàëüíûå ìåíþ
	MACRO_SHELL                =   1, // Ôàéëîâûå ïàíåëè
	MACRO_VIEWER               =   2, // Âíóòðåííÿÿ ïðîãðàììà ïðîñìîòðà
	MACRO_EDITOR               =   3, // Ðåäàêòîð
	MACRO_DIALOG               =   4, // Äèàëîãè
	MACRO_SEARCH               =   5, // Áûñòðûé ïîèñê â ïàíåëÿõ
	MACRO_DISKS                =   6, // Ìåíþ âûáîðà äèñêîâ
	MACRO_MAINMENU             =   7, // Îñíîâíîå ìåíþ
	MACRO_MENU                 =   8, // Ïðî÷èå ìåíþ
	MACRO_HELP                 =   9, // Ñèñòåìà ïîìîùè
	MACRO_INFOPANEL            =  10, // Èíôîðìàöèîííàÿ ïàíåëü
	MACRO_QVIEWPANEL           =  11, // Ïàíåëü áûñòðîãî ïðîñìîòðà
	MACRO_TREEPANEL            =  12, // Ïàíåëü äåðåâà ïàïîê
	MACRO_FINDFOLDER           =  13, // Ïîèñê ïàïîê
	MACRO_USERMENU             =  14, // Ìåíþ ïîëüçîâàòåëÿ
	MACRO_AUTOCOMPLETION       =  15, // Ñïèñîê àâòîäîïîëíåíèÿ

	MACRO_COMMON,                     // ÂÅÇÄÅ! - äîëæåí áûòü ïðåäïîñëåäíèì, ò.ê. ïðèîðèòåò ñàìûé íèçøèé !!!
	MACRO_LAST                        // Äîëæåí áûòü âñåãäà ïîñëåäíèì! Èñïîëüçóåòñÿ â öèêëàõ
};

enum MACROFLAGS_MFLAGS
{
	MFLAGS_MODEMASK            =0x000000FF, // ìàñêà äëÿ âûäåëåíèÿ îáëàñòè äåéñòâèÿ (îáëàñòè íà÷àëà èñïîëíåíèÿ) ìàêðîñà

	MFLAGS_DISABLEOUTPUT       =0x00000100, // ïîäàâèòü îáíîâëåíèå ýêðàíà âî âðåìÿ âûïîëíåíèÿ ìàêðîñà
	MFLAGS_NOSENDKEYSTOPLUGINS =0x00000200, // ÍÅ ïåðåäàâàòü ïëàãèíàì êëàâèøè âî âðåìÿ çàïèñè/âîñïðîèçâåäåíèÿ ìàêðîñà
	MFLAGS_RUNAFTERFARSTARTED  =0x00000400, // ýòîò ìàêðîñ óæå çàïóñêàëñÿ ïðè ñòàðòå ÔÀÐà
	MFLAGS_RUNAFTERFARSTART    =0x00000800, // ýòîò ìàêðîñ çàïóñêàåòñÿ ïðè ñòàðòå ÔÀÐà

	MFLAGS_EMPTYCOMMANDLINE    =0x00001000, // çàïóñêàòü, åñëè êîìàíäíàÿ ëèíèÿ ïóñòà
	MFLAGS_NOTEMPTYCOMMANDLINE =0x00002000, // çàïóñêàòü, åñëè êîìàíäíàÿ ëèíèÿ íå ïóñòà
	MFLAGS_EDITSELECTION       =0x00004000, // çàïóñêàòü, åñëè åñòü âûäåëåíèå â ðåäàêòîðå
	MFLAGS_EDITNOSELECTION     =0x00008000, // çàïóñêàòü, åñëè åñòü íåò âûäåëåíèÿ â ðåäàêòîðå

	MFLAGS_SELECTION           =0x00010000, // àêòèâíàÿ:  çàïóñêàòü, åñëè åñòü âûäåëåíèå
	MFLAGS_PSELECTION          =0x00020000, // ïàññèâíàÿ: çàïóñêàòü, åñëè åñòü âûäåëåíèå
	MFLAGS_NOSELECTION         =0x00040000, // àêòèâíàÿ:  çàïóñêàòü, åñëè åñòü íåò âûäåëåíèÿ
	MFLAGS_PNOSELECTION        =0x00080000, // ïàññèâíàÿ: çàïóñêàòü, åñëè åñòü íåò âûäåëåíèÿ
	MFLAGS_NOFILEPANELS        =0x00100000, // àêòèâíàÿ:  çàïóñêàòü, åñëè ýòî ïëàãèíîâàÿ ïàíåëü
	MFLAGS_PNOFILEPANELS       =0x00200000, // ïàññèâíàÿ: çàïóñêàòü, åñëè ýòî ïëàãèíîâàÿ ïàíåëü
	MFLAGS_NOPLUGINPANELS      =0x00400000, // àêòèâíàÿ:  çàïóñêàòü, åñëè ýòî ôàéëîâàÿ ïàíåëü
	MFLAGS_PNOPLUGINPANELS     =0x00800000, // ïàññèâíàÿ: çàïóñêàòü, åñëè ýòî ôàéëîâàÿ ïàíåëü
	MFLAGS_NOFOLDERS           =0x01000000, // àêòèâíàÿ:  çàïóñêàòü, åñëè òåêóùèé îáúåêò "ôàéë"
	MFLAGS_PNOFOLDERS          =0x02000000, // ïàññèâíàÿ: çàïóñêàòü, åñëè òåêóùèé îáúåêò "ôàéë"
	MFLAGS_NOFILES             =0x04000000, // àêòèâíàÿ:  çàïóñêàòü, åñëè òåêóùèé îáúåêò "ïàïêà"
	MFLAGS_PNOFILES            =0x08000000, // ïàññèâíàÿ: çàïóñêàòü, åñëè òåêóùèé îáúåêò "ïàïêà"

	MFLAGS_REG_MULTI_SZ        =0x10000000, // òåêñò ìàêðîñà ìíîãîñòðî÷íûé (REG_MULTI_SZ)

	MFLAGS_NEEDSAVEMACRO       =0x40000000, // íåîáõîäèìî ýòîò ìàêðîñ çàïîìíèòü
	MFLAGS_DISABLEMACRO        =0x80000000, // ýòîò ìàêðîñ îòêëþ÷åí
};


// êîäû âîçâðàòà äëÿ KeyMacro::GetCurRecord()
enum MACRORECORDANDEXECUTETYPE
{
	MACROMODE_NOMACRO          =0,  // íå â ðåæèìå ìàêðî
	MACROMODE_EXECUTING        =1,  // èñïîëíåíèå: áåç ïåðåäà÷è ïëàãèíó ïèìï
	MACROMODE_EXECUTING_COMMON =2,  // èñïîëíåíèå: ñ ïåðåäà÷åé ïëàãèíó ïèìï
	MACROMODE_RECORDING        =3,  // çàïèñü: áåç ïåðåäà÷è ïëàãèíó ïèìï
	MACROMODE_RECORDING_COMMON =4,  // çàïèñü: ñ ïåðåäà÷åé ïëàãèíó ïèìï
};

class Panel;

struct TMacroFunction;
typedef bool (*INTMACROFUNC)(const TMacroFunction*);

enum INTMF_FLAGS{
	IMFF_UNLOCKSCREEN               =0x00000001,
	IMFF_DISABLEINTINPUT            =0x00000002,
};

struct TMacroFunction
{
	const wchar_t *Name;             // èìÿ ôóíêöèè
	int nParam;                      // êîëè÷åñòâî ïàðàìåòðîâ
	int oParam;                      // íåîáÿçàòåëüíûå ïàðàìåòðû
	TMacroOpCode Code;               // áàéòêîä ôóíêöèè
	const wchar_t *fnGUID;           // GUID îáðàáîò÷èêà ôóíêöèè

	int    BufferSize;               // Ðàçìåð áóôåðà êîìïèëèðîâàííîé ïîñëåäîâàòåëüíîñòè
	DWORD *Buffer;                   // êîìïèëèðîâàííàÿ ïîñëåäîâàòåëüíîñòü (OpCode) ìàêðîñà
	//wchar_t  *Src;                   // îðèãèíàëüíûé "òåêñò" ìàêðîñà
	//wchar_t  *Description;           // îïèñàíèå ìàêðîñà

	const wchar_t *Syntax;           // Ñèíòàêñèñ ôóíêöèè

	DWORD IntFlags;                  // ôëàãè èç INTMF_FLAGS (â îñíîâíîì îòâå÷àþùèå "êàê âûçûâàòü ôóíêöèþ")
	INTMACROFUNC Func;               // ôóíêöèÿ
};

struct MacroRecord
{
	DWORD  Flags;         // Ôëàãè ìàêðîïîñëåäîâàòåëüíîñòè
	int    Key;           // Íàçíà÷åííàÿ êëàâèøà
	int    BufferSize;    // Ðàçìåð áóôåðà êîìïèëèðîâàííîé ïîñëåäîâàòåëüíîñòè
	DWORD *Buffer;        // êîìïèëèðîâàííàÿ ïîñëåäîâàòåëüíîñòü (OpCode) ìàêðîñà
	wchar_t  *Src;           // îðèãèíàëüíûé "òåêñò" ìàêðîñà
	wchar_t  *Description;   // îïèñàíèå ìàêðîñà
	DWORD  Reserved[2];   // çàðåçåðâèðîâàíî
};

#define STACKLEVEL      32

struct MacroState
{
	int KeyProcess;
	int Executing;
	int MacroPC;
	int ExecLIBPos;
	int MacroWORKCount;
	bool UseInternalClipboard;
	struct MacroRecord *MacroWORK; // ò.í. òåêóùåå èñïîëíåíèå
	INPUT_RECORD cRec; // "îïèñàíèå ðåàëüíî íàæàòîé êëàâèøè"

	bool AllocVarTable;
	TVarTable *locVarTable;

	void Init(TVarTable *tbl);
};


struct MacroPanelSelect {
	int     Action;
	DWORD   ActionFlags;
	int     Mode;
	int64_t Index;
	TVar    *Item;
};

/* $TODO:
    1. Óäàëèòü IndexMode[], Sort()
    2. Èç MacroLIB ñäåëàòü
       struct MacroRecord *MacroLIB[MACRO_LAST];
*/
class KeyMacro
{
	private:
		DWORD MacroVersion;

		static DWORD LastOpCodeUF; // ïîñëåäíèé íå çàíÿòûé OpCode äëÿ UserFunction (îòíîñèòåëüíî KEY_MACRO_U_BASE)
		// äëÿ ôóíêöèé
		static size_t CMacroFunction;
		static size_t AllocatedFuncCount;
		static TMacroFunction *AMacroFunction;

		// òèï çàïèñè - ñ âûçîâîì äèàëîãà íàñòðîåê èëè...
		// 0 - íåò çàïèñè, 1 - ïðîñòàÿ çàïèñü, 2 - âûçîâ äèàëîãà íàñòðîåê
		int Recording;
		int InternalInput;
		int IsRedrawEditor;

		int Mode;
		int StartMode;

		struct MacroState Work;
		struct MacroState PCStack[STACKLEVEL];
		int CurPCStack;

		// ñþäà "ìîãóò" ïèñàòü òîëüêî ïðè ÷òåíèè ìàêðîñîâ (çàíåñåíèå íîâîãî),
		// à èñïîëíÿòü ÷åðåç MacroWORK
		int MacroLIBCount;
		struct MacroRecord *MacroLIB;

		int IndexMode[MACRO_LAST][2];

		int RecBufferSize;
		DWORD *RecBuffer;
		wchar_t *RecSrc;

		class LockScreen *LockScr;

	private:
		int ReadVarsConst(int ReadMode, string &strBuffer);
		int ReadMacroFunction(int ReadMode, string &strBuffer);
		int WriteVarsConst(int WriteMode);
		int ReadMacros(int ReadMode, string &strBuffer);
		DWORD AssignMacroKey();
		int GetMacroSettings(int Key,DWORD &Flags);
		void InitInternalVars(BOOL InitedRAM=TRUE);
		void InitInternalLIBVars();
		void ReleaseWORKBuffer(BOOL All=FALSE); // óäàëèòü âðåìåííûé áóôåð

		DWORD SwitchFlags(DWORD& Flags,DWORD Value);
		string &MkRegKeyName(int IdxMacro,string &strRegKeyName);

		BOOL CheckEditSelected(DWORD CurFlags);
		BOOL CheckInsidePlugin(DWORD CurFlags);
		BOOL CheckPanel(int PanelMode,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckCmdLine(int CmdLength,DWORD Flags);
		BOOL CheckFileFolder(Panel *ActivePanel,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckAll(int CheckMode,DWORD CurFlags);
		void Sort();
		TVar FARPseudoVariable(DWORD Flags,DWORD Code,DWORD& Err);
		DWORD GetOpCode(struct MacroRecord *MR,int PC);
		DWORD SetOpCode(struct MacroRecord *MR,int PC,DWORD OpCode);

	private:
		static LONG_PTR WINAPI AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
		static LONG_PTR WINAPI ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

	public:
		KeyMacro();
		~KeyMacro();

	public:
		int ProcessKey(int Key);
		int GetKey();
		int PeekKey();
		bool IsOpCode(DWORD p);
		bool CheckWaitKeyFunc();

		int PushState(bool CopyLocalVars=FALSE);
		int PopState();
		int GetLevelState() {return CurPCStack;};

		int  IsRecording() {return(Recording);};
		int  IsExecuting() {return(Work.Executing);};
		int  IsExecutingLastKey();
		int  IsDsableOutput() {return CheckCurMacroFlags(MFLAGS_DISABLEOUTPUT);};
		void SetMode(int Mode) {KeyMacro::Mode=Mode;};
		int  GetMode() {return(Mode);};

		void DropProcess();

		void RunStartMacro();

		// Ïîìåñòèòü âðåìåííîå ñòðîêîâîå ïðåäñòàâëåíèå ìàêðîñà
		int PostNewMacro(const wchar_t *PlainText,DWORD Flags=0,DWORD AKey=0,BOOL onlyCheck=FALSE);
		// Ïîìåñòèòü âðåìåííûé ðåêîðä (áèíàðíîå ïðåäñòàâëåíèå)
		int PostNewMacro(struct MacroRecord *MRec,BOOL NeedAddSendFlag=0,BOOL IsPluginSend=FALSE);

		int  LoadMacros(BOOL InitedRAM=TRUE,BOOL LoadAll=TRUE);
		void SaveMacros(BOOL AllSaved=TRUE);

		int GetStartIndex(int Mode) {return IndexMode[Mode<MACRO_LAST-1?Mode:MACRO_LAST-1][0];}
		// Ôóíêöèÿ ïîëó÷åíèÿ èíäåêñà íóæíîãî ìàêðîñà â ìàññèâå
		int GetIndex(int Key, int Mode, bool UseCommon=true);
		// ïîëó÷åíèå ðàçìåðà, çàíèìàåìîãî óêàçàííûì ìàêðîñîì
		int GetRecordSize(int Key, int Mode);

		bool GetPlainText(string& Dest);
		int  GetPlainTextSize();

		void SetRedrawEditor(int Sets) {IsRedrawEditor=Sets;}

		void RestartAutoMacro(int Mode);

		// ïîëó÷èòü äàííûå î ìàêðîñå (âîçâðàùàåò ñòàòóñ)
		int GetCurRecord(struct MacroRecord* RBuf=nullptr,int *KeyPos=nullptr);
		// ïðîâåðèòü ôëàãè òåêóùåãî èñïîëíÿåìîãî ìàêðîñà.
		BOOL CheckCurMacroFlags(DWORD Flags);

		static const wchar_t* GetSubKey(int Mode);
		static int   GetSubKey(const wchar_t *Mode);
		static int   GetMacroKeyInfo(bool FromReg,int Mode,int Pos,string &strKeyName,string &strDescription);
		static wchar_t *MkTextSequence(DWORD *Buffer,int BufferSize,const wchar_t *Src=nullptr);
		// èç ñòðîêîâîãî ïðåäñòàâëåíèÿ ìàêðîñà ñäåëàòü MacroRecord
		int ParseMacroString(struct MacroRecord *CurMacro,const wchar_t *BufPtr,BOOL onlyCheck=FALSE);
		BOOL GetMacroParseError(DWORD* ErrCode, COORD* ErrPos, string *ErrSrc);
		BOOL GetMacroParseError(string *Err1, string *Err2, string *Err3, string *Err4);

		static void SetMacroConst(const wchar_t *ConstName, const TVar Value);
		static DWORD GetNewOpCode();

		static size_t GetCountMacroFunction();
		static const TMacroFunction *GetMacroFunction(size_t Index);
		static void RegisterMacroIntFunction();
		static TMacroFunction *RegisterMacroFunction(const TMacroFunction *tmfunc);
		static bool UnregMacroFunction(size_t Index);
};

BOOL WINAPI KeyMacroToText(int Key,string &strKeyText0);
int WINAPI KeyNameMacroToKey(const wchar_t *Name);
void initMacroVarTable(int global);
void doneMacroVarTable(int global);
bool checkMacroConst(const wchar_t *name);
const wchar_t *eStackAsString(int Pos=0);

inline bool IsMenuArea(int Area){return Area==MACRO_MAINMENU || Area==MACRO_MENU || Area==MACRO_DISKS || Area==MACRO_USERMENU || Area==MACRO_AUTOCOMPLETION;}
