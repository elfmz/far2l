/*
  TARGZ.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

//#define USE_TAR_H


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
#pragma comment(linker, "-subsystem:console")
#pragma comment(linker, "-merge:.rdata=.text")

#endif
#endif
*/

uint64_t __cdecl _strtoui64 (const char *nptr,char **endptr,int ibase);
static uint64_t __cdecl _strtoxq (const char *nptr,const char **endptr,int ibase,int flags);
int64_t __cdecl _strtoi64(const char *nptr,char **endptr,int ibase);

#if defined(USE_TAR_H)
#include "tar.h"

#else

struct posix_header
{                               /* byte offset */
  char name[100];               /*   0 = 0x000 */
  char mode[8];                 /* 100 = 0x064 */
  char uid[8];                  /* 108 = 0x06C */
  char gid[8];                  /* 116 = 0x074 */
  char size[12];                /* 124 = 0x07C */
  char mtime[12];               /* 136 = 0x088 */
  char chksum[8];               /* 148 = 0x094 */
  char typeflag;                /* 156 = 0x09C */
  char linkname[100];           /* 157 = 0x09D */
  char magic[6];                /* 257 = 0x101 */
  char version[2];              /* 263 = 0x107 */
  char uname[32];               /* 265 = 0x109 */
  char gname[32];               /* 297 = 0x129 */
  char devmajor[8];             /* 329 = 0x149 */
  char devminor[8];             /* 337 = 0x151 */
  char prefix[155];             /* 345 = 0x159 */
                                /* 500 = 0x1F4 */
};

#define TMAGIC   "ustar"    // ustar and a null
#define TMAGLEN  6
#define GNUTMAGIC  "GNUtar"    // 7 chars and a null
#define GNUTMAGLEN 7
#define TVERSION "00"       // 00 and no null
#define TVERSLEN 2

/* OLDGNU_MAGIC uses both magic and version fields, which are contiguous.
   Found in an archive, it indicates an old GNU header format, which will be
   hopefully become obsolescent.  With OLDGNU_MAGIC, uname and gname are
   valid, though the header is not truly POSIX conforming.  */
#define OLDGNU_MAGIC "ustar  "  /* 7 chars and a null */


enum archive_format
{
  DEFAULT_FORMAT,       /* format to be decided later */
  V7_FORMAT,            /* old V7 tar format */
  OLDGNU_FORMAT,        /* GNU format as per before tar 1.12 */
  POSIX_FORMAT,         /* restricted, pure POSIX format */
  GNU_FORMAT            /* POSIX format with GNU extensions */
};


#define BLOCKSIZE 512
typedef union block {
  char  buffer[BLOCKSIZE];
  struct posix_header header;
} TARHeader;

/* Identifies the *next* file on the tape as having a long linkname.  */
#define GNUTYPE_LONGLINK 'K'

/* Identifies the *next* file on the tape as having a long name.  */
#define GNUTYPE_LONGNAME 'L'

/* Values used in typeflag field.  */
#define REGTYPE   '0'    /* regular file */
#define AREGTYPE '\0'    /* regular file */
#define LNKTYPE  '1'    /* link */
#define SYMTYPE  '2'    /* reserved */
#define CHRTYPE  '3'    /* character special */
#define BLKTYPE  '4'    /* block special */
#define DIRTYPE  '5'    /* directory */
#define FIFOTYPE '6'    /* FIFO special */
#define CONTTYPE '7'    /* reserved */

#endif

enum {TAR_FORMAT,GZ_FORMAT,Z_FORMAT,BZ_FORMAT,XZ_FORMAT};

typedef union {
  int64_t i64;
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } Part;
} FAR_INT64;



int IsTarHeader(const unsigned char *Data,int DataSize);
int64_t GetOctal(const char *Str);
int GetArcItemGZIP(struct PluginPanelItem *Item,struct ArcItemInfo *Info);
int GetArcItemTAR(struct PluginPanelItem *Item,struct ArcItemInfo *Info);
char *AdjustTARFileName(char *FileName);
static int64_t Oct2Size (const char *where0, size_t digs0);

