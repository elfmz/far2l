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
	HA *nix specific routines
***********************************************************************/

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>
#include <string.h>
#include "ha.h"
#include "error.h"
#include "archive.h"

#define HA_ISVTX     0x0200
#define HA_ISGID     0x0400
#define HA_ISUID     0x0800
#define HA_IRUSR     0x0100
#define HA_IWUSR     0x0080
#define HA_IXUSR     0x0040
#define HA_IRGRP     0x0020
#define HA_IWGRP     0x0010
#define HA_IXGRP     0x0008
#define HA_IROTH     0x0004
#define HA_IWOTH     0x0002
#define HA_IXOTH     0x0001
#define HA_IFMT      0xf000
#define HA_IFIFO     0x1000
#define HA_IFCHR     0x2000
#define HA_IFDIR     0x4000
#define HA_IFBLK     0x6000
#define HA_IFREG     0x8000
#define HA_IFLNK     0xa000
#define HA_IFSOCK    0xc000
#define HA_ISDIR(m)  ((m&HA_IFMT)==HA_IFDIR)
#define HA_ISCHR(m)  ((m&HA_IFMT)==HA_IFCHR)
#define HA_ISBLK(m)  ((m&HA_IFMT)==HA_IFBLK)
#define HA_ISLNK(m)  ((m&HA_IFMT)==HA_IFLNK)
#define HA_ISFIFO(m) ((m&HA_IFMT)==HA_IFIFO)
#define HA_ISSOCK(m) ((m&HA_IFMT)==HA_IFSOCK)

typedef struct {
    unsigned mtype;
    unsigned attr;
    unsigned user;
    unsigned group;
} Mdhd;

#define MDHDLEN      7           /* Length of Mdhd in archive */

static Mdhd mdhd;
struct stat filestat;

static void sig_handler(int signo) {

    error(1,ERR_INT,signo);
}

void md_init(void) {

    signal(SIGINT,sig_handler);
    signal(SIGTERM,sig_handler);
    signal(SIGPIPE,sig_handler);
    signal(SIGQUIT,sig_handler);
    umask(0);
}

U32B md_systime(void) {

    return (U32B)time(NULL);
}

static mode_t md_tomdattr(U16B haattr) {

    mode_t mdattr;

    mdattr=0;
    if (haattr&HA_IRUSR) mdattr|=S_IRUSR;
    if (haattr&HA_IWUSR) mdattr|=S_IWUSR;
    if (haattr&HA_ISUID) mdattr|=S_ISUID;
    if (haattr&HA_IXUSR) mdattr|=S_IXUSR;
    if (haattr&HA_IRGRP) mdattr|=S_IRGRP;
    if (haattr&HA_IWGRP) mdattr|=S_IWGRP;
    if (haattr&HA_ISGID) mdattr|=S_ISGID;
    if (haattr&HA_IXGRP) mdattr|=S_IXGRP;
    if (haattr&HA_IROTH) mdattr|=S_IROTH;
    if (haattr&HA_IWOTH) mdattr|=S_IWOTH;
#ifdef S_ISVTX
    if (haattr&HA_ISVTX) mdattr|=S_ISVTX;
#endif
    if (haattr&HA_IXOTH) mdattr|=S_IXOTH;
    return mdattr;
}

static U16B md_tohaattr(mode_t mdattr) {

    U16B haattr;

    haattr=0;
    if (mdattr&S_IRUSR) haattr|=HA_IRUSR;
    if (mdattr&S_IWUSR) haattr|=HA_IWUSR;
    if (mdattr&S_ISUID) haattr|=HA_ISUID;
    if (mdattr&S_IXUSR) haattr|=HA_IXUSR;
    if (mdattr&S_IRGRP) haattr|=HA_IRGRP;
    if (mdattr&S_IWGRP) haattr|=HA_IWGRP;
    if (mdattr&S_ISGID) haattr|=HA_ISGID;
    if (mdattr&S_IXGRP) haattr|=HA_IXGRP;
    if (mdattr&S_IROTH) haattr|=HA_IROTH;
    if (mdattr&S_IWOTH) haattr|=HA_IWOTH;
    if (mdattr&S_IXOTH) haattr|=HA_IXOTH;
#ifdef S_ISVTX
    if (mdattr&S_ISVTX) haattr|=HA_ISVTX;
#endif
    if (S_ISDIR(mdattr)) haattr|=HA_IFDIR;
    else if (S_ISCHR(mdattr)) haattr|=HA_IFCHR;
    else if (S_ISBLK(mdattr)) haattr|=HA_IFBLK;
#ifdef S_ISLNK
    else if (S_ISLNK(mdattr)) haattr|=HA_IFLNK;
#endif
#ifdef S_ISFIFO
    else if (S_ISFIFO(mdattr)) haattr|=HA_IFIFO;
#endif
#ifdef S_ISSOCK
    else if (S_ISSOCK(mdattr)) haattr|=HA_IFSOCK;
#endif
    return haattr;
}

