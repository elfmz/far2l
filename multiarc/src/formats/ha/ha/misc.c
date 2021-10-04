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
	HA miscalneous routines
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ha.h"
#include "error.h"

typedef void (*Voidfunc)(void);
     
struct culist {
    union {
	Voidfunc func;
	struct {
	    char *name;
	    int handle;
	} fileinfo;
    } arg;
    struct culist *prev,*next;
    unsigned char flags;
};

int skipemptypath=0,sloppymatch=1;
static struct culist cuhead={{NULL},NULL,NULL,0}; 
char **patterns;
unsigned patcnt;

/**********************************************************************
  Size test
*/

void testsizes(void) {

    if (sizeof(U16B)!=2) error(0,ERR_SIZE,"U16B");
    if (sizeof(S16B)!=2) error(0,ERR_SIZE,"S16B");
    if (sizeof(U32B)!=4) error(0,ERR_SIZE,"U32B");
    if (sizeof(S32B)!=4) error(0,ERR_SIZE,"S32B");
}


/**********************************************************************
	Cleanup routines
*/

void *cu_add(unsigned char flags, ...) {

    struct culist *ptr,*mark;
    va_list vaptr;
    char *string;
    
    mark=cuhead.next;
    ptr=malloc(sizeof(struct culist));
    if (ptr==NULL) error(1,ERR_MEM,"add_cleanup()");
    ptr->flags=flags;
    va_start(vaptr,flags);
    if (flags&CU_FUNC) ptr->arg.func=va_arg(vaptr,Voidfunc);
    else if ((flags&CU_RMFILE) || (flags&CU_RMDIR)) {
	string=va_arg(vaptr,char *);
	if ((ptr->arg.fileinfo.name=malloc(strlen(string)+1))==NULL) 
	  error(0,ERR_MEM,"cu_add()");
	strcpy(ptr->arg.fileinfo.name,string);
	if (!(flags&CU_RMDIR)) ptr->arg.fileinfo.handle=va_arg(vaptr,int);
    }
    va_end(vaptr);
    ptr->next=cuhead.next;
    if (cuhead.next!=NULL) cuhead.next->prev=ptr;
    ptr->prev=&cuhead;
    cuhead.next=ptr;
    return mark;
}

void *cu_getmark(void) {

    return cuhead.next;
}

void cu_relax(void *mark) {

    struct culist *ptr;
    
    for (ptr=cuhead.next;ptr!=NULL && ptr!=mark;ptr=ptr->next) {
	if (ptr->flags&CU_CANRELAX) ptr->flags|=CU_RELAXED; 
    }
}

void cu_do(void *mark) {

    struct culist *ptr;
    
    for (ptr=cuhead.next;ptr!=NULL && ptr!=mark;) {
	if ((ptr->flags&CU_FUNC) && ptr->arg.func!=NULL) {
	    if (!(ptr->flags&CU_RELAXED)) ptr->arg.func();
	}
	else if ((ptr->flags&CU_RMFILE) && ptr->arg.fileinfo.name!=NULL) {
	    close(ptr->arg.fileinfo.handle);
	    if (!(ptr->flags&CU_RELAXED) && remove(ptr->arg.fileinfo.name)<0) {
		error(0,ERR_REMOVE,ptr->arg.fileinfo.name);
	    }
	    free(ptr->arg.fileinfo.name);
	}
	else if ((ptr->flags&CU_RMDIR) && ptr->arg.fileinfo.name!=NULL) {
	    if (!(ptr->flags&CU_RELAXED)) rmdir(ptr->arg.fileinfo.name);
	    free(ptr->arg.fileinfo.name);
	}
	cuhead.next=ptr->next;
	free(ptr);
	ptr=cuhead.next;
    }
}

/**********************************************************************
	Simple path handling
*/

char *getname(char *fullpath) {

    int i,j;
    static char *name=NULL;
    
    if (name!=NULL) free(name),name=NULL;
    for (i=j=strlen(fullpath);--i>=0;) {
	if ((unsigned char)fullpath[i]==0xff) {
	    if ((name=malloc(j-i))==NULL) error(1,ERR_MEM,"getname()");
	    strcpy(name,fullpath+i+1);
	    return name;
	}
    }
    return fullpath;
}

char *getpath(char *fullpath) {

    int i;
    static char *path=NULL;
    
    if (path!=NULL) free(path),path=NULL;
    for(i=strlen(fullpath);--i;) {
	if ((unsigned char)fullpath[i]==0xff) {
	    if ((path=malloc(i+2))==NULL) error(1,ERR_MEM,"getpath()");
	    strncpy(path,fullpath,i+1);
	    path[i+1]=0;
	    return path;
	}
    }
    return "";
}

char *fullpath(char *path, char *name) {

    static char *fullpath=NULL;
    int need_delim;
    
    if (fullpath!=NULL) free(fullpath),fullpath=NULL;
    if (path==NULL || *path==0) return name;
    if ((unsigned char)path[strlen(path)-1]!=0xff) need_delim=1;
    else need_delim=0;
    if ((fullpath=malloc(strlen(path)+strlen(name)+need_delim+1))==NULL) {
	error(1,ERR_MEM,"fullpath()");
    }			
    strcpy(fullpath,path);
    strcpy(fullpath+strlen(fullpath)+need_delim,name);
    if (need_delim) fullpath[strlen(fullpath)]=0xff;
    return fullpath;
}

void makepath(char *hapath) {

    char *last,*path;
    
    for (last=strchr(hapath,0xff);last!=NULL;last=strchr(last+1,0xff)) {
	*last=0;
	if (access((path=md_tomdpath(hapath)),F_OK)) {
	    if (mkdir(path,DEF_DIRATTR)<0) error(0,ERR_MKDIR,path);
	}
	*last=0xff;
    }	
}


/**********************************************************************
	General filename matching (for paths in ha format !)
*/

static int matchpattern(char *matchpath, char *matchpat, 
			char *path, char *name) {

    if (((*matchpath==0 && skipemptypath && sloppymatch) || 
	 strcmp(matchpath,path)==0) &&
	md_namecmp(matchpat,name)) return 1;
    return 0;
}


int match(char *path, char *name) {

    int i;
    char *fullhapath;
    
    for (i=0;i<patcnt;++i) {
	fullhapath=md_tohapath(patterns[i]);
	if (matchpattern(getpath(fullhapath),getname(fullhapath),
			 md_strcase(path),md_strcase(name))) return 1;	
    }	
    return 0;	
}