HANDLE ArcHandle;
DWORD SFXSize;
FAR_INT64 NextPosition,FileSize;
int ArcType;
enum archive_format TarArchiveFormat;
char ZipName[NM];

typedef int  (WINAPI *FARSTDMKLINK)(const char *Src,const char *Dest,DWORD Flags);

typedef void (__cdecl *MAFREE)(void *block);
typedef void * (__cdecl *MAMALLOC)(size_t size);

MAFREE MA_free;
MAMALLOC MA_malloc;

void  WINAPI _export TARGZ_SetFarInfo(const struct PluginStartupInfo *Info)
{
  MA_free=(MAFREE)Info->FSF->Reserved[1];
  MA_malloc=(MAMALLOC)Info->FSF->Reserved[0];
}

void WINAPI UnixTimeToFileTime( DWORD time, FILETIME * ft );

BOOL WINAPI _export TARGZ_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  SFXSize=0;
  if (IsTarHeader(Data,DataSize))
  {
    ArcType=TAR_FORMAT;
    return(TRUE);
  }

  if (DataSize<2)
    return(FALSE);

  if (Data[0]==0x1f && Data[1]==0x8b)
  {
    ArcType=GZ_FORMAT;
  }
  else if (Data[0]==0x1f && Data[1]==0x9d)
    ArcType=Z_FORMAT;
  else if (Data[0]=='B' && Data[1]=='Z')
    ArcType=BZ_FORMAT;
  else if (DataSize>=6 && memcmp(Data, "\xFD\x37\x7A\x58\x5A\x00", 6) == 0)
    ArcType=XZ_FORMAT;
  else
    return(FALSE);

  const char *NamePtr=(const char *)strrchr((char*)Name,'/');
  NamePtr=(NamePtr==NULL) ? Name:NamePtr+1;
  strcpy(ZipName,NamePtr);
  const char *Dot=(const char *)strrchr((char*)NamePtr,'.');

  if (Dot!=NULL)
  {
    Dot++;
    if (strcasecmp(Dot,"tgz")==0 || strcasecmp(Dot,"taz")==0)
      strcpy(&ZipName[Dot-NamePtr],"tar");
    else
      ZipName[Dot-NamePtr-1]=0;
  }

  return(TRUE);
}


BOOL WINAPI _export TARGZ_OpenArchive(const char *Name,int *Type)
{
  ArcHandle=WINPORT(CreateFile)(MB2Wide(Name).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  if (ArcHandle==INVALID_HANDLE_VALUE)
    return(FALSE);
  *Type=ArcType;

  FileSize.Part.LowPart=WINPORT(GetFileSize)(ArcHandle,(LPDWORD) &FileSize.Part.HighPart);

  NextPosition.i64=0;
  return(TRUE);
}

DWORD WINAPI _export TARGZ_GetSFXPos(void)
{
  return SFXSize;
}

int WINAPI _export TARGZ_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  if (ArcType!=TAR_FORMAT)
  {
    if (*ZipName)
    {
      switch (ArcType)
      {
		  case BZ_FORMAT: case XZ_FORMAT:
			Item->PackSize=Item->FindData.nFileSizeLow=FileSize.Part.LowPart;
			Item->PackSizeHigh=Item->FindData.nFileSizeHigh=FileSize.Part.HighPart;
			strncpy(Item->FindData.cFileName,ZipName,ARRAYSIZE(Item->FindData.cFileName)-1);
			*ZipName=0;
			return(GETARC_SUCCESS);

		  default:
		    return GetArcItemGZIP(Item,Info);
      }
    }
    else
      return(GETARC_EOF);
  }
  return GetArcItemTAR(Item,Info);
}

