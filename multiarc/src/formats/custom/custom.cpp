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
using namespace oldfar;
#include "fmt.hpp"
#include <errno.h>

#define PCRE_STATIC
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

BOOL WINAPI OpenArchivePipe(const char *Name, int *Type);
int GetString(char *Str, int MaxSize);
int HexCharToNum(int HexChar);
int GetSectionName(int Num, char *Name, int MaxSize);

DWORD GetIniString(LPCSTR lpAppName,LPCSTR lpKeyName,LPCSTR lpDefault,LPSTR lpReturnedString,DWORD nSize);
UINT GetIniInt(LPCSTR lpAppName,LPCSTR lpKeyName,INT lpDefault);

void FillFormat(const char *TypeName);
void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft);
int StringToInt(const char *str);
int64_t StringToInt64(const char *str);
int StringToIntHex(const char *str);
void ParseListingItemRegExp(Match match,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess);
void ParseListingItemPlain(const char *CurFormat, const char *CurStr,
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
    char *m_command;
    char m_fileName[NM];

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
    MetaReplacer(const char *command, const char *arcName)
        :   m_command(strdup(command))
    {
        strncpy(m_fileName, arcName, sizeof(m_fileName));
    }

    virtual ~MetaReplacer()
    {
        free(m_command);
    }

    void replaceTo(char *buffer)
    {
        char *dest = buffer;

        *dest = 0;

        bool bReplacedSomething = false;

        for(const char *command = m_command; *command;)
        {
            Meta m(command);

            if(!m.isValidMeta())
            {
                *dest++ = *command++;
                *dest = 0;
                continue;
            }

            command += m.getLength();
            bReplacedSomething = true;

            char *var = m_fileName;
            char *lastSlash = strrchr(var, '/');

            if(m.getFlags() & Meta::useNameOnly)
            {
                var = lastSlash ? lastSlash + 1 : var;
            }
            else if(m.getFlags() & Meta::usePathOnly)
            {
                if(lastSlash)
                    *lastSlash = 0;
            }

            bool bQuote = (m.getFlags() & Meta::quoteAll)
                || ((m.getFlags() & Meta::quoteWithSpaces) && strchr(var, ' '));

            if(bQuote)
                strcat(dest, "\"");

            strcat(dest, var);

            if(bQuote)
                strcat(dest, "\"");

            if(m.getFlags() & Meta::useForwardSlashes)
            {
            }


            if(lastSlash)
                *lastSlash = '/';

            while(*dest)
                dest++;

        }

        if(!bReplacedSomething) // there were no meta-symbols, should use old-style method
        {
            strcat(buffer, " ");
            strcat(buffer, m_fileName);
        }

    }
};


///////////////////////////////////////////////////////////////////////////////
// Variables

int     CurType;
char    *OutData;
DWORD   OutDataPos, OutDataSize;

char    FormatFileName[NM],UserFormatFileName[NM];


char    StartText[PROF_STR_LEN], EndText[PROF_STR_LEN];

CustomStringList *Format = 0;
CustomStringList *IgnoreStrings = 0;

int     IgnoreErrors;
int     ArcChapters;

const char Str_TypeName[] = "TypeName";


///////////////////////////////////////////////////////////////////////////////
// Library function pointers

FARSTDMKTEMP        MkTemp;


///////////////////////////////////////////////////////////////////////////////
// Exported functions

void WINAPI _export CUSTOM_SetFarInfo(const struct PluginStartupInfo *Info)
{
    MkTemp = Info->FSF->MkTemp;
}

DWORD WINAPI _export CUSTOM_LoadFormatModule(const char *ModuleName)
{
    strcpy(FormatFileName, ModuleName);
    strcpy(strrchr(FormatFileName, '/') + 1, "custom.ini");
    strcpy(UserFormatFileName,FormatFileName);
    strcpy(strrchr(UserFormatFileName,'.'),"_user.ini");
    return (0);
}

