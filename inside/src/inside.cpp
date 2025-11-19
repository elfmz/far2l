#include "Globals.h"
#include "plain/PluginImplPlain.h"
#include <fcntl.h>
#ifndef __APPLE__
# include "elf/PluginImplELF.h"
# include <elf.h>
#endif
# include "Storage.h"
#include <sudo.h>
#include <utils.h>

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
	G.plugin_path = path;
//	fprintf(stderr, "Inside::PluginModuleOpen\n");
}

SHAREDSYMBOL int WINAPI _export GetMinFarVersion(void)
{
	#define MAKEFARVERSION(major,minor) ( ((major)<<16) | (minor))
	return MAKEFARVERSION(2, 1);
}


SHAREDSYMBOL void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
	G.Startup(Info);
//	fprintf(stderr, "Inside::SetStartupInfo\n");
}

#define MINIMAL_LEN	0x34

static bool HasElvenEars(const unsigned char *Data, int DataSize)
{
#ifndef __APPLE__
	//FAR reads at least 0x1000 bytes if possible, so may assume full ELF header available if its there
	if (DataSize < MINIMAL_LEN || Data[0] != 0x7F || Data[1] != 'E'|| Data[2] != 'L'|| Data[3] != 'F')
		return false;
	if (Data[4] != 1 && Data[4] != 2) // Bitness: 32/64
		return false;
	if (Data[5] != 0 && Data[5] != 1 && Data[5] != 2) // Endianness: None(?)/LSB/MSB
		return false;
	if (Data[6] != 1) // Version: 1
		return false;

	return true;
#else
	return false;
#endif
}

static const char *DetectPlainKind(const char *Name, const unsigned char *Data, int DataSize)
{
	const char *ext = Name ? strrchr(Name, '.') : nullptr;
	// %PDF-1.2
	if (DataSize >= 8 && Data[0] == '%' && Data[1] == 'P' && Data[2] == 'D' && Data[3] == 'F' &&
		Data[4] == '-' && Data[5] >= '0' && Data[5] <= '9' && Data[6] == '.' && Data[7] >= '0' && Data[7] <= '9') {
		return "PDF";

	} else if (DataSize >= 8 && Data[0] == 0xd0 && Data[1] == 0xcf && Data[2] == 0x11 && Data[3] == 0xE0 &&
		Data[4] == 0xA1 && Data[5] == 0xB1 && Data[6] == 0x1A && Data[7] == 0xE1) {
		if (!ext || strcasecmp(ext, ".doc") == 0) { // disinct from excel/ppt etc
			return "DOC";
		}

		if (strcasecmp(ext, ".ppt") == 0) {
			return "PPT";
		}

		if (strcasecmp(ext, ".xls") == 0) {
			return "XLS";
		}

	} else if (DataSize >= 8 && Data[0] == '{' && Data[1] == '\\' && Data[2] == 'r' && Data[3] == 't' && Data[4] == 'f'
			&& ext && strcasecmp(ext, ".rtf") == 0) { // ensure
			return "RTF";

	} else if (DataSize >= 8 && Data[0] == 0xff && Data[1] == 0xd8 && Data[2] == 0xff
		&& ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)) {
			return "JPG";

	} else if (DataSize >= 8 && Data[0] == 0x89 && Data[1] == 0x50 && Data[2] == 0x4e && Data[3] == 0x47
		&& ext && strcasecmp(ext, ".png") == 0) {
		return "PNG";

	} else if (DataSize >= 8 && Data[0] == 'A' && Data[1] == 'T' && Data[2] == '&' && Data[3] == 'T'
		&& Data[4] == 'F' && Data[5] == 'O' && Data[6] == 'R' && Data[7] == 'M'
		&& ext && strcasecmp(ext, ".djvu") == 0) { // Ensure
		return "DJVU";

	} else if (DataSize >= 8 && Data[0] == 0x49 && Data[1] == 0x44 && Data[2] == 0x33
		&& ext && strcasecmp(ext, ".mp3") == 0) {
		return "MP3";

	} else if (DataSize >= 8 && Data[0] == 'M' && Data[1] == 'A' && Data[2] == 'C'
		&& ext && strcasecmp(ext, ".ape") == 0) {
		return "APE";

	} else if (DataSize >= 8 && Data[0] == 'w' && Data[1] == 'v' && Data[2] == 'p' && Data[3] == 'k'
		&& ext && strcasecmp(ext, ".vw") == 0) {
		return "WV";

	} else if (DataSize >= 8 && Data[0] == 'f' && Data[1] == 'L' && Data[2] == 'a' && Data[3] == 'C'
		&& ext && strcasecmp(ext, ".flac") == 0) {
		return "FLAC";

	} else if (DataSize >= 8 && Data[0] == 'O' && Data[1] == 'g' && Data[2] == 'g' && Data[3] == 'S'
		&& ext && (strcasecmp(ext, ".ogg") == 0 || strcasecmp(ext, ".oga") == 0 || strcasecmp(ext, ".opus") == 0)) {
		return "OGG";

	} else if (DataSize >= 12 && memcmp(Data + 4, "ftypM4A ", 8) == 0
		&& ext && strcasecmp(ext, ".m4a") == 0) {
		return "M4A";

	} else if (DataSize >= 4 && Data[0] == 0x4d && Data[1] == 0x5a
		&& ext && (strcasecmp(ext, ".exe") == 0 || strcasecmp(ext, ".dll") == 0 || strcasecmp(ext, ".sys") == 0
			|| strcasecmp(ext, ".drv") == 0 || strcasecmp(ext, ".ocx") == 0 || strcasecmp(ext, ".efi") == 0)) {
		return "PE";

	} else if ((DataSize >= 8) &&
		((Data[0] == 0xfe && Data[1] == 0xed && Data[2] == 0xfa && Data[3] == 0xce) ||
		 (Data[0] == 0xce && Data[1] == 0xfa && Data[2] == 0xed && Data[3] == 0xfe) ||
		 (Data[0] == 0xca && Data[1] == 0xfe && Data[2] == 0xba && Data[3] == 0xbe))) {
		return "Mach-O";
	}

	return nullptr;
}


