/*
  CAB.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <sudo.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
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

typedef BYTE u1;
typedef WORD u2;
typedef DWORD u4;

struct CFHEADER
{
	u4 signature;
	u4 reserved1;
	u4 cbCabinet;
	u4 reserved2;
	u4 coffFiles;
	u4 nFiles;
	u1 versionMinor;
	u1 versionMajor;
	u2 cFolders;
	u2 cFiles;
	u2 flags;
	u2 setID;
	u2 iCabinet;
};

struct CFFILE
{
	u4 cbFile;
	u4 uoffFolderStart;
	u2 iFolder;
	u2 date;
	u2 time;
	u2 attribs;
	char szName[256];
};

static int ArcHandle = -1;
static DWORD SFXSize, FilesNumber;
static DWORD UnpVer;

BOOL WINAPI _export CAB_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	int I;
	if (!CanBeExecutableFileHeader(Data, DataSize) && DataSize > 0x1000)
		DataSize = 0x1000;

	for (I = 0; I <= (int)(DataSize - sizeof(struct CFHEADER)); I++) {
		const unsigned char *D = Data + I;
		if (D[0] == 'M' && D[1] == 'S' && D[2] == 'C' && D[3] == 'F') {
			struct CFHEADER *Header = (struct CFHEADER *)(Data + I);
			if (Header->cbCabinet > sizeof(Header) && Header->coffFiles > sizeof(Header)
					&& Header->coffFiles < 0xffff && Header->versionMajor > 0 && Header->versionMajor < 0x10
					&& Header->cFolders > 0) {
				SFXSize = I;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

static void CloseArcHandle()
{
	sdc_close(ArcHandle);
	ArcHandle = -1;
}

BOOL WINAPI _export CAB_OpenArchive(const char *Name, int *Type, bool Silent)
{
	struct CFHEADER MainHeader;
	int ReadSize;
	int I;

	ArcHandle = sdc_open(Name, O_RDONLY);
	if (ArcHandle == -1)
		return FALSE;

	*Type = 0;

	lseek(ArcHandle, SFXSize, SEEK_SET);
	I = SFXSize;
	ReadSize = sdc_read(ArcHandle, &MainHeader, sizeof(MainHeader));
	if (ReadSize != sizeof(MainHeader))
		return CloseArcHandle(), FALSE;

	if (!CAB_IsArchive(NULL, (u1 *)&MainHeader, sizeof(MainHeader))) {
		struct stat s = {0};
		if (sdc_fstat(ArcHandle, &s) == -1 || (s.st_mode & S_IFMT) != S_IFREG) {
			return CloseArcHandle(), FALSE;
		}

		ReadSize = (s.st_size > 0x100000) ? 0x100000 : s.st_size;

		// todo: replace with sdc_read cuz mmap is not sudo-able
		LPBYTE Data = (LPBYTE)mmap(NULL, ReadSize, PROT_READ, MAP_PRIVATE, ArcHandle, 0);
		if (Data == (LPBYTE)MAP_FAILED)
			return CloseArcHandle(), FALSE;

		I = CAB_IsArchive(NULL, Data, ReadSize);
		if (I)
			memcpy(&MainHeader, Data + SFXSize, sizeof(MainHeader));

		munmap(Data, ReadSize);
		if (I == 0)
			return CloseArcHandle(), FALSE;
	} else
		SFXSize = I;

	lseek(ArcHandle, SFXSize + MainHeader.coffFiles, SEEK_SET);
	FilesNumber = MainHeader.cFiles;
	if (FilesNumber == 65535 && (MainHeader.flags & 8))
		FilesNumber = MainHeader.nFiles;
	UnpVer = MainHeader.versionMajor * 256 + MainHeader.versionMinor;

	while (FilesNumber && (MainHeader.flags & 1)) {
		char *EndPos;
		struct CFFILE FileHeader;
		ReadSize = sdc_read(ArcHandle, &FileHeader, sizeof(FileHeader));
		if (ReadSize < 18)
			return CloseArcHandle(), FALSE;

		if (FileHeader.iFolder == 0xFFFD || FileHeader.iFolder == 0xFFFF) {
			EndPos = FileHeader.szName;
			while (EndPos - (char *)&FileHeader < (int)sizeof(FileHeader) && *EndPos)
				EndPos++;
			if (EndPos - (char *)&FileHeader >= (int)sizeof(FileHeader))
				return CloseArcHandle(), FALSE;

			lseek(ArcHandle, (LONG)((EndPos - (char *)&FileHeader + 1) - ReadSize), SEEK_CUR);
			FilesNumber--;
		} else {
			lseek(ArcHandle, 0 - ReadSize, SEEK_CUR);
			break;
		}
	}
	///
	return (TRUE);
}

int WINAPI _export CAB_GetArcItem(struct ArcItemInfo *Info)
{
	struct CFFILE FileHeader;

	int ReadSize;
	char *EndPos;
	FILETIME lft;

	if (FilesNumber-- == 0)
		return GETARC_EOF;
	ReadSize = sdc_read(ArcHandle, &FileHeader, sizeof(FileHeader));
	if (ReadSize < 18)
		return GETARC_READERROR;

	EndPos = FileHeader.szName;
	while (EndPos - (char *)&FileHeader < (int)sizeof(FileHeader) && *EndPos)
		EndPos++;
	if (EndPos - (char *)&FileHeader >= (int)sizeof(FileHeader))
		return GETARC_BROKEN;

	lseek(ArcHandle, (LONG)((EndPos - (char *)&FileHeader + 1) - ReadSize), SEEK_CUR);

	EndPos = FileHeader.szName;
	if (EndPos[0] == '\\' && EndPos[1] != '\\')
		EndPos++;

	Info->PathName.assign(EndPos, strnlen(EndPos, &FileHeader.szName[ARRAYSIZE(FileHeader.szName)] - EndPos));

	for (auto &c : Info->PathName) {
		if (c == '\\')
			c = '/';
	}

#define _A_ENCRYPTED 8
	Info->dwFileAttributes = FileHeader.attribs
			& (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN
					| FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DIRECTORY);
	Info->Encrypted = FileHeader.attribs & _A_ENCRYPTED;
	Info->nPhysicalSize = 0;
	Info->nFileSize = FileHeader.cbFile;
	WINPORT(DosDateTimeToFileTime)(FileHeader.date, FileHeader.time, &lft);
	WINPORT(LocalFileTimeToFileTime)(&lft, &Info->ftLastWriteTime);
	Info->UnpVer = UnpVer;
	return (GETARC_SUCCESS);
}

BOOL WINAPI _export CAB_CloseArchive(struct ArcInfo *Info)
{
	Info->SFXSize = SFXSize;
	return (sdc_close(ArcHandle));
}

DWORD WINAPI _export CAB_GetSFXPos(void)
{
	return SFXSize;
}

BOOL WINAPI _export CAB_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	if (Type == 0) {
		FormatName = "CAB";
		DefaultExt = "cab";
		return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export CAB_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	if (Type == 0) {
		static const char *Commands[] = {/*Extract               */ "MsCab -i0 -FAR {-ap%%R} {-p%%P} {%%S} x "
																	"%%A @%%LMA",
				/*Extract without paths */ "MsCab -i0 -FAR {-p%%P} {%%S} e %%A @%%LMA",
				/*Test                  */ "MsCab -i0 {-p%%P} {%%S} t %%A",
				/*Delete                */ "MsCab -i0 -FAR {-p%%P} {%%S} d %%A @%%LMA",
				/*Comment archive       */ "",
				/*Comment files         */ "",
				/*Convert to SFX        */ "MsCab {%%S} s %%A",
				/*Lock archive          */ "",
				/*Protect archive       */ "",
				/*Recover archive       */ "",
				/*Add files             */ "MsCab -i0 -dirs {-ap%%R} {-p%%P} {%%S} a %%A @%%LNMA",
				/*Move files            */ "MsCab -i0 -dirs {-ap%%R} {-p%%P} {%%S} m %%A @%%LNMA",
				/*Add files and folders */ "MsCab -r0 -i0 -dirs {-ap%%R} {-p%%P} {%%S} a %%A @%%LNMA",
				/*Move files and folders*/ "MsCab -r0 -i0 -dirs {-ap%%R} {-p%%P} {%%S} m %%A @%%LNMA",
				/*"All files" mask      */ "*"};
		if (Command < (int)(ARRAYSIZE(Commands))) {
			Dest = Commands[Command];
			return (TRUE);
		}
	}
	return (FALSE);
}
