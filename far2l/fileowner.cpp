/*
fileowner.cpp

Кэш SID`ов и функция GetOwner
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

#include "fileowner.hpp"
#include "pathmix.hpp"
#include "DList.hpp"
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

bool WINAPI GetFileOwner(const wchar_t *Computer,const wchar_t *Name, FARString &strOwner)
{
	struct stat s = {};
	if (stat(Wide2MB(Name).c_str(), &s)==0) {
		struct passwd *pw = getpwuid(s.st_uid);
		if (pw) {
			strOwner = pw->pw_name;
			return true;
		}
	}
	
	WINPORT(TranslateErrno)();
	return false;
}

bool WINAPI GetFileGroup(const wchar_t *Computer,const wchar_t *Name, FARString &strGroup)
{
	struct stat s = {};
	if (stat(Wide2MB(Name).c_str(), &s)==0) {
		struct group  *gr = getgrgid(s.st_gid);
		if (gr) {
			strGroup = gr->gr_name;
			return true;
		}
	}

	WINPORT(TranslateErrno)();
	return false;
}

bool SetOwner(LPCWSTR Object, LPCWSTR Owner)
{		
	struct passwd *p = getpwnam(Wide2MB(Owner).c_str());
	if ( p) {
		if (chown(Wide2MB(Object).c_str(), p->pw_uid, -1)==0)
			return true;
	}
		
	WINPORT(TranslateErrno)();
	return false;
}

bool SetGroup(LPCWSTR Object, LPCWSTR Group)
{
	struct group *g = getgrnam(Wide2MB(Group).c_str());
	if (g) {
		if (chown(Wide2MB(Object).c_str(), -1, g->gr_gid)==0)
			return true;
	}
		
	WINPORT(TranslateErrno)();
	return false;
}
