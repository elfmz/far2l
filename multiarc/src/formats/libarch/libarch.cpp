#include <unistd.h>
#include <memory>
#include <utils.h>

#include <windows.h>
#include "libarch_utils.h"

#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"

enum MenuFormats
{
	MF_TAR,
	MF_TARGZ,
	MF_CPIOGZ,

	MF_NOT_DISPLAYED = 0x10000
};

BOOL WINAPI _export LIBARCH_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	struct archive *a = archive_read_new();
	if (!a)
		return FALSE;

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);

	int r = LibArchCall(archive_read_open_memory, a, (void *)Data, (size_t)DataSize);
	if (r == ARCHIVE_OK) {
		archive_entry *ae = nullptr;
		LibArchCall(archive_read_next_header, a, &ae);
		if (archive_format(a) == ARCHIVE_FORMAT_RAW) {
			// allow only raw formats that has compressed filters,
			// otherwise any file will be represented as archive
			// that contains file's plain data
			r = ARCHIVE_EOF;
			for (int i = 0, ii = archive_filter_count(a); i < ii; ++i) {
				if (archive_filter_code(a, i) != 0) {
					r = ARCHIVE_OK;
					break;
				}
			}
		}
	}

	archive_read_free(a);
	fprintf(stderr, "PluginImplArc::sIsSupportedHeading: r=%d\n", r);


	return (r == ARCHIVE_OK) ? TRUE : FALSE;
}


static void ArcTM(FILETIME &dst, time_t sec, unsigned long nsec)
{
	timespec ts;
	ts.tv_sec = sec;
	ts.tv_nsec = nsec;

	WINPORT(FileTime_UnixToWin32)(ts, &dst);
}

static std::unique_ptr<LibArchOpenRead> s_arc;

BOOL WINAPI _export LIBARCH_OpenArchive(const char *Name, int *Type)
{
	try {
		LibArchOpenRead *arc = new LibArchOpenRead(Name);
		s_arc.reset(arc);
		*Type = (arc->Format() == ARCHIVE_FORMAT_TAR) ? MF_TAR : MF_NOT_DISPLAYED;
		if (arc->Format() == ARCHIVE_FORMAT_RAW) {
			for (int i = 0, ii = archive_filter_count(arc->Get()); i < ii; ++i) {
				int fc = archive_filter_code(arc->Get(), i);
				if (fc == ARCHIVE_FILTER_GZIP) {
					if (arc->Format() == ARCHIVE_FORMAT_CPIO) {
						*Type = MF_CPIOGZ;
					} else if (arc->Format() == ARCHIVE_FORMAT_TAR) {
						*Type = MF_TARGZ;
					}
				} else if (fc != 0) {
					*Type = MF_NOT_DISPLAYED;
				}
			}
		}

	} catch(std::exception &e) {
		fprintf(stderr, "LIBARCH_OpenArchive('%s'): %s\n", Name, e.what());
		return FALSE;
	}

	return TRUE;
}

DWORD WINAPI _export LIBARCH_GetSFXPos(void)
{
	return 0;
}

int WINAPI _export LIBARCH_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
	try {
		if (!s_arc)
			throw std::runtime_error("no archive opened");

		struct archive_entry *entry = s_arc->NextHeader();
		if (!entry)
			return GETARC_EOF;

		const char *pathname = archive_entry_pathname(entry);
		strncpy(Item->FindData.cFileName, pathname ? pathname : "", sizeof(Item->FindData.cFileName) - 1);

		uint64_t sz = archive_entry_size(entry);
		Item->PackSize = Item->FindData.nFileSizeLow = sz&0xffffffff;
		Item->PackSizeHigh = Item->FindData.nFileSizeHigh = (sz>>32)&0xffffffff;

		Item->FindData.dwUnixMode = archive_entry_mode(entry);
		Item->FindData.dwFileAttributes =
			WINPORT(EvaluateAttributesA)(Item->FindData.dwUnixMode, Item->FindData.cFileName);

		if (archive_entry_ctime_is_set(entry)) {
			ArcTM(Item->FindData.ftCreationTime, archive_entry_ctime(entry), archive_entry_ctime_nsec(entry));
		} else if (archive_entry_birthtime_is_set(entry)) {
			ArcTM(Item->FindData.ftCreationTime, archive_entry_birthtime(entry), archive_entry_birthtime_nsec(entry));
		} else if (archive_entry_mtime_is_set(entry)) {
			ArcTM(Item->FindData.ftCreationTime, archive_entry_mtime(entry), archive_entry_mtime_nsec(entry));
		}

		if (archive_entry_mtime_is_set(entry)) {
			ArcTM(Item->FindData.ftLastWriteTime, archive_entry_mtime(entry), archive_entry_mtime_nsec(entry));
		} else {
			Item->FindData.ftLastWriteTime = Item->FindData.ftCreationTime;
		}

		if (archive_entry_atime_is_set(entry)) {
			ArcTM(Item->FindData.ftLastAccessTime, archive_entry_atime(entry), archive_entry_atime_nsec(entry));
		} else {
			Item->FindData.ftLastAccessTime = Item->FindData.ftLastWriteTime;
		}

	} catch(std::exception &e) {
		fprintf(stderr, "LIBARCH_GetArcItem: %s\n", e.what());
		return GETARC_READERROR;
	}

	return GETARC_SUCCESS;
}