SHAREDSYMBOL HANDLE WINAPI _export OpenFilePlugin(const char *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (!G.IsStarted())
		return INVALID_HANDLE_VALUE;

	const bool elf = HasElvenEars(Data, DataSize);
	const char *plain = elf ? nullptr : DetectPlainKind(Name, Data, DataSize);

//	fprintf(stderr, "Inside: OpenFilePlugin('%s' .. 0x%x): %s %s\n", Name, OpMode, elf ? "ELF" : "", plain ? plain : "");

	if (!elf && !plain)
		return INVALID_HANDLE_VALUE;

	// Well, it really looks like valid ELF or some plain document file

	if ( (OpMode & OPM_FIND) != 0) {
		// In case of open from find file dialog - allow digging into plain document files,
		// but not ELFs - there is nothing interesting to search inside of disasm etc.
		if (elf)
			return INVALID_HANDLE_VALUE;

	} else if ((OpMode & (OPM_PGDN | OPM_COMMANDS)) == 0) {
		// If user called us with Ctrl+PgDn - then proceed for any file
		// Otherwise proceed only for ELF file and only if its not eXecutable, to allow user execute it by Enter
		if (!elf)
			return INVALID_HANDLE_VALUE;

		struct stat s = {};
		if (sdc_stat(Name, &s) == -1) // in case of any uncertainlity...
			return INVALID_HANDLE_VALUE;

		if ((s.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
			return INVALID_HANDLE_VALUE;
	}

	PluginImpl *out = nullptr;
#ifndef __APPLE__
	if (elf) {
		out = new PluginImplELF(Name, Data[4], Data[5]);

	} else
#endif
	if (plain) {
		out = new PluginImplPlain(Name, plain);
	} else {
		ABORT();
	}

	return out ? (HANDLE)out : INVALID_HANDLE_VALUE;
}


SHAREDSYMBOL HANDLE WINAPI _export OpenPlugin(int OpenFrom, INT_PTR Item)
{
	if (!G.IsStarted() || OpenFrom != OPEN_COMMANDLINE)
		return INVALID_HANDLE_VALUE;

	if (strncmp((const char *)Item, G.command_prefix.c_str(), G.command_prefix.size()) != 0
	|| ((const char *)Item)[G.command_prefix.size()] != ':') {
		return INVALID_HANDLE_VALUE;
	}

	std::string path( &((const char *)Item)[G.command_prefix.size() + 1] );
	if (path.size() >1 && path[0] == '\"' && path[path.size() - 1] == '\"')
		path = path.substr(1, path.size() - 2);
	if (path.empty())
		return INVALID_HANDLE_VALUE;

	int fd = sdc_open(path.c_str(), O_RDONLY);
	if (fd == -1)
		return INVALID_HANDLE_VALUE;

	unsigned char data[MINIMAL_LEN];
	int data_len = sdc_read(fd, data, sizeof(data));
	sdc_close(fd);
	return OpenFilePlugin(path.c_str(), data, data_len, OPM_COMMANDS);
}

SHAREDSYMBOL void WINAPI _export ClosePlugin(HANDLE hPlugin)
{
	delete (PluginImpl *)hPlugin;
}


SHAREDSYMBOL int WINAPI _export GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFindData(pPanelItem, pItemsNumber, OpMode);
}


SHAREDSYMBOL void WINAPI _export FreeFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	((PluginImpl *)hPlugin)->FreeFindData(PanelItem, ItemsNumber);
}


