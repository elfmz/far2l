#include "headers.hpp"

#include <crc64.h>
#include <fstream>
#include "Mounts.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "help.hpp"
#include "vmenu.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "execute.hpp"
#include "manager.hpp"
#include "ConfigRW.hpp"
#include "HotkeyLetterDialog.hpp"
#include "MountInfo.h"

namespace Mounts
{
	#define HOTKEYS_SECTION "MountsHotkeys"
	#define ID_ROOT    0
	#define ID_HOME    1
	#define ID_ANOTHER 2

	static wchar_t DefaultHotKey(int id, const FARString &path)
	{
		switch (id) {
			case ID_ROOT:
				return L'/';

			case ID_HOME:
				return L'~';

			case ID_ANOTHER:
				return L'-';
		}

		return 0;
	}

	static std::string SettingsKey(int id)
	{
		return StrPrintf("%08x", id);
	}

	static int GenerateIdFromPath(const FARString &path)
	{
		if (path == WGOOD_SLASH)
			return ID_ROOT;

		const std::string &path_mb = path.GetMB();
		const uint64_t id64 = crc64(0, (unsigned char *)path_mb.c_str(), path_mb.size());
		uint32_t out = (uint32_t)(id64 ^ (id64 >> 32));
		if (out == ID_ROOT || out == ID_HOME || out == ID_ANOTHER) {
			out^= 0xffffffff;
		}
		return (int)out;
	}

	/////

	Enum::Enum(FARString &another_curdir)
	{
		bool has_rootfs = false;
		if (Opt.ChangeDriveMode & DRIVE_SHOW_MOUNTS)
		{
			AddMounts(has_rootfs);
		}
		AddFavorites(has_rootfs);

		if (!has_rootfs) {
			emplace(begin(), Entry(WGOOD_SLASH, Msg::MountsRoot, false, ID_ROOT));
		}

		emplace(begin(), Entry( GetMyHome(), Msg::MountsHome, false, ID_HOME));
		emplace(begin(), Entry( another_curdir, Msg::MountsOther, false, ID_ANOTHER));

		ConfigReader cfg_reader(HOTKEYS_SECTION);
		for (auto &m : *this) {
			if (max_path < m.path.CellsCount())
				max_path = m.path.CellsCount();
			if (max_col3 < m.col3.CellsCount())
				max_col3 = m.col3.CellsCount();
			if (max_col2 < m.col2.CellsCount())
				max_col2 = m.col2.CellsCount();
			wchar_t def_hk[] = {DefaultHotKey(m.id, m.path), 0};
			auto hk = cfg_reader.GetString(SettingsKey(m.id), def_hk);
			m.hotkey = hk.IsEmpty() ? 0 : *hk.CPtr();
		}
	}

	static void ExpandMountpointInfo(const Mountpoint &mp, FARString &str)
	{
		FARString val = L" ";
		if (mp.bad) {
			val = L"?";
		} else if (mp.read_only) {
			val = L"!";
		}
		ReplaceStrings(str, L"$S", val);

		FileSizeToStr(val, mp.total, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX);
		ReplaceStrings(str, L"$T", val);

		FileSizeToStr(val, mp.avail, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX);
		ReplaceStrings(str, L"$A", val);

		FileSizeToStr(val, mp.free_, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX);
		ReplaceStrings(str, L"$F", val);

		FileSizeToStr(val, mp.total - mp.free_, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX);
		ReplaceStrings(str, L"$U", val);

		if (mp.total)
			val.Format(L"%lld", (mp.avail * 100) / mp.total);
		else
			val = L"NA";
		ReplaceStrings(str, L"$a", val);

		if (mp.total)
			val.Format(L"%lld", (mp.free_ * 100) / mp.total);
		else
			val = L"NA";
		ReplaceStrings(str, L"$f", val);

		if (mp.total)
			val.Format(L"%lld", ((mp.total - mp.free_) * 100) / mp.total);
		else
			val = L"NA";
		ReplaceStrings(str, L"$u", val);

		val = mp.filesystem;
		ReplaceStrings(str, L"$N", val);

		val = mp.device;
		ReplaceStrings(str, L"$D", val);
	}

	class Aligner
	{
		std::vector<size_t> _maximums;

		static bool IsWordDiv(wchar_t c)
		{
			return !iswalnum(c) && c != L'.' && c != L',';
		}

		static size_t LeftWordLength(const FARString &str, size_t pos)
		{
			ssize_t out = pos;
			while (out >= 0 && !IsWordDiv(str.At(out))) {
				--out;
			}
			return ssize_t(pos) - out;
		}

		static size_t RightWordLength(const FARString &str, size_t pos)
		{
			size_t out = pos;
			while (out < str.GetLength() && !IsWordDiv(str.At(out))) {
				++out;
			}
			return out - pos;
		}

