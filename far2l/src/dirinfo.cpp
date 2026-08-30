/*
dirinfo.cpp

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

#include "headers.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "dirinfo.hpp"
#include "plugapi.hpp"
#include "keys.hpp"
#include "scantree.hpp"
#include "savescr.hpp"
#include "lang.hpp"
#include "RefreshFrameManager.hpp"
#include "TPreRedrawFunc.hpp"
#include "ctrlobj.hpp"
#include "filefilter.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "constitle.hpp"
#include "keyboard.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "wakeful.hpp"
#include "config.hpp"

class ScannedINodes
{
	struct Hash
	{
		size_t operator()(const std::pair<uint64_t, uint64_t> &inode) const
		{
			size_t out = static_cast<size_t>(inode.first);
			out^= static_cast<size_t>(inode.second + 0x9e3779b97f4a7c15ULL + (inode.first << 6)
					+ (inode.first >> 2));
			return out;
		}
	};

	std::unordered_map<std::pair<uint64_t, uint64_t>, bool, Hash> _s;

public:
	ScannedINodes()
	{
		_s.reserve(1024);
	}

	inline bool Visit(uint64_t d, uint64_t ino, bool due_symlink = false)
	{
		const auto &ir = _s.emplace(std::make_pair(d, ino), !due_symlink);
		if (ir.second) {
			return true;
		}
		if (!due_symlink) {
			ir.first->second = true;
		}
		return false;
	}

	std::pair<size_t, size_t> ExtraStats() const
	{
		std::unordered_set<uint64_t> devs;
		size_t outer_symlinks = 0;
		for (const auto &it : _s) {
			devs.emplace(it.first.first);
			if (!it.second) {
				outer_symlinks++;
			}
		}
		return std::make_pair(devs.size(), outer_symlinks);
	}
};

struct ExtraSummaryCollector
{
	std::unordered_map<std::wstring, DirInfoTypeStats> _type2stats;
	std::wstring _type;

	void Add(const wchar_t *name, DWORD attrs, uint64_t size)
	{
		if (attrs == 0xffffffff) {
			_type = L"BAD";
		} else if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
			_type = (attrs & FILE_ATTRIBUTE_BROKEN) ? L"BAD-LINK" : L"LINK";
		} else if (attrs & FILE_ATTRIBUTE_DEVICE) {
			_type = L"DEV";
			size = 0;
		} else if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			_type = L"DIR";
		} else {
			// foobar.so -> ".so"
			// foobar.tar.gz -> ".tar.gz"
			// foobar.so.1.2 -> ".so"
			// foobar.xxx.yyy -> ".yyy"
			// foobar.xxx. -> ".xxx"
			// foobar -> "NOEXT"
			// foobar. -> "NOEXT"
			// .gitignore -> "DOTFILE"
			const wchar_t *dot = nullptr, *end = name + wcslen(name);
			bool digits_only = true;
			for (const wchar_t *p = end; p != name;) {
				--p;
				if (*p == '.') {
					if (digits_only) {
						end = p;
					}
					dot = p;
				} else if (*p < '0' || *p > '9') {
					digits_only = false;
				}
				if (*p == '/') {
					break;
				}
			}
			if (dot && dot + 1 < end) {
				if (dot != name && dot[-1] != '/') {
					_type.assign(dot, end);
					while (_type.size() > 1) {
						auto p =_type.find('.', 1);
						if (p == std::wstring::npos || p + 1 == _type.size()) {
							break;
						}
						if (p == 4 && wcsncasecmp(_type.c_str(), L".tar", 4) == 0) {
							break;
						}
						_type.erase(0, p);
					}
					for (auto &c : _type) {
						if (c >= L'A' && c <= L'Z') {
							c+= L'a' - L'A';
						}
					}
				} else {
					_type = L"DOTFILE";
				}
			} else {
				_type = L"NOEXT";
			}
		}
		auto &stats = _type2stats[_type];
		stats.Size+= size;
		stats.Count++;
	}

	std::unique_ptr<DirInfoExtraSummary> Summarize() const
	{
		auto out = std::make_unique<DirInfoExtraSummary>();
		for (const auto &ts : _type2stats) {
			out->type_stats.emplace_back(ts);
		}
		// sort by disk usage descending order
		std::sort(out->type_stats.begin(), out->type_stats.end(), [](const auto &a, const auto &b) {
		    if (a.second.Size != b.second.Size) {
				return a.second.Size > b.second.Size;
			}
		    if (a.second.Count != b.second.Count) {
				return a.second.Count > b.second.Count;
			}
			return a.first < b.first;
		});
		return out;
	}
};

int DirInfo::FromFS(const wchar_t *DirName, DWORD Flags, FileFilter *Filter, DirInfoProgressTracker *tracker)
{
	operator =(DirInfo{});
	std::optional<ExtraSummaryCollector> xsc;
	if (Flags & GETDIRINFO_EXTRASUMMARY) {
		xsc.emplace();
	}

	FARString strFullDirName;
	FARString strFullName, strCurDirName, strLastDirName;
	ConvertNameToFull(DirName, strFullDirName);
	SaveScreen SaveScr;
	UndoGlobalSaveScrPtr UndSaveScr(&SaveScr);
	wakeful W;
	ScanTree ScTree(FALSE, TRUE,
			((Flags & GETDIRINFO_SCANSYMLINKDEF) ? -1 : ((Flags & GETDIRINFO_SCANSYMLINK) != 0)));
	FAR_FIND_DATA_EX FindData;
	/*
		$ 20.03.2002 DJ
		для . - покажем имя родительского каталога
	*/
	FARString strShowDirName = DirName;

	if (DirName[0] == L'.' && !DirName[1]) {
		const wchar_t *p = LastSlash(strFullDirName);
		if (p)
			strShowDirName = p + 1;
	}

	std::optional<RefreshFrameManager> frref;
	if ( (Flags & GETDIRINFO_DONTREDRAWFRAME) == 0) {
		frref.emplace();
	}
	// DWORD SectorsPerCluster=0,BytesPerSector=0,FreeClusters=0,Clusters=0;

	// Временные хранилища имён каталогов
	strLastDirName.Clear();
	strCurDirName.Clear();
	ScTree.SetFindPath(DirName, L"*", 0);
	ScannedINodes scanned_inodes;
	const bool count_dir_size = !Opt.OnlyFilesSize;
	const bool scan_symlinks = ScTree.IsSymlinksScanEnabled();
	const bool can_break = !CtrlObject->Macro.IsExecuting() && !WinPortTesting();

	if (count_dir_size) {	// include size of root dir's node
		struct stat s{};
		if (sdc_stat(Wide2MB(DirName).c_str(), &s) == 0) {
			FileSize = s.st_size;
			PhysicalSize = ((DWORD64)s.st_blocks) * 512;
		}
	}

	clock_t LastUpdateTime = 0;
	while (ScTree.GetNextName(&FindData, strFullName)) {
		clock_t CurTime = GetProcessUptimeMSec();
		if ( (can_break || tracker) && CurTime - LastUpdateTime > 100) {
			INPUT_RECORD rec{};
			if (tracker) {
				tracker->OnDirInfoProgress(strShowDirName);
			}
			if (can_break) switch (PeekInputRecord(&rec)) {
				case 0:
				case KEY_IDLE:
					LastUpdateTime = CurTime;
					break;
				case KEY_NONE:
					if ((Flags & GETDIRINFO_ENHBREAK) != 0 && rec.EventType == MOUSE_EVENT
							&& rec.Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
						// !!! Its a workaround.
						// TODO: fix PeekInputRecord - it should return KEY_MSWHEEL_UP/KEY_MSWHEEL_DOWN in such case
						return -1;
					}
				case KEY_ALT:
				case KEY_CTRL:
				case KEY_SHIFT:
				case KEY_RALT:
				case KEY_RCTRL:
					GetInputRecord(&rec);
					break;
				case KEY_ESC:
				case KEY_BREAK:
					GetInputRecord(&rec);
					return 0;
				default:

					if (Flags & GETDIRINFO_ENHBREAK) {
						return -1;
					}

					GetInputRecord(&rec);
					break;
			}
		}

		const DWORD file_attributes = FindData.dwFileAttributes;
		const bool is_directory = (file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		const bool is_reparse_point = (file_attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

		if (is_directory) {
			/*
				Если каталог не попадает под фильтр то его надо полностью
				пропустить - иначе при включенном подсчёте total
				он учтётся (mantis 551)
			*/
			if ((is_reparse_point && !scan_symlinks) || (Filter && !Filter->FileInFilter(FindData))) {
				ScTree.SkipDir();
				continue;
			}
			/*
				Счётчик каталогов наращиваем только если не включен фильтр,
				в противном случае это будем делать в подсчёте количества файлов
			*/
			if (!Filter) {
				DirCount++;
			}
			if (tracker) {
				strShowDirName = strFullName;
			}
		} else {
			if (Filter) {
				/*
					$ 17.04.2005 KM
					Проверка попадания файла в условия фильра
				*/
				if (!Filter->FileInFilter(FindData)) {
					continue;
				}

				/*
					Наращиваем счётчик каталогов при включенном фильтре только тогда,
					когда в таком каталоге найден файл, удовлетворяющий условиям
					фильтра.
				*/
				strCurDirName = strFullName;
				CutToSlash(strCurDirName);	//???

				if (strCurDirName != strLastDirName) {
					DirCount++;
					strLastDirName = strCurDirName;
				}
			}

			FileCount++;
		}

		if (!is_directory || count_dir_size) {
			if (is_reparse_point) {
				struct stat s{};
				if (sdc_lstat(strFullName.GetMB().c_str(), &s) == 0) {
					if (scanned_inodes.Visit(s.st_dev, s.st_ino, false)) {
						FileSize+= s.st_size;
						PhysicalSize+= ((DWORD64)s.st_blocks) * 512;
						if (xsc) {
							xsc->Add(FindData.strFileName, FindData.dwFileAttributes, ((DWORD64)s.st_blocks) * 512);
						}
					}
				}
			}
			if (scanned_inodes.Visit(FindData.UnixDevice, FindData.UnixNode, is_reparse_point)) {
				FileSize+= FindData.nFileSize;
				PhysicalSize+= FindData.nPhysicalSize;
				if (xsc) {
					xsc->Add(FindData.strFileName,
						FindData.dwFileAttributes & (~FILE_ATTRIBUTE_REPARSE_POINT),
						FindData.nPhysicalSize);
				}
			}
		}
	}

	if (xsc) {
		ExtraSummary = xsc->Summarize();
		const auto &xs = scanned_inodes.ExtraStats();
		ExtraSummary->filesystems = xs.first;
		ExtraSummary->outer_symlinks = xs.second;
	}

	return 1;
}

int DirInfo::FromPlugin(HANDLE hPlugin, const wchar_t *DirName, DWORD Flags)
{
	operator = ({});
	std::optional<ExtraSummaryCollector> xsc;
	if (Flags & GETDIRINFO_EXTRASUMMARY) {
		xsc.emplace();
	}

	PluginPanelItem *PanelItem = nullptr;
	int ItemsNumber, ExitCode;
	PluginHandle *ph = (PluginHandle *)hPlugin;

	if ((ExitCode = FarGetPluginDirList((INT_PTR)ph->pPlugin, ph->hPlugin, DirName, &PanelItem, &ItemsNumber))
			== TRUE)	// INT_PTR - BUGBUG
	{
		for (int I = 0; I < ItemsNumber; I++) {
			auto &FindData = PanelItem[I].FindData;
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DirCount++;
			} else {
				FileCount++;
				FileSize+= FindData.nFileSize;
				PhysicalSize+= FindData.nPhysicalSize ? FindData.nPhysicalSize : FindData.nFileSize;
			}
			if (xsc) {
				xsc->Add(FindData.lpwszFileName, FindData.dwFileAttributes,
					(FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
						? 0 : (FindData.nPhysicalSize ? FindData.nPhysicalSize : FindData.nFileSize)
				);
			}
		}
	}

	if (PanelItem)
		FarFreePluginDirList(PanelItem, ItemsNumber);

	if (xsc) {
		ExtraSummary = xsc->Summarize();
	}
	return (ExitCode);
}
