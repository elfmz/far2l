#include "MultiArc.hpp"
#include "marclng.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <windows.h>
#include <string>
#include <vector>

extern std::string gMultiArcPluginPath;

BOOL FileExists(const char* Name)
{
  struct stat s = {0};
  return sdc_stat(Name, &s)!=-1;
}

static void mystrlwr(char *p) 
{
	for (;*p;++p) *p = tolower(*p);
}

static void mystrupr(char *p) 
{
	for (;*p;++p) *p = toupper(*p);
}

BOOL GoToFile(const char *Target, BOOL AllowChangeDir)
{
  if(!Target || !*Target)
    return FALSE;
  BOOL rc=FALSE, search=TRUE;
  PanelRedrawInfo PRI;
  PanelInfo PInfo;
  char Name[NM], Dir[NM*5];
  int pathlen;

  strcpy(Name,FSF.PointToName(const_cast<char*>(Target)));
  pathlen=(int)(FSF.PointToName(const_cast<char*>(Target))-Target);
  if(pathlen)
    memcpy(Dir,Target,pathlen);
  Dir[pathlen]=0;

  FSF.Trim(Name);
  FSF.Trim(Dir);
  FSF.Unquote(Name);
  FSF.Unquote(Dir);

  Info.Control(INVALID_HANDLE_VALUE,FCTL_UPDATEPANEL,(void*)1);
  Info.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
  pathlen=strlen(Dir);
  if(pathlen)
  {
    if(*PInfo.CurDir &&
       PInfo.CurDir[strlen(PInfo.CurDir)-1]!='/' // old path != "*\"
       && Dir[pathlen-1]=='/')
      Dir[pathlen-1]=0;

    if(0!=strcmp(Dir,PInfo.CurDir))
    {
      if(AllowChangeDir)
      {
        Info.Control(INVALID_HANDLE_VALUE,FCTL_SETPANELDIR,&Dir);
        Info.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
      }
      else
        search=FALSE;
    }
  }

  PRI.CurrentItem=PInfo.CurrentItem;
  PRI.TopPanelItem=PInfo.TopPanelItem;
  if(search)
  {
    for(int J=0; J < PInfo.ItemsNumber; J++)
    {
      if(!strcmp(Name,
         FSF.PointToName(PInfo.PanelItems[J].FindData.cFileName)))
      {
        PRI.CurrentItem=J;
        PRI.TopPanelItem=J;
        rc=TRUE;
        break;
      }
    }
  }
  return rc?Info.Control(INVALID_HANDLE_VALUE,FCTL_REDRAWPANEL,&PRI):FALSE;
}

int __isspace(int Chr)
{
   return Chr == 0x09 || Chr == 0x0A || Chr == 0x0B || Chr == 0x0C || Chr == 0x0D || Chr == 0x20;
}


const char *GetMsg(int MsgId)
{
  return Info.GetMsg(Info.ModuleNumber,MsgId);
}


int rar_main(int argc, char *argv[]);
extern "C" int sevenz_main(int argc, char *argv[]);

#ifdef HAVE_LIBARCHIVE
extern "C" int libarch_main(int numargs, char *args[]);
#endif

SHAREDSYMBOL int BuiltinMain(int argc, char * argv[])
{
	if (!argc)
		return -1;

	int r = -2;
	if (strcmp(argv[0], "rar")==0) {
		r = rar_main(argc, &argv[0]);
	} else if (strcmp(argv[0], "7z")==0) {
		r = sevenz_main(argc, &argv[0]);
#ifdef HAVE_LIBARCHIVE
	} else if (strcmp(argv[0], "libarch")==0) {
		r = libarch_main(argc, &argv[0]);
#endif
	} else		
		fprintf(stderr, "BuiltinMain: bad target '%s'\n", argv[0]);	
	return r;
}



/* $ 13.09.2000 tran
   запуск треда для ожидания момента убийства лист файла */
   /*
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
*/
DWORD WINAPI ThreadWhatWaitingForKillListFile(LPVOID par)
{
    KillStruct *ks=(KillStruct*)par;

    WINPORT(WaitForSingleObject)(ks->hProcess,INFINITE);
    WINPORT(CloseHandle)(ks->hThread);
    WINPORT(CloseHandle)(ks->hProcess);
    sdc_unlink(ks->ListFileName);
    free((LPVOID)ks);
    return SUPER_PUPER_ZERO;
}
/* tran 13.09.2000 $ */

