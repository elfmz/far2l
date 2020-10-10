#include <unistd.h>
#include <memory>
#include <utils.h>
#include <sudo.h>
#include <fcntl.h>
#include <locale.h>

#include <windows.h>
#include "libarch_utils.h"

#include <pluginold.hpp>
using namespace oldfar;

#include "fmt.hpp"

#include "libarch_cmd.h"

enum MenuFormats
{
	MF_TAR,
	MF_TARGZ,
	MF_CPIOGZ,
	MF_CAB,
	MF_ISO,

	MF_NOT_DISPLAYED = 0x10000
};

struct IsArchiveContext
{
	const char *Name;
	const unsigned char *Head;
	size_t HeadSize;

	off_t FileSize;
	off_t Pos;
	int fd;

	unsigned char Temp[0x1000];
};

static int IsArchive_Close(struct archive *a, void *data)
{
	((IsArchiveContext *)data)->Pos = 0;
	return ARCHIVE_OK;
}

static int64_t IsArchive_Seek(struct archive *, void *data, int64_t offset, int whence)
{
	IsArchiveContext *ctx = (IsArchiveContext *)data;
	if (whence == SEEK_SET) {
		ctx->Pos = offset;

	} else if (whence == SEEK_END) {
		ctx->Pos = ctx->FileSize + offset;

	} else {
		ctx->Pos+= offset;
	}

	if (ctx->Pos > ctx->FileSize) {
		ctx->Pos = ctx->FileSize;
	}

	return ctx->Pos;
}

static int64_t IsArchive_Skip(struct archive *, void *data, int64_t request)
{
	IsArchiveContext *ctx = (IsArchiveContext *)data;
	int64_t avail = ctx->FileSize - ctx->Pos;
	if (request > avail) {
		request = (avail > 0) ? avail : 0;
	}
	ctx->Pos+= request;
	return request;
}

static ssize_t IsArchive_Read(struct archive *, void *data, const void **buff)
{
	IsArchiveContext *ctx = (IsArchiveContext *)data;

	if (ctx->Pos < (off_t)ctx->HeadSize) {
		*buff = ctx->Head + (size_t)ctx->Pos;
		ssize_t r = ctx->HeadSize - ctx->Pos;
		ctx->Pos = ctx->HeadSize;
		return r;
	}

	if (ctx->fd == -1) {
		// fprintf(stderr, "IsArchive_Read: opening %s\n", ctx->Name);
		ctx->fd = sdc_open(ctx->Name, O_RDONLY);
		if (ctx->fd == -1) {
			return ARCHIVE_FATAL;
		}
	}

	if (ctx->Pos > ctx->FileSize) {
		ctx->Pos = ctx->FileSize;
	}

	size_t piece = (ctx->FileSize - ctx->Pos > (off_t)sizeof(ctx->Temp))
		? sizeof(ctx->Temp) : (size_t)(ctx->FileSize - ctx->Pos);

	if (ctx->fd != -1) {
		ssize_t r = sdc_pread(ctx->fd, ctx->Temp, sizeof(ctx->Temp), ctx->Pos );
		if (r >= 0) {
			piece = r;
		}
	}

	*buff = ctx->Temp;
	ctx->Pos+= piece;

	return piece;
}

