#pragma once

/*
fileview.hpp

Просмотр файла - надстройка над viewer.cpp
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
#include "viewer.hpp"
#include "keybar.hpp"

class FileViewer : public Frame
{
private:
	virtual void Show();
	virtual void DisplayObject();
	SudoClientRegion _sdc_rgn;
	Viewer View;
	int RedrawTitle;
	KeyBar ViewKeyBar;
	bool AutoClose;
	bool F3KeyOnly;
	int FullScreen;
	int DisableEdit;
	int DisableHistory;

	FARString strName;

	typedef class Frame inherited;
	/*
		$ 17.08.2001 KM
		Добавлено для поиска по AltF7. При редактировании найденного файла из
		архива для клавиши F2 сделать вызов ShiftF2.
	*/
	int SaveToSaveAs;
	FARString strPluginData;
	FileHolderPtr UngreppedFH;
	int64_t UngreppedPos{0};

	void GrepFilter();
	void GrepFilterDismiss();

public:
	FileViewer(FileHolderPtr NewFileHolder, int EnableSwitch = FALSE, int DisableHistory = FALSE,
			int DisableEdit = FALSE, long ViewStartPos = -1, const wchar_t *PluginData = nullptr,
			NamesList *ViewNamesList = nullptr, int ToSaveAs = FALSE, UINT aCodePage = CP_AUTODETECT);
	FileViewer(FileHolderPtr NewFileHolder, int EnableSwitch, int DisableHistory, const wchar_t *Title, int X1,
			int Y1, int X2, int Y2, UINT aCodePage = CP_AUTODETECT);

	virtual ~FileViewer();

public:
	void Init(FileHolderPtr NewFileHolder, int EnableSwitch, int DisableHistory, long ViewStartPos,
			const wchar_t *PluginData, NamesList *ViewNamesList, int ToSaveAs);
	virtual void InitKeyBar();
	virtual int ProcessKey(FarKey Key);
	virtual int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);
	virtual int64_t VMProcess(MacroOpcode OpCode, void *vParam = nullptr, int64_t iParam = 0);
	virtual void ShowConsoleTitle();
	virtual void OnDestroy();
	virtual void OnChangeFocus(int focus);

	virtual int GetTypeAndName(FARString &strType, FARString &strName);
	virtual const wchar_t *GetTypeName() { return L"[FileView]"; };		///
	virtual int GetType() { return MODALTYPE_VIEWER; }

	void SetEnableF6(int AEnable)
	{
		DisableEdit = !AEnable;
		InitKeyBar();
	}
	void SetPluginData(const wchar_t *PluginData) { strPluginData = PluginData; }

	/* $ Введена для нужд CtrlAltShift OT */
	virtual int FastHide();

	/*
		$ 17.08.2001 KM
		Добавлено для поиска по AltF7. При редактировании найденного файла из
		архива для клавиши F2 сделать вызов ShiftF2.
	*/
	void SetSaveToSaveAs(int ToSaveAs)
	{
		SaveToSaveAs = ToSaveAs;
		InitKeyBar();
	}
	int ViewerControl(int Command, void *Param);
	BOOL IsFullScreen() { return FullScreen; }
	virtual FARString &GetTitle(FARString &Title, int SubLen = -1, int TruncSize = 0);
	int64_t GetViewFileSize() const;
	int64_t GetViewFilePos() const;
	void ShowStatus();
	void SetAutoClose(bool AC) { AutoClose = AC; }
};

void ModalViewFile(const std::string &pathname);
void ViewConsoleHistory(HANDLE con_hnd, bool modal, bool autoclose);
