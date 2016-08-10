#pragma once

/*
fileedit.hpp

Ðåäàêòèðîâàíèå ôàéëà - íàäñòðîéêà íàä editor.cpp
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
#include "editor.hpp"
#include "keybar.hpp"

class NamesList;

// êîäû âîçâðàòà Editor::SaveFile()
enum
{
	SAVEFILE_ERROR   = 0,         // ïûòàëèñü ñîõðàíÿòü, íå ïîëó÷èëîñü
	SAVEFILE_SUCCESS = 1,         // ëèáî óñïåøíî ñîõðàíèëè, ëèáî ñîõðàíÿòü áûëî íå íàäî
	SAVEFILE_CANCEL  = 2          // ñîõðàíåíèå îòìåíåíî, ðåäàêòîð íå çàêðûâàòü
};

// êàê îòêðûâàòü
enum FEOPMODEEXISTFILE
{
	FEOPMODE_QUERY        =0x00000000,
	FEOPMODE_NEWIFOPEN    =0x10000000,
	FEOPMODE_USEEXISTING  =0x20000000,
	FEOPMODE_BREAKIFOPEN  =0x30000000,
	FEOPMODE_RELOAD       =0x40000000,
};

enum FFILEEDIT_FLAGS
{
	FFILEEDIT_NEW                   = 0x00010000,  // Ýòîò ôàéë ÑÎÂÅÐØÅÍÍÎ! íîâûé èëè åãî óñïåëè ñòåðåòü! Íåòó òàêîãî è âñå òóò.
	FFILEEDIT_REDRAWTITLE           = 0x00020000,  // Íóæíî ðåäðàâèòü çàãîëîâîê?
	FFILEEDIT_FULLSCREEN            = 0x00040000,  // Ïîëíîýêðàííûé ðåæèì?
	FFILEEDIT_DISABLEHISTORY        = 0x00080000,  // Çàïðåòèòü çàïèñü â èñòîðèþ?
	FFILEEDIT_ENABLEF6              = 0x00100000,  // Ïåðåêëþ÷àòüñÿ âî âüþâåð ìîæíî?
	FFILEEDIT_SAVETOSAVEAS          = 0x00200000,  // $ 17.08.2001 KM  Äîáàâëåíî äëÿ ïîèñêà ïî AltF7.
	//   Ïðè ðåäàêòèðîâàíèè íàéäåííîãî ôàéëà èç àðõèâà äëÿ
	//   êëàâèøè F2 ñäåëàòü âûçîâ ShiftF2.
	FFILEEDIT_SAVEWQUESTIONS        = 0x00400000,  // ñîõðàíèòü áåç âîïðîñîâ
	FFILEEDIT_LOCKED                = 0x00800000,  // çàáëîêèðîâàòü?
	FFILEEDIT_OPENFAILED            = 0x01000000,  // ôàéë îòêðûòü íå óäàëîñü
	FFILEEDIT_DELETEONCLOSE         = 0x02000000,  // óäàëèòü â äåñòðóêòîðå ôàéë âìåñòå ñ êàòàëîãîì (åñëè òîò ïóñò)
	FFILEEDIT_DELETEONLYFILEONCLOSE = 0x04000000,  // óäàëèòü â äåñòðóêòîðå òîëüêî ôàéë
	FFILEEDIT_CANNEWFILE            = 0x10000000,  // äîïóñêàåòñÿ íîâûé ôàéë?
	FFILEEDIT_SERVICEREGION         = 0x20000000,  // èñïîëüçóåòñÿ ñåðâèñíàÿ îáëàñòü
	FFILEEDIT_CODEPAGECHANGEDBYUSER = 0x40000000,
};


class FileEditor : public Frame
{
	public:
		FileEditor(const wchar_t *Name, UINT codepage, DWORD InitFlags,int StartLine=-1,int StartChar=-1,const wchar_t *PluginData=nullptr,int OpenModeExstFile=FEOPMODE_QUERY);
		FileEditor(const wchar_t *Name, UINT codepage, DWORD InitFlags,int StartLine,int StartChar,const wchar_t *Title,int X1,int Y1,int X2,int Y2,int DeleteOnClose=0,int OpenModeExstFile=FEOPMODE_QUERY);
		virtual ~FileEditor();

		void ShowStatus();
		void SetLockEditor(BOOL LockMode);
		bool IsFullScreen() {return Flags.Check(FFILEEDIT_FULLSCREEN)!=FALSE;}
		void SetNamesList(NamesList *Names);
		void SetEnableF6(int AEnableF6) { Flags.Change(FFILEEDIT_ENABLEF6,AEnableF6); InitKeyBar(); }
		// Äîáàâëåíî äëÿ ïîèñêà ïî AltF7. Ïðè ðåäàêòèðîâàíèè íàéäåííîãî ôàéëà èç
		// àðõèâà äëÿ êëàâèøè F2 ñäåëàòü âûçîâ ShiftF2.
		void SetSaveToSaveAs(int ToSaveAs) { Flags.Change(FFILEEDIT_SAVETOSAVEAS,ToSaveAs); InitKeyBar(); }
		virtual BOOL IsFileModified() const { return m_editor->IsFileModified(); };
		virtual int GetTypeAndName(string &strType, string &strName);
		int EditorControl(int Command,void *Param);
		void SetCodePage(UINT codepage);  //BUGBUG
		BOOL IsFileChanged() const { return m_editor->IsFileChanged(); };
		virtual int64_t VMProcess(int OpCode,void *vParam=nullptr,int64_t iParam=0);
		void GetEditorOptions(EditorOptions& EdOpt);
		void SetEditorOptions(EditorOptions& EdOpt);
		void CodepageChangedByUser() {Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);};
		virtual void Show();
		void SetPluginTitle(const wchar_t *PluginTitle);

		static const FileEditor *CurrentEditor;

	private:
		Editor *m_editor;
		KeyBar EditKeyBar;
		NamesList *EditNamesList;
		bool F4KeyOnly;
		string strFileName;
		string strFullFileName;
		string strStartDir;
		string strTitle;
		string strPluginTitle;
		string strPluginData;
		FAR_FIND_DATA_EX FileInfo;
		wchar_t AttrStr[4];            // 13.02.2001 IS - Ñþäà çàïîìíèì áóêâû àòðèáóòîâ, ÷òîáû íå âû÷èñëÿòü èõ ìíîãî ðàç
		DWORD FileAttributes;          // 12.02.2001 IS - ñþäà çàïîìíèì àòðèáóòû ôàéëà ïðè îòêðûòèè, ïðèãîäÿòñÿ ãäå-íèáóäü...
		BOOL  FileAttributesModified;  // 04.11.2003 SKV - íàäî ëè âîññòàíàâëèâàòü àòòðèáóòû ïðè save
		DWORD SysErrorCode;
		bool m_bClosing;               // 28.04.2005 AY: true êîãäà ðåäàêòîð çàêðûâàåòüñÿ (ò.å. â äåñòðóêòîðå)
		bool bEE_READ_Sent;
		bool m_bAddSignature;
		bool BadConversion;
		UINT m_codepage; //BUGBUG

		virtual void DisplayObject();
		int  ProcessQuitKey(int FirstSave,BOOL NeedQuestion=TRUE);
		BOOL UpdateFileList();
		/* Ret:
		      0 - íå óäàëÿòü íè÷åãî
		      1 - óäàëÿòü ôàéë è êàòàëîã
		      2 - óäàëÿòü òîëüêî ôàéë
		*/
		void SetDeleteOnClose(int NewMode);
		int ReProcessKey(int Key,int CalledFromControl=TRUE);
		bool AskOverwrite(const string& FileName);
		void Init(const wchar_t *Name, UINT codepage, const wchar_t *Title, DWORD InitFlags, int StartLine, int StartChar, const wchar_t *PluginData, int DeleteOnClose, int OpenModeExstFile);
		virtual void InitKeyBar();
		virtual int ProcessKey(int Key);
		virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
		virtual void ShowConsoleTitle();
		virtual void OnChangeFocus(int focus);
		virtual void SetScreenPosition();
		virtual const wchar_t *GetTypeName() {return L"[FileEdit]";};
		virtual int GetType() { return MODALTYPE_EDITOR; }
		virtual void OnDestroy();
		virtual int GetCanLoseFocus(int DynamicMode=FALSE);
		virtual int FastHide(); // äëÿ íóæä CtrlAltShift
		// âîçâðàùàåò ïðèçíàê òîãî, ÿâëÿåòñÿ ëè ôàéë âðåìåííûì
		// èñïîëüçóåòñÿ äëÿ ïðèíÿòèÿ ðåøåíèÿ ïåðåõîäèòü â êàòàëîã ïî CtrlF10
		BOOL isTemporary();
		virtual void ResizeConsole();
		int LoadFile(const wchar_t *Name, int &UserBreak);
		//TextFormat, Codepage è AddSignature èñïîëüçóþòñÿ ÒÎËÜÊÎ, åñëè bSaveAs = true!
		int SaveFile(const wchar_t *Name, int Ask, bool bSaveAs, int TextFormat = 0, UINT Codepage = CP_UNICODE, bool AddSignature=false);
		void SetTitle(const wchar_t *Title);
		virtual string &GetTitle(string &Title,int SubLen=-1,int TruncSize=0);
		BOOL SetFileName(const wchar_t *NewFileName);
		int ProcessEditorInput(INPUT_RECORD *Rec);
		void ChangeEditKeyBar();
		DWORD EditorGetFileAttributes(const wchar_t *Name);
		void SetPluginData(const wchar_t *PluginData);
		const wchar_t *GetPluginData() {return strPluginData.CPtr();}
		bool LoadFromCache(EditorCacheParams *pp);
		void SaveToCache();
};

bool dlgOpenEditor(string &strFileName, UINT &codepage);