BOOL WINAPI _export LIBARCH_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
	struct archive *a = archive_read_new();
	if (!a)
		return FALSE;

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);

	IsArchiveContext ctx = {Name, Data, (size_t)DataSize, (off_t)DataSize, 0, -1, {}};
	struct stat s{};
	if (Name && sdc_stat(Name, &s) == 0) {
		ctx.FileSize = s.st_size;
	}

	archive_read_set_read_callback(a, IsArchive_Read);
	archive_read_set_seek_callback(a, IsArchive_Seek);
	archive_read_set_skip_callback(a, IsArchive_Skip);
	archive_read_set_close_callback(a, IsArchive_Close);
	archive_read_set_callback_data(a, &ctx);

	int r = archive_read_open1(a);
	if (r == ARCHIVE_OK || r == ARCHIVE_WARN) {
		archive_entry *ae = nullptr;
		LibArchCall(archive_read_next_header, a, &ae);
		if (LibArch_DetectedFormatHasCompression(a)) {
			r = ARCHIVE_OK;
		} else {
			r = ARCHIVE_EOF;
		}
		archive_read_close(a);
	}

	archive_read_free(a);

	if (ctx.fd != -1) {
		sdc_close(ctx.fd);
	}

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
		LibArchOpenRead *arc = new LibArchOpenRead(Name, "", "");
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

		} else if (arc->Format() == ARCHIVE_FORMAT_ISO9660) {
			*Type = MF_ISO;

		} else if (arc->Format() == ARCHIVE_FORMAT_CAB) {
			*Type = MF_CAB;
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

		const char *pathname;
		struct archive_entry *entry;


		for (;;) {
			entry = s_arc->NextHeader();
			if (!entry)
				return GETARC_EOF;

			pathname = LibArch_EntryPathname(entry);
			if (pathname && *pathname && strcmp(pathname, ".") != 0 && strcmp(pathname, "..") != 0) {
				break;
			}
		}
		strncpy(Item->FindData.cFileName, pathname, sizeof(Item->FindData.cFileName) - 1);

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
		{"CPIOGZ","cpio"},
		{"CAB","cab"},
		{"ISO","iso"}
	};

	switch (Type) {
		case MF_TAR:
		case MF_TARGZ:
		case MF_CPIOGZ:
		case MF_CAB:
		case MF_ISO:
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
	/*Extract               */"^libarch X %%A -- %%FMq4096",
	/*Extract without paths */"^libarch x %%A -- %%FMq4096",
	/*Test                  */"^libarch t %%A",
	/*Delete                */"^libarch d %%A -- %%FMq4096",
	/*Comment archive       */"",
	/*Comment files         */"",
	/*Convert to SFX        */"",
	/*Lock archive          */"",
	/*Protect archive       */"",
	/*Recover archive       */"",
	/*Add files             */"^libarch a:<<fmt>> %%A -@%%R -- %%FMq4096",
	/*Move files            */"^libarch m:<<fmt>> %%A -@%%R -- %%FMq4096",
	/*Add files and folders */"^libarch A:<<fmt>> %%A -@%%R -- %%FMq4096",
	/*Move files and folders*/"^libarch M:<<fmt>> %%A -@%%R -- %%FMq4096",
	/*"All files" mask      */""
	};

	static const char *CommandsCab[] = {
	/*Extract               */"^libarch X %%A -- %%FMq4096",
	/*Extract without paths */"^libarch x %%A -- %%FMq4096",
	/*Test                  */"^libarch t %%A",
	/*Delete                */"^libarch d %%A -- %%FMq4096",
	/*Comment archive       */"",
	/*Comment files         */"",
	/*Convert to SFX        */"",
	/*Lock archive          */"",
	/*Protect archive       */"",
	/*Recover archive       */"",
	/*Add files             */"lcab %%FMq4096 %%A",
	/*Move files            */"",
	/*Add files and folders */"lcab -r %%FMq4096 %%A",
	/*Move files and folders*/"",
	/*"All files" mask      */"." // lcab  picky regarding ending dir with /
	};

	if (Command >= (int)ARRAYSIZE(Commands)) {
		return FALSE;
	}

	if (Type == MF_CAB) {
		strcpy(Dest, CommandsCab[Command]);
		return TRUE;
	}

	std::string cmd = Commands[Command];
	size_t p = cmd.find(":<<fmt>>");
	if (p != std::string::npos) {
		switch (Type) {
			case MF_TAR: cmd.replace(p, 8, ":tar:plain"); break;
			case MF_TARGZ: cmd.replace(p, 8, ":tar:gz"); break;
			case MF_CPIOGZ: cmd.replace(p, 8, ":cpio:gz"); break;
			case MF_ISO: cmd.replace(p, 8, ":iso"); break;
			default: cmd.replace(p, 8, "");
		}
	}
	strcpy(Dest, cmd.c_str());
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

extern "C" int libarch_main(int numargs, char *args[])
{
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "UTF-8");

	if (numargs < 3) {
		printf("Usage: ^arch <command> <archive_name> [-pwd=OPTIONAL PASSWORD] [-cs=OPTIONAL CHARSET] [-@OPTIONAL ARCHIVE ROOT] [--] [OPTIONAL LIST OF FILES]\n\n"
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

	try {
		bool ok;

		int files_cnt = numargs - 3;
		char **files = &args[3];
		LibarchCommandOptions arc_opts;
		while (files_cnt > 0 && files[0][0] == '-') {
			if (files[0][1] == '@') {
				arc_opts.root_path = files[0] + 2;

			} else if (strncmp(files[0], "-cs=", 4) == 0) {
				arc_opts.charset = files[0] + 4;

			} else if (strncmp(files[0], "-pwd=", 5) == 0) {
				LibArch_SetPassprhase(files[0] + 5);

			} else {
				if (files[0][1] == '-') {
					++files;
					--files_cnt;
				} else
					fprintf(stderr, "Unknown option: %s - treating as file\n", files[0]);

				break;
			}
			++files;
			--files_cnt;
		}

		switch (*(args[1])) {
			case 't': case 'x': case 'X':
				ok = LIBARCH_CommandRead(args[1], args[2], arc_opts, files_cnt, files);
				break;

			case 'd':
				ok = LIBARCH_CommandDelete(args[1], args[2], arc_opts, files_cnt, files);
				break;

			case 'a': case 'A': case 'm': case 'M':
				ok = LIBARCH_CommandAdd(args[1], args[2], arc_opts, files_cnt, files);
				break;

			default:
				throw std::runtime_error(std::string("bad command: ").append(args[1]));
		}

		return ok ? 0 : 1;

	} catch( std::exception &e) {
		fprintf(stderr, "Exception: %s\n", e.what());
		return -1;
	}
}

