#include <windows.h>
#include "MultiArc.hpp"
#include "marclng.hpp"
#include <farkeys.hpp>

ArcCommand::ArcCommand(struct PluginPanelItem *PanelItem,int ItemsNumber,
                       const char *FormatString,const char *ArcName,const char *ArcDir,
                       const char *Password,const char *AllFilesMask,int IgnoreErrors,
                       int CommandType,int ASilent,const char *RealArcDir)
{

  Silent=ASilent;
//  CommentFile=INVALID_HANDLE_VALUE; //$ AA 25.11.2001
  *CommentFileName=0; //$ AA 25.11.2001
//  ExecCode=-1;
/* $ 28.11.2000 AS
*/
  ExecCode=(DWORD)-1;

  if (*FormatString==0)
    return;
  //char QPassword[NM+5],QTempPath[NM+5];
  char Command[MAX_COMMAND_LENGTH];
  ArcCommand::PanelItem=PanelItem;
  ArcCommand::ItemsNumber=ItemsNumber;
  strcpy(ArcCommand::ArcName,ArcName);
  strcpy(ArcCommand::ArcDir,ArcDir);
  strcpy(ArcCommand::RealArcDir,RealArcDir ? RealArcDir:"");
  FSF.QuoteSpaceOnly(strcpy(ArcCommand::Password,Password));
  strcpy(ArcCommand::AllFilesMask,AllFilesMask);
  //WINPORT(GetTempPath)(ARRAYSIZE(TempPath),TempPath);
  strcpy(TempPath, "/tmp");//todo
  *PrefixFileName=0;
  *ListFileName=0;
  NameNumber=-1;
  *NextFileName=0;
  do
  {
    PrevFileNameNumber=-1;
    strcpy(Command,FormatString);
    if (!ProcessCommand(Command,CommandType,IgnoreErrors,ListFileName))
      NameNumber=-1;
    if (*ListFileName)
    {
      if ( !Opt.Background )
        remove(ListFileName);
      *ListFileName=0;
    }
  } while (NameNumber!=-1 && NameNumber<ItemsNumber);
}


int ArcCommand::ProcessCommand(char *Command,int CommandType,int IgnoreErrors,
                               char *ListFileName)
{
  MaxAllowedExitCode=0;
  DeleteBraces(Command);

  for (char *CurPtr=Command;*CurPtr;)
  {
    int Length=strlen(Command);
    switch(ReplaceVar(CurPtr,Length))
    {
      case 1:
        CurPtr+=Length;
        break;
      case -1:
        return FALSE;
      default:
        CurPtr++;
        break;
    }
  }

  if (*Command)
  {
    int Hide=Opt.HideOutput;
    if ((Hide==1 && CommandType==0) || CommandType==2)
      Hide=0;
    ExecCode=Execute(this,Command,Hide,Silent,!*Password,ListFileName);
    if(ExecCode==RETEXEC_ARCNOTFOUND)
    {
      return FALSE;
    }
    if (ExecCode<=MaxAllowedExitCode)
      ExecCode=0;
    if (!IgnoreErrors && ExecCode!=0)
    {
      if(!Silent)
      {
        char ErrMsg[200];
        char NameMsg[NM];
        FSF.sprintf(ErrMsg,(char *)GetMsg(MArcNonZero),ExecCode);
        const char *MsgItems[]={GetMsg(MError),NameMsg,ErrMsg,GetMsg(MOk)};
        FSF.TruncPathStr(strncpy(NameMsg,ArcName,sizeof(NameMsg)),MAX_WIDTH_MESSAGE);
        Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
      }
      return FALSE;
    }
  }
  else
  {
    if(!Silent)
    {
      const char *MsgItems[]={GetMsg(MError),GetMsg(MArcCommandNotFound),GetMsg(MOk)};
      Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    }
    return FALSE;
  }
  return TRUE;
}


void ArcCommand::DeleteBraces(char *Command)
{
  char CheckStr[512],*CurPtr,*EndPtr;
  int NonEmptyVar;
  while (1)
  {
    if ((Command=strchr(Command,'{'))==NULL)
      return;
    if ((EndPtr=strchr(Command+1,'}'))==NULL)
      return;
    for (NonEmptyVar=0,CurPtr=Command+1;CurPtr<EndPtr-2;CurPtr++)
    {
      int Length;
      strncpy(CheckStr,CurPtr,4);
      if (CheckStr[0]=='%' && CheckStr[1]=='%' && strchr("FfLl",CheckStr[2])!=NULL)
      {
        NonEmptyVar=(ItemsNumber>0);
        break;
      }
      Length=0;
      if (ReplaceVar(CheckStr,Length))
        if (Length>0)
        {
          NonEmptyVar=1;
          break;
        }
    }

    if (NonEmptyVar)
    {
      *Command=*EndPtr=' ';
      Command=EndPtr+1;
    }
    else
    {
      char TmpStr[MAX_COMMAND_LENGTH];
      strcpy(TmpStr,EndPtr+1);
      strcpy(Command,TmpStr);
    }
  }
}