int GetArcItemGZIP(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  DWORD ReadSize;
  struct GZHeader
  {
    BYTE Mark[2];
    BYTE Method;
    BYTE Flags;
    DWORD FileTime;
    BYTE ExtraFlags;
    BYTE HostOS;
  } Header;

  Item->FindData.cFileName[0] = 0;

  if (!WINPORT(ReadFile)(ArcHandle,&Header,sizeof(Header),&ReadSize,NULL))
    return(GETARC_READERROR);

  Item->PackSize=FileSize.Part.LowPart;
  Item->PackSizeHigh=FileSize.Part.HighPart;

  if (ArcType==Z_FORMAT)
  {
    strcpy(Item->FindData.cFileName,ZipName);
    *ZipName=0;
    Item->FindData.nFileSizeLow=FileSize.Part.LowPart;
    Item->FindData.nFileSizeHigh=FileSize.Part.HighPart;
    return(GETARC_SUCCESS);
  }

  if (Header.Flags & 2) // skip CRC16
    WINPORT(SetFilePointer)(ArcHandle,2,NULL,FILE_CURRENT);

  if (Header.Flags & 4) // skip FEXTRA
  {
    WORD ExtraLength = 0;
    if (!WINPORT(ReadFile)(ArcHandle,&ExtraLength,sizeof(ExtraLength),&ReadSize,NULL))
      return(GETARC_READERROR);
    WINPORT(SetFilePointer)(ArcHandle,ExtraLength,NULL,FILE_CURRENT);
  }

  if (Header.Flags & 8)
    if (!WINPORT(ReadFile)(ArcHandle,Item->FindData.cFileName,sizeof(Item->FindData.cFileName),&ReadSize,NULL))
      return(GETARC_READERROR);

  if (*Item->FindData.cFileName == 0) {
    strncpy(Item->FindData.cFileName, ZipName, sizeof(Item->FindData.cFileName) );

  } else { // workaround for tar.gz archives that has original name set but without .tar extension
           // since tar archives detection relies on extension, it should be there (#173)
    const char *ZipExt = strrchr(ZipName, '.');
    if (ZipExt && strcasecmp(ZipExt, ".tar") == 0) {
        const char *OrigExt = strrchr(Item->FindData.cFileName, '.');
        if (!OrigExt || strcasecmp(OrigExt, ZipExt) != 0) {
          strncat(Item->FindData.cFileName, ZipExt, sizeof(Item->FindData.cFileName));
      }
    }
  }

  *ZipName=0;

  UnixTimeToFileTime(Header.FileTime,&Item->FindData.ftLastWriteTime);

  Info->Comment=(Header.Flags & 16)!=0;
  Info->Encrypted=(Header.Flags & 32)!=0;
  WINPORT(SetFilePointer)(ArcHandle,-4,NULL,FILE_END);

  if (!WINPORT(ReadFile)(ArcHandle,&Item->FindData.nFileSizeLow,sizeof(Item->FindData.nFileSizeLow),&ReadSize,NULL))
    return(GETARC_READERROR);

  return(GETARC_SUCCESS);
}


