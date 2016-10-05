/*
cvtname.cpp

Функций для преобразования имен файлов/путей.
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


#include "cvtname.hpp"
#include "cddrv.hpp"
#include "syslog.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "strmix.hpp"
#include <set>

#define IsColon(str)         (str == L':')
#define IsDot(str)           (str == L'.')
#define IsSlashForward(str)  (str == L'/')
#define IsQuestion(str)      (str == L'?')

enum PATH_PFX_TYPE
{
	PPT_NONE,
	PPT_DRIVE,
	PPT_ROOT,
	PPT_PREFIX,
	PPT_NT
};

PATH_PFX_TYPE Point2Root(LPCWSTR stPath, size_t& PathOffset)
{
	if (stPath)
	{
		PATH_PFX_TYPE nPrefix = PPT_NONE;
		LPCWSTR pstPath=stPath;

		//Skip root entry, network share, device, nt notation or symlink prefix: "\", "\\", "\\.\", "\\?\", "\??\"
		//prefix "\" or "/"
		if (IsSlash(*pstPath))
		{
			pstPath++;
			nPrefix = PPT_ROOT;

			//prefix "\"
			if (IsSlashForward(pstPath[-1]))
			{
				//prefix "\\" - network
				if (IsSlashForward(pstPath[0]))
				{
					pstPath++;
					nPrefix = PPT_PREFIX;

					//prefix "\\.\" - device
					if (IsDot(pstPath[0]) && IsSlashForward(pstPath[1]))
					{
						pstPath += 2;
					}
					else
					{
						//prefix "\\?\" - nt notation
						if (IsQuestion(pstPath[0]) && IsSlashForward(pstPath[1]))
						{
							pstPath += 2;
							nPrefix = PPT_NT;

							//prefix "\\?\UNC\" - nt notation (UNC)
							if (!StrCmpN(pstPath, L"UNC/", 4))
							{
								pstPath += 4;
							}
						}
					}
				}
				else
				{
					if (IsQuestion(pstPath[0]) && IsQuestion(pstPath[1]) && IsSlashForward(pstPath[2]))    //prefix "\??\" symlink
					{
						pstPath += 3;
						nPrefix = PPT_NT;
					};
				}
			}
		}

		//Skip path to next slash (or path end) if was "special" prefix
		if (nPrefix == PPT_PREFIX || nPrefix == PPT_NT)
		{
			while (*pstPath)
			{
				if (IsSlash(*pstPath))
				{
					pstPath++;
					break;
				}

				pstPath++;
			}
		}
		else
		{
			//Skip logical drive letter name
			if (pstPath[0] && IsColon(pstPath[1]))
			{
				pstPath += 2;
				nPrefix = PPT_DRIVE;

				//Skip root slash
				if (IsSlash(*pstPath))
				{
					pstPath++;
					nPrefix = PPT_PREFIX;
				}
			}
		}

		PathOffset=pstPath-stPath;
		return (nPrefix);
	}

	return (PPT_NONE);
}

void MixToFullPath(FARString& strPath)
{
	//Skip all path to root (with slash if exists)
	LPWSTR pstPath=strPath.GetBuffer();
	size_t PathOffset=0;
	Point2Root(pstPath,PathOffset);
	pstPath+=PathOffset;

	//Process "." and ".." if exists
	for (int m = 0; pstPath[m];)
	{
		//fragment "."
		if (IsDot(pstPath[m]) && (!m || IsSlash(pstPath[m - 1])))
		{
			LPCWSTR pstSrc;
			LPWSTR pstDst;

			switch (pstPath[m + 1])
			{
					//fragment ".\"
				case L'\\':
					//fragment "./"
				case L'/':
				{
					for (pstSrc = pstPath + m + 2, pstDst = pstPath + m; *pstSrc; pstSrc++, pstDst++)
					{
						*pstDst = *pstSrc;
					}

					*pstDst = 0;
					continue;
				}
				break;
				//fragment "." at the end
				case 0:
				{
					pstPath[m] = 0;
					continue;
				}
				break;
				//fragment "..\" or "../" or ".." at the end
				case L'.':
				{
					if (IsSlash(pstPath[m + 2]) || !pstPath[m + 2])
					{
						int n;

						//Calculate subdir name offset
						for (n = m - 2; (n >= 0) && (!IsSlash(pstPath[n])); n--);

						n = (n < 0) ? 0 : n + 1;

						//fragment "..\" or "../"
						if (pstPath[m + 2])
						{
							for (pstSrc = pstPath + m + 3, pstDst = pstPath + n; *pstSrc; pstSrc++, pstDst++)
							{
								*pstDst = *pstSrc;
							}

							*pstDst = 0;
						}
						//fragment ".." at the end
						else
						{
							pstPath[n] = 0;
						}

						m = n;
						continue;
					}
				}
				break;
			}
		}

		m++;
	}

	strPath.ReleaseBuffer();
}

bool MixToFullPath(LPCWSTR stPath, FARString& strDest, LPCWSTR stCurrentDir)
{
	size_t lPath=wcslen(NullToEmpty(stPath)),
	       lCurrentDir=wcslen(NullToEmpty(stCurrentDir)),
	       lFullPath=lPath+lCurrentDir;

	if (lFullPath > 0)
	{
		strDest.Clear();
		LPCWSTR pstPath = nullptr, pstCurrentDir = nullptr;
		bool blIgnore = false;
		size_t PathOffset=0;
		PATH_PFX_TYPE PathType=Point2Root(stPath,PathOffset);
		pstPath=stPath+PathOffset;

		switch (PathType)
		{
			case PPT_NONE: //"abc"
			{
				pstCurrentDir=stCurrentDir;
			}
			break;
			case PPT_DRIVE: //"C:" or "C:abc"
			{
				WCHAR DriveVar[]={L'=',*stPath,L':',L'\0'};
				FARString strValue;

				if (apiGetEnvironmentVariable(DriveVar,strValue))
				{
					strDest=strValue;
				}
				else
				{
					if (Upper(*stPath)==Upper(*stCurrentDir))
					{
						strDest=stCurrentDir;
					}
					else
					{
						strDest=DriveVar+1;
					}
				}

				AddEndSlash(strDest);
			}
			break;
			case PPT_ROOT: //"\" or "\abc"
			{
				if (stCurrentDir)
				{
					size_t PathOffset=0;

					if (Point2Root(stCurrentDir,PathOffset)!=PPT_NONE)
					{
						strDest=FARString(stCurrentDir,PathOffset);
					}
				}
			}
			break;
			case PPT_PREFIX: //"C:\abc"
			{
				pstPath=stPath;
			}
			break;
			case PPT_NT: //"\\?\abc"
			{
				blIgnore=true;
				pstPath=stPath;
			}
			break;
		}

		if (pstCurrentDir)
		{
			strDest+=pstCurrentDir;
			AddEndSlash(strDest);
		}

		if (pstPath)
		{
			strDest+=pstPath;
		}

		if (!blIgnore)
			MixToFullPath(strDest);

		return true;
	}

	return false;
}


/*
  Преобразует Src в полный РЕАЛЬНЫЙ путь с учетом reparse point.
  Note that Src can be partially non-existent.
*/
void ConvertNameToReal(const wchar_t *Src, FARString &strDest)
{
	if (*Src==GOOD_SLASH) {
		std::string s = Wide2MB(Src), cutoff;
		char buf[PATH_MAX + 1];
		for (;;) {
			if (sdc_realpath(s.c_str(), buf)) {
				buf[sizeof(buf)-1] = 0;
				if (strcmp(buf, s.c_str())!=0) {
					strDest = buf;
					if (!cutoff.empty())
						strDest.Append(cutoff.c_str());
					return;
				}
				break;
			}
			
			size_t p = s.rfind(GOOD_SLASH);
			if (p==std::string::npos || p==0) break;
			cutoff.insert(0, s.c_str() + p);
			s.resize(p);
		}
	}
	strDest = Src;
}