int ArcCommand::ReplaceVar(char *Command,int &Length)
{
  char Chr=Command[2]&(~0x20);
  if (Command[0]!='%' || Command[1]!='%' || Chr < 'A' || Chr > 'Z')
    return FALSE;
  char SaveStr[MAX_COMMAND_LENGTH],LocalAllFilesMask[NM];
  int QuoteName=0,UseSlash=FALSE,FolderMask=FALSE,FolderName=FALSE;
  int NameOnly=FALSE,PathOnly=FALSE,AnsiCode=FALSE;
  int MaxNamesLength=127;

  int VarLength=3;

  strcpy(LocalAllFilesMask,AllFilesMask);

  while (1)
  {
    int BreakScan=FALSE;
    Chr=Command[VarLength];
    if (Command[2]=='F' && Chr >= '0' && Chr <= '9')
    {
      MaxNamesLength=FSF.atoi(&Command[VarLength]);
      while (Chr >= '0' && Chr <= '9')
        Chr=Command[++VarLength];
      continue;
    }
    if (Command[2]=='E' && Chr >= '0' && Chr <= '9')
    {
      MaxAllowedExitCode=FSF.atoi(&Command[VarLength]);
      while (Chr >= '0' && Chr <= '9')
        Chr=Command[++VarLength];
      continue;
    }
    switch(Command[VarLength])
    {
      case 'A':
        AnsiCode=TRUE;
        break;
      case 'Q':
        QuoteName=1;
        break;
      case 'q':
        QuoteName=2;
        break;
      case 'S':
        UseSlash=TRUE;
        break;
      case 'M':
        FolderMask=TRUE;
        break;
      case 'N':
        FolderName=TRUE;
        break;
      case 'W':
        NameOnly=TRUE;
        break;
      case 'P':
        PathOnly=TRUE;
        break;
      case '*':
        strcpy(LocalAllFilesMask,"*");
        break;
      default:
        BreakScan=TRUE;
        break;
    }
    if (BreakScan)
      break;
    VarLength++;
  }
  if ((MaxNamesLength-=Length)<=0)
    MaxNamesLength=1;
  if (MaxNamesLength>MAX_COMMAND_LENGTH-512)
    MaxNamesLength=MAX_COMMAND_LENGTH-512;
  if (FolderMask==FALSE && FolderName==FALSE)
    FolderName=TRUE;
  strcpy(SaveStr,Command+VarLength);
  switch(Command[2])
  {
    case 'A':
      strcpy(Command,ArcName);
      //if (AnsiCode)
        //OemToChar(Command,Command);
      if (PathOnly)
      {
        char *NamePtr=(char *)FSF.PointToName(Command);
        if (NamePtr!=Command)
          *(NamePtr-1)=0;
        else
          strcpy(Command," ");
      }
      FSF.QuoteSpaceOnly(Command);
      break;
    case 'a':
      {
        int Dot=strchr(FSF.PointToName(ArcName),'.')!=NULL;
        strcpy(Command, ArcName);//TODO: ConvertNameToShort(ArcName,Command);
        char *Slash=strrchr(ArcName, GOOD_SLASH);
		struct stat s;
        if (stat(ArcName, &s)==-1 && Slash!=NULL && Slash!=ArcName)
        {
          char Path[NM];
          strcpy(Path,ArcName);
          Path[Slash-ArcName]=0;
          strcpy(Command, Path); //TODO: ConvertNameToShort(Path,Command);
          strcat(Command,Slash);
        }
        if (Dot && strchr(FSF.PointToName(Command),'.')==NULL)
          strcat(Command,".");
        //if (AnsiCode)
          //OemToChar(Command,Command);
        if (PathOnly)
        {
          char *NamePtr=(char *)FSF.PointToName(Command);
          if (NamePtr!=Command)
            *(NamePtr-1)=0;
          else
            strcpy(Command," ");
        }
      }
      FSF.QuoteSpaceOnly(Command);
      break;
    case 'D':
      *Command=0;
      break;
    case 'E':
      *Command=0;
      break;
    case 'l':
    case 'L':
      if (!MakeListFile(ListFileName,QuoteName,UseSlash,
                        FolderName,NameOnly,PathOnly,FolderMask,
                        LocalAllFilesMask,AnsiCode))
        return -1;
      char QListName[NM+2];
      FSF.QuoteSpaceOnly(strcpy(QListName,ListFileName));
      strcpy(Command,QListName);
      break;
    case 'P':
      strcpy(Command,Password);
      break;
    case 'C':
      if(*CommentFileName) //второй раз сюда не лезем
        break;
      {
        *Command=0;

        HANDLE CommentFile;
        //char CommentFileName[MAX_PATH];
        char Buf[512];
        /*SECURITY_ATTRIBUTES sa;

        sa.nLength=sizeof(sa);
        sa.lpSecurityDescriptor=NULL;
        sa.bInheritHandle=TRUE; //WTF???
		*/

        if(FSF.MkTemp(CommentFileName, "FAR") &&
          (CommentFile=WINPORT(CreateFile)(MB2Wide(CommentFileName).c_str(), GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, //&sa
                       /*FILE_ATTRIBUTE_TEMPORARY|*//*FILE_FLAG_DELETE_ON_CLOSE*/0, NULL))
                       != INVALID_HANDLE_VALUE)
        {
          DWORD Count;
          if(Info.InputBox(GetMsg(MComment), GetMsg(MInputComment), NULL, "", Buf, sizeof(Buf), NULL, 0))
          //??тут можно и заполнить строку комментарием, но надо знать, файловый
          //?? он или архивный. да и имя файла в архиве тоже надо знать...
          {
            WINPORT(WriteFile)(CommentFile, Buf, strlen(Buf), &Count, NULL);
            strcpy(Command, CommentFileName);
            WINPORT(CloseHandle)(CommentFile);
          }
          WINPORT(FlushConsoleInputBuffer)(NULL);//GetStdHandle(STD_INPUT_HANDLE));
        }
      }
      break;
    case 'R':
      strcpy(Command,RealArcDir);
      if (UseSlash)
      {
      }
      FSF.QuoteSpaceOnly(Command);
      break;
    case 'W':
      strcpy(Command,TempPath);
      break;
    case 'f':
    case 'F':
      if (PanelItem!=NULL)
      {
        char CurArcDir[NM];
        strcpy(CurArcDir,ArcDir);
        int Length=strlen(CurArcDir);
        if (Length>0 && CurArcDir[Length-1]!='/')
          strcat(CurArcDir,"/");

        char Names[MAX_COMMAND_LENGTH];
        *Names=0;

        if (NameNumber==-1)
          NameNumber=0;

        while (NameNumber<ItemsNumber || Command[2]=='f')
        {
          char Name[NM*2];

          int IncreaseNumber=0,FileAttr;
          if (*NextFileName)
          {
            FSF.sprintf(Name,"%s%s%s",PrefixFileName,CurArcDir,NextFileName);
            *NextFileName=0;
            FileAttr=0;
          }
          else
          {
            int N;
            if (Command[2]=='f' && PrevFileNameNumber!=-1)
              N=PrevFileNameNumber;
            else
            {
              N=NameNumber;
              IncreaseNumber=1;
            }
            if (N>=ItemsNumber)
              break;

            *PrefixFileName=0;
            char *cFileName=PanelItem[N].FindData.cFileName;

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
            if(*cFileName == '/')
              FSF.sprintf(Name,"%s%s",PrefixFileName,cFileName+1);
            else
              FSF.sprintf(Name,"%s%s%s",PrefixFileName,CurArcDir,cFileName);
            NormalizePath(Name,Name);
            FileAttr=PanelItem[N].FindData.dwFileAttributes;
            PrevFileNameNumber=N;
          }
          //if (AnsiCode)
            //OemToChar(Name,Name);
          if (NameOnly)
          {
            char NewName[NM];
            strcpy(NewName,FSF.PointToName(Name));
            strcpy(Name,NewName);
          }
          if (PathOnly)
          {
            char *NamePtr=(char *)FSF.PointToName(Name);
            if (NamePtr!=Name)
              *(NamePtr-1)=0;
            else
              strcpy(Name," ");
          }
          if (*Names==0 || (strlen(Names)+strlen(Name)<MaxNamesLength && Command[2]!='f'))
          {
            NameNumber+=IncreaseNumber;
            if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
              char FolderMaskName[NM];
              //strcpy(LocalAllFilesMask,PrefixFileName);
              FSF.sprintf(FolderMaskName,"%s/%s",Name,LocalAllFilesMask);
              if (PathOnly)
              {
                strcpy(FolderMaskName,Name);
                char *NamePtr=(char *)FSF.PointToName(FolderMaskName);
                if (NamePtr!=FolderMaskName)
                  *(NamePtr-1)=0;
                else
                  strcpy(FolderMaskName," ");
              }
              if (FolderMask)
              {
                if (FolderName)
                  strcpy(NextFileName,FolderMaskName);
                else
                  strcpy(Name,FolderMaskName);
              }
            }

            if (QuoteName==1)
              FSF.QuoteSpaceOnly(Name);
            else
              if (QuoteName==2)
                QuoteText(Name);
//            if (UseSlash)


            if (*Names)
              strcat(Names," ");
            strcat(Names,Name);
          }
          else
            break;
        }
        strcpy(Command,Names);
      }
      else
        *Command=0;
      break;
    default:
      return FALSE;
  }
  Length=strlen(Command);
  strcat(Command,SaveStr);
  return TRUE;
}