int Execute(HANDLE hPlugin, const std::string &CmdStr, int HideOutput, int Silent, int NeedSudo, int ShowCommand, char *ListFileName)
{
  if (!CmdStr.empty() && (CmdStr[0]==' ' || CmdStr[0]=='\t')) { //FSF.LTrim(ExpandedCmd); //$ AA 12.11.2001
    std::string CmdStrTrimmed = CmdStr;
    do {
      CmdStrTrimmed.erase(0, 1);
    } while (!CmdStrTrimmed.empty() && (CmdStrTrimmed[0]==' ' || CmdStrTrimmed[0]=='\t'));

    return Execute(hPlugin, CmdStrTrimmed, HideOutput, Silent, NeedSudo, ShowCommand, ListFileName);
  }
  
 // STARTUPINFO si;
  //PROCESS_INFORMATION pi;
  int ExitCode,LastError;

  //memset(&si,0,sizeof(si));
  //si.cb=sizeof(si);

  //HANDLE hChildStdoutRd,hChildStdoutWr;
  HANDLE StdInput=NULL;//GetStdHandle(STD_INPUT_HANDLE);
  HANDLE StdOutput=NULL;//GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE StdError=NULL;//GetStdHandle(STD_ERROR_HANDLE);
  HANDLE hScreen=NULL;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (HideOutput)
  {
    /*SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 32768))
    {
      SetStdHandle(STD_OUTPUT_HANDLE,hChildStdoutWr);
      SetStdHandle(STD_ERROR_HANDLE,hChildStdoutWr);*/

      if (!Silent)
      {
        hScreen=Info.SaveScreen(0,0,-1,-1);
        const char *MsgItems[]={"",GetMsg(MWaitForExternalProgram)};
        Info.Message(Info.ModuleNumber,0,NULL,MsgItems,
                      ARRAYSIZE(MsgItems),0);
      }
    /*}
    else
      HideOutput=FALSE;*/
  }
  else
  {
    WINPORT(GetConsoleScreenBufferInfo)(StdOutput,&csbi);

    char Blank[1024];
    FSF.sprintf(Blank,"%*s",csbi.dwSize.X,"");
    for (int Y=0;Y<csbi.dwSize.Y;Y++)
      Info.Text(0,Y,LIGHTGRAY,Blank);
    Info.Text(0,0,0,NULL);

    COORD C;
    C.X=0;
    C.Y=csbi.dwCursorPosition.Y;
    WINPORT(SetConsoleCursorPosition)(StdOutput,C);
  }


  DWORD ConsoleMode;
  WINPORT(GetConsoleMode)(StdInput,&ConsoleMode);
  WINPORT(SetConsoleMode)(StdInput,ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|
                 ENABLE_ECHO_INPUT|ENABLE_MOUSE_INPUT);

  WCHAR SaveTitle[512];
  WINPORT(GetConsoleTitle)(SaveTitle,sizeof(SaveTitle));
  if (ShowCommand)
    WINPORT(SetConsoleTitle)(StrMB2Wide(CmdStr).c_str());

  /* $ 14.02.2001 raVen
     делать окошку minimize, если в фоне */
/*  if (Opt.Background)
  {
    si.dwFlags=si.dwFlags | STARTF_USESHOWWINDOW;
    si.wShowWindow=SW_MINIMIZE;
  }*/
  /* raVen $ */
  DWORD flags = (HideOutput) ? EF_HIDEOUT : 0;
  if (NeedSudo)
    flags|= EF_SUDO;
  if (!ShowCommand)
    flags|= EF_NOCMDPRINT;

  if (*CmdStr.c_str()=='^') {
    LastError = ExitCode = FSF.ExecuteLibrary(gMultiArcPluginPath.c_str(), 
						"BuiltinMain", CmdStr.c_str() + 1, flags);
  } else {
    LastError = ExitCode = FSF.Execute(CmdStr.c_str(), flags);
  }
	
  /*if (HideOutput)
  {
    SetStdHandle(STD_OUTPUT_HANDLE,StdOutput);
    SetStdHandle(STD_ERROR_HANDLE,StdError);
    WINPORT(CloseHandle)(hChildStdoutWr);
  }*/

  WINPORT(SetLastError)(LastError);
  /*if(!ExitCode2 ) //$ 06.03.2002 AA
  {
    char Msg[100];
    char ExecuteName[NM];
    strncpy(ExecuteName,ExpandedCmd+(*ExpandedCmd=='"'), NM);
    char *Ptr;
    Ptr=strchr(ExecuteName,(*ExpandedCmd=='"')?'"':' ');
    if(Ptr)
      *Ptr=0;
    FindExecuteFile(ExecuteName,NULL,0);

    char NameMsg[NM];
    FSF.TruncPathStr(strcpyn(NameMsg,ExecuteName,sizeof(NameMsg)),MAX_WIDTH_MESSAGE);
    FSF.sprintf(Msg,GetMsg(MCannotFindArchivator),NameMsg);
    const char *MsgItems[]={GetMsg(MError),Msg, GetMsg(MOk)};
    Info.Message(Info.ModuleNumber,FMSG_WARNING|FMSG_ERRORTYPE,
                 NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    ExitCode=RETEXEC_ARCNOTFOUND;
  }*/

  WINPORT(SetConsoleTitle)(SaveTitle);
  WINPORT(SetConsoleMode)(StdInput,ConsoleMode);
/*  if (!HideOutput)
  {
    SMALL_RECT src;
    COORD dest;
    CHAR_INFO fill;
    src.Left=0;
    src.Top=2;
    src.Right=csbi.dwSize.X;
    src.Bottom=csbi.dwSize.Y;
    dest.X=dest.Y=0;
    fill.Char.AsciiChar=' ';
    fill.Attributes=7;
    WINPORT(ScrollConsoleScreenBuffer)(StdOutput,&src,NULL,dest,&fill);
    Info.Control(hPlugin,FCTL_SETUSERSCREEN,NULL);
  }*/
  if (hScreen)
  {
    Info.RestoreScreen(NULL);
    Info.RestoreScreen(hScreen);
  }

  return ExitCode;
}


