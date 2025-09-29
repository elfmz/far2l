/*
  CUSTOM.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <sudo.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <farplug-mb.h>
#include <KeyFileHelper.h>
#include <utils.h>
#include <string>
#include <vector>
#include <memory>
using namespace oldfar;
#include "fmt.hpp"
#include <errno.h>

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

#undef isspace
#define isspace(c) ((c) == ' ' || (c) == '\t')

#ifdef _MSC_VER
// #pragma comment(linker, "-subsystem:console")
// #pragma comment(linker, "-merge:.rdata=.text")
#endif

typedef union
{
	int64_t i64;
	struct
	{
		DWORD LowPart;
		LONG HighPart;
	} Part;
} FAR_INT64;

///////////////////////////////////////////////////////////////////////////////
// Forward declarations

static int GetString(char *Str, int MaxSize);

static bool CheckIniFiles();
static const KeyFileValues *GetSection(int Num, std::string &Name);

static void FillFormat(const KeyFileValues *Values);
static void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft);
static int StringToInt(const char *str);
static int64_t StringToInt64(const char *str);
static void ParseListingItemPlain(const char *CurFormat, const char *CurStr, struct ArcItemInfo *Info,
		SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess);

///////////////////////////////////////////////////////////////////////////////
// Constants

enum
{
	PROF_STR_LEN = 256
};

///////////////////////////////////////////////////////////////////////////////
// Class CustomStringList

class CustomStringList
{
public:
	CustomStringList()
		:
		pNext(0)
	{
		str[0] = '\0';
	}

	~CustomStringList()
	{
		if (pNext)
			delete pNext;
	}

	CustomStringList *Add()
	{
		if (pNext)
			delete pNext;
		pNext = new CustomStringList;
		return pNext;
	}

	CustomStringList *Next() { return pNext; }

	char *Str() { return str; }

	void Empty()
	{
		if (pNext)
			delete pNext;
		pNext = 0;
		str[0] = '\0';
	}

protected:
	char str[PROF_STR_LEN];
	CustomStringList *pNext;
};

///////////////////////////////////////////////////////////////////////////////
// Class MetaReplacer

class MetaReplacer
{
	std::string m_command;
	std::string m_fileName;

	class Meta
	{
	public:
		enum Type
		{
			invalidType = -1,
			archiveName,
			shortArchiveName
		};

		enum Flags
		{
			quoteWithSpaces   = 1,
			quoteAll          = 2,
			useForwardSlashes = 4,
			useNameOnly       = 8,
			usePathOnly       = 16,
			useANSI           = 32
		};

	private:
		bool m_isValid;
		Type m_type;
		unsigned int m_flags;
		int m_length;

	public:
		Meta(const char *start)
			:
			m_isValid(false), m_type(invalidType), m_flags(0), m_length(0)
		{
			if ((start[0] != '%') || (start[1] != '%'))
				return;

			char typeChars[] = {'A', 'a'};
			char flagChars[] = {'Q', 'q', 'S', 'W', 'P', 'A'};

			for (size_t i = 0; i < sizeof(typeChars); ++i)
				if (start[2] == typeChars[i])
					m_type = (Type)i;

			if (m_type == invalidType)
				return;

			const char *p = start + 3;

			for (; *p; ++p) {
				bool isFlagChar = false;
				for (size_t i = 0; i < sizeof(flagChars); ++i)
					if (*p == flagChars[i]) {
						m_flags|= 1 << i;
						isFlagChar = true;
					}

				if (!isFlagChar)
					break;
			}

			m_length = (int)(p - start);

			m_isValid = true;
		}

		bool isValidMeta() const { return m_isValid; }

		Type getType() const { return m_type; }

		unsigned int getFlags() const { return m_flags; }

		int getLength() const { return m_length; }
	};

public:
	MetaReplacer(const std::string &command, const char *arcName)
		:
		m_command(command), m_fileName(arcName)
	{}

	virtual ~MetaReplacer() {}

	void replaceTo(std::string &out)
	{
		out.clear();
		bool bReplacedSomething = false;
		std::string var;

		for (const char *command = m_command.c_str(); *command;) {
			Meta m(command);

			if (!m.isValidMeta()) {
				out+= *command++;
				continue;
			}

			command+= m.getLength();
			bReplacedSomething = true;

			size_t lastSlash = m_fileName.rfind('/');
			if (lastSlash != std::string::npos && (m.getFlags() & Meta::useNameOnly) != 0) {
				var.assign(m_fileName.c_str() + lastSlash + 1);
			} else if (lastSlash != std::string::npos && (m.getFlags() & Meta::usePathOnly) != 0) {
				var.assign(m_fileName.c_str(), lastSlash);
			} else {
				var.assign(m_fileName);
			}

			bool bQuote = (m.getFlags() & Meta::quoteAll)
					|| ((m.getFlags() & Meta::quoteWithSpaces) && var.find(' ') != std::string::npos);

			if (bQuote)
				out+= '\"';

			out+= var;

			if (bQuote)
				out+= '\"';

			if (m.getFlags() & Meta::useForwardSlashes) {
			}
		}

		if (!bReplacedSomething)	// there were no meta-symbols, should use old-style method
		{
			out+= ' ';
			out+= m_fileName;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// Variables

static int CurTypeIndex = -1;
static char *OutData = nullptr;
static DWORD OutDataPos = 0, OutDataSize = 0;

static std::vector<std::pair<std::string, std::unique_ptr<KeyFileReadHelper>>> FormatFileNameKFH;

static std::string StartText, EndText;

static CustomStringList *Format = 0;
static CustomStringList *IgnoreStrings = 0;

static int IgnoreErrors;
static int ArcChapters;

static const char Str_TypeName[] = "TypeName";

static bool CurSilent;

///////////////////////////////////////////////////////////////////////////////
// Library function pointers

static FARSTDMKTEMP MkTemp = NULL;
static FARAPIMESSAGE FarMessage = NULL;
static INT_PTR FarModuleNumber = 0;

///////////////////////////////////////////////////////////////////////////////
// Exported functions

void WINAPI _export CUSTOM_SetFarInfo(const struct PluginStartupInfo *Info)
{
	MkTemp = Info->FSF->MkTemp;
	FarMessage = Info->Message;
	FarModuleNumber = Info->ModuleNumber;
}

DWORD WINAPI _export CUSTOM_LoadFormatModule(const char *ModuleName)
{
	std::string s = ModuleName;
	size_t p = s.rfind(GOOD_SLASH);
	if (p != std::string::npos) {
		s.resize(p + 1);
		s+= "custom.ini";
		FormatFileNameKFH.emplace_back(std::make_pair(s, nullptr));

		if (TranslateInstallPath_Lib2Share(s)) {
			FormatFileNameKFH.emplace_back(std::make_pair(s, nullptr));
		}
	}

	FormatFileNameKFH.emplace_back(std::make_pair(InMyConfig("plugins/multiarc/custom.ini", false), nullptr));

	CheckIniFiles();
	return (0);
}

static bool CheckID(const std::vector<unsigned char> &ID, int IDPos, const unsigned char *Data, int DataSize)
{
	if (IDPos >= 0)
		return (IDPos <= DataSize - (int)ID.size()) && (memcmp(Data + IDPos, &ID[0], (int)ID.size()) == 0);

	for (int I = 0; I <= DataSize - (int)ID.size(); I++) {
		if (memcmp(Data + I, &ID[0], ID.size()) == 0)
			return true;
	}

	return false;
}

BOOL WINAPI _export CUSTOM_IsArchive(const char *FName, const unsigned char *Data, int DataSize)
{
	if (!CheckIniFiles())
		return FALSE;

	char *Dot = strrchr((char *)FName, '.');

	std::string TypeName, Name, Ext;
	char IDName[32];
	std::vector<unsigned char> ID;

	for (int I = 0;; I++) {
		const KeyFileValues *Values = GetSection(I, TypeName);
		if (!Values)
			break;

		Name = Values->GetString(Str_TypeName, TypeName.c_str());

		if (Name.empty())
			continue;

		bool SpecifiedID = false, FoundID = false;
		for (unsigned int J = 0; J != (unsigned int)-1; ++J) {
			if (J != 0)
				snprintf(IDName, sizeof(IDName), "ID%u", J);
			else
				strcpy(IDName, "ID");

			if (!Values->GetBytes(ID, IDName) || ID.empty()) {
				break;
			}
			SpecifiedID = true;

			strcat(IDName, "Pos");
			int IDPos = Values->GetInt(IDName, -1);

			FoundID = CheckID(ID, IDPos, Data, DataSize);
			if (FoundID)
				break;
		}

		if (SpecifiedID) {
			if (!FoundID)
				continue;

			if (Values->GetInt("IDOnly", 0)) {
				CurTypeIndex = I;
				return (TRUE);
			}
		}

		Ext = Values->GetString("Extension", "");

		if (Dot != NULL && !Ext.empty() && strcasecmp(Dot + 1, Ext.c_str()) == 0) {
			CurTypeIndex = I;
			return (TRUE);
		}
	}
	return (FALSE);
}

DWORD WINAPI _export CUSTOM_GetSFXPos(void)
{
	return 0;
}

BOOL WINAPI _export CUSTOM_OpenArchive(const char *Name, int *Type, bool Silent)
{
	std::string TypeName;
	const KeyFileValues *Values = GetSection(CurTypeIndex, TypeName);
	if (!Values)
		return (FALSE);

	std::string Command = Values->GetString("List", "");

	if (Command.empty())
		return (FALSE);

	IgnoreErrors = Values->GetInt("IgnoreErrors", 0);
	*Type = CurTypeIndex;

	ArcChapters = -1;

	MetaReplacer meta(Command, Name);

	meta.replaceTo(Command);

	char TempName[NM];
	if (MkTemp(TempName, "FAR") == NULL)
		return (FALSE);
	sdc_remove(TempName);

	//    HANDLE OutHandle = WINPORT(CreateFile)(MB2Wide(TempName).c_str(), GENERIC_READ | GENERIC_WRITE,
	//                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
	//                              FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);

	WCHAR SaveTitle[512]{};

	WINPORT(GetConsoleTitle)(NULL, SaveTitle, ARRAYSIZE(SaveTitle) - 1);
	WINPORT(SetConsoleTitle)(NULL, StrMB2Wide(Command).c_str());

	// char ExpandedCmd[512];

	// WINPORT(Environment::ExpandString)(Command, ExpandedCmd, sizeof(ExpandedCmd));
	std::string cmd = Command;
	cmd+= "  >";	// 2>/dev/null
	cmd+= TempName;
	DWORD ExitCode = system(cmd.c_str());
	if (ExitCode && !CurSilent) {
		const auto &ToolNotFoundMsg = Values->GetString("ToolNotFound", "");
		if (!ToolNotFoundMsg.empty()) {
			size_t trim_pos = cmd.find_first_of("|>");
			if (trim_pos != std::string::npos) {
				cmd.resize(trim_pos);
			}
			cmd.insert(0, "command -v ");
			if (system(cmd.c_str()) != 0) {
				std::string title = "MultiArc: ";
				title+= TypeName;
				const char *MsgItems[] = {title.c_str(), ToolNotFoundMsg.c_str()};
				FarMessage(FarModuleNumber, FMSG_WARNING | FMSG_MB_OK, NULL, MsgItems, ARRAYSIZE(MsgItems),
						0);
			}
		}
	}

	if (ExitCode) {
		ExitCode = (ExitCode < Values->GetUInt("Errorlevel", 1000));
	} else {
		ExitCode = 1;
	}

	if (ExitCode) {
		OutData = NULL;
		int fd = sdc_open(TempName, O_RDONLY);
		if (fd != -1) {
			OutDataSize = OutDataPos = 0;
			struct stat s = {0};
			sdc_fstat(fd, &s);
			if (s.st_size > 0) {
				OutData = (char *)calloc(s.st_size + 1, 1);
				if (OutData) {
					for (OutDataSize = 0; OutDataSize < s.st_size;) {
						int piece = (s.st_size - OutDataSize < 0x10000) ? s.st_size - OutDataSize : 0x10000;
						int r = sdc_read(fd, OutData + OutDataSize, piece);
						if (r <= 0)
							break;
						OutDataSize+= r;
					}
				}
			}
			sdc_close(fd);
		}

		if (OutData == NULL)
			ExitCode = 0;
	}
	// fprintf(stderr, "OutData: '%s'\n", OutData);

	WINPORT(SetConsoleTitle)(NULL, SaveTitle);

	sdc_remove(TempName);
	FillFormat(Values);

	if (ExitCode && OutDataSize == 0) {
		free(OutData);
		OutData = nullptr;
	}

	CurSilent = Silent;

	return (ExitCode);
}

int WINAPI _export CUSTOM_GetArcItem(struct ArcItemInfo *Info)
{
	char Str[512];
	CustomStringList *CurFormatNode = Format;
	SYSTEMTIME stModification, stCreation, stAccess, syst;

	ZeroFill(stModification);
	ZeroFill(stCreation);
	ZeroFill(stAccess);
	WINPORT(GetSystemTime)(&syst);

	while (GetString(Str, sizeof(Str))) {

			if (!StartText.empty()) {
				if (StartText == Str) {
					StartText.clear();
				}
				continue;
			}

			if (!EndText.empty()) {
				if (EndText == Str) {
					break;
				}
			}

			ParseListingItemPlain(CurFormatNode->Str(), Str, Info, stModification, stCreation, stAccess);

		CurFormatNode = CurFormatNode->Next();
		if (!CurFormatNode || !CurFormatNode->Next()) {
			MakeFiletime(stModification, syst, &Info->ftLastWriteTime);
			MakeFiletime(stCreation, syst, &Info->ftCreationTime);
			MakeFiletime(stAccess, syst, &Info->ftLastAccessTime);
			while (!Info->PathName.empty() && isspace(Info->PathName.back()))
				Info->PathName.pop_back();

			return (GETARC_SUCCESS);
		}
	}

	return (GETARC_EOF);
}

BOOL WINAPI _export CUSTOM_CloseArchive(struct ArcInfo *Info)
{
	if (IgnoreErrors)
		Info->Flags|= AF_IGNOREERRORS;

	if (ArcChapters < 0)
		ArcChapters = 0;
	Info->Chapters = ArcChapters;

	free(OutData);
	OutData = nullptr;

	delete Format;
	delete IgnoreStrings;
	Format = 0;
	IgnoreStrings = 0;
	CurTypeIndex = -1;

	return (TRUE);
}

BOOL WINAPI _export CUSTOM_GetFormatName(int Type, std::string &FormatName, std::string &DefaultExt)
{
	std::string TypeName;
	const KeyFileValues *Values = GetSection(Type, TypeName);

	if (!Values)
		return (FALSE);

	FormatName = Values->GetString(Str_TypeName, TypeName.c_str());
	DefaultExt = Values->GetString("Extension");
	return !FormatName.empty();
}

BOOL WINAPI _export CUSTOM_GetDefaultCommands(int Type, int Command, std::string &Dest)
{
	std::string TypeName, FormatName;

	const KeyFileValues *Values = GetSection(Type, TypeName);

	if (!Values)
		return (FALSE);

	FormatName = Values->GetString(Str_TypeName, TypeName.c_str());

	if (FormatName.empty())
		return (FALSE);

	static const char *CmdNames[] = {"Extract", "ExtractWithoutPath", "Test", "Delete", "Comment",
			"CommentFiles", "SFX", "Lock", "Protect", "Recover", "Add", "Move", "AddRecurse", "MoveRecurse",
			"AllFilesMask"};

	if (Command < (int)(ARRAYSIZE(CmdNames))) {
		Dest = Values->GetString(CmdNames[Command], "");
		return (TRUE);
	}

	return (FALSE);
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

static const KeyFileValues *GetSection(int Num, std::string &Name)
{
	if (Num < 0) {
		return nullptr;
	}
	for (const auto &i : FormatFileNameKFH)
		if (i.second) {
			const auto &sections = i.second->EnumSections();
			if (Num < (int)sections.size()) {
				return i.second->GetSectionValues(sections[Num]);
			}
			Num-= (int)sections.size();
		}
	return nullptr;
}

static bool CheckIniFiles()
{
	bool out = false;

	for (auto &i : FormatFileNameKFH) {
		struct stat s
		{};
		if (stat(i.first.c_str(), &s) == -1) {
			i.second.reset();

		} else {
			out = true;
			if (i.second) {
				const auto &ls = i.second->LoadedFileStat();
				if (s.st_ino == ls.st_ino && s.st_mtime == ls.st_mtime && s.st_size == ls.st_size) {
					continue;
				}
				i.second.reset();
			}
			i.second.reset(new KeyFileReadHelper(i.first));
		}
	}

	return out;
}

static void FillFormat(const KeyFileValues *Values)
{
	StartText = Values->GetString("Start", "");
	EndText = Values->GetString("End", "");

	int FormatNumber = 0;

	delete Format;
	Format = new CustomStringList;
	for (CustomStringList *CurFormat = Format;; CurFormat = CurFormat->Add()) {
		const auto &FormatName = StrPrintf("Format%d", FormatNumber++);
		Values->GetChars(CurFormat->Str(), PROF_STR_LEN, FormatName, "");
		if (*CurFormat->Str() == 0)
			break;
	}

	int Number = 0;

	delete IgnoreStrings;
	IgnoreStrings = new CustomStringList;
	for (CustomStringList *CurIgnoreString = IgnoreStrings;; CurIgnoreString = CurIgnoreString->Add()) {
		const auto &Name = StrPrintf("IgnoreString%d", Number++);
		Values->GetChars(CurIgnoreString->Str(), PROF_STR_LEN, Name, "");
		if (*CurIgnoreString->Str() == 0)
			break;
	}
}

static int GetString(char *Str, int MaxSize)
{
	if (OutDataPos >= OutDataSize)
		return (FALSE);

	int StartPos = OutDataPos;

	while (OutDataPos < OutDataSize) {
		int Ch = OutData[OutDataPos];

		if (Ch == '\r' || Ch == '\n')
			break;
		OutDataPos++;
	}

	int Length = OutDataPos - StartPos;
	int DestLength = Length >= MaxSize ? MaxSize - 1 : Length;

	memcpy(Str, OutData + StartPos, DestLength);
	Str[DestLength] = 0;

	while (OutDataPos < OutDataSize) {
		int Ch = OutData[OutDataPos];

		if (Ch != '\r' && Ch != '\n')
			break;
		OutDataPos++;
	}

	return (TRUE);
}

static void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft)
{
	if (st.wDay == 0)
		st.wDay = syst.wDay;
	if (st.wMonth == 0)
		st.wMonth = syst.wMonth;
	if (st.wYear == 0)
		st.wYear = syst.wYear;
	else {
		if (st.wYear < 50)
			st.wYear+= 2000;
		else if (st.wYear < 100)
			st.wYear+= 1900;
	}

	FILETIME ft;

	if (WINPORT(SystemTimeToFileTime)(&st, &ft)) {
		WINPORT(LocalFileTimeToFileTime)(&ft, pft);
	}
}

static int StringToInt(const char *str)
{
	int i = 0;
	for (const char *p = str; p && *p; ++p)
		if (isdigit(*p))
			i = i * 10 + (*p - '0');
	return i;
}

static int64_t StringToInt64(const char *str)
{
	int64_t i = 0;
	for (const char *p = str; p && *p; ++p)
		if (isdigit(*p))
			i = i * 10 + (*p - '0');
	return i;
}

static int StringToIntHex(const char *str)
{
	int i = 0;
	for (const char *p = str; p && *p; ++p)
		if (isxdigit(*p)) {
			char dig_sub = (*p >= 'a' ? 'a' : (*p >= 'A' ? 'A' : '0'));
			i = i * 16 + (*p - dig_sub);
		}
	return i;
}

static void ParseListingItemPlain(const char *CurFormat, const char *CurStr, struct ArcItemInfo *Info,
		SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess)
{
	enum
	{
		OP_OUTSIDE,
		OP_INSIDE,
		OP_SKIP
	} OptionalPart = OP_OUTSIDE;
	int IsChapter = 0;

	for (; *CurStr && *CurFormat; CurFormat++, CurStr++) {
		if (OptionalPart == OP_SKIP) {
			if (*CurFormat == ')')
				OptionalPart = OP_OUTSIDE;
			CurStr--;
			continue;
		}
		switch (*CurFormat) {
			case '*':
				if (isspace(*CurStr) || !*CurStr)
					CurStr--;
				else
					while (/*CurStr[0] && */ CurStr[1]	/*&& !isspace(CurStr[0]) */
							&& !isspace(CurStr[1]))
						CurStr++;
				break;
			case 'n':
				if (*CurStr)
					Info->PathName+= *CurStr;
				break;
			case 'c':
				if (*CurStr) {
					if (!Info->Description)
						Info->Description.reset(new std::string);
					Info->Description->append(1, *CurStr);
				}
				break;
			case '.': {
				while (!Info->PathName.empty() && isspace(Info->PathName.back()))
					Info->PathName.pop_back();

				if (!Info->PathName.empty())
					Info->PathName+= '.';
			} break;
			case 'z':
				if (isdigit(*CurStr)) {
					Info->nFileSize*= 10;
					Info->nFileSize+= (*CurStr - '0');
				} else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'p':
				if (isdigit(*CurStr)) {
					Info->nPhysicalSize*= 10;
					Info->nPhysicalSize+= (*CurStr - '0');
				} else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'a':
				switch (*CurStr) {
					case 'd':
					case 'D':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_DIRECTORY;
						break;
					case 'h':
					case 'H':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_HIDDEN;
						break;
					case 'a':
					case 'A':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_ARCHIVE;
						break;
					case 'r':
					case 'R':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_READONLY;
						break;
					case 's':
					case 'S':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_SYSTEM;
						break;
					case 'c':
					case 'C':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_COMPRESSED;
						break;
					case 'x':
					case 'X':
						Info->dwFileAttributes|= FILE_ATTRIBUTE_EXECUTABLE;
						break;
				}
				break;
				// MODIFICATION DATETIME
			case 'y':
				if (isdigit(*CurStr))
					stModification.wYear = stModification.wYear * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'd':
				if (isdigit(*CurStr))
					stModification.wDay = stModification.wDay * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 't':
				if (isdigit(*CurStr))
					stModification.wMonth = stModification.wMonth * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'T': {
				static const char *Months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
						"Sep", "Oct", "Nov", "Dec"};

				for (size_t I = 0; I < sizeof(Months) / sizeof(Months[0]); I++)
					if (strncasecmp(CurStr, Months[I], 3) == 0) {
						stModification.wMonth = (WORD)(I + 1);
						while (CurFormat[1] == 'T' && CurStr[1]) {
							CurStr++;
							CurFormat++;
						}
						break;
					}
			} break;
			case 'h':
				if (isdigit(*CurStr))
					stModification.wHour = stModification.wHour * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'H':
				switch (*CurStr) {
					case 'A':
					case 'a':
						if (stModification.wHour == 12)
							stModification.wHour-= 12;
						break;
					case 'P':
					case 'p':
						if (stModification.wHour < 12)
							stModification.wHour+= 12;
						break;
				}
				break;
			case 'm':
				if (isdigit(*CurStr))
					stModification.wMinute = stModification.wMinute * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 's':
				if (isdigit(*CurStr))
					stModification.wSecond = stModification.wSecond * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
				// ACCESS DATETIME
			case 'b':
				if (isdigit(*CurStr))
					stAccess.wDay = stAccess.wDay * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'v':
				if (isdigit(*CurStr))
					stAccess.wMonth = stAccess.wMonth * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'e':
				if (isdigit(*CurStr))
					stAccess.wYear = stAccess.wYear * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'x':
				if (isdigit(*CurStr))
					stAccess.wHour = stAccess.wHour * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'l':
				if (isdigit(*CurStr))
					stAccess.wMinute = stAccess.wMinute * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'k':
				if (isdigit(*CurStr))
					stAccess.wSecond = stAccess.wSecond * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
				// CREATION DATETIME
			case 'j':
				if (isdigit(*CurStr))
					stCreation.wDay = stCreation.wDay * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'g':
				if (isdigit(*CurStr))
					stCreation.wMonth = stCreation.wMonth * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'f':
				if (isdigit(*CurStr))
					stCreation.wYear = stCreation.wYear * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'o':
				if (isdigit(*CurStr))
					stCreation.wHour = stCreation.wHour * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'i':
				if (isdigit(*CurStr))
					stCreation.wMinute = stCreation.wMinute * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'u':
				if (isdigit(*CurStr))
					stCreation.wSecond = stCreation.wSecond * 10 + (*CurStr - '0');
				else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'r':
				if (isxdigit(toupper(*CurStr))) {
					char dig_sub = (*CurStr >= 'a' ? 'a' : (*CurStr >= 'A' ? 'A' : '0'));
					Info->CRC32 = Info->CRC32 * 16 + (*CurStr - dig_sub);
				} else if (OP_INSIDE == OptionalPart) {
					CurStr--;
					OptionalPart = OP_SKIP;
				}
				break;
			case 'C':
				if (*CurStr == '-') {
					IsChapter = 1;
					ArcChapters = 0;
				} else if (isdigit(*CurStr)) {
					if (IsChapter)
						ArcChapters = ArcChapters * 10 + (*CurStr - '0');
					else
						Info->Chapter = Info->Chapter * 10 + (*CurStr - '0');
				}
				break;
			case '(':
				OptionalPart = OP_INSIDE;
				CurStr--;
				break;
			case ')':
				OptionalPart = OP_OUTSIDE;
				CurStr--;
				break;
		}
	}
}
