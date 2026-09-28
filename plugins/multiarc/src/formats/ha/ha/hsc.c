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
	HA HSC method
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "ha.h"
#include "haio.h"
#include "acoder.h"
#include "hsc.h"
#include "error.h"

#define IECLIM		32	       /* initial escape counter upper limit */
#define NECLIM		5	       /* no escape expected counter limit */
#define NECTLIM		4	       /* */
#define NECMAX		10	       /* no escape expected counter maximum */
#define MAXCLEN		4	       /* assumed to be 4 in several places */
#define NUMCON		10000	       /* number of contexts to remember */
#define NUMCFB		32760	       /* number of frequencies to remember */
#define ESCTH		3	       /* threshold for escape calculation */
#define MAXTVAL		8000	       /* maximum frequency value */
#define RFMINI		4	       /* initial refresh counter value */
#define HTLEN	        16384	       /* length of hash table */
#define NIL		0xffff	       /* NIL pointer in lists */
#define ESC		256	       /* escape symbol */

typedef unsigned char Context[4];

/* model data */
static Context curcon;		      /* current context */
static U16B *ht=NULL;		      /* hash table */
static U16B *hp=NULL;		      /* hash list pointer array */
static Context *con=NULL;	      /* context array */
static unsigned char *cl=NULL;	      /* context length array */
static unsigned char *cc=NULL;	      /* character counts */
static U16B *ft=NULL;		      /* total frequency of context */
static unsigned char *fe=NULL;	      /* frequencys under ESCTH in context */
static U16B *elp=NULL;		      /* expire list previous pointer array */
static U16B *eln=NULL;		      /* expire list next pointer array */
static U16B elf,ell;		      /* first and last of expire list */
static unsigned char *rfm=NULL;	      /* refresh counter array */
static U16B *fa=NULL;		      /* frequency array */
static unsigned char *fc=NULL;	      /* character for frequency array */
static U16B *nb=NULL;		      /* next pointer for frequency array */
static U16B fcfbl;		      /* pointer to free frequency blocks */
static U16B nrel;		      /* context for frequency block release */

/* frequency mask system */
static unsigned char cmask[256];      /* masked characters */
static unsigned char cmstack[256];    /* stack of cmask[] entries to clear */
static S16B cmsp;		      /* pointer to cmstack */

/* escape propability modifying system variables */
static unsigned char nec;	      /* counter for no escape expected */
static unsigned char iec[MAXCLEN+1];  /* initial escape counters */

/* update stack variables */
static U16B usp;		      /* stack pointer */
static U16B cps[MAXCLEN+1]; 	      /* context pointers */
static U16B as[MAXCLEN+1];	      /* indexes to frequency array */

/* miscalneous */
static S16B dropcnt;		      /* counter for context len drop */
static unsigned char maxclen;	      /* current maximum length for context */
static U16B hrt[HTLEN];		      /* semi random data for hashing */
static U16B hs[MAXCLEN+1]; 	      /* hash stack for context search */
static S16B cslen;		      /* length of context to search */

/***********************************************************************
	Cleanup routine
***********************************************************************/

void hsc_cleanup(void) {

    if (ht!=NULL) free(ht),ht=NULL;
    if (fc!=NULL) free(fc),fc=NULL;
    if (fa!=NULL) free(fa),fa=NULL;
    if (ft!=NULL) free(ft),ft=NULL;
    if (fe!=NULL) free(fe),fe=NULL;
    if (nb!=NULL) free(nb),nb=NULL;
    if (hp!=NULL) free(hp),hp=NULL;
    if (elp!=NULL) free(elp),elp=NULL;
    if (eln!=NULL) free(eln),eln=NULL;
    if (cl!=NULL) free(cl),cl=NULL;
    if (cc!=NULL) free(cc),cc=NULL;
    if (rfm!=NULL) free(rfm),rfm=NULL;
    if (con!=NULL) free(con),con=NULL;
}


/***********************************************************************
	System initialization
***********************************************************************/

static  U16B make_context(unsigned char cl, S16B c);

