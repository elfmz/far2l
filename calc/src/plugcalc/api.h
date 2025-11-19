//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#ifndef _CALC_API_H_
#define _CALC_API_H_

#include <vector>
#include <string>
#include <windows.h>
#include <farplug-wide.h>
#include <farkeys.h>
#include <farcolor.h>

#define CALC_PREFIX L"calc:"

struct CalcCoord
{
	short X;
	short Y;
};

struct CalcRect
{
	short Left;
	short Top;
	short Right;
	short Bottom;
};

struct CalcColor
{
	uint64_t Flags;
	unsigned int ForegroundColor;
	unsigned int BackgroundColor;
	void* Reserved;
};

struct CalcDialogItemColors
{
	size_t StructSize;
	uint64_t Flags;
	size_t ColorsCount;
	struct CalcColor* Colors;
};



///////////////////////////////////////////////////////////////////////////////////

typedef void *DLGHANDLE;
typedef intptr_t CALC_INT_PTR;
typedef CALC_INT_PTR (__stdcall *CALCDLGPROC)(DLGHANDLE hdlg, int msg, int param1, void *param2);


class CalcDialog
{
protected:
	uint64_t editColor, selColor, highlightColor;

public:
	CalcDialog();
	virtual ~CalcDialog();

	bool Init(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
					struct FarDialogItem *Item, unsigned int ItemsNumber);
	intptr_t Run();

	void EnableRedraw(bool);
	void ResizeDialog(const CalcCoord & dims);
	void RedrawDialog();
	void GetDlgRect(CalcRect *rect);
	void Close(int exitcode);

	void GetDlgItemShort(int id, FarDialogItem *item);
	void SetDlgItemShort(int id, const FarDialogItem & item);
	void SetItemPosition(int id, const CalcRect & rect);
	int  GetFocus();
	void SetFocus(int id);
	void EditChange(int id, const FarDialogItem & item);
	void SetSelection(int id, const EditorSelect & sel);
	void SetCursorPos(int id, const CalcCoord & pos);
	void GetText(int id, std::wstring &str);
	void SetText(int id, const std::wstring & str);
	bool IsChecked(int id);
	void AddHistory(int id, const std::wstring & str);

public:
	virtual CALC_INT_PTR OnInitDialog(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnClose(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnResizeConsole(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnDrawDialog(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnButtonClick(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnGotFocus(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnEditChange(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnKey(int param1, void *param2) { return -1; }
	virtual CALC_INT_PTR OnCtrlColorDlgItem(int param1, void *param2) { return -1; }

public:
	typedef CALC_INT_PTR (CalcDialog::*CalcDialogCallback)(int param1, void *param2);
	CalcDialogCallback *msg_tbl;
protected:
	DLGHANDLE hdlg;
};

///////////////////////////////////////////////////////////////////////////////////

class CalcDialogFuncs
{
public:
	virtual void EnableRedraw(DLGHANDLE, bool) = 0;
	virtual void ResizeDialog(DLGHANDLE, const CalcCoord & dims) = 0;
	virtual void RedrawDialog(DLGHANDLE) = 0;
	virtual void Close(DLGHANDLE, int exitcode) = 0;
	virtual void GetDlgRect(DLGHANDLE, CalcRect *rect) = 0;

	virtual void GetDlgItemShort(DLGHANDLE, int id, FarDialogItem *item) = 0;
	virtual void SetDlgItemShort(DLGHANDLE, int id, const FarDialogItem & item) = 0;
	virtual void SetItemPosition(DLGHANDLE, int id, const CalcRect & rect) = 0;
	virtual int  GetFocus(DLGHANDLE) = 0;
	virtual void SetFocus(DLGHANDLE, int id) = 0;
	virtual void EditChange(DLGHANDLE, int id, const FarDialogItem & item) = 0;
	virtual void SetSelection(DLGHANDLE, int id, const EditorSelect & sel) = 0;
	virtual void SetCursorPos(DLGHANDLE, int id, const CalcCoord & pos) = 0;
	virtual void GetText(DLGHANDLE, int id, std::wstring &str) = 0;
	virtual void SetText(DLGHANDLE, int id, const std::wstring & str) = 0;
	virtual void AddHistory(DLGHANDLE, int id, const std::wstring & str) = 0;
	virtual bool IsChecked(DLGHANDLE, int id) = 0;

public:
	virtual DLGHANDLE DialogInit(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
								struct FarDialogItem *Item, unsigned int ItemsNumber,
								CALCDLGPROC dlgProc) = 0;
	virtual intptr_t DialogRun(DLGHANDLE) = 0;
	virtual void DialogFree(DLGHANDLE) = 0;
	virtual CALC_INT_PTR DefDlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2) = 0;

	virtual CalcDialog::CalcDialogCallback *GetMessageTable() = 0;
};

///////////////////////////////////////////////////////////////////////////////////

class CalcApi
{
public:
	virtual ~CalcApi() {}

	virtual void GetPluginInfo(void *pinfo, const wchar_t *name) = 0;
	virtual const wchar_t *GetMsg(int MsgId) = 0;
	virtual CalcDialogFuncs *GetDlgFuncs() = 0;

	virtual intptr_t Message(unsigned long Flags, const wchar_t *HelpTopic, const wchar_t * const *Items,
						int ItemsNumber, int ButtonsNumber) = 0;

	virtual intptr_t Menu(int X, int Y, int MaxHeight, unsigned long long Flags,
					const wchar_t *Title, const wchar_t *HelpTopic,
					const std::vector<FarMenuItem> & Items) = 0;

	virtual void EditorGet(EditorGetString *string, EditorInfo *info) = 0;
	virtual void SetSelection(const EditorSelect & sel) = 0;
	virtual void EditorInsert(const wchar_t *text) = 0;
	virtual void EditorRedraw() = 0;

	virtual void GetDlgColors(uint64_t *edit_color, uint64_t *sel_color, uint64_t *highlight_color) = 0;

	virtual int  GetCmdLine(std::wstring &cmd) = 0;
	virtual void SetCmdLine(const std::wstring & cmd) = 0;

	virtual bool SettingsBegin() = 0;
	virtual bool SettingsEnd() = 0;
	virtual bool SettingsGet(const char *name, std::wstring *sval, int *lval) = 0;
	virtual bool SettingsSet(const char *name, const std::wstring *sval, const int *ival) = 0;

	virtual const wchar_t *GetModuleName() = 0;

};

extern CalcApi *CreateApiFar2(const struct PluginStartupInfo *Info);

extern CalcApi *api;

#endif
