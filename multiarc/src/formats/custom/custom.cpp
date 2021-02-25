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
#include <pluginold.hpp>
#include <KeyFileHelper.h>
#include <utils.h>
#include <string>
#include <vector>
#include <memory>
using namespace oldfar;
#include "fmt.hpp"
#include <errno.h>

#include "pcre++.h"
using namespace PCRE;

#if defined(__BORLANDC__)
    #pragma option -a1
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
    #pragma pack(1)
    #if defined(__LCC__)
        #define _export __declspec(dllexport)
    #endif
#else
    #pragma pack(push,1)
    #if _MSC_VER
        #define _export
    #endif
#endif


#undef isspace
#define isspace(c) ((c)==' ' || (c)=='\t')

#ifdef _MSC_VER
//#pragma comment(linker, "-subsystem:console")
//#pragma comment(linker, "-merge:.rdata=.text")
#endif

typedef union {
  int64_t i64;
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } Part;
} FAR_INT64;


///////////////////////////////////////////////////////////////////////////////
// Forward declarations

static int GetString(char *Str, int MaxSize);

static bool CheckIniFiles();
static const KeyFileValues *GetSection(int Num, std::string &Name);
static DWORD KeyFileValuePSZ(const KeyFileValues *Values,LPCSTR lpKeyName,LPCSTR lpDefault,LPSTR lpReturnedString,DWORD nSize);

static void FillFormat(const KeyFileValues *Values);
static void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft);
static int StringToInt(const char *str);
static int64_t StringToInt64(const char *str);
static void ParseListingItemRegExp(Match match,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess);
static void ParseListingItemPlain(const char *CurFormat, const char *CurStr,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess);

///////////////////////////////////////////////////////////////////////////////
// Constants

enum { PROF_STR_LEN = 256 };


///////////////////////////////////////////////////////////////////////////////
// Class CustomStringList

class CustomStringList
{
public:
    CustomStringList()
        :   pNext(0)
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

    CustomStringList *Next()
    {
        return pNext;
    }

    char *Str()
    {
        return str;
    }

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
            quoteWithSpaces = 1,
            quoteAll = 2,
            useForwardSlashes = 4,
            useNameOnly = 8,
            usePathOnly = 16,
            useANSI = 32
        };

    private:
        bool m_isValid;
        Type m_type;
        unsigned int m_flags;
        int m_length;

    public:
        Meta(const char *start)
            :   m_isValid(false), m_type(invalidType), m_flags(0), m_length(0)
        {
            if((start[0] != '%') || (start[1] != '%'))
                return;

            char typeChars[] = { 'A', 'a' };
            char flagChars[] = { 'Q', 'q', 'S', 'W', 'P', 'A' };

            for(size_t i = 0; i < sizeof(typeChars); ++i)
                if(start[2] == typeChars[i])
                    m_type = (Type) i;

            if(m_type == invalidType)
                return;

            const char *p = start + 3;

            for(; *p; ++p)
            {
                bool isFlagChar = false;
                for(size_t i = 0; i < sizeof(flagChars); ++i)
                    if(*p == flagChars[i])
                    {
                        m_flags |= 1 << i;
                        isFlagChar = true;
                    }

                if(!isFlagChar)
                    break;
            }

            m_length = (int)(p - start);

            m_isValid = true;

        }

        bool isValidMeta() const
        {
            return m_isValid;
        }

        Type getType() const
        {
            return m_type;
        }

        unsigned int getFlags() const
        {
            return m_flags;
        }

        int getLength() const
        {
            return m_length;
        }
    };


  public:
    MetaReplacer(const std::string &command, const char *arcName)
        :   m_command(command), m_fileName(arcName)
    {
    }

    virtual ~MetaReplacer()
    {
    }

    void replaceTo(std::string &out)
    {
        out.clear();
        bool bReplacedSomething = false;
        std::string var;

        for(const char *command = m_command.c_str(); *command;)
        {
            Meta m(command);

            if(!m.isValidMeta())
            {
                out+= *command++;
                continue;
            }

            command+= m.getLength();
            bReplacedSomething = true;

            size_t lastSlash = m_fileName.rfind('/');
            if(lastSlash != std::string::npos && (m.getFlags() & Meta::useNameOnly) != 0)
            {
                var.assign(m_fileName.c_str() + lastSlash + 1);
            }
            else if(lastSlash != std::string::npos && (m.getFlags() & Meta::usePathOnly) != 0)
            {
                var.assign(m_fileName.c_str(), lastSlash);
            }
            else
            {
                var.assign(m_fileName);
            }

            bool bQuote = (m.getFlags() & Meta::quoteAll)
                || ((m.getFlags() & Meta::quoteWithSpaces) && var.find(' ') != std::string::npos);

            if(bQuote)
                out+= '\"';

            out+= var;

            if(bQuote)
                out+= '\"';

            if(m.getFlags() & Meta::useForwardSlashes)
            {
            }
        }

        if(!bReplacedSomething) // there were no meta-symbols, should use old-style method
        {
            out+= ' ';
            out+= m_fileName;
        }

    }
};


