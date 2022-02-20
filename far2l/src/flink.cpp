/*
flink.cpp

Куча разных функций по обработке Link`ов - Hard&Sym
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


#include "copy.hpp"
#include "flink.hpp"
#include "cddrv.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "panelmix.hpp"
#include "message.hpp"
#include "lang.hpp"
#include "dirmix.hpp"
#include "treelist.hpp"

int WINAPI GetNumberOfLinks(const wchar_t *Name)
{
	struct stat s = {};
	if (sdc_stat(Wide2MB(Name).c_str(), &s)!=0)
		return 1;
		
	return (s.st_nlink > 0) ? s.st_nlink : 1;
}

static std::string SymName(const wchar_t *ExistingName,const wchar_t *NewName)
{
	fprintf(stderr, "SymTarget('%ls', '%ls')\n", ExistingName, NewName);
	std::string out(Wide2MB(NewName));
	if (!out.empty() && out[out.size()-1]==GOOD_SLASH) {
		const wchar_t *slash = wcsrchr(ExistingName, GOOD_SLASH);
		out+= Wide2MB(slash ? slash + 1 : ExistingName);
	}
	return out;
}

static std::string SymSubject(const wchar_t *ExistingName)
{
	if (*ExistingName==GOOD_SLASH)
		return Wide2MB(ExistingName);
//todo: use ConvertNameToReal
	if (ExistingName[0]=='.' && ExistingName[1]==GOOD_SLASH)
		ExistingName+= 2;

	FARString path;
	apiGetCurrentDirectory(path);
	AddEndSlash(path);
	path+= ExistingName;
	
	return Wide2MB(path.CPtr());
}

int WINAPI MkHardLink(const wchar_t *ExistingName,const wchar_t *NewName)
{
	int r = sdc_link( SymSubject(ExistingName).c_str() , SymName(ExistingName, NewName).c_str() );
	if (r!=0) {
		return 0;
	}
	
	return 1;
}

int WINAPI MkSymLink(const wchar_t *ExistingName, const wchar_t *NewName, ReparsePointTypes LinkType, bool CanShowMsg)
{
	int r = sdc_symlink( SymSubject(ExistingName).c_str() , SymName(ExistingName, NewName).c_str() );
	if (r!=0) {
		if (CanShowMsg) {
			Message(MSG_WARNING,1,Msg::Error,Msg::CopyCannotCreateJunctionToFile,NewName, Msg::Ok);
		}
		
		return 0;
	}
	
	return 1;
}

int WINAPI FarMkLink(const wchar_t *ExistingName, const wchar_t *NewName, DWORD Flags)
{
	int Result=0;

	if (ExistingName && *ExistingName && NewName && *NewName)
	{
		int Op=Flags&0xFFFF;

		switch (Op)
		{
			case FLINK_HARDLINK:
				Result=MkHardLink(ExistingName, NewName);
				break;
			case FLINK_JUNCTION:
			case FLINK_VOLMOUNT:
			case FLINK_SYMLINKFILE:
			case FLINK_SYMLINKDIR:
				ReparsePointTypes LinkType=RP_JUNCTION;

				Result=MkSymLink(ExistingName, NewName,LinkType,(Flags&FLINK_SHOWERRMSG) != 0);
		}
	}

	if (Result && !(Flags&FLINK_DONOTUPDATEPANEL))
		ShellUpdatePanels(nullptr, FALSE);

	return Result;
}
