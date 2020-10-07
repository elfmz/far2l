/*
  RAR.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#define STRICT

#include <windows.h>
#include <string.h>
#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"
#include "marclng.hpp"
#include "dll.hpp"

//TODO: we have full-featured unrar code on board, use it to unpack instead of unrar executable!

#if defined(__BORLANDC__)
  #pragma option -a1
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
  #pragma pack(1)
#else
  #pragma pack(push,1)
#endif



#define  LHD_LARGE          0x0100
#define  LHD_UNICODE        0x0200
#define  LHD_SALT           0x0400
#define  LHD_EXTTIME        0x1000
#define  LHD_WINDOWMASK     0x00e0
#define  LHD_DIRECTORY      0x00e0
#define  LONG_BLOCK         0x8000

enum HEADER_TYPE {
  MARK_HEAD=0x72,
  MAIN_HEAD=0x73,
  FILE_HEAD=0x74,
  COMM_HEAD=0x75,
  AV_HEAD=0x76,
  SUB_HEAD=0x77,
  PROTECT_HEAD=0x78,
  SIGN_HEAD=0x79,
  NEWSUB_HEAD=0x7a,
  ENDARC_HEAD=0x7b
};

static const char * const RarOS[]={"DOS","OS/2","Windows","Unix","MacOS","BeOS"};

static DWORD SFXSize,Flags;

static HANDLE hArcData;
static int RHCode,PFCode;
static struct RAROpenArchiveDataEx OpenArchiveData;
static struct RARHeaderDataEx HeaderData;

static char Password[NM/2];

static FARAPIINPUTBOX FarInputBox=NULL;
static FARAPIGETMSG   FarGetMsg=NULL;
static INT_PTR MainModuleNumber=-1;
static FARAPIMESSAGE FarMessage=NULL;
static FARSTDSPRINTF FarSprintf=NULL;

#define UnicodeToOEM(src,dst,lendst)    WINPORT(WideCharToMultiByte)(CP_UTF8,0,(src),-1,(dst),(lendst),NULL,FALSE)
#define  Min(x,y) (((x)<(y)) ? (x):(y))



void  WINAPI _export RAR_SetFarInfo(const struct PluginStartupInfo *Info)
{
   FarInputBox=Info->InputBox;
   FarGetMsg=Info->GetMsg;
   MainModuleNumber=Info->ModuleNumber;
   FarMessage=Info->Message;
   FarSprintf=Info->FSF->sprintf;
}

int CALLBACK CallbackProc(UINT msg,LPARAM UserData,LPARAM P1,LPARAM P2)
{
  switch(msg)
  {
    case UCM_NEEDPASSWORD:
    {
      if(FarInputBox(FarGetMsg(MainModuleNumber,MGetPasswordTitle),
                     FarGetMsg(MainModuleNumber,MGetPassword),NULL,
                     Password,Password,sizeof(Password)-1,NULL,FIB_PASSWORD))
      {
        //OemToChar(Password, Password);
        strncpy((char *)P1,Password,(int)P2);
        return(0);
      }
      return 1;
    }
  }
  return(0);
}

BOOL WINAPI _export RAR_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  for (int I=0;I<DataSize-7;I++)
  {
    const unsigned char *D=Data+I;

    // check for RAR5.0 signature
    if (D[0]==0x52 && D[1]==0x61 && D[2]==0x72 && D[3]==0x21 &&
        D[4]==0x1a && D[5]==0x07 && D[6]==0x01 && D[7]==0x00)
    {
      SFXSize=I;
      return(TRUE);
    }

    if (D[0]==0x52 && D[1]==0x45 && D[2]==0x7e && D[3]==0x5e &&
        (I==0 || (DataSize>31 && Data[28]==0x52 && Data[29]==0x53 &&
        Data[30]==0x46 && Data[31]==0x58)))
    //if (D[0]==0x52 && D[1]==0x45 && D[2]==0x7e && D[3]==0x5e)
    {
      SFXSize=I;
      return(TRUE);
    }

    // check marker block
    // The marker block is actually considered as a fixed byte sequence: 0x52 0x61 0x72 0x21 0x1a 0x07 0x00
    if (D[0]==0x52 && D[1]==0x61 && D[2]==0x72 && D[3]==0x21 &&
        D[4]==0x1a && D[5]==0x07 && D[6]==0 &&
        D[9]==0x73)                                             // next "archive header"? (Header type: 0x73)
    {
      SFXSize=I;
      return(TRUE);
    }
  }
  return(FALSE);
}


BOOL WINAPI _export RAR_OpenArchive(const char *Name,int *Type)
{
  DWORD ReadSize;


  memset(&OpenArchiveData,0,sizeof(OpenArchiveData));
  OpenArchiveData.ArcName=(char*)Name;
  OpenArchiveData.CmtBuf=NULL;
  OpenArchiveData.CmtBufSize=0;
  OpenArchiveData.OpenMode=RAR_OM_LIST;
  hArcData = RAROpenArchiveEx(&OpenArchiveData);
  if (OpenArchiveData.OpenResult!=0)
     return FALSE;

  Flags=OpenArchiveData.Flags;
  RARSetCallback(hArcData, CallbackProc, 0);
  HeaderData.CmtBuf=NULL;
  HeaderData.CmtBufSize=0;
  /*
  if(Flags&0x80)
  {
    if((RHCode=pRARReadHeaderEx(hArcData,&HeaderData)) != 0)
      return FALSE;
  }
  */

  return(TRUE);
}


