#include <windows.h>
#include "MultiArc.hpp"
#include "marclng.hpp"
#include <farkeys.hpp>
#include <fcntl.h>

ArcCommand::ArcCommand(struct PluginPanelItem *PanelItem,int ItemsNumber,
                       const char *FormatString,const char *ArcName,const char *ArcDir,
                       const char *Password,const char *AllFilesMask,int IgnoreErrors,
                       int CommandType,int ASilent,const char *RealArcDir,int DefaultCodepage)
{
  NeedSudo = false;
  Silent=ASilent;
//  CommentFile=INVALID_HANDLE_VALUE; //$ AA 25.11.2001
  *CommentFileName=0; //$ AA 25.11.2001
//  ExecCode=-1;
/* $ 28.11.2000 AS
*/
  ExecCode=(DWORD)-1;

  ArcCommand::DefaultCodepage = DefaultCodepage;

//  fprintf(stderr, "ArcCommand::ArcCommand = %d, FormatString=%s\n", ArcCommand::DefaultCodepage, FormatString);
  if (*FormatString==0)
    return;

  bool arc_modify = (CommandType!=CMD_EXTRACT && CommandType!=CMD_EXTRACTWITHOUTPATH && CommandType!=CMD_TEST);

  if (ArcDir && *ArcDir && sudo_client_is_required_for(ArcDir, false)==1) {
      NeedSudo = true;
  } else if (RealArcDir && *RealArcDir && sudo_client_is_required_for(RealArcDir, false)==1) {
      NeedSudo = true;
  } else if (ArcName && *ArcName && sudo_client_is_required_for(ArcName, arc_modify)==1) {
      NeedSudo = true;
  } else if (CommandType==CMD_EXTRACT || CommandType==CMD_EXTRACTWITHOUTPATH || CommandType==CMD_TEST) {
	  if (sudo_client_is_required_for(".", true) == 1)
		  NeedSudo = true;
  } else if (sudo_client_is_required_for(".", false) == 1) {
		  NeedSudo = true;
  }

  //char QPassword[NM+5],QTempPath[NM+5];
  ArcCommand::PanelItem = PanelItem;
  ArcCommand::ItemsNumber = ItemsNumber;
  ArcCommand::ArcName = ArcName;
  ArcCommand::ArcDir = ArcDir;
  ArcCommand::RealArcDir = RealArcDir ? RealArcDir : "";
  ArcCommand::Password = Password; QuoteCmdArgIfNeed(ArcCommand::Password);
  ArcCommand::AllFilesMask = AllFilesMask;
  //WINPORT(GetTempPath)(ARRAYSIZE(TempPath),TempPath);
  ArcCommand::TempPath = InMyTemp();
  *PrefixFileName=0;
  *ListFileName=0;
  NameNumber=-1;
  NextFileName.clear();
  do
  {
    PrevFileNameNumber=-1;
    if (!ProcessCommand(FormatString, CommandType, IgnoreErrors, ListFileName))
      NameNumber=-1;
    if (*ListFileName)
    {
      if ( !Opt.Background )
        sdc_remove(ListFileName);
      *ListFileName=0;
    }
  } while (NameNumber!=-1 && NameNumber<ItemsNumber);
}