static char *attrstring(unsigned haattr) {

    static char as[12];
	as[sizeof(as) - 1] = 0;
    snprintf(as, sizeof(as) - 1, "%c%c%c%c%c%c%c%c%c%c",
	    HA_ISFIFO(haattr)?'f':HA_ISSOCK(haattr)?'s':HA_ISLNK(haattr)?'l':
	    HA_ISDIR(haattr)?'d':HA_ISCHR(haattr)?'c':HA_ISBLK(haattr)?'b':'-',
	    (haattr&HA_IRUSR)?'r':'-',
	    (haattr&HA_IWUSR)?'w':'-',
	    (haattr&HA_ISUID)?(haattr&
			       HA_IXUSR)?'s':'S':(haattr&HA_IXUSR)?'x':'-',
	    (haattr&HA_IRGRP)?'r':'-',
	    (haattr&HA_IWGRP)?'w':'-',
	    (haattr&HA_ISGID)?(haattr&
			       HA_IXGRP)?'s':'S':(haattr&HA_IXGRP)?'x':'-',
	    (haattr&HA_IROTH)?'r':'-',
	    (haattr&HA_IWOTH)?'w':'-',
	    (haattr&HA_ISVTX)?(haattr&
			       HA_IXOTH)?'t':'T':(haattr&HA_IXOTH)?'x':'-'
	    );
    return as;
}

void md_gethdr(int len, int mode) {

    static int longest=0;
    static unsigned char *buf=NULL;

    if (len>longest) {
	if (buf!=NULL) buf=realloc(buf,len);
	else buf=malloc(len);
	if (buf==NULL) error(1,ERR_MEM,"md_gethdr()");
	longest=len;
    }
    PERROR_ON_FAIL("read", read(arcfile,buf,len));
    mdhd.mtype=buf[0];
    if (mdhd.mtype==UNIXMDH) {
	mdhd.attr=buf[1]|(buf[2]<<8);
	mdhd.user=buf[3]|(buf[4]<<8);
	mdhd.group=buf[5]|(buf[6]<<8);
    }
    else {
	switch (mode) {
	  case M_DIR:
	    mdhd.attr=md_tohaattr(DEF_DIRATTR);
	    mdhd.attr|=HA_IFDIR;
	    break;
	  default:
	    mdhd.attr=md_tohaattr(DEF_FILEATTR);
	    break;
	}
    }
}

void md_puthdr(void) {

    unsigned char buf[MDHDLEN];

    buf[0]=UNIXMDH;
    buf[1]=mdhd.attr&0xff;
    buf[2]=(mdhd.attr>>8)&0xff;
    buf[3]=mdhd.user&0xff;
    buf[4]=(mdhd.user>>8)&0xff;
    buf[5]=mdhd.group&0xff;
    buf[6]=(mdhd.group>>8)&0xff;
    PERROR_ON_FAIL("write",write(arcfile,buf,MDHDLEN));
}

int md_filetype(char *path, char *name) {

    char *fullpath;

    if (!strcmp(name,".") || !strcmp(name,"..")) return T_SKIP;
    fullpath=md_pconcat(0,path,name);
    if (lstat(fullpath,&filestat)<0) {
	error(0,ERR_STAT,fullpath);
	free(fullpath);
	return T_SKIP;
    }
    free(fullpath);
    if (filestat.st_ino==arcstat.st_ino) return T_SKIP;
    if (S_ISDIR(filestat.st_mode)) return T_DIR;
    if (S_ISREG(filestat.st_mode)) return T_REGULAR;
    return T_SPECIAL;
}

