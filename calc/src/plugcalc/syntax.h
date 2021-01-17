//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#ifndef SYNTAX_H
#define SYNTAX_H

#ifdef USE_CREGEXP
#include <regexp/cregexp.h>
#else
#include <trex/trex.h>
#endif

enum CALC_ADDON_FLAGS
{
	CALC_ADDON_FLAG_NONE = 0,
	CALC_ADDON_FLAG_DELIM = 1,
	CALC_ADDON_FLAG_DIALOG = 2,
};

enum CALC_RADIX
{
	CALC_RADIX_EXPONENTIAL = -10,
	CALC_RADIX_REPEATING = -110,
	CALC_RADIX_CONTINUED = -210,
};

struct SSyntax
{
	wchar_t *name;
  	wchar_t *name_set;	// XXX:
  	wchar_t *mean;

#ifdef USE_CREGEXP	
	CRegExp *re;
#else
	TRex *re;
#endif
	
	union
	{
		int priority;
		int radix;
	};

	int flags;

  	SSyntax *next;
  
  	SSyntax();
  	~SSyntax();
};

typedef struct SSyntax *PSyntax;

struct SVars:SSyntax
{
  	SArg value;
  	SVars();
  	~SVars();
};

typedef struct SVars *PVars;

#endif // of SYNTAX_H





















