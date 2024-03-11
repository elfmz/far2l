/*
pathmix.cpp

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

#include "headers.hpp"

#include "pathmix.hpp"
#include "strmix.hpp"
#include "panel.hpp"
#include <time.h>
#include <sys/time.h>

NTPath::NTPath(LPCWSTR Src)
{
	if (Src && *Src) {
		Str = Src;
		if (!HasPathPrefix(Src) && *Src != GOOD_SLASH) {
			ConvertNameToFull(Str, Str);
		}
	}
}

bool IsAbsolutePath(const wchar_t *Path)
{
	return Path && (HasPathPrefix(Path) || IsLocalPath(Path) || IsNetworkPath(Path));
}

bool IsNetworkPath(const wchar_t *Path)
{
	return false;
}

bool IsNetworkServerPath(const wchar_t *Path)
{
	return false;
}

bool IsLocalPath(const wchar_t *Path)
{
	return (Path && Path[0] == LGOOD_SLASH);	// && Path[1] != L'/'
}

bool IsLocalRootPath(const wchar_t *Path)
{
	return (Path && Path[0] == LGOOD_SLASH && !Path[1]);
}

bool HasPathPrefix(const wchar_t *Path)
{
	return false;
}

bool IsLocalPrefixPath(const wchar_t *Path)
{
	return HasPathPrefix(Path);
}

bool IsLocalPrefixRootPath(const wchar_t *Path)
{
	return IsLocalPrefixPath(Path) && !Path[7];
}

bool IsLocalVolumePath(const wchar_t *Path)
{
	return HasPathPrefix(Path);		// todo && !_wcsnicmp(&Path[4],L"Volume{",7) && Path[47] == L'}';
}

bool IsLocalVolumeRootPath(const wchar_t *Path)
{
	return IsLocalVolumePath(Path) && (!Path[48] || (IsSlash(Path[48]) && !Path[49]));
}

bool PathCanHoldRegularFile(const wchar_t *Path)
{
	if (!IsNetworkPath(Path))
		return true;

	/* \\ */
	unsigned offset = 0;

	const wchar_t *p = FirstSlash(Path + offset);

	/* server || server\ */
	if (!p || !*(p + 1))
		return false;

	return true;
}

bool IsPluginPrefixPath(const wchar_t *Path)	// Max:
{
	if (Path[0] == GOOD_SLASH)
		return false;

	const wchar_t *pC = wcschr(Path, L':');

	if (!pC)
		return false;

	const wchar_t *pS = FirstSlash(Path);

	if (pS && pS < pC)
		return false;

	return true;
}

bool TestParentFolderName(const wchar_t *Name)
{
	return Name[0] == L'.' && Name[1] == L'.' && (!Name[2] || (IsSlash(Name[2]) && !Name[3]));
}

bool TestCurrentFolderName(const wchar_t *Name)
{
	return Name[0] == L'.' && (!Name[1] || (IsSlash(Name[1]) && !Name[2]));
}

bool TestCurrentDirectory(const wchar_t *TestDir)
{
	FARString strCurDir;

	if (apiGetCurrentDirectory(strCurDir) && !StrCmp(strCurDir, TestDir))
		return true;

	return false;
}

const wchar_t *WINAPI PointToName(const wchar_t *lpwszPath)
{
	return PointToName(lpwszPath, nullptr);
}

const wchar_t *PointToName(FARString &strPath)
{
	const wchar_t *lpwszPath = strPath.CPtr();
	const wchar_t *lpwszEndPtr = lpwszPath + strPath.GetLength();
	return PointToName(lpwszPath, lpwszEndPtr);
}

const wchar_t *PointToName(const wchar_t *lpwszPath, const wchar_t *lpwszEndPtr)
{
	if (!lpwszPath)
		return nullptr;

	if (lpwszEndPtr) {
		for (const wchar_t *lpwszScanPtr = lpwszEndPtr; lpwszScanPtr != lpwszPath;) {
			--lpwszScanPtr;
			if (IsSlash(*lpwszScanPtr))
				return lpwszScanPtr + 1;
		}
	} else {
		const wchar_t *lpwszLastSlashPtr = nullptr;
		for (const wchar_t *lpwszScanPtr = lpwszPath; *lpwszScanPtr; ++lpwszScanPtr) {
			if (IsSlash(*lpwszScanPtr))
				lpwszLastSlashPtr = lpwszScanPtr;
		}
		if (lpwszLastSlashPtr)
			return lpwszLastSlashPtr + 1;
	}

	return lpwszPath;
}