SHAREDSYMBOL int WINAPI _export SetDirectory(HANDLE hPlugin,const char *Dir,int OpMode)
{
	return ((PluginImpl *)hPlugin)->SetDirectory(Dir, OpMode);
}


SHAREDSYMBOL int WINAPI _export DeleteFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}


SHAREDSYMBOL int WINAPI _export GetFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
	int ItemsNumber,int Move,char *DestPath,int OpMode)
{
	return ((PluginImpl *)hPlugin)->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
}


SHAREDSYMBOL int WINAPI _export PutFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,
	int ItemsNumber,int Move,int OpMode)
{
	return ((PluginImpl *)hPlugin)->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
}


SHAREDSYMBOL void WINAPI _export ExitFAR()
{
}


SHAREDSYMBOL void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);

	Info->Flags = PF_FULLCMDLINE;
	static const char *PluginCfgStrings[1];
	PluginCfgStrings[0] = (char*)G.GetMsg(MTitle);
	Info->PluginConfigStrings = PluginCfgStrings;
	Info->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);

	static char s_command_prefix[G.MAX_COMMAND_PREFIX + 1] = {}; // WHY?
	strncpy(s_command_prefix, G.command_prefix.c_str(), sizeof(s_command_prefix) - 1);
	Info->CommandPrefix = s_command_prefix;
}

SHAREDSYMBOL void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	((PluginImpl *)hPlugin)->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI _export ProcessHostFile(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
	return ((PluginImpl *)hPlugin)->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI _export ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState)
{
	return ((PluginImpl *)hPlugin)->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI _export Configure(int ItemNumber)
{
	if (!G.IsStarted())
		return 0;

	struct FarDialogItem fdi[] = {
		{DI_DOUBLEBOX,  3,  1,  70, 5,  0, {}, 0, 0, {}},
		{DI_TEXT,      -1,  2,  0,  2,  0, {}, 0, 0, {}},
		{DI_BUTTON,     0,  4,  0,  4,  0, {}, DIF_CENTERGROUP, 0, {}},
		{DI_BUTTON,     0,  4,  0,  4,  0, {}, DIF_CENTERGROUP, 0, {}}
	};

	CharArrayCpyZ(fdi[0].Data, G.GetMsg(MTitle));
	CharArrayCpyZ(fdi[1].Data, G.GetMsg(MDescription));
	CharArrayCpyZ(fdi[2].Data, G.GetMsg(MOK));
	CharArrayCpyZ(fdi[3].Data, G.GetMsg(MCleanup));

	if (G.info.Dialog(G.info.ModuleNumber, -1, -1, 74, 7, NULL, fdi, ARRAYSIZE(fdi)) == 3) {
		Storage::Clear();
	}
	return 1;
}
