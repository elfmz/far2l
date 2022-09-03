#include <fcntl.h>
#include <assert.h>
#include "MultiArc.hpp"
#include "marclng.hpp"

PluginClass::PluginClass(int ArcPluginNumber)
{
  *ArcName=0;
  *CurDir=0;
  PluginClass::ArcPluginNumber=ArcPluginNumber;
  DizPresent=FALSE;
  bGOPIFirstCall=true;
  *farlang=0;
  ZeroFill(CurArcInfo);
}


PluginClass::~PluginClass()
{
  FreeArcData();
}

void PluginClass::FreeArcData()
{
  for (auto &I : ArcData)
  {
    if(I.UserData && (I.Flags & PPIF_USERDATA))
    {
      struct ArcItemUserData *aud=(struct ArcItemUserData*)I.UserData;
      if(aud->Prefix)
        free((void *)aud->Prefix);
      if(aud->LinkName)
        free((void *)aud->LinkName);
      free((void *)I.UserData);
    }
  }
  ArcData.clear();
}

int PluginClass::PreReadArchive(const char *Name)
{
  if (sdc_stat(Name, &ArcStat) == -1) {
    return FALSE;
  }

  strcpy(ArcName,Name);

  if (strchr(FSF.PointToName(ArcName),'.')==NULL)
    strcat(ArcName,".");

  return TRUE;
}


static void SanitizeString(std::string &s)
{
	while (!s.empty() && !s.back()) {
		s.pop_back();
	}
}