/*
	Аналог PointToName, только для строк типа
	"name\" (оканчивается на слеш) возвращает указатель на name, а не на пустую
	строку
*/
const wchar_t *WINAPI PointToFolderNameIfFolder(const wchar_t *Path)
{
	if (!Path)
		return nullptr;

	const wchar_t *NamePtr = Path, *prevNamePtr = Path;

	while (*Path) {
		if (IsSlash(*Path)) {
			prevNamePtr = NamePtr;
			NamePtr = Path + 1;
		}

		++Path;
	}

	return ((*NamePtr) ? NamePtr : prevNamePtr);
}

const wchar_t *PointToExt(const wchar_t *lpwszPath)
{
	if (!lpwszPath)
		return nullptr;

	const wchar_t *lpwszEndPtr = lpwszPath;

	while (*lpwszEndPtr)
		lpwszEndPtr++;

	return PointToExt(lpwszPath, lpwszEndPtr);
}

const wchar_t *PointToExt(FARString &strPath)
{
	return PointToExt(strPath.CPtr(), strPath.CEnd());
}

const wchar_t *PointToExt(const wchar_t *lpwszPath, const wchar_t *lpwszEndPtr)
{
	if (!lpwszPath || !lpwszEndPtr)
		return nullptr;

	const wchar_t *lpwszExtPtr = lpwszEndPtr;

	while (lpwszExtPtr != lpwszPath) {
		if (*lpwszExtPtr == L'.') {
			return lpwszExtPtr;
		}

		if (IsSlash(*lpwszExtPtr))
			return lpwszEndPtr;

		lpwszExtPtr--;
	}

	return lpwszEndPtr;
}

void AddEndSlash(FARString &strPath)
{
	if (strPath.IsEmpty() || strPath[strPath.GetLength() - 1] != GOOD_SLASH)
		strPath+= GOOD_SLASH;
}

void AddEndSlash(std::wstring &strPath)
{
	if (strPath.empty() || strPath.back() != GOOD_SLASH)
		strPath+= GOOD_SLASH;
}

BOOL WINAPI AddEndSlash(wchar_t *Path)
{
	const size_t len = wcslen(Path);
	if (len && Path[len - 1] == GOOD_SLASH)
		return FALSE;

	Path[len] = GOOD_SLASH;
	Path[len + 1] = 0;
	return TRUE;
}

bool DeleteEndSlash(wchar_t *Path, bool AllEndSlash)
{
	bool Ret = false;
	size_t len = StrLength(Path);

	while (len && IsSlash(Path[--len])) {
		Ret = true;
		Path[len] = L'\0';

		if (!AllEndSlash)
			break;
	}

	return Ret;
}

bool DeleteEndSlash(std::wstring &strPath, bool AllEndSlash)
{
	bool out = false;
	while (!strPath.empty() && IsSlash(strPath.back())) {
		out = true;
		strPath.pop_back();
		if (!AllEndSlash)
			break;
	}
	return out;
}

bool DeleteEndSlash(FARString &strPath, bool AllEndSlash)
{
	size_t LenToSlash = strPath.GetLength();

	while (LenToSlash != 0 && IsSlash(strPath.At(LenToSlash - 1))) {
		--LenToSlash;
		if (!AllEndSlash)
			break;
	}

	if (LenToSlash == strPath.GetLength())
		return false;

	strPath.Truncate(LenToSlash);
	return true;
}

bool CutToSlash(FARString &strStr, bool bInclude)
{
	size_t pos;

	if (FindLastSlash(pos, strStr)) {
		if (bInclude)
			strStr.Truncate(pos);
		else
			strStr.Truncate(pos + 1);

		return true;
	}

	return false;
}

bool CutToSlash(std::wstring &strStr, bool bInclude)
{
	size_t pos = strStr.rfind(GOOD_SLASH);
	if (pos == std::string::npos)
		return false;

	strStr.resize(bInclude ? pos + 1 : pos);
	return true;
}

