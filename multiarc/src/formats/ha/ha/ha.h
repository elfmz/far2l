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
	HA main include file
***********************************************************************/

#include "machine.h"

#define VERSION	 "0.9999"
#define BANNER  "HA " VERSION " Copyright (c) 1995 Harri Hirvola\n"

#define CU_CANRELAX    0x01
#define CU_RELAXED     0x02
#define CU_FUNC        0x04
#define CU_RMFILE      0x08
#define CU_RMDIR       0x10

extern char *myname;			/* Name of this program 	*/
extern char **patterns;			/* List of file patterns 	*/
extern unsigned patcnt;			/* File pattern count 		*/
extern int quiet;			/* Be quiet !			*/
extern int useattr;			/* Set/get attributes		*/
extern int special;			/* Find special files		*/

extern unsigned char infodat[];         /* HA information data          */
extern unsigned infolen;                /* HA information data length   */

extern int sloppymatch;                 /* How to use path information in archive seeks */
extern int skipemptypath;

/* Miscalneous routines */

void testsizes(void);
void *cu_add(unsigned long flags, ...);
void *cu_getmark(void);
void cu_relax(void *mark);
void cu_do(void *mark);
char *fullpath(char *path, char *name);
char *getpath(char *fullpath);
char *getname(char *fullpath);
void makepath(char *hapath);
int match(char *path, char *name);




