/*
  TARGZ.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

// #define USE_TAR_H

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <stdlib.h>
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
#pragma comment(linker, "-subsystem:console")
#pragma comment(linker, "-merge:.rdata=.text")

#endif
#endif
*/

#if defined(USE_TAR_H)
#include "tar.h"

#else

struct posix_header
{						/* byte offset */
	char name[100];		/*   0 = 0x000 */
	char mode[8];		/* 100 = 0x064 */
	char uid[8];		/* 108 = 0x06C */
	char gid[8];		/* 116 = 0x074 */
	char size[12];		/* 124 = 0x07C */
	char mtime[12];		/* 136 = 0x088 */
	char chksum[8];		/* 148 = 0x094 */
	char typeflag;		/* 156 = 0x09C */
	char linkname[100]; /* 157 = 0x09D */
	char magic[6];		/* 257 = 0x101 */
	char version[2];	/* 263 = 0x107 */
	char uname[32];		/* 265 = 0x109 */
	char gname[32];		/* 297 = 0x129 */
	char devmajor[8];	/* 329 = 0x149 */
	char devminor[8];	/* 337 = 0x151 */
	char prefix[155];	/* 345 = 0x159 */
						/* 500 = 0x1F4 */
};

#define TMAGIC       "ustar"		// ustar and a null
#define TMAGLEN      6
#define GNUTMAGIC    "GNUtar"		// 7 chars and a null
#define GNUTMAGLEN   7
#define TVERSION     "00"			// 00 and no null
#define TVERSLEN     2

/* OLDGNU_MAGIC uses both magic and version fields, which are contiguous.
   Found in an archive, it indicates an old GNU header format, which will be
   hopefully become obsolescent.  With OLDGNU_MAGIC, uname and gname are
   valid, though the header is not truly POSIX conforming.  */
#define OLDGNU_MAGIC "ustar  "		/* 7 chars and a null */

enum archive_format
{
	DEFAULT_FORMAT, /* format to be decided later */
	V7_FORMAT,		/* old V7 tar format */
	OLDGNU_FORMAT,	/* GNU format as per before tar 1.12 */
	POSIX_FORMAT,	/* restricted, pure POSIX format */
	GNU_FORMAT		/* POSIX format with GNU extensions */
};

#define BLOCKSIZE        512
typedef union block
{
	char buffer[BLOCKSIZE];
	struct posix_header header;
} TARHeader;

/* Identifies the *next* file on the tape as having a long linkname.  */
#define GNUTYPE_LONGLINK 'K'

/* Identifies the *next* file on the tape as having a long name.  */
#define GNUTYPE_LONGNAME 'L'

#define GNUTYPE_PAXHDR 'x'

/* Values used in typeflag field.  */
#define REGTYPE        '0'		/* regular file */
#define AREGTYPE       '\0'		/* regular file */
#define LNKTYPE        '1'		/* link */
#define SYMTYPE        '2'		/* reserved */
#define CHRTYPE        '3'		/* character special */
#define BLKTYPE        '4'		/* block special */
#define DIRTYPE        '5'		/* directory */
#define FIFOTYPE       '6'		/* FIFO special */
#define CONTTYPE       '7'		/* reserved */

#endif

enum
{
	TAR_FORMAT,
	GZ_FORMAT,
	Z_FORMAT,
	BZ_FORMAT,
	XZ_FORMAT
};

typedef union
{
	int64_t i64;
	struct
	{
		DWORD LowPart;
		LONG HighPart;
	} Part;
} FAR_INT64;

static int IsTarHeader(const unsigned char *Data, int DataSize);
static int64_t GetOctal(const char *Str);
static int GetArcItemGZIP(struct ArcItemInfo *Info);
static int GetArcItemTAR(struct ArcItemInfo *Info);
static int64_t Oct2Size(const char *where0, size_t digs0);

static HANDLE ArcHandle;
static FAR_INT64 NextPosition, FileSize;
static int ArcType;
static enum archive_format TarArchiveFormat;
static std::string ZipName;

typedef int(WINAPI *FARSTDMKLINK)(const char *Src, const char *Dest, DWORD Flags);

typedef void(__cdecl *MAFREE)(void *block);
typedef void *(__cdecl *MAMALLOC)(size_t size);

static MAFREE MA_free;
static MAMALLOC MA_malloc;

void WINAPI _export TARGZ_SetFarInfo(const struct PluginStartupInfo *Info)
{
	MA_free = (MAFREE)Info->FSF->Reserved[1];
	MA_malloc = (MAMALLOC)Info->FSF->Reserved[0];
}

