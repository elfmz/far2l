//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include <algorithm>

#include <sgml/sgml.h>
#include <utils.h>
#include "api.h"
#include "newparse.h"
#include "calc.h"
#include "config.h"
#include "messages.h"

CalcProperties props;

enum CALC_PARAM_CHECK_TYPE
{
	CALC_PARAM_CHECK_BOOL,
	CALC_PARAM_CHECK_RADIO,
	CALC_PARAM_CHECK_LINES,
	CALC_PARAM_CHECK_PERIOD,
	CALC_PARAM_CHECK_DECIMAL,
	CALC_PARAM_CHECK_ARGS,
	CALC_PARAM_CHECK_DELIM,
	CALC_PARAM_CHECK_LENGTH,
	CALC_PARAM_CHECK_FRAC,
};

static const struct
{
	int *ival;
	wchar_t *sval;	// if sval != NULL then (int)ival = max strlen

	const char *reg_name;
	const wchar_t *def_value;

	CALC_PARAM_CHECK_TYPE check_type;
} param_table[] =
{
	{ &props.auto_update, NULL, "autoUpdate", L"1", CALC_PARAM_CHECK_BOOL },
	{ &props.case_sensitive, NULL, "caseSensitive", L"0", CALC_PARAM_CHECK_BOOL },
	{ &props.pad_zeroes, NULL, "padZeroes", L"1", CALC_PARAM_CHECK_BOOL },
	/*{ &props.right_align, NULL, L"rightAlign", L"0", CALC_PARAM_CHECK_BOOL },*/

	{ &props.history_hide, NULL, "historyHide", L"1", CALC_PARAM_CHECK_RADIO },
	{ &props.history_above, NULL, "historyAbove", L"0", CALC_PARAM_CHECK_RADIO },
	{ &props.history_below, NULL, "historyBelow", L"0", CALC_PARAM_CHECK_RADIO },
	{ &props.history_lines, NULL, "historyLines", L"8", CALC_PARAM_CHECK_LINES },
	{ &props.autocomplete, NULL, "autoComplete", L"0", CALC_PARAM_CHECK_BOOL },

	{ &props.use_regional, NULL, "useRegional", L"0", CALC_PARAM_CHECK_BOOL },
	{ (int *)sizeof(props.decimal_point), props.decimal_point, "decimalPoint", L".", CALC_PARAM_CHECK_DECIMAL },
	{ (int *)sizeof(props.args), props.args, "Args", L",", CALC_PARAM_CHECK_ARGS },
	{ &props.use_delim, NULL, "useDelim", L"0", CALC_PARAM_CHECK_BOOL },
	{ (int *)sizeof(props.digit_delim), props.digit_delim, "digitDelim", L"'", CALC_PARAM_CHECK_DELIM },

	// registry-only
	{ &props.max_period, NULL, "maxPeriod", L"10", CALC_PARAM_CHECK_PERIOD },
	{ &props.result_length, NULL, "resultLength", L"128", CALC_PARAM_CHECK_LENGTH },
	{ &props.rep_fraction_max_start, NULL, "repFractionMaxStart", L"32", CALC_PARAM_CHECK_FRAC },
	{ &props.rep_fraction_max_period, NULL, "repFractionMaxPeriod", L"128", CALC_PARAM_CHECK_FRAC },
	{ &props.cont_fraction_max, NULL, "contFractionMax", L"64", CALC_PARAM_CHECK_FRAC },

	{ NULL, NULL, "", 0, CALC_PARAM_CHECK_BOOL },
};