char* QuoteText(char *Str)
{
  int LastPos=strlen(Str);
  memmove(Str+1,Str,LastPos+1);
  Str[LastPos+1]=*Str='"';
  Str[LastPos+2]=0;
  return Str;
}


void InitDialogItems(const struct InitDialogItem *Init,struct FarDialogItem *Item,
                    int ItemsNumber)
{
  int I;
  struct FarDialogItem *PItem=Item;
  const struct InitDialogItem *PInit=Init;
  for (I=0;I<ItemsNumber;I++,PItem++,PInit++)
  {
    PItem->Type=PInit->Type;
    PItem->X1=PInit->X1;
    PItem->Y1=PInit->Y1;
    PItem->X2=PInit->X2;
    PItem->Y2=PInit->Y2;
    PItem->Focus=PInit->Focus;
    PItem->History=(const char *)PInit->Selected;
    PItem->Flags=PInit->Flags;
    PItem->DefaultButton=PInit->DefaultButton;
    strcpy(PItem->Data,((DWORD_PTR)PInit->Data<2000)?GetMsg((unsigned int)(DWORD_PTR)PInit->Data):PInit->Data);
  }
}


static void __InsertCommas(char *Dest)
{
  int I;
  for (I=strlen(Dest)-4;I>=0;I-=3)
    if (Dest[I])
    {
      memmove(Dest+I+2,Dest+I+1,strlen(Dest+I));
      Dest[I+1]=',';
    }
}

void InsertCommas(unsigned long Number,char *Dest)
{
  __InsertCommas(FSF.itoa(Number,Dest,10));
}

void InsertCommas(int64_t Number,char *Dest)
{
  __InsertCommas(FSF.itoa64(Number,Dest,10));
}


