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
  HA main program
***********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include "error.h"
#include "ha.h"
#include "archive.h"
#include "haio.h"
#include "cpy.h"
#include "asc.h"
#include "hsc.h"

/***********************************************************************
  Commands
*/

#define EXTRACT		'e'
#define PEXTRACT	'x'
#define LIST		'l'
#define TEST		't'
#define INFO            'h'

char *myname;
int quiet=0,useattr=0;
static unsigned ilen=0;
static int fulllist=0,usepath=1,yes=0,touch=0;
static char *defpat[]={ALLFILES};
static int metqueue[M_UNK+1]={M_UNK};
static void dummy(void) {
	/* Do nothing */
}

struct {
    char *name;
    void (*decode)(void);
    void (*cleanup)(void); } method[]= {
	{"CPY",cpy,dummy},
	{"ASC",asc_unpack,asc_cleanup},
	{"HSC",hsc_unpack,hsc_cleanup},
	{"3"},{"4"},{"5"},{"6"},{"7"},{"8"},{"9"},{"10"},{"11"},{"12"},{"13"},
	{"DIR"},
	{"SPC"}
    };

static void banner(void) {

    fprintf(stderr,BANNER);
    fflush(stderr);
}

static unsigned getinfo(unsigned char *buf, unsigned blen) {

    static unsigned char *idat=infodat;
    unsigned i;

    for (i=0;i<blen && --ilen;i++) {
	buf[i]=*idat++;
    }
    return i;
}

static void info(void) {

    setoutput(STDOUT_FILENO,0,"stdout");
    ilen=infolen;
    inspecial=getinfo;
    ibl=0;
    fprintf(stdout,BANNER);
    fflush(stdout);
    (*method[M_HSC].decode)();
    fflush(stdout);
    _exit(lasterror);
}


static void usage(int ex) {

    banner();
    fprintf(stderr,"\n usage : HA <cmd> archive [files]"
	    "\n"
	    "\n commands :"
	    "\n   e[aqty]      - Extract files"
	    "\n   x[aqty]      - eXtract files with pathnames"
	    "\n   l[f]         - List files     t[q]         - Test files"
	    "\n"
	    "\n switches :"
	    "\n   t      - Touch files"
	    "\n   f      - Full listing"
		"\n   y      - assume Yes on all questions"
	    "\n   a      - set system specific file Attributes"
	    "\n   e      - Exclude pathnames"
	    "\n   q      - Quiet operation"
	    "\n"
	    "\nType \"ha h | more\" to get more information about HA."
	    "\n"
	    );
    fflush(stderr);
    if (ex) {
	cu_do(NULL);
	_exit(ex);
    }
}

static int yesno(char *format, char *string) {

    int rep;

    if (yes || quiet) return 1;
    printf(format,string);
    fflush(stdout);
    for (rep=0;!rep;) {
	switch (rep=getchar()) {
	  case 'Y' :
	  case 'y' :
	  case 'N' :
	  case 'n' :
	  case 'A' :
	  case 'a' :
	    break;
	  default :
	    rep=0;
	    break;
	}
    }
    if (rep=='Y'||rep=='y') rep=1;
    else if (rep=='A'||rep=='a') rep=yes=1;
    else rep=0;
    return rep;
}

static void backstep(int len) {

    while (--len) putchar(0x08);
}

static void do_list(void) {

    U32B tcs,tos;
    unsigned files;
    Fheader *hd;

    arc_reset();
    if ((hd=arc_seek())==NULL) error(1,ERR_NOFILES);
    printf("\n  filename        original    compressed"
	   "   rate     date        time   m");
    if (fulllist)  {
	printf("\n CRC-32    path");
	md_listhdr();
    }
    printf("\n================================="
	   "==========================================");
    tcs=tos=files=0;
    for(;;) {
	if (hd->type==0xff) continue;
	if (fulllist && files) {
	    printf("\n-------------------------"
		   "--------------------------------------------------");
	}
	printf("\n  %-15s %-11" F_32B " %-11" F_32B " %3d.%d %%   %s  %s",
	       hd->name,(unsigned long)hd->olen,(unsigned long)hd->clen,
	       (hd->olen==0?100:(int)(100*hd->clen/hd->olen)),
	       (hd->olen==0?0:(int)((1000*hd->clen/hd->olen)%10)),
	       md_timestring(hd->time),method[hd->type].name);
	if (fulllist) {
	    printf("\n %08" FX_32B "  %s",(unsigned long)hd->crc,
		   *hd->path==0?"(none)":md_tomdpath(hd->path));
	    md_listdat();
	}
	tcs+=hd->clen;
	tos+=hd->olen;
	++files;
	if ((hd=arc_seek())==NULL) break;
    }
    printf("\n============================="
	   "==============================================");
    printf("\n  %-4d            %-11" F_32B " %-11" F_32B " %3d.%d %%\n",
	   files,(unsigned long)tos,(unsigned long)tcs,(tos==0?100:(int)(100*tcs/tos)),
	   (tos==0?0:(int)((1000*tcs/tos)%10)));
}

