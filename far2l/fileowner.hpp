#pragma once

/*
fileowner.hpp

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

#include <map>

template <class ID, const char *(*UNCACHED_LOOKUP)(ID)>
class CachedFileLookupT
{
	struct Cache : std::map<ID, FARString> {} _cache;
	
public:
	const FARString &Lookup(ID id)
	{
		auto cache_it = _cache.lower_bound(id);
		if (cache_it == _cache.end() || cache_it->first != id) {
			const char *uncached_value = UNCACHED_LOOKUP(id);
			if (!uncached_value) {
				uncached_value = "";
			}
			cache_it = _cache.insert(cache_it, std::make_pair(id, FARString(uncached_value)));
		}

		return cache_it->second;
	}
};

const char *OwnerNameByID(uid_t id);
const char *GroupNameByID(gid_t id);

typedef CachedFileLookupT<uid_t, &OwnerNameByID> CachedFileOwnerLookup;
typedef CachedFileLookupT<gid_t, &GroupNameByID> CachedFileGroupLookup;


bool WINAPI GetFileOwner(const wchar_t *Computer,const wchar_t *Name, FARString &strOwner);
bool WINAPI GetFileGroup(const wchar_t *Computer,const wchar_t *Name, FARString &strGroup);

bool SetOwner(LPCWSTR Object, LPCWSTR Owner);
bool SetGroup(LPCWSTR Object, LPCWSTR Group);
