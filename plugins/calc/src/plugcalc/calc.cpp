//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>

//  XXX: Modified version! Added support for FAR2/UNICODE, BigNumbers etc.

//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include <utils.h>

#ifdef USE_CREGEXP
#include <regexp/clocale.h>
#endif

#include <sgml/sgml.h>

#include "api.h"

#include "newparse.h"
#include "calc.h"
#include "config.h"
#include "messages.h"

static wchar_t *coreReturn = NULL;

static FarDialogItem *cur_dlg_items = NULL;
static int cur_dlg_items_num = 0, cur_dlg_id = 0, cur_dlg_items_numedits = 0;
static int cur_dlg_need_recalc = 0;
static CalcCoord cur_dlg_dlg_size;

// XXX:
static int calc_cx = 55, calc_cy = 8;
static int cx_column_width[10] = { 0 };

int calc_edit_length = calc_cx - 23; // extern-referenced

static struct Addons
{
	unsigned num_custom;
	int max_len;
	int radio_id1, radio_id2;
	int edit_id1, edit_id2;
} addons_info;

static const FarDialogItem dialog_template[] =
{
	{ DI_DOUBLEBOX,    3,  1, 52, 11, 0, {}, 0, 0, L"" },
	{ DI_TEXT,         5,  3,  0,  0, 0, {}, 0, 0, L"&expr:" },
	{ DI_EDIT,        10,  3, 49,  0, 0, {(DWORD_PTR)L"calc_expr"}, DIF_FOCUS | DIF_HISTORY, 0, L"" },
	{ DI_TEXT,         5,  4,  0,  0, 0, {}, 0, 0, L"" },
}; //type,x1,y1,x2,y2,Sel,Flags,Data

#define DIALOG_BASE_LINE	5

static const int CALC_EDIT_ID = 2, CALC_TYPE_ID = 3;

static const wchar_t types[][20] =
{
	L"type:big number",
	L"type:int64", L"type:uint64", L"type:int ", L"type:short ", L"type:char ",
	L"type:uint ", L"type:ushort", L"type:byte", L"type:double", L"type:float"
};


static CalcParser *parser = NULL;

static int calc_error = 0;
static int curRadio = 4;

//////////////////////////////////////////////////////////////
// FAR exports
void CalcStartup()
{
	srand((unsigned)time(0));
}

void CalcOpenFromEditor()
{
	InitDynamicData();
	EditorDialog();
	DeInitDynamicData();
}

void CalcOpen(const wchar_t *expression)
{
	InitDynamicData();
	ShellDialog(expression);
	DeInitDynamicData();
}

