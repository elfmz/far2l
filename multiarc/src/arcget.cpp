#include "MultiArc.hpp"
#include "marclng.hpp"
#include <farkeys.hpp>
#include <errno.h>

class TRecur //$ 07.04.2002 AA
{
public:
  static int Count;
  TRecur(){Count++;}
  ~TRecur(){Count--;}
};
int TRecur::Count=0;

inline void CreateDirectory(char *FullPath) //$ 16.05.2002 AA
{
  if(!FileExists(FullPath))
    for(char *c=FullPath; *c; c++)
    {
      if(*c!=' ')
      {
        for(; *c; c++)
          if(*c=='/')
          {
            *c=0;
            mkdir(FullPath, 0777);
            *c='/';
          }
        mkdir(FullPath, 0777);
        break;
      }
    }
}

int PluginClass::GetFiles(PluginPanelItem *PanelItem, int ItemsNumber,
                          int Move, char *DestPath, int OpMode)
{
  //êîñòûëü ïðîòèâ çàöèêëèâàíèÿ â FAR'å ïðè Quick View àðõèâîâ ñ ïàðîëåì
  TRecur Recur;   //$ 07.04.2002 AA
  if(Recur.Count>1 && OpMode&(OPM_VIEW|OPM_QUICKVIEW))
    return 0;

  char SaveDir[NM];
  if (!getcwd(SaveDir, sizeof(SaveDir)))
	  strcpy(SaveDir, ".");
  char Command[512],AllFilesMask[32];
  if (ItemsNumber==0)
    return /*0*/1; //$ 07.02.2002 AA ÷òîáû ìíîãîòîìíûå CABû íîðìàëüíî ðàñïàêîâûâàëèñü
  if (*DestPath)
    FSF.AddEndSlash(DestPath);
  const char *PathHistoryName="ExtrDestPath";
  InitDialogItem InitItems[]={
  /* 0 */{DI_DOUBLEBOX,3,1,72,13,0,0,0,0,(char *)MExtractTitle},
  /* 1 */{DI_TEXT,5,2,0,0,0,0,0,0,(char *)MExtractTo},
  /* 2 */{DI_EDIT,5,3,70,3,1,(DWORD_PTR)PathHistoryName,DIF_HISTORY,0,DestPath},
  /* 3 */{DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,""},
  /* 4 */{DI_TEXT,5,5,0,0,0,0,0,0,(char *)MExtrPassword},
  /* 5 */{DI_PSWEDIT,5,6,35,5,0,0,0,0,""},
  /* 6 */{DI_TEXT,3,7,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,""},
  /* 7 */{DI_CHECKBOX,5,8,0,0,0,0,0,0,(char *)MExtrWithoutPaths},
  /* 8 */{DI_CHECKBOX,5,9,0,0,0,0,0,0,(char *)MBackground},
  /* 9 */{DI_CHECKBOX,5,10,0,0,0,0,0,0,(char *)MExtrDel},
  /*10 */{DI_TEXT,3,11,0,11,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,""},
  /*11 */{DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,1,(char *)MExtrExtract},
  /*12 */{DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,0,(char *)MExtrCancel},
  };

  FarDialogItem DialogItems[ARRAYSIZE(InitItems)];
  InitDialogItems(InitItems,DialogItems,ARRAYSIZE(InitItems));

  int AskVolume=(OpMode & (OPM_FIND|OPM_VIEW|OPM_EDIT))==0 &&
                CurArcInfo.Volume && *CurDir==0;

  if (!AskVolume)
  {
    DialogItems[7].Selected=TRUE;
    for (int I=0;I<ItemsNumber;I++)
      if (PanelItem[I].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        DialogItems[7].Selected=FALSE;
        break;
      }
  }

  Opt.UserBackground=0; // $ 14.02.2001 raVen //ñáðîñ ãàëêè "ôîíîâàÿ àðõèâàöèÿ"
  if ((OpMode & ~OPM_SILENT) & ~OPM_TOPLEVEL)
    Opt.OldUserBackground=0; // $ 03.07.02 AY: åñëè OPM_SILENT íî íå èç çà Shift-F2 ïðè íåñêîëüêî âûáðàíûõ àðõèâàõ
  DialogItems[8].Selected=Opt.UserBackground;
  DialogItems[9].Selected=Move;

  if ((OpMode & OPM_SILENT)==0)
  {
    int AskCode=Info.Dialog(Info.ModuleNumber,-1,-1,76,15,"ExtrFromArc",
                DialogItems,ARRAYSIZE(DialogItems));
    if (AskCode!=11)
      return -1;
    strcpy(DestPath,DialogItems[2].Data);
    FSF.Unquote(DestPath);
    Opt.UserBackground=DialogItems[8].Selected;
    Opt.OldUserBackground=Opt.UserBackground; // $ 02.07.2002 AY: çàïîìíèì è íå áóäåì íå ãäå ñáðàñûâàòü
    //SetRegKey(HKEY_CURRENT_USER,"","Background",Opt.UserBackground); // $ 06.02.2002 AA
  }

  LastWithoutPathsState=DialogItems[7].Selected;

  Opt.Background=OpMode & OPM_SILENT ? Opt.OldUserBackground : Opt.UserBackground; // $ 02.07.2002 AY: Åñëè OPM_SILENT çíà÷èò âûáðàíî íåñêîëüêî àðõèâîâ

  /*int SpaceOnly=TRUE;
  for (int I=0;DestPath[I]!=0;I++)
    if (DestPath[I]!=' ')
    {
      SpaceOnly=FALSE;
      break;
    }

  if (!SpaceOnly)
  {
    for (char *ChPtr=DestPath;*ChPtr!=0;ChPtr++)
      if (*ChPtr=='/')
      {
        *ChPtr=0;
        CreateDirectory(DestPath,NULL);
        *ChPtr='/';
      }
    CreateDirectory(DestPath,NULL);
  }*/
  CreateDirectory(DestPath); //$ 16.05.2002 AA


  if (*DestPath && DestPath[strlen(DestPath)-1]!=':')
    FSF.AddEndSlash(DestPath);
  GetCommandFormat(CMD_ALLFILESMASK,AllFilesMask,sizeof(AllFilesMask));

  PluginPanelItem MaskPanelItem;

  if (AskVolume)
  {
    char VolMsg[300];
    int MsgCode;

    /*if(OpMode & OPM_TOPLEVEL) // $ 16.02.2002 AA
    {
      //?? åñòü ðàçíèöà ìåæäó èçâëå÷åíèåì âûäåëåííûõ ôàéëîâ òîìà è
      //èçâëå÷åíèåì èç âûäåëåííûõ òîìîâ. çäåñü ìîæíî åå ó÷åñòü.
      //êàê ìèíèìóì - íóæíî èçìåíèòü íàäïèñü â ìåññàäæáîêñå
      MsgCode=1;
    }
    else        */
    {
      char NameMsg[NM];
      FSF.TruncPathStr(strncpy(NameMsg,FSF.PointToName(ArcName),sizeof(NameMsg)),MAX_WIDTH_MESSAGE);
      FSF.sprintf(VolMsg,GetMsg(MExtrVolume),FSF.PointToName(NameMsg));
      const char *MsgItems[]={GetMsg(MExtractTitle),VolMsg,GetMsg(MExtrVolumeAsk1),
                        GetMsg(MExtrVolumeAsk2),GetMsg(MExtrVolumeSelFiles),
                        GetMsg(MExtrAllVolumes)};
      MsgCode=Info.Message(Info.ModuleNumber,0,NULL,MsgItems,ARRAYSIZE(MsgItems),2);
    }
    if (MsgCode<0)
      return -1;
    if (MsgCode==1)
    {
      memset(&MaskPanelItem,0,sizeof(MaskPanelItem));
      strcpy(MaskPanelItem.FindData.cFileName,AllFilesMask);
      strcpy(MaskPanelItem.FindData.cAlternateFileName,AllFilesMask);
      if (ItemsInfo.Encrypted)
        MaskPanelItem.Flags=F_ENCRYPTED;
      PanelItem=&MaskPanelItem;
      ItemsNumber=1;
    }
  }

  int CommandType=LastWithoutPathsState ? CMD_EXTRACTWITHOUTPATH:CMD_EXTRACT;
  GetCommandFormat(CommandType,Command,sizeof(Command));

  if (*DialogItems[5].Data==0 && strstr(Command,"%%P")!=NULL)
    for (int I=0;I<ItemsNumber;I++)
      if ((PanelItem[I].Flags & F_ENCRYPTED) || (ItemsInfo.Encrypted &&
          (PanelItem[I].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
      {
        if(OpMode&OPM_FIND || !GetPassword(DialogItems[5].Data,FSF.PointToName(ArcName)))
          return -1;
        break;
      }

  if (chdir(DestPath)) fprintf(stderr, "chdir('%s') - %u\n", DestPath, errno);
  int SaveHideOut=Opt.HideOutput;
  if (OpMode & OPM_FIND)
    Opt.HideOutput=2;
  int IgnoreErrors=(CurArcInfo.Flags & AF_IGNOREERRORS);

  ArcCommand ArcCmd(PanelItem,ItemsNumber,Command,ArcName,CurDir,
             DialogItems[5].Data,AllFilesMask,IgnoreErrors,
             (OpMode & OPM_VIEW)!=0,(OpMode & OPM_FIND),CurDir);

  //ïîñëåäóþùèå îïåðàöèè (òåñòèðîâàíèå è òä) íå äîëæíû áûòü ôîíîâûìè
  Opt.Background=0; // $ 06.02.2002 AA

  Opt.HideOutput=SaveHideOut;
  if (chdir(SaveDir)) fprintf(stderr, "chdir('%s') - %u\n", SaveDir, errno);
  if (!IgnoreErrors && ArcCmd.GetExecCode()!=0)
    if (!(OpMode & OPM_VIEW))
      return 0;

  if (DialogItems[9].Selected)
    DeleteFiles(PanelItem,ItemsNumber,TRUE);

  if (Opt.UpdateDescriptions)
    for (int I=0;I<ItemsNumber;I++)
      PanelItem[I].Flags|=PPIF_PROCESSDESCR;

  return 1;
}