///////////////////////////////////////////////////////////////////////////////
// Variables

static int     CurTypeIndex = -1;
static char    *OutData = nullptr;
static DWORD   OutDataPos = 0, OutDataSize = 0;

static std::vector<std::pair<std::string, std::unique_ptr<KeyFileReadHelper>>> FormatFileNameKFH;

static std::string StartText, EndText;

static CustomStringList *Format = 0;
static CustomStringList *IgnoreStrings = 0;

static int     IgnoreErrors;
static int     ArcChapters;

static const char Str_TypeName[] = "TypeName";

static bool CurSilent;

///////////////////////////////////////////////////////////////////////////////
// Library function pointers

static FARSTDMKTEMP        MkTemp = NULL;
static FARAPIMESSAGE       FarMessage = NULL;
static INT_PTR             FarModuleNumber = 0;

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

	FormatFileNameKFH.emplace_back(
		std::make_pair(InMyConfig("multiarc/custom.ini", false), nullptr));

	CheckIniFiles();
	return (0);
}


static bool CheckID(const std::vector<unsigned char> &ID, int IDPos, const unsigned char *Data, int DataSize)
{
	if (IDPos >= 0)
		return (IDPos <= DataSize - (int)ID.size()) && (memcmp(Data + IDPos, &ID[0], (int)ID.size()) == 0);

	for (int I = 0; I <= DataSize - (int)ID.size(); I++)
	{
		if (memcmp(Data + I, &ID[0], ID.size()) == 0)
			return true;
	}

	return false;
}