BOOL WINAPI _export CUSTOM_IsArchive(const char *Name, const unsigned char *Data, int DataSize)
{
    char *Dot = strrchr((char *) Name, '.');

    for(int I = 0;; I++)
    {
        char TypeName[NM], Name[NM], Ext[NM], ID[512];
        int IDPos;

        if(!GetSectionName(I, TypeName, sizeof(TypeName)))
            break;

        GetIniString(TypeName, Str_TypeName, TypeName, Name, sizeof(Name));

        if(*Name == 0)
            break;

        GetIniString(TypeName, "ID", "", ID, sizeof(ID));
        IDPos = GetIniInt(TypeName, "IDPos", -1);

        if(*ID)
        {
            unsigned char IDData[256], *CurID = (unsigned char *) &ID[0];
            int IDLength = 0;

            while(1)
            {
                while(isspace(*CurID))
                    CurID++;
                if(*CurID == 0)
                    break;
                IDData[IDLength++] = HexCharToNum(CurID[0]) * 16 + HexCharToNum(CurID[1]);
                while(*CurID && !isspace(*CurID))
                    CurID++;
            }

            int Found = FALSE;

            if(IDPos >= 0)
                Found = (IDPos <= DataSize - IDLength) && (memcmp(Data + IDPos, IDData, IDLength) == 0);
            else
            {
                for(int I = 0; I <= DataSize - IDLength; I++)
                    if(memcmp(Data + I, IDData, IDLength) == 0)
                    {
                        Found = TRUE;
                        break;
                    }
            }
            if(Found)
            {
                if(GetIniInt(TypeName, "IDOnly", 0))
                {
                    CurType = I;
                    return (TRUE);
                }
            }
            else
                continue;
        }

        GetIniString(TypeName, "Extension", "", Ext, sizeof(Ext));

        if(Dot != NULL && *Ext != 0 && strcasecmp(Dot + 1, Ext) == 0)
        {
            CurType = I;
            return (TRUE);
        }
    }
    return (FALSE);
}

DWORD WINAPI _export CUSTOM_GetSFXPos(void)
{
    return 0;
}