bool ArcCommand::ProcessCommand(std::string FormatString, int CommandType, int IgnoreErrors,
                               char *pcListFileName)
{
  MaxAllowedExitCode=0;
  DeleteBraces(FormatString);

//  for (char *CurPtr=Command; *CurPtr;)
  std::string tmp, NonVar, Command;
  while (!FormatString.empty())
  {
    tmp = FormatString;
    int r = ReplaceVar(tmp);
//    fprintf(stderr, "ReplaceVar: %d  '%s' -> '%s'\n", r, FormatString.c_str(), tmp.c_str());
    if (r < 0)
      return false;
    if (r == 0) {
      r = 1;
      NonVar+= FormatString[0];
    } else {
      Command+= ExpandEnv(NonVar);
      Command+= tmp;
      NonVar.clear();
    }

    FormatString.erase(0, r);
  }
  Command+= ExpandEnv(NonVar);
//  fprintf(stderr, "Command='%s'\n", Command.c_str());

  if (Command.empty())
  {
    if(!Silent)
    {
      const char *MsgItems[]={GetMsg(MError),GetMsg(MArcCommandNotFound),GetMsg(MOk)};
      Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    }
    return false;
  }

  int Hide = Opt.HideOutput;
  if ((Hide == 1 && CommandType == 0) || CommandType == 2)
    Hide = 0;

  ExecCode = Execute(this, Command, Hide, Silent, NeedSudo, Password.empty(), ListFileName);
  fprintf(stderr, "ArcCommand::ProcessCommand: ExecCode=%d for '%s'\n", ExecCode, Command.c_str());

// Unzip in MacOS definatelt doesn't have -I and -O options, so dont even try encoding workarounds
#ifndef __WXOSX__
  if (ExecCode == 11 && strncmp(Command.c_str(), "unzip ", 6) == 0) {
    // trying as utf8
    std::string CommandRetry = Command;
    CommandRetry.insert(6, "-I utf8 -O utf8 ");
    ExecCode = Execute(this, CommandRetry, Hide, Silent, NeedSudo, Password.empty(), ListFileName);
    if (ExecCode == 11) {
      // "11" means file was not found in archive. retrying as oem
      CommandRetry = Command;
      unsigned int retry_cp = (DefaultCodepage > 0) ? DefaultCodepage : WINPORT(GetOEMCP)();
      CommandRetry.insert(6, StrPrintf("-I CP%u -O CP%u ", retry_cp, retry_cp));
      ExecCode = Execute(this, CommandRetry, Hide, Silent, NeedSudo, Password.empty(), ListFileName);
    }
    if (ExecCode == 1) {
      // "1" exit code for unzip is warning only, no need to bother user
      ExecCode = 0;
    }
  } else if (ExecCode == 12 && strncmp(Command.c_str(), "zip -d", 6) == 0) {

     for(size_t i_entries = 6; i_entries + 1 < Command.size(); ++i_entries) {
       if (Command[i_entries] == ' ' && Command[i_entries + 1] != ' ' && Command[i_entries + 1] != '-' ) {
         i_entries = Command.find(' ', i_entries + 1);
         if (i_entries != std::string::npos) {
           std::wstring wstr = StrMB2Wide(Command.substr(i_entries));
           std::vector<char> oemstr(wstr.size() * 6 + 2);
           char def_char = '\x01';
           BOOL def_char_used = FALSE;
           unsigned int retry_cp = (DefaultCodepage > 0) ? DefaultCodepage : WINPORT(GetOEMCP)();
           WINPORT(WideCharToMultiByte)(retry_cp, 0, wstr.c_str(),
               wstr.size() + 1, &oemstr[0], oemstr.size() - 1, &def_char, &def_char_used);
           if (!def_char_used) {
             std::string CommandRetry = Command.substr(0, i_entries);
             CommandRetry.append(&oemstr[0]);
             ExecCode = Execute(this, CommandRetry.c_str(), Hide, Silent, NeedSudo, Password.empty(), ListFileName);
             fprintf(stderr, "ArcCommand::ProcessCommand: retry ExecCode=%d for '%s'\n", ExecCode, CommandRetry.c_str());
           } else {
             fprintf(stderr, "ArcCommand::ProcessCommand: can't translate to CP%u: '%s'\n", retry_cp, Command.c_str());
           }
         }
         break;
       }
     }
  }
#endif
  if (ExecCode==RETEXEC_ARCNOTFOUND)
    return false;

  if (ExecCode <= MaxAllowedExitCode)
    ExecCode = 0;

  if (!IgnoreErrors && ExecCode!=0)
  {
    if(!Silent)
    {
      char ErrMsg[200];
      char NameMsg[NM];
      FSF.sprintf(ErrMsg,(char *)GetMsg(MArcNonZero),ExecCode);
      const char *MsgItems[]={GetMsg(MError),NameMsg,ErrMsg,GetMsg(MOk)};

      FSF.TruncPathStr(strncpy(NameMsg,ArcName.c_str(),sizeof(NameMsg)),MAX_WIDTH_MESSAGE);
      Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    }
    return false;
  }

  return true;
}


void ArcCommand::DeleteBraces(std::string &Command)
{
  std::string CheckStr;
  for (size_t left = std::string::npos;;)
  {
    size_t right = Command.rfind('}', left);
    if (right == std::string::npos || right == 0)
      return;

    left = Command.rfind('{', right - 1);
    if (left == std::string::npos)
      return;

    bool NonEmptyVar = false;
    for (size_t i = left + 1; i + 2 < right; ++i)
    {
      if (Command[i] == '%' && Command[i + 1] == '%'  && strchr("FfLl", Command[i + 2]) != NULL)
      {
        NonEmptyVar = (ItemsNumber > 0);
        break;
      }
      CheckStr.assign(Command.c_str() + i, std::min((size_t)4, right - i));
      if (ReplaceVar(CheckStr) && CheckStr.size() > 0)
      {
        NonEmptyVar = true;
        break;
      }
    }

    if (NonEmptyVar)
    {
      Command[left] = ' ';
      Command[right] = ' ';
    }
    else
    {
      Command.erase(left, right - left + 1);
    }
  }
}