int GetArcItemTAR(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  TARHeader TAR_hdr;
  DWORD ReadSize;
  BOOL SkipItem=FALSE;
  char *LongName = NULL;
  char namebuf[sizeof(TAR_hdr.header.prefix) + 1 + sizeof(TAR_hdr.header.name) + 1];
  do
  {
    NextPosition.Part.LowPart=WINPORT(SetFilePointer)(ArcHandle,NextPosition.Part.LowPart,&NextPosition.Part.HighPart,FILE_BEGIN);

    if (NextPosition.i64 == (int64_t)-1 && WINPORT(GetLastError)() != NO_ERROR)
      return(GETARC_READERROR);

    if (NextPosition.i64 > FileSize.i64)
      return(GETARC_UNEXPEOF);

    if (!WINPORT(ReadFile)(ArcHandle,&TAR_hdr,sizeof(TAR_hdr),&ReadSize,NULL))
      return(GETARC_READERROR);

    if (ReadSize==0 || *TAR_hdr.header.name==0)
      return(GETARC_EOF);

//    fprintf(stderr, "TAR_hdr.header.typeflag='%c' %x size=%s\n", TAR_hdr.header.typeflag, TAR_hdr.header.typeflag, TAR_hdr.header.size);
    if (TAR_hdr.header.typeflag == GNUTYPE_LONGLINK || TAR_hdr.header.typeflag == GNUTYPE_LONGNAME)
    {
      SkipItem=TRUE;
    }
    else
    {
      // TODO: GNUTYPE_LONGLINK
      DWORD dwUnixMode = 0;
      SkipItem=FALSE;
      char *EndPos;
      if (LongName != NULL)
      {
        EndPos = AdjustTARFileName (LongName);
      }
      else
      {
        char *np = namebuf;
        if(TAR_hdr.header.prefix[0])
        {
          memcpy (np, TAR_hdr.header.prefix, sizeof(TAR_hdr.header.prefix));
          np[sizeof (TAR_hdr.header.prefix)] = '\0';
          np += strlen(np);
          *np++ = '/';
        }
        memcpy (np, TAR_hdr.header.name, sizeof(TAR_hdr.header.name));
        np[sizeof(TAR_hdr.header.name)] = '\0';
        EndPos = AdjustTARFileName(namebuf);
      }
      strncpy(Item->FindData.cFileName, EndPos, sizeof(Item->FindData.cFileName));
      Item->FindData.nFileSizeHigh=0;
      dwUnixMode = (DWORD)GetOctal(TAR_hdr.header.mode);
      switch (TAR_hdr.header.typeflag) {
        case REGTYPE: case AREGTYPE:
          dwUnixMode|= S_IFREG;
          break;

        case SYMTYPE:
          Item->FindData.dwReserved0=IO_REPARSE_TAG_SYMLINK;
          //fallthrough

        case LNKTYPE:
          dwUnixMode|= S_IFLNK;
          break;

        case CHRTYPE:
          dwUnixMode|= S_IFCHR;
          break;

        case BLKTYPE:
          dwUnixMode|= S_IFBLK;
          break;

        case FIFOTYPE:
          dwUnixMode|= S_IFIFO;
          break;

        case DIRTYPE:
          dwUnixMode|= S_IFDIR;
          break;
      }

      if ((dwUnixMode & S_IFMT) == S_IFLNK) //TAR_hdr.header.typeflag == SYMTYPE || TAR_hdr.header.typeflag == LNKTYPE
      {
        if((Item->UserData=(DWORD_PTR)MA_malloc(strlen(TAR_hdr.header.linkname)+2)) != 0)
        {
          EndPos = AdjustTARFileName (TAR_hdr.header.linkname);
          if(TAR_hdr.header.typeflag == LNKTYPE)
            *(char*)Item->UserData='/';
          strncpy((char*)Item->UserData+(TAR_hdr.header.typeflag == LNKTYPE?1:0),EndPos,strlen(TAR_hdr.header.linkname)+1);
        }
      }

      Item->FindData.dwFileAttributes=WINPORT(EvaluateAttributesA)(dwUnixMode, Item->FindData.cFileName);
      Item->FindData.dwUnixMode=dwUnixMode;

      UnixTimeToFileTime((DWORD)GetOctal(TAR_hdr.header.mtime),&Item->FindData.ftLastWriteTime);
    }

    FAR_INT64 TarItemSize;
    TarItemSize.i64 = (TAR_hdr.header.typeflag == DIRTYPE) ? 0 : // #348
			Oct2Size(TAR_hdr.header.size,sizeof(TAR_hdr.header.size));
    Item->PackSize=Item->FindData.nFileSizeLow=TarItemSize.Part.LowPart;
    Item->PackSizeHigh=Item->FindData.nFileSizeHigh=TarItemSize.Part.HighPart;

    strcpy(Info->HostOS,TarArchiveFormat==POSIX_FORMAT?"POSIX":(TarArchiveFormat==V7_FORMAT?"V7":""));
    Info->UnpVer=256+11+(TarArchiveFormat >= POSIX_FORMAT?1:0); //!!!

    FAR_INT64 PrevPosition=NextPosition;
    // for LNKTYPE - only sizeof(TAR_hdr)
    NextPosition.i64+=(int64_t)sizeof(TAR_hdr)+(TAR_hdr.header.typeflag == LNKTYPE ? int64_t(0) : TarItemSize.i64);

    if (NextPosition.i64 & int64_t(511))
      NextPosition.i64+=int64_t(512)-(NextPosition.i64 & int64_t(511));

    if (PrevPosition.i64 >= NextPosition.i64)
      return(GETARC_BROKEN);

    // TODO: GNUTYPE_LONGLINK
    if (TAR_hdr.header.typeflag == GNUTYPE_LONGNAME || TAR_hdr.header.typeflag == GNUTYPE_LONGLINK)
    {
      PrevPosition.i64+=(int64_t)sizeof(TAR_hdr);
      WINPORT(SetFilePointer) (ArcHandle,PrevPosition.Part.LowPart,&PrevPosition.Part.HighPart,FILE_BEGIN);
      // we can't have two LONGNAME records in a row without a file between them
      if (LongName != NULL)
        return GETARC_BROKEN;
      LongName = (char *)malloc(Item->PackSize);
      DWORD BytesRead;
      WINPORT(ReadFile)(ArcHandle,LongName,Item->PackSize,&BytesRead,NULL);
      if (BytesRead != Item->PackSize)
      {
        free(LongName);
        return GETARC_BROKEN;
      }
    }
  } while (SkipItem);

  if (LongName)
    free(LongName);
  return(GETARC_SUCCESS);
}