static const struct
{
	DialogItemTypes Type;
	int				    X1, Y1, X2, Y2;		// if Y1 == 0, then Y1=last_Y1;  if Y1 == -1, then Y1=last_Y1+1
	unsigned long long  Flags;
	int				    name_ID;
	int				    *prop_ivalue;
	wchar_t			    *prop_svalue;
} dlgItems[] =
{
	{ DI_DOUBLEBOX,    3,  1, 54, 14, 0, mConfigName, NULL, NULL },
	{ DI_CHECKBOX,     5, -1,  0,  0, DIF_FOCUS, mConfigShowResults, &props.auto_update, NULL },
	{ DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigCaseSensitive, &props.case_sensitive, NULL },

	{ DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigPadZeroes, &props.pad_zeroes, NULL },
#if 0
	{ DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigRightAligned, &props.right_align, NULL },

	{ DI_TEXT,        41,  0,  0,  0, DIF_DISABLE, mConfigMaxPeriod, NULL, NULL },
	{ DI_FIXEDIT,     53,  0, 55,  0, DIF_DISABLE, 0, &props.max_period, NULL },
#endif

	{ DI_TEXT,         5, -1,  0,  0, DIF_BOXCOLOR|DIF_SEPARATOR, mConfigHistory, NULL, NULL },

	{ DI_RADIOBUTTON,  5, -1,  0,  0, DIF_GROUP, mConfigHide, &props.history_hide, NULL },
	{ DI_RADIOBUTTON, 19,  0,  0,  0, DIF_DISABLE, mConfigShowAbove, &props.history_above, NULL },
	{ DI_RADIOBUTTON, 37,  0,  0,  0, DIF_DISABLE, mConfigShowBelow, &props.history_below, NULL },

	{ DI_FIXEDIT,      5, -1,  7,  0, DIF_DISABLE, 0, &props.history_lines, NULL },
	{ DI_TEXT,         9,  0,  0,  0, DIF_DISABLE, mConfigLines, NULL, NULL },
	{ DI_CHECKBOX,    19,  0,  0,  0, 0, mConfigAutocomplete, &props.autocomplete, NULL },

	{ DI_TEXT,         5, -1,  0,  0, DIF_BOXCOLOR|DIF_SEPARATOR, mConfigDelimiters, NULL, NULL },

	{ DI_TEXT,         5, -1,  0,  0, 0, mConfigDecimalSymbol, NULL, NULL },
	{ DI_FIXEDIT,     22,  0, 22,  0 , 0, 0, (int *)sizeof(props.decimal_point), props.decimal_point },
	{ DI_CHECKBOX,    25,  0,  0,  0, 0, mConfigDigitDelimiter, &props.use_delim, NULL },
	{ DI_FIXEDIT,     51,  0, 51,  0, 0, 0, (int *)sizeof(props.digit_delim), props.digit_delim },

	{ DI_TEXT,         5, -1,  0,  0, 0, mConfigArguments, NULL, NULL },
	{ DI_FIXEDIT,     22,  0, 22,  0, 0, 0, (int *)sizeof(props.args), props.args },
	{ DI_CHECKBOX,    25,  0,  0,  0, 0, mConfigUseRegional, &props.use_regional, NULL },

	{ DI_TEXT,         5, -1,  0,  0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, NULL, NULL },

	{ DI_BUTTON,       0, -1,  0,  0, DIF_CENTERGROUP | DIF_DEFAULT, mOk, NULL, NULL },
	{ DI_BUTTON,       0,  0,  0,  0, DIF_CENTERGROUP, mCancel, NULL, NULL },
	{ DI_BUTTON,       0,  0,  0,  0, DIF_CENTERGROUP|DIF_BTNNOCLOSE|DIF_DISABLE, mAdvanced, NULL, NULL },
};


BOOL LoadConfig()
{
	api->SettingsBegin();
	for (int i = 0; param_table[i].ival; i++)
	{
		if (param_table[i].sval)
		{
			std::wstring str;
			if (api->SettingsGet(param_table[i].reg_name, &str, NULL))
				wcsncpy(param_table[i].sval, str.c_str(), (intptr_t)param_table[i].ival / sizeof(wchar_t));
			else
				wcsncpy(param_table[i].sval, param_table[i].def_value, (intptr_t)param_table[i].ival / sizeof(wchar_t));
		} else
		{
			if (!api->SettingsGet(param_table[i].reg_name, NULL, param_table[i].ival))
				*param_table[i].ival = _wtoi(param_table[i].def_value);
		}
	}
	api->SettingsEnd();
	return TRUE;
}

BOOL SaveConfig()
{
	api->SettingsBegin();
	for (int i = 0; param_table[i].ival; i++)
	{
		if (param_table[i].sval)
		{
			std::wstring str = param_table[i].sval;
			api->SettingsSet(param_table[i].reg_name, &str, NULL);
		}
		else
		{
			api->SettingsSet(param_table[i].reg_name, NULL, param_table[i].ival);
		}
	}
	api->SettingsEnd();
	return TRUE;
}

