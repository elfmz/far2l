#ifndef __FARFMT_HPP__
#define __FARFMT_HPP__
/*
  FMT.HPP

  Archive Support API for FAR Manager 1.75 and MultiArc plugin

  Copyright (c) 1996-2000 Eugene Roshal
  Copyrigth (c) 2000-2008 FAR group
*/

#if !defined(_WIN64)
#if defined(__BORLANDC__)
#pragma option -a2
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
#pragma pack(2)
#else
#pragma pack(push, 2)
#if _MSC_VER
#ifdef _export
#undef _export
#endif
#define _export
#endif
#endif
#endif

#include "../../WinPort/WinCompat.h"
#include <string>
#include <memory>
#include <utils.h>

enum GETARC_CODE
{
	GETARC_EOF       = 0,
	GETARC_SUCCESS   = 1,
	GETARC_BROKEN    = 2,
	GETARC_UNEXPEOF  = 3,
	GETARC_READERROR = 4,
};

struct ArcItemAttributes
{
	int Solid{};
	int Comment{};
	int Encrypted{};
	int DictSize{};
	int UnpVer{};
	int Chapter{};
	int Codepage{};

	DWORD dwFileAttributes{};
	DWORD dwUnixMode{};
	DWORD Flags{};
	DWORD NumberOfLinks{};
	DWORD CRC32{};

	FILETIME ftCreationTime{};
	FILETIME ftLastAccessTime{};
	FILETIME ftLastWriteTime{};
	DWORD64 nPhysicalSize{};
	DWORD64 nFileSize{};

	// NULL or ptr to statically alloc'ed literal - no need to free
	const char *HostOS{};

	// keep rarely used strings in std::unique_ptr to save memory
	std::unique_ptr<std::string> Description;
	std::unique_ptr<std::string> LinkName;
	std::unique_ptr<std::string> Prefix;
};

struct ArcItemInfo : ArcItemAttributes
{
	std::string PathName;
};

enum ARCINFO_FLAGS
{
	AF_AVPRESENT    = 0x00000001,
	AF_IGNOREERRORS = 0x00000002,
	AF_HDRENCRYPTED = 0x00000080,
};

struct ArcInfo
{
	int SFXSize;
	int Volume;
	int Comment;
	int Recovery;
	int Lock;
	DWORD Flags;
	DWORD Reserved;
	int Chapters;
};

#if defined(__BORLANDC__) || defined(_MSC_VER) || defined(__GNUC__) || defined(__WATCOMC__)
#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI _export LoadFormatModule(const char *ModuleName);
void WINAPI _export SetFarInfo(const struct PluginStartupInfo *Info);

BOOL WINAPI _export IsArchive(const char *Name, const unsigned char *Data, int DataSize);
DWORD WINAPI _export GetSFXPos(void);
BOOL WINAPI _export OpenArchive(const char *Name, int *TypeArc);
int WINAPI _export GetArcItem(struct ArcItemInfo *Info);
BOOL WINAPI _export CloseArchive(struct ArcInfo *Info);
BOOL WINAPI _export GetFormatName(int TypeArc, std::string &FormatName, std::string &DefaultExt);
BOOL WINAPI _export GetDefaultCommands(int TypeArc, int Command, std::string &Dest);

#ifdef __cplusplus
};
#endif
#endif

#if !defined(_WIN64)
#if defined(__BORLANDC__)
#pragma option -a.
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
#pragma pack()
#else
#pragma pack(pop)
#endif
#endif

bool CanBeExecutableFileHeader(const unsigned char *Data, int DataSize);

#endif	/* __FARFMT_HPP__ */
