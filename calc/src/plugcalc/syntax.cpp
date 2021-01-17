//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "newparse.h"
#include "sarg.h"
#include "syntax.h"


SSyntax::SSyntax()
{
	next = NULL;
	name_set = NULL;
	name = mean = NULL;
	priority = 0;
	flags = 0;

	re = NULL;
}

// XXX:
SSyntax::~SSyntax()
{
	if (next) delete next;
	if (name) delete [] name;
	if (name_set) delete [] name_set;
	if (mean) delete [] mean;
#ifdef USE_CREGEXP
	if (re) delete re;
#else
	if (re) trex_free(re);
#endif
}

SVars::SVars()
{ 
}

SVars::~SVars()
{ 
}