BOOL CheckConfig()
{
	wchar_t decimal = 0, args = 0;
	for (int i = 0; param_table[i].ival; i++)
	{
		int *v = param_table[i].ival;
		wchar_t *s = param_table[i].sval;
		switch (param_table[i].check_type)
		{
		case CALC_PARAM_CHECK_BOOL:
			*v = (*v != 0) ? 1 : 0;
			break;
		case CALC_PARAM_CHECK_RADIO:
			*v = std::min(std::max(*v, 0), 2);
			break;
		case CALC_PARAM_CHECK_LINES:
			*v = std::min(std::max(*v, 0), 10);
			break;
		case CALC_PARAM_CHECK_PERIOD:
			*v = std::min(std::max(*v, 0), 100);
			break;
		case CALC_PARAM_CHECK_LENGTH:
			*v = std::min(std::max(*v, 16), 512);
			break;
		case CALC_PARAM_CHECK_FRAC:
			*v = std::min(std::max(*v, 16), 512);
			break;
		case CALC_PARAM_CHECK_DECIMAL:
//			if (props.use_regional)
//			{
//				GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_SDECIMAL, s, 2);
//			}
			if (wcschr(L".,", s[0]) == NULL)
				s[0] = '.';
			decimal = s[0];
			s[1] = '\0';
			break;
		case CALC_PARAM_CHECK_ARGS:
			if (s[0] == '\0' || wcschr(L",;.:", s[0]) == NULL)
				s[0] = ',';
			if (s[0] == decimal)
				s[0] = (decimal == L',') ? ';' : ',';
			args = s[0];
			s[1] = '\0';
			break;
		case CALC_PARAM_CHECK_DELIM:
//			if (props.use_regional)
//			{
//				GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_SMONTHOUSANDSEP , s, 2);
//			}
			if (wcschr(L" ,'`\xa0", s[0]) == NULL)
				s[0] = ' ';
			if (s[0] == decimal || s[0] == args)
				s[0] = ' ';

			s[1] = '\0';
			break;
		}
	}
	return TRUE;
}

class DlgConfig : public CalcDialog
{

};

bool ConfigDialog()
{
	const int num = sizeof(dlgItems)/sizeof(dlgItems[0]);
	wchar_t tmpnum[num][32];

	int ExitCode, ok_id = -2;

	FarDialogItem *dlg = new FarDialogItem[num];
	if (!dlg)
		return false;
	memset(dlg, 0, num * sizeof(FarDialogItem));
	int Y1 = 0;
	for (int i = 0; i < num; i++)
	{
		dlg[i].Type = dlgItems[i].Type;	dlg[i].Flags = dlgItems[i].Flags;
		dlg[i].X1 = dlgItems[i].X1;		dlg[i].X2 = dlgItems[i].X2;

		dlg[i].Y1 = (dlgItems[i].Y1 == -1) ? (Y1+1) : ((dlgItems[i].Y1 == 0) ? Y1 : dlgItems[i].Y1);
		dlg[i].Y2 = (dlgItems[i].Type == DI_FIXEDIT) ? dlg[i].Y1 : dlgItems[i].Y2;

		Y1 = dlg[i].Y1;

		if (dlgItems[i].name_ID == mOk)
			ok_id = i;

		if (dlgItems[i].Type == DI_FIXEDIT)
		{
			if (dlgItems[i].prop_svalue)
				dlg[i].PtrData = dlgItems[i].prop_svalue;
			else if (dlgItems[i].prop_ivalue)
			{
				_itow(*dlgItems[i].prop_ivalue, tmpnum[i], 10);
				dlg[i].PtrData = tmpnum[i];
			}
		}
		else
		{
			if (dlgItems[i].Type == DI_BUTTON && (dlgItems[i].Flags & DIF_DEFAULT))
				dlg[i].DefaultButton = 1;
			dlg[i].PtrData = dlgItems[i].name_ID ? api->GetMsg(dlgItems[i].name_ID) : L"";
			if (dlgItems[i].prop_ivalue && !dlgItems[i].prop_svalue)
				dlg[i].Param.Selected = *dlgItems[i].prop_ivalue;
		}
	}

	dlg[0].Y2 = Y1 + 1;

	DlgConfig cfg;
	if (!cfg.Init(CALC_DIALOG_CONFIG, -1, -1, 58, 16, L"Config", dlg, num))
		return false;

	ExitCode = (int)cfg.Run();

	if (ExitCode == ok_id)
	{
		for (int i = 0; i < num; i++)
		{
			std::wstring str;
			if (dlgItems[i].prop_svalue)
			{
				cfg.GetText(i, str);
				wcsncpy(dlgItems[i].prop_svalue, str.c_str(), (intptr_t)dlgItems[i].prop_ivalue / sizeof(wchar_t));
			}
			else if (dlgItems[i].prop_ivalue)
			{
				if (dlgItems[i].Type == DI_FIXEDIT)
				{
					cfg.GetText(i, str);
					*dlgItems[i].prop_ivalue = _wtoi(str.c_str());
				}
				else
				{
					*dlgItems[i].prop_ivalue = cfg.IsChecked(i) ? 1 : 0;
				}
			}
		}
	}

	delete [] dlg;

	return true;
}