void WINAPI UnixTimeToFileTime(DWORD time, FILETIME *ft);

BOOL WINAPI _export TARGZ_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	if (IsTarHeader(Data, DataSize)) {
		ArcType = TAR_FORMAT;
		return (TRUE);
	}

	if (DataSize < 2)
		return (FALSE);

	if (Data[0] == 0x1f && Data[1] == 0x8b)
		ArcType = GZ_FORMAT;
	else if (Data[0] == 0x1f && Data[1] == 0x9d)
		ArcType = Z_FORMAT;
	else if (Data[0] == 'B' && Data[1] == 'Z')
		ArcType = BZ_FORMAT;
	else if (DataSize >= 6 && memcmp(Data, "\xFD\x37\x7A\x58\x5A\x00", 6) == 0)
		ArcType = XZ_FORMAT;
	else
		return (FALSE);

	const char *NamePtr = (const char *)strrchr((char *)Name, '/');
	NamePtr = (NamePtr == NULL) ? Name : NamePtr + 1;
	ZipName = NamePtr;
	const char *Dot = (const char *)strrchr((char *)NamePtr, '.');

	if (Dot != NULL) {
		ZipName.resize(Dot - NamePtr);
		if (strcasecmp(Dot + 1, "tgz") == 0 || strcasecmp(Dot + 1, "taz") == 0)
			ZipName+= ".tar";
	}

	return (TRUE);
}

BOOL WINAPI _export TARGZ_OpenArchive(const char *Name, int *Type, bool Silent)
{
	ArcHandle = WINPORT(CreateFile)(MB2Wide(Name).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ArcHandle == INVALID_HANDLE_VALUE)
		return (FALSE);
	*Type = ArcType;

	FileSize.Part.LowPart = WINPORT(GetFileSize)(ArcHandle, (LPDWORD)&FileSize.Part.HighPart);

	NextPosition.i64 = 0;
	return (TRUE);
}

DWORD WINAPI _export TARGZ_GetSFXPos(void)
{
	return 0;
}

int WINAPI _export TARGZ_GetArcItem(struct ArcItemInfo *Info)
{
	if (ArcType != TAR_FORMAT) {
		if (!ZipName.empty()) {
			switch (ArcType) {
				case BZ_FORMAT:
				case XZ_FORMAT:
					Info->nFileSize = FileSize.i64;
					Info->nPhysicalSize = FileSize.i64;
					Info->PathName = ZipName;
					ZipName.clear();
					return (GETARC_SUCCESS);

				default:
					return GetArcItemGZIP(Info);
			}
		} else
			return (GETARC_EOF);
	}
	return GetArcItemTAR(Info);
}

static int GetArcItemGZIP(struct ArcItemInfo *Info)
{
	DWORD ReadSize;
	struct GZHeader
	{
		BYTE Mark[2];
		BYTE Method;
		BYTE Flags;
		DWORD FileTime;
		BYTE ExtraFlags;
		BYTE HostOS;
	} Header;

	Info->PathName.clear();

	if (!WINPORT(ReadFile)(ArcHandle, &Header, sizeof(Header), &ReadSize, NULL))
		return (GETARC_READERROR);

	Info->nPhysicalSize = FileSize.i64;

	if (ArcType == Z_FORMAT) {
		Info->PathName = ZipName;
		ZipName.clear();
		Info->nFileSize = FileSize.i64;
		return (GETARC_SUCCESS);
	}

	if (Header.Flags & 2)	// skip CRC16
		WINPORT(SetFilePointer)(ArcHandle, 2, NULL, FILE_CURRENT);

	if (Header.Flags & 4)		// skip FEXTRA
	{
		WORD ExtraLength = 0;
		if (!WINPORT(ReadFile)(ArcHandle, &ExtraLength, sizeof(ExtraLength), &ReadSize, NULL))
			return (GETARC_READERROR);
		LITEND_INPLACE(ExtraLength);
		WINPORT(SetFilePointer)(ArcHandle, ExtraLength, NULL, FILE_CURRENT);
	}

	char cFileName[NM + 1] = {0};
	if (Header.Flags & 8)
		if (!WINPORT(ReadFile)(ArcHandle, cFileName, sizeof(cFileName) - 1, &ReadSize, NULL))
			return (GETARC_READERROR);

	if (cFileName[0] == 0) {
		Info->PathName = ZipName;

	} else {	// workaround for tar.gz archives that has original name set but without .tar extension
				// since tar archives detection relies on extension, it should be there (#173)
		Info->PathName = cFileName;
		const char *ZipExt = strrchr(ZipName.c_str(), '.');
		if (ZipExt && strcasecmp(ZipExt, ".tar") == 0) {
			const char *OrigExt = strrchr(cFileName, '.');
			if (!OrigExt || strcasecmp(OrigExt, ZipExt) != 0) {
				Info->PathName+= ZipExt;
			}
		}
	}

	ZipName.clear();

	UnixTimeToFileTime(LITEND(Header.FileTime), &Info->ftLastWriteTime);

	Info->Comment = (Header.Flags & 16) != 0;
	Info->Encrypted = (Header.Flags & 32) != 0;
	WINPORT(SetFilePointer)(ArcHandle, -4, NULL, FILE_END);

	uint32_t nFileSizeRaw = 0;
	if (!WINPORT(ReadFile)(ArcHandle, &nFileSizeRaw, sizeof(nFileSizeRaw), &ReadSize, NULL))
		return (GETARC_READERROR);
	Info->nFileSize = LITEND(nFileSizeRaw);

	return (GETARC_SUCCESS);
}