	public:
		void Analyze(const FARString &str)
		{
			size_t m = 0;
			const int str_len = str.GetLength();
			for (int i = str_len - 2; i >= 0; --i) {
				if (str.At(i) == '$' && ((str.At(i + 1) == '<' && i >= 0)
						|| (str.At(i + 1) == '>' && i + 2 <= str_len))
					) {
					const size_t len = (str.At(i + 1) == '>')
						? RightWordLength(str, i + 2) : LeftWordLength(str, i - 1);
					if (_maximums.size() <= m) {
						_maximums.emplace_back(len);
					} else if (_maximums[m] < len) {
						_maximums[m] = len;
					}
					++m;
				}
			}
		}

		void Apply(FARString &str)
		{
			size_t m = 0;
			const int str_len = str.GetLength();
			for (int i = str_len - 2; i >= 0; --i) {
				if (str.At(i) == '$' && ((str.At(i + 1) == '<' && i >= 0)
						|| (str.At(i + 1) == '>' && i + 2 <= str_len))
					) {
					const size_t len = (str.At(i + 1) == '>')
						? RightWordLength(str, i + 2) : LeftWordLength(str, i - 1);
					str.Replace(i, 2, L' ', _maximums[m] - len);
					++m;
				}
			}
		}
	};

	void Enum::AddMounts(bool &has_rootfs)
	{
		MountInfo mi(true);
		for (const auto &mp : mi.Enum()) {
			emplace_back();
			auto &e = back();
			e.path = mp.path;
			e.col2 = Opt.ChangeDriveColumn2;
			e.col3 = Opt.ChangeDriveColumn3;
			ExpandMountpointInfo(mp, e.col2);
			ExpandMountpointInfo(mp, e.col3);
			
			if (e.path == WGOOD_SLASH) {
				has_rootfs = true;
			} else {
				e.unmountable = true;
			}
			e.id = GenerateIdFromPath(e.path);
		}

		// apply replace $> and $< spacers
		Aligner al_path, al_col2, al_col3;
		for (const auto &e : *this) {
			al_path.Analyze(e.path);
			al_col2.Analyze(e.col2);
			al_col3.Analyze(e.col3);
		}
		for (auto &e : *this) {
			al_path.Apply(e.path);
			al_col2.Apply(e.col2);
			al_col3.Apply(e.col3);
		}
	}

	void Enum::AddFavorites(bool &has_rootfs)
	{
		std::ifstream favis(InMyConfig("favorites"));
		if (favis.is_open()) {
			std::string line;
			while (std::getline(favis, line)) {
				StrTrim(line, " \t\r\n");
				if (line.empty() || line.front() == '#') {
					continue;
				}
				Environment::ExpandString(line, true, true);
				std::vector<std::string> sublines;
				StrExplode(sublines, line, "\n");
				for (const auto &subline : sublines) {
					std::vector<std::string> parts;
					StrExplode(parts, subline, "\t");
					if (!parts.empty()) {
						emplace_back();
						auto &e = back();
						e.path = parts.front();
						if (parts.size() > 1) {
							e.col3 = parts.back();
							if (parts.size() > 2) {
								e.col2 = parts[1];
							}
						}
						e.id = GenerateIdFromPath(e.path);
						if (e.path == WGOOD_SLASH) {
							has_rootfs = true;

						} else if (*e.path.CPtr() == L'/') {
							e.unmountable = true;
						}
					}
				}
			}
		}
	}
	//////

	bool Unmount(const FARString &path, bool force)
	{
		std::string cmd = GetMyScriptQuoted("unmount.sh");
		cmd+= " \"";
		cmd+= EscapeCmdStr(Wide2MB(path));
		cmd+= "\"";
		if (force) {
			cmd+= " force";
		}
		int r = farExecuteA(cmd.c_str(), 0);
		if (FrameManager) {
			auto *current_frame = FrameManager->GetCurrentFrame();
			if (current_frame) {
				FrameManager->RefreshFrame(current_frame);
			}
		}
		return r == 0;
	}

	void EditHotkey(const FARString &path, int id)
	{
		wchar_t def_hk[] = {DefaultHotKey(id, path), 0};
		const auto &Key = SettingsKey(id);
		const auto &Setting = ConfigReader(HOTKEYS_SECTION).GetString(Key, def_hk);
		WCHAR Letter[2] = {Setting.IsEmpty() ? 0 : Setting[0], 0};
		if (HotkeyLetterDialog(Msg::LocationHotKeyTitle, path.CPtr(), Letter[0])) {
			ConfigWriter cw(HOTKEYS_SECTION);
			if (Letter[0])
				cw.SetString(Key, Letter);
			else
				cw.RemoveKey(Key);
		}
	}
}