int ArcCommand::MakeListFile(char *ListFileName,int QuoteName,
                int UseSlash,int FolderName,int NameOnly,int PathOnly,
                int FolderMask,char *LocalAllFilesMask,int AnsiCode)
{
//  FILE *ListFile;
  HANDLE ListFile;
  DWORD WriteSize;
  /*SECURITY_ATTRIBUTES sa;

  sa.nLength=sizeof(sa);
  sa.lpSecurityDescriptor=NULL;
  sa.bInheritHandle=TRUE; //WTF??? 
  */

  if (FSF.MkTemp(ListFileName,"FAR")==NULL ||
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

  char CurArcDir[NM];
  char Buf[3*NM];

  if(NameOnly)
    *CurArcDir=0;
  else
    strcpy( CurArcDir, ArcDir );

  int Length=strlen(CurArcDir);
  if (Length>0 && CurArcDir[Length-1]!='/')
    strcat(CurArcDir,"/");

//  if (UseSlash);

  for (int I=0;I<ItemsNumber;I++)
  {
    char FileName[NM];
    strncpy(FileName,PanelItem[I].FindData.cFileName, ARRAYSIZE(FileName));
    if (NameOnly)
    {
      strncpy(FileName, FSF.PointToName(FileName), ARRAYSIZE(FileName));
    }
    if (PathOnly)
    {
      char *Ptr=(char*)FSF.PointToName(FileName);
      *Ptr=0;
    }
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
           strncpy(FileName,aud->LinkName,sizeof(FileName));
      }
    }

    int Error=FALSE;
    if (((FileAttr & FILE_ATTRIBUTE_DIRECTORY)==0 || FolderName))
    {
      char OutName[NM];
      // CHECK for BUGS!!
      if(*FileName == '/')
        FSF.sprintf(OutName,"%s%s",PrefixFileName,FileName+1);
      else
        FSF.sprintf(OutName,"%s%s%s",PrefixFileName,CurArcDir,FileName);
      NormalizePath(OutName,OutName);
      if (QuoteName==1)
        FSF.QuoteSpaceOnly(OutName);
      else
        if (QuoteName==2)
          QuoteText(OutName);
      //if (AnsiCode)
        //OemToChar(OutName,OutName);

      strcpy(Buf,OutName);strcat(Buf, NATIVE_EOL);
      Error=WINPORT(WriteFile)(ListFile,Buf,strlen(Buf),&WriteSize,NULL) == FALSE;
      //Error=fwrite(Buf,1,strlen(Buf),ListFile) != strlen(Buf);
    }
    if (!Error && (FileAttr & FILE_ATTRIBUTE_DIRECTORY) && FolderMask)
    {
      char OutName[NM];
      FSF.sprintf(OutName,"%s%s%s%c%s",PrefixFileName,CurArcDir,FileName,'/',LocalAllFilesMask); 
      if (QuoteName==1)
        FSF.QuoteSpaceOnly(OutName);
      else
        if (QuoteName==2)
          QuoteText(OutName);
      //if (AnsiCode)
        //OemToChar(OutName,OutName);
      strcpy(Buf,OutName); strcat(Buf, NATIVE_EOL);
      Error=WINPORT(WriteFile)(ListFile,Buf,strlen(Buf),&WriteSize,NULL) == FALSE;
      //Error=fwrite(Buf,1,strlen(Buf),ListFile) != strlen(Buf);
    }
    if (Error)
    {
      WINPORT(CloseHandle)(ListFile);
      remove(ListFileName);
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
    remove(CommentFileName);
}
