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
#include "config.hpp"

FilePositionCache::FilePositionCache(FilePositionCacheKind kind)
	: _kind(kind),
	_kf_path(InMyConfig( (kind == FPCK_VIEWER) ? "viewer.pos" : "editor.pos"))
{
}

FilePositionCache::~FilePositionCache()
{
	CheckForSave();
}

void FilePositionCache::ApplyElementsLimit()
{
	AssertConfigLoaded();
	int MaxPositionCache = Opt.MaxPositionCache;

	if ((int)_kfh->SectionsCount() > MaxPositionCache + MaxPositionCache / 4 + 16)
	{
		std::vector<std::string> sections = _kfh->EnumSections();
		std::sort(sections.begin(), sections.end(),
			[&](const std::string &a, const std::string &b) -> bool {
			return _kfh->GetULL(a.c_str(), "TS", 0) <  _kfh->GetULL(b.c_str(), "TS", 0);
		});

		int cnt = (int)sections.size() - MaxPositionCache;
		for (int i = 0; i < cnt; ++i) {
			_kfh->RemoveSection(sections[i].c_str());
		}
	}
}

void FilePositionCache::CheckForSave()
{
	// If saving enabled then save _kfh to file and release,
	// so other instances will see changes immediately and our instance
	// will see other's changes too.
	// If save is disabled then keep _kfh - it will serve as in memory
	// storage for our instance lifetime or until user will enable saving.

	if (_kfh &&
			((_kind == FPCK_VIEWER && Opt.ViOpt.SavePos)
			|| (_kind == FPCK_EDITOR && Opt.EdOpt.SavePos) ) )
 	{
		if (_kfh->Save()) {
			_kfh.reset();
		}
	}
}

static void MakeSectionName(const wchar_t *name, std::string &section)
{
	if (*name != L'<') {
		FARString full_name;
		ConvertNameToFull(name, full_name);
		Wide2MB(full_name, section);

	} else {
		Wide2MB(name, section);
	}

	FilePathHashSuffix(section);
}

static size_t ParamCountToSave(const DWORD64 *param)
{
	for (size_t i = POSCACHE_PARAM_COUNT; i != 0; --i) {
		if (param[i - 1] != 0) {
			return i;
		}
	}

	return 0;
}

static size_t PositionCountToSave(const DWORD64 *position)
{
	if (position) {
		for (size_t i = POSCACHE_BOOKMARK_COUNT; i != 0; --i) {
			if (position[i - 1] != POS_NONE) {
				return i;
			}
		}
	}

	return 0;
}

void FilePositionCache::AddPosition(const wchar_t *name, PosCache& poscache)
{
	if (!_kfh) {
		_kfh.reset(new KeyFileHelper(_kf_path.c_str()));
	}

	ApplyElementsLimit();

	bool have_some_to_save = true;
	std::string section;
	MakeSectionName(name, section);

	size_t save_count = ParamCountToSave(poscache.Param);
	if (save_count) {
		_kfh->PutBytes(section.c_str(), "Par",
			save_count * sizeof(poscache.Param[0]), (unsigned char *)&poscache.Param[0]);

	} else {
		have_some_to_save = false;
		for (unsigned int i = 0; i < ARRAYSIZE(poscache.Position); ++i) {
			if (PositionCountToSave(poscache.Position[i]) ){
				have_some_to_save = true;
				break;
			}
		}
		if (have_some_to_save) {
			_kfh->RemoveKey(section.c_str(), "Par");
		}
	}

	if (have_some_to_save) {
		for (unsigned int i = 0; i < ARRAYSIZE(poscache.Position); ++i) {
			char key[64];
			sprintf(key, "Pos%u", i);
			save_count = PositionCountToSave(poscache.Position[i]);
			if (save_count) {
				_kfh->PutBytes(section.c_str(), key,
					save_count * sizeof(poscache.Position[i][0]), (unsigned char *)poscache.Position[i]);
			} else {
				_kfh->RemoveKey(section.c_str(), key);
			}
		}

		_kfh->PutULLAsHex(section.c_str(), "TS", time(NULL));

	} else {
		_kfh->RemoveSection(section.c_str());
	}

	CheckForSave();
}

bool FilePositionCache::GetPosition(const wchar_t *name, PosCache& poscache)
{
	std::string section;
	MakeSectionName(name, section);

	std::unique_ptr<KeyFileReadSection> tmp_kfrs;
	const KeyFileValues *values;
	if (_kfh) {
		values = _kfh->GetSectionValues(section.c_str());
		if (!values) {
			return false;
		}

	} else {
		tmp_kfrs.reset(new KeyFileReadSection(_kf_path.c_str(), section.c_str()));
		if (!tmp_kfrs->SectionLoaded()) {
			return false;
		}
		values = tmp_kfrs.get();
	}

	memset(&poscache.Param[0], 0, sizeof(poscache.Param));
	values->GetBytes("Par",
		sizeof(poscache.Param), (unsigned char *)&poscache.Param[0]);

	for (unsigned int i = 0; i < ARRAYSIZE(poscache.Position); ++i) if (poscache.Position[i]) {
		memset(poscache.Position[i], 0xff,
			sizeof(poscache.Position[i][0]) * POSCACHE_BOOKMARK_COUNT);
		char key[64];
		sprintf(key, "Pos%u", i);
		values->GetBytes(key, sizeof(poscache.Position[i][0]) * POSCACHE_BOOKMARK_COUNT,
			(unsigned char *)poscache.Position[i]);
	}

	return true;
}