static int GetArcItemTAR(struct ArcItemInfo *Info)
{
	TARHeader TAR_hdr;
	DWORD ReadSize;
	BOOL SkipItem = FALSE;
	std::vector<char> LongName;
	do {
		NextPosition.Part.LowPart = WINPORT(SetFilePointer)(ArcHandle, NextPosition.Part.LowPart,
				&NextPosition.Part.HighPart, FILE_BEGIN);

		if (NextPosition.i64 == (int64_t)-1 && WINPORT(GetLastError)() != NO_ERROR)
			return (GETARC_READERROR);

		if (NextPosition.i64 > FileSize.i64)
			return (GETARC_UNEXPEOF);

		if (!WINPORT(ReadFile)(ArcHandle, &TAR_hdr, sizeof(TAR_hdr), &ReadSize, NULL))
			return (GETARC_READERROR);

		if (ReadSize == 0 || *TAR_hdr.header.name == 0)
			return (GETARC_EOF);

		// fprintf(stderr, "TAR_hdr.header.typeflag='%c' %x size=%s\n", TAR_hdr.header.typeflag, TAR_hdr.header.typeflag, TAR_hdr.header.size);
		if (TAR_hdr.header.typeflag == GNUTYPE_LONGLINK || TAR_hdr.header.typeflag == GNUTYPE_LONGNAME
				|| TAR_hdr.header.typeflag == GNUTYPE_PAXHDR) {
			SkipItem = TRUE;
		} else {
			// TODO: GNUTYPE_LONGLINK
			DWORD dwUnixMode = 0;
			SkipItem = FALSE;
			if (!LongName.empty()) {
				Info->PathName = LongName.data();
			} else {
				Info->PathName.clear();
				if (TAR_hdr.header.prefix[0]) {
					CharArrayAssignToStr(Info->PathName, TAR_hdr.header.prefix);
					Info->PathName+= '/';
				}
				CharArrayAppendToStr(Info->PathName, TAR_hdr.header.name);
			}

			Info->nFileSize = 0;
			dwUnixMode = (DWORD)GetOctal(TAR_hdr.header.mode);
			switch (TAR_hdr.header.typeflag) {
				case REGTYPE:
				case AREGTYPE:
					dwUnixMode|= S_IFREG;
					break;

				case SYMTYPE:
					// fallthrough
				case LNKTYPE:
					dwUnixMode|= S_IFLNK;
					break;

				case CHRTYPE:
					dwUnixMode|= S_IFCHR;
					break;

				case BLKTYPE:
					dwUnixMode|= S_IFBLK;
					break;

				case FIFOTYPE:
					dwUnixMode|= S_IFIFO;
					break;

				case DIRTYPE:
					dwUnixMode|= S_IFDIR;
					break;
			}

			if ((dwUnixMode & S_IFMT) == S_IFLNK)		// TAR_hdr.header.typeflag == SYMTYPE || TAR_hdr.header.typeflag == LNKTYPE
			{
				Info->LinkName.reset(new std::string(TAR_hdr.header.linkname));
				if (TAR_hdr.header.typeflag == LNKTYPE)
					Info->LinkName->insert(0, "/");
			}

			Info->dwFileAttributes = WINPORT(EvaluateAttributesA)(dwUnixMode, Info->PathName.c_str());
			Info->dwUnixMode = dwUnixMode;

			UnixTimeToFileTime((DWORD)GetOctal(TAR_hdr.header.mtime), &Info->ftLastWriteTime);
		}

		DWORD64 TarItemSize = (TAR_hdr.header.typeflag == DIRTYPE)		// #348
				? 0
				: Oct2Size(TAR_hdr.header.size, sizeof(TAR_hdr.header.size));

		Info->nFileSize = TarItemSize;
		Info->nPhysicalSize = TarItemSize;

		Info->HostOS = (TarArchiveFormat == POSIX_FORMAT)
				? "POSIX"
				: (TarArchiveFormat == V7_FORMAT ? "V7" : nullptr);
		Info->UnpVer = 256 + 11 + (TarArchiveFormat >= POSIX_FORMAT ? 1 : 0);	//!!!

		FAR_INT64 PrevPosition = NextPosition;
		// for LNKTYPE - only sizeof(TAR_hdr)
		NextPosition.i64+=
				(int64_t)sizeof(TAR_hdr) + (TAR_hdr.header.typeflag == LNKTYPE ? int64_t(0) : TarItemSize);

		if (NextPosition.i64 & int64_t(511))
			NextPosition.i64+= int64_t(512) - (NextPosition.i64 & int64_t(511));

		if (PrevPosition.i64 >= NextPosition.i64)
			return (GETARC_BROKEN);

		if (TAR_hdr.header.typeflag == GNUTYPE_LONGNAME || TAR_hdr.header.typeflag == GNUTYPE_LONGLINK
				|| TAR_hdr.header.typeflag == GNUTYPE_PAXHDR) {
			PrevPosition.i64+= (int64_t)sizeof(TAR_hdr);
			WINPORT(SetFilePointer)
			(ArcHandle, PrevPosition.Part.LowPart, &PrevPosition.Part.HighPart, FILE_BEGIN);
			// we can't have two LONGNAME records in a row without a file between them
			if (!LongName.empty())
				return GETARC_BROKEN;
			LongName.resize(Info->nPhysicalSize + 1);
			DWORD BytesRead;
			WINPORT(ReadFile)(ArcHandle, LongName.data(), Info->nPhysicalSize, &BytesRead, NULL);
			if (BytesRead != Info->nPhysicalSize)
				return GETARC_BROKEN;

			if (TAR_hdr.header.typeflag == GNUTYPE_PAXHDR) {	// pax extended header: consists of sequence of "len key=value\n" strings
				// currently using only "path" key - its a modern way to specify long file path
				std::vector<char> PathRecord;
				for (DWORD i = 0; i < BytesRead;) {
					int len = atoi(&LongName[i]);
					if (len <= 0 || (DWORD)len > BytesRead - i) {
						fprintf(stderr, "%s: ext record bad len=%d at %x\n", __FUNCTION__, len, i);
						break;
					}

					char *spc = strchr(&LongName[i], ' ');
					char *eq = strchr(&LongName[i], '=');
					if (!spc || !eq || eq < spc || LongName[i + len - 1] != '\n') {
						fprintf(stderr, "%s: ext record bad at %x\n", __FUNCTION__, i);
						break;
					}
					if (strncmp(spc, " path=", 6) == 0) {
						PathRecord.assign(eq + 1, &LongName[i + len - 1]);
						PathRecord.emplace_back(0);
					}
					i+= len;
				}
				LongName.swap(PathRecord);
			}
		}

	} while (SkipItem);

	return (GETARC_SUCCESS);
}