int WINAPI _export RAR_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  RHCode = RARReadHeaderEx(hArcData,&HeaderData);
  if(RHCode!=0) {
      if(RHCode==ERAR_BAD_DATA)
         return GETARC_READERROR;//GETARC_BROKEN;
      return GETARC_EOF;	  
  }
  UnicodeToOEM(HeaderData.FileNameW,Item->FindData.cFileName,ARRAYSIZE(Item->FindData.cFileName)-1);
  //strcpyn(Item->FindData.cFileName,HeaderData.FileName,sizeof(Item->FindData.cFileName)-1);
  
  Item->FindData.dwFileAttributes = WINPORT(EvaluateAttributes)(HeaderData.FileAttr, HeaderData.FileNameW);
  Item->FindData.dwUnixMode = HeaderData.FileAttr;
  
  //HeaderData.FileAttr is unreliable - sometimes its a UNIX mode, sometimes Windows attributes
  //so sync with directory attribute in HeaderData.Flags
  if (HeaderData.Flags & RHDF_DIRECTORY) {
	  Item->FindData.dwFileAttributes|= FILE_ATTRIBUTE_DIRECTORY;
	  switch (Item->FindData.dwUnixMode & S_IFMT) {
		  case S_IFREG: case 0:
			Item->FindData.dwUnixMode&= ~S_IFMT;
			Item->FindData.dwUnixMode|= S_IFDIR;
	  }
  } else {
	  Item->FindData.dwFileAttributes&= ~FILE_ATTRIBUTE_DIRECTORY;
	  switch (Item->FindData.dwUnixMode & S_IFMT) {
		  case S_IFDIR: case 0:
			Item->FindData.dwUnixMode&= ~S_IFMT;
			Item->FindData.dwUnixMode|= S_IFREG;
	  }
  }
  
  Item->FindData.nFileSizeLow=HeaderData.UnpSize;
  Item->FindData.nFileSizeHigh=HeaderData.UnpSizeHigh;
  Item->PackSizeHigh=HeaderData.PackSizeHigh;
  Item->PackSize=HeaderData.PackSize;
  Item->CRC32=(DWORD)HeaderData.FileCRC;

  FILETIME lft;
  WINPORT(DosDateTimeToFileTime)(HIWORD(HeaderData.FileTime),LOWORD(HeaderData.FileTime),&lft);
  WINPORT(LocalFileTimeToFileTime)(&lft,&Item->FindData.ftLastWriteTime);

  if (HeaderData.HostOS<ARRAYSIZE(RarOS))
    strcpy(Info->HostOS,RarOS[HeaderData.HostOS]);
  Info->Solid=Flags & 8;
  Info->Comment=HeaderData.Flags & 8;
  Info->Encrypted=HeaderData.Flags & 4;
  Info->DictSize=64<<((HeaderData.Flags & 0x00e0)>>5);
  if (Info->DictSize > 4096)
    Info->DictSize=64;
  Info->UnpVer=(HeaderData.UnpVer/10)*256+(HeaderData.UnpVer%10);
  if ((PFCode=RARProcessFile(hArcData,RAR_SKIP,NULL,NULL))!=0) {
	return (RHCode==ERAR_BAD_DATA) ? GETARC_BROKEN : GETARC_READERROR;
  }
	
  return GETARC_SUCCESS;
}


BOOL WINAPI _export RAR_CloseArchive(struct ArcInfo *Info)
{
  Info->SFXSize=SFXSize;
  Info->Volume=Flags & 1;
  Info->Comment=Flags & 2;
  Info->Lock=Flags & 4;
  Info->Recovery=Flags & 64;

  if (Flags & 32)
    Info->Flags|=AF_AVPRESENT;
  if (Flags & 0x80)
    Info->Flags|=AF_HDRENCRYPTED;

  *Password=0;

  return RARCloseArchive(hArcData);
}

DWORD WINAPI _export RAR_GetSFXPos(void)
{
  return SFXSize;
}


BOOL WINAPI _export RAR_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  if (Type==0)
  {
    strcpy(FormatName,"RAR");
    strcpy(DefaultExt,"rar");
    return(TRUE);
  }
  return(FALSE);
}



BOOL WINAPI _export RAR_GetDefaultCommands(int Type,int Command,char *Dest)
{
  if (Type==0)
  {
    // Console RAR 2.50 commands
    static const char *Commands[]={
    /*Extract               */"^rar x {-p%%P} {-ap%%R} -y -c- -kb -- %%A @%%LNM",
    /*Extract without paths */"^rar e {-p%%P} -y -c- -kb -- %%A @%%LNM",
    /*Test                  */"^rar t -y {-p%%P} -- %%A",
    /*Delete                */"rar d -y {-w%%W} -- %%A @%%LNM",
    /*Comment archive       */"rar c -y {-w%%W} -- %%A",
    /*Comment files         */"rar cf -y {-w%%W} -- %%A @%%LNM",
    /*Convert to SFX        */"rar s -y -- %%A",
    /*Lock archive          */"rar k -y -- %%A",
    /*Protect archive       */"rar rr -y -- %%A",
    /*Recover archive       */"rar r -y -- %%A",
    /*Add files             */"rar a -y {-p%%P} {-ap%%R} {-w%%W} {%%S} -- %%A @%%LN",
    /*Move files            */"rar m -y {-p%%P} {-ap%%R} {-w%%W} {%%S} -- %%A @%%LN",
    /*Add files and folders */"rar a -r0 -y {-p%%P} {-ap%%R} {-w%%W} {%%S} -- %%A @%%LN",
    /*Move files and folders*/"rar m -r0 -y {-p%%P} {-ap%%R} {-w%%W} {%%S} -- %%A @%%LN",
    /*"All files" mask      */"*.*"
    };
    if (Command<(int)(ARRAYSIZE(Commands)))
    {
      strcpy(Dest,Commands[Command]);
      return(TRUE);
    }
  }
  return(FALSE);
}


