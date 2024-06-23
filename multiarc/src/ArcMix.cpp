#include "MultiArc.hpp"
#include "marclng.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <windows.h>
#include <Environment.h>
#include <string>
#include <vector>

extern std::string gMultiArcPluginPath;

BOOL FileExists(const char *Name)
{
	struct stat s = {0};
	return sdc_stat(Name, &s) != -1;
}

BOOL GoToFile(const char *Target, BOOL AllowChangeDir)
{
	if (!Target || !*Target)
		return FALSE;

	const char *TargetName = FSF.PointToName(const_cast<char *>(Target));
	std::string TargetDir(Target, TargetName - Target);

	PanelInfo PInfo;
	Info.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, (void *)1);
	Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);

	if (!TargetDir.empty()) {
		if (*PInfo.CurDir && PInfo.CurDir[strlen(PInfo.CurDir) - 1] != '/') {		// old path != "*\"
			StrTrimRight(TargetDir, "/");
		}
		if (TargetDir != PInfo.CurDir) {
			if (!AllowChangeDir) {
				return FALSE;
			}
			Info.Control(INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, (void *)TargetDir.c_str());
			Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);
		}
	}

	for (int i = 0; i < PInfo.ItemsNumber; i++) {
		if (!strcmp(TargetName, FSF.PointToName(PInfo.PanelItems[i].FindData.cFileName))) {
			PanelRedrawInfo PRI{};
			PRI.CurrentItem = i;
			PRI.TopPanelItem = i;
			return Info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, &PRI);
		}
	}

	return FALSE;
}

int __isspace(int Chr)
{
	return Chr == 0x09 || Chr == 0x0A || Chr == 0x0B || Chr == 0x0C || Chr == 0x0D || Chr == 0x20;
}

const char *GetMsg(int MsgId)
{
	return Info.GetMsg(Info.ModuleNumber, MsgId);
}

#ifdef HAVE_UNRAR
int rar_main(int argc, char *argv[]);
#endif
extern "C" int sevenz_main(int argc, char *argv[]);
extern "C" int ha_main(int argc, char *argv[]);

#ifdef HAVE_LIBARCHIVE
extern "C" int libarch_main(int numargs, char *args[]);
#endif

SHAREDSYMBOL int BuiltinMain(int argc, char *argv[])
{
	if (!argc)
		return -1;

	int r = -2;

	if (strcmp(argv[0], "7z") == 0) {
		r = sevenz_main(argc, &argv[0]);
#ifdef HAVE_UNRAR
	} else if (strcmp(argv[0], "rar") == 0) {
		r = rar_main(argc, &argv[0]);
#endif
	} else if (strcmp(argv[0], "7z") == 0) {
		r = sevenz_main(argc, &argv[0]);
	} else if (strcmp(argv[0], "ha") == 0) {
		r = ha_main(argc, &argv[0]);
#ifdef HAVE_LIBARCHIVE
	} else if (strcmp(argv[0], "libarch") == 0) {
		r = libarch_main(argc, &argv[0]);
#endif
	} else
		fprintf(stderr, "BuiltinMain: bad target '%s'\n", argv[0]);
	return r;
}

/* $ 13.09.2000 tran
   запуск треда для ожидания момента убийства лист файла */
#if 0
static DWORD WINAPI ThreadWhatWaitingForKillListFile(LPVOID par)
{
    KillStruct *ks=(KillStruct*)par;

    WINPORT(WaitForSingleObject)(ks->hProcess,INFINITE);
    WINPORT(CloseHandle)(ks->hThread);
    WINPORT(CloseHandle)(ks->hProcess);
    sdc_unlink(ks->ListFileName);
    free((LPVOID)ks);
    return SUPER_PUPER_ZERO;
}
void StartThreadForKillListFile(PROCESS_INFORMATION *pi,char *list)
{
    if ( pi==0 || list==0 || *list==0)
        return;
    KillStruct *ks;
    DWORD dummy;

    ks=(KillStruct*)malloc(GPTR,sizeof(KillStruct));
    if ( ks==0 )
        return ;

    ks->hThread=pi->hThread;
    ks->hProcess=pi->hProcess;
    strcpy(ks->ListFileName,list);

    WINPORT(CloseHandle)(WINPORT(CreateThread)(NULL,0xf000,ThreadWhatWaitingForKillListFile,ks,0 ,&dummy));
}

