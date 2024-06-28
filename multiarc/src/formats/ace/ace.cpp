/*
  ACE.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
#include <malloc.h>
#endif
#include <stddef.h>
#include <memory.h>
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

// #define CALC_CRC

static struct OSIDType
{
	BYTE Type;
	char Name[15];
} OSID[] = {
		{0,  "MS-DOS"  },
		{1,  "OS/2"    },
		{2,  "Win32"   },
		{3,  "Unix"    },
		{4,  "MAC-OS"  },
		{5,  "Win NT"  },
		{6,  "Primos"  },
		{7,  "APPLE GS"},
		{8,  "ATARI"   },
		{9,  "VAX VMS" },
		{10, "AMIGA"   },
		{11, "NEXT"    },
};

struct ACEHEADER
{
	WORD CRC16;			// CRC16 over block
	WORD HeaderSize;	// size of the block(from HeaderType)
	BYTE HeaderType;	// archive header type is 0
	WORD HeaderFlags;
	BYTE Signature[7];	// '**ACE**'
	BYTE VerExtract;	// version needed to extract archive
	BYTE VerCreate;		// version used to create the archive
	BYTE Host;			// HOST-OS for ACE used to create the archive
	BYTE VolumeNum;		// which volume of a multi-volume-archive is it?
	DWORD AcrTime;		// date and time in MS-DOS format
	BYTE Reserved[8];	// 8 bytes reserved for the future
};

static HANDLE ArcHandle;
static DWORD NextPosition, FileSize, SFXSize;
static struct ACEHEADER MainHeader;
static int HostOS = 0, UnpVer = 0;

#if defined(CALC_CRC)
#define CRCPOLY  0xEDB88320
#define CRC_MASK 0xFFFFFFFF

static unsigned crctable[256];
static BOOL CRCTableCreated = FALSE;

static void make_crctable(void)
{
	unsigned r, i, j;
	for (i = 0; i <= 255; i++) {
		for (r = i, j = 8; j; j--)
			r = (r & 1) ? (r >> 1) ^ CRCPOLY : (r >> 1);
		crctable[i] = r;
	}
	CRCTableCreated = TRUE;
}

static DWORD getcrc(DWORD crc, BYTE *addr, int len)
{
	if (!CRCTableCreated)
		make_crctable();
	while (len--)
		crc = crctable[(BYTE)crc ^ (*addr++)] ^ (crc >> 8);
	return (crc);
}
#endif

BOOL WINAPI _export ACE_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	if (!CanBeExecutableFileHeader(Data, DataSize) && DataSize > 0x1000)
		DataSize = 0x1000;

	for (int I = 0; I < (int)(DataSize - sizeof(struct ACEHEADER)); I++) {
		const unsigned char *D = Data + I;
		if (D[0] == '*' && D[1] == '*' && D[2] == 'A' && D[3] == 'C' && D[4] == 'E' && D[5] == '*'
				&& D[6] == '*') {
			struct ACEHEADER *Header = (struct ACEHEADER *)(Data + I - 7);
#if defined(CALC_CRC)
			DWORD crc = CRC_MASK;
			crc = getcrc(crc, &Header->HeaderType, Header->HeaderSize);
#endif

#ifndef offsetof
#define offsetof(s_name, m_name) (_SIZE_T) & (((s_name _FAR *)0)->m_name)
#endif

			if (Header->HeaderType == 0
					&& Header->HeaderSize >= (sizeof(ACEHEADER) - offsetof(ACEHEADER, HeaderFlags))
#if defined(CALC_CRC)
					&& LOWORD(crc) == LOWORD(Header->CRC16)
#endif
			) {
				SFXSize = I - 7;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

BOOL WINAPI _export ACE_OpenArchive(const char *Name, int *Type, bool Silent)
{
	DWORD ReadSize;

	ArcHandle = WINPORT(CreateFile)(MB2Wide(Name).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ArcHandle == INVALID_HANDLE_VALUE)
		return (FALSE);

	*Type = 0;

	FileSize = WINPORT(GetFileSize)(ArcHandle, NULL);
	NextPosition = SFXSize;

	WINPORT(SetFilePointer)(ArcHandle, NextPosition, NULL, FILE_BEGIN);
	if (!WINPORT(ReadFile)(ArcHandle, &MainHeader, sizeof(MainHeader), &ReadSize, NULL)
			|| ReadSize != sizeof(MainHeader)) {
		WINPORT(CloseHandle)(ArcHandle);
		return (FALSE);
	}

	NextPosition = SFXSize + MainHeader.HeaderSize + sizeof(WORD) * 2;
	WINPORT(SetFilePointer)(ArcHandle, NextPosition, NULL, FILE_BEGIN);

	for (int I = 0; I < (int)(ARRAYSIZE(OSID)); ++I)
		if (OSID[I].Type == MainHeader.Host) {
			HostOS = I;
			break;
		}

	UnpVer = (MainHeader.VerExtract / 10) * 256 + (MainHeader.VerExtract % 10);

	return (TRUE);
}

int WINAPI _export ACE_GetArcItem(struct ArcItemInfo *Info)
{
	struct ACEHEADERBLOCK
	{
		WORD CRC16;
		WORD HeaderSize;
	} Block;

	DWORD ReadSize;
	BYTE *TempBuf;

	while (1) {
		NextPosition = WINPORT(SetFilePointer)(ArcHandle, NextPosition, NULL, FILE_BEGIN);
		if (NextPosition == 0xFFFFFFFF)
			return (GETARC_READERROR);

		if (NextPosition > FileSize)
			return (GETARC_UNEXPEOF);

		if (!WINPORT(ReadFile)(ArcHandle, &Block, sizeof(Block), &ReadSize, NULL))
			return (GETARC_READERROR);

		if (!ReadSize || !Block.HeaderSize)		//???
			return (GETARC_EOF);

		TempBuf = (BYTE *)malloc(Block.HeaderSize);
		if (!TempBuf)
			return (GETARC_READERROR);

		if (!WINPORT(ReadFile)(ArcHandle, TempBuf, Block.HeaderSize, &ReadSize, NULL)) {
			free(TempBuf);
			return (GETARC_READERROR);
		}

		if (ReadSize == 0 || Block.HeaderSize != ReadSize) {
			free(TempBuf);
			return (GETARC_EOF);
		}

#if defined(CALC_CRC)
		DWORD crc = CRC_MASK;
		crc = getcrc(crc, TempBuf, Block.HeaderSize);
		if (LOWORD(crc) != LOWORD(Block.CRC16)) {
			free(TempBuf);
			return (GETARC_BROKEN);
		}
#endif

		NextPosition+= sizeof(Block) + Block.HeaderSize;

		if (*TempBuf == 1)		// File block
		{
			struct ACEHEADERFILE
			{
				BYTE HeaderType;
				WORD HeaderFlags;
				DWORD PackSize;
				DWORD UnpSize;
				WORD FTime;		// File Time an Date Stamp
				WORD FDate;		// File Time an Date Stamp
				DWORD FileAttr;
				DWORD CRC32;
				WORD TechInfo;
				WORD DictSize;	//????
				WORD Reserved;
				WORD FileNameSize;
				char FileName[1];
			} *FileHeader;
			FileHeader = (struct ACEHEADERFILE *)TempBuf;
			if (FileHeader->HeaderFlags & 1) {
				Info->dwFileAttributes = FileHeader->FileAttr;
				if (FileHeader->FileNameSize > Block.HeaderSize - (sizeof(ACEHEADERFILE) - 1))
					return (GETARC_BROKEN);
				Info->PathName.assign(FileHeader->FileName, FileHeader->FileNameSize);
				Info->nFileSize = FileHeader->UnpSize;
				Info->nPhysicalSize = FileHeader->PackSize;
				FILETIME lft;
				WINPORT(DosDateTimeToFileTime)(FileHeader->FDate, FileHeader->FTime, &lft);
				WINPORT(LocalFileTimeToFileTime)(&lft, &Info->ftLastWriteTime);

				Info->Solid = FileHeader->HeaderFlags & (1 << 15) ? 1 : 0;
				Info->Encrypted = FileHeader->HeaderFlags & (1 << 14) ? 1 : 0;
				Info->Comment = FileHeader->HeaderFlags & 2 ? 1 : 0;
				Info->UnpVer = UnpVer;
				//?????
				Info->DictSize = 1 << FileHeader->DictSize;
				//?????
				Info->HostOS = OSID[HostOS].Name;
				Info->CRC32 = FileHeader->CRC32;

				NextPosition+= FileHeader->PackSize;
				break;
			}
		}
		struct ACERECORDS
		{
			BYTE HeaderType;	// header type of recovery records is 2
			WORD HeaderFlags;	// Bit 0   1 (RecSize field present)
			DWORD RecSize;
		} *RecHeader;

		RecHeader = (struct ACERECORDS *)TempBuf;
		if (RecHeader->HeaderFlags & 1)
			NextPosition+= RecHeader->RecSize;
		free(TempBuf);
	}
	free(TempBuf);
	return (GETARC_SUCCESS);
}

BOOL WINAPI _export ACE_CloseArchive(struct ArcInfo *Info)
{
	if (Info) {
		Info->SFXSize = SFXSize;
		Info->Volume = MainHeader.HeaderFlags & (1 << 11) ? 1 : 0;
		Info->Comment = MainHeader.HeaderFlags & 2 ? 1 : 0;
		Info->Recovery = MainHeader.HeaderFlags & (1 << 13) ? 1 : 0;
		Info->Lock = MainHeader.HeaderFlags & (1 << 14) ? 1 : 0;
		Info->Flags|= MainHeader.HeaderFlags & (1 << 12) ? AF_AVPRESENT : 0;
	}
	return (WINPORT(CloseHandle)(ArcHandle));
}

DWORD WINAPI _export ACE_GetSFXPos(void)
{
	return SFXSize;
}

BOOL WINAPI _export ACE_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	if (Type == 0) {
		FormatName = "ACE";
		DefaultExt = "ace";
		return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export ACE_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	if (Type == 0) {
		static const char *Commands[] = {/*Extract               */ "unace x -y {-p%%P} %%A %%F",
				/*Extract without paths */ "unace e -y {-p%%P} %%A %%F",
				/*Test                  */ "unace t -y {-p%%P} %%A %%F",
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
				/*"All files" mask      */ "*"};
		if (Command < (int)(ARRAYSIZE(Commands))) {
			Dest = Commands[Command];
			return (TRUE);
		}
	}
	return (FALSE);
}
