/*
  LZH.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
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

// OS ID - Host OS
static struct OSIDType
{
	BYTE Type;
	char Name[15];
} OSID[] = {
		{'M', "MS-DOS"},
		{'2', "OS/2"  },
		{'9', "OS9"   },
		{'K', "OS/68K"},
		{'3', "OS/386"},
		{'H', "HUMAN" },
		{'U', "Unix"  },
		{'C', "CP/M"  },
		{'F', "FLEX"  },
		{'m', "Mac"   },
		{'R', "Runser"},
		{'J', "Java"  },
		{'w', "Win 95"},
		{'W', "Win NT"},
};

// Dictionary size
static struct DictSizeType
{
	BYTE Type[2];
	short Size;
} DictSize[] = {
		{{'h', '0'}, 0 },
		{{'h', '1'}, 4 },
		{{'h', '2'}, 8 },
		{{'h', '3'}, 8 },
		{{'h', '4'}, 4 },
		{{'h', '5'}, 8 },
		{{'z', 's'}, 2 },
		{{'z', '4'}, 0 },
		{{'h', '6'}, 32},
		{{'h', '7'}, 64},
		{{'h', 'd'}, 0 },
		{{'z', '5'}, 4 },
};

static HANDLE ArcHandle;
static DWORD NextPosition, SFXSize, FileSize;

void WINAPI UnixTimeToFileTime(DWORD time, FILETIME *ft);

void WINAPI _export LZH_SetFarInfo(const struct PluginStartupInfo *Info)
{
	;
}

struct LZH_Level0
{
	/* 00 */ BYTE HeadSize;		// Header Size in Bytes
	/* 01 */ BYTE CheckSum;		// Header Checksum
	/* 02 */ BYTE HeadID[3];	// Header ID Code
	/* 05 */ BYTE Method;		// Compression Method
	/* 06 */ BYTE free1;
	/* 07 */ DWORD PackSize;	// Packed File Size
	/* 11 */ DWORD UnpSize;		// Original File Size not decompressed yet
	/* 15 */ WORD FTime;		// File Time an Date Stamp
	/* 17 */ WORD FDate;		// File Time an Date Stamp
	/* 19 */ BYTE FAttr;		// File Attributes
	/* 20 */ BYTE FLevel;		// level = 0x00
	/*
	Level 0
	   21      1 byte   Filename / path length in bytes (f)
	   22     (f)bytes  Filename / path
	   22+(f)  2 bytes  CRC-16 of original file
	   24+(f) (n)bytes  Compressed data
	*/
};

struct LZH_Level1
{
	/* 00 */ BYTE HeadSize;		// Header Size in Bytes
	/* 01 */ BYTE CheckSum;		// Header Checksum
	/* 02 */ BYTE HeadID[3];	// Header ID Code
	/* 05 */ BYTE Method;		// Compression Method
	/* 06 */ BYTE free1;
	/* 07 */ DWORD PackSize;	// Packed File Size
	/* 11 */ DWORD UnpSize;		// Original File Size not decompressed yet
	/* 15 */ WORD FTime;		// File Time an Date Stamp
	/* 17 */ WORD FDate;		// File Time an Date Stamp
	/* 19 */ BYTE Reserved;		// =x020
	/* 20 */ BYTE FLevel;		// level = 0x01
	/*
	Level 1
	   21      1 byte   Filename / path length in bytes (f)
	   22     (f)bytes  Filename / path
	   22+(f)  2 bytes  CRC-16 of original file
	   24+(f)  1 byte   OS ID
	   25+(f)  2 bytes  Next header size(x) (0 means no extension header)
	[ // Extension headers
			   1 byte   Extension type
		   (x)-3 bytes  Extension fields
			   2 bytes  Next header size(x) (0 means no next extension header)
	]*
			  (n)bytes  Compressed data
	*/
};

struct LZH_Level2
{
	/* 00 */ WORD HeadSize;		// Total size of archived file header (h)
	/* 02 */ BYTE HeadID[3];	// Header ID Code
	/* 05 */ BYTE Method;		// Compression Method
	/* 06 */ BYTE free1;
	/* 07 */ DWORD PackSize;	// Packed File Size
	/* 11 */ DWORD UnpSize;		// Original File Size not decompressed yet
	/* 15 */ WORD FTime;		// File Time an Date Stamp
	/* 17 */ WORD FDate;		// File Time an Date Stamp
	/* 19 */ BYTE Reserved;		//
	/* 20 */ BYTE FLevel;		// level = 0x02
	/*
	Level 2
	[
	 21      2 bytes  CRC-16 of original file
	 23      1 byte   OS ID
	 24      2 bytes  Next header size(x) (0 means no extension header)
			 1 byte   Extension type
		 (x)-3 bytes  Extension fields
			 2 bytes  Next header size(x) (0 means no next extension header)
	]*
			(n)bytes  Compressed data
	*/
};