static void CutToPathOrSpace(std::string &Path)
{
  size_t slash = Path.rfind(GOOD_SLASH);
  if (slash != std::string::npos)
    Path.resize(slash);
  else
    Path = ' ';
}

int ArcCommand::ReplaceVar(std::string &Command)
{
  int MaxNamesLength = 0x10000;
  std::string LocalAllFilesMask = AllFilesMask;
  bool UseSlash = false;
  bool FolderMask = false;
  bool FolderName = false;
  bool NameOnly = false;
  bool PathOnly = false;
  int QuoteName = 0;

  if (Command.size() < 3)
    return 0;

  char Chr = Command[2] & (~0x20);
  if (Command[0] != '%' || Command[1] != '%' || Chr < 'A' || Chr > 'Z')
    return 0;

  int VarLength = 3;

  while (VarLength < Command.size())
  {
    bool BreakScan = false;
    Chr = Command[VarLength];
    if (Command[2]=='F' && Chr >= '0' && Chr <= '9')
    {
      MaxNamesLength = FSF.atoi(Command.c_str() + VarLength);
      while (Chr >= '0' && Chr <= '9' && VarLength < Command.size())
        Chr = Command[++VarLength];
      continue;
    }
    if (Command[2]=='E' && Chr >= '0' && Chr <= '9')
    {
      MaxAllowedExitCode = FSF.atoi(Command.c_str() + VarLength);
      while (Chr >= '0' && Chr <= '9' && VarLength < Command.size())
        Chr=Command[++VarLength];
      continue;
    }
    switch (Command[VarLength])
    {
      case 'A': break; /* deprecated AnsiCode = true; */
      case 'Q': QuoteName = 1; break;
      case 'q': QuoteName = 2; break;
      case 'S': UseSlash = true; break;
      case 'M': FolderMask = true; break;
      case 'N': FolderName = true; break;
      case 'W': NameOnly = true; break;
      case 'P': PathOnly = true; break;
      case '*': LocalAllFilesMask = "*"; break;
      default: BreakScan = true;
    }
    if (BreakScan)
      break;
    VarLength++;
  }

  if ( (MaxNamesLength-=(int)Command.size()) <= 0)
    MaxNamesLength = 1;

  if (!FolderMask && !FolderName)
    FolderName = true;

/////////////////////////////////
  switch (Command[2])
  {
    case 'T': /* charset, if known */
      Command.clear();
      for (int N = 0; N < ItemsNumber; ++N)
      {
        if(PanelItem[N].UserData && (PanelItem[N].Flags & PPIF_USERDATA))
        {
          struct ArcItemUserData *aud=(struct ArcItemUserData*)PanelItem[N].UserData;
          if(aud->SizeStruct == sizeof(struct ArcItemUserData) && aud->Codepage > 0)
          {
            Command = StrPrintf("CP%u", aud->Codepage);
            break;
          }
        }
      }
      if (Command.empty() && DefaultCodepage > 0)
            Command = StrPrintf("CP%u", DefaultCodepage);

      break;

    case 'A': case 'a': /* deprecated: short name - works same as normal name */
      Command = ArcName;
      if (PathOnly)
        CutToPathOrSpace(Command);
      QuoteCmdArgIfNeed(Command);
      break;

    case 'D': case 'E':
      Command.clear();
      break;

    case 'L': case 'l':
      if (!MakeListFile(ListFileName, QuoteName, UseSlash, FolderName,
         NameOnly, PathOnly, FolderMask, LocalAllFilesMask.c_str())) {
        return -1;
      }
      Command = ListFileName;
      QuoteCmdArgIfNeed(Command);
      break;

    case 'P':
      Command = Password;
      break;

    case 'C':
      if (*CommentFileName) //второй раз сюда не лезем
        break;
      {
        Command.clear();
        int CommentFile;
        if (FSF.MkTemp(CommentFileName, "FAR") && (CommentFile =
          sdc_open(CommentFileName, O_CREAT | O_TRUNC | O_RDWR, 0660)) != -1)
        {
          char Buf[512];
          if(Info.InputBox(GetMsg(MComment), GetMsg(MInputComment), NULL, "", Buf, sizeof(Buf), NULL, 0))
          //??тут можно и заполнить строку комментарием, но надо знать, файловый
          //?? он или архивный. да и имя файла в архиве тоже надо знать...
          {
            sdc_write(CommentFile, Buf, strlen(Buf));
            sdc_close(CommentFile);
            Command = CommentFileName;
          }
          WINPORT(FlushConsoleInputBuffer)(NULL);//GetStdHandle(STD_INPUT_HANDLE));
        }
      }
      break;

    case 'r':
      Command = RealArcDir;
      if (!Command.empty())
        Command+= '/';
      break;
    case 'R':
      Command = RealArcDir;
      if (UseSlash)
      {
      }
      QuoteCmdArgIfNeed(Command);
      break;

    case 'W':
      Command = TempPath;
      break;

    case 'F': case 'f':
      if (PanelItem!=NULL)
      {
        std::string CurArcDir = ArcDir;
        if (!CurArcDir.empty() && CurArcDir[CurArcDir.size() - 1] != GOOD_SLASH)
          CurArcDir+= GOOD_SLASH;

        std::string Names, Name;

        if (NameNumber == -1)
          NameNumber = 0;

        while (NameNumber < ItemsNumber || Command[2] == 'f')
        {
          int IncreaseNumber = 0;
          DWORD FileAttr;
          if (!NextFileName.empty())
          {
            Name = PrefixFileName;
            Name+= CurArcDir;
            Name+= NextFileName;
            NextFileName.clear();
            FileAttr = 0;
          }
          else
          {
            int N;
            if (Command[2]=='f' && PrevFileNameNumber!=-1)
              N = PrevFileNameNumber;
            else
            {
              N = NameNumber;
              IncreaseNumber=1;
            }
            if (N >= ItemsNumber)
              break;

            *PrefixFileName=0;
            char *cFileName = PanelItem[N].FindData.cFileName;

            if(PanelItem[N].UserData && (PanelItem[N].Flags & PPIF_USERDATA))
            {
              struct ArcItemUserData *aud=(struct ArcItemUserData*)PanelItem[N].UserData;
              if(aud->SizeStruct == sizeof(struct ArcItemUserData))
              {
                if(aud->Prefix)
                  strncpy(PrefixFileName,aud->Prefix,sizeof(PrefixFileName));
                if(aud->LinkName)
                  cFileName=aud->LinkName;
              }
            }
            // CHECK for BUGS!!
            Name = PrefixFileName;
            if(*cFileName != GOOD_SLASH)
            {
              Name+= CurArcDir;
              Name+= cFileName;
            }
            else
              Name+= cFileName+1;
            NormalizePath(Name);
            FileAttr = PanelItem[N].FindData.dwFileAttributes;
            PrevFileNameNumber = N;
          }
          if (NameOnly)
          {
            size_t slash = Name.rfind(GOOD_SLASH);
            if (slash != std::string::npos)
              Name.erase(0, slash + 1);
          }
          if (PathOnly)
            CutToPathOrSpace(Name);
          if (Names.empty() || (Names.size() + Name.size() < MaxNamesLength && Command[2] != 'f'))
          {
            NameNumber+= IncreaseNumber;
            if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
              std::string FolderMaskName = Name;
              if (!PathOnly)
              {
                FolderMaskName+= GOOD_SLASH;
                FolderMaskName+= LocalAllFilesMask;
              }
              else
                CutToPathOrSpace(FolderMaskName);
              if (FolderMask)
              {
                if (FolderName)
                  NextFileName.swap(FolderMaskName);
                else
                  Name.swap(FolderMaskName);
              }
            }

            if (QuoteName==1)
              QuoteCmdArgIfNeed(Name);
            else if (QuoteName==2)
              QuoteCmdArg(Name);

            if (!Names.empty())
              Names+= ' ';
            Names+= Name;
          }
          else
            break;
        }
        Command.swap(Names);
      }
      else
        Command.clear();
      break;
    default:
      return 0;
  }

  return VarLength;
}