static void init_model(void) {

    register S16B i;
    S32B z,l,h,t;

    ht=malloc(HTLEN*sizeof(*ht));
    hp=malloc(NUMCON*sizeof(*hp));
    elp=malloc(NUMCON*sizeof(*elp));
    eln=malloc(NUMCON*sizeof(*eln));
    cl=malloc(NUMCON*sizeof(*cl));
    cc=malloc(NUMCON*sizeof(*cc));
    ft=malloc(NUMCON*sizeof(*ft));
    fe=malloc(NUMCON*sizeof(*fe));
    rfm=malloc(NUMCON*sizeof(*rfm));
    con=malloc(NUMCON*sizeof(*con));
    fc=malloc(NUMCFB*sizeof(*fc));
    fa=malloc(NUMCFB*sizeof(*fa));
    nb=malloc(NUMCFB*sizeof(*nb));
    if (hp==NULL || elp==NULL || eln==NULL ||
	cl==NULL || rfm==NULL || con==NULL ||
	cc==NULL || ft==NULL || fe==NULL ||
	fc==NULL || fa==NULL || nb==NULL || ht==NULL) {
	hsc_cleanup();
	error(1,ERR_MEM,"init_model()");
    }
    maxclen=MAXCLEN;
    iec[0]=(IECLIM>>1);
    for (i=1;i<=MAXCLEN;++i) iec[i]=(IECLIM>>1)-1;
    dropcnt=NUMCON/4;
    nec=0;
    nrel=0;
    hs[0]=0;
    for (i=0;i<HTLEN;++i) ht[i]=NIL;
    for (i=0;i<NUMCON;++i) {
	eln[i]=i+1;
	elp[i]=i-1;
	cl[i]=0xff;
	nb[i]=NIL;
    }
    elf=0;
    ell=NUMCON-1;
    for (i=NUMCON;i<NUMCFB-1;++i) nb[i]=i+1;
    nb[i]=NIL;
    fcfbl=NUMCON;
    curcon[3]=curcon[2]=curcon[1]=curcon[0]=0;
    cmsp=0;
    for (i=0;i<256;++i) cmask[i]=0;
    for (z=10,i=0;i<HTLEN;++i) {
	h=z/(2147483647L/16807L);
	l=z%(2147483647L/16807L);
	if ((t=16807L*l-(2147483647L%16807L)*h)>0) z=t;
	else z=t+2147483647L;
	hrt[i]=(U16B)z&(HTLEN-1);
    }
}

static void init_unpack(void) {

    init_model();
    ac_init_decode();
}


/***********************************************************************
	Finite context model
***********************************************************************/

#define HASH(s,l,h)	{				          \
			  h=0;                                    \
			  if (l) h=hrt[s[0]];                     \
			  if (l>1) h=hrt[(s[1]+h)&(HTLEN-1)];     \
			  if (l>2) h=hrt[(s[2]+h)&(HTLEN-1)];     \
			  if (l>3) h=hrt[(s[3]+h)&(HTLEN-1)];     \
			}

#define move_context(c) curcon[3]=curcon[2],curcon[2]=curcon[1], \
			curcon[1]=curcon[0],curcon[0]=c

static  void release_cfblocks(void) {

    register U16B i,j,d;

    do {
	do if (++nrel==NUMCON) nrel=0; while (nb[nrel]==NIL);
	for (i=0;i<=usp;++i) if ((cps[i]&0x7fff)==nrel) break;
    } while (i<=usp);
    for (i=nb[nrel],d=fa[nrel];i!=NIL;i=nb[i]) if (fa[i]<d) d=fa[i];
    ++d;
    if (fa[nrel]<d) {
	for (i=nb[nrel];fa[i]<d && nb[i]!=NIL;i=nb[i]);
	fa[nrel]=fa[i];
	fc[nrel]=fc[i];
	j=nb[i];
	nb[i]=fcfbl;
	fcfbl=nb[nrel];
	if ((nb[nrel]=j)==NIL) {
	    cc[nrel]=0;
	    fe[nrel]=(ft[nrel]=fa[nrel])<ESCTH?1:0;
	    return;
	}
    }
    fe[nrel]=(ft[nrel]=fa[nrel]/=d)<ESCTH?1:0;
    cc[nrel]=0;
    for (j=nrel,i=nb[j];i!=NIL;) {
	if (fa[i]<d) {
	    nb[j]=nb[i];
	    nb[i]=fcfbl;
	    fcfbl=i;
	    i=nb[j];
	}
	else {
	    ++cc[nrel];
	    ft[nrel]+=fa[i]/=d;
	    if (fa[i]<ESCTH) fe[nrel]++;
	    j=i;
	    i=nb[i];
	}
    }
}

