#pragma once

/*
fileedit.hpp

Редактирование файла - надстройка над editor.cpp
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

// коды возврата Editor::SaveFile()
enum
{
	SAVEFILE_ERROR   = 0,         // пытались сохранять, не получилось
	SAVEFILE_SUCCESS = 1,         // либо успешно сохранили, либо сохранять было не надо
	SAVEFILE_CANCEL  = 2          // сохранение отменено, редактор не закрывать
};

// как открывать
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
	FFILEEDIT_NEW                   = 0x00010000,  // Этот файл СОВЕРШЕННО! новый или его успели стереть! Нету такого и все тут.
	FFILEEDIT_REDRAWTITLE           = 0x00020000,  // Нужно редравить заголовок?
	FFILEEDIT_FULLSCREEN            = 0x00040000,  // Полноэкранный режим?
	FFILEEDIT_DISABLEHISTORY        = 0x00080000,  // Запретить запись в историю?
	FFILEEDIT_ENABLEF6              = 0x00100000,  // Переключаться во вьювер можно?
	FFILEEDIT_SAVETOSAVEAS          = 0x00200000,  // $ 17.08.2001 KM  Добавлено для поиска по AltF7.
	//   При редактировании найденного файла из архива для
	//   клавиши F2 сделать вызов ShiftF2.
	FFILEEDIT_SAVEWQUESTIONS        = 0x00400000,  // сохранить без вопросов
	FFILEEDIT_LOCKED                = 0x00800000,  // заблокировать?
	FFILEEDIT_OPENFAILED            = 0x01000000,  // файл открыть не удалось
	FFILEEDIT_DELETEONCLOSE         = 0x02000000,  // удалить в деструкторе файл вместе с каталогом (если тот пуст)
	FFILEEDIT_DELETEONLYFILEONCLOSE = 0x04000000,  // удалить в деструкторе только файл
	FFILEEDIT_CANNEWFILE            = 0x10000000,  // допускается новый файл?
	FFILEEDIT_SERVICEREGION         = 0x20000000,  // используется сервисная область
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
		// Добавлено для поиска по AltF7. При редактировании найденного файла из
		// архива для клавиши F2 сделать вызов ShiftF2.
		void SetSaveToSaveAs(int ToSaveAs) { Flags.Change(FFILEEDIT_SAVETOSAVEAS,ToSaveAs); InitKeyBar(); }
		virtual BOOL IsFileModified() const { return m_editor->IsFileModified(); };
		virtual int GetTypeAndName(FARString &strType, FARString &strName);
		int EditorControl(int Command,void *Param);
		void SetCodePage(UINT codepage);  //BUGBUG
		BOOL IsFileChanged() const { return m_editor->IsFileChanged(); };
		virtual int64_t VMProcess(int OpCode,void *vParam=nullptr,int64_t iParam=0);
		void GetEditorOptions(EditorOptions& EdOpt);
		void SetEditorOptions(EditorOptions& EdOpt);
		void CodepageChangedByUser() {Flags.Set(FFILEEDIT_CODEPAGECHANGEDBYUSER);};
		virtual void Show();
		void SetPluginTitle(const wchar_t *PluginTitle);

		struct ISaveObserver
		{
			virtual void OnEditedFileSaved(const wchar_t *FileName) = 0;
		};

		void SetSaveObserver(ISaveObserver *observer = nullptr) { SaveObserver = observer;}

		static const FileEditor *CurrentEditor;

	private:
		Editor *m_editor;
		KeyBar EditKeyBar;
		NamesList *EditNamesList;
		bool F4KeyOnly;
		FARString strFileName;
		FARString strFullFileName;
		FARString strStartDir;
		FARString strTitle;
		FARString strPluginTitle;
		FARString strPluginData;
		FARString strLoadedFileName;
		FAR_FIND_DATA_EX FileInfo;
		wchar_t AttrStr[4]; // 13.02.2001 IS - Сюда запомним буквы атрибутов, чтобы не вычислять их много раз
		IUnmakeWritablePtr FileUnmakeWritable;
		DWORD SysErrorCode;
		bool m_bClosing; // 28.04.2005 AY: true когда редактор закрываеться (т.е. в деструкторе)
		bool bEE_READ_Sent;
		FemaleBool m_AddSignature;
		bool BadConversion;
		UINT m_codepage; //BUGBUG
		int SaveAsTextFormat;
		ISaveObserver *SaveObserver = nullptr;

		virtual void DisplayObject();
		int  ProcessQuitKey(int FirstSave,BOOL NeedQuestion=TRUE);
		BOOL UpdateFileList();
		bool DecideAboutSignature();
		/* Ret:
		      0 - не удалять ничего
		      1 - удалять файл и каталог
		      2 - удалять только файл
		*/
		void SetDeleteOnClose(int NewMode);
		int ReProcessKey(int Key,int CalledFromControl=TRUE);
		bool AskOverwrite(const FARString& FileName);
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
		virtual int FastHide(); // для нужд CtrlAltShift
		// возвращает признак того, является ли файл временным
		// используется для принятия решения переходить в каталог по CtrlF10
		BOOL isTemporary();
		virtual void ResizeConsole();
		int LoadFile(const wchar_t *Name, int &UserBreak);
		//TextFormat, Codepage и AddSignature используются ТОЛЬКО, если bSaveAs = true!
		int SaveFile(const wchar_t *Name, int Ask, bool bSaveAs, int TextFormat = 0, UINT Codepage = CP_UTF8, bool AddSignature=false);
		void SetTitle(const wchar_t *Title);
		virtual FARString &GetTitle(FARString &Title,int SubLen=-1,int TruncSize=0);
		BOOL SetFileName(const wchar_t *NewFileName);
		int ProcessEditorInput(INPUT_RECORD *Rec);
		void ChangeEditKeyBar();
		DWORD EditorGetFileAttributes(const wchar_t *Name);
		void SetPluginData(const wchar_t *PluginData);
		const wchar_t *GetPluginData() {return strPluginData.CPtr();}
		bool LoadFromCache(EditorCacheParams *pp);
		void SaveToCache();
};

bool dlgOpenEditor(FARString &strFileName, UINT &codepage);
void ModalEditTempFile(const std::string &pathname, bool scroll_to_end);//erases file internally

struct BaseEditedFileUploader : public FileEditor::ISaveObserver
{
	struct timespec mtim{};

	void GetCurrentTimestamp()
	{
		struct stat s{};
		if (sdc_stat(strTempFileName.GetMB().c_str(), &s) == 0)
		{
			mtim = s.st_mtim;
		}
	}

	virtual void OnEditedFileSaved(const wchar_t *FileName)
	{
		if (strTempFileName != FileName)
		{
			fprintf(stderr, "OnEditedFileSaved: '%ls' != '%ls'\n", strTempFileName.CPtr(), FileName);
			return;
		}

		UploadTempFile();
		GetCurrentTimestamp();
	}

protected:
	FARString strTempFileName;

	virtual void UploadTempFile() = 0;

public:
	BaseEditedFileUploader(const FARString &strTempFileName_)
	:
		strTempFileName(strTempFileName_)
	{
		GetCurrentTimestamp();
	}

	virtual ~BaseEditedFileUploader()
	{
	}

	void UploadIfTimestampChanged()
	{
		struct stat s{};
		if (sdc_stat(strTempFileName.GetMB().c_str(), &s) == 0
		 && (mtim.tv_sec != s.st_mtim.tv_sec || mtim.tv_nsec != s.st_mtim.tv_nsec))
		{
			UploadTempFile();
		}
	}
};