int PluginClass::ReadArchive(const char *Name,int OpMode)
{
  bGOPIFirstCall=true;
  FreeArcData();
  DizPresent=FALSE;

  if (sdc_stat(Name, &ArcStat) == -1)
    return FALSE;

  if (!ArcPlugin->OpenArchive(ArcPluginNumber,Name,&ArcPluginType,(OpMode & OPM_SILENT)!=0))
    return FALSE;

  ItemsInfo = ArcItemInfo{};
  ZeroFill(CurArcInfo);
  TotalSize=PackedSize=0;

  HANDLE hScreen=Info.SaveScreen(0,0,-1,-1);

  DWORD UpdateTime = GetProcessUptimeMSec() + 1000;
  int WaitMessage=FALSE;
  int GetItemCode;

  while (1)
  {
    try {
      ArcData.emplace_back();
    } catch (std::exception &e) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
      break;
    }

    auto &CurItemInfo = ArcData.back();
    GetItemCode=ArcPlugin->GetArcItem(ArcPluginNumber,&CurItemInfo);
    SanitizeString(CurItemInfo.PathName);
    SanitizeString(CurItemInfo.HostOS);
    SanitizeString(CurItemInfo.Description);


    if (GetItemCode!=GETARC_SUCCESS)
    {
      ArcData.pop_back();
      break;
    }

    if ((ArcData.size() & 0x1f)==0)
    {
      if (CheckForEsc())
      {
        ArcData.pop_back();
        FreeArcData();
        ArcPlugin->CloseArchive(ArcPluginNumber,&CurArcInfo);
        Info.RestoreScreen(NULL);
        Info.RestoreScreen(hScreen);
        return FALSE;
      }

      const DWORD Now = GetProcessUptimeMSec();
      if (Now >= UpdateTime)
      {
        UpdateTime = Now + 100;
        char FilesMsg[100];
        char NameMsg[NM];
        FSF.sprintf(FilesMsg,GetMsg(MArcReadFiles),(unsigned int)ArcData.size());
        const char *MsgItems[]={GetMsg(MArcReadTitle),GetMsg(MArcReading),NameMsg,FilesMsg};
        FSF.TruncPathStr(strncpy(NameMsg,Name,sizeof(NameMsg)-1),MAX_WIDTH_MESSAGE);
        Info.Message(Info.ModuleNumber,WaitMessage ? FMSG_KEEPBACKGROUND:0,NULL,MsgItems,
                   ARRAYSIZE(MsgItems),0);
        WaitMessage=TRUE;
      }
    }

    if (!CurItemInfo.Description.empty())
    {
//      CurArcData.Description=new char[strlen(CurItemInfo.Description)+1];
//      if (CurArcData.Description)
//        strcpy(CurArcData.Description,CurItemInfo.Description);
      DizPresent=TRUE;
    }

    if (ItemsInfo.HostOS != CurItemInfo.HostOS)
      ItemsInfo.HostOS = (ItemsInfo.HostOS.empty() ? CurItemInfo.HostOS : GetMsg(MSeveralOS));

    if (ItemsInfo.Codepage <= 0)
      ItemsInfo.Codepage=CurItemInfo.Codepage;

    ItemsInfo.Solid|=CurItemInfo.Solid;
    ItemsInfo.Comment|=CurItemInfo.Comment;
    ItemsInfo.Encrypted|=CurItemInfo.Encrypted;

    if (CurItemInfo.Encrypted)
      CurItemInfo.Flags|=F_ENCRYPTED;

    if (CurItemInfo.DictSize>ItemsInfo.DictSize)
      ItemsInfo.DictSize=CurItemInfo.DictSize;

    if (CurItemInfo.UnpVer>ItemsInfo.UnpVer)
      ItemsInfo.UnpVer=CurItemInfo.UnpVer;

    CurItemInfo.NumberOfLinks=1;

    NormalizePath(CurItemInfo.PathName);
    //fprintf(stderr, "PATH: %s\n", CurItemInfo.PathName.c_str());

    struct ArcItemUserData *aud=NULL;
    char *Pref=NULL;

    const char *NamePtr=CurItemInfo.PathName.c_str();
    const char *EndPos=NamePtr;
    while(*EndPos == '.') EndPos++;
    if(*EndPos == '/')
      while(*EndPos == '/') EndPos++;
    else
      EndPos=NamePtr;
    if(EndPos != NamePtr)
    {
      Pref=(char *)malloc((int)(EndPos-NamePtr)+1);
      if(Pref)
      {
        memcpy(Pref,NamePtr,(int)(EndPos-NamePtr));
        Pref[(int)(EndPos-NamePtr)]=0;
      }
    }

    if(CurItemInfo.UserData || Pref || CurItemInfo.Codepage > 0)
    {
       if((aud=(struct ArcItemUserData*)malloc(sizeof(struct ArcItemUserData))) != NULL)
       {
         CurItemInfo.Flags |= PPIF_USERDATA;
         aud->SizeStruct=sizeof(struct ArcItemUserData);
         aud->Prefix=Pref;
         aud->LinkName=CurItemInfo.UserData?(char *)CurItemInfo.UserData:NULL;
         aud->Codepage=CurItemInfo.Codepage;
         CurItemInfo.UserData=(DWORD_PTR)aud;
       }
       else
         CurItemInfo.UserData=0;
    }
    if(!CurItemInfo.UserData && Pref)
      free(Pref);


    if (EndPos!=NamePtr)
    {
      assert(size_t(EndPos - NamePtr) <= CurItemInfo.PathName.size());
      CurItemInfo.PathName.erase(0, EndPos - NamePtr);
    }

    if (!CurItemInfo.PathName.empty() && CurItemInfo.PathName.back() == '/')
    {
      CurItemInfo.PathName.pop_back();
      CurItemInfo.dwFileAttributes|=FILE_ATTRIBUTE_DIRECTORY;
    }

    TotalSize+=CurItemInfo.nFileSize;
    PackedSize+=CurItemInfo.nPhysicalSize;
  }

  Info.RestoreScreen(NULL);
  Info.RestoreScreen(hScreen);

  ArcData.shrink_to_fit();
  ArcPlugin->CloseArchive(ArcPluginNumber,&CurArcInfo);

  if(GetItemCode != GETARC_EOF && GetItemCode != GETARC_SUCCESS)
  {
    switch(GetItemCode)
    {
      case GETARC_BROKEN:
        GetItemCode=MBadArchive;
        break;

      case GETARC_UNEXPEOF:
        GetItemCode=MUnexpEOF;
        break;

      case GETARC_READERROR:
        GetItemCode=MReadError;
        break;
    }

    char NameMsg[NM];
    const char *MsgItems[]={GetMsg(MError),NameMsg,GetMsg(GetItemCode),GetMsg(MOk)};
    FSF.TruncPathStr(strncpy(NameMsg,Name,sizeof(NameMsg)-1),MAX_WIDTH_MESSAGE);
    Info.Message(Info.ModuleNumber,FMSG_WARNING,NULL,MsgItems,ARRAYSIZE(MsgItems),1);
    return FALSE; // Mantis#0001241
  }

  //Info.RestoreScreen(NULL);
  //Info.RestoreScreen(hScreen);
  return TRUE;
}

