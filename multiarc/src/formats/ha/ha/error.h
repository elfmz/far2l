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
	HA error hadling 
***********************************************************************/

#define	ERR_UNKNOWN	1	/* Unknown error 			*/
#define ERR_NOTIMP	2	/* Command not implemented 		*/
#define ERR_ARCOPEN	3	/* Could not open archive		*/
#define ERR_MEM		4	/* Out of memory			*/
#define ERR_NOHA	5	/* Not a ha archive			*/
#define ERR_WRITE	6	/* Write error				*/
#define ERR_READ	7	/* Read error 				*/
#define ERR_INT		8	/* Got signal...			*/
#define ERR_NOFILES	9	/* No files found			*/
#define ERR_REMOVE	10	/* Could not remove 			*/
#define ERR_INVSW	11	/* Invalid switch			*/
#define ERR_TOONEW	12	/* Version identifier too high		*/
#define ERR_TOOOLD	13	/* Version identifier too old		*/
#define ERR_UNKMET	14	/* Unknown compression method		*/
#define ERR_SEEK	15	/* Lseek error				*/
#define ERR_OPEN	16	/* Could not open file			*/
#define ERR_MKDIR	17	/* Could not make directory		*/
#define ERR_CRC		18	/* CRC error 				*/
#define ERR_WRITENN	19	/* Write error (no name)		*/
#define ERR_STAT	20	/* Stat failed				*/
#define ERR_DIROPEN     21      /* Open dir                             */
#define ERR_CORRUPTED   22      /* Corrupted archive                    */
#define ERR_SIZE        23      /* Wrong data type size                 */
#define ERR_HOW         24      /* How to handle                        */
#define ERR_RDLINK      25      /* Readlink() error                     */
#define ERR_MKLINK      26      /* Symlinklink() error                  */
#define ERR_MKFIFO      27      /* Mkfifo() error                       */
 
extern int inerror;		/* Current error value */
extern int lasterror;           /* Last error value */

void error(int fatal, int number, ...);

