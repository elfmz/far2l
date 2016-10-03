/*
  7z.cpp

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
  Copyrigth (c) 2016 elfmz
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <stddef.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <pluginold.hpp>
using namespace oldfar;
#include "fmt.hpp"
#include "./C/7z.h"
#include "./C/7zAlloc.h"
#include "./C/7zFile.h"
#include "./C/7zCrc.h"

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




/////////////////////////////


static ISzAlloc g_alloc_imp = { SzAlloc, SzFree };

#define SzArEx_GetFilePackedSize(p, i) ((p)->db.PackPositions ? (p)->db.PackPositions[(i) + 1] - (p)->db.PackPositions[i] : 0)

class Traverser
{
	CSzArEx _db;
	CFileInStream _file_stream;
	CLookToRead _look_stream;
	std::wstring _tmp_str;
	
	ISzAlloc _alloc_imp, _alloc_temp_imp;
	UInt16 *_temp;
	size_t _temp_size;
	int _index;
	bool _opened, _valid;
	
public:
	Traverser(const char *path) : _temp(NULL), _opened(false), _valid(false), _index(0), _temp_size(0)
	{
		if (InFile_Open(&_file_stream.file, path))
			return;
		_opened = true;
		
		_alloc_imp.Alloc = SzAlloc;
		_alloc_imp.Free = SzFree;
			
		_alloc_temp_imp.Alloc = SzAllocTemp;
		_alloc_temp_imp.Free = SzFreeTemp;

		FileInStream_CreateVTable(&_file_stream);
		LookToRead_CreateVTable(&_look_stream, False);
		
		_look_stream.realStream = &_file_stream.s;
		LookToRead_Init(&_look_stream);
		
		static volatile int i = (CrcGenerateTable(), 0);
		
		SzArEx_Init(&_db);
			
		if (SzArEx_Open(&_db, &_look_stream.s, &_alloc_imp, &_alloc_temp_imp)==SZ_OK) 
			_valid = true;
	}
	
	~Traverser()
	{
		if (_opened) {
			SzArEx_Free(&_db, &g_alloc_imp);
			SzFree(NULL, _temp);
			File_Close(&_file_stream.file);
		}
	}
	
	bool Valid() const
	{
		return _valid;
	}
	
	int Next(struct PluginPanelItem *Item, struct ArcItemInfo *Info)
	{
		if (!_valid)
			return GETARC_READERROR;
			
		if (_index >= _db.NumFiles )
			return GETARC_EOF;
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        
        unsigned is_dir = SzArEx_IsDir(&_db, _index);
        size_t len = SzArEx_GetFileNameUtf16(&_db, _index, NULL);
		if (len > _temp_size)
		{
			SzFree(NULL, _temp);
			_temp_size = len + 16;
			_temp = (UInt16 *)SzAlloc(NULL, _temp_size * sizeof(_temp[0]));
			if (!_temp) {
				_temp_size = 0;
				return GETARC_READERROR;
			}
		}
		
        SzArEx_GetFileNameUtf16(&_db, _index, _temp);
		_tmp_str.clear();
		_tmp_str.reserve(len);
		for (size_t i = 0; i < len; ++i) {
			_tmp_str+= (wchar_t)(uint16_t)_temp[i];
		}
		const std::string &name = StrWide2MB(_tmp_str);
		
		
		DWORD attribs = SzBitWithVals_Check(&_db.Attribs, _index) ? _db.Attribs.Vals[_index] : 0;
		DWORD crc32 = SzBitWithVals_Check(&_db.CRCs, _index) ? _db.CRCs.Vals[_index] : 0;
		UInt64 file_size = SzArEx_GetFileSize(&_db, _index);
		UInt64 packed_size = SzArEx_GetFilePackedSize(&_db, _index);
		
		FILETIME ftm = {}, ftc = {};
		if (SzBitWithVals_Check(&_db.MTime, _index)) {
			memcpy(&ftm, &_db.MTime.Vals[_index], sizeof(ftm));
		}
		if (SzBitWithVals_Check(&_db.CTime, _index)) {
			memcpy(&ftc, &_db.CTime.Vals[_index], sizeof(ftc));
		}
		
		++_index;
		
		attribs&=~ (FILE_ATTRIBUTE_BROKEN | FILE_ATTRIBUTE_EXECUTABLE);
		strncpy(Item->FindData.cFileName, name.c_str(), ARRAYSIZE(Item->FindData.cFileName)-1);
		Item->FindData.dwFileAttributes = attribs;
		Item->FindData.dwUnixMode = is_dir ? 0755 : 0644;
		Item->FindData.nFileSizeLow = file_size & 0xffffffff;
		Item->FindData.nFileSizeHigh = (file_size >> 32) & 0xffffffff;
		Item->PackSizeHigh = packed_size & 0xffffffff;
		Item->PackSize = (packed_size >> 32) & 0xffffffff;
		Item->CRC32 = crc32;
		
		Item->FindData.ftLastWriteTime = ftm;
		Item->FindData.ftCreationTime = ftc;
		

		Info->Solid = 0;
		Info->Comment = 0;
		Info->Encrypted = 0;
		Info->DictSize = 0;
		Info->UnpVer = 0;		
		
		return GETARC_SUCCESS;
	}
};

///////////////////////////////////


static Traverser *s_selected_traverser = NULL;
static const unsigned char s_magic_of_7z[] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C, 0x00};//last byte actually k7zMajorVersion

BOOL WINAPI _export SEVENZ_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
	if (DataSize < sizeof(s_magic_of_7z) || memcmp(Data, s_magic_of_7z, sizeof(s_magic_of_7z))!=0) {
		return FALSE;
	}

	return TRUE;
}


BOOL WINAPI _export SEVENZ_OpenArchive(const char *Name,int *Type)
{
	Traverser *t = new Traverser(Name);
	if (!t->Valid()) {
		return FALSE;
		delete t;
	}
	
	delete s_selected_traverser;
	s_selected_traverser = t;
	return TRUE;
}



int WINAPI _export SEVENZ_GetArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info)
{
	if (!s_selected_traverser)
		return GETARC_READERROR;
		
	return s_selected_traverser->Next(Item, Info);
}


BOOL WINAPI _export SEVENZ_CloseArchive(struct ArcInfo *Info)
{
	if (!s_selected_traverser)
		return FALSE;
		
	delete s_selected_traverser;
	s_selected_traverser = NULL;
	return TRUE;
}



BOOL WINAPI _export SEVENZ_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  if (Type==0)
  {
    strcpy(FormatName,"7Z");
    strcpy(DefaultExt,"7z");
    return TRUE;
  }
  return FALSE;
}



BOOL WINAPI _export SEVENZ_GetDefaultCommands(int Type,int Command,char *Dest)
{
  if (Type==0)
  {
    static const char *Commands[]={
    /*Extract               */"^7z x %%A %%FMq*4096",
    /*Extract without paths */"^7z e %%A %%FMq*4096",
    /*Test                  */"^7z t %%A",
    /*Delete                */"7z d {-p%%P} %%A @%%LN",
    /*Comment archive       */"",
    /*Comment files         */"",
    /*Convert to SFX        */"",
    /*Lock archive          */"",
    /*Protect archive       */"",
    /*Recover archive       */"",
    /*Add files             */"7z a -y {-p%%P} %%A @%%LN",
    /*Move files            */"",
    /*Add files and folders */"7z a -y -r {-p%%P} %%A @%%LN",
    /*Move files and folders*/"",
    /*"All files" mask      */"*"
    };
    if (Command<(int)(ARRAYSIZE(Commands)))
    {
      strcpy(Dest,Commands[Command]);
      return(TRUE);
    }
  }
  return(FALSE);
}
