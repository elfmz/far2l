/*
  ARC.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyrigth (c) 2004 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <sudo.h>
#include <farplug-mb.h>
using namespace oldfar;
#include "fmt.hpp"

#if defined(__BORLANDC__)
#pragma option -a1
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#if _MSC_VER
#define _export
#endif
#endif

/*
#ifdef _MSC_VER
#if _MSC_VER < 1310
#pragma comment(linker, "/ignore:4078")
#pragma comment(linker, "/merge:.data=.")
#pragma comment(linker, "/merge:.rdata=.")
#pragma comment(linker, "/merge:.text=.")
#pragma comment(linker, "/section:.,RWE")
#endif
#endif
*/

#define ARCMARK 0x1A	// special archive marker
#define FNLEN   13		// file name length
#define ARCVER  9		// archive header version code

/*
OFFSET              Count TYPE   Description

0000h                   1 byte   ID=1Ah
0001h                   1 byte   Compression method (see table 0001)
0002h                  12 char   File name
000Fh                   1 dword  Compressed file size
0013h                   1 dword  File date in MS-DOS format (see table 0009)
0017h                   1 word   16-bit CRC
0019h                   1 dword  Original file size

(Table 0001)
ARC compression types
	0 - End of archive marker
	1 - unpacked (obsolete) - ARC 1.0 ?
	2 - unpacked - ARC 3.1
	3 - packed (RLE encoding)
	4 - squeezed (after packing)
	5 - crunched (obsolete) - ARC 4.0
	6 - crunched (after packing) (obsolete) - ARC 4.1
	7 - crunched (after packing, using faster hash algorithm) - ARC 4.6
	8 - crunched (after packing, using dynamic LZW variations) - ARC 5.0
	9 - Squashed c/o Phil Katz (no packing) (var. on crunching)
   10 - crushed (PAK only)
   11 - distilled (PAK only)
12-19 -  to 19 unknown (ARC 6.0 or later) - ARC 7.0 (?)
20-29 - ?informational items? - ARC 6.0
30-39 - ?control items? - ARC 6.0
  40+ - reserved

According to SEA's technical memo, the information and control items
were added to ARC 6.0. Information items use the same headers as archived
files, although the original file size (and name?) can be ignored.

OFFSET              Count TYPE   Description
0000h                   2 byte   Length of header (includes "length"
								 and "type"?)
0002h                   1 byte   (sub)type
0003h                   ? byte   data

Informational item types as used by ARC 6.0 :

Block type    Subtype   Description
   20                   archive info
				0       archive description (ASCIIZ)
				1       name of creator program (ASCIIZ)
				2       name of modifier program (ASCIIZ)

   21                   file info
				0       file description (ASCIIZ)
				1       long name (if not MS-DOS "8.3" filename)
				2       extended date-time info (reserved)
				3       icon (reserved)
				4       file attributes (ASCIIZ)

						Attributes use an uppercase letter to signify the
						following:

								R       read access
								W       write access
								H       hidden file
								S       system file
								N       network shareable

   22                   operating system info (reserved)


Informational item types as used by PAK release 1.5 :

	 Basic archives end with a short header, containing just the
marker (26) and the end of file value (0).  PAK release 1.5 extended
this format by adding information after this end of file marker.  Each
extended record has the following header:

Marker (1 byte)  - always 254
type (1 byte)    - type of record
File (2 bytes)   - # of file in archive to which this record refers,
				   or 0 for the entire archive.
length (4 bytes) - size of record

	 Type      Meaning
	   0  End of file
	   1  Remark
	   2  Path
	   3  Security envelope
	   4  Error correction codes (not implemented in PAK 2.xx)


(Table 0009)
Format of the MS-DOS time stamp (32-bit)
The MS-DOS time stamp is limited to an even count of seconds, since the
count for seconds is only 5 bits wide.

  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 |<---- year-1980 --->|<- month ->|<--- day ---->|

  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 |<--- hour --->|<---- minute --->|<- second/2 ->|


*/