/* tran 13.09.2000 $ */
#endif

int Execute(HANDLE hPlugin, const std::string &CmdStr,
	int HideOutput, int Silent, int NeedSudo, int ShowCommand, const char *ListFileName)
{
	if (!CmdStr.empty() && (CmdStr[0] == ' ' || CmdStr[0] == '\t')) {	// FSF.LTrim(ExpandedCmd); //$ AA 12.11.2001
		std::string CmdStrTrimmed = CmdStr;
		do {
			CmdStrTrimmed.erase(0, 1);
		} while (!CmdStrTrimmed.empty() && (CmdStrTrimmed[0] == ' ' || CmdStrTrimmed[0] == '\t'));

		return Execute(hPlugin, CmdStrTrimmed, HideOutput, Silent, NeedSudo, ShowCommand, ListFileName);
	}

	HANDLE hScreen = NULL;
	WCHAR SaveTitle[512];
	if (HideOutput && !Silent) {
		hScreen = Info.SaveScreen(0, 0, -1, -1);
		const char *MsgItems[] = {"", GetMsg(MWaitForExternalProgram)};
		Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 0);
	}
	if (ShowCommand) {
		WINPORT(GetConsoleTitle)(NULL, SaveTitle, ARRAYSIZE(SaveTitle) - 1);
		SaveTitle[ARRAYSIZE(SaveTitle) - 1] = 0;
		WINPORT(SetConsoleTitle)(NULL, StrMB2Wide(CmdStr).c_str());
	}

	int ExitCode;

	DWORD flags = (HideOutput) ? EF_HIDEOUT : 0;
	if (NeedSudo)
		flags|= EF_SUDO;
	if (!ShowCommand)
		flags|= EF_NOCMDPRINT;

	if (*CmdStr.c_str() == '^') {
		ExitCode = FSF.ExecuteLibrary(gMultiArcPluginPath.c_str(), "BuiltinMain", CmdStr.c_str() + 1, flags);
	} else {
		ExitCode = FSF.Execute(CmdStr.c_str(), flags);
	}


	if (ShowCommand) {
		WINPORT(SetConsoleTitle)(NULL, SaveTitle);
	}
	if (hScreen) {
		Info.RestoreScreen(NULL);
		Info.RestoreScreen(hScreen);
	}

	return ExitCode;
}

char *QuoteText(char *Str)
{
	int LastPos = strlen(Str);
	memmove(Str + 1, Str, LastPos + 1);
	Str[LastPos + 1] = *Str = '"';
	Str[LastPos + 2] = 0;
	return Str;
}

void InitDialogItems(const struct InitDialogItem *Init, struct FarDialogItem *Item, int ItemsNumber)
{
	int I;
	struct FarDialogItem *PItem = Item;
	const struct InitDialogItem *PInit = Init;
	for (I = 0; I < ItemsNumber; I++, PItem++, PInit++) {
		PItem->Type = PInit->Type;
		PItem->X1 = PInit->X1;
		PItem->Y1 = PInit->Y1;
		PItem->X2 = PInit->X2;
		PItem->Y2 = PInit->Y2;
		PItem->Focus = PInit->Focus;
		PItem->History = (const char *)PInit->Selected;
		PItem->Flags = PInit->Flags;
		PItem->DefaultButton = PInit->DefaultButton;
		CharArrayCpyZ(PItem->Data,
				((DWORD_PTR)PInit->Data < 2000) ? GetMsg((unsigned int)(DWORD_PTR)PInit->Data) : PInit->Data);
	}
}

std::string NumberWithCommas(unsigned long long Number)
{
	std::string out = std::to_string(Number);
	for (int I = int(out.size()) - 4; I >= 0; I-= 3)
		out.insert(I + 1, 1, ',');

	return out;
}

int MA_ToPercent(int32_t N1, int32_t N2)
{
	if (N1 > 10000) {
		N1/= 100;
		N2/= 100;
	}
	if (N2 == 0)
		return 0;
	if (N2 < N1)
		return 100;
	return (int)(N1 * 100 / N2);
}

int MA_ToPercent(int64_t N1, int64_t N2)
{
	if (N1 > 10000) {
		N1/= 100;
		N2/= 100;
	}
	if (N2 == 0)
		return 0;
	if (N2 < N1)
		return 100;
	return (int)(N1 * 100 / N2);
}