int MA_ToPercent(int32_t N1, int32_t N2)
{
  if (N1 > 10000)
  {
    N1/=100;
    N2/=100;
  }
  if (N2==0)
    return 0;
  if (N2<N1)
    return 100;
  return (int)(N1*100/N2);
}

int MA_ToPercent(int64_t N1,int64_t N2)
{
  if (N1 > 10000)
  {
    N1/=100;
    N2/=100;
  }
  if (N2==0)
    return 0;
  if (N2<N1)
    return 100;
  return (int)(N1*100/N2);
}

int IsCaseMixed(const char *Str)
{
  while (*Str && !isalpha(*Str))
    Str++;
  int Case=islower(*Str);
  while (*(Str++))
    if (isalpha(*Str) && islower(*Str)!=Case)
      return TRUE;
  return FALSE;
}


int CheckForEsc()
{
  int ExitCode=FALSE;
  while (1)
  {
    INPUT_RECORD rec;
    /*static*/ HANDLE hConInp=NULL;//GetStdHandle(STD_INPUT_HANDLE);
    DWORD ReadCount;
    WINPORT(PeekConsoleInput)(hConInp,&rec,1,&ReadCount);
    if (ReadCount==0)
      break;
    WINPORT(ReadConsoleInput)(hConInp,&rec,1,&ReadCount);
    if (rec.EventType==KEY_EVENT)
      if (rec.Event.KeyEvent.wVirtualKeyCode==VK_ESCAPE &&
          rec.Event.KeyEvent.bKeyDown)
        ExitCode=TRUE;
  }
  return ExitCode;
}


char *GetCommaWord(char *Src,char *Word,char Separator)
{
  int WordPos,SkipBrackets;
  if (*Src==0)
    return NULL;
  SkipBrackets=FALSE;
  for (WordPos=0;*Src!=0;Src++,WordPos++)
  {
    if (*Src=='[' && strchr(Src+1,']')!=NULL)
      SkipBrackets=TRUE;
    if (*Src==']')
      SkipBrackets=FALSE;
    if (*Src==Separator && !SkipBrackets)
    {
      Word[WordPos]=0;
      Src++;
      while (__isspace(*Src))
        Src++;
      return Src;
    }
    else
      Word[WordPos]=*Src;
  }
  Word[WordPos]=0;
  return Src;
}

int FindExecuteFile(char *OriginalName,char *DestName,int SizeDest)
{
	std::string cmd = "which ";
	cmd+= OriginalName;
	FILE *f = popen(cmd.c_str(), "r");
	if (f==NULL) {
		perror("FindExecuteFile - popen");
		return FALSE;
	}
	if (!fgets(DestName, SizeDest - 1, f))
		DestName[0] = 0;
	else
		DestName[SizeDest - 1] = 0;
	pclose(f);
	
	char *e = strchr(DestName, '\n');
	if (e) *e = 0;
	e = strchr(DestName, '\r');
	if (e) *e = 0;
	return DestName[0] ? TRUE : FALSE;
}


char *SeekDefExtPoint(char *Name, char *DefExt/*=NULL*/, char **Ext/*=NULL*/)
{
  FSF.Unquote(Name); //$ AA 15.04.2003 для правильной обработки имен в кавычках
  Name=FSF.PointToName(Name);
  char *TempExt=strrchr(Name, '.');
  if(!DefExt)
    return TempExt;
  if(Ext)
    *Ext=TempExt;
  return (TempExt!=NULL)?(strcasecmp(TempExt+1, DefExt)?NULL:TempExt):NULL;
}

BOOL AddExt(char *Name, char *Ext)
{
  char *ExtPnt;
  FSF.Unquote(Name); //$ AA 15.04.2003 для правильной обработки имен в кавычках
  if(Name && *Name && !SeekDefExtPoint(Name, Ext, &ExtPnt))
  {
    // transform Ext
    char NewExt[NM], *Ptr;
    strncpy(NewExt,Ext,sizeof(NewExt));

    int Up=0, Lw=0;
    Ptr=Name;
    while(*Ptr)
    {
      if(isalpha(*Ptr))
      {
        if(islower(*Ptr)) Lw++;
        if(isupper(*Ptr)) Up++;
      }
      ++Ptr;
    }

    if (Lw)
      mystrlwr(NewExt);
    else  if (Up)
      mystrupr(NewExt);

    if(ExtPnt && !*(ExtPnt+1))
      strcpy(ExtPnt+1, NewExt);
    else
      FSF.sprintf(Name+strlen(Name), ".%s", NewExt);
    return TRUE;
  }
  return FALSE;
}