static void do_extract(void) {

    Fheader *hd;
    char *ofname;
    unsigned char *sdata;
    int of,newdir;
    void *cumark;

    arc_reset();
    if ((hd=arc_seek())==NULL) error(1,ERR_NOFILES);
    do {
	if (usepath) {
	    makepath(hd->path);
	    ofname=md_tomdpath(fullpath(hd->path,hd->name));
	}
	else ofname=md_tomdpath(hd->name);
	switch(hd->type) {
	  case M_SPECIAL:
	    if (!access(ofname,F_OK)) {
		if (!yesno("\nOverwrite special file %s ? (y/n/a) ",ofname))
		  break;
	    }
	    if (!quiet) {
		printf("\nMaking    SPC        %s",ofname);
		backstep(strlen(ofname)+8);
	    }
	    if (hd->clen) {
		if ((sdata=malloc(hd->clen))==NULL)
		  error(1,ERR_MEM,"do_extract()");
		if (read(arcfile,sdata,hd->clen)!=hd->clen)
		  error(1,ERR_READ,arcname);
	    }
	    else sdata=NULL;
	    if (!md_mkspecial(ofname,hd->clen,sdata)) {
		if (sdata!=NULL) free(sdata);
		break;
	    }
	    if (sdata!=NULL) free(sdata);
	    if (touch) md_setft(ofname,md_systime());
	    else md_setft(ofname,hd->time);
	    if (!quiet) printf("DONE");
	    break;
	  case M_DIR:
	    if (!(newdir=access(ofname,F_OK)) && useattr) {
		if (!yesno("\nRemake directory %s ? (y/n/a) ",ofname)) break;
	    }
	    if (!quiet) {
		printf("\nMaking    DIR        %s",ofname);
		backstep(strlen(ofname)+8);
	    }
	    if (newdir) {
		if (mkdir(ofname,DEF_DIRATTR)<0) error(0,ERR_MKDIR,ofname);
	    }
	    if (touch) md_setft(ofname,md_systime());
	    else md_setft(ofname,hd->time);
	    if (useattr) md_setfattrs(ofname);
	    if (!quiet) printf("DONE");
	    break;
	  default:
	    of=open(ofname,O_WRONLY|O_CREAT|O_EXCL,DEF_FILEATTR);
	    if (of<0) {
		if (!yesno("\nOverwrite file %s ? (y/n/a) ",ofname)) continue;
		if (remove(ofname)<0) {
		    error(0,ERR_REMOVE,ofname);
		    continue;
		}
		if ((of=open(ofname,O_WRONLY|O_CREAT|O_EXCL,
			     DEF_FILEATTR))<0) error(0,ERR_OPEN,ofname);
	    }
	    setinput(arcfile,0,arcname);
	    if (quiet) setoutput(of,CRCCALC,ofname);
	    else setoutput(of,CRCCALC|PROGDISP,ofname);
	    if (!quiet) {
		printf("\nUnpacking %s        %s",
		       method[hd->type].name,ofname);
		backstep(strlen(ofname)+8);
	    }
	    fflush(stdout);
	    if (hd->olen!=0) {
		totalsize=hd->olen;
		cumark=cu_add(CU_FUNC,method[hd->type].cleanup);
		cu_add(CU_RMFILE|CU_CANRELAX,ofname,of);
		(*method[hd->type].decode)();
		cu_relax(cumark);
		cu_do(cumark);
	    }
	    else if (!quiet) printf("100 %%");
	    fflush(stdout);
	    close(of);
	    if (hd->crc!=getcrc()) error(0,ERR_CRC,NULL);
	    if (touch) md_setft(ofname,md_systime());
	    else md_setft(ofname,hd->time);
	    if (useattr) md_setfattrs(ofname);
	    break;
	}
    } while ((hd=arc_seek())!=NULL);
    if (!quiet) printf("\n");
}