BOOL WINAPI _export TARGZ_CloseArchive(struct ArcInfo *Info)
{
  return(WINPORT(CloseHandle)(ArcHandle));
}


BOOL WINAPI _export TARGZ_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  static const char * const FmtAndExt[5][2]={
    {"TAR","tar"},
    {"GZip","gz"},
    {"Z(Unix)","z"},
    {"BZip","bz2"},
    {"XZip","xz"},
  };
  switch(Type)
  {
    case TAR_FORMAT:
    case GZ_FORMAT:
    case Z_FORMAT:
    case BZ_FORMAT:
	case XZ_FORMAT:
      strcpy(FormatName,FmtAndExt[Type][0]);
      strcpy(DefaultExt,FmtAndExt[Type][1]);
      return(TRUE);
  }
  return(FALSE);
}


BOOL WINAPI _export TARGZ_GetDefaultCommands(int Type,int Command,char *Dest)
{
   static const char * Commands[5][15]=
   {
     { // TAR_FORMAT
       "tar --force-local -xf %%A %%FSq32768",
       "tar --force-local -O -xf %%A %%fSq > %%fWq",
       "",
       "tar --delete --force-local -f %%A %%FSq32768",
       "",
       "",
       "",
       "",
       "",
       "",
       "tar --force-local -rf %%A %%FSq32768",
       "tar --force-local --remove-files -rf %%A %%FSq32768",
       "tar --force-local -rf %%A %%FSq32768",
       "tar --force-local --remove-files -rf %%A %%FSq32768",
       "*"
     },

     { // GZ_FORMAT
       "gzip -cd %%A >%%fq",
       "gzip -cd %%A >%%fq",
       "gzip -t %%A",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "gzip -c %%fq >%%A",
       "gzip %%fq",
       "gzip -c %%fq >%%A",
       "gzip %%fq",
       "*"
     },

     { // Z_FORMAT
       "gzip -cd %%A >%%fq",
       "gzip -cd %%A >%%fq",
       "gzip -t %%A",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "*"
     },

     { // BZ_FORMAT
       "bzip2 -cd %%A >%%fq",
       "bzip2 -cd %%A >%%fq",
       "bzip2 -cd %%A >/dev/null",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "bzip2 -c %%fq >%%A",
       "bzip2 %%fq",
       "bzip2 -c %%fq >%%A",
       "bzip2 %%fq",
       "*"
     },
     { // BZ_FORMAT
       "xz -cd %%A >%%fq",
       "xz -cd %%A >%%fq",
       "xz -cd %%A >/dev/null",
       "",
       "",
       "",
       "",
       "",
       "",
       "",
       "xz -c %%fq >%%A",
       "xz %%fq",
       "xz -c %%fq >%%A",
       "xz %%fq",
       "*"
     },
   };
   if (Type >= TAR_FORMAT && Type <= XZ_FORMAT && Command < (int)(ARRAYSIZE(Commands[Type])))
   {
     strcpy(Dest,Commands[Type][Command]);
     return(TRUE);
   }
   return(FALSE);
}