int ArcCommand::MakeListFile(char *ListFileName, int QuoteName,
                int UseSlash, int FolderName, int NameOnly, int PathOnly,
                int FolderMask, const char *LocalAllFilesMask)
{
//  FILE *ListFile;
  HANDLE ListFile;
  DWORD WriteSize;
  /*SECURITY_ATTRIBUTES sa;

  sa.nLength=sizeof(sa);
  sa.lpSecurityDescriptor=NULL;
  sa.bInheritHandle=TRUE; //WTF??? 
  */

  if (FSF.MkTemp(ListFileName, "FAR")==NULL ||
     (ListFile=WINPORT(CreateFile)(MB2Wide(ListFileName).c_str(), GENERIC_WRITE,
                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                   NULL, CREATE_ALWAYS, //&sa
                   FILE_FLAG_SEQUENTIAL_SCAN,NULL)) == INVALID_HANDLE_VALUE)

  {
    if(!Silent)
    {
      char NameMsg[NM];
      const char *MsgItems[]={GetMsg(MError),GetMsg(MCannotCreateListFile),NameMsg,GetMsg(MOk)};
      FSF.TruncPathStr(strncpy(NameMsg,ListFileName,sizeof(NameMsg)),MAX_WIDTH_MESSAGE);
      Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    }
/* $ 25.07.2001 AA
    if(ListFile != INVALID_HANDLE_VALUE)
      WINPORT(CloseHandle)(ListFile);
25.07.2001 AA $*/
    return FALSE;
  }

  std::string CurArcDir, FileName, OutName;
  char Buf[3*NM];

  if (!NameOnly)
    CurArcDir = ArcDir;

  if (!CurArcDir.empty() && CurArcDir[CurArcDir.size() - 1] != GOOD_SLASH)
    CurArcDir+= GOOD_SLASH;

//  if (UseSlash);

  for (int I=0;I<ItemsNumber;I++)
  {
    if (NameOnly)
      FileName = FSF.PointToName(PanelItem[I].FindData.cFileName);
    else if (PathOnly)
      FileName.assign(PanelItem[I].FindData.cFileName, FSF.PointToName(PanelItem[I].FindData.cFileName) - PanelItem[I].FindData.cFileName);
    else
      FileName = PanelItem[I].FindData.cFileName;

    int FileAttr=PanelItem[I].FindData.dwFileAttributes;

    *PrefixFileName=0;
    if(PanelItem[I].UserData && (PanelItem[I].Flags & PPIF_USERDATA))
    {
      struct ArcItemUserData *aud=(struct ArcItemUserData*)PanelItem[I].UserData;
      if(aud->SizeStruct == sizeof(struct ArcItemUserData))
      {
        if(aud->Prefix)
          strncpy(PrefixFileName,aud->Prefix,sizeof(PrefixFileName));
        if(aud->LinkName)
           FileName = aud->LinkName;
      }
    }

    int Error=FALSE;
    if (((FileAttr & FILE_ATTRIBUTE_DIRECTORY)==0 || FolderName))
    {
      // CHECK for BUGS!!
      OutName = PrefixFileName;
      if (*FileName.c_str() != '/')
      {
        OutName+= CurArcDir;
        OutName+= FileName;
      }
      else
        OutName+= FileName.c_str() + 1;

      NormalizePath(OutName);

      if (QuoteName==1)
        QuoteCmdArgIfNeed(OutName);
      else if (QuoteName==2)
        QuoteCmdArg(OutName);

      OutName+= NATIVE_EOL;

      Error=WINPORT(WriteFile)(ListFile, OutName.c_str(), OutName.size(), &WriteSize, NULL) == FALSE;
      //Error=fwrite(Buf,1,strlen(Buf),ListFile) != strlen(Buf);
    }
    if (!Error && (FileAttr & FILE_ATTRIBUTE_DIRECTORY) && FolderMask)
    {
      OutName = PrefixFileName;
      OutName+= CurArcDir;
      OutName+= FileName;
      OutName+= '/';
      OutName+= LocalAllFilesMask;
      if (QuoteName==1)
        QuoteCmdArgIfNeed(OutName);
      else if (QuoteName==2)
        QuoteCmdArg(OutName);
      OutName+= NATIVE_EOL;
      Error=WINPORT(WriteFile)(ListFile, OutName.c_str(), OutName.size(), &WriteSize, NULL) == FALSE;
      //Error=fwrite(Buf,1,strlen(Buf),ListFile) != strlen(Buf);
    }
    if (Error)
    {
      WINPORT(CloseHandle)(ListFile);
      sdc_remove(ListFileName);
      if(!Silent)
      {
        const char *MsgItems[]={GetMsg(MError),GetMsg(MCannotCreateListFile),GetMsg(MOk)};
        Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
      }
      return FALSE;
    }
  }

  WINPORT(CloseHandle)(ListFile);
/*
  if (!WINPORT(CloseHandle)(ListFile))
  {
    // clearerr(ListFile);
    WINPORT(CloseHandle)(ListFile);
    DeleteFile(ListFileName);
    if(!Silent)
    {
      char *MsgItems[]={GetMsg(MError),GetMsg(MCannotCreateListFile),GetMsg(MOk)};
      Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    }
    return FALSE;
  }
*/
  return TRUE;
}

ArcCommand::~ArcCommand() //$ AA 25.11.2001
{
/*  if(CommentFile!=INVALID_HANDLE_VALUE)
    WINPORT(CloseHandle)(CommentFile);*/
    sdc_remove(CommentFileName);
}

