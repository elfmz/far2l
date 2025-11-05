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
	HA archive handling
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "ha.h"
#include "archive.h"
#include "haio.h"
#include "machine.h"

#define STRING	32

int arcfile=-1;
char *arcname=NULL;
struct stat arcstat;
static unsigned arccnt=0;
static U32B nextheader=4,thisheader,arcsize;

static U32B getvalue(int len) {

    unsigned char buf[4];
    U32B val;
    int i;

    if (read(arcfile,buf,len)!=len) error(1,ERR_READ,arcname);
    for (val=i=0;i<len;++i) val|=(U32B)buf[i]<<(i<<3);
    return val;
}

static void putvalue(U32B val, int len) {

    unsigned char buf[4];
    int i;

    for (i=0;i<len;++i,val>>=8) buf[i]=(unsigned char) val&0xff;
    if (write(arcfile,&buf,len)!=len) error(1,ERR_WRITE,arcname);
}

static char *getstring(void) {

    char *sptr;
    int offset;

    if ((sptr=malloc(STRING))==NULL) error(1,ERR_MEM,"getstring()");
    for (offset=0;;offset++) {
	if (read(arcfile,sptr+offset,1)!=1) error(1,ERR_READ,arcname);
	if (sptr[offset]==0) break;
	if ((offset&(STRING-1))==0) {
	    if ((sptr=realloc(sptr,STRING))==NULL)
	      error(1,ERR_MEM,"getstring()");
	}
    }
    return sptr;
}

static void putstring(char *string) {

    int len;

    len=strlen(string)+1;
    if (write(arcfile,string,len)!=len) error(1,ERR_WRITE,arcname);
}

static Fheader *getheader(void) {

    static Fheader hd={0,0,0,0,0,0,NULL,NULL,0};

    if ((hd.ver=getvalue(1))!=0xff) {
	hd.type=hd.ver&0xf;
	hd.ver>>=4;
	if (hd.ver>MYVER) error(1,ERR_TOONEW);
	if (hd.ver<LOWVER) error(1,ERR_TOOOLD);
	if (hd.type!=M_SPECIAL && hd.type!=M_DIR && hd.type>=M_UNK)
	  error(1,ERR_UNKMET,hd.type);
    }
    hd.clen=getvalue(4);
    hd.olen=getvalue(4);
    hd.crc=getvalue(4);
    hd.time=getvalue(4);
    if (hd.path!=NULL) free(hd.path);
    hd.path=getstring();
    if (hd.name!=NULL) free(hd.name);
    hd.name=getstring();
    hd.mdilen=(unsigned)getvalue(1);
    hd.mylen=hd.mdilen+20+strlen(hd.path)+strlen(hd.name);
    md_gethdr(hd.mdilen,hd.type);
    return &hd;
}

static void putheader(Fheader *hd) {

    putvalue((hd->ver<<4)|hd->type,1);
    putvalue(hd->clen,4);
    putvalue(hd->olen,4);
    putvalue(hd->crc,4);
    putvalue(hd->time,4);
    putstring(hd->path);
    putstring(hd->name);
    putvalue(hd->mdilen,1);
    md_puthdr();
}

void arc_close(void) {

    if (arcfile>=0) {
	close(arcfile);
	if (!arccnt) {
	    if (remove(arcname)) error(1,ERR_REMOVE,arcname);
	}
    }
}

static U32B arc_scan(void) {

    U32B pos;
    unsigned i;
    Fheader *hd;

    pos=4;
    for (i=0;i<arccnt;++i) {
	if (pos>=arcsize) {
	    error(0,ERR_CORRUPTED);
	    arccnt=i;
	    return pos;
	}
	if (lseek(arcfile,pos,SEEK_SET)<0) error(1,ERR_SEEK,"arc_seek()");
	hd=getheader();
	pos+=hd->clen+hd->mylen;
	if (hd->ver==0xff) {
	    --i;
	}
    }
    return pos;
}

void arc_open(char *aname) {

    char id[2];

    arcname=md_arcname(aname);
    if ((arcfile=open(arcname,AO_RDOFLAGS))>=0) {
	if (fstat(arcfile,&arcstat)!=0) error(1,ERR_STAT,arcname);
	arcsize=arcstat.st_size;
	if (read(arcfile,id,2)!=2 || id[0]!='H' || id[1]!='A') {
	    error(1,ERR_NOHA,arcname);
	}
	arccnt=(unsigned)getvalue(2);
	arcsize=arc_scan();
	if (!quiet) printf("\nArchive : %s (%d files)\n",arcname,arccnt);
    }
    else error(1,ERR_ARCOPEN,arcname);
    cu_add(CU_FUNC,arc_close);
}

void arc_reset(void) {

    nextheader=4;
}

Fheader *arc_seek(void) {

    static Fheader *hd;

    for (;;) {
	if (nextheader>=arcsize) return NULL;
	if (lseek(arcfile,nextheader,SEEK_SET)<0)
	  error(1,ERR_SEEK,"arc_seek()");
	hd=getheader();
	thisheader=nextheader;
	nextheader+=hd->clen+hd->mylen;
	if (hd->ver!=0xff && match(hd->path,hd->name)) return hd;
    }
}
