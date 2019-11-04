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
	if (Src&&*Src)
	{
		Str=Src;
		if (!HasPathPrefix(Src) && *Src!='/')
		{
			ConvertNameToFull(Str,Str);
		}
	}
}

bool IsAbsolutePath(const wchar_t *Path)
{
	return Path && (HasPathPrefix(Path) || IsLocalPath(Path) || IsNetworkPath(Path));
}

bool IsNetworkPath(const wchar_t *Path)
{
	return Path && ((Path[0] == GOOD_SLASH && Path[1] == GOOD_SLASH && !HasPathPrefix(Path))||(HasPathPrefix(Path) && !StrCmpNI(Path+4,L"UNC/",4)));
}

bool IsNetworkServerPath(const wchar_t *Path)
{
/*
	"//server/share/" is valid windows path.
	"//server/" is not.
*/
	bool Result=false;
	if(IsNetworkPath(Path))
	{
		LPCWSTR SharePtr=wcspbrk(HasPathPrefix(Path)?Path+8:Path+2,L"/");
		if(!SharePtr || !SharePtr[1] || IsSlash(SharePtr[1]))
		{
			Result=true;
		}
	}
	return Result;
}

bool IsLocalPath(const wchar_t *Path)
{
	return (Path && Path[0] == L'/' && Path[1] != L'/');
}

bool IsLocalRootPath(const wchar_t *Path)
{
	return (Path && Path[0] == L'/' && !Path[1]);
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
	return HasPathPrefix(Path);//todo  && !_wcsnicmp(&Path[4],L"Volume{",7) && Path[47] == L'}';
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
	if (!p || !*(p+1))
		return false;

	return true;
}

bool IsPluginPrefixPath(const wchar_t *Path) //Max:
{
	if (Path[0] == GOOD_SLASH)
		return false;

	const wchar_t* pC = wcschr(Path, L':');

	if (!pC)
		return false;

	const wchar_t* pS = FirstSlash(Path);

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

	if (apiGetCurrentDirectory(strCurDir) && !StrCmp(strCurDir,TestDir))
		return true;

	return false;
}

const wchar_t* WINAPI PointToName(const wchar_t *lpwszPath)
{
	return PointToName(lpwszPath,nullptr);
}

const wchar_t* PointToName(FARString& strPath)
{
	const wchar_t *lpwszPath=strPath.CPtr();
	const wchar_t *lpwszEndPtr=lpwszPath+strPath.GetLength();
	return PointToName(lpwszPath,lpwszEndPtr);
}

const wchar_t* PointToName(const wchar_t *lpwszPath,const wchar_t *lpwszEndPtr)
{
	if (!lpwszPath)
		return nullptr;

	const wchar_t *lpwszNamePtr = lpwszEndPtr;

	if (!lpwszNamePtr)
	{
		lpwszNamePtr=lpwszPath;

		while (*lpwszNamePtr) lpwszNamePtr++;
	}

	while (lpwszNamePtr != lpwszPath)
	{
		if (IsSlash(*lpwszNamePtr))
			return lpwszNamePtr+1;

		lpwszNamePtr--;
	}

	if (IsSlash(*lpwszPath))
		return lpwszPath+1;
	else
		return lpwszPath;
}

//   Аналог PointToName, только для строк типа
//   "name\" (оканчивается на слеш) возвращает указатель на name, а не на пустую
//   строку
const wchar_t* WINAPI PointToFolderNameIfFolder(const wchar_t *Path)
{
	if (!Path)
		return nullptr;

	const wchar_t *NamePtr=Path, *prevNamePtr=Path;

	while (*Path)
	{
		if (IsSlash(*Path))
		{
			prevNamePtr=NamePtr;
			NamePtr=Path+1;
		}

		++Path;
	}

	return ((*NamePtr)?NamePtr:prevNamePtr);
}

const wchar_t* PointToExt(const wchar_t *lpwszPath)
{
	if (!lpwszPath)
		return nullptr;

	const wchar_t *lpwszEndPtr = lpwszPath;

	while (*lpwszEndPtr) lpwszEndPtr++;

	return PointToExt(lpwszPath,lpwszEndPtr);
}