int IsTarHeader(const BYTE *Data,int DataSize)
{
  size_t I;
  struct posix_header *Header;

  if (DataSize<(int)sizeof(struct posix_header))
    return(FALSE);

  Header=(struct posix_header *)Data;

  if(!strcmp (Header->magic, TMAGIC))
    TarArchiveFormat = POSIX_FORMAT;
  else if(!strcmp (Header->magic, OLDGNU_MAGIC))
    TarArchiveFormat = OLDGNU_FORMAT;
  else
    TarArchiveFormat = V7_FORMAT;

  for (I=0; Header->name[I] && I < sizeof(Header->name); I++)
    if (Header->name[I] < ' ')
      return FALSE;

  //for (I=0; I < (&Header->typeflag - &Header->mode[0]); I++)
  for (I=0; I < sizeof(Header->mode); I++)
  {
    int Mode=Header->mode[I];
    if (Mode > '7' || (Mode < '0' && Mode && Mode != ' '))
      return FALSE;
  }

  for (I=0; Header->mtime[I] && I < sizeof(Header->mtime); I++)
    if (Header->mtime[I] < ' ')
      return FALSE;

  int64_t Sum=256;
  for(I=0; I <= 147; I++)
    Sum+=Data[I];

  for(I=156; I < 512; I++)
    Sum+=Data[I];

  return(Sum==GetOctal(Header->chksum));
/*
  if(lstrcmp(Header->name,"././@LongLink"))
  {
    int64_t Seconds=GetOctal(Header->mtime);
    if (Seconds < 300000000i64 || Seconds > 1500000000i64)
    {
      if(Header->typeflag != DIRTYPE && Header->typeflag != SYMTYPE)
        return(FALSE);
    }
  }
  return(TRUE);
*/
}


char *AdjustTARFileName(char *FileName)
{
  char *EndPos = FileName;
  while( *EndPos ) EndPos++;
  return FileName;
}


int64_t GetOctal(const char *Str)
{
  char *endptr;
  return _strtoi64(Str,&endptr,8);
//  return(strtoul(Str,&endptr,8));
}

static int64_t Oct2Size (const char *where0, size_t digs0)
{
  int64_t value;
  const char *where = where0;
  size_t digs = digs0;

  for (;;)
  {
    if (!digs)
    {
       return -1;
    }

    if (*where != ' ')
      break;
    where++;
    digs--;
  }

  value = 0;
  while (digs != 0 && *where >= '0' && *where <= '7')
  {
    if (((value << 3) >> 3) != value)
      goto out_of_range;
    value = (value << 3) | (*where++ - '0');
    --digs;
  }

  if (digs && *where && *where != ' ')
    return -1;

  return value;

out_of_range:
  return -1;
}

//#if _MSC_VER < 1310

// strtoq.c

/***
*strtoi64, strtoui64(nptr,endptr,ibase) - Convert ascii string to int64_t un/signed
*    int.
*
*Purpose:
*    Convert an ascii string to a 64-bit int64_t value.  The base
*    used for the caculations is supplied by the caller.  The base
*    must be in the range 0, 2-36.  If a base of 0 is supplied, the
*    ascii string must be examined to determine the base of the
*    number:
*        (a) First char = '0', second char = 'x' or 'X',
*            use base 16.
*        (b) First char = '0', use base 8
*        (c) First char in range '1' - '9', use base 10.
*
*    If the 'endptr' value is non-NULL, then strtoq/strtouq places
*    a pointer to the terminating character in this value.
*    See ANSI standard for details
*
*Entry:
*    nptr == NEAR/FAR pointer to the start of string.
*    endptr == NEAR/FAR pointer to the end of the string.
*    ibase == integer base to use for the calculations.
*
*    string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*    Good return:
*        result
*
*    Overflow return:
*        strtoi64 -- _I64_MAX or _I64_MIN
*        strtoui64 -- _UI64_MAX
*        strtoi64/strtoui64 -- errno == ERANGE
*
*    No digits or bad base return:
*        0
*        endptr = nptr*
*
*Exceptions:
*    None.
*******************************************************************************/