static  U16B make_context(unsigned char conlen, S16B c) {

    register S16B i;
    register U16B nc,fp;

    nc=ell;
    ell=elp[nc];
    elp[elf]=nc;
    eln[nc]=elf;
    elf=nc;
    if (cl[nc]!=0xff) {
	if (cl[nc]==MAXCLEN && --dropcnt==0) maxclen=MAXCLEN-1;
	HASH(con[nc],cl[nc],i);
	if (ht[i]==nc) ht[i]=hp[nc];
	else {
	    for (i=ht[i];hp[i]!=nc;i=hp[i]);
	    hp[i]=hp[nc];
	}
	if (nb[nc]!=NIL) {
	    for (fp=nb[nc];nb[fp]!=NIL;fp=nb[fp]);
	    nb[fp]=fcfbl;
	    fcfbl=nb[nc];
	}
    }
    nb[nc]=NIL;
    fe[nc]=ft[nc]=fa[nc]=1;
    fc[nc]=c;
    rfm[nc]=RFMINI;
    cc[nc]=0;
    cl[nc]=conlen;
    con[nc][0]=curcon[0];
    con[nc][1]=curcon[1];
    con[nc][2]=curcon[2];
    con[nc][3]=curcon[3];
    HASH(curcon,conlen,i);
    hp[nc]=ht[i];
    ht[i]=nc;
    return nc;
}

static  void el_movefront(U16B cp) {

    if (cp==elf) return;
    if (cp==ell) ell=elp[cp];
    else {
	elp[eln[cp]]=elp[cp];
	eln[elp[cp]]=eln[cp];
    }
    elp[elf]=cp;
    eln[cp]=elf;
    elf=cp;
}

static void  add_model(S16B c) {

    register U16B i;
    register S16B cp;

    while (usp!=0) {
	i=as[--usp];
	cp=cps[usp];
	if (cp&0x8000) {
	    cp&=0x7fff;
	    if (fcfbl==NIL) release_cfblocks();
	    nb[i]=fcfbl;
	    i=nb[i];
	    fcfbl=nb[fcfbl];
	    nb[i]=NIL;
	    fa[i]=1;
	    fc[i]=c;
	    ++cc[cp];
	    ++fe[cp];
	}
	else if (++fa[i]==ESCTH) --fe[cp];
	if ((fa[i]<<1)<++ft[cp]/(cc[cp]+1)) --rfm[cp];
	else if (rfm[cp]<RFMINI) ++rfm[cp];
	if (!rfm[cp] || ft[cp]>=MAXTVAL) {
	    ++rfm[cp];
	    fe[cp]=ft[cp]=0;
	    for (i=cp;i!=NIL;i=nb[i]) {
		if (fa[i]>1) {
		    ft[cp]+=fa[i]>>=1;
		    if (fa[i]<ESCTH) ++fe[cp];
		}
		else {
		    ++ft[cp];
		    ++fe[cp];
		}
	    }
	}
    }
}

static  U16B find_next(void) {

    register S16B i,k;
    register U16B cp;

    for (i=cslen-1;i>=0;--i) {
	k=hs[i];
	for (cp=ht[k];cp!=NIL;cp=hp[cp]) {
	    if (cl[cp]==i) {
		switch (i) {
		  case 4:
		    if (curcon[3]!=con[cp][3]) break;
		  case 3:
		    if (curcon[2]!=con[cp][2]) break;
		  case 2:
		    if (curcon[1]!=con[cp][1]) break;
		  case 1:
		    if (curcon[0]!=con[cp][0]) break;
		  case 0:
		    cslen=i;
		    return cp;
		}
	    }
	}
    }
    return NIL;
}

static  U16B find_longest(void) {

    hs[1]=hrt[curcon[0]];
    hs[2]=hrt[(curcon[1]+hs[1])&(HTLEN-1)];
    hs[3]=hrt[(curcon[2]+hs[2])&(HTLEN-1)];
    hs[4]=hrt[(curcon[3]+hs[3])&(HTLEN-1)];
    usp=0;
    while(cmsp) cmask[cmstack[--cmsp]]=0;
    cslen=MAXCLEN+1;
    return find_next();
}

static U16B adj_escape_prob(U16B esc, U16B cp) {

    if (ft[cp]==1) return iec[cl[cp]]>=(IECLIM>>1)?2:1;
    if (cc[cp]==255) return 1;
    if (cc[cp] && ((cc[cp]+1)<<1)>=ft[cp]) {
	esc=(S16B)((S32B)esc*((cc[cp]+1)<<1)/ft[cp]);
	if (cc[cp]+1==ft[cp]) esc+=(cc[cp]+1)>>1;
    }
    return esc?esc:1;
}