bool PluginClass::EnsureFindDataUpToDate(int OpMode)
{
  if (!ArcData.empty())
  {
    struct stat NewArcStat{};
    if (sdc_stat(ArcName, &NewArcStat) == -1)
      return false;

    if (ArcStat.st_mtime == NewArcStat.st_mtime && ArcStat.st_size == NewArcStat.st_size)
      return true;
  }

  DWORD size = (DWORD)Info.AdvControl(Info.ModuleNumber,ACTL_GETPLUGINMAXREADDATA,(void *)0);
  int fd = sdc_open(ArcName, O_RDONLY);
  if (fd == -1)
      return false;

  unsigned char *Data = (unsigned char *)malloc(size);
  ssize_t read_size = Data ? sdc_read(fd, Data, size) : -1;
  sdc_close(fd);

  if (read_size <= 0)
      return false;

  DWORD SFXSize = 0;

  bool ReadArcOK = (ArcPlugin->IsArchive(ArcPluginNumber, ArcName, Data, read_size, &SFXSize)
              && ReadArchive(ArcName, OpMode));

  free(Data);

  return ReadArcOK;
}

static void ArcItemInfo2PluginPanelItem(const ArcItemInfo &aai, PluginPanelItem &ppi)
{
	ZeroFill(ppi);
	ppi.FindData.ftCreationTime = aai.ftCreationTime;
	ppi.FindData.ftLastAccessTime = aai.ftLastAccessTime;
	ppi.FindData.ftLastWriteTime = aai.ftLastWriteTime;
	ppi.FindData.nPhysicalSize = aai.nPhysicalSize;
	ppi.FindData.nFileSize = aai.nFileSize;
	ppi.FindData.dwFileAttributes = aai.dwFileAttributes;
	ppi.FindData.dwUnixMode = aai.dwUnixMode;
	strncpy(ppi.FindData.cFileName, aai.PathName.c_str(), ARRAYSIZE(ppi.FindData.cFileName));
	ppi.UserData = aai.UserData;
	ppi.Flags = aai.Flags;
	ppi.NumberOfLinks = aai.NumberOfLinks;
	ppi.CRC32 = aai.CRC32;
    ppi.Description = aai.Description.empty() ? nullptr : (char *)aai.Description.c_str();
}

int PluginClass::GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
  if (!EnsureFindDataUpToDate(OpMode))
    return FALSE;

  size_t CurDirLength = strlen(CurDir);
  *pPanelItem=NULL;
  *pItemsNumber=0;
  int AlocatedItemsNumber=0;
  for (const auto &I : ArcData)
  {
    ArcItemInfo CurItem = I;
    const char *Name = I.PathName.c_str();
    BOOL Append=FALSE;

    if (Name[0]==GOOD_SLASH)
      Append=TRUE;

    if (Name[0]=='.' && (Name[1]==GOOD_SLASH || (Name[1]=='.' && Name[2]==GOOD_SLASH)))
      Append=TRUE;

    if (!Append && strlen(Name) > CurDirLength && 
		strncmp(Name,CurDir,CurDirLength)==0 && (CurDirLength==0 || Name[CurDirLength]==GOOD_SLASH))
    {
      const char *StartName = Name + CurDirLength + (CurDirLength!=0);
      const char *Slash = strchr(StartName,'/');
      size_t NameLen;
      if (Slash != NULL)
      {
        NameLen = Slash - StartName;
        CurItem.dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;
        CurItem.nFileSize=0;
        CurItem.nPhysicalSize=0;
      } else
		NameLen = strlen(StartName);

	  if (StartName	!= Name)
      {
        assert(size_t(StartName - Name) <= I.PathName.size());
        CurItem.PathName.erase(0, StartName - Name);
      }
      CurItem.PathName.resize(NameLen);
      Append=TRUE;

      if (CurItem.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        for (int J=0; J < *pItemsNumber; J++)
          if ((*pPanelItem)[J].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            if (StrMatchArray(CurItem.PathName, (*pPanelItem)[J].FindData.cFileName))
            {
              Append=FALSE;
              (*pPanelItem)[J].FindData.dwFileAttributes |= CurItem.dwFileAttributes;
            }
      }
    }

    if (Append)
    {
      PluginPanelItem *NewPanelItem=*pPanelItem;
      if (*pItemsNumber>=AlocatedItemsNumber)
      {
        AlocatedItemsNumber=AlocatedItemsNumber+256+AlocatedItemsNumber/4;
        NewPanelItem=(PluginPanelItem *)realloc(*pPanelItem,AlocatedItemsNumber*sizeof(PluginPanelItem));

        if (NewPanelItem==NULL)
          break;

        *pPanelItem=NewPanelItem;
      }
      ArcItemInfo2PluginPanelItem(CurItem, NewPanelItem[*pItemsNumber]);
      //NewPanelItem[*pItemsNumber]=CurItem;
      (*pItemsNumber)++;
    }

  }
  if (*pItemsNumber>0)
    *pPanelItem=(PluginPanelItem *)realloc(*pPanelItem,*pItemsNumber*sizeof(PluginPanelItem));
  return TRUE;
}