bool CalcConfig()
{
	LoadConfig();
	CheckConfig();
	SaveConfig();
	if (ConfigDialog())
	{
		CheckConfig();
		SaveConfig();
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////
// loading settings
void InitDynamicData()
{
#ifdef USE_CREGEXP
	locale.cl_setlocale(towupper, towlower);
#endif

	srand((unsigned)time(0));

	LoadConfig();
	CheckConfig();
	SaveConfig();

	CalcParser::SetDelims(props.decimal_point[0], props.args[0], props.use_delim ? props.digit_delim[0] : 0);

	CalcParser::InitTables(props.rep_fraction_max_start, props.rep_fraction_max_period, props.cont_fraction_max);

	PSgmlEl BaseRc = new CSgmlEl;

	std::string calcset(StrWide2MB(api->GetModuleName()));
	size_t p = calcset.rfind('/');
	if (p != std::string::npos)
		calcset.resize(p + 1);

	calcset+= "calcset.csr";
	struct stat s{};
	if (stat(calcset.c_str(), &s) == -1)
		TranslateInstallPath_Lib2Share(calcset);

	BaseRc->parse(calcset);

	CalcParser::ProcessData(BaseRc, props.case_sensitive != 0);
	CalcParser::AddAll();

	delete BaseRc;

	memset(&addons_info, 0, sizeof(addons_info));

	if (parser != NULL)
		delete parser;
	parser = new CalcParser();

	CalcParser::ProcessAddons();
	CalcParser::ProcessDialogData();

}

void DeInitDynamicData()
{
	if (parser != NULL)
	{
		delete parser;
		parser = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// editor dialog
void EditorDialog()
{
	wchar_t *Text = NULL;
	int s, e, i;
	bool After = true, skip_eq = false;
	int num_fmi = 1 + CalcParser::main_addons_num;
	std::vector<FarMenuItem> fmi(num_fmi);
	fmi[0].Text = api->GetMsg(mMenuName);
	fmi[0].Selected = 1;
	for (i = 0; i < num_fmi - 1; i++)
	{
		fmi[i + 1].Text = CalcParser::addons[i].name.c_str();
	}

	i = (int)api->Menu(-1, -1, num_fmi, FMENU_WRAPMODE, L"action", NULL, fmi);

	if (i == -1)
		return;
	if (i == 0)
	{
		CalcShowDialog();
		if (coreReturn)
		{
			api->EditorInsert(coreReturn);
			free(coreReturn);
			coreReturn = NULL;
		}
		return;
	}

	EditorGetString EditStr { };
	EditorInfo EditInfo { };
	EditStr.StringNumber = -1;
	api->EditorGet(&EditStr, &EditInfo);

	for (e = (int)EditInfo.CurPos - 1; e > 0; e--)
		if (EditStr.StringText[e] != ' ')
			break;
	for (s = e; s > 0; s--)
		if (*PWORD(EditStr.StringText + s) == 0x2020)
			break;

	if (EditInfo.BlockStartLine == EditInfo.CurLine &&
			EditStr.SelStart != -1 && EditStr.SelEnd != -1)
	{
		// selection
		s = (int)EditStr.SelStart;
		e = (int)EditStr.SelEnd - 1;
		EditorSelect EditSel = { };
		EditSel.BlockType = BTYPE_NONE;
		api->SetSelection(EditSel);
		After = (int)EditInfo.CurPos >= (s + e)/2;
	}

	Text = (wchar_t *)malloc((e - s + 2) * sizeof(wchar_t));
	if (Text)
	{
		memmove(Text, EditStr.StringText + s, (e - s + 1) * sizeof(wchar_t));
		Text[e - s + 1] = 0;

		// if the last char is '='
		if (Text[e - s] == '=')
		{
			Text[e - s] = 0;
			skip_eq = true;
		}

		SArg Res = parser->Parse(Text, props.case_sensitive != 0);
		free(Text);

		Text = convertToString(Res, i - 1, 0, false, props.pad_zeroes != 0, false, NULL);
		if (Text)
		{
			if (After && !skip_eq)
				api->EditorInsert(L"=");
			if (!parser->GetError())
				api->EditorInsert(Text);
			if (!After && !skip_eq)
				api->EditorInsert(L"=");

			EditStr.StringNumber = -1;
			api->EditorGet(&EditStr, &EditInfo);
			EditorSelect EditSel = { };
			EditSel.BlockType = BTYPE_STREAM;
			EditSel.BlockStartLine = EditInfo.CurLine;
			EditSel.BlockStartPos  = EditInfo.CurPos - (int)wcslen(Text) - !After;
			EditSel.BlockWidth = (int)wcslen(Text);
			EditSel.BlockHeight = 1;
			api->SetSelection(EditSel);
			free(Text);
		}
	}

	api->EditorRedraw();
}

//////////////////////////////////////////////////////////////////////////
// shell dialog
void ShellDialog(const wchar_t *expression)
{
	int i = 0;
	if (expression == nullptr && CalcParser::GetNumDialogs() > 0)
	{
		while ( (i = CalcMenu(i)) != 0)
		{
			if (i == -1)
				return;
			ShowUnitsDialog(i);
		}
	}

	CalcShowDialog(expression);
	if (coreReturn)
	{
		std::wstring cmd;
		if (api->GetCmdLine(cmd))
		{
			cmd += coreReturn;
			api->SetCmdLine(cmd);
		}
		free(coreReturn);
		coreReturn = NULL;
	}
}

int CalcMenu(int c)
{
	unsigned i;
	int ret;

	std::vector<FarMenuItem> MenuEls(CalcParser::DialogsNum + 1);
	MenuEls[0].Text = _wcsdup(api->GetMsg(mName));

	PDialogData dd;
	for(i = 1, dd = CalcParser::DialogData; dd; dd = dd->Next, i++)
		MenuEls[i].Text = _wcsdup(dd->Name);

	MenuEls[c].Selected = TRUE;
	ret = (int)api->Menu(-1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
					api->GetMsg(mDialogs), L"Contents", MenuEls);

	for(i = 0; i < MenuEls.size(); i++)
		free((void *)MenuEls[i].Text);

	return ret;
}

/////////////////////////////////////////////////////////////////////

void getConsoleWindowSize(int *sX, int *sY)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi);
	*sX = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	*sY = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void SetUnitsDialogDims()
{
	int i, j, d, cx, cy;
	PDialogData dd = CalcParser::DialogData;
	PDialogElem de, de1;

	FarDialogItem *dialog = cur_dlg_items;

	for (i = 0; i < cur_dlg_id-1; i++)
		dd = dd->Next;
	de = dd->Elem;

	int sX, sY;
	getConsoleWindowSize(&sX, &sY);
	cx = sX - 2;
	cy = sY - 1;
	j = sY - 5;

	if (!((dd->num + 2) / j))
	{
		cy = dd->num + 4;
		cx = sX * 2 / 3;
	}

	dialog[0].X1 = 3;
	dialog[0].Y1 = 1;
	dialog[0].X2 = cx - 3;
	dialog[0].Y2 = cy - 2;

	cur_dlg_dlg_size.X = cx;
	cur_dlg_dlg_size.Y = cy;

	cur_dlg_items = dialog;

	int col_width = (cx - 9) / (dd->num / (j + 1) + 1);
	for (int i = 0; i < 10; i++)
		cx_column_width[i] = 0;

	int oldk = -1, col_textlen = 0;
	for (d = 1, i = 0, de1 = de; i < dd->num; i++,d++, de1 = de1->Next)
	{
		int k = 5 + (i / j) * (cx - 9) / (dd->num / (j + 1) + 1);
		if (k != oldk)
		{
			// find max text length for this column
			PDialogElem de2 = de1;
			col_textlen = 0;
			for (int ii = i; de2 && ii < i + j; ii++, de2 = de2->Next)
			{
				if (de2->Type != 0)
				{
					int l = (int)wcslen(de2->Name);
					if (l > col_textlen)
						col_textlen = l;
				}
			}
			oldk = k;
		}
		de1->column_idx = i/j;
		if (!de1->Type)
		{
			dialog[d].X1 = k;
			dialog[d].Y1 = i%j + 2;
		} else
		{
			int cl = (col_width/2 > col_textlen+2) ? col_textlen+2 : col_width/2;
			dialog[d].X1 = k + 1;
			dialog[d].Y1 = i%j + 2;
			dialog[d].X2 = dialog[d].X1 + cl;
			d++;
			dialog[d].X1 = k + cl;
			dialog[d].Y1 = i%j + 2;
			int w = col_width/2 + (col_width/2 - cl);
			dialog[d].X2 = dialog[d].X1 + w - 2;
			if (cx_column_width[i/j] < w)
				cx_column_width[i/j] = w;
		}
	}

}

class DlgUnits : public CalcDialog
{
public:
	PDialogData dd;
	bool inside_setdlgmessage;

	DlgUnits(PDialogData d)
	{
		inside_setdlgmessage = false;
		dd = d;
	}

	virtual ~DlgUnits()
	{
	}

	virtual CALC_INT_PTR OnInitDialog(int param1, void *param2)
	{
		if (coreReturn)
		{
			free(coreReturn);
			coreReturn = NULL;
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnResizeConsole(int param1, void *param2)
	{
		SetUnitsDialogDims();
		EnableRedraw(false);
		ResizeDialog(cur_dlg_dlg_size);
		for (int i = 0; i < cur_dlg_items_num; i++)
		{
			CalcRect rect;
			rect.Left = cur_dlg_items[i].X1;
			rect.Top = cur_dlg_items[i].Y1;
			rect.Right = cur_dlg_items[i].X2;
			rect.Bottom = cur_dlg_items[i].Y2;
			SetItemPosition(i, rect);
		}
		EnableRedraw(true);

		int curid = GetFocus();
		if (curid >= 0)
		{
			std::wstring str;
			GetText(curid, str);
			SetText(curid, str);
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnGotFocus(int param1, void *param2)
	{
		std::wstring str;
		GetText(param1, str);
		int id = 1;
		PDialogElem de;
		for (de = dd->Elem; de; de = de->Next, id++)
		{
			if (de->Type)
			{
				id++;
				EditorSelect EditSel = { };
				EditSel.BlockType = BTYPE_STREAM;
				EditSel.BlockStartLine = -1;
				EditSel.BlockHeight = 1;
				EditSel.BlockStartPos = 0;
				EditSel.BlockWidth = (id == param1) ? (int)str.size() : 0;
				SetSelection(id, EditSel);
			}
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnEditChange(int param1, void *param2)
	{
		if (inside_setdlgmessage)
			return FALSE;

		if (!props.auto_update)
			return FALSE;

		return OnEditChangeInternal(param1, param2);
	}

	virtual CALC_INT_PTR OnEditChangeInternal(int param1, void *param2)
	{
		FarDialogItem *Item = (FarDialogItem *)param2;
		Big val = 0;

		EnableRedraw(false);
		bool was_error = false;
		SArg res = parser->Parse(Item->PtrData, props.case_sensitive != 0);
		if (parser->GetError())
			was_error = true;

		int id = 1;
		PDialogElem de;
		for (de = dd->Elem; de; de = de->Next, id++)
		{
			if (de->Type) id++;
			if (id == param1)
			{
				if (de->input != NULL && de->input->parser != NULL)
				{
					SArg args[2];
					args[0] = res;
					try
					{
						val = de->input->parser->eval(args).GetBig();
					}
					catch(CALC_ERROR)
					{
						was_error = true;
					}
				} else
					val = res.GetBig() * de->scale;
				break;
			}
		}

		id = 1;
		for (de = dd->Elem; de; de = de->Next, id++)
		{
			if (de->Type)
			{
				id++;
				std::wstring str;
				if (!was_error)
				{
					int idx = CALC_CONV_UNITS;
					Big tmp;
					if (de->addon_idx >= 0)
					{
						idx = de->addon_idx;
						tmp = val;
					}
					else
					{
						tmp = de->scale != 0 ? val / de->scale : 0;
					}

					wchar_t *pwz = convertToString(tmp, idx, cx_column_width[de->column_idx] - 4, false, props.pad_zeroes != 0, true, nullptr);
					if (pwz)
					{
						str = pwz;
						free(pwz);
					}
					else
						str.clear();
				}

				if (id != param1)
				{
					//XXX:
					inside_setdlgmessage = true;
					SetText(id, str);
					CalcCoord coord;
					coord.X = 0;
					SetCursorPos(id, coord);
					inside_setdlgmessage = false;
				}
			}
		}
		EnableRedraw(true);
		return TRUE;
	}

	virtual CALC_INT_PTR OnKey(int param1, void *param2)
	{
		DWORD key = (DWORD)(DWORD_PTR)param2;
		DWORD ctrls = (key & KEY_CTRLMASK);
		key&= ~KEY_CTRLMASK;

		if (key == KEY_ENTER)
		{
			std::wstring str;
			GetText(param1, str);

			if ((ctrls & KEY_CTRL) != 0)
			{
				if (coreReturn)
					free(coreReturn);
				coreReturn = _wcsdup(str.c_str());
				Close(CALC_EDIT_ID);
				return TRUE;
			}

			SArg res = parser->Parse(str.c_str(), props.case_sensitive != 0);

			wchar_t *pwz = convertToString(res, CALC_CONV_ENTER, props.result_length, false, props.pad_zeroes != 0, false, NULL);
			if (pwz)
			{
				if (!props.auto_update)
				{
					FarDialogItem Item;
					Item.PtrData = pwz;
					OnEditChangeInternal(param1, (void *)&Item);
				}

				SetText(param1, pwz);
				free(pwz);
			}
			OnGotFocus(param1, NULL);

			return TRUE;
		}
		return -1;
	}

	virtual CALC_INT_PTR OnCtrlColorDlgItem(int Param1, void *Param2)
	{
		uint64_t *ItemColor = reinterpret_cast<uint64_t *>(Param2);
		FarDialogItem item;
		GetDlgItemShort(Param1, &item);
		if (item.Flags & DIF_BOXCOLOR)
		{
			ItemColor[0] = highlightColor;
			ItemColor[2] = highlightColor;
		}
		return 1;
	}
};

void ShowUnitsDialog(int no)
{
	int i, d;
	PDialogData dd = CalcParser::DialogData;
	PDialogElem de, de1;

	cur_dlg_id = no;
	for (i = 0; i < no-1; i++)
		dd = dd->Next;
	de = dd->Elem;

	int dsize = 1+dd->num*2;

	FarDialogItem *dialog = new FarDialogItem [dsize];
	memset(dialog, 0, sizeof(FarDialogItem) * dsize);
	dialog[0].Type = DI_DOUBLEBOX;

	// XXX:
	dialog[0].PtrData = _wcsdup(dd->Name);

	for (d = 1, i = 0, de1 = de; i < dd->num; i++,d++, de1 = de1->Next)
	{
		if (!de1->Type)
		{
			dialog[d].Type = DI_TEXT;
			dialog[d].Flags = DIF_BOXCOLOR;	// used to set highlight colors
			dialog[d].PtrData = _wcsdup(de1->Name);
		} else
		{
			dialog[d].Type = DI_TEXT;
			dialog[d].PtrData = _wcsdup(de1->Name);
			d++;
			dialog[d].Type = DI_EDIT;
		}
	}

	cur_dlg_items = dialog;
	cur_dlg_items_num = dsize;
	SetUnitsDialogDims();

	DlgUnits units(dd);
	units.Init(CALC_DIALOG_UNITS, -1, -1, cur_dlg_dlg_size.X, cur_dlg_dlg_size.Y, L"Contents", dialog, dsize);
	units.Run();

	cur_dlg_items = NULL;

	for (i = 0; i < dsize; i++)
		free((void *)dialog[i].PtrData);
	delete [] dialog;
}

//////////////////////////////////////////////////////////////////////////////////////


static void SetCalcDialogDims(FarDialogItem *dialog)
{
	int sX, sY;

	getConsoleWindowSize(&sX, &sY);

	calc_cx = sX > 80 ? 55 * sX / 80 : 55;
	if (calc_cx > 90)
		calc_cx = 90;

	calc_edit_length = calc_cx - 20 - addons_info.max_len;

	calc_cy = 8 + addons_info.num_custom;

	dialog[0].X2 = calc_cx - 3;
	dialog[0].Y2 = calc_cy - 2;
	dialog[CALC_EDIT_ID].X2 = calc_cx - 6;

	cur_dlg_dlg_size.X = calc_cx;
	cur_dlg_dlg_size.Y = calc_cy;

	int basenum = ARRAYSIZE(dialog_template);

	for (int i = 0; i < cur_dlg_items_numedits; i++)
	{
		dialog[basenum + cur_dlg_items_numedits + i].X1 = 12 + addons_info.max_len;
		dialog[basenum + cur_dlg_items_numedits + i].X2 = calc_cx - 6;
		dialog[basenum + cur_dlg_items_numedits + i].Y1 = basenum + 1 + i;
	}

//	dialog[cur_dlg_items_num - 2].X2 = calc_cx + 1; 		// hint separator
	dialog[cur_dlg_items_num - 1].X2 = calc_cx - 5;		// hint
}

class DlgCalc : public CalcDialog
{
	const wchar_t *_initialExpression;

public:
	DlgCalc(const wchar_t *initialExpression = nullptr)
		: _initialExpression(initialExpression)
	{
	}

	virtual CALC_INT_PTR OnInitDialog(int param1, void *param2)
	{
		if (coreReturn)
		{
			free(coreReturn);
			coreReturn = NULL;
		}
		if (_initialExpression)
		{
			SetText(CALC_EDIT_ID, _initialExpression);
			UpdateEditID();
		}
		return FALSE;
	}

	virtual CALC_INT_PTR OnResizeConsole(int param1, void *param2)
	{
		SetCalcDialogDims(cur_dlg_items);
		EnableRedraw(false);
		ResizeDialog(cur_dlg_dlg_size);

		for (int i = 0; i < cur_dlg_items_num; i++)
		{
			CalcRect rect;
			rect.Left = cur_dlg_items[i].X1;
			rect.Top = cur_dlg_items[i].Y1;
			rect.Right = cur_dlg_items[i].X2;
			rect.Bottom = cur_dlg_items[i].Y2;
			SetItemPosition(i, rect);
		}

		EnableRedraw(true);

		cur_dlg_need_recalc = 1;
		return -1;
	}

	virtual CALC_INT_PTR OnDrawDialog(int param1, void *param2)
	{
		if (cur_dlg_need_recalc)
		{
			cur_dlg_need_recalc = 0;
			std::wstring str;
			GetText(CALC_EDIT_ID, str);
			SetText(CALC_EDIT_ID, str);
		}
		return -1;
	}

	virtual CALC_INT_PTR OnButtonClick(int param1, void *param2)
	{
		curRadio = param1;
		return -1;
	}

	virtual CALC_INT_PTR OnClose(int param1, void *param2)
	{
		std::wstring str;
		GetText(CALC_EDIT_ID, str);
		AddHistory(CALC_EDIT_ID, str);
		CalcRect sr;
		GetDlgRect(&sr);
		//api->SettingsSet(L"calcX", NULL, sr.Left);
		//api->SettingsSet(L"calcY", NULL, sr.Top);
		return -1;
	}

	void UpdateEditID()
	{
		// to update item2
		FarDialogItem item{};
		GetDlgItemShort(CALC_EDIT_ID, &item);
		EditChange(CALC_EDIT_ID, item);
	}

	virtual CALC_INT_PTR OnKey(int param1, void *param2)
	{
		DWORD key = (DWORD)(DWORD_PTR)param2;
		DWORD ctrls = (key & KEY_CTRLMASK);
		key&= ~KEY_CTRLMASK;

		if (key == KEY_F2)
		{
			int i = CalcMenu(0);
			if (i && i != -1) ShowUnitsDialog(i);
			if (coreReturn)
			{
				std::wstring str;
				GetText(CALC_EDIT_ID, str);
				str += coreReturn;
				SetText(CALC_EDIT_ID, str);
				UpdateEditID();
			}
		}

		if (key == KEY_ENTER)
		{
			FarDialogItem *force_update_item = NULL;
			std::wstring str;
			GetText(CALC_EDIT_ID, str);

			if ((ctrls & KEY_CTRL) != 0)
			{
				if (coreReturn)
					free(coreReturn);
				coreReturn = _wcsdup(str.c_str());
				Close(CALC_EDIT_ID);
			}

			AddHistory(CALC_EDIT_ID, str);
			SArg res = parser->Parse(str.c_str(), props.case_sensitive != 0);
			if (!props.auto_update)
			{
				force_update_item = (FarDialogItem *)malloc(sizeof(FarDialogItem) + (str.size() + 1) * sizeof(wchar_t));
				if (force_update_item)
				{
					force_update_item->PtrData = (const wchar_t *)(force_update_item + 1);
					wcscpy((wchar_t *)force_update_item->PtrData, str.c_str());
				}
			}

			if (param1 >= addons_info.radio_id1 && param1 <= addons_info.radio_id2 && curRadio != param1)
			{
				FarDialogItem fdi;
				GetDlgItemShort(curRadio, &fdi);
				fdi.Param.Selected = 0;
				SetDlgItemShort(curRadio, fdi);

				GetDlgItemShort(param1, &fdi);
				fdi.Param.Selected = 1;
				SetDlgItemShort(param1, fdi);

				curRadio = param1;
			}

			int loc_Radio = curRadio;
			if (param1 >= addons_info.radio_id1 && param1 <= addons_info.radio_id2)
				loc_Radio = param1;
			loc_Radio -= addons_info.radio_id1;

			const wchar_t *text = L"";
			wchar_t *free_text = nullptr;
			if (loc_Radio >= 0)
			{
				text = free_text = convertToString(res, loc_Radio,
					props.result_length, true, props.pad_zeroes != 0, false, NULL);
				if (text == NULL)
				{
					loc_Radio = -1;
					text = L"";
				}
			}

			int set_sel = calc_error;
			int text_len = (int)wcslen(text);
			SetText(CALC_EDIT_ID, text);

			if (loc_Radio >= 0)
				free(free_text);

			if (!props.auto_update)
			{
				OnEditChangeInternal(CALC_EDIT_ID, force_update_item, true);
			} else
			{
				UpdateEditID();
			}

			if (set_sel)
			{
				EditorSelect EditSel {};
				EditSel.BlockType = BTYPE_STREAM;
				EditSel.BlockStartLine = -1;
				EditSel.BlockHeight = 1;
				EditSel.BlockStartPos = 0;
				EditSel.BlockWidth = text_len;
				SetSelection(CALC_EDIT_ID, EditSel);
			} else
			{
				CalcCoord cursor;
				cursor.X = 0;
				cursor.Y = 0;
				SetCursorPos(CALC_EDIT_ID, cursor);
				cursor.X = text_len;
				SetCursorPos(CALC_EDIT_ID, cursor);
			}
			SetFocus(CALC_EDIT_ID);
			return TRUE;
		}
		return -1;
	}

	virtual CALC_INT_PTR OnEditChange(int param1, void *param2)
	{
		return OnEditChangeInternal(param1, param2, false);
	}

	CALC_INT_PTR OnEditChangeInternal(int param1, void *param2, bool force_update)
	{
		if (param1 == CALC_EDIT_ID)
		{
			FarDialogItem *Item = (FarDialogItem *)param2;

			if (!props.auto_update && !force_update)
			{
				// do nothing
			}
			else if (Item->PtrData[0] == '\0')
			{
				EnableRedraw(false);
				SetText(CALC_TYPE_ID, L"");
				for (int i = addons_info.edit_id1; i <= addons_info.edit_id2; i++)
					SetText(i, L"");
				EnableRedraw(true);
			} else
			{
				EnableRedraw(false);
				SArg res = parser->Parse(Item->PtrData, props.case_sensitive != 0);

				if (parser->GetError())
				{
					calc_error = 1;

					SetText(CALC_TYPE_ID, api->GetMsg(mNoError + parser->GetError()));
					for (int i = addons_info.edit_id1; i <= addons_info.edit_id2; i++)
						SetText(i, L"");

				} else
				{
					calc_error = 0;

					CalcCoord cursor{};
					cursor.X = 0;
					cursor.Y = 0;
					std::wstring tmp = types[res.gettype()];
					if (res.gettype() == SA_CHAR)
					{
						if ((char)res != 0)
						{
							tmp += L"'";
							tmp += (char)res;
							if ((char)res == '&')
								tmp += '&';
							tmp += L"'";
						}
					}
					tmp += L"        ";
					SetText(CALC_TYPE_ID, tmp);

					for (unsigned i = 0; i < parser->main_addons_num; i++)
					{
						CALC_ERROR error_code = ERR_OK;
						wchar_t *pwz = convertToString(res, i, 0, false, props.pad_zeroes != 0, true, &error_code);
						if (pwz)
						{
							SetText(addons_info.edit_id1 + i, pwz);
							free(pwz);
						}
						else if (error_code != ERR_OK)
						{
							SetText(addons_info.edit_id1 + i, api->GetMsg(mNoError + error_code));
						}
						SetCursorPos(addons_info.edit_id1 + i, cursor);
					}
				}
				EnableRedraw(true);
			}

			if (force_update)
				free(Item);
		}

		return TRUE;
	}

	virtual CALC_INT_PTR OnCtrlColorDlgItem(int Param1, void *Param2)
	{
		uint64_t *ItemColor = reinterpret_cast<uint64_t *>(Param2);
		if (Param1 >= addons_info.edit_id1 && Param1 <= addons_info.edit_id2)
		{
			ItemColor[0] = editColor;
			ItemColor[2] = selColor;

		}
		return 1;
	}
};

// XXX:
int get_visible_len(const wchar_t *str)
{
	int len = 0;
	for (int k = 0; str[k]; k++)
	{
		if (str[k] == '&' && str[k+1] == '&')
		{
			len++;
			k++;
		}
		else if (str[k] != '&')
			len++;
	}
	return len;
}

// XXX:
void CalcShowDialog(const wchar_t *expression)
{
//	FarDialogItem *dialog;

//	unsigned basenum = sizeof(dialog_template) / sizeof(dialog_template[0]);
//	unsigned totalnum = basenum;
	unsigned i;

	addons_info.num_custom = 0;
	addons_info.max_len = 0;

	for (addons_info.num_custom = 0; addons_info.num_custom < parser->main_addons_num; addons_info.num_custom++)
	{
		int len = get_visible_len(parser->addons[addons_info.num_custom].name.c_str());
		if (len > addons_info.max_len)
			addons_info.max_len = len;
	}

//	totalnum = basenum + addons_info.num_custom * 2;
//	dialog = new FarDialogItem [totalnum];
	std::vector<FarDialogItem> dialog;

	for (const auto &templated_item : dialog_template) {
		dialog.emplace_back(templated_item);
	}
	//size_t basenum = dialog.size();
		//dialog[i] = dialog_template[i];
	dialog[0].PtrData = _wcsdup(api->GetMsg(mName));

	if (!props.autocomplete)
		dialog[2].Flags |= DIF_NOAUTOCOMPLETE;

	addons_info.radio_id1 = (int)dialog.size();
	addons_info.radio_id2 = addons_info.radio_id1 + addons_info.num_custom - 1;
	addons_info.edit_id1 = addons_info.radio_id2 + 1;
	addons_info.edit_id2 = addons_info.edit_id1 + addons_info.num_custom - 1;

	// add radio-buttons
	for (i = 0; i < addons_info.num_custom; i++)
	{
		FarDialogItem it {};
		it.Type = DI_RADIOBUTTON;
		it.X1 = 6;
		it.Y1 = DIALOG_BASE_LINE + i;

		const wchar_t *from = parser->addons[i].name.c_str();
		wchar_t *tmp = (wchar_t *)malloc((wcslen(from) + addons_info.max_len + 2) * sizeof(wchar_t));
		if (tmp)
		{
			int pad_len = addons_info.max_len - get_visible_len(from);

			wmemset(tmp, ' ', pad_len);

			wcscpy(tmp + pad_len, from);
			wcscat(tmp, L":");
		}

		it.PtrData = tmp;

		it.Flags = (i == 0) ? DIF_GROUP : 0;
		it.Param.Selected = (curRadio == (int)dialog.size()) ? 1 : 0; //int(i + addons_info.radio_id1)

		dialog.emplace_back(it);
	}

	// add edit-boxes
	for (i = 0; i < addons_info.num_custom; i++)
	{
		FarDialogItem it {};
		it.Type = DI_EDIT;
		it.PtrData = L"";
		it.Flags = DIF_READONLY;//DIF_DISABLE;

//		dialog[basenum + addons_info.num_custom + i] = it;
		dialog.emplace_back(it);
	}

	// add hint
	FarDialogItem it {};
	it.Type = DI_TEXT;
	it.X1 = 5;
	it.Y1 = DIALOG_BASE_LINE + addons_info.num_custom;
//	it.PtrData = L"";
//	it.Flags = DIF_SEPARATOR;
//	dialog.emplace_back(it);
//	it.Y1++;
	it.Flags = DIF_CENTERTEXT | DIF_DISABLE;
	it.PtrData = api->GetMsg(mBottomHint);
	dialog.emplace_back(it);

	cur_dlg_items = &dialog[0];
	cur_dlg_items_num = (int)dialog.size();
	cur_dlg_items_numedits = addons_info.num_custom;
	SetCalcDialogDims(&dialog[0]);

//	if (expression)
//	{
//		dialog[2].PtrData = expression;
//	}
	DlgCalc calc(expression);
	calc.Init(CALC_DIALOG_MAIN, -1, -1, cur_dlg_dlg_size.X, cur_dlg_dlg_size.Y,
				L"Contents", &dialog[0], (int)dialog.size());

	calc.Run();

	free((void *)dialog[0].PtrData);

	for (i = 0; i < addons_info.num_custom; i++)
		free((void *)dialog[addons_info.radio_id1 + i].PtrData);
}

SDialogElem::SDialogElem()
{
	Next = 0;
	addon_idx = -1;
	input = NULL;
	scale_expr = NULL;
}

SDialogElem::~SDialogElem()
{
	if (Next)
		delete Next;
	if (input)
		delete input;
	if (scale_expr)
		delete scale_expr;
}

SDialogData::SDialogData()
{
	Next = 0;
	Elem = 0;
}

SDialogData::~SDialogData()
{
	delete Elem;
	if (Next) delete Next;
}