FARString &CutToFolderNameIfFolder(FARString &strPath)
{
	wchar_t *lpwszPath = strPath.GetBuffer();
	wchar_t *lpwszNamePtr = lpwszPath, *lpwszprevNamePtr = lpwszPath;

	while (*lpwszPath) {
		if (IsSlash(*lpwszPath)) {
			lpwszprevNamePtr = lpwszNamePtr;
			lpwszNamePtr = lpwszPath + 1;
		}

		++lpwszPath;
	}

	if (*lpwszNamePtr)
		*lpwszNamePtr = 0;
	else
		*lpwszprevNamePtr = 0;

	strPath.ReleaseBuffer();
	return strPath;
}

const wchar_t *FirstSlash(const wchar_t *String)
{
	do {
		if (IsSlash(*String))
			return String;
	} while (*String++);

	return nullptr;
}

const wchar_t *LastSlash(const wchar_t *String)
{
	const wchar_t *Start = String;

	while (*String++)
		;

	while (--String != Start && !IsSlash(*String))
		;

	return IsSlash(*String) ? String : nullptr;
}

bool FindSlash(size_t &Pos, const FARString &Str, size_t StartPos)
{
	for (size_t p = StartPos; p < Str.GetLength(); p++) {
		if (IsSlash(Str[p])) {
			Pos = p;
			return true;
		}
	}

	return false;
}

bool FindLastSlash(size_t &Pos, const FARString &Str)
{
	for (size_t p = Str.GetLength(); p > 0; p--) {
		if (IsSlash(Str[p - 1])) {
			Pos = p - 1;
			return true;
		}
	}

	return false;
}

FARString ExtractFileName(const FARString &Path)
{
	size_t p;

	if (FindLastSlash(p, Path))
		p++;
	else
		p = 0;

	return FARString(Path.CPtr() + p, Path.GetLength() - p);
}

FARString ExtractFilePath(const FARString &Path)
{
	size_t p;

	if (!FindLastSlash(p, Path))
		p = 0;

	return FARString(Path.CPtr(), p);
}

bool IsRootPath(const FARString &Path)
{
	return Path == WGOOD_SLASH;
}

static std::string LookupExecutableInEnvPath(const char *file)
{
	std::string out;

	FARString str_path;
	apiGetEnvironmentVariable("PATH", str_path);
	const std::string &mb_path = str_path.GetMB();

	for (const char *s = mb_path.c_str(); *s;) {
		const char *p = strchr(s, ':');

		if (p != NULL) {
			out.assign(s, p - s);
		} else {
			out.assign(s);
		}

		if (out.empty() || out[out.size() - 1] != GOOD_SLASH) {
			out+= GOOD_SLASH;
		}
		out+= file;
		struct stat st{};
		if (stat(out.c_str(), &st) == 0) {
			break;
		}
		out.clear();
		if (p == NULL)
			break;
		s = p + 1;
	}

	return out;
}

FARString LookupExecutable(const char *file)
{
	FARString out;
	if (file[0] == GOOD_SLASH) {
		out = file;

	} else if (file[0] == '.' && file[1] == GOOD_SLASH) {
		apiGetCurrentDirectory(out);
		out+= file + 1;

	} else {
		out = LookupExecutableInEnvPath(file);
		if (out.IsEmpty()) {
			apiGetCurrentDirectory(out);
			out+= GOOD_SLASH;
			out+= file;
		}
	}
	return out;
}

bool PathHasParentPrefix(const FARString &Path)
{
	return (Path.Begins(GOOD_SLASH) || Path.Begins(L"./") || Path == L".");
}

void EnsurePathHasParentPrefix(FARString &Path)
{
	if (!PathHasParentPrefix(Path)) {
		Path.Insert(0, L"./");
	}
}

static dev_t GetDeviceId(const std::string &path)
{
	struct stat st{};
	if (stat(path.c_str(), &st) == 0) {
		return st.st_dev;
	}
	const size_t slash = path.rfind(GOOD_SLASH);
	if (slash != 0 && slash != std::string::npos) {
		return GetDeviceId(path.substr(0, slash));
	}
	return 0;
}

bool ArePathesAtSameDevice(const FARString &path1, const FARString &path2)
{
	return GetDeviceId(path1.GetMB()) == GetDeviceId(path2.GetMB());
}

FARString EscapeDevicePath(FARString path)
{
	const dev_t dev = GetDeviceId(path.GetMB());
	while (path.GetLength() > 1 && CutToSlash(path, true)) {
		const dev_t xdev = GetDeviceId(path.GetMB());
		if (dev != xdev) {
			break;
		}
	}
	if (path.GetLength() == 0) {
		path = WGOOD_SLASH;
	}
	return path;
}