void PluginClass::FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber)
{
  if(PanelItem) free(PanelItem);
}


int PluginClass::SetDirectory(const char *Dir,int OpMode)
{
  if (*Dir=='/' && *(++Dir)==0)
  {
    *CurDir=0;
    return TRUE;
  }

  if (strcmp(Dir,"..")==0)
  {
    if (*CurDir==0)
      return FALSE;

    char *Slash=strrchr(CurDir,'/');
    if (Slash!=NULL)
      *Slash=0;
    else
      *CurDir=0;
  }
  else
  {
    int Found=FALSE;
    int CurDirLength=strlen(CurDir);
    if (CurDirLength!=0)
      CurDirLength++;

    size_t NewDirLength=strlen(Dir);

    for (const auto &I : ArcData)
    {
      const char *CurName=I.PathName.c_str();

      if (strlen(CurName)>=CurDirLength+NewDirLength && strncmp(CurName+CurDirLength,Dir,NewDirLength)==0)
      {
        char Ch=CurName[CurDirLength+NewDirLength];
        if (Ch=='/' || Ch==0)
        {
          Found=TRUE;
          break;
        }
      }
    }

    if (!Found)
      return FALSE;

    if (*CurDir==0 || *Dir==0 || strchr(Dir,'/')!=0)
      strcpy(CurDir,Dir);
    else
    {
      FSF.AddEndSlash(CurDir);
      strcat(CurDir,Dir);
    }
  }

  return TRUE;
}

bool PluginClass::FarLangChanged()
{
  const char *tmplang = getenv("FARLANG");

  if (!tmplang)
	  tmplang = "English";

  if (!strcmp(tmplang, farlang))
    return false;

  strcpy(farlang, tmplang);

  return true;
}

