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


#include "fileattr.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "pathmix.hpp"
#include "fileowner.hpp"

int ESetFileMode(const wchar_t *Name, DWORD Mode, int SkipMode)
{
//_SVS(SysLog(L"Attr=0x%08X",Attr));
	fprintf(stderr, "ESetFileMode('%ls', 0x%x)\n", Name, Mode);
	
	const std::string &mb_name = Wide2MB(Name);
	
	for (;;) {
		if (sdc_chmod(mb_name.c_str(), Mode)==0) break;
		
		int Code;

		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,
				Msg::SetAttrCannotFor,Name,Msg::HRetry,Msg::HSkip,Msg::HSkipAll,Msg::HCancel);

		switch (Code) {
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


int ESetFileTime(const wchar_t *Name, FILETIME *AccessTime, FILETIME *ModifyTime, DWORD FileAttr,int SkipMode)
{
	fprintf(stderr, "ESetFileTime('%ls', %p, %p)\n", Name, AccessTime, ModifyTime );

	if (!AccessTime && !ModifyTime)
		return SETATTR_RET_OK;		

	const std::string &mb_name = Wide2MB(Name);
	struct stat s = {};
	if (sdc_stat(mb_name.c_str(), &s)!=0) memset(&s, 0, sizeof(s));

	if (AccessTime) WINPORT(FileTime_Win32ToUnix)(AccessTime, &s.st_atim);
	if (ModifyTime) WINPORT(FileTime_Win32ToUnix)(ModifyTime, &s.st_mtim);

	for(;;) {
		struct timeval times[2] = { {s.st_atim.tv_sec, (int)(s.st_atim.tv_nsec/1000)}, 
									{s.st_mtim.tv_sec, (int)(s.st_mtim.tv_nsec/1000)} };
		
		if (sdc_utimes(mb_name.c_str(), times)==0) break;
		
		int Code;

		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,
			             Msg::SetAttrTimeCannotFor,Name,Msg::HRetry, //BUGBUG
			             Msg::HSkip,Msg::HSkipAll,Msg::HCancel);

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

int ESetFileOwner(LPCWSTR Name,LPCWSTR Owner,int SkipMode)
{
	fprintf(stderr, "ESetFileOwner('%ls', '%ls')\n", Name, Owner);
	int Ret=SETATTR_RET_OK;
	while (!SetOwner(Name,Owner))
	{
		int Code;
		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,Msg::SetAttrOwnerCannotFor,Name,Msg::HRetry,Msg::HSkip,Msg::HSkipAll,Msg::HCancel);

		if (Code==1 || Code<0)
		{
			Ret=SETATTR_RET_SKIP;
			break;
		}
		else if (Code==2)
		{
			Ret=SETATTR_RET_SKIPALL;
			break;
		}
		else if (Code==3)
		{
			Ret=SETATTR_RET_ERROR;
			break;
		}
	}
	return Ret;
}

int ESetFileGroup(LPCWSTR Name,LPCWSTR Group,int SkipMode)
{
	fprintf(stderr, "ESetFileGroup('%ls', '%ls')\n", Name, Group);
	int Ret=SETATTR_RET_OK;
	while (!SetGroup(Name, Group))
	{
		int Code;
		if (SkipMode!=-1)
			Code=SkipMode;
		else
			Code=Message(MSG_WARNING|MSG_ERRORTYPE,4,Msg::Error,Msg::SetAttrGroupCannotFor,Name,Msg::HRetry,Msg::HSkip,Msg::HSkipAll,Msg::HCancel);

		if (Code==1 || Code<0)
		{
			Ret=SETATTR_RET_SKIP;
			break;
		}
		else if (Code==2)
		{
			Ret=SETATTR_RET_SKIPALL;
			break;
		}
		else if (Code==3)
		{
			Ret=SETATTR_RET_ERROR;
			break;
		}
	}
	return Ret;
}
