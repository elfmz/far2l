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

#include "fileowner.hpp"
#include "pathmix.hpp"
#include "DList.hpp"
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

const char *OwnerNameByID(uid_t id)
{
	struct passwd *pw = getpwuid(id);
	if (!pw) {
		perror("OwnerNameByID");
		return NULL;
	}
	return pw->pw_name;
}

const char *GroupNameByID(gid_t id)
{
	struct group *gr = getgrgid(id);
	if (!gr) {
		perror("GroupNameByID");
		return NULL;
	}
	return gr->gr_name;
}

bool WINAPI GetFileOwner(const wchar_t *Computer, const wchar_t *Name, FARString &strOwner)
{
	struct stat s{};
	if (sdc_stat(Wide2MB(Name).c_str(), &s) == 0) {
		const char *sz = OwnerNameByID(s.st_uid);
		if (sz) {
			strOwner = sz;
			return true;
		}
	}

	return false;
}

bool WINAPI GetFileGroup(const wchar_t *Computer, const wchar_t *Name, FARString &strGroup)
{
	struct stat s{};
	if (sdc_stat(Wide2MB(Name).c_str(), &s) == 0) {
		const char *sz = GroupNameByID(s.st_gid);
		if (sz) {
			strGroup = sz;
			return true;
		}
	}

	return false;
}

bool SetOwner(LPCWSTR Object, LPCWSTR Owner)
{
	struct passwd *p = getpwnam(Wide2MB(Owner).c_str());
	if (p) {
		if (sdc_chown(Wide2MB(Object).c_str(), p->pw_uid, -1) == 0)
			return true;
	} else
		perror("getpwnam");

	return false;
}

bool SetGroup(LPCWSTR Object, LPCWSTR Group)
{
	struct group *g = getgrnam(Wide2MB(Group).c_str());
	if (g) {
		if (sdc_chown(Wide2MB(Object).c_str(), -1, g->gr_gid) == 0)
			return true;
	} else
		perror("getgrnam");

	return false;
}
