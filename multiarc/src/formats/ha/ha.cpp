/*
  HA.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"


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


/*
#ifdef _MSC_VER
#if _MSC_VER < 1310
#pragma comment(linker, "/ignore:4078")
#pragma comment(linker, "/merge:.data=.")
#pragma comment(linker, "/merge:.rdata=.")
#pragma comment(linker, "/merge:.text=.")
#pragma comment(linker, "/section:.,RWE")
#endif
#endif
*/

static HANDLE ArcHandle;
static DWORD NextPosition,FileSize;


void WINAPI UnixTimeToFileTime( DWORD time, FILETIME * ft );

void  WINAPI _export HA_SetFarInfo(const struct PluginStartupInfo *Info)
{
  ;
}

BOOL WINAPI _export HA_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  if (DataSize<26 || Data[0]!='H' || Data[1]!='A' || Data[3]>32)
    return(FALSE);
  int Type=Data[4] & 0xf;
  if ((Type>2 && Type<14) || Data[4]>0x2f)
    return(FALSE);
  return(TRUE);
}


BOOL WINAPI _export HA_OpenArchive(const char *Name,int *Type,bool Silent)
{
  ArcHandle=WINPORT(CreateFile)(MB2Wide(Name).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  if (ArcHandle==INVALID_HANDLE_VALUE)
    return(FALSE);

  *Type=0;

  FileSize=WINPORT(GetFileSize)(ArcHandle,NULL);

  NextPosition=4;
  return(TRUE);
}


int WINAPI _export HA_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  struct HaHeader
  {
    BYTE Type;
    DWORD PackSize;
    DWORD UnpSize;
    DWORD CRC;
    DWORD FileTime;
  } Header;
  DWORD ReadSize;
  NextPosition=WINPORT(SetFilePointer)(ArcHandle,NextPosition,NULL,FILE_BEGIN);
  if (NextPosition==0xFFFFFFFF)
    return(GETARC_READERROR);
  if (NextPosition>FileSize)
    return(GETARC_UNEXPEOF);
  if (!WINPORT(ReadFile)(ArcHandle,&Header,sizeof(Header),&ReadSize,NULL))
    return(GETARC_READERROR);
  if (ReadSize==0)
    return(GETARC_EOF);
  char Path[3*NM],Name[NM];
  if (!WINPORT(ReadFile)(ArcHandle,Path,sizeof(Path),&ReadSize,NULL) || ReadSize==0)
    return(GETARC_READERROR);
  Path[NM-1]=0;
  int PathLength=strlen(Path)+1;
  strncpy(Name,Path+PathLength,sizeof(Name));
  int Length=PathLength+strlen(Name)+1;
  DWORD PrevPosition=NextPosition;
  NextPosition+=sizeof(Header)+Length+Path[Length]+1+Header.PackSize;
  if (PrevPosition>=NextPosition)
    return(GETARC_BROKEN);
  char *EndSym=strrchr(Path,255);
  if (EndSym!=NULL)
    *EndSym=0;
  if (*Path)
    strcat(Path,"/");
  strcat(Path,Name);
  for (int I=0;Path[I]!=0;I++)
    if ((unsigned char)Path[I]==0xff)
      Path[I]='/';
  strncpy(Item->FindData.cFileName,Path,sizeof(Item->FindData.cFileName));
  Item->FindData.dwFileAttributes=(Header.Type & 0xf)==0xe ? FILE_ATTRIBUTE_DIRECTORY:0;
  Item->CRC32=Header.CRC;
  UnixTimeToFileTime(Header.FileTime,&Item->FindData.ftLastWriteTime);
  Item->FindData.nFileSizeLow=Header.UnpSize;
  Item->FindData.nFileSizeHigh=0;
  Item->PackSize=Header.PackSize;
  return(GETARC_SUCCESS);
}


BOOL WINAPI _export HA_CloseArchive(struct ArcInfo *Info)
{
  return(WINPORT(CloseHandle)(ArcHandle));
}

BOOL WINAPI _export HA_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  if (Type==0)
  {
    strcpy(FormatName,"HA");
    strcpy(DefaultExt,"ha");
    return(TRUE);
  }
  return(FALSE);
}


BOOL WINAPI _export HA_GetDefaultCommands(int Type,int Command,char *Dest)
{
  if (Type==0)
  {
    static const char *Commands[]={
    /*Extract               */"ha xay %%a %%FMQ",
    /*Extract without paths */"ha eay %%a %%FMQ",
    /*Test                  */"ha t %%a %%FMQ",
    /*Delete                */"ha d %%a %%FMQ",
    /*Comment archive       */"",
    /*Comment files         */"",
    /*Convert to SFX        */"",
    /*Lock archive          */"",
    /*Protect archive       */"",
    /*Recover archive       */"",
    /*Add files             */"ha as2{%%S} %%a %%FQ",
    /*Move files            */"ha asm2{%%S} %%a %%FQ",
    /*Add files and folders */"ha asr2{%%S} %%a %%FMQ",
    /*Move files and folders*/"ha asmr2{%%S} %%a %%FMQ",
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
