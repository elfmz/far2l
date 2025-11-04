/***********************************************************************
  This file is part of HA, a general purpose file archiver.
  Copyright (C) 1995 Harri Hirvola

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
************************************************************************
	HA arithmetic coder
***********************************************************************/

/***********************************************************************
This file contains some small changes made by Nico de Vries (AIP-NL)
allowing it to be compiled with Borland C++ 3.1.
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include "ha.h"
#include "haio.h"
#include "acoder.h"

static U16B h,l,v;
static S16B gpat;

/***********************************************************************
  Bit I/O
***********************************************************************/

#define getbit(b) 	{ gpat<<=1;				\
			  if (!(gpat&0xff)) {			\
				gpat=getbyte();			\
				if (gpat&0x100) gpat=0x100;	\
				else {				\
					gpat<<=1;		\
					gpat|=1;		\
				}				\
			  }					\
			  b|=(gpat&0x100)>>8;			\
			}

/***********************************************************************
  Arithmetic decoding
***********************************************************************/
void ac_in(U16B low, U16B high, U16B tot) {

    register U32B r;

    r=(U32B)(h-l)+1;
    h=(U16B)(r*high/tot-1)+l;
    l+=(U16B)(r*low/tot);
    while (!((h^l)&0x8000)) {
	l<<=1;
	h<<=1;
	h|=1;
	v<<=1;
	getbit(v);
    }
    while ((l&0x4000)&&!(h&0x4000)) {
	l<<=1;
	l&=0x7fff;
	h<<=1;
	h|=0x8001;
	v<<=1;
	v^=0x8000;
	getbit(v);
    }
}

U16B ac_threshold_val(U16B tot) {

    register U32B r;

    r=(U32B)(h-l)+1;
    return (U16B)((((U32B)(v-l)+1)*tot-1)/r);
}


void ac_init_decode(void) {

    h=0xffff;
    l=0;
    gpat=0;
    v=getbyte()<<8;
    v|=0xff&getbyte();
}