BOOL WINAPI _export CUSTOM_IsArchive(const char *FName, const unsigned char *Data, int DataSize)
{
	if (!CheckIniFiles())
		return FALSE;

    char *Dot = strrchr((char *) FName, '.');

	std::string TypeName, Name, Ext;
	char IDName[32];
	std::vector<unsigned char> ID;

    for (int I = 0;; I++)
    {
		const KeyFileValues *Values = GetSection(I, TypeName);
		if (!Values)
			break;

		Name = Values->GetString(Str_TypeName, TypeName.c_str());

        if (Name.empty())
            continue;

		bool SpecifiedID = false, FoundID = false;
		for (unsigned int J = 0; J != (unsigned int)-1; ++J)
		{
			if (J != 0)
				sprintf(IDName, "ID%u", J);
			else
				strcpy(IDName, "ID");

	        if (!Values->GetBytes(IDName, ID) || ID.empty()) {
				break;
			}
			SpecifiedID = true;

			strcat(IDName, "Pos");
			int IDPos = Values->GetInt(IDName, -1);

			FoundID = CheckID(ID, IDPos, Data, DataSize);
			if (FoundID)
				break;
		}

		if (SpecifiedID)
		{
			if (!FoundID)
				continue;

			if (Values->GetInt("IDOnly", 0))
			{
				CurTypeIndex = I;
				return (TRUE);
			}
		}

		Ext = Values->GetString("Extension", "");

        if(Dot != NULL && !Ext.empty() && strcasecmp(Dot + 1, Ext.c_str()) == 0)
        {
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
    if(MkTemp(TempName, "FAR") == NULL)
        return (FALSE);
    sdc_remove(TempName);
		


//    HANDLE OutHandle = WINPORT(CreateFile)(MB2Wide(TempName).c_str(), GENERIC_READ | GENERIC_WRITE,
  //                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
    //                              FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);

    DWORD ConsoleMode;

    WINPORT(GetConsoleMode)(NULL, &ConsoleMode);
    WINPORT(SetConsoleMode)(NULL, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                   ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT);
    WCHAR SaveTitle[512]{};

    WINPORT(GetConsoleTitle)(SaveTitle, ARRAYSIZE(SaveTitle) - 1);
    WINPORT(SetConsoleTitle)(StrMB2Wide(Command).c_str());

    //char ExpandedCmd[512];

    //WINPORT(ExpandEnvironmentStrings)(Command, ExpandedCmd, sizeof(ExpandedCmd));
    std::string cmd = Command;
    cmd+= "  >"; //2>/dev/null
    cmd+= TempName;
    DWORD ExitCode = system(cmd.c_str());
	if (ExitCode && !CurSilent)
	{
		const auto &ToolNotFoundMsg = Values->GetString("ToolNotFound", "");
		if (!ToolNotFoundMsg.empty())
		{
			size_t trim_pos = cmd.find_first_of("|>");
			if (trim_pos != std::string::npos) {
				cmd.resize(trim_pos);
			}
			cmd.insert(0, "command -v ");
			if (system(cmd.c_str()) != 0)
			{
				std::string title = "MultiArc: ";
				title+= TypeName;
				const char *MsgItems[] = { title.c_str(), ToolNotFoundMsg.c_str() };
				FarMessage(FarModuleNumber, FMSG_WARNING | FMSG_MB_OK, NULL, MsgItems, ARRAYSIZE(MsgItems), 0);
			}
		}
	}


    if(ExitCode)
    {
        ExitCode = (ExitCode < Values->GetUInt("Errorlevel", 1000));
    } else {
        ExitCode = 1;
    }

    if(ExitCode)
    {
        OutData = NULL;
		int fd = sdc_open(TempName, O_RDONLY);
		if (fd!=-1) {
			OutDataSize = OutDataPos = 0;
			struct stat s = {0};
			sdc_fstat(fd, &s);
			if (s.st_size > 0) {
				OutData = (char *) calloc(s.st_size + 1, 1);
				if (OutData) {
					for (OutDataSize = 0; OutDataSize < s.st_size;) {
						int piece = (s.st_size - OutDataSize < 0x10000) ? s.st_size - OutDataSize : 0x10000;
						int r = sdc_read(fd, OutData + OutDataSize, piece);
						if (r<=0) break;
						OutDataSize+= r;
					}
				}
			}
			sdc_close(fd);
		}
		
        if(OutData == NULL)
            ExitCode = 0;
    }
//	fprintf(stderr, "OutData: '%s'\n", OutData);

    WINPORT(SetConsoleTitle)(SaveTitle);
    WINPORT(SetConsoleMode)(NULL, ConsoleMode);

	sdc_remove(TempName);
    FillFormat(Values);

    if(ExitCode && OutDataSize == 0)
    {
        free(OutData);
		OutData = nullptr;
    }

	CurSilent = Silent;
	
    return (ExitCode);
}


int WINAPI _export CUSTOM_GetArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info)
{
    char Str[512];
    CustomStringList *CurFormatNode = Format;
    SYSTEMTIME stModification, stCreation, stAccess, syst;

    memset(&stModification, 0, sizeof(stModification));
    memset(&stCreation, 0, sizeof(stCreation));
    memset(&stAccess, 0, sizeof(stAccess));
    WINPORT(GetSystemTime)(&syst);

    while(GetString(Str, sizeof(Str)))
    {
        RegExp re;

        if(!StartText.empty())
        {
            if(re.compile(StartText.c_str()))
            {
                if(re.match(Str))
                    StartText.clear();
            }
            else
            {
                if((StartText[0] == '^' && strncmp(Str, StartText.c_str() + 1, StartText.size() - 1) == 0) ||
                   (StartText[0] != '^' && strstr(Str, StartText.c_str()) != NULL))
                {
                    StartText.clear();
                }
            }
            continue;
        }

        if(!EndText.empty())
        {
            if(re.compile(EndText.c_str()))
            {
                if(re.match(Str))
                    break;
            }
            else if(EndText[0] == '^')
            {
                if(strncmp(Str, EndText.c_str() + 1, EndText.size() - 1) == 0)
                    break;
            }
            else if(strstr(Str, EndText.c_str()) != NULL)
                break;

        }

        bool bFoundIgnoreString = false;
        for(CustomStringList * CurIgnoreString = IgnoreStrings; CurIgnoreString->Next(); CurIgnoreString = CurIgnoreString->Next())
        {
            if(re.compile(CurIgnoreString->Str()))
            {
                if(re.match(Str))
                    bFoundIgnoreString = true;
            }
            else if(*CurIgnoreString->Str() == '^')
            {
                if(strncmp(Str, CurIgnoreString->Str() + 1, strlen(CurIgnoreString->Str() + 1)) == 0)
                    bFoundIgnoreString = true;
            }
            else if(strstr(Str, CurIgnoreString->Str()) != NULL)
                bFoundIgnoreString = true;
        }

        if(bFoundIgnoreString)
            continue;

        if(re.compile(CurFormatNode->Str()))
        {
            if(Match match = re.match(Str))
                ParseListingItemRegExp(match, Item, Info, stModification, stCreation, stAccess);
        }
        else
            ParseListingItemPlain(CurFormatNode->Str(), Str, Item, Info, stModification, stCreation, stAccess);

        CurFormatNode = CurFormatNode->Next();
        if(!CurFormatNode || !CurFormatNode->Next())
        {
            MakeFiletime(stModification, syst, &Item->FindData.ftLastWriteTime);
            MakeFiletime(stCreation, syst, &Item->FindData.ftCreationTime);
            MakeFiletime(stAccess, syst, &Item->FindData.ftLastAccessTime);

            for(int I = strlen(Item->FindData.cFileName) - 1; I >= 0; I--)
            {
                int Ch = Item->FindData.cFileName[I];

                if(Ch == ' ' || Ch == '\t')
                    Item->FindData.cFileName[I] = 0;
                else
                    break;
            }
            return (GETARC_SUCCESS);
        }
    }

    return (GETARC_EOF);
}


BOOL WINAPI _export CUSTOM_CloseArchive(struct ArcInfo * Info)
{
    if(IgnoreErrors)
        Info->Flags |= AF_IGNOREERRORS;

    if(ArcChapters < 0)
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


BOOL WINAPI _export CUSTOM_GetFormatName(int Type, char *FormatName, char *DefaultExt)
{
    std::string TypeName;
	const KeyFileValues *Values = GetSection(Type, TypeName);

    if(!Values)
        return (FALSE);

    KeyFileValuePSZ(Values, Str_TypeName, TypeName.c_str(), FormatName, 64);
    KeyFileValuePSZ(Values, "Extension", "", DefaultExt, NM);

    return (*FormatName != 0);
}


BOOL WINAPI _export CUSTOM_GetDefaultCommands(int Type, int Command, char *Dest)
{
	std::string TypeName, FormatName;

	const KeyFileValues *Values = GetSection(Type, TypeName);

    if (!Values)
        return (FALSE);

    FormatName = Values->GetString(Str_TypeName, TypeName.c_str());

    if (FormatName.empty())
        return (FALSE);

    static const char *CmdNames[] = { "Extract", "ExtractWithoutPath", "Test", "Delete",
        "Comment", "CommentFiles", "SFX", "Lock", "Protect", "Recover",
        "Add", "Move", "AddRecurse", "MoveRecurse", "AllFilesMask"
    };

    if(Command < (int)(ARRAYSIZE(CmdNames)))
    {
        KeyFileValuePSZ(Values, CmdNames[Command], "", Dest, 512);
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
	for (const auto &i : FormatFileNameKFH) if (i.second) {
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
		struct stat s{};
		if (stat(i.first.c_str(), &s) == -1) {
			i.second.reset();

		} else {
			out = true;
			if (i.second) {
				const auto &ls = i.second->LoadedFileStat();
				if (s.st_ino == ls.st_ino
						&& s.st_mtime == ls.st_mtime
						&& s.st_size == ls.st_size) {
					continue;
				}
				i.second.reset();
			}
			i.second.reset(new KeyFileReadHelper(i.first));
		}
	}

	return out;
}

static DWORD KeyFileValuePSZ(const KeyFileValues *Values,LPCSTR lpKeyName,LPCSTR lpDefault,LPSTR lpReturnedString,DWORD nSize)
{
	const std::string &s = Values->GetString(lpKeyName, lpDefault);
	strncpy(lpReturnedString, s.c_str(), nSize);
	lpReturnedString[nSize - 1] = 0;
	return strlen(lpReturnedString);
}

static void FillFormat(const KeyFileValues *Values)
{
    StartText = Values->GetString("Start", "");
    EndText = Values->GetString("End", "");

    int FormatNumber = 0;

    delete Format;
    Format = new CustomStringList;
    for(CustomStringList * CurFormat = Format;; CurFormat = CurFormat->Add())
    {
        char FormatName[100];

        sprintf(FormatName, "Format%d", FormatNumber++);
        KeyFileValuePSZ(Values, FormatName, "", CurFormat->Str(), PROF_STR_LEN);
        if(*CurFormat->Str() == 0)
            break;
    }

    int Number = 0;

    delete IgnoreStrings;
    IgnoreStrings = new CustomStringList;
    for(CustomStringList * CurIgnoreString = IgnoreStrings;; CurIgnoreString = CurIgnoreString->Add())
    {
        char Name[100];

        sprintf(Name, "IgnoreString%d", Number++);
        KeyFileValuePSZ(Values, Name, "", CurIgnoreString->Str(), PROF_STR_LEN);
        if(*CurIgnoreString->Str() == 0)
            break;
    }
}

static int GetString(char *Str, int MaxSize)
{
	//memset(Str, 0, MaxSize);
    if(OutDataPos >= OutDataSize)
        return (FALSE);

    int StartPos = OutDataPos;

    while(OutDataPos < OutDataSize)
    {
        int Ch = OutData[OutDataPos];

        if(Ch == '\r' || Ch == '\n')
            break;
        OutDataPos++;
    }

    int Length = OutDataPos - StartPos;
    int DestLength = Length >= MaxSize ? MaxSize - 1 : Length;

    memcpy(Str, OutData + StartPos, DestLength);
	Str[DestLength] = 0;

    while(OutDataPos < OutDataSize)
    {
        int Ch = OutData[OutDataPos];

        if(Ch != '\r' && Ch != '\n')
            break;
        OutDataPos++;
    }

    return (TRUE);
}

static void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft)
{
    if(st.wDay == 0)
        st.wDay = syst.wDay;
    if(st.wMonth == 0)
        st.wMonth = syst.wMonth;
    if(st.wYear == 0)
        st.wYear = syst.wYear;
    else
    {
        if(st.wYear < 50)
            st.wYear += 2000;
        else if(st.wYear < 100)
            st.wYear += 1900;
    }

    FILETIME ft;

    if(WINPORT(SystemTimeToFileTime)(&st, &ft))
    {
        WINPORT(LocalFileTimeToFileTime)(&ft, pft);
    }
}

static int StringToInt(const char *str)
{
    int i = 0;
    for(const char *p = str; p && *p; ++p)
        if(isdigit(*p))
            i = i * 10 + (*p - '0');
    return i;
}

static int64_t StringToInt64(const char *str)
{
    int64_t i = 0;
    for(const char *p = str; p && *p; ++p)
        if(isdigit(*p))
            i = i * 10 + (*p - '0');
    return i;
}

static int StringToIntHex(const char *str)
{
    int i = 0;
    for(const char *p = str; p && *p; ++p)
        if(isxdigit(*p))
        {
            char dig_sub = (*p >= 'a' ? 'a' : (*p >= 'A' ? 'A' : '0'));
            i = i * 16 + (*p - dig_sub);
        }
    return i;
}

static void ParseListingItemRegExp(Match match,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess)
{
    if(const char *p = match["name"])
        strncpy(Item->FindData.cFileName, p, sizeof(Item->FindData.cFileName) );
    if(const char *p = match["description"])
        strncpy(Info->Description, p, sizeof(Info->Description) );

    FAR_INT64 SizeFile;
    SizeFile.i64 = StringToInt64(match["size"]);
    Item->FindData.nFileSizeLow  = SizeFile.Part.LowPart;
    Item->FindData.nFileSizeHigh = SizeFile.Part.HighPart;
    SizeFile.i64 = StringToInt64(match["packedSize"]);
    Item->PackSize               = SizeFile.Part.LowPart;
    Item->PackSizeHigh           = SizeFile.Part.HighPart;

    for(const char *p = match["attr"]; p && *p; ++p)
    {
        switch(*p)
        {
            case 'd': case 'D': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;  break;
            case 'h': case 'H': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;     break;
            case 'a': case 'A': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;    break;
            case 'r': case 'R': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;   break;
            case 's': case 'S': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;     break;
            case 'c': case 'C': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED; break;
            case 'x': case 'X': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_EXECUTABLE; break;
        }
    }

    stModification.wYear    = StringToInt(match["mYear"]);
    stModification.wDay     = StringToInt(match["mDay"]);
    stModification.wMonth   = StringToInt(match["mMonth"]);

    if(const char *p = match["mMonthA"])
    {
        static const char *Months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

        for(size_t I = 0; I < sizeof(Months) / sizeof(Months[0]); I++)
            if(strncasecmp(p, Months[I], 3) == 0)
            {
                stModification.wMonth = (WORD)(I + 1);
                break;
            }
    }

    stModification.wHour    = StringToInt(match["mHour"]);

    if(const char *p = match["mAMPM"])
    {
        switch(*p)
        {
        case 'A': case 'a':
            if(stModification.wHour == 12)
                stModification.wHour -= 12;
            break;
        case 'P': case 'p':
            if(stModification.wHour < 12)
                stModification.wHour += 12;
            break;
        }
    }

    stModification.wMinute  = StringToInt(match["mMin"]);
    stModification.wSecond  = StringToInt(match["mSec"]);

    stAccess.wDay           = StringToInt(match["aDay"]);
    stAccess.wMonth         = StringToInt(match["aMonth"]);
    stAccess.wYear          = StringToInt(match["aYear"]);
    stAccess.wHour          = StringToInt(match["aHour"]);
    stAccess.wMinute        = StringToInt(match["aMin"]);
    stAccess.wSecond        = StringToInt(match["aSec"]);

    stCreation.wDay         = StringToInt(match["cDay"]);
    stCreation.wMonth       = StringToInt(match["cMonth"]);
    stCreation.wYear        = StringToInt(match["cYear"]);
    stCreation.wHour        = StringToInt(match["cHour"]);
    stCreation.wMinute      = StringToInt(match["cMin"]);
    stCreation.wSecond      = StringToInt(match["cSec"]);

    Item->CRC32             = StringToIntHex(match["CRC"]);

}


static void ParseListingItemPlain(const char *CurFormat, const char *CurStr,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess)
{
    enum
    { OP_OUTSIDE, OP_INSIDE, OP_SKIP }
    OptionalPart = OP_OUTSIDE;
    int IsChapter = 0;

    FAR_INT64 SizeFile;

    for(; *CurStr && *CurFormat; CurFormat++, CurStr++)
    {
		if(OptionalPart == OP_SKIP)
        {
            if(*CurFormat == ')')
                OptionalPart = OP_OUTSIDE;
            CurStr--;
            continue;
        }
        switch (*CurFormat)
        {
        case '*':
            if(isspace(*CurStr) || !CurStr)
                CurStr--;
            else
                while( /*CurStr[0] && */ CurStr[1] /*&& !isspace(CurStr[0]) */
                      && !isspace(CurStr[1]))
                    CurStr++;
            break;
        case 'n':
            strncat(Item->FindData.cFileName, CurStr, 1);
            break;
        case 'c':
            strncat(Info->Description, CurStr, 1);
            break;
        case '.':
            {
                for(int I = strlen(Item->FindData.cFileName); I >= 0; I--)
                    if(isspace(Item->FindData.cFileName[I]))
                        Item->FindData.cFileName[I] = 0;
                if(*Item->FindData.cFileName)
                    strncat(Item->FindData.cFileName, ".", sizeof(Item->FindData.cFileName) );
            }
            break;
        case 'z':
            if(isdigit(*CurStr))
            {
                SizeFile.Part.LowPart=Item->FindData.nFileSizeLow;
                SizeFile.Part.HighPart=Item->FindData.nFileSizeHigh;
                SizeFile.i64=SizeFile.i64 * 10 + (*CurStr - '0');
                Item->FindData.nFileSizeLow=SizeFile.Part.LowPart;
                Item->FindData.nFileSizeHigh=SizeFile.Part.HighPart;
            }
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'p':
            if(isdigit(*CurStr))
            {
                SizeFile.Part.LowPart=Item->PackSize;
                SizeFile.Part.HighPart=Item->PackSizeHigh;
                SizeFile.i64=SizeFile.i64 * 10 + (*CurStr - '0');
                Item->PackSize=SizeFile.Part.LowPart;
                Item->PackSizeHigh=SizeFile.Part.HighPart;
            }
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'a':
            switch (*CurStr)
            {
                case 'd': case 'D': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;  break;
                case 'h': case 'H': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;     break;
                case 'a': case 'A': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;    break;
                case 'r': case 'R': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;   break;
                case 's': case 'S': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;     break;
                case 'c': case 'C': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED; break;
                case 'x': case 'X': Item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_EXECUTABLE; break;
            }
            break;
// MODIFICATION DATETIME
        case 'y':
            if(isdigit(*CurStr))
                stModification.wYear = stModification.wYear * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'd':
            if(isdigit(*CurStr))
                stModification.wDay = stModification.wDay * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 't':
            if(isdigit(*CurStr))
                stModification.wMonth = stModification.wMonth * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'T':
            {
                static const char *Months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                };

                for(size_t I = 0; I < sizeof(Months) / sizeof(Months[0]); I++)
                    if(strncasecmp(CurStr, Months[I], 3) == 0)
                    {
                        stModification.wMonth = (WORD)(I + 1);
                        while(CurFormat[1] == 'T' && CurStr[1])
                        {
                            CurStr++;
                            CurFormat++;
                        }
                        break;
                    }
            }
            break;
        case 'h':
            if(isdigit(*CurStr))
                stModification.wHour = stModification.wHour * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'H':
            switch (*CurStr)
            {
                case 'A': case 'a':
                    if(stModification.wHour == 12)
                        stModification.wHour -= 12;
                    break;
                case 'P': case 'p':
                    if(stModification.wHour < 12)
                        stModification.wHour += 12;
                    break;
            }
            break;
        case 'm':
            if(isdigit(*CurStr))
                stModification.wMinute = stModification.wMinute * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 's':
            if(isdigit(*CurStr))
                stModification.wSecond = stModification.wSecond * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
// ACCESS DATETIME
        case 'b':
            if(isdigit(*CurStr))
                stAccess.wDay = stAccess.wDay * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'v':
            if(isdigit(*CurStr))
                stAccess.wMonth = stAccess.wMonth * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'e':
            if(isdigit(*CurStr))
                stAccess.wYear = stAccess.wYear * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'x':
            if(isdigit(*CurStr))
                stAccess.wHour = stAccess.wHour * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'l':
            if(isdigit(*CurStr))
                stAccess.wMinute = stAccess.wMinute * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'k':
            if(isdigit(*CurStr))
                stAccess.wSecond = stAccess.wSecond * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
// CREATION DATETIME
        case 'j':
            if(isdigit(*CurStr))
                stCreation.wDay = stCreation.wDay * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'g':
            if(isdigit(*CurStr))
                stCreation.wMonth = stCreation.wMonth * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'f':
            if(isdigit(*CurStr))
                stCreation.wYear = stCreation.wYear * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'o':
            if(isdigit(*CurStr))
                stCreation.wHour = stCreation.wHour * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'i':
            if(isdigit(*CurStr))
                stCreation.wMinute = stCreation.wMinute * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'u':
            if(isdigit(*CurStr))
                stCreation.wSecond = stCreation.wSecond * 10 + (*CurStr - '0');
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'r':
            if(isxdigit(toupper(*CurStr)))
            {
                char dig_sub = (*CurStr >= 'a' ? 'a' : (*CurStr >= 'A' ? 'A' : '0'));

                Item->CRC32 = Item->CRC32 * 16 + (*CurStr - dig_sub);
            }
            else if(OP_INSIDE == OptionalPart)
            {
                CurStr--;
                OptionalPart = OP_SKIP;
            }
            break;
        case 'C':
            if(*CurStr == '-')
            {
                IsChapter = 1;
                ArcChapters = 0;
            }
            else if(isdigit(*CurStr))
            {
                if(IsChapter)
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
