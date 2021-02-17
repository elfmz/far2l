/*
poscache.cpp

Кэш позиций в файлах для viewer/editor
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
#include <algorithm>

#include "poscache.hpp"
#include "udlist.hpp"
#include "registry.hpp"
#include "config.hpp"

#define MSIZE_PARAM1           (sizeof(DWORD64)*5)
#define MSIZE_PARAM            (Opt.MaxPositionCache*MSIZE_PARAM1)
#define MSIZE_POSITION1        (BOOKMARK_COUNT*sizeof(DWORD64)*4)
#define MSIZE_POSITION         (Opt.MaxPositionCache*MSIZE_POSITION1)

#define PARAM_POS(Pos)         ((Pos)*MSIZE_PARAM1)
#define POSITION_POS(Pos,Idx)  (((Pos)*4+(Idx))*BOOKMARK_COUNT*sizeof(DWORD64))

LPCWSTR EmptyPos=L"0,0,0,0,0,\"$\"";


FilePositionCache::FilePositionCache(FilePositionCacheKind kind)
	:_kind(kind)
{
}

FilePositionCache::~FilePositionCache()
{
	DontNeedKeyFileHelper();
}

void FilePositionCache::NeedKeyFileHelper()
{
	if (_kfh) {
		return;
	}

	_kfh.reset(new KeyFileHelper(
		InMyConfig( (_kind == FPCK_VIEWER) ? "Viewer.CachePos" : "Editor.CachePos").c_str()));
}

void FilePositionCache::DontNeedKeyFileHelper()
{
	if ( (_kind == FPCK_VIEWER && Opt.ViOpt.SavePos)
		|| (_kind == FPCK_EDITOR && Opt.EdOpt.SavePos) )
 	{
		if (_kfh->Save()) {
			_kfh.reset();
		}
	}
}

void FilePositionCache::AddPosition(const wchar_t *name, PosCache& poscache)
{
	NeedKeyFileHelper();

	if (!Opt.MaxPositionCache)
	{
		GetRegKey(L"System", L"MaxPositionCache", Opt.MaxPositionCache, MAX_POSITIONS);
	}

	if ((int)_kfh->SectionsCount() > Opt.MaxPositionCache + Opt.MaxPositionCache/2 + 16)
	{
		std::vector<std::string> sections = _kfh->EnumSections();
		std::sort(sections.begin(), sections.end(), 
			[&](const std::string & a, const std::string & b) -> bool
		{
			return _kfh->GetULL(a.c_str(), "Timestamp", 0)
					<  _kfh->GetULL(b.c_str(), "Timestamp", 0);
		});

		size_t cnt = sections.size() - Opt.MaxPositionCache;
		for (size_t i = 0; i < cnt && i < sections.size(); ++i) {
			_kfh->RemoveSection(sections[i].c_str());
		}
	}

	std::string section;
	Wide2MB(name, section);

	char key[64];
	for (unsigned int i = 0; i < ARRAYSIZE(poscache.Param); ++i) {
		sprintf(key, "Param_%u", i);
		_kfh->PutULL(section.c_str(), key, poscache.Param[i]);
	}
	for (unsigned int i = 0; i < ARRAYSIZE(poscache.Position); ++i) {
		for (unsigned int j = 0; j < BOOKMARK_COUNT; ++j) {
			sprintf(key, "Position_%u_%u", i, j);
			if (poscache.Position[i]) {
				_kfh->PutULL(section.c_str(), key, poscache.Position[i][j]);
			} else {
				_kfh->RemoveKey(section.c_str(), key);
			}
		}
	}

	_kfh->PutULL(section.c_str(), "Timestamp", time(NULL));

	DontNeedKeyFileHelper();
}

bool FilePositionCache::GetPosition(const wchar_t *name, PosCache& poscache)
{
	NeedKeyFileHelper();

	std::string section;
	Wide2MB(name, section);
	const auto *values = _kfh->GetSectionValues(section.c_str());
	if (!values) {
		return false;
	}

	char key[64];

	for (unsigned int i = 0; i < ARRAYSIZE(poscache.Param); ++i) {
		sprintf(key, "Param_%u", i);
		poscache.Param[i] = values->GetULL(key, poscache.Param[i]);
	}

	for (unsigned int i = 0; i < ARRAYSIZE(poscache.Position); ++i) {
		for (unsigned int j = 0; j < BOOKMARK_COUNT; ++j) {
			if (poscache.Position[i]) {
				sprintf(key, "Position_%u_%u", i, j);
				poscache.Position[i][j] = values->GetULL(key, 0);
			}
		}
	}

	// values = nullptr;

	_kfh->PutULL(section.c_str(), "Timestamp", time(NULL));

	DontNeedKeyFileHelper();

	return true;
}