const wchar_t* PointToExt(FARString& strPath)
{
	const wchar_t *lpwszPath=strPath.CPtr();
	const wchar_t *lpwszEndPtr=lpwszPath+strPath.GetLength();
	return PointToExt(lpwszPath,lpwszEndPtr);
}

const wchar_t* PointToExt(const wchar_t *lpwszPath,const wchar_t *lpwszEndPtr)
{
	if (!lpwszPath || !lpwszEndPtr)
		return nullptr;

	const wchar_t *lpwszExtPtr = lpwszEndPtr;

	while (lpwszExtPtr != lpwszPath)
	{
		if (*lpwszExtPtr==L'.')
		{
//			if (IsSlash(*(lpwszExtPtr-1)) || *(lpwszExtPtr-1)==L':')
//				return lpwszEndPtr;
///			else
				return lpwszExtPtr;
		}

		if (IsSlash(*lpwszExtPtr))
			return lpwszEndPtr;

		lpwszExtPtr--;
	}

	return lpwszEndPtr;
}

BOOL AddEndSlash(wchar_t *Path, wchar_t TypeSlash)
{
	BOOL Result=FALSE;

	if (Path)
	{
		/* $ 06.12.2000 IS
		  ! Теперь функция работает с обоими видами слешей, также происходит
		    изменение уже существующего конечного слеша на такой, который
		    встречается чаще.
		*/
		wchar_t *end;
		int Slash=0, BackSlash=0;

		if (!TypeSlash)
		{
			end=Path;

			while (*end)
			{
				Slash+=(*end==GOOD_SLASH);
				BackSlash+=(*end==L'/');
				end++;
			}
		}
		else
		{
			end=Path+StrLength(Path);

			if (TypeSlash == GOOD_SLASH)
				Slash=1;
			else
				BackSlash=1;
		}

		int Length=(int)(end-Path);
		char c= '/';
		Result=TRUE;

		if (!Length)
		{
			*end=c;
			end[1]=0;
		}
		else
		{
			end--;

			if (!IsSlash(*end))
			{
				end[1]=c;
				end[2]=0;
			}
			else
			{
				*end=c;
			}
		}
	}

	return Result;
}


BOOL WINAPI AddEndSlash(wchar_t *Path)
{
	return AddEndSlash(Path, 0);
}


BOOL AddEndSlash(FARString &strPath)
{
	return AddEndSlash(strPath, 0);
}

BOOL AddEndSlash(FARString &strPath, wchar_t TypeSlash)
{
	wchar_t *lpwszPath = strPath.GetBuffer(strPath.GetLength()+2);
	BOOL Result = AddEndSlash(lpwszPath, TypeSlash);
	strPath.ReleaseBuffer();
	return Result;
}

bool DeleteEndSlash(wchar_t *Path, bool AllEndSlash)
{
	bool Ret = false;
	size_t len = StrLength(Path);

	while (len && IsSlash(Path[--len]))
	{
		Ret = true;
		Path[len] = L'\0';

		if (!AllEndSlash)
			break;
	}

	return Ret;
}

BOOL WINAPI DeleteEndSlash(FARString &strPath, bool AllEndSlash)
{
	BOOL Ret=FALSE;

	if (!strPath.IsEmpty())
	{
		size_t len=strPath.GetLength();
		wchar_t *lpwszPath = strPath.GetBuffer();

		while (len && IsSlash(lpwszPath[--len]))
		{
			Ret=TRUE;
			lpwszPath[len] = L'\0';

			if (!AllEndSlash)
				break;
		}

		strPath.ReleaseBuffer();
	}

	return Ret;
}

bool CutToSlash(FARString &strStr, bool bInclude)
{
	size_t pos;

	if (FindLastSlash(pos,strStr))
	{
		if (pos==3 && HasPathPrefix(strStr))
			return false;

		if (bInclude)
			strStr.SetLength(pos);
		else
			strStr.SetLength(pos+1);

		return true;
	}

	return false;
}

