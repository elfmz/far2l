/*
fileattr.cpp

Работа с атрибутами файлов
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "fileattr.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "fileowner.hpp"

static int SetFileEncryption(const wchar_t *Name,int State);
static int SetFileCompression(const wchar_t *Name,int State);
static bool SetFileSparse(const wchar_t *Name,bool State);


int ESetFileAttributes(const wchar_t *Name,DWORD Attr,int SkipMode)
{
//_SVS(SysLog(L"Attr=0x%08X",Attr));
	if (Attr&FILE_ATTRIBUTE_DIRECTORY && Attr&FILE_ATTRIBUTE_TEMPORARY)
		Attr&=~FILE_ATTRIBUTE_TEMPORARY;

	while (!apiSetFileAttributes(Name,Attr))
	{
		int Code;

		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
			             MSG(MSetAttrCannotFor),Name,MSG(MHRetry),MSG(MHSkip),MSG(MHSkipAll),MSG(MHCancel));

		switch (Code)
		{
			case -2:
			case -1:
			case 1:
				return SETATTR_RET_SKIP;
			case 2:
				return SETATTR_RET_SKIPALL;
			case 3:
				return SETATTR_RET_ERROR;
		}
	}

	return SETATTR_RET_OK;
}


int ESetFileCompression(const wchar_t *Name,int State,DWORD FileAttr,int SkipMode)
{
	return SETATTR_RET_ERROR;
}

int ESetFileEncryption(const wchar_t *Name,int State,DWORD FileAttr,int SkipMode,int Silent)
{
	return SETATTR_RET_ERROR;
}


int ESetFileTime(const wchar_t *Name,FILETIME *LastWriteTime,FILETIME *CreationTime,FILETIME *LastAccessTime,FILETIME *ChangeTime,DWORD FileAttr,int SkipMode)
{
	if (!LastWriteTime && !CreationTime && !LastAccessTime && !ChangeTime)
		return SETATTR_RET_OK;

	for(;;)
	{
		if (FileAttr & FILE_ATTRIBUTE_READONLY)
			apiSetFileAttributes(Name,FileAttr & ~FILE_ATTRIBUTE_READONLY);

		bool SetTime=false;
		DWORD LastError=ERROR_SUCCESS;
		File file;
		if (!file.Open(Name,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
		                           nullptr,OPEN_EXISTING,
		                           FILE_FLAG_OPEN_REPARSE_POINT))
		{
			LastError=WINPORT(GetLastError)();
		}
		else
		{
			SetTime=file.SetTime(CreationTime,LastAccessTime,LastWriteTime, ChangeTime);
			LastError=WINPORT(GetLastError)();
			file.Close();

			if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && LastError==ERROR_NOT_SUPPORTED)   // FIX: Mantis#223
			{
				string strDriveRoot;
				GetPathRoot(Name, strDriveRoot);

				if (WINPORT(GetDriveType)(strDriveRoot)==DRIVE_REMOTE) break;
			}
		}

		if (FileAttr & FILE_ATTRIBUTE_READONLY)
			apiSetFileAttributes(Name,FileAttr);

		WINPORT(SetLastError)(LastError);

		if (SetTime)
			break;

		int Code;

		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
			             MSG(MSetAttrTimeCannotFor),Name,MSG(MHRetry), //BUGBUG
			             MSG(MHSkip),MSG(MHSkipAll),MSG(MHCancel));

		switch (Code)
		{
			case -2:
			case -1:
			case 3:
				return SETATTR_RET_ERROR;
			case 2:
				return SETATTR_RET_SKIPALL;
			case 1:
				return SETATTR_RET_SKIP;
		}
	}

	return SETATTR_RET_OK;
}

int ESetFileSparse(const wchar_t *Name,bool State,DWORD FileAttr,int SkipMode)
{
	return SETATTR_RET_ERROR;
}

int ESetFileOwner(LPCWSTR Name,LPCWSTR Owner,int SkipMode)
{
	//todo: system("chown Owner Name");
	return SETATTR_RET_ERROR;
}
