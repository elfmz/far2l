/*
drivemix.cpp

Misc functions for drive/disk info
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


#include "drivemix.hpp"
#include "config.hpp"
#include "cddrv.hpp"
#include "pathmix.hpp"
#include <errno.h>

static int StatForPathOrItsParent(const wchar_t *path, struct stat &s)
{
	std::string mb;
	if (*path != GOOD_SLASH) {
		FARString strSrcFullName;
		ConvertNameToFull(path, strSrcFullName);
		if (!strSrcFullName.IsEmpty())
			mb = strSrcFullName.GetMB();
	}
	if (mb.empty())
		mb = Wide2MB(path);

	for (;;) {
		int r = sdc_stat(mb.c_str(), &s);
		if (r == 0 || mb.size() < 2)
			return r;

		size_t slash = mb.rfind(GOOD_SLASH);		
		if (slash==std::string::npos || slash==0)
			return -1;
		
		mb.resize(slash);
	}
}

int CheckDisksProps(const wchar_t *SrcPath,const wchar_t *DestPath,int CheckedType)
{
	if (CheckedType == CHECKEDPROPS_ISSAMEDISK)
	{
		struct stat s_src = {}, s_dest = {};
		int r = StatForPathOrItsParent(SrcPath, s_src);
		if (r==-1) {
			fprintf(stderr, "CheckDisksProps: stat errno=%u for src='%ls'\n", errno, SrcPath);
			return false;
		}
				
		r = StatForPathOrItsParent(DestPath, s_dest);
		if (r==-1) {
			fprintf(stderr, "CheckDisksProps: stat errno=%u for dest='%ls'\n", errno, DestPath);
			return false;
		}

		return (s_src.st_dev==s_dest.st_dev) ? TRUE : FALSE;
	}

	if (CheckedType == CHECKEDPROPS_ISDST_ENCRYPTION)
	{
		return FALSE;
	}

	return TRUE;
}