FARString &CutToFolderNameIfFolder(FARString &strPath)
{
	wchar_t *lpwszPath = strPath.GetBuffer();
	wchar_t *lpwszNamePtr=lpwszPath, *lpwszprevNamePtr=lpwszPath;

	while (*lpwszPath)
	{
		if (IsSlash(*lpwszPath))
		{
			lpwszprevNamePtr=lpwszNamePtr;
			lpwszNamePtr=lpwszPath+1;
		}

		++lpwszPath;
	}

	if (*lpwszNamePtr)
		*lpwszNamePtr=0;
	else
		*lpwszprevNamePtr=0;

	strPath.ReleaseBuffer();
	return strPath;
}

const wchar_t *FirstSlash(const wchar_t *String)
{
	do
	{
		if (IsSlash(*String))
			return String;
	}
	while (*String++);

	return nullptr;
}

const wchar_t *LastSlash(const wchar_t *String)
{
	const wchar_t *Start = String;

	while (*String++)
		;

	while (--String!=Start && !IsSlash(*String))
		;

	return IsSlash(*String)?String:nullptr;
}

bool FindSlash(size_t &Pos, const FARString &Str, size_t StartPos)
{
	for (size_t p = StartPos; p < Str.GetLength(); p++)
	{
		if (IsSlash(Str[p]))
		{
			Pos = p;
			return true;
		}
	}

	return false;
}

bool FindLastSlash(size_t &Pos, const FARString &Str)
{
	for (size_t p = Str.GetLength(); p > 0; p--)
	{
		if (IsSlash(Str[p - 1]))
		{
			Pos = p - 1;
			return true;
		}
	}

	return false;
}

// find path root component (drive letter / volume name / server share) and calculate its length
size_t GetPathRootLength(const FARString &Path)
{
	unsigned PrefixLen = 0;
	bool IsUNC = false;

	/* if (Path.Equal(0,8,L"//?/UNC/",8))
	{
		PrefixLen = 8;
		IsUNC = true;
	}
	else if (Path.Equal(0,4,L"//?/",4) || Path.Equal(0,4,L"\/??\/",4) || Path.Equal(0,4,L"//./",4))
	{
		PrefixLen = 4;
	}
	else */ if (Path.Equal(0,2,L"//",2))
	{
		PrefixLen = 2;
		IsUNC = true;
	}

	if (!PrefixLen && !Path.Equal(1, L':'))
		return 0;

	size_t p;

	if (!FindSlash(p, Path, PrefixLen))
		p = Path.GetLength();

	if (IsUNC)
		if (!FindSlash(p, Path, p + 1))
			p = Path.GetLength();

	return p;
}

FARString ExtractPathRoot(const FARString &Path)
{
	size_t PathRootLen = GetPathRootLength(Path);

	if (PathRootLen)
		return FARString(Path.CPtr(), PathRootLen).Append(GOOD_SLASH);
	else
		return FARString();
}

FARString ExtractFileName(const FARString &Path)
{
	size_t p;

	if (FindLastSlash(p, Path))
		p++;
	else
		p = 0;

	size_t PathRootLen = GetPathRootLength(Path);

	if (p <= PathRootLen && PathRootLen)
		return FARString();

	return FARString(Path.CPtr() + p, Path.GetLength() - p);
}

FARString ExtractFilePath(const FARString &Path)
{
	size_t p;

	if (!FindLastSlash(p, Path))
		p = 0;

	size_t PathRootLen = GetPathRootLength(Path);

	if (p <= PathRootLen && PathRootLen)
		return FARString(Path.CPtr(), PathRootLen).Append(GOOD_SLASH);

	return FARString(Path.CPtr(), p);
}

bool IsRootPath(const FARString &Path)
{
	size_t PathRootLen = GetPathRootLength(Path);

	if (Path.GetLength() == PathRootLen)
		return true;

	if (Path.GetLength() == PathRootLen + 1 && IsSlash(Path[Path.GetLength() - 1]))
		return true;

	return false;
}