int md_newfile(void) {

    mdhd.attr=md_tohaattr(filestat.st_mode);
    mdhd.user=filestat.st_uid;
    mdhd.group=filestat.st_gid;
    return MDHDLEN;
}

int md_special(char *fullname, unsigned char **sdata) {

    static unsigned char *dat=NULL;
    int len;

    if (dat!=NULL) {
	free(dat);
	dat=NULL;
    }
    if (HA_ISCHR(mdhd.attr)||HA_ISBLK(mdhd.attr)) {
	if ((dat=malloc(sizeof(dev_t)))==NULL) error(1,ERR_MEM,"md_special()");
	*(dev_t*)dat=filestat.st_rdev;
	*sdata=dat;
	return sizeof(dev_t);
    }
    if (HA_ISLNK(mdhd.attr)) {
	if ((dat=malloc(1024))==NULL) error(1,ERR_MEM,"md_special()");
	if ((len=readlink(fullname,(char*)dat,1024))<0)
	  error(1,ERR_RDLINK,fullname);
	dat[len]=0;
	*sdata=dat;
	return len+1;
    }
    else {
	*sdata=dat;
	return 0;
    }
}

int md_mkspecial(char *ofname,unsigned sdlen,unsigned char *sdata) {

    if (mdhd.mtype!=UNIXMDH) {
	error(0,ERR_HOW,ofname);
	return 0;
    }
    if (HA_ISCHR(mdhd.attr)) {
	mknod(ofname,md_tomdattr(mdhd.attr)|S_IFCHR,*(dev_t*)sdata);
	if (useattr) {
		PERROR_ON_FAIL("chown", chown(ofname,mdhd.user,mdhd.group));
	}
	return 1;
    }
    else if (HA_ISBLK(mdhd.attr)) {
	mknod(ofname,md_tomdattr(mdhd.attr)|S_IFBLK,*(dev_t*)sdata);
	if (useattr) {
		PERROR_ON_FAIL("chown", chown(ofname,mdhd.user,mdhd.group));
	}
	return 1;
    }
#ifdef S_ISLNK
    else if (HA_ISLNK(mdhd.attr)) {
	if (symlink(ofname,(char*)sdata)<0) error(0,ERR_MKLINK,sdata,ofname);
	if (useattr) {
	    PERROR_ON_FAIL("chmod", chmod(ofname,md_tomdattr(mdhd.attr)));
	    PERROR_ON_FAIL("chown", chown(ofname,mdhd.user,mdhd.group));
	}
	return 1;
    }
#endif
#ifdef S_ISFIFO
    else if (HA_ISFIFO(mdhd.attr)) {
	if (mkfifo(ofname,md_tomdattr(mdhd.attr))<0)
	  error(0,ERR_MKFIFO,sdata,ofname);
	if (useattr) PERROR_ON_FAIL("chown", chown(ofname,mdhd.user,mdhd.group));
	return 1;
    }
#endif
    error(0,ERR_HOW,ofname);
    return 0;
}

void md_setfattrs(char *file) {

    if (useattr) {
	PERROR_ON_FAIL("chmod", chmod(file,md_tomdattr(mdhd.attr)));
	PERROR_ON_FAIL("chown", chown(file,mdhd.user,mdhd.group));
    }
}

void md_setft(char *file,U32B time) {

    struct utimbuf utb;

    utb.actime=time;
    utb.modtime=time;
    PERROR_ON_FAIL("utime", utime(file,&utb));
}

void md_listhdr(void) {

    printf("\n attr");
}

void md_listdat(void) {

    printf("\n %s",attrstring(mdhd.attr));
}

char *md_timestring(time_t t) {

    static char ts[40];
    struct tm *tim;

    tim=localtime(&t);
    sprintf(ts,"%04d-%02d-%02d  %02d:%02d",tim->tm_year+1900,tim->tm_mon+1,
	    tim->tm_mday,tim->tm_hour,tim->tm_min);
    return ts;
}