static  S16B decode_first(U16B cp) {

    register U16B c;
    register U16B tv;
    register U16B i;
    register S16B sum,tot,esc,cf;
    register unsigned char sv;

    esc=adj_escape_prob(fe[cp],cp);
    tot=ft[cp];
    if (nec>=NECLIM) {
	if (tot<=NECTLIM && nec==NECMAX) sv=2;
	else sv=1;
	tot<<=sv;
	tv=ac_threshold_val(tot+esc)>>sv;
	for (c=cp,sum=0;;c=nb[c]) {
	    if (c==NIL) break;
	    if (sum+fa[c]<=tv) sum+=fa[c];
	    else {
		cf=fa[c]<<sv;
		break;
	    }
	}
	sum<<=sv;
    }
    else {
	tv=ac_threshold_val(tot+esc);
	for (c=cp,sum=0;;c=nb[c]) {
	    if (c==NIL) break;
	    if (sum+fa[c]<=tv) sum+=fa[c];
	    else {
		cf=fa[c];
		break;
	    }
	}
    }
    usp=1;
    if (c!=NIL) {
	ac_in(sum,sum+cf,tot+esc);
	if (ft[cp]==1 && iec[cl[cp]]) --iec[cl[cp]];
	as[0]=c;
	cps[0]=cp;
	c=fc[c];
	if (nec<NECMAX) ++nec;
    }
    else {
	ac_in(tot,tot+esc,tot+esc);
	if (ft[cp]==1 && iec[cl[cp]]<IECLIM) ++iec[cl[cp]];
	for (i=cp;i!=NIL;sum=i,i=nb[i]) {
	    cmstack[cmsp++]=fc[i];
	    cmask[fc[i]]=1;
	}
	cps[0]=0x8000|cp;
	as[0]=sum;
	c=ESC;
	nec=0;
    }
    return c;
}

static  S16B decode_rest(U16B cp) {

    register U16B c;
    register U16B tv;
    register U16B i;
    register S16B sum,tot,esc,cf;

    esc=tot=0;
    for (i=cp;i!=NIL;i=nb[i]) {
	if (!cmask[fc[i]]) {
	    tot+=fa[i];
	    if (fa[i]<ESCTH) ++esc;
	}
    }
    esc=adj_escape_prob(esc,cp);
    tv=ac_threshold_val(tot+esc);
    for (c=cp,sum=0;;c=nb[c]) {
	if (c==NIL) break;
	if (!cmask[fc[c]]) {
	    if (sum+fa[c]<=tv) sum+=fa[c];
	    else {
		cf=fa[c];
		break;
	    }
	}
    }
    if (c!=NIL) {
	ac_in(sum,sum+cf,tot+esc);
	if (ft[cp]==1 && iec[cl[cp]]) --iec[cl[cp]];
	as[usp]=c;
	cps[usp++]=cp;
	c=fc[c];
	++nec;  /* must add test used in code_first() if NECMAX<5 ! */
    }
    else {
	ac_in(tot,tot+esc,tot+esc);
	if (ft[cp]==1 && iec[cl[cp]]<IECLIM) ++iec[cl[cp]];
	for (i=cp;i!=NIL;sum=i,i=nb[i]) {
	    if (!cmask[fc[i]]) {
		cmstack[cmsp++]=fc[i];
		cmask[fc[i]]=1;
	    }
	}
	cps[usp]=0x8000|cp;
	as[usp++]=sum;		/* sum holds last i !! */
	c=ESC;
    }
    return c;
}

static  S16B decode_new(void) {

    register S16B c;
    register U16B tv,sum,tot;

    tot=257-cmsp;
    tv=ac_threshold_val(tot);
    for (c=sum=0;c<256;++c) {
	if (cmask[c]) continue;
	if (sum+1<=tv) ++sum;
	else break;
    }
    ac_in(sum,sum+1,tot);
    return c;
}

#define decode_byte(cp) (cmsp?decode_rest(cp):decode_first(cp))

/***********************************************************************
	Decoding
***********************************************************************/

void hsc_unpack(void) {

    S16B c;
    U16B cp;
    unsigned char ncmax,ncmin;

    init_unpack();
    for (;;) {
	cp=find_longest();
	ncmin=cp==NIL?0:cl[cp]+1;
	ncmax=maxclen+1;
	for(;;) {
	    if (cp==NIL) {
		c=decode_new();
		break;
	    }
	    if ((c=decode_byte(cp))!=ESC) {
		el_movefront(cp);
		break;
	    }
	    cp=find_next();
	}
	if (c==ESC) break;
	add_model(c);
	while (ncmax>ncmin) make_context(--ncmax,c);
	putbyte(c);
	move_context(c);
    }
    flush();
    hsc_cleanup();
}

