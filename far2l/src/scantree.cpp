/*
scantree.cpp

Сканирование текущего каталога и, опционально, подкаталогов на
предмет имен файлов
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

#include "scantree.hpp"
#include "syslog.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "processname.hpp"

ScanTree::ScanTree(int RetUpDir, int Recurse, int ScanJunction)
{
	Flags.Change(FSCANTREE_RETUPDIR, RetUpDir);
	Flags.Change(FSCANTREE_RECUR, Recurse);
	Flags.Change(FSCANTREE_SCANSYMLINK, (ScanJunction == -1 ? Opt.ScanJunction : ScanJunction));
}

void ScanTree::SetFindPath(const wchar_t *Path, const wchar_t *Mask, const DWORD NewScanFlags)
{
	Flags.Flags = (Flags.Flags & 0x0000FFFF) | (NewScanFlags & 0xFFFF0000);
	strFindPath = *Path ? Path : L".";
	strFindMask = wcscmp(Mask, L"*") ? Mask : L"";
	ScanDirStack.clear();

	if (strFindPath != WGOOD_SLASH) {
		DeleteEndSlash(strFindPath);
	}

	ScanDirStack.emplace_back();
	ConvertNameToReal(strFindPath.c_str(), ScanDirStack.back().RealPath);

	StartEnumSubdir();
}

void ScanTree::CheckForEnterSubdir(const FAR_FIND_DATA_EX *fdata)
{
	if ((fdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || !Flags.Check(FSCANTREE_RECUR))
		return;

	if ((fdata->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 && !Flags.Check(FSCANTREE_SCANSYMLINK))
		return;

	const bool InsideSymlink =
			(fdata->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 || IsInsideSymlink();

	ScanDirStack.emplace_back();
	strFindPath.append(fdata->strFileName.CPtr(), fdata->strFileName.GetLength());

	if (fdata->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		ConvertNameToReal(strFindPath.c_str(), ScanDirStack.back().RealPath);

		// check if converted path points to same location is already scanned or to parent path of already scanned location
		// NB: in original FAR here was exact-match check all paths (not only symlinks)
		// that caused excessive scan from FS root cuz in Linux links pointing to / are usual situation unlike Windows
		// NB2: There're two recursive protection checks, excessive on first look:
		// Check by path: if real path is equal OR represents parent of some previously scanned path
		// Check by inode: inode matches some previously scanned inode
		// while check-by-path 'includes' check-by-node functionality, later is needed cuz in some
		// complex cases realpath() fails with error like ELOOP fails resulting path be unresolved,
		// so check-by-inode works as last resort to avoid unlimited recursion scan in such cases.
		const auto &RealPath = ScanDirStack.back().RealPath;
		for (auto it = ScanDirStack.rbegin();;) {
			if (ScanDirStack.rend() == ++it)
				break;
			const auto &IthPath = it->RealPath;
			if ((it->UnixDevice == fdata->UnixDevice && it->UnixNode == fdata->UnixNode)
					|| (IthPath.Begins(RealPath)
							&& (IthPath.GetLength() == RealPath.GetLength() || IthPath.At(RealPath.GetLength()) == GOOD_SLASH
									|| RealPath.GetLength() == 1))) {	// recursion! revert state changes made so far and bail out
				ScanDirStack.pop_back();
				strFindPath.resize(strFindPath.size() - fdata->strFileName.GetLength());
				return;
			}
		}
	} else
		ScanDirStack.back().RealPath = strFindPath;

	ScanDirStack.back().UnixDevice = fdata->UnixDevice;
	ScanDirStack.back().UnixNode = fdata->UnixNode;
	ScanDirStack.back().InsideSymlink = InsideSymlink;

	StartEnumSubdir();
}

void ScanTree::StartEnumSubdir()
{
	if (!strFindPath.empty() && strFindPath.back() != LGOOD_SLASH)
		strFindPath+= LGOOD_SLASH;

	DWORD WinPortFindFlags = 0;
	if (Flags.Check(FSCANTREE_NOLINKS))
		WinPortFindFlags|= FIND_FILE_FLAG_NO_LINKS;
	if (Flags.Check(FSCANTREE_NOFILES))
		WinPortFindFlags|= FIND_FILE_FLAG_NO_FILES;
	if (Flags.Check(FSCANTREE_NODEVICES))
		WinPortFindFlags|= FIND_FILE_FLAG_NO_DEVICES;
	if (Flags.Check(FSCANTREE_CASE_INSENSITIVE))
		WinPortFindFlags|= FIND_FILE_FLAG_CASE_INSENSITIVE;

	strFindPath+= L'*';		// append temporary asterisk

	ScanDirStack.back().Enumer.reset(
			new FindFile(strFindPath.c_str(), Flags.Check(FSCANTREE_SCANSYMLINK), WinPortFindFlags));

	strFindPath.pop_back();		// strip asterisk
}

void ScanTree::LeaveSubdir()
{
	if (!ScanDirStack.empty()) {
		ScanDirStack.pop_back();
		size_t p = strFindPath.rfind(GOOD_SLASH, strFindPath.size() - 2);
		if (p != std::string::npos) {
			strFindPath.resize(p + 1);
		}

	} else
		fprintf(stderr, "ScanTree::LeaveSubdir() invoked on empty stack!\n");
}

bool ScanTree::ScanDir::GetNext(FAR_FIND_DATA_EX *fdata, bool FilesFirst)
{
	if (Enumer)
		for (;;) {
			if (!Enumer->Get(*fdata)) {
				Enumer.reset();
				break;
			}
			if (!FilesFirst || (fdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				return true;

			Postponed.emplace_back(std::move(*fdata));
		}

	if (!Postponed.empty()) {
		*fdata = std::move(Postponed.front());
		Postponed.pop_front();
		return true;
	}

	return false;
}

bool ScanTree::GetNextName(FAR_FIND_DATA_EX *fdata, FARString &strFullName)
{
	Flags.Clear(FSCANTREE_SECONDDIRNAME);

	for (;;) {
		if (ScanDirStack.empty())
			return false;

		if (!ScanDirStack.back().GetNext(fdata, Flags.Check(FSCANTREE_FILESFIRST))) {
			if (!Flags.Check(FSCANTREE_RETUPDIR) || ScanDirStack.size() == 1) {
				LeaveSubdir();
				continue;
			}

			strFullName = strFindPath;
			DeleteEndSlash(strFullName);
			apiGetFindDataEx(strFullName, *fdata);
			LeaveSubdir();
			Flags.Set(FSCANTREE_SECONDDIRNAME);
			return true;
		}

		const bool Matched = strFindMask.empty() || CmpName(strFindMask.c_str(), fdata->strFileName, false);
		if (Matched) {
			strFullName = strFindPath;
			strFullName+= fdata->strFileName;
		}

		CheckForEnterSubdir(fdata);

		if (Matched)
			return true;
	}
}

void ScanTree::SkipDir()
{
	LeaveSubdir();
}