int IsCaseMixed(const char *Str)
{
	while (*Str && !isalpha(*Str))
		Str++;
	int Case = islower(*Str);
	while (*(Str++))
		if (isalpha(*Str) && islower(*Str) != Case)
			return TRUE;
	return FALSE;
}

int CheckForEsc()
{
	WORD KeyCode = VK_ESCAPE;
	return WINPORT(CheckForKeyPress)(NULL, &KeyCode, 1, CFKP_KEEP_OTHER_EVENTS) != 0;
}

char *GetCommaWord(char *Src, char *Word, char Separator)
{
	int WordPos, SkipBrackets;
	if (*Src == 0)
		return NULL;
	SkipBrackets = FALSE;
	for (WordPos = 0; *Src != 0; Src++, WordPos++) {
		if (*Src == '[' && strchr(Src + 1, ']') != NULL)
			SkipBrackets = TRUE;
		if (*Src == ']')
			SkipBrackets = FALSE;
		if (*Src == Separator && !SkipBrackets) {
			Word[WordPos] = 0;
			Src++;
			while (__isspace(*Src))
				Src++;
			return Src;
		} else
			Word[WordPos] = *Src;
	}
	Word[WordPos] = 0;
	return Src;
}

int FindExecuteFile(char *OriginalName, char *DestName, int SizeDest)
{
	std::string cmd = "which ";
	cmd+= OriginalName;
	FILE *f = popen(cmd.c_str(), "r");
	if (f == NULL) {
		perror("FindExecuteFile - popen");
		return FALSE;
	}
	if (!fgets(DestName, SizeDest - 1, f))
		DestName[0] = 0;
	else
		DestName[SizeDest - 1] = 0;
	pclose(f);

	char *e = strchr(DestName, '\n');
	if (e)
		*e = 0;
	e = strchr(DestName, '\r');
	if (e)
		*e = 0;
	return DestName[0] ? TRUE : FALSE;
}

size_t FindExt(const std::string &Name)
{
	size_t p = Name.find_last_of("./");
	if (p == std::string::npos || Name[p] != '.') {
		return std::string::npos;
	}
	return p + 1;
}

bool DelExt(std::string &Name, const std::string &Ext)
{
	const size_t p = FindExt(Name);
	if (p != std::string::npos && strcasecmp(Name.c_str() + p, Ext.c_str()) == 0) {
		Name.resize(p - 1);
		return true;
	}
	return false;
}

bool AddExt(std::string &Name, const std::string &Ext)
{// FSF.Unquote(Name);      //$ AA 15.04.2003 для правильной обработки имен в кавычках
	size_t p = FindExt(Name);
	if (p != std::string::npos) {
		if (strcasecmp(Ext.c_str(), Name.c_str() + p) == 0) {
			return false;
		}
		Name.resize(p);
	} else {
		Name+= '.';
		p = Name.size();
	}
	if (Ext.empty()) {
		return false;
	}

	bool has_lowers = false, has_uppers = false;
	for (const auto &c : Name) {
		if (isalpha(c)) {
			has_lowers = has_lowers || islower(c);
			has_uppers = has_uppers || isupper(c);
		}
	}
	Name+= Ext;
	if (has_lowers || has_uppers) {
		for (; p < Name.size(); ++p) {
			Name[p] = has_lowers ? tolower(Name[p]) : toupper(Name[p]);
		}
	}
	return true;
}

std::string FormatMessagePath(const char *path, bool extract_name, int truncate)
{
	if (extract_name) {
		path = FSF.PointToName((char *)path);
	}
	char buf[NM];
	strncpy(buf, path, ARRAYSIZE(buf) - 1);
	buf[ARRAYSIZE(buf) - 1] = 0;
	if (truncate >= 0) {
		FSF.TruncPathStr(buf, truncate ? truncate : (GetScrX() - 14));
	}
	return buf;
}

int WINAPI GetPassword(std::string &Password, const char *FileName)
{
	char InPass[512];
	CharArrayCpyZ(InPass, Password.c_str());
	const auto &Prompt = StrPrintf(GetMsg(MGetPasswordForFile), FileName);
	if (Info.InputBox((const char *)GetMsg(MGetPasswordTitle), Prompt.c_str(), NULL, NULL, InPass,
				sizeof(InPass), NULL, FIB_PASSWORD | FIB_ENABLEEMPTY)) {
		CharArrayAssignToStr(Password, InPass);
		return TRUE;
	}
	return FALSE;
}