struct RecHeader
{
	BYTE HeadId;	// special archive marker = 0x1A or 0xFE
	BYTE Type;		// Compression method or
					//  type of record:  0  End of file,
					//                   1  Remark,
					//                   2  Path,
					//                   3  Security envelope,
					//                   4  Error correction codes (not implemented in PAK 2.xx)
};

struct ARCHeader
{
	BYTE HeadId;		// special archive marker = 0x1A or 0xFE
	BYTE Type;			// Compression method or
	char Name[FNLEN];	// File name
	DWORD CompSize;		// Compressed file size
	WORD FileDate;		// File date in MS-DOS format
	WORD FileTime;		// File time in MS-DOS format
	WORD CRC16;			// 16-bit CRC
	DWORD OrigSize;		// Original file size
};

struct PAKExtHeader
{
	struct RecHeader RHead;
	WORD FileId;	// # of file in archive to which this record refers, or 0 for the entire archive.
	DWORD RecSize;	// size of record
					// BYTE  Data[];              // Original file size
};

enum
{
	ARC_FORMAT
};

static HANDLE ArcHandle;
static DWORD NextPosition, FileSize, SFXSize;
static int ArcType;
static DWORD OffsetComment;		//, OffsetComment0;
// static int CommentSize=-1;
static int ArcComment;