BOOL WINAPI _export TARGZ_CloseArchive(struct ArcInfo *Info)
{
	return (WINPORT(CloseHandle)(ArcHandle));
}

BOOL WINAPI _export TARGZ_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	static const char *const FmtAndExt[5][2] = {
			{"TAR",     "tar"},
			{"GZip",    "gz" },
			{"Z(Unix)", "z"  },
			{"BZip",    "bz2"},
			{"XZip",    "xz" },
	};
	switch (Type) {
		case TAR_FORMAT:
		case GZ_FORMAT:
		case Z_FORMAT:
		case BZ_FORMAT:
		case XZ_FORMAT:
			FormatName = FmtAndExt[Type][0];
			DefaultExt = FmtAndExt[Type][1];
			return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export TARGZ_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	static const char *Commands[5][15] = {
			{	// TAR_FORMAT
#if defined(__TAR_LIMITED_ARGS__)
					"tar -xf %%A %%FSq32768", "tar -O -xf %%A %%fSq > %%fWq", "",
					"tar --delete -f %%A %%FSq32768", "", "", "", "", "", "", "tar -cf %%A %%FSq32768",
					"tar --remove-files -cf %%A %%FSq32768", "tar -cf %%A %%FSq32768",
					"tar --remove-files -cf %%A %%FSq32768", "*"
#else
					"tar --force-local -xf %%A %%FSq32768", "tar --force-local -O -xf %%A %%fSq > %%fWq", "",
					"tar --delete --force-local -f %%A %%FSq32768", "", "", "", "", "", "",
					"tar --force-local -rf %%A %%FSq32768",
					"tar --force-local --remove-files -rf %%A %%FSq32768",
					"tar --force-local -rf %%A %%FSq32768",
					"tar --force-local --remove-files -rf %%A %%FSq32768", "*"
#endif
			},

			{	// GZ_FORMAT
					"gzip -cd %%A >%%fq", "gzip -cd %%A >%%fq", "gzip -t %%A", "", "", "", "", "", "", "",
					"gzip -c %%fq >%%A", "gzip %%fq", "gzip -c %%fq >%%A", "gzip %%fq", "*"},

			{	// Z_FORMAT
					"gzip -cd %%A >%%fq", "gzip -cd %%A >%%fq", "gzip -t %%A", "", "", "", "", "", "", "", "",
					"", "", "", "*"},

			{	// BZ_FORMAT
					"bzip2 -cd %%A >%%fq", "bzip2 -cd %%A >%%fq", "bzip2 -cd %%A >/dev/null", "", "", "", "",
					"", "", "", "bzip2 -c %%fq >%%A", "bzip2 %%fq", "bzip2 -c %%fq >%%A", "bzip2 %%fq", "*"},
			{	// BZ_FORMAT
					"xz -cd %%A >%%fq", "xz -cd %%A >%%fq", "xz -cd %%A >/dev/null", "", "", "", "", "", "",
					"", "xz -c %%fq >%%A", "xz %%fq", "xz -c %%fq >%%A", "xz %%fq", "*"},
	};
	if (Type >= TAR_FORMAT && Type <= XZ_FORMAT && Command < (int)(ARRAYSIZE(Commands[Type]))) {
		Dest = Commands[Type][Command];
		return (TRUE);
	}
	return (FALSE);
}

