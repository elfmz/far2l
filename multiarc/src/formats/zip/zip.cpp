/*
  ZIP.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <vector>
#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"

static BOOL CPToUTF8( UINT cp, LPCSTR s, LPSTR d, int dlen )
{
	if (!s || !d || dlen<=1 )
		return FALSE;

	std::vector<wchar_t> buf((1 + strlen( s )) * 2);

	int r = WINPORT(MultiByteToWideChar)( cp, 0, s, -1, &buf[0], buf.size() );
	if (r >= 0) {
		r = WINPORT(WideCharToMultiByte)( CP_UTF8, 0, &buf[0], r, d, dlen - 1, NULL, NULL );
		d[ (r >= 0) ? r : 0 ] = 0;
	} else
		d[0] = 0;
	return TRUE;
}

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
static ULARGE_INTEGER SFXSize,NextPosition,FileSize;
static int ArcComment,FirstRecord;
static bool bTruncated;

struct ZipHeader
{
  DWORD Signature;
  WORD VerToExtract;
  WORD BitFlag;
  WORD Method;
  WORD LastModTime;
  WORD LastModDate;
  DWORD Crc32;
  DWORD SizeCompr;
  DWORD SizeUncompr;
  WORD FileNameLen;
  WORD ExtraFieldLen;
  // FileName[];
  // ExtraField[];
};

const size_t MIN_HEADER_LEN=sizeof(ZipHeader);

inline BOOL IsValidHeader(const unsigned char *Data, const unsigned char *DataEnd)
{
  ZipHeader* pHdr=(ZipHeader*)Data;
  //const WORD Zip64=45;
  return (0x04034b50==pHdr->Signature
    && (pHdr->Method<20  || pHdr->Method==98 || pHdr->Method == 99)
    && (pHdr->VerToExtract&0x00FF) < 0xFF //version is in the low byte
    && Data+MIN_HEADER_LEN+pHdr->FileNameLen+pHdr->ExtraFieldLen<DataEnd);
}

ULONGLONG GetFilePosition(HANDLE Handle)
{
  ULARGE_INTEGER ul;
  ul.QuadPart=0;
  ul.u.LowPart=WINPORT(SetFilePointer)(Handle, 0, (PLONG)&ul.u.HighPart, FILE_CURRENT);
  return ul.QuadPart;
}

BOOL WINAPI _export ZIP_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  if (DataSize>=4 && Data[0]=='P' && Data[1]=='K' && Data[2]==5 && Data[3]==6)
  {
    SFXSize.QuadPart=0;
    return(TRUE);
  }
  if (DataSize<(int)MIN_HEADER_LEN) return FALSE;
  const unsigned char *MaxData=Data+DataSize-MIN_HEADER_LEN;
  const unsigned char *DataEnd=Data+DataSize;
  for (const unsigned char *CurData=Data; CurData<MaxData; CurData++)
  {
    if (IsValidHeader(CurData, DataEnd))
    {
      SFXSize.QuadPart=(DWORD)(CurData-Data);
      return(TRUE);
    }
  }
  return(FALSE);
}

BOOL WINAPI _export ZIP_OpenArchive(const char *Name,int *Type)
{
  ArcHandle=WINPORT(CreateFile)(MB2Wide(Name).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  if (ArcHandle==INVALID_HANDLE_VALUE)
    return(FALSE);

  *Type=0;

  ArcComment=FALSE;
  FirstRecord=TRUE;

  FileSize.u.LowPart=WINPORT(GetFileSize)(ArcHandle,&FileSize.u.HighPart);

  char ReadBuf[1024];
  DWORD ReadSize;
  int Buf;
  bool bFound=false, bLast=false;

  if (FileSize.QuadPart<sizeof(ReadBuf)-18)
  {
    WINPORT(SetFilePointer)(ArcHandle,0,NULL,FILE_BEGIN);
    bLast=true;
  }
  else
    WINPORT(SetFilePointer)(ArcHandle,-((signed)(sizeof(ReadBuf)-18)),NULL,FILE_END);

  for (Buf=0; Buf<64 && !bFound; Buf++)
  {
    WINPORT(ReadFile)(ArcHandle,ReadBuf,sizeof(ReadBuf),&ReadSize,NULL);
    for (int I=ReadSize-4; I>=0; I--)
    {
      if (ReadBuf[I]==0x50 && ReadBuf[I+1]==0x4b && ReadBuf[I+2]==0x05 &&
          ReadBuf[I+3]==0x06)
      {
        WINPORT(SetFilePointer)(ArcHandle,I+16-ReadSize,NULL,FILE_CURRENT);
        WINPORT(ReadFile)(ArcHandle,&NextPosition.u.LowPart,sizeof(NextPosition.u.LowPart),&ReadSize,NULL);
        NextPosition.u.HighPart=0;
        bFound=true;
        break;
      }
    }
    if (bFound || bLast)
      break;

    if (WINPORT(SetFilePointer)(ArcHandle,-((signed)(sizeof(ReadBuf)-4))-((signed)(ReadSize)),NULL,FILE_CURRENT) == INVALID_SET_FILE_POINTER
        && WINPORT(GetLastError)() != NO_ERROR)
    {
      WINPORT(SetFilePointer)(ArcHandle,0,NULL,FILE_BEGIN);
      bLast=true;
    }
  }

  bTruncated=!bFound;
  if (bTruncated)
    NextPosition.QuadPart=SFXSize.QuadPart;
  return(TRUE);
}


int WINAPI _export ZIP_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  struct ZipHd1
  {
    DWORD Mark;
    BYTE UnpVer;
    BYTE UnpOS;
    WORD Flags;
    WORD Method;
    DWORD ftime;
    DWORD CRC;
    DWORD PackSize;
    DWORD UnpSize;
    WORD NameLen;
    WORD AddLen;
  } ZipHd1;

  struct ZipHd2
  {
    DWORD Mark;
    BYTE PackVer;
    BYTE PackOS;
    BYTE UnpVer;
    BYTE UnpOS;
    WORD Flags;
    WORD Method;
    DWORD ftime;
    DWORD CRC;
    DWORD PackSize;
    DWORD UnpSize;
    WORD NameLen;
    WORD AddLen;
    WORD CommLen;
    WORD DiskNum;
    WORD ZIPAttr;
    DWORD Attr;
    DWORD Offset;
  } ZipHeader;

  DWORD ReadSize;

  NextPosition.u.LowPart=WINPORT(SetFilePointer)(ArcHandle,NextPosition.u.LowPart,(PLONG)&NextPosition.u.HighPart,FILE_BEGIN);
  if (NextPosition.u.LowPart == INVALID_SET_FILE_POINTER && WINPORT(GetLastError)() != NO_ERROR)
    return(GETARC_READERROR);
  if (NextPosition.QuadPart>FileSize.QuadPart)
    return(GETARC_UNEXPEOF);
  if (bTruncated)
  {
    if (!WINPORT(ReadFile)(ArcHandle,&ZipHd1,sizeof(ZipHd1),&ReadSize,NULL))
      return(GETARC_READERROR);
    memset(&ZipHeader,0,sizeof(ZipHeader));
    ZipHeader.Mark=ZipHd1.Mark;
    ZipHeader.UnpVer=ZipHd1.UnpVer;
    ZipHeader.UnpOS=ZipHd1.UnpOS;
    ZipHeader.Flags=ZipHd1.Flags;
    ZipHeader.Method=ZipHd1.Method;
    ZipHeader.ftime=ZipHd1.ftime;
    ZipHeader.PackSize=ZipHd1.PackSize;
    ZipHeader.UnpSize=ZipHd1.UnpSize;
    ZipHeader.NameLen=ZipHd1.NameLen;
    ZipHeader.AddLen=ZipHd1.AddLen;
  }
  else
  {
    if (!WINPORT(ReadFile)(ArcHandle,&ZipHeader,sizeof(ZipHeader),&ReadSize,NULL))
      return(GETARC_READERROR);
    if (ZipHeader.Mark!=0x02014b50 && ZipHeader.Mark!=0x06054b50)
    {
      if (FirstRecord)
      {
        if (SFXSize.QuadPart>0)
        {
          NextPosition.QuadPart+=SFXSize.QuadPart;
          WINPORT(SetFilePointer)(ArcHandle,NextPosition.u.LowPart,(PLONG)&NextPosition.u.HighPart,FILE_BEGIN);
          if (!WINPORT(ReadFile)(ArcHandle,&ZipHeader,sizeof(ZipHeader),&ReadSize,NULL))
            return(GETARC_READERROR);
        }
        if (ZipHeader.Mark!=0x02014b50 && ZipHeader.Mark!=0x06054b50)
        {
          bTruncated=true;
          NextPosition.QuadPart=SFXSize.QuadPart;
          return(ZIP_GetArcItem(Item,Info));
        }
      }
      else
        return(GETARC_UNEXPEOF);
    }
  }

  FirstRecord=FALSE;

  if (ReadSize==0 || ZipHeader.Mark==0x06054b50 ||
      (bTruncated && ZipHeader.Mark==0x02014b50))
  {
    if (!bTruncated && *(WORD *)((char *)&ZipHeader+20)!=0)
      ArcComment=TRUE;
    return(GETARC_EOF);
  }
  
  DWORD SizeToRead=(ZipHeader.NameLen<ARRAYSIZE(Item->FindData.cFileName)-1) ? ZipHeader.NameLen : ARRAYSIZE(Item->FindData.cFileName)-1;
  if (!WINPORT(ReadFile)(ArcHandle,Item->FindData.cFileName,SizeToRead,&ReadSize,NULL) ||
      ReadSize!=SizeToRead)
    return(GETARC_READERROR);
    
  Item->FindData.cFileName[ARRAYSIZE(Item->FindData.cFileName)-1]=0;

  char *EndPos = Item->FindData.cFileName;
  while( *EndPos ) EndPos++;

  Item->FindData.nFileSizeHigh=0;
  Item->FindData.nFileSizeLow=ZipHeader.UnpSize;
  Item->PackSizeHigh=0;
  Item->PackSize=ZipHeader.PackSize;
  Item->CRC32=ZipHeader.CRC;
  FILETIME lft;
  WINPORT(DosDateTimeToFileTime)(HIWORD(ZipHeader.ftime),LOWORD(ZipHeader.ftime),&lft);
  WINPORT(LocalFileTimeToFileTime)(&lft,&Item->FindData.ftLastWriteTime);
  if (ZipHeader.Flags & 1)
    Info->Encrypted=TRUE;
  if (ZipHeader.CommLen > 0)
    Info->Comment=TRUE;
  static const char *ZipOS[]={"DOS","Amiga","VAX/VMS","Unix","VM/CMS","Atari ST",
                        "OS/2","Mac-OS","Z-System","CP/M","TOPS-20",
                        "Win32","SMS/QDOS","Acorn RISC OS","Win32 VFAT","MVS",
                        "BeOS","Tandem"};
  if (ZipHeader.PackOS<ARRAYSIZE(ZipOS))
    strncpy(Info->HostOS,ZipOS[ZipHeader.PackOS],ARRAYSIZE(Info->HostOS)-1);

//  if (ZipHeader.PackOS==11 && ZipHeader.PackVer>20 && ZipHeader.PackVer<25)
  if (ZipHeader.Flags&0x800) { // Bit 11 - language encoding flag (EFS) - means filename&comment fields are UTF8
  } else if (ZipHeader.PackOS==11 && ZipHeader.PackVer>20 && ZipHeader.PackVer<25)
    CPToUTF8(CP_ACP, Item->FindData.cFileName,Item->FindData.cFileName, ARRAYSIZE(Item->FindData.cFileName));
  else if (ZipHeader.PackOS==11 || ZipHeader.PackOS==0)
    CPToUTF8(CP_OEMCP, Item->FindData.cFileName, Item->FindData.cFileName, ARRAYSIZE(Item->FindData.cFileName));

  Info->UnpVer=(ZipHeader.UnpVer/10)*256+(ZipHeader.UnpVer%10);
  Info->DictSize=32;
  
  if ((ZipHeader.PackOS==3 || ZipHeader.PackOS==7) && (ZipHeader.Attr&0xffff0000)!=0) {
	Item->FindData.dwUnixMode = ZipHeader.Attr>>16;
	Item->FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(Item->FindData.dwUnixMode, Item->FindData.cFileName);
  } else {
	Item->FindData.dwUnixMode = 0;
	Item->FindData.dwFileAttributes = ZipHeader.Attr & 0x3f;	  
  }

  memset(Info->Description, 0, sizeof(Info->Description));

  // Search for extra block
  ULARGE_INTEGER ExtraFieldEnd;

  for( ExtraFieldEnd.QuadPart = GetFilePosition(ArcHandle) + ZipHeader.AddLen;
       ExtraFieldEnd.QuadPart > GetFilePosition(ArcHandle); )
  {
    struct ExtraBlockHeader
    {
      WORD Type;
      WORD Length;
    }
    BlockHead;

    if (!WINPORT(ReadFile)(ArcHandle, &BlockHead, sizeof(BlockHead), &ReadSize,NULL)
            || ReadSize!=sizeof(BlockHead) )
      return(GETARC_READERROR);

//	fprintf(stderr, "BlockHead.Type=0x%x\n", BlockHead.Type);
    if (0xA==BlockHead.Type) // NTFS Header ID
    {
      WINPORT(SetFilePointer)(ArcHandle, 4, NULL, FILE_CURRENT); // Skip the reserved 4 bytes

      ULARGE_INTEGER NTFSExtraBlockEnd;
      // Search for file times attribute
      for( NTFSExtraBlockEnd.QuadPart = GetFilePosition(ArcHandle) - 4 + BlockHead.Length;
           NTFSExtraBlockEnd.QuadPart > GetFilePosition(ArcHandle);
      )
      {
        struct NTFSAttributeHeader
        {
          WORD Tag;
          WORD Length;
        }
        AttrHead;

        if (!WINPORT(ReadFile)(ArcHandle, &AttrHead, sizeof(AttrHead), &ReadSize,NULL)
                || ReadSize!=sizeof(AttrHead) )
          return(GETARC_READERROR);

        if (1!=AttrHead.Tag) // File times attribute tag
          // Move to attribute end
          WINPORT(SetFilePointer)(ArcHandle, AttrHead.Length, NULL, FILE_CURRENT);
        else
        { // Read file times
          struct TimesAttribute
          {
            FILETIME Modification;
            FILETIME Access;
            FILETIME Creation;
          }
          Times;

          if (!WINPORT(ReadFile)(ArcHandle, &Times, sizeof(Times), &ReadSize,NULL)
                  || ReadSize!=sizeof(Times) )
            return(GETARC_READERROR);

          Item->FindData.ftLastWriteTime = Times.Modification;
          Item->FindData.ftLastAccessTime = Times.Access;
          Item->FindData.ftCreationTime = Times.Creation;
        }
      }
    }
    else if (0x1==BlockHead.Type) // ZIP64
    {
     struct ZIP64Descriptor
     {
       ULARGE_INTEGER OriginalSize;             //    8 bytes               Original uncompressed file size
       ULARGE_INTEGER CompressedSize;           //    8 bytes               Size of compressed data
       ULARGE_INTEGER RelativeHeaderOffset;     //    8 bytes               Offset of local header record
       DWORD          DiskStartNumber;          //    4 bytes               Number of the disk on which this file starts
     }
     ZIP64;

     if (!WINPORT(ReadFile)(ArcHandle, &ZIP64, BlockHead.Length, &ReadSize,NULL)
             || ReadSize!=BlockHead.Length )
       return(GETARC_READERROR);

     if (BlockHead.Length>=4)
     {
       Item->FindData.nFileSizeHigh=ZIP64.OriginalSize.u.HighPart;
       Item->FindData.nFileSizeLow=ZIP64.OriginalSize.u.LowPart;
     }
     if (BlockHead.Length>=8)
     {
       Item->PackSizeHigh=ZIP64.CompressedSize.u.HighPart;
       Item->PackSize=ZIP64.CompressedSize.u.LowPart;
     }
    }
    else if ((0x7075==BlockHead.Type || 0x6375==BlockHead.Type) // Unicode Path Extra Field || Unicode Comment Extra Field
		&& BlockHead.Length > sizeof(uint8_t) + sizeof(uint32_t))
	{
		uint8_t version = 0;
		uint32_t strcrc = 0;
		std::vector<char> strbuf(BlockHead.Length - sizeof(uint32_t));
		if (!WINPORT(ReadFile)(ArcHandle, &version, sizeof(version), &ReadSize, NULL) || ReadSize != sizeof(version) )
            return(GETARC_READERROR);
		if (!WINPORT(ReadFile)(ArcHandle, &strcrc, sizeof(strcrc), &ReadSize, NULL) || ReadSize != sizeof(strcrc) )
            return(GETARC_READERROR);
		if (!WINPORT(ReadFile)(ArcHandle, &strbuf[0], strbuf.size()-1, &ReadSize, NULL) || ReadSize != strbuf.size()-1 )
            return(GETARC_READERROR);
		if (version!=1)
			fprintf(stderr, "ZIP: Unicode Extra Field 0x%x unknown version %u\n", BlockHead.Type, version);
		
		if (0x7075==BlockHead.Type) {
			strncpy(Item->FindData.cFileName, &strbuf[0], ARRAYSIZE(Item->FindData.cFileName) - 1);
		} else {
			strncpy(Info->Description, &strbuf[0], ARRAYSIZE(Info->Description) - 1);
		}
	}
	else // Move to extra block end
      WINPORT(SetFilePointer)(ArcHandle, BlockHead.Length, NULL, FILE_CURRENT);
  }
  // ZipHeader.AddLen is more reliable than the sum of all BlockHead.Length
  WINPORT(SetFilePointer)(ArcHandle, ExtraFieldEnd.u.LowPart, (PLONG)&ExtraFieldEnd.u.HighPart, FILE_BEGIN);
// End of NTFS file times support

// Read the in-archive file comment if any
  if (ZipHeader.CommLen>0)
  {
	  ReadSize = 0;
	  if (!Info->Description[0]) //we could already get UTF-8 description
	  {
		DWORD SizeToRead = (ZipHeader.CommLen>255) ? 255 : ZipHeader.CommLen;

		if (!WINPORT(ReadFile)(ArcHandle, Info->Description, SizeToRead, &ReadSize, NULL)
            || ReadSize != SizeToRead ) return(GETARC_READERROR);
	  }
	// Skip comment tail
    WINPORT(SetFilePointer)(ArcHandle, ZipHeader.CommLen - ReadSize, NULL, FILE_CURRENT);
  }

  ULARGE_INTEGER SeekLen;
  if (bTruncated)
  {
    SeekLen.u.HighPart=Item->PackSizeHigh;
    SeekLen.u.LowPart=Item->PackSize;
  }
  else
    SeekLen.QuadPart=0;
  WINPORT(SetFilePointer)(ArcHandle,SeekLen.u.LowPart,(PLONG)&SeekLen.u.HighPart,FILE_CURRENT);
  NextPosition.QuadPart=GetFilePosition(ArcHandle);

  return(GETARC_SUCCESS);
}


BOOL WINAPI _export ZIP_CloseArchive(struct ArcInfo *Info)
{
  if(Info)
  {
    Info->SFXSize=(int)SFXSize.QuadPart;
    Info->Comment=ArcComment;
  }
  return(WINPORT(CloseHandle)(ArcHandle));
}

DWORD WINAPI _export ZIP_GetSFXPos(void)
{
  return (DWORD)SFXSize.QuadPart;
}

BOOL WINAPI _export ZIP_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  if (Type==0)
  {
    strcpy(FormatName,"ZIP");
    strcpy(DefaultExt,"zip");
    return(TRUE);
  }
  return(FALSE);
}


BOOL WINAPI _export ZIP_GetDefaultCommands(int Type,int Command,char *Dest)
{
  if (Type==0)
  {
#if 0
    // Console PKZIP 4.0/Win32 commands
    static const char *Commands[]={
    /*Extract               */"pkzipc -ext -dir -over=all -nozip -mask=none -times=mod {-pass=%%P} %%A @%%LNMA",
    /*Extract without paths */"pkzipc -ext -over=all -nozip -mask=none -times=mod {-pass=%%P} %%A @%%LNMA",
    /*Test                  */"pkzipc -test=all -nozip {-pass=%%P} %%A",
    /*Delete                */"pkzipc -delete -nozip {-temp=%%W} %%A @%%LNMA",
    /*Comment archive       */"pkzipc -hea -nozip {-temp=%%W} %%A",
    /*Comment files         */"pkzipc -com=all -nozip {-temp=%%W} %%A",
    /*Convert to SFX        */"pkzipc -sfx -nozip %%A",
    /*Lock archive          */"",
    /*Protect archive       */"",
    /*Recover archive       */"%comspec% /c echo.|pkzipc -fix -nozip %%A",
    /*Add files             */"pkzipc -add -attr=all -nozip {-pass=%%P} {-temp=%%W} %%A @%%LNMA",
    /*Move files            */"pkzipc -add -move -attr=all -nozip {-pass=%%P} {-temp=%%W} %%A @%%LNMA",
    /*Add files and folders */"pkzipc -add -attr=all -dir -nozip {-pass=%%P} {-temp=%%W} %%A @%%LNMA",
    /*Move files and folders*/"pkzipc -add -move -attr=all -dir -nozip {-pass=%%P} {-temp=%%W} %%A @%%LNMA",
    /*"All files" mask      */"*.*"
    };
#else
    // Linux zip/unzip 
    static const char *Commands[]={
    /*Extract               */"unzip -o {-P %%P} %%A %%FMq4096",
    /*Extract without paths */"unzip -j -o {-P %%P} %%A %%FMq4096",
    /*Test                  */"unzip -t {-P %%P} %%A",
    /*Delete                */"zip -d {-P %%P} %%A %%FMq4096",
    /*Comment archive       */"",
    /*Comment files         */"",
    /*Convert to SFX        */"",
    /*Lock archive          */"",
    /*Protect archive       */"",
    /*Recover archive       */"",
    /*Add files             */"zip {-P %%P} %%A %%Fq4096",
    /*Move files            */"zip -m {-P %%P} %%A %%Fq4096",
    /*Add files and folders */"zip -r {-P %%P} %%A %%Fq4096",
    /*Move files and folders*/"zip -r -m {-P %%P} %%A %%Fq4096",
    /*"All files" mask      */"*"
    };
#endif
    if (Command<(int)(ARRAYSIZE(Commands)))
    {
      strcpy(Dest,Commands[Command]);
      return(TRUE);
    }
  }
  return(FALSE);
}