// Косметические преобразования строки пути.
// CheckFullPath используется в FCTL_SET[ANOTHER]PANELDIR
FARString& PrepareDiskPath(FARString &strPath, bool CheckFullPath)
{
	// elevation not required during cosmetic operation 

	if (!strPath.IsEmpty())
	{
		if (strPath.At(1)==L':' || (strPath.At(0)==L'/' && strPath.At(1)==L'/'))
		{
			bool DoubleSlash = strPath.At(1)==L'/';
			while(ReplaceStrings(strPath,L"//",L"/"));
			if(DoubleSlash)
			{
				strPath = "/"+strPath;
			}

			if (CheckFullPath)
			{
				ConvertNameToFull(strPath,strPath);
				size_t FullLen=strPath.GetLength();
				wchar_t *lpwszPath=strPath.GetBuffer(),*Src=lpwszPath;

				if (IsLocalPath(lpwszPath))
				{
					Src+=2;

					if (IsSlash(*Src))
						Src++;
				}

				if (*Src)
				{
					wchar_t *Dst=Src;

					for (wchar_t c=*Src; ; Src++,c=*Src)
					{
						if (IsSlash(c) || (!c && !IsSlash(*(Src-1))))
						{
							*Src=0;
							FAR_FIND_DATA_EX fd;
							BOOL find=apiGetFindDataEx(lpwszPath,fd);
							*Src=c;

							if (find)
							{
								size_t n=fd.strFileName.GetLength();
								int n1=(int)(n-(Src-Dst));

								if (n1>0)
								{
									size_t dSrc=Src-lpwszPath,dDst=Dst-lpwszPath;
									strPath.ReleaseBuffer(FullLen);
									lpwszPath=strPath.GetBuffer(FullLen+n1+1);
									Src=lpwszPath+dSrc;
									Dst=lpwszPath+dDst;
								}

								if (n1)
								{
									wmemmove(Src+n1,Src,FullLen-(Src-lpwszPath)+1);
									Src+=n1;
									FullLen+=n1;
								}

								wmemcpy(Dst,fd.strFileName,n);
							}

							if (c)
							{
								Dst=Src+1;
							}
						}

						if (!*Src)
							break;
					}
				}

				strPath.ReleaseBuffer(FullLen);
			}

			wchar_t *lpwszPath = strPath.GetBuffer();

			if (lpwszPath[0]==L'/' && lpwszPath[1]==L'/')
			{
				if (IsLocalPrefixPath(lpwszPath))
				{
					lpwszPath[4] = Upper(lpwszPath[4]);
				}
				else
				{
					wchar_t *ptr=&lpwszPath[2];

					while (*ptr && !IsSlash(*ptr))
					{
						*ptr=Upper(*ptr);
						ptr++;
					}
				}
			}
			else
			{
				lpwszPath[0]=Upper(lpwszPath[0]);
			}

			strPath.ReleaseBuffer(strPath.GetLength());
		}
	}

	return strPath;
}

void GetPathRoot(const wchar_t *Path, FARString &strRoot)
{
	FARString RealPath;
	ConvertNameToReal(Path, RealPath);
	strRoot = ExtractPathRoot(RealPath);
}

void ConvertNameToFull(const wchar_t *lpwszSrc, FARString &strDest)
{
	FARString strCurDir;
	apiGetCurrentDirectory(strCurDir);
	FARString strSrc = lpwszSrc;
	MixToFullPath(strSrc,strDest,strCurDir);
}