BOOL WINAPI _export CUSTOM_OpenArchive(const char *Name, int *Type)
{
    char TypeName[NM], Command[512];

    if(!GetSectionName(CurType, TypeName, sizeof(TypeName)))
        return (FALSE);

    GetIniString(TypeName, "List", "", Command, sizeof(Command));

    if(*Command == 0)
        return (FALSE);

    IgnoreErrors = GetIniInt(TypeName, "IgnoreErrors", 0);
    *Type = CurType;

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
    WCHAR SaveTitle[512];

    WINPORT(GetConsoleTitle)(SaveTitle, sizeof(SaveTitle));
    WINPORT(SetConsoleTitle)(MB2Wide(Command).c_str());

    //char ExpandedCmd[512];

    //WINPORT(ExpandEnvironmentStrings)(Command, ExpandedCmd, sizeof(ExpandedCmd));
	std::string cmd = Command;
	cmd+= " 2>&1 >";
	cmd+= TempName;
	DWORD ExitCode = system(cmd.c_str());
	

    if(ExitCode)
    {
        ExitCode = (ExitCode < GetIniInt(TypeName, "Errorlevel", 1000));
    }

    if(ExitCode)
    {
        OutData = NULL;
		int fd = sdc_open(TempName, O_RDONLY);
		if (fd!=-1) {
			struct stat s = {0};
			sdc_fstat(fd, &s);
			if (s.st_size > 0) {
				OutData = (char *) calloc(s.st_size + 1, 1);
				if (OutData) {
					for (off_t i = 0; i < s.st_size;) {
						int piece = (s.st_size - i < 0x10000) ? s.st_size - i : 0x10000;
						int r = sdc_read(fd, OutData + i, piece);
						if (r<=0) break;
						i+= r;
					}
				}
			}
			sdc_close(fd);
		}
		
        if(OutData == NULL)
            ExitCode = 0;
    }

    WINPORT(SetConsoleTitle)(SaveTitle);
    WINPORT(SetConsoleMode)(NULL, ConsoleMode);

    FillFormat(TypeName);

    if(ExitCode && OutDataSize == 0)
    {
        free((HGLOBAL) OutData);
        return (OpenArchivePipe(Name, Type));
    }

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

        if(*StartText)
        {
            if(re.compile(StartText))
            {
                if(re.match(Str))
                    *StartText = 0;
            }
            else
            {
                if((*StartText == '^' && strncmp(Str, StartText + 1, strlen(StartText + 1)) == 0) ||
                   (*StartText != '^' && strstr(Str, StartText) != NULL))
                {
                    *StartText = 0;
                }
            }
            continue;
        }

        if(*EndText)
        {
            if(re.compile(EndText))
            {
                if(re.match(Str))
                    break;
            }
            else if(*EndText == '^')
            {
                if(strncmp(Str, EndText + 1, strlen(EndText + 1)) == 0)
                    break;
            }
            else if(strstr(Str, EndText) != NULL)
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

    free((HGLOBAL) OutData);

    delete Format;
    delete IgnoreStrings;
    Format = 0;
    IgnoreStrings = 0;

    return (TRUE);
}


BOOL WINAPI _export CUSTOM_GetFormatName(int Type, char *FormatName, char *DefaultExt)
{
    char TypeName[NM];

    if(!GetSectionName(Type, TypeName, sizeof(TypeName)))
        return (FALSE);

    GetIniString(TypeName, Str_TypeName, TypeName, FormatName, 64);
    GetIniString(TypeName, "Extension", "", DefaultExt, NM);

    return (*FormatName != 0);
}


BOOL WINAPI _export CUSTOM_GetDefaultCommands(int Type, int Command, char *Dest)
{
    char TypeName[NM], FormatName[NM];

    if(!GetSectionName(Type, TypeName, sizeof(TypeName)))
        return (FALSE);

    GetIniString(TypeName, Str_TypeName, TypeName, FormatName, 64);

    if(*FormatName == 0)
        return (FALSE);

    static const char *CmdNames[] = { "Extract", "ExtractWithoutPath", "Test", "Delete",
        "Comment", "CommentFiles", "SFX", "Lock", "Protect", "Recover",
        "Add", "Move", "AddRecurse", "MoveRecurse", "AllFilesMask"
    };

    if(Command < (int)(ARRAYSIZE(CmdNames)))
    {
        GetIniString(TypeName, CmdNames[Command], "", Dest, 512);
        return (TRUE);
    }

    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

int HexCharToNum(int HexChar)
{
    if(HexChar >= '0' && HexChar <= '9')
        return (HexChar - '0');
    else if(HexChar >= 'A' && HexChar <= 'F')
        return (HexChar - 'A' + 10);
    else if(HexChar >= 'a' && HexChar <= 'f')
        return (HexChar - 'a' + 10);
    return (0);
}



int GetSectionName(int Num, char *Name, int MaxSize)
{
	{
		KeyFileHelper kfh(FormatFileName);
		const std::vector<std::string> &sections = kfh.EnumSections();
		if (Num < sections.size()) {
			strncpy(Name, sections[Num].c_str(), MaxSize);
			return TRUE;		
		}
		Num-= sections.size();
	}
	{
		KeyFileHelper kfh(UserFormatFileName);
		const std::vector<std::string> &sections = kfh.EnumSections();
		if (Num < sections.size()) {
			strncpy(Name, sections[Num].c_str(), MaxSize);
			return TRUE;		
		}
	}

	return FALSE;
}

DWORD GetIniString(LPCSTR lpAppName,LPCSTR lpKeyName,LPCSTR lpDefault,LPSTR lpReturnedString,DWORD nSize)
{
	KeyFileHelper(FormatFileName).GetChars(lpReturnedString, nSize, lpAppName,lpKeyName, lpDefault);
	KeyFileHelper(UserFormatFileName).GetChars(lpReturnedString, nSize, lpAppName,lpKeyName, lpReturnedString);
	return strlen(lpReturnedString);
}

UINT GetIniInt(LPCSTR lpAppName,LPCSTR lpKeyName,INT lpDefault)
{
	KeyFileHelper(FormatFileName).GetInt(lpAppName,lpKeyName, KeyFileHelper(UserFormatFileName).GetInt(lpAppName,lpKeyName, lpDefault) );
}

void FillFormat(const char *TypeName)
{
    GetIniString(TypeName, "Start", "", StartText, sizeof(StartText));
    GetIniString(TypeName, "End", "", EndText, sizeof(EndText));

    int FormatNumber = 0;

    delete Format;
    Format = new CustomStringList;
    for(CustomStringList * CurFormat = Format;; CurFormat = CurFormat->Add())
    {
        char FormatName[100];

        sprintf(FormatName, "Format%d", FormatNumber++);
        GetIniString(TypeName, FormatName, "", CurFormat->Str(), PROF_STR_LEN);
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
        GetIniString(TypeName, Name, "", CurIgnoreString->Str(), PROF_STR_LEN);
        if(*CurIgnoreString->Str() == 0)
            break;
    }
}

int GetString(char *Str, int MaxSize)
{
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

    strncpy(Str, OutData + StartPos, DestLength + 1);

    while(OutDataPos < OutDataSize)
    {
        int Ch = OutData[OutDataPos];

        if(Ch != '\r' && Ch != '\n')
            break;
        OutDataPos++;
    }

    return (TRUE);
}

void MakeFiletime(SYSTEMTIME st, SYSTEMTIME syst, LPFILETIME pft)
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


BOOL WINAPI OpenArchivePipe(const char *Name, int *Type)
{
    char TypeName[NM], Command[512];

    if(!GetSectionName(CurType, TypeName, sizeof(TypeName)))
        return (FALSE);
    GetIniString(TypeName, "List", "", Command, sizeof(Command));
    if(*Command == 0)
        return (FALSE);
    *Type = CurType;


    MetaReplacer meta(Command, Name);

    meta.replaceTo(Command);
	int fd[2];
	if (pipe(fd)==-1) {
		perror("OpenArchivePipe: pipe");
		return FALSE;
	}
	int pid = fork();
	if (pid==-1) {
		perror("OpenArchivePipe: fork");
		close(fd[0]);
		close(fd[1]);
		return FALSE;		
	}
	
	if (pid==0) {
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		close(fd[0]); close(fd[1]);
		execl("/bin/sh", "sh", "-c", Command, NULL);
		perror("OpenArchivePipe: execl");
		exit(errno ? errno : -1);
	} 
	
	
    DWORD ConsoleMode;

    WINPORT(GetConsoleMode)(NULL, &ConsoleMode);
    WINPORT(SetConsoleMode)(NULL, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                   ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT);
    WCHAR SaveTitle[512];

    WINPORT(GetConsoleTitle)(SaveTitle, sizeof(SaveTitle));
    WINPORT(SetConsoleTitle)(MB2Wide(Command).c_str());

	const int ReadSize = 32768;

	OutDataSize = OutDataPos = 0;
	OutData = (char *) NULL;//malloc(GMEM_FIXED, 0);

	while(1) {
		char *tmp = (char *) realloc(OutData, OutDataSize + ReadSize);
		if (!tmp) {
			perror("OpenArchivePipe: realloc");
			free(OutData);
			close(fd[0]); close(fd[1]);
			return FALSE;
		}
		OutData = tmp;
		int r = read(fd[0], OutData + OutDataSize, ReadSize);
		if (r<=0) break;
		OutDataSize += r;
	}
	
	int status = 0x7fff;
	if (waitpid(pid, &status, 0)==-1) {
		perror("OpenArchivePipe: waitpid");
	}
	BOOL out = (status < GetIniInt(TypeName, "Errorlevel", 1000));

	if(!out) {
		free(OutData);
		OutData = NULL;
	}

    WINPORT(SetConsoleTitle)(SaveTitle);
    WINPORT(SetConsoleMode)(NULL, ConsoleMode);

    FillFormat(TypeName);

    return (out);
}

int StringToInt(const char *str)
{
    int i = 0;
    for(const char *p = str; p && *p; ++p)
        if(isdigit(*p))
            i = i * 10 + (*p - '0');
    return i;
}

int64_t StringToInt64(const char *str)
{
    int64_t i = 0;
    for(const char *p = str; p && *p; ++p)
        if(isdigit(*p))
            i = i * 10 + (*p - '0');
    return i;
}

int StringToIntHex(const char *str)
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

void ParseListingItemRegExp(Match match,
    struct PluginPanelItem *Item, struct ArcItemInfo *Info,
    SYSTEMTIME &stModification, SYSTEMTIME &stCreation, SYSTEMTIME &stAccess)
{

    if(const char *p = match["name"])
        strcat(Item->FindData.cFileName, p);
    if(const char *p = match["description"])
        strcat(Info->Description, p);

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


void ParseListingItemPlain(const char *CurFormat, const char *CurStr,
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
                    strcat(Item->FindData.cFileName, ".");
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
