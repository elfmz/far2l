//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#ifndef _CALC_CONFIG_H_
#define _CALC_CONFIG_H_

// configuration

BOOL InitConfig();
BOOL LoadConfig();
BOOL CheckConfig();
BOOL SaveConfig();

struct CalcProperties
{
	int auto_update;
	int case_sensitive;
	int max_period;
	int pad_zeroes;
	int right_align;

	int history_hide;
	int history_above;
	int history_below;
	int history_lines;
	int autocomplete;

	int use_regional, use_delim;
	wchar_t decimal_point[2];
	wchar_t args[2];
	wchar_t digit_delim[2];

	int result_length;
	int rep_fraction_max_start;
	int rep_fraction_max_period;
	int cont_fraction_max;
};

extern CalcProperties props;

#endif // of _CALC_CONFIG_H_
