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
*************************************************************************/

#define MYVER	2			/* Version info in archives 	*/
#define LOWVER	2			/* Lowest supported version 	*/

enum {M_CPY=0,M_ASC,M_HSC,M_UNK,M_DIR=14,M_SPECIAL};  /* Method types	*/

#define T_DIR           1               /* Types for file type check    */
#define T_SPECIAL       2
#define T_SKIP          3
#define T_REGULAR       4

#define MSDOSMDH	1		/* Identifiers for machine 	*/
#define UNIXMDH		2		/*   specific header data 	*/

typedef struct {			/* Header of file in archive 	*/
    unsigned char type;
    unsigned char ver;
    U32B clen;
    U32B olen;
    U32B time;
    U32B crc;
    char *path;
    char *name;
    unsigned mdilen;
    unsigned mylen;
} Fheader;

extern int arcfile;			/* Archive handle 		*/
extern char *arcname;                   /* Archive name                 */
extern struct stat arcstat;             /* Archive status (when opened) */

void arc_open(char *arcname);
void arc_reset(void);
Fheader *arc_seek(void);