#ifdef _NEW_ARC_SORT_
void WritePrivateProfileInt(char *Section, char *Key, int Value, char *Ini)
{
  char Buf32[32];
  wsprintf(Buf32, "%d", Value);
  WritePrivateProfileString(Section, Key, Buf32, Ini);
}
#endif

int WINAPI GetPassword(char *Password,const char *FileName)
{
  char Prompt[2*NM],InPass[512];
  FSF.sprintf(Prompt,GetMsg(MGetPasswordForFile),FileName);
  if(Info.InputBox((const char*)GetMsg(MGetPasswordTitle),
                  (const char*)Prompt,NULL,NULL,
                  InPass,sizeof(InPass),NULL,FIB_PASSWORD|FIB_ENABLEEMPTY))
  {
    strcpy(Password,InPass);
    return TRUE;
  }
  return FALSE;
}


// Number of 100 nanosecond units from 01.01.1601 to 01.01.1970
#define EPOCH_BIAS    I64(116444736000000000)

void WINAPI UnixTimeToFileTime( DWORD time, FILETIME * ft )
{
  *(int64_t*)ft = EPOCH_BIAS + time * I64(10000000);
}

int GetScrX(void)
{
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
  if(WINPORT(GetConsoleScreenBufferInfo)(NULL,&ConsoleScreenBufferInfo))//GetStdHandle(STD_OUTPUT_HANDLE)
    return ConsoleScreenBufferInfo.dwSize.X;
  return 0;
}



int PathMayBeAbsolute(const char *Path)
{
  return (Path && *Path=='/');
}

/*
  преобразует строку
    "cdrecord-1.6.1/mkisofs-1.12b4/../cdrecord/cd_misc.c"
  в
    "cdrecord-1.6.1/cdrecord/cd_misc.c"
*/
void NormalizePath(const char *lpcSrcName,char *lpDestName)
{
  char *DestName=lpDestName;
  char *Ptr;
  char *SrcName=strdup(lpcSrcName);
  char *oSrcName=SrcName;
  int dist;

  while(*SrcName)
  {
    Ptr=strchr(SrcName,'/');

    if(!Ptr)
      Ptr=SrcName+strlen(SrcName);

    dist=(int)(Ptr-SrcName)+1;

    if(dist == 1 && (*SrcName == '/'))
    {
      *DestName=*SrcName;
      DestName++;
      SrcName++;
    }
    else if(dist == 2 && *SrcName == '.')
    {
      SrcName++;

      if(*SrcName == 0)
        DestName--;
      else
        SrcName++;
    }
    else if(dist == 3 && *SrcName == '.' && SrcName[1] == '.')
    {
      if(!PathMayBeAbsolute(lpDestName))
      {
        char *ptrCurDestName=lpDestName, *Temp=NULL;

        for ( ; ptrCurDestName < DestName-1; ptrCurDestName++)
        {
           if (*ptrCurDestName == '/')
             Temp = ptrCurDestName;
        }

        if(!Temp)
          Temp=lpDestName;

        DestName=Temp;
      }
      else
      {
         if(SrcName[2] == '/')
           SrcName++;
      }

      SrcName+=2;
    }
    else
    {
      strncpy(DestName, SrcName, dist);
      dist--;
      DestName += dist;
      SrcName  += dist;
    }

    *DestName=0;
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
  if (str.empty())
    return str;

  std::vector<WCHAR> ExpandedCmd(str.size() + 0x1000);
  for (;;ExpandedCmd.resize(ExpandedCmd.size() + 0x1000))
  {
    if (WINPORT(ExpandEnvironmentStrings)(StrMB2Wide(str).c_str(), &ExpandedCmd[0], ExpandedCmd.size()) < ExpandedCmd.size())
	  break;
  }
  Wide2MB(&ExpandedCmd[0], str);
  return str;
}