static uint64_t __cdecl _strtoxq (const char *nptr,const char **endptr,int ibase,int flags)
{
/* flag values */
#define FL_UNSIGNED   1       /* strtouq called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */
#undef _UI64_MAX
#define _UI64_MAX     uint64_t(0xFFFFFFFFFFFFFFFF)
#define _UI64_MAXDIV8 uint64_t(0x1FFFFFFFFFFFFFFF)
#undef _I64_MIN
#define _I64_MIN    (int64_t(-9223372036854775807) )
#undef _I64_MAX
#define _I64_MAX      int64_t(9223372036854775807)

    const char *p;
    char c;
    uint64_t number;
    unsigned digval;
    uint64_t maxval;

    p = nptr;            /* p is our scanning pointer */
    number = 0;            /* start with zero */

    c = *p++;            /* read char */
    while (c == 0x09 || c == 0x0D || c == 0x20)
        c = *p++;        /* skip whitespace */

    if (c == '-') {
        flags |= FL_NEG;    /* remember minus sign */
        c = *p++;
    }
    else if (c == '+')
        c = *p++;        /* skip sign */

    if (ibase < 0 || ibase == 1 || ibase > 36) {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr = nptr;
        return 0L;        /* return 0 */
    }
    else if (ibase == 0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != '0')
            ibase = 10;
        else if (*p == 'x' || *p == 'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 16) {
        /* we might have 0x in front of number; remove if there */
        if (c == '0' && (*p == 'x' || *p == 'X')) {
            ++p;
            c = *p++;    /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
#ifdef _MSC_VER
#if _MSC_VER >= 1310
    maxval = (uint64_t)0x1FFFFFFFFFFFFFFFi64; // hack for VC.2003 = _UI64_MAX/8 :-)
#else
    maxval = _UI64_MAX / (uint64_t)ibase;
#endif
#else
    maxval = _UI64_MAX / (uint64_t)ibase;
#endif

    for (;;) {    /* exit in middle of loop */
        /* convert c to value */
        if ( (BYTE)c >= '0' && (BYTE)c <= '9' )
            digval = c - '0';
        else if ( (BYTE)c >= 'A' && (BYTE)c <= 'Z')
            digval = c - 'A' + 10;
        else if ((BYTE)c >= 'a' && (BYTE)c <= 'z')
            digval = c - 'a' + 10;
        else
            break;
        if (digval >= (unsigned)ibase)
            break;        /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number < maxval || (number == maxval &&
        (uint64_t)digval <= _UI64_MAX % (uint64_t)ibase)) {
            /* we won't overflow, go ahead and multiply */
            number = number * (uint64_t)ibase + (uint64_t)digval;
        }
        else {
            /* we would have overflowed -- set the overflow flag */
            flags |= FL_OVERFLOW;
        }

        c = *p++;        /* read next digit */
    }

    --p;                /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT)) {
        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p = nptr;
        number = 0;        /* return 0 */
    }
    else if ( (flags & FL_OVERFLOW) ||
              ( !(flags & FL_UNSIGNED) &&
                ( ( (flags & FL_NEG) && (number > -_I64_MIN) ) ||
                  ( !(flags & FL_NEG) && (number > _I64_MAX) ) ) ) )
    {
        /* overflow or signed overflow occurred */
//        errno = ERANGE;
        if ( flags & FL_UNSIGNED )
            number = _UI64_MAX;
        else if ( flags & FL_NEG )
            number = _I64_MIN;
        else
            number = _I64_MAX;
    }
    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr = p;

    if (flags & FL_NEG)
        /* negate result if there was a neg sign */
        number = (uint64_t)(-(int64_t)number);

    return number;            /* done. */
}

int64_t __cdecl _strtoi64(const char *nptr,char **endptr,int ibase)
{
    return (int64_t) _strtoxq(nptr, (const char **)endptr, ibase, 0);
}
uint64_t __cdecl _strtoui64 (const char *nptr,char **endptr,int ibase)
{
    return _strtoxq(nptr, (const char **)endptr, ibase, FL_UNSIGNED);
}
//#endif