// Number of 100 nanosecond units from 01.01.1601 to 01.01.1970
#define EPOCH_BIAS I64(116444736000000000)

void WINAPI UnixTimeToFileTime(DWORD time, FILETIME *ft)
{
	*(int64_t *)ft = EPOCH_BIAS + time * I64(10000000);
}

int GetScrX(void)
{
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if (WINPORT(GetConsoleScreenBufferInfo)(NULL, &ConsoleScreenBufferInfo))	// GetStdHandle(STD_OUTPUT_HANDLE)
		return ConsoleScreenBufferInfo.dwSize.X;
	return 0;
}

int PathMayBeAbsolute(const char *Path)
{
	return (Path && *Path == '/');
}

/*
  преобразует строку
	"cdrecord-1.6.1/mkisofs-1.12b4/../cdrecord/cd_misc.c"
  в
	"cdrecord-1.6.1/cdrecord/cd_misc.c"
*/
void NormalizePath(const char *lpcSrcName, char *lpDestName)
{
	char *DestName = lpDestName;
	char *Ptr;
	char *SrcName = strdup(lpcSrcName);
	char *oSrcName = SrcName;
	int dist;

	while (*SrcName) {
		Ptr = strchr(SrcName, '/');

		if (!Ptr)
			Ptr = SrcName + strlen(SrcName);

		dist = (int)(Ptr - SrcName) + 1;

		if (dist == 1 && (*SrcName == '/')) {
			*DestName = *SrcName;
			DestName++;
			SrcName++;
		} else if (dist == 2 && *SrcName == '.') {
			SrcName++;

			if (*SrcName == 0)
				DestName--;
			else
				SrcName++;
		} else if (dist == 3 && *SrcName == '.' && SrcName[1] == '.') {
			if (!PathMayBeAbsolute(lpDestName)) {
				char *ptrCurDestName = lpDestName, *Temp = NULL;

				for (; ptrCurDestName < DestName - 1; ptrCurDestName++) {
					if (*ptrCurDestName == '/')
						Temp = ptrCurDestName;
				}

				if (!Temp)
					Temp = lpDestName;

				DestName = Temp;
			} else {
				if (SrcName[2] == '/')
					SrcName++;
			}

			SrcName+= 2;
		} else {
			strncpy(DestName, SrcName, dist);
			dist--;
			DestName+= dist;
			SrcName+= dist;
		}

		*DestName = 0;
	}

	free(oSrcName);
}

std::string &NormalizePath(std::string &path)
{
	std::vector<char> dest(std::max((size_t)MAX_PATH, path.size()) + 1);
	NormalizePath(path.c_str(), &dest[0]);
	path = &dest[0];
	return path;
}

std::string &ExpandEnv(std::string &str)
{
	Environment::ExpandString(str, false);
	return str;
}

bool CanBeExecutableFileHeader(const unsigned char *Data, int DataSize)
{
	if (DataSize < 16)
		return false;

	if (Data[0] == 0x7f && Data[1] == 'E' && Data[2] == 'L' && Data[3] == 'F')
		return true;

	if (Data[0] == 'M' && Data[1] == 'Z')
		return true;

	if (Data[0] == 'Z' && Data[1] == 'M')
		return true;

	if (Data[0] == 0xca && Data[1] == 0xfe && Data[2] == 0xba && Data[3] == 0xbe)
		return true;

	if (Data[0] == 0xfe && Data[1] == 0xed && Data[2] == 0xfa && (Data[3] == 0xce || Data[3] == 0xcf))
		return true;

	if (Data[3] == 0xfe && Data[2] == 0xed && Data[1] == 0xfa && (Data[0] == 0xce || Data[0] == 0xcf))
		return true;

	return false;
}

std::string GetDialogControlText(HANDLE hDlg, int id)
{
	int len = Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, id, 0);
	std::vector<char> buf(std::max(len + 1, 32));
	Info.SendDlgMessage(hDlg, DM_GETTEXTPTR, id, (ULONG_PTR)buf.data());
	return std::string(buf.data());
}

void SetDialogControlText(HANDLE hDlg, int id, const char *str)
{
	Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, id, (ULONG_PTR)str);
}

void SetDialogControlText(HANDLE hDlg, int id, const std::string &str)
{
	Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, id, (ULONG_PTR)str.c_str());
}
