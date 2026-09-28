//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#ifndef _CALC_H_
#define _CALC_H_

enum
{
	CALC_DIALOG_MAIN,
	CALC_DIALOG_UNITS,
	CALC_DIALOG_CONFIG,
};

void CalcStartup();
void CalcExit();
void CalcOpenFromEditor();
void CalcOpen(const wchar_t *expression = nullptr);
bool CalcConfig();

void InitDynamicData();
void DeInitDynamicData();

void EditorDialog();
void ShellDialog(const wchar_t *expression = nullptr);
bool ConfigDialog();
int  CalcMenu(int c);
void ShowDialog(int no);
void CalcShowDialog(const wchar_t *expression = nullptr);

void SetActive(int Act);

#endif