void PluginClass::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
  Info->StructSize=sizeof(*Info);
  Info->Flags=OPIF_USEFILTER|OPIF_USESORTGROUPS|OPIF_USEHIGHLIGHTING|
              OPIF_ADDDOTS|OPIF_COMPAREFATTIME;
  Info->HostFile=ArcName;
  Info->CurDir=CurDir;

  if (bGOPIFirstCall)
    ArcPlugin->GetFormatName(ArcPluginNumber,ArcPluginType,FormatName,DefExt);

  char NameTitle[NM];
  strncpy(NameTitle,FSF.PointToName(ArcName),sizeof(NameTitle)-1);

  {
    struct PanelInfo PInfo;
    if(::Info.Control((HANDLE)this,FCTL_GETPANELSHORTINFO,&PInfo))
    {     //TruncStr
      FSF.TruncPathStr(NameTitle,(PInfo.PanelRect.right-PInfo.PanelRect.left+1-(strlen(FormatName)+3+4)));
    }
  }

  FSF.sprintf(Title," %s:%s%s%s ",FormatName,NameTitle, *CurDir ? "/" : "", *CurDir ? CurDir : "");

  Info->PanelTitle=Title;

  if (bGOPIFirstCall || FarLangChanged())
  {
    FSF.sprintf(Format,GetMsg(MArcFormat),FormatName);

    memset(InfoLines,0,sizeof(InfoLines));
    FSF.sprintf(InfoLines[0].Text,GetMsg(MInfoTitle),FSF.PointToName(ArcName));
    InfoLines[0].Separator=TRUE;
    FSF.sprintf(InfoLines[1].Text,GetMsg(MInfoArchive));
    strcpy(InfoLines[1].Data,FormatName);

    if (ItemsInfo.UnpVer!=0)
      FSF.sprintf(InfoLines[1].Data+strlen(InfoLines[1].Data)," %d.%d",
              ItemsInfo.UnpVer/256,ItemsInfo.UnpVer%256);

    if (!ItemsInfo.HostOS.empty())
      FSF.sprintf(InfoLines[1].Data+strlen(InfoLines[1].Data),"/%s",ItemsInfo.HostOS.c_str());

    strcpy(InfoLines[2].Text,GetMsg(MInfoArcType));

    if (ItemsInfo.Solid)
      strcpy(InfoLines[2].Data,GetMsg(MInfoSolid));

    if (CurArcInfo.SFXSize)
    {
      if (*InfoLines[2].Data)
        strcat(InfoLines[2].Data," ");
      strcat(InfoLines[2].Data,GetMsg(MInfoSFX));
    }

    if (CurArcInfo.Flags & AF_HDRENCRYPTED)
    {
      if (*InfoLines[2].Data)
        strcat(InfoLines[2].Data," ");
      strcat(InfoLines[2].Data,GetMsg(MInfoHdrEncrypted));
    }

    if (CurArcInfo.Volume)
    {
      if (*InfoLines[2].Data)
        strcat(InfoLines[2].Data," ");
      strcat(InfoLines[2].Data,GetMsg(MInfoVolume));
    }

    if (*InfoLines[2].Data==0)
      strcpy(InfoLines[2].Data,GetMsg(MInfoNormal));

    strcpy(InfoLines[3].Text,GetMsg(MInfoArcComment));
    strcpy(InfoLines[3].Data,CurArcInfo.Comment ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[4].Text,GetMsg(MInfoFileComments));
    strcpy(InfoLines[4].Data,ItemsInfo.Comment ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[5].Text,GetMsg(MInfoPasswords));
    strcpy(InfoLines[5].Data,ItemsInfo.Encrypted ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[6].Text,GetMsg(MInfoRecovery));
    strcpy(InfoLines[6].Data,CurArcInfo.Recovery ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[7].Text,GetMsg(MInfoLock));
    strcpy(InfoLines[7].Data,CurArcInfo.Lock ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[8].Text,GetMsg(MInfoAuthVer));
    strcpy(InfoLines[8].Data,(CurArcInfo.Flags & AF_AVPRESENT) ? GetMsg(MInfoPresent):GetMsg(MInfoAbsent));
    strcpy(InfoLines[9].Text,GetMsg(MInfoDict));

    if (ItemsInfo.DictSize==0)
      strcpy(InfoLines[9].Data,"???");
    else
      FSF.sprintf(InfoLines[9].Data,"%d %s",ItemsInfo.DictSize,GetMsg(MInfoDictKb));

    strcpy(InfoLines[10].Text,GetMsg(MInfoChapters));
    if(CurArcInfo.Chapters)
      //FSF.sprintf(InfoLines[10].Data,"%d/%d",ItemsInfo.Chapter,CurArcInfo.Chapters);
      FSF.sprintf(InfoLines[10].Data,"%d",CurArcInfo.Chapters);
    else
      strcpy(InfoLines[10].Data,GetMsg(MInfoAbsent));

    strcpy(InfoLines[11].Text,GetMsg(MInfoTotalFiles));
    FSF.sprintf(InfoLines[11].Data,"%ld",(unsigned long)ArcData.size());
    strcpy(InfoLines[12].Text,GetMsg(MInfoTotalSize));
    InsertCommas(TotalSize,InfoLines[12].Data);
    strcpy(InfoLines[13].Text,GetMsg(MInfoPackedSize));
    InsertCommas(PackedSize,InfoLines[13].Data);
    strcpy(InfoLines[14].Text,GetMsg(MInfoRatio));
    FSF.sprintf(InfoLines[14].Data,"%d%%",MA_ToPercent(PackedSize,TotalSize));

    memset(&KeyBar,0,sizeof(KeyBar));
    KeyBar.ShiftTitles[1-1]=(char*)"";
    KeyBar.AltTitles[6-1]=(char*)GetMsg(MAltF6);
    KeyBar.AltShiftTitles[9-1]=(char*)GetMsg(MAltShiftF9);
  }

  Info->Format=Format;
  Info->KeyBar=&KeyBar;
  Info->InfoLines=InfoLines;
  Info->InfoLinesNumber=ARRAYSIZE(InfoLines);

  strcpy(DescrFilesString,Opt.DescriptionNames);
  size_t DescrFilesNumber=0;
  char *NamePtr=DescrFilesString;

  while (DescrFilesNumber<ARRAYSIZE(DescrFiles))
  {
    while (__isspace(*NamePtr))
      NamePtr++;
    if (*NamePtr==0)
      break;
    DescrFiles[DescrFilesNumber++]=NamePtr;
    if ((NamePtr=strchr(NamePtr,','))==NULL)
      break;
    *(NamePtr++)=0;
  }

  Info->DescrFiles=DescrFiles;

  if (!Opt.ReadDescriptions || DizPresent)
    Info->DescrFilesNumber=0;
  else
    Info->DescrFilesNumber=(int)DescrFilesNumber;

  bGOPIFirstCall = false;
}
