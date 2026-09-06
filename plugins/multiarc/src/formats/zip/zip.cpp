/*
  ZIP.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <vector>
#include <farplug-mb.h>
using namespace oldfar;
#include "fmt.hpp"

#if !defined(ZIP_LIBARCHIVE) && defined(HAVE_LIBARCHIVE)
#include <archive.h>
#if (ARCHIVE_VERSION_NUMBER >= 3002000)		// earlier libarchive has weak charsets support and doesnt support passwords at all
#define ZIP_LIBARCHIVE 1
#endif
#endif

#ifndef ZIP_LIBARCHIVE
#define ZIP_LIBARCHIVE 0
#endif

static void CPToUTF8(UINT cp, std::string &s)
{
	if (!s.empty()) {
		std::vector<wchar_t> buf((1 + s.size()) * 2 + 1);
		int r = WINPORT(MultiByteToWideChar)(cp, 0, s.c_str(), -1, &buf[0], buf.size() - 1);
		if (r >= 0) {
			Wide2MB(&buf[0], s);
		}
	}
}

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

static HANDLE ArcHandle;
static ULARGE_INTEGER SFXSize, NextPosition, FileSize;
static int ArcComment, FirstRecord;
static bool bTruncated;

struct ZipHeader
{
	DWORD Signature;
	WORD VerToExtract;
	WORD BitFlag;
	WORD Method;
	WORD LastModTime;
	WORD LastModDate;
	DWORD Crc32;
	DWORD SizeCompr;
	DWORD SizeUncompr;
	WORD FileNameLen;
	WORD ExtraFieldLen;
	// FileName[];
	// ExtraField[];
};

const size_t MIN_HEADER_LEN = sizeof(ZipHeader);

inline BOOL IsValidHeader(const unsigned char *Data, const unsigned char *DataEnd)
{
	ZipHeader *pHdr = (ZipHeader *)Data;
	// const WORD Zip64=45;
	return (0x04034b50 == LITEND(pHdr->Signature)
			&& (LITEND(pHdr->Method) < 20 || LITEND(pHdr->Method) == 98 || LITEND(pHdr->Method) == 99)
			&& (LITEND(pHdr->VerToExtract) & 0x00FF) < 0xFF		// version is in the low byte
			&& Data + MIN_HEADER_LEN + LITEND(pHdr->FileNameLen) + LITEND(pHdr->ExtraFieldLen) < DataEnd);
}

static ULONGLONG GetFilePosition(HANDLE Handle)
{
	ULARGE_INTEGER ul;
	ul.QuadPart = 0;
	ul.u.LowPart = WINPORT(SetFilePointer)(Handle, 0, (PLONG)&ul.u.HighPart, FILE_CURRENT);
	return ul.QuadPart;
}

static inline bool IsZIPFileMagic(const unsigned char *Data, int DataSize)
{
	return (DataSize >= 4 && Data[0] == 'P' && Data[1] == 'K'
			&& ((Data[2] == 3 && Data[3] == 4) || (Data[2] == 5 && Data[3] == 6)));
}

BOOL WINAPI _export ZIP_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	if (IsZIPFileMagic(Data, DataSize)) {
		SFXSize.QuadPart = 0;
		return (TRUE);
	}
	if (DataSize < (int)MIN_HEADER_LEN)
		return FALSE;

	if (!CanBeExecutableFileHeader(Data, DataSize) && DataSize > 0x1000)
		DataSize = 0x1000;

	const unsigned char *MaxData = Data + DataSize - MIN_HEADER_LEN;
	const unsigned char *DataEnd = Data + DataSize;
	for (const unsigned char *CurData = Data; CurData < MaxData; CurData++) {
		if (IsValidHeader(CurData, DataEnd)) {
			SFXSize.QuadPart = (DWORD)(CurData - Data);
			return (TRUE);
		}
	}
	return (FALSE);
}

BOOL WINAPI _export ZIP_OpenArchive(const char *Name, int *Type, bool Silent)
{
	ArcHandle = WINPORT(CreateFile)(MB2Wide(Name).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ArcHandle == INVALID_HANDLE_VALUE)
		return (FALSE);

	*Type = 0;

	ArcComment = FALSE;
	FirstRecord = TRUE;

	FileSize.u.LowPart = WINPORT(GetFileSize)(ArcHandle, &FileSize.u.HighPart);

	unsigned char ReadBuf[1024];
	DWORD ReadSize;
	int Buf;
	bool bFound = false, bLast = false;

	if (FileSize.QuadPart < sizeof(ReadBuf) - 18) {
		WINPORT(SetFilePointer)(ArcHandle, 0, NULL, FILE_BEGIN);
		bLast = true;
	} else
		WINPORT(SetFilePointer)(ArcHandle, -((signed)(sizeof(ReadBuf) - 18)), NULL, FILE_END);

	BYTE EOCD_Magic2 = 0x05;
	for (Buf = 0; Buf < 64 && !bFound; Buf++) {
		if (!WINPORT(ReadFile)(ArcHandle, ReadBuf, sizeof(ReadBuf), &ReadSize, NULL))
			break;
		for (int I = ((int)ReadSize) - 4; I >= 0; I--) {
			if (ReadBuf[I] == 0x50 && ReadBuf[I + 1] == 0x4B && ReadBuf[I + 3] == 0x06
					&& ReadBuf[I + 2] == EOCD_Magic2) {
				DWORD ReadSizeO = 0;
				if (EOCD_Magic2 == 0x06) {
					WINPORT(SetFilePointer)(ArcHandle, I + 48 - ReadSize, NULL, FILE_CURRENT);
					WINPORT(ReadFile)
					(ArcHandle, &NextPosition.QuadPart, sizeof(NextPosition.QuadPart), &ReadSizeO, NULL);
					LITEND_INPLACE(NextPosition.QuadPart);
					bFound = true;
					break;
				} else {
					WINPORT(SetFilePointer)(ArcHandle, I + 16 - ReadSize, NULL, FILE_CURRENT);
					WINPORT(ReadFile)
					(ArcHandle, &NextPosition.u.LowPart, sizeof(NextPosition.u.LowPart), &ReadSizeO, NULL);
					LITEND_INPLACE(NextPosition.u.LowPart);
					if (NextPosition.u.LowPart != 0xffffffff) {
						NextPosition.u.HighPart = 0;
						bFound = true;
						break;
					}
					WINPORT(SetFilePointer)
					(ArcHandle, -(LONG)(ReadSizeO + I + 16 - ReadSize), NULL, FILE_CURRENT);
					// continue searching, but for EOCD64
					EOCD_Magic2 = 0x06;
				}
			}
		}
		if (bFound || bLast)
			break;

		if (WINPORT(SetFilePointer)(ArcHandle, -((signed)(sizeof(ReadBuf) - 4)) - ((signed)(ReadSize)), NULL,
					FILE_CURRENT)
						== INVALID_SET_FILE_POINTER
				&& WINPORT(GetLastError)() != NO_ERROR) {
			WINPORT(SetFilePointer)(ArcHandle, 0, NULL, FILE_BEGIN);
			bLast = true;
		}
	}

	bTruncated = !bFound;
	if (bTruncated)
		NextPosition.QuadPart = SFXSize.QuadPart;
	return (TRUE);
}

int WINAPI _export ZIP_GetArcItem(struct ArcItemInfo *Info)
{
	struct ZipHd1
	{
		DWORD Mark;
		BYTE UnpVer;
		BYTE UnpOS;
		WORD Flags;
		WORD Method;
		DWORD ftime;
		DWORD CRC;
		DWORD PackSize;
		DWORD UnpSize;
		WORD NameLen;
		WORD AddLen;
	} ZipHd1;

	struct ZipHd2
	{
		DWORD Mark;
		BYTE PackVer;
		BYTE PackOS;
		BYTE UnpVer;
		BYTE UnpOS;
		WORD Flags;
		WORD Method;
		DWORD ftime;
		DWORD CRC;
		DWORD PackSize;
		DWORD UnpSize;
		WORD NameLen;
		WORD AddLen;
		WORD CommLen;
		WORD DiskNum;
		WORD ZIPAttr;
		DWORD Attr;
		DWORD Offset;
	} ZipHeader;

	DWORD ReadSize;

	NextPosition.u.LowPart = WINPORT(
			SetFilePointer)(ArcHandle, NextPosition.u.LowPart, (PLONG)&NextPosition.u.HighPart, FILE_BEGIN);
	if (NextPosition.u.LowPart == INVALID_SET_FILE_POINTER && WINPORT(GetLastError)() != NO_ERROR)
		return (GETARC_READERROR);
	if (NextPosition.QuadPart > FileSize.QuadPart) {
		fprintf(stderr, "ZIP_GetArcItem: NextPosition=0x%llx > FileSize=0x%llx\n",
				(unsigned long long)NextPosition.QuadPart, (unsigned long long)FileSize.QuadPart);
		return (GETARC_UNEXPEOF);
	}

	if (bTruncated) {
		if (!WINPORT(ReadFile)(ArcHandle, &ZipHd1, sizeof(ZipHd1), &ReadSize, NULL))
			return (GETARC_READERROR);
		ZeroFill(ZipHeader);
		ZipHeader.Mark = ZipHd1.Mark;
		ZipHeader.UnpVer = ZipHd1.UnpVer;
		ZipHeader.UnpOS = ZipHd1.UnpOS;
		ZipHeader.Flags = ZipHd1.Flags;
		ZipHeader.Method = ZipHd1.Method;
		ZipHeader.ftime = ZipHd1.ftime;
		ZipHeader.PackSize = ZipHd1.PackSize;
		ZipHeader.UnpSize = ZipHd1.UnpSize;
		ZipHeader.NameLen = ZipHd1.NameLen;
		ZipHeader.AddLen = ZipHd1.AddLen;
	} else {
		if (!WINPORT(ReadFile)(ArcHandle, &ZipHeader, sizeof(ZipHeader), &ReadSize, NULL))
			return (GETARC_READERROR);
		if (LITEND(ZipHeader.Mark) != 0x02014b50 && LITEND(ZipHeader.Mark) != 0x06054b50
				&& LITEND(ZipHeader.Mark) != 0x06064b50 && LITEND(ZipHeader.Mark) != 0x07064b50) {
			if (FirstRecord) {
				if (SFXSize.QuadPart > 0) {
					NextPosition.QuadPart+= SFXSize.QuadPart;
					WINPORT(SetFilePointer)
					(ArcHandle, NextPosition.u.LowPart, (PLONG)&NextPosition.u.HighPart, FILE_BEGIN);
					if (!WINPORT(ReadFile)(ArcHandle, &ZipHeader, sizeof(ZipHeader), &ReadSize, NULL))
						return (GETARC_READERROR);
				}
				if (LITEND(ZipHeader.Mark) != 0x02014b50 && LITEND(ZipHeader.Mark) != 0x06054b50) {
					bTruncated = true;
					NextPosition.QuadPart = SFXSize.QuadPart;
					return (ZIP_GetArcItem(Info));
				}
			} else {
				fprintf(stderr, "ZIP_GetArcItem: unexpected ZipHeader.Mark=0x%x\n", LITEND(ZipHeader.Mark));
				return (GETARC_UNEXPEOF);
			}
		}
	}

	FirstRecord = FALSE;

	if (ReadSize == 0 || LITEND(ZipHeader.Mark) == 0x06054b50
			|| (bTruncated && LITEND(ZipHeader.Mark) == 0x02014b50)) {
		if (!bTruncated && *(WORD *)((char *)&ZipHeader + 20) != 0)
			ArcComment = TRUE;
		return (GETARC_EOF);
	}
	if (LITEND(ZipHeader.Mark) == 0x06064b50)		// EOCD64
	{
		return (GETARC_EOF);
	}
	if (LITEND(ZipHeader.Mark) == 0x07064b50)		// EOCD64Locator
	{
		NextPosition.QuadPart+= 20;
		return ZIP_GetArcItem(Info);
	}

	char cFileName[NM + 1] = {0};
	DWORD SizeToRead = std::min(DWORD(LITEND(ZipHeader.NameLen)), DWORD(ARRAYSIZE(cFileName) - 1));
	if (!WINPORT(ReadFile)(ArcHandle, cFileName, SizeToRead, &ReadSize, NULL) || ReadSize != SizeToRead)
		return (GETARC_READERROR);

	Info->PathName.assign(cFileName, strnlen(cFileName, ReadSize));
	Info->nFileSize = LITEND(ZipHeader.UnpSize);
	Info->nPhysicalSize = LITEND(ZipHeader.PackSize);
	Info->CRC32 = LITEND(ZipHeader.CRC);
	FILETIME lft;
	WINPORT(DosDateTimeToFileTime)(HIWORD(LITEND(ZipHeader.ftime)), LOWORD(LITEND(ZipHeader.ftime)), &lft);
	WINPORT(LocalFileTimeToFileTime)(&lft, &Info->ftLastWriteTime);
	if (LITEND(ZipHeader.Flags) & 1)
		Info->Encrypted = TRUE;
	if (LITEND(ZipHeader.CommLen) > 0)
		Info->Comment = TRUE;
	static const char *ZipOS[] = {"DOS", "Amiga", "VAX/VMS", "Unix", "VM/CMS", "Atari ST", "OS/2", "Mac-OS",
			"Z-System", "CP/M", "TOPS-20", "Win32", "SMS/QDOS", "Acorn RISC OS", "Win32 VFAT", "MVS", "BeOS",
			"Tandem"};
	if (ZipHeader.PackOS < ARRAYSIZE(ZipOS))
		Info->HostOS = ZipOS[ZipHeader.PackOS];

	Info->Codepage = 0;

	//  if (ZipHeader.PackOS==11 && ZipHeader.PackVer>20 && ZipHeader.PackVer<25)
	if (LITEND(ZipHeader.Flags) & 0x800) {	// Bit 11 - language encoding flag (EFS) - means filename&comment fields are UTF8
		;

	} else if (ZipHeader.PackOS == 11 && ZipHeader.PackVer >= 20) {		// && ZipHeader.PackVer<25
        // NTFS of Windows (11) and packer version >= 20, both headers in ANSI

		CPToUTF8(CP_ACP, Info->PathName);
		Info->Codepage = WINPORT(GetACP)();

	} else if (ZipHeader.PackOS == 0 && ZipHeader.PackVer >= 25 && ZipHeader.PackVer <= 40) {
        // Special case: PKZIP for Windows 2.5, 2.6, 4.0
        // Local header in ANSI, central header in OEM

		// See InfoZip's unzip source code.
		// File name is unzpriv.h, search for "Convert filename (and file comment string)"

		CPToUTF8(CP_OEMCP, Info->PathName);
		// libarchive use local header, we use central header. libarchive needs another charset here
		Info->Codepage = WINPORT(GetACP)();

	} else if (ZipHeader.PackOS == 11 || ZipHeader.PackOS == 6 || ZipHeader.PackOS == 0) {
        // NTFS of Windows (11), HPFS of OS/2 (6), FAT of DOS (0)

		CPToUTF8(CP_OEMCP, Info->PathName);
		Info->Codepage = WINPORT(GetOEMCP)();
	} // In all other cases do not touch encoding, assume system one

	Info->UnpVer = (ZipHeader.UnpVer / 10) * 256 + (ZipHeader.UnpVer % 10);
	Info->DictSize = 32;

	if ((ZipHeader.PackOS == 3 || ZipHeader.PackOS == 7) && (LITEND(ZipHeader.Attr) & 0xffff0000) != 0) {
		Info->dwUnixMode = LITEND(ZipHeader.Attr) >> 16;
		if ((Info->dwUnixMode & S_IFMT) == 0) {
			Info->dwUnixMode|= S_IFREG;
		}
		Info->dwFileAttributes = WINPORT(EvaluateAttributesA)(Info->dwUnixMode, Info->PathName.c_str());
	} else {
		Info->dwUnixMode = 0;
		Info->dwFileAttributes = LITEND(ZipHeader.Attr) & 0x3f;
	}

	Info->Description.reset();

	// Search for extra block
	ULARGE_INTEGER ExtraFieldEnd;

	for (ExtraFieldEnd.QuadPart = GetFilePosition(ArcHandle) + LITEND(ZipHeader.AddLen);
			ExtraFieldEnd.QuadPart > GetFilePosition(ArcHandle);) {
		struct ExtraBlockHeader
		{
			WORD Type;
			WORD Length;
		} BlockHead;

		if (!WINPORT(ReadFile)(ArcHandle, &BlockHead, sizeof(BlockHead), &ReadSize, NULL)
				|| ReadSize != sizeof(BlockHead))
			return (GETARC_READERROR);

		//  fprintf(stderr, "BlockHead.Type=0x%x Length=0x%x\n", BlockHead.Type, BlockHead.Length);
		if (0xA == LITEND(BlockHead.Type))								// NTFS Header ID
		{
			WINPORT(SetFilePointer)(ArcHandle, 4, NULL, FILE_CURRENT);	// Skip the reserved 4 bytes

			ULARGE_INTEGER NTFSExtraBlockEnd;
			// Search for file times attribute
			for (NTFSExtraBlockEnd.QuadPart = GetFilePosition(ArcHandle) - 4 + LITEND(BlockHead.Length);
					NTFSExtraBlockEnd.QuadPart > GetFilePosition(ArcHandle);) {
				struct NTFSAttributeHeader
				{
					WORD Tag;
					WORD Length;
				} AttrHead;

				if (!WINPORT(ReadFile)(ArcHandle, &AttrHead, sizeof(AttrHead), &ReadSize, NULL)
						|| ReadSize != sizeof(AttrHead))
					return (GETARC_READERROR);

				if (1 != LITEND(AttrHead.Tag)) {	// File times attribute tag
					// Move to attribute end
					WINPORT(SetFilePointer)(ArcHandle, LITEND(AttrHead.Length), NULL, FILE_CURRENT);
				} else {	// Read file times
					struct TimesAttribute
					{
						FILETIME Modification;
						FILETIME Access;
						FILETIME Creation;
					} Times;

					if (!WINPORT(ReadFile)(ArcHandle, &Times, sizeof(Times), &ReadSize, NULL)
							|| ReadSize != sizeof(Times))
						return (GETARC_READERROR);

					LITEND_INPLACE_FILETIME(Times.Modification);
					Info->ftLastWriteTime = Times.Modification;

					LITEND_INPLACE_FILETIME(Times.Access);
					Info->ftLastAccessTime = Times.Access;

					LITEND_INPLACE_FILETIME(Times.Creation);
					Info->ftCreationTime = Times.Creation;
				}
			}
		} else if (0x1 == LITEND(BlockHead.Type))		// ZIP64
		{
			struct ZIP64Descriptor
			{
				ULARGE_INTEGER OriginalSize;			//    8 bytes               Original uncompressed file size
				ULARGE_INTEGER CompressedSize;			//    8 bytes               Size of compressed data
				ULARGE_INTEGER RelativeHeaderOffset;	//    8 bytes               Offset of local header record
				DWORD DiskStartNumber;					//    4 bytes               Number of the disk on which this file starts
			} ZIP64;

			if (!WINPORT(ReadFile)(ArcHandle, &ZIP64, std::min(LITEND(BlockHead.Length), (WORD)sizeof(ZIP64)),
						&ReadSize, NULL)
					|| ReadSize != LITEND(BlockHead.Length)) {
				return (GETARC_READERROR);
			}
			if (LITEND(BlockHead.Length) == sizeof(ZIP64)) {
				Info->nFileSize = LITEND(ZIP64.OriginalSize.QuadPart);
				Info->nPhysicalSize = LITEND(ZIP64.CompressedSize.QuadPart);
			} else if (LITEND(BlockHead.Length) != 8)		// if size is 8 - then it contains only RelativeHeaderOffset
			{
				fprintf(stderr, "Unexpected ZIP64 block length %u\n", LITEND(BlockHead.Length));
			}
		} else if ((0x7075 == LITEND(BlockHead.Type) || 0x6375 == LITEND(BlockHead.Type))		// Unicode Path Extra Field || Unicode Comment Extra Field
				&& LITEND(BlockHead.Length) > sizeof(uint8_t) + sizeof(uint32_t)) {
			uint8_t version = 0;
			uint32_t strcrc = 0;
			std::vector<char> strbuf(LITEND(BlockHead.Length) - sizeof(uint32_t));
			if (!WINPORT(ReadFile)(ArcHandle, &version, sizeof(version), &ReadSize, NULL)
					|| ReadSize != sizeof(version))
				return (GETARC_READERROR);
			if (!WINPORT(ReadFile)(ArcHandle, &strcrc, sizeof(strcrc), &ReadSize, NULL)
					|| ReadSize != sizeof(strcrc))
				return (GETARC_READERROR);
			if (!WINPORT(ReadFile)(ArcHandle, strbuf.data(), strbuf.size() - 1, &ReadSize, NULL)
					|| ReadSize != strbuf.size() - 1)
				return (GETARC_READERROR);
			if (version != 1)
				fprintf(stderr, "ZIP: Unicode Extra Field 0x%x unknown version %u\n", LITEND(BlockHead.Type),
						version);

			if (0x7075 == LITEND(BlockHead.Type)) {
				Info->PathName = strbuf.data();
#if ZIP_LIBARCHIVE	// if libarchive not used need to pass non-UTF8 codepage to zip/unzip workarounds
				Info->Codepage = 0;
#endif
			} else {
				Info->Description.reset(new std::string(strbuf.data()));
			}
		} else	// Move to extra block end
			WINPORT(SetFilePointer)(ArcHandle, LITEND(BlockHead.Length), NULL, FILE_CURRENT);
	}
	// ZipHeader.AddLen is more reliable than the sum of all LITEND(BlockHead.Length)
	WINPORT(SetFilePointer)(ArcHandle, ExtraFieldEnd.u.LowPart, (PLONG)&ExtraFieldEnd.u.HighPart, FILE_BEGIN);
	// End of NTFS file times support

	// Read the in-archive file comment if any
	if (LITEND(ZipHeader.CommLen) > 0) {
		ReadSize = 0;
		if (!Info->Description)		// we could already get UTF-8 description
		{
			char Description[255];
			DWORD SizeToRead = std::min(DWORD(LITEND(ZipHeader.CommLen)), DWORD(ARRAYSIZE(Description)));
			if (!WINPORT(ReadFile)(ArcHandle, Description, SizeToRead, &ReadSize, NULL)
					|| ReadSize != SizeToRead)
				return (GETARC_READERROR);
			Info->Description.reset(new std::string(Description, ReadSize));
		}
		// Skip comment tail
		WINPORT(SetFilePointer)(ArcHandle, LITEND(ZipHeader.CommLen) - ReadSize, NULL, FILE_CURRENT);
	}

	ULARGE_INTEGER SeekLen;
	SeekLen.QuadPart = bTruncated ? Info->nPhysicalSize : 0;
	WINPORT(SetFilePointer)(ArcHandle, SeekLen.u.LowPart, (PLONG)&SeekLen.u.HighPart, FILE_CURRENT);
	NextPosition.QuadPart = GetFilePosition(ArcHandle);
	return (GETARC_SUCCESS);
}

BOOL WINAPI _export ZIP_CloseArchive(struct ArcInfo *Info)
{
	if (Info) {
		Info->SFXSize = (int)SFXSize.QuadPart;
		Info->Comment = ArcComment;
	}
	return (WINPORT(CloseHandle)(ArcHandle));
}

DWORD WINAPI _export ZIP_GetSFXPos(void)
{
	return (DWORD)SFXSize.QuadPart;
}

BOOL WINAPI _export ZIP_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	if (Type == 0) {
		FormatName = "ZIP";
		DefaultExt = "zip";
		return (TRUE);
	}
	return (FALSE);
}

BOOL WINAPI _export ZIP_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	if (Type == 0) {
#if ZIP_LIBARCHIVE
		// Console PKZIP 4.0/Win32 commands
		static const char *Commands[] = {/*Extract               */ "^libarch X %%A -@%%R {-cs=%%T} "
																	"{-pwd=%%P} -- %%FMq4096",
				/*Extract without paths */ "^libarch x %%A {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Test                  */ "^libarch t %%A {-cs=%%T}",
				/*Delete                */ "^libarch d %%A {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Comment archive       */ "",
				/*Comment files         */ "",
				/*Convert to SFX        */ "",
				/*Lock archive          */ "",
				/*Protect archive       */ "",
				/*Recover archive       */ "",
				/*Add files             */ "^libarch a:zip %%A -@%%R {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Move files            */ "^libarch m:zip %%A -@%%R {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Add files and folders */ "^libarch A:zip %%A -@%%R {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*Move files and folders*/ "^libarch M:zip %%A -@%%R {-cs=%%T} {-pwd=%%P} -- %%FMq4096",
				/*"All files" mask      */ ""};
#else
		// Linux zip/unzip
		static const char *Commands[] = {/*Extract               */ "unzip -o {-P %%P} %%A %%FMq4096",
				/*Extract without paths */ "unzip -j -o {-P %%P} %%A %%FMq4096",
				/*Test                  */ "unzip -t {-P %%P} %%A",
				/*Delete                */ "zip -d {-P %%P} %%A %%FMq4096",
				/*Comment archive       */ "",
				/*Comment files         */ "",
				/*Convert to SFX        */ "",
				/*Lock archive          */ "",
				/*Protect archive       */ "",
				/*Recover archive       */ "",
				/*Add files             */ "zip {-P %%P} %%A %%Fq4096",
				/*Move files            */ "zip -m {-P %%P} %%A %%Fq4096",
				/*Add files and folders */ "zip -r {-P %%P} %%A %%Fq4096",
				/*Move files and folders*/ "zip -r -m {-P %%P} %%A %%Fq4096",
				/*"All files" mask      */ "*"};
#endif
		if (Command < (int)(ARRAYSIZE(Commands))) {
			Dest = Commands[Command];
			return (TRUE);
		}
	}
	return (FALSE);
}