char *md_arcname(char *name_req) {

    int pos;
    char *newname;

    pos=strlen(name_req);
    if (pos>3 &&
	tolower(name_req[pos-1])=='a' &&
	tolower(name_req[pos-2])=='h' &&
	name_req[pos-3]=='.') return name_req;
    if ((newname=malloc(pos+4))==NULL) error(1,ERR_MEM,"md_arcname()");
    strcpy(newname,name_req);
    strcpy(newname+pos,".ha");
    return newname;
}

void md_truncfile(int fh, U32B len) {

    PERROR_ON_FAIL("ftruncate", ftruncate(fh,len));
}

char *md_tohapath(char *mdpath) {

    int i,j;
    static char *hapath=NULL;

    if (hapath!=NULL) free(hapath),hapath=NULL;
    j=strlen(mdpath);
    for (i=0;mdpath[i];++i) if (mdpath[i]!='.' && mdpath[i]!='/') break;
    while (i>0 && mdpath[i-1]=='.') --i;
    if (i==0) skipemptypath=1;
    else skipemptypath=0;
    if ((hapath=malloc(j+1-i))==NULL) error(1,ERR_MEM,"md_tohapath()");
    strcpy(hapath,mdpath+i);
    for (i=0;hapath[i];++i) if (hapath[i]=='/') hapath[i]=0xff;
    return md_strcase(hapath);
}

char *md_tomdpath(char *hapath) {

    int i;
    static char *mdpath=NULL;

    if (mdpath!=NULL) free(mdpath),mdpath=NULL;
    if ((mdpath=malloc(strlen(hapath)+1))==NULL)
      error(1,ERR_MEM,"md_tomdpath()");
    strcpy(mdpath,hapath);
    for (i=0;mdpath[i];++i) if ((unsigned char)mdpath[i]==0xff) mdpath[i]='/';
    return mdpath;
}

char *md_strippath(char *mdfullpath) {

    int i;
    static char *plainpath=NULL;

    if (plainpath!=NULL) free(plainpath),plainpath=NULL;
    if ((plainpath=malloc(strlen(mdfullpath)+1))==NULL)
      error(1,ERR_MEM,"md_strippath()");
    strcpy(plainpath,mdfullpath);
    for (i=strlen(plainpath)-1;i>=0;i--) {
	if (plainpath[i]=='/') break;
    }
    plainpath[i+1]=0;
    return plainpath;
}

char *md_stripname(char *mdfullpath) {

    int i;
    static char *plainname=NULL;

    if (plainname!=NULL) free(plainname),plainname=NULL;
    if ((plainname=malloc(strlen(mdfullpath)+1))==NULL)
      error(1,ERR_MEM,"md_stripname()");
    for (i=strlen(mdfullpath)-1;i>0;i--) {
	if (mdfullpath[i]=='/') {
	    i++;
	    break;
	}
    }
    strcpy(plainname,mdfullpath+i);
    return plainname;
}

char *md_pconcat(int delim2, char *head, char *tail) {

    char *newpath;
    int headlen,delim1;

    delim1=0;
    if ((headlen=strlen(head))!=0)  {
	if (head[headlen-1]!='/') delim1=1;
    }
    if ((newpath=malloc(headlen+strlen(tail)+delim2+delim1+1))==NULL)
      error(1,ERR_MEM,"md_pconcat()");
    if (headlen!=0) strcpy(newpath,head);
    if (delim1) newpath[headlen]='/';
    strcpy(newpath+headlen+delim1,tail);
    if (delim2) strcpy(newpath+strlen(newpath),"/");
    return newpath;
}

int md_namecmp(char *pat, char *cmp) {

    if (*pat==0) return !*cmp;
    if (*pat=='?') {
	if (!*cmp) return 0;
	return md_namecmp(pat+1,cmp+1);
    }
    if (*pat=='*') {
	if (*(pat+1)==0) return 1;
	for (;*cmp;++cmp) {
	    if (md_namecmp(pat+1,cmp)) return 1;
	}
	return 0;
    }
    if (*pat=='\\') {
	++pat;
	if (*pat==0) return 0;
    }
    if (*pat==*cmp) return md_namecmp(pat+1,cmp+1);
    return 0;
}