BOOL WINAPI _export LIBARCH_CloseArchive(struct ArcInfo *Info)
{
	s_arc.reset();
	return TRUE;
}


BOOL WINAPI _export LIBARCH_GetFormatName(int Type, char *FormatName, char *DefaultExt)
{
	static const char * const FmtAndExt[5][2]={
		{"TAR","tar"},
		{"TARGZ","tar.gz"},
		{"CPIOGZ","cpio"}
	};

	switch (Type) {
		case MF_TAR:
		case MF_TARGZ:
		case MF_CPIOGZ:
			strcpy(FormatName, FmtAndExt[Type][0]);
			strcpy(DefaultExt, FmtAndExt[Type][1]);
			return TRUE;

		case MF_NOT_DISPLAYED:
			strcpy(FormatName, "");
			strcpy(DefaultExt, "");
			return TRUE;
	}

  	return FALSE;
}

BOOL WINAPI _export LIBARCH_GetDefaultCommands(int Type, int Command, char *Dest)
{
	static const char *Commands[] = {
	/*Extract               */"^libarch X %%A %%FMq*4096",
	/*Extract without paths */"^libarch x %%A %%FMq*4096",
	/*Test                  */"^libarch t %%A",
	/*Delete                */"^libarch d %%A %%FMq*4096",
	/*Comment archive       */"",
	/*Comment files         */"",
	/*Convert to SFX        */"",
	/*Lock archive          */"",
	/*Protect archive       */"",
	/*Recover archive       */"",
	/*Add files             */"^libarch a:<<fmt>> %%A %%FMq*4096",
	/*Move files            */"^libarch m:<<fmt>> %%A %%FMq*4096",
	/*Add files and folders */"^libarch A:<<fmt>> %%A %%FMq*4096",
	/*Move files and folders*/"^libarch M:<<fmt>> %%A %%FMq*4096",
	/*"All files" mask      */"*"
	};

	if (Command < (int)ARRAYSIZE(Commands)) {
		std::string cmd = Commands[Command];
		size_t p = cmd.find(":<<fmt>>");
		if (p != std::string::npos) {
			switch (Type) {
				case MF_TAR: cmd.replace(p, 8, ":tar:plain"); break;
				case MF_TARGZ: cmd.replace(p, 8, ":tar:gz"); break;
				case MF_CPIOGZ: cmd.replace(p, 8, ":cpio:gz"); break;
				default: cmd.replace(p, 8, "");
			}
		}
		strcpy(Dest, cmd.c_str());
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool LIBARCH_CommandHandler(LibArchOpenRead &arc, const char *cmd, const char *wanted_path = nullptr);
bool LIBARCH_CommandHandler(LibArchOpenWrite &arc, const char *cmd, const char *wanted_path = nullptr);
bool LIBARCH_CommandDelete(const char *arc_path, int files_cnt, char *files[]);

template <class OPEN_T>
	static bool LIBARCH_Command( int numargs, char *args[])
{
	bool out = true;

	OPEN_T arc(args[2], args[1]);

	if (numargs == 3) {
		if (!LIBARCH_CommandHandler(arc, args[1])) {
			out = false;
		}

	} else for (int i = 3; i < numargs; ++i) {
		if (!LIBARCH_CommandHandler(arc, args[1], args[i])) {
			out = false;
		}
	}

	return out;
}

extern "C" int libarch_main(int numargs, char *args[])
{
	if (numargs < 3) {
		printf("Usage: ^arch <command> <archive_name> [OPTIONAL LIST OF FILES]\n\n"
			"<Commands>\n"
			"  t: Test integrity of archive\n"
			"  x: Extract files from archive (without using directory names)\n"
			"  X: eXtract files with full paths\n"
			"  d: delete files from archive\n"
			"  a: add files without paths\n"
			"  m: move files into archive without paths\n"
			"  A: add files with full paths\n"
			"  M: move files into archive with full paths\n");
		return 1;
	}

	int out = 0;
	try {
		switch (*(args[1])) {
			case 'd':
				if (!LIBARCH_CommandDelete(args[2], numargs - 3, &args[3])) {
					out = 1;
				}
				break;

			case 't': case 'x': case 'X':
				if (!LIBARCH_Command<LibArchOpenRead>(numargs, args)) {
					out = 1;
				}
				break;

			case 'a': case 'A': case 'm': case 'M':
				if (!LIBARCH_Command<LibArchOpenWrite>(numargs, args)) {
					out = 1;
				}
				break;

			default:
				throw std::runtime_error(std::string("bad command: ").append(args[1]));
		}

	} catch( std::exception &e) {
		fprintf(stderr, "Exception: %s\n", e.what());
		return -1;
	}

	return out;
}
