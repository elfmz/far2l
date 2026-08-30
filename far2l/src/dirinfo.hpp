#pragma once

/*
dirinfo.hpp

GetDirInfo & GetPluginDirInfo
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

class FileFilter;

enum GETDIRINFOFLAGS
{
	GETDIRINFO_ENHBREAK        = 0x00000001,
	GETDIRINFO_DONTREDRAWFRAME = 0x00000002,
	GETDIRINFO_SCANSYMLINK     = 0x00000004,
	GETDIRINFO_SCANSYMLINKDEF  = 0x00000008,
	GETDIRINFO_EXTRASUMMARY    = 0x00000020,
};

struct DirInfoProgressTracker
{
	virtual void OnDirInfoProgress(const wchar_t *WalkedNowDir) = 0;
};

struct DirInfoTypeStats
{
	uint64_t Count{};
	uint64_t Size{};
};

struct DirInfoExtraSummary
{
	std::vector<std::pair<std::wstring, DirInfoTypeStats>> type_stats;
	uint64_t filesystems{0};
	uint64_t outer_symlinks{0};
};

struct DirInfo
{
	uint64_t FileSize{};
	uint64_t PhysicalSize{};
	uint64_t DirCount{};
	uint64_t FileCount{};

	std::unique_ptr<DirInfoExtraSummary> ExtraSummary;

	int FromFS(const wchar_t *DirName, DWORD Flags = GETDIRINFO_SCANSYMLINKDEF, FileFilter *Filter = nullptr, DirInfoProgressTracker *tracker = nullptr);
	int FromPlugin(HANDLE hPlugin, const wchar_t *DirName, DWORD Flags = GETDIRINFO_SCANSYMLINKDEF);
};