BOOL WINAPI _export ARC_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	int I = 0;
	if (!CanBeExecutableFileHeader(Data, DataSize) && DataSize > 0x1000)
		DataSize = 0x1000;

	/*
	  if(Data[0] == 'M' && Data[1] == 'Z') // SFX
	  {
		PIMAGE_DOS_HEADER pMZHeader=(PIMAGE_DOS_HEADER)Data;
		I=(pMZHeader->e_cp-1)*512+pMZHeader->e_cblp;
	  }
	*/
	if (DataSize > (int)(sizeof(struct RecHeader) + sizeof(struct ARCHeader))) {
		const struct ARCHeader *D = (const struct ARCHeader *)(Data + I);

		if (D->HeadId == ARCMARK && D->Type <= 11 &&											//???
				((D->FileDate >> 5) & 0b1111) >= 1 && ((D->FileDate >> 5) & 0b1111) <= 12 &&	// month 1 .. 12
				(D->FileTime & 0b11111) <= 30 &&												// seconds divided by 2
				((D->FileTime >> 5) & 0b111111) <= 59 &&										// minutes
				((D->FileTime >> 11) & 0b11111) <= 23 &&										// minutes
				D->CompSize <= 0xffffff00 && D->OrigSize <= 0xffffff00 && !!D->CompSize == !!D->OrigSize) {
			struct stat s
			{};
			if (sdc_stat(Name, &s) == 0 && off_t(D->CompSize + sizeof(ARCHeader)) <= s.st_size) {
				SFXSize = I;
				ArcType = ARC_FORMAT;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

DWORD WINAPI _export ARC_GetSFXPos(void)
{
	return SFXSize;
}

BOOL WINAPI _export ARC_OpenArchive(const char *Name, int *Type, bool Silent)
{
	ArcHandle = WINPORT(CreateFile)(MB2Wide(Name).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ArcHandle == INVALID_HANDLE_VALUE)
		return (FALSE);

	if (Type)
		*Type = ArcType;

	FileSize = WINPORT(GetFileSize)(ArcHandle, NULL);
	NextPosition = SFXSize;

	OffsetComment = 0;
	ArcComment = 0;

	return (TRUE);
}

int WINAPI _export ARC_GetArcItem(struct ArcItemInfo *Info)
{
	ARCHeader Header;
	DWORD ReadSize;
	NextPosition = WINPORT(SetFilePointer)(ArcHandle, NextPosition, NULL, FILE_BEGIN);

	if (NextPosition == 0xFFFFFFFF)
		return (GETARC_READERROR);

	if (NextPosition > FileSize)
		return (GETARC_UNEXPEOF);

	if (!WINPORT(ReadFile)(ArcHandle, &Header, sizeof(WORD), &ReadSize, NULL))
		return (GETARC_READERROR);

	if (!ReadSize || ReadSize != 2)
		return (GETARC_EOF);

	if (Header.HeadId != 0x1A || ((Header.Type) & 0x80))
		return GETARC_BROKEN;

	if (Header.Type == 0) {
		DWORD CurPos = WINPORT(SetFilePointer)(ArcHandle, ReadSize + NextPosition, NULL, FILE_BEGIN);
		if (CurPos != (DWORD)-1 && CurPos < FileSize)
			ArcComment = 1;
		return (GETARC_EOF);
	}

	if (!WINPORT(ReadFile)(ArcHandle, ((char *)&Header) + sizeof(WORD), sizeof(Header) - sizeof(WORD),
				&ReadSize, NULL))
		return (GETARC_READERROR);

	NextPosition+= sizeof(Header) + Header.CompSize;
	if (Header.Type == 1)		// old style is shorter
	{
		// WINPORT(SetFilePointer)(ArcHandle,-(sizeof(DWORD)),NULL,FILE_CURRENT); // ¤«ï ¢ à¨ ­â  á ¨§¢«¥ç¥­¨¥¬!
		Header.Type = 2;					// convert header to new format
		Header.OrigSize = Header.CompSize;	// size is same when not packed
		NextPosition-= sizeof(DWORD);
	}

	if (OffsetComment) {
		char Description[32];
		WINPORT(SetFilePointer)(ArcHandle, OffsetComment, NULL, FILE_BEGIN);
		if (WINPORT(ReadFile)(ArcHandle, Description, 32, &ReadSize, NULL))
			Info->Description.reset(new std::string(Description, ReadSize));
		OffsetComment+= 32;
		Info->Comment = 1;
	}

	CharArrayAssignToStr(Info->PathName, Header.Name);
	Info->nFileSize = Header.OrigSize;
	Info->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;	//??
	Info->nPhysicalSize = Header.CompSize;

	Info->CRC32 = (DWORD)Header.CRC16;

	FILETIME lft;
	WINPORT(DosDateTimeToFileTime)(Header.FileDate, Header.FileTime, &lft);
	WINPORT(LocalFileTimeToFileTime)(&lft, &Info->ftLastWriteTime);

	int Ver = 6 * 256;
	if (Header.Type == 1)
		Ver = 1 * 256;
	else if (Header.Type == 2)
		Ver = 3 * 256 + 1;
	else if (Header.Type <= 5)
		Ver = 4 * 256;
	else if (Header.Type == 6)
		Ver = 4 * 256 + 1;
	else if (Header.Type == 7)
		Ver = 4 * 256 + 6;
	else if (Header.Type <= 9)
		Ver = 5 * 256;

	Info->UnpVer = Ver;

	return (GETARC_SUCCESS);
}

BOOL WINAPI _export ARC_CloseArchive(struct ArcInfo *Info)
{
	Info->Comment = ArcComment;
	return (WINPORT(CloseHandle)(ArcHandle));
}

BOOL WINAPI _export ARC_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	if (Type == ARC_FORMAT) {
		FormatName = "ARC";
		DefaultExt = "arc";
		return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export ARC_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	if (Type == ARC_FORMAT) {
		static const char *Commands[] = {/*Extract               */ "arc xo{g%%P} %%A %%FMQ",
				/*Extract without paths */ "arc eo{g%%P} %%A %%FMQ",
				/*Test                  */ "arc t{g%%P} %%A %%FMQ",
				/*Delete                */ "arc d{g%%P} %%A %%FMQ",
				/*Comment archive       */ "",
				/*Comment files         */ "",
				/*Convert to SFX        */ "",
				/*Lock archive          */ "",
				/*Protect archive       */ "",
				/*Recover archive       */ "",
				/*Add files             */ "arc a{%%S}{g%%P} %%a %%FMQ",
				/*Move files            */ "arc m{%%S}{g%%P} %%a %%FMQ",
				/*Add files and folders */ "arc a{%%S}{g%%P} %%a %%FMQ",
				/*Move files and folders*/ "arc m{%%S}{g%%P} %%a %%FMQ",
				/*"All files" mask      */ "*"};
		if (Command < (int)(ARRAYSIZE(Commands))) {
			Dest = Commands[Command];
			return (TRUE);
		}
	}
	return (FALSE);
}