static void do_test(void) {

    Fheader *hd;
    char *ofname;
    void *cumark;

    arc_reset();
    if ((hd=arc_seek())==NULL) error(1,ERR_NOFILES);
    do {
	ofname=md_tomdpath(fullpath(hd->path,hd->name));
	switch(hd->type) {
	  case M_DIR:
	    if (!quiet) printf("\nTesting DIR DONE   %s",ofname);
	    break;
	  case M_SPECIAL:
	    if (!quiet) printf("\nTesting SPC DONE   %s",ofname);
	    break;
	  default:
	    setinput(arcfile,0,arcname);
	    if (quiet) setoutput(-1,CRCCALC,"none ??");
	    else setoutput(-1,CRCCALC|PROGDISP,"none ??");
	    if (!quiet) {
		printf("\nTesting %s        %s",method[hd->type].name,ofname);
		backstep(strlen(ofname)+8);
		fflush(stdout);
	    }
	    if (hd->olen!=0) {
		totalsize=hd->olen;
		cumark=cu_add(CU_FUNC,method[hd->type].cleanup);
		(*method[hd->type].decode)();
		cu_do(cumark);
	    }
	    else if (!quiet) printf("100 %%");
	    fflush(stdout);
	    if (hd->crc!=getcrc()) error(0,ERR_CRC,NULL);
	    break;
	}
    } while ((hd=arc_seek())!=NULL);
    if (!quiet) printf("\n");
}

static void switchparse(char *s, char *valid) {

    int i;

    while (*s) {
	if (strchr(valid,tolower(*s))==NULL) error(1,ERR_INVSW,*s);
	switch (tolower(*s)) {
	  case 'q':
	    quiet=1;
	    break;
	  case 'y':
	    yes=1;
	    break;
	  case 'f':
	    fulllist=1;
	    break;
	  case 'a':
	    useattr=1;
	    break;
	  case 't':
	    touch=1;
	    break;
	  case 'e':
	    usepath=0;
	    break;
	  case '0':
	  case '1':
	  case '2':
	    for (*s-='0',i=0;i<M_UNK;++i) {
		if (metqueue[i]==*s) break;
		else if (metqueue[i]==M_UNK) {
		    metqueue[i]=*s;
		    metqueue[i+1]=M_UNK;
		    break;
		}
	    }
	    break;
	  default :
	    error(1,ERR_INVSW,NULL,*s);
	    break;
	}
	++s;
    }
    if (!quiet) banner();
}

static void fix_methods(void) {

    int i;

    if (metqueue[0]==M_UNK) {
	metqueue[0]=M_ASC;
	metqueue[1]=M_CPY;
	metqueue[2]=M_UNK;
    }
    else {
	for (i=0;metqueue[i]!=M_UNK;++i) {
	    if (metqueue[i]==M_CPY) break;
	}
	if (metqueue[i]==M_UNK) {
	    metqueue[i]=M_CPY;
	    metqueue[i+1]=M_UNK;
	}
    }
}

static void (*parse_cmds(char *cs[]))(void) {

    void (*cmd)(void)=do_list;

    switch(tolower(cs[0][0])) {
      case EXTRACT:
	usepath=0;
      case PEXTRACT:
	switchparse(cs[0]+1,"aqty");
	arc_open(cs[1]);
	cmd=do_extract;
	break;
      case TEST:
	switchparse(cs[0]+1,"qe");
	arc_open(cs[1]);
	cmd=do_test;
	break;
      case LIST:
	switchparse(cs[0]+1,"f");
	arc_open(cs[1]);
	cmd=do_list;
	break;
      case INFO:
	info();
	break;
      default:
	usage(ERR_UNKNOWN);
    }
    return cmd;
}

int ha_main(int argc, char *argv[]) {

    void (*command)(void);

    myname=argv[0];
    md_init();
    if (argc<2) usage(ERR_UNKNOWN);
    command=parse_cmds(argv+1);
    testsizes();
    switch (argc) {
      case 2:
	if (command!=info) usage(ERR_UNKNOWN);
	break;
      case 3:
	patterns=defpat;
	patcnt=1;
	break;
      default :
	patterns=argv+3;
	patcnt=argc-3;
	break;
    }
    command();
    cu_do(NULL);
    return lasterror;
}
