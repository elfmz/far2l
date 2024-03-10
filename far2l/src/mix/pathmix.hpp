#pragma once

/*
pathmix.hpp

Misc functions for processing of path names
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

#include "FARString.hpp"

const size_t cVolumeGuidLen = 48;

class NTPath
{
	FARString Str;

public:
	NTPath(LPCWSTR Src);

	operator LPCWSTR() const { return Str; }

	const FARString Get() const { return Str; }
};

inline int IsSlash(wchar_t x)
{
	return x == GOOD_SLASH;
}
inline int IsSlashA(char x)
{
	return x == GOOD_SLASH;
}

bool IsNetworkPath(const wchar_t *Path);
bool IsNetworkServerPath(const wchar_t *Path);
bool IsLocalPath(const wchar_t *Path);
bool IsLocalRootPath(const wchar_t *Path);
bool IsLocalPrefixPath(const wchar_t *Path);
bool IsLocalPrefixRootPath(const wchar_t *Path);
bool IsLocalVolumePath(const wchar_t *Path);
bool IsLocalVolumeRootPath(const wchar_t *Path);
bool IsAbsolutePath(const wchar_t *Path);
bool IsRootPath(const FARString &Path);
bool HasPathPrefix(const wchar_t *Path);
bool PathCanHoldRegularFile(const wchar_t *Path);
bool IsPluginPrefixPath(const wchar_t *Path);

bool CutToSlash(FARString &strStr, bool bInclude = false);
bool CutToSlash(std::wstring &strStr, bool bInclude = false);

FARString &CutToFolderNameIfFolder(FARString &strPath);

const wchar_t *WINAPI PointToName(const wchar_t *lpwszPath);
const wchar_t *PointToName(FARString &strPath);
const wchar_t *PointToName(const wchar_t *lpwszPath, const wchar_t *lpwszEndPtr);
const wchar_t *WINAPI PointToFolderNameIfFolder(const wchar_t *lpwszPath);
const wchar_t *PointToExt(const wchar_t *lpwszPath);
const wchar_t *PointToExt(FARString &strPath);
const wchar_t *PointToExt(const wchar_t *lpwszPath, const wchar_t *lpwszEndPtr);

void AddEndSlash(FARString &strPath);
void AddEndSlash(std::wstring &strPath);
BOOL WINAPI AddEndSlash(wchar_t *Path);

bool DeleteEndSlash(wchar_t *Path, bool AllEndSlash = false);
bool DeleteEndSlash(std::wstring &strPath, bool AllEndSlash = false);
bool DeleteEndSlash(FARString &strPath, bool AllEndSlash = false);

const wchar_t *FirstSlash(const wchar_t *String);
const wchar_t *LastSlash(const wchar_t *String);
bool FindSlash(size_t &Pos, const FARString &Str, size_t StartPos = 0);
bool FindLastSlash(size_t &Pos, const FARString &Str);

bool TestParentFolderName(const wchar_t *Name);
bool TestCurrentFolderName(const wchar_t *Name);
bool TestCurrentDirectory(const wchar_t *TestDir);

FARString ExtractFileName(const FARString &Path);
FARString ExtractFilePath(const FARString &Path);

template <bool (*PTranslateFN)(std::wstring &s)>
static bool TranslateFarString(FARString &str)
{
	std::wstring tmp(str.CPtr(), str.GetLength());
	if (!PTranslateFN(tmp))
		return false;

	str.Copy(tmp.c_str(), tmp.size());
	return true;
}

FARString LookupExecutable(const char *file);

bool PathHasParentPrefix(const FARString &Path);
void EnsurePathHasParentPrefix(FARString &Path);

bool ArePathesAtSameDevice(const FARString &path1, const FARString &path2);
FARString EscapeDevicePath(FARString path);