static int IsTarHeader(const BYTE *Data, int DataSize)
{
	size_t I;
	struct posix_header *Header;

	if (DataSize < (int)sizeof(struct posix_header))
		return (FALSE);

	Header = (struct posix_header *)Data;

	if (!strcmp(Header->magic, TMAGIC))
		TarArchiveFormat = POSIX_FORMAT;
	else if (!strcmp(Header->magic, OLDGNU_MAGIC))
		TarArchiveFormat = OLDGNU_FORMAT;
	else
		TarArchiveFormat = V7_FORMAT;

	for (I = 0; Header->name[I] && I < sizeof(Header->name); I++)
		if (Header->name[I] < ' ')
			return FALSE;

	// for (I=0; I < (&Header->typeflag - &Header->mode[0]); I++)
	for (I = 0; I < sizeof(Header->mode); I++) {
		int Mode = Header->mode[I];
		if (Mode > '7' || (Mode < '0' && Mode && Mode != ' '))
			return FALSE;
	}

	for (I = 0; Header->mtime[I] && I < sizeof(Header->mtime); I++)
		if (Header->mtime[I] < ' ')
			return FALSE;

	int64_t Sum = 256;
	for (I = 0; I <= 147; I++)
		Sum+= Data[I];

	for (I = 156; I < 512; I++)
		Sum+= Data[I];

	return (Sum == GetOctal(Header->chksum));
	/*
	  if(lstrcmp(Header->name,"././@LongLink"))
	  {
		int64_t Seconds=GetOctal(Header->mtime);
		if (Seconds < 300000000i64 || Seconds > 1500000000i64)
		{
		  if(Header->typeflag != DIRTYPE && Header->typeflag != SYMTYPE)
			return(FALSE);
		}
	  }
	  return(TRUE);
	*/
}

static int64_t GetOctal(const char *Str)
{
	return strtoll(Str, nullptr, 8);
}

static int64_t Oct2Size(const char *where0, size_t digs0)
{
	int64_t value;
	const char *where = where0;
	size_t digs = digs0;

	for (;;) {
		if (!digs) {
			return -1;
		}

		if (*where != ' ')
			break;
		where++;
		digs--;
	}

	value = 0;
	while (digs != 0 && *where >= '0' && *where <= '7') {
		if (((value << 3) >> 3) != value)
			return -1;	// goto out_of_range;
		value = (value << 3) | (*where++ - '0');
		--digs;
	}

	if (digs && *where && *where != ' ')
		return -1;

	return value;
}