typedef union
{
	struct LZH_Level0 l0;
	struct LZH_Level1 l1;
	struct LZH_Level2 l2;
} LZH_Header;

static BOOL CheckLZHHeader(struct LZH_Level0 *lzh)
{
	return lzh->HeadID[0] == '-' && lzh->HeadID[1] == 'l' && (lzh->HeadID[2] == 'h' || lzh->HeadID[2] == 'z')
			&& ((lzh->Method >= '0' && lzh->Method <= '9') || lzh->Method == 'd' || lzh->Method == 's')
			&& lzh->free1 == '-' && lzh->FLevel <= 2;
}

BOOL WINAPI _export LZH_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	if (!CanBeExecutableFileHeader(Data, DataSize) && DataSize > 0x1000)
		DataSize = 0x1000;

	for (int I = 0; I < DataSize - 5; I++) {
		struct LZH_Level0 *lzh = (struct LZH_Level0 *)(Data + I);
		if (CheckLZHHeader(lzh)) {
			const unsigned char *D = Data + I;
			if (lzh->FLevel == 0 && D[21] > 0 && D[22] != 0
					&& (lzh->HeadSize - 2 - (int)D[21]) == sizeof(struct LZH_Level0) - 1) {
				SFXSize = I;
				return (TRUE);
			}

			if ((lzh->FLevel == 1) || (lzh->FLevel == 2))		// !TODO:
			{
				SFXSize = I;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

BOOL WINAPI _export LZH_OpenArchive(const char *Name, int *Type, bool Silent)
{
	ArcHandle = WINPORT(CreateFile)(MB2Wide(Name).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ArcHandle == INVALID_HANDLE_VALUE)
		return (FALSE);
	*Type = 0;
	FileSize = WINPORT(GetFileSize)(ArcHandle, NULL);

	NextPosition = SFXSize;
	return (TRUE);
}

static void ReadOverflowable(HANDLE Handle, DWORD ChunkSize, void *Buf, DWORD BufSize, LPDWORD BufFilled)
{
	*BufFilled = 0;
	WINPORT(ReadFile)(Handle, Buf, std::min(ChunkSize, BufSize), BufFilled, NULL);
	if (ChunkSize > BufSize && *BufFilled == BufSize)
		WINPORT(SetFilePointer)(Handle, ChunkSize - BufSize, NULL, FILE_CURRENT);
}

int WINAPI _export LZH_GetArcItem(struct ArcItemInfo *Info)
{
	LZH_Header LzhHeader;

	size_t I;
	DWORD ReadSize;

	WORD PathSize = 0;
	WORD CRC16 = 0;

	NextPosition = WINPORT(SetFilePointer)(ArcHandle, NextPosition, NULL, FILE_BEGIN);

	if (NextPosition == 0xFFFFFFFF)
		return (GETARC_READERROR);

	if (NextPosition > FileSize)
		return (GETARC_UNEXPEOF);

	if (!WINPORT(ReadFile)(ArcHandle, &LzhHeader.l0, sizeof(struct LZH_Level0), &ReadSize, NULL))
		return (GETARC_READERROR);

	if (ReadSize <= 10 || !CheckLZHHeader(&LzhHeader.l0))
		return (GETARC_EOF);

	// Offset = 21

	BYTE OsId = 0;
	char FileName[NM];
	FileName[ARRAYSIZE(FileName) - 1] = 0;
	FileName[0] = 0;
	char PathName[NM];
	PathName[ARRAYSIZE(PathName) - 1] = 0;
	PathName[0] = 0;
	DWORD Attr = 0;

	if (LzhHeader.l0.FLevel < 2) {
		BYTE FNameLen;
		WINPORT(ReadFile)(ArcHandle, &FNameLen, 1, &ReadSize, NULL);
		// Offset 22

		if (FNameLen > 0) {
			ReadOverflowable(ArcHandle, FNameLen, FileName, ARRAYSIZE(FileName) - 1, &ReadSize);
		} else if (LzhHeader.l0.FLevel == 0)
			return (GETARC_BROKEN);		//???

		// Offset 22+(f)
		FileName[(DWORD)FNameLen] = 0;
		WINPORT(ReadFile)(ArcHandle, &CRC16, 2, &ReadSize, NULL);	// CRC-16

		// Offset 24+(f)
		if (LzhHeader.l0.FLevel == 1)
			WINPORT(ReadFile)(ArcHandle, &OsId, 1, &ReadSize, NULL);	// OS ID
																		// Offset 25+(f)
		else {
			OsId = 'M';
			Attr = (DWORD)LzhHeader.l0.FAttr;
		}
	} else {
		// Offset = 21
		WINPORT(ReadFile)(ArcHandle, &CRC16, 2, &ReadSize, NULL);	// CRC-16
		// Offset 23
		WINPORT(ReadFile)(ArcHandle, &OsId, 1, &ReadSize, NULL);	// OS ID
																	// Offset 24
	}

	// Offset: L1=25+(f), L2=24
	if (LzhHeader.l0.FLevel >= 1) {
		WORD NextHeaderSize;
		BYTE ExtType;	//, FileNameSize;

		do {
			WINPORT(ReadFile)(ArcHandle, &NextHeaderSize, 2, &ReadSize, NULL);	// Next Header Size
			if (!NextHeaderSize)
				break;

			WINPORT(ReadFile)(ArcHandle, &ExtType, 1, &ReadSize, NULL);		// Extension type

			switch (ExtType) {
				case 0x00:		// Common header
				{
					/*
					   1 byte   Extension type (0x00)
					   2 bytes  CRC-16 of header
					  [1 bytes  Information] (Optional)
					   2 bytes  Next header size
					*/
					WINPORT(ReadFile)(ArcHandle, &CRC16, 2, &ReadSize, NULL);	// CRC-16
					if (NextHeaderSize - 3 - 2 >= 0)
						WINPORT(SetFilePointer)
						(ArcHandle, NextHeaderSize - 3 - 2, NULL, FILE_CURRENT);		//???
					break;
				}

				case 0x01:		// File name header
				{
					/*
					   1 byte   Extension type (0x01)
					   ? bytes  File name
					   2 bytes  Next header size
					*/
					ReadOverflowable(ArcHandle, NextHeaderSize - 3, FileName, ARRAYSIZE(FileName) - 1,
							&ReadSize);
					FileName[NextHeaderSize - 3] = 0;
					break;
				}

				case 0x02:		// Directory name header
				{
					/*
					   1 byte   Extension type (0x02)
					   ? bytes  Directory name
					   2 bytes  Next header size
					*/
					ReadOverflowable(ArcHandle, NextHeaderSize - 3, PathName, ARRAYSIZE(PathName) - 1,
							&ReadSize);
					PathName[PathSize = ReadSize] = 0;

					if (PathName[strlen(PathName) - 1] != '/')
						strcat(PathName, "/");

					if (!FileName[0])
						Attr = FILE_ATTRIBUTE_DIRECTORY;

					break;
				}

				case 0x40:		// MS-DOS attribute header
				{
					/*
					   1 byte   Extension type (0x40)
					   2 bytes  Attr
					   2 bytes  Next header size
					*/
					ReadOverflowable(ArcHandle, NextHeaderSize - 3, &Attr, 2, &ReadSize);
					// Attr &= 0x3f;
					break;
				}

				case 0x3F:		// Comment header
				{
					/*
					   1 byte   Extension type (0x3f)
					   ? bytes  Comments
					   2 bytes  Next header size
					*/
					char Description[256] = {0};
					ReadOverflowable(ArcHandle, NextHeaderSize - 3, Description, ARRAYSIZE(Description),
							&ReadSize);
					Info->Description.reset(new std::string(Description, ReadSize));
					break;
				}

				default:
					WINPORT(SetFilePointer)(ArcHandle, NextHeaderSize - 3, NULL, FILE_CURRENT);
			}
		} while (NextHeaderSize != 0);
	}

	// Convert 0xFF and '\\' to '/'
	for (I = 0; PathName[I]; I++)
		if (PathName[I] == '\xff' || PathName[I] == '\\')
			PathName[I] = '/';

	for (I = 0; FileName[I]; I++)
		if (FileName[I] == '\xff' || FileName[I] == '\\')
			FileName[I] = '/';

	Info->CRC32 = (DWORD)CRC16;

	// correct NextPosition
	DWORD PrevPosition = NextPosition;
	if (LzhHeader.l0.FLevel == 2)
		ReadSize = *(WORD *)&LzhHeader.l0.HeadSize;
	else
		ReadSize = (WORD)LzhHeader.l0.HeadSize + 2;
	NextPosition+= LzhHeader.l0.PackSize + ReadSize;

	if (PrevPosition >= NextPosition || PathSize > NM)
		return (GETARC_BROKEN);

	CharArrayAssignToStr(Info->PathName, PathName);
	CharArrayAppendToStr(Info->PathName, FileName);

	Info->dwFileAttributes = Attr;

	//<????>
	if (LzhHeader.l0.Method == '0' || (LzhHeader.l0.Method == '4' && LzhHeader.l0.HeadID[2] == 'z'))
		Info->nPhysicalSize = LzhHeader.l0.UnpSize;
	else
		Info->nPhysicalSize = (LzhHeader.l0.Method == 'd') ? 0 : LzhHeader.l0.PackSize;
	//</????>
	Info->nFileSize = LzhHeader.l0.UnpSize;

	FILETIME lft;
	if (LzhHeader.l0.FLevel == 2)		// level-2, Original file time stamp(UNIX type, seconds since 1970)
	{
		UnixTimeToFileTime(MAKELONG(LzhHeader.l0.FTime, LzhHeader.l0.FDate), &Info->ftLastWriteTime);
	} else		// Original file date/time (Generic time stamp)
	{
		WINPORT(DosDateTimeToFileTime)(LzhHeader.l0.FDate, LzhHeader.l0.FTime, &lft);
		WINPORT(LocalFileTimeToFileTime)(&lft, &Info->ftLastWriteTime);
	}

	// OS ID - Host OS
	for (I = 0; I < ARRAYSIZE(OSID); ++I) {
		if (OSID[I].Type == OsId) {
			Info->HostOS = OSID[I].Name;
			break;
		}
	}

	// Dictionary size
	Info->DictSize = 0;
	for (I = 0; I < ARRAYSIZE(DictSize); ++I) {
		if (DictSize[I].Type[0] == LzhHeader.l0.HeadID[2] && DictSize[I].Type[1] == LzhHeader.l0.Method) {
			Info->DictSize = DictSize[I].Size;
			break;
		}
	}

	return (GETARC_SUCCESS);
}

BOOL WINAPI _export LZH_CloseArchive(struct ArcInfo *Info)
{
	if (Info)
		Info->SFXSize = SFXSize;
	return (WINPORT(CloseHandle)(ArcHandle));
}

DWORD WINAPI _export LZH_GetSFXPos(void)
{
	return SFXSize;
}

BOOL WINAPI _export LZH_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	if (Type == 0) {
		FormatName = "LZH";
		DefaultExt = "lzh";
		return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export LZH_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	if (Type == 0) {
		// Correct Commands for LHA 2.55!
		static const char *Commands[] = {
#ifdef HAVE_LIBARCHIVE
				/*Extract               */ "^libarch X %%A {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Extract without paths */ "^libarch x %%A {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Test                  */ "^libarch t %%A {-cs=%%T}",
#else
				/*Extract               */ "lhasa x -a -c -d -m {-w%%W} %%a @%%lM",
				/*Extract without paths */ "lhasa e -a -c -m {-w%%W} %%a @%%lM",
				/*Test                  */ "lhasa t -r2 -a -m {-w%%W} %%a",
#endif
				/*Delete                */ "",
				/*Comment archive       */ "",
				/*Comment files         */ "",
				/*Convert to SFX        */ "",
				/*Lock archive          */ "",
				/*Protect archive       */ "",
				/*Recover archive       */ "",
				/*Add files             */ "",
				/*Move files            */ "",
				/*Add files and folders */ "",
				/*Move files and folders*/ "",
#ifdef HAVE_LIBARCHIVE
				/*"All files" mask      */ ""
#else
				/*"All files" mask      */ "*.*"
#endif
		};
		if (Command < (int)(ARRAYSIZE(Commands))) {
			Dest = Commands[Command];
			return (TRUE);
		}
	}
	return (FALSE);
}
