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
		if (path == L"/")
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
		MountInfo mi(true);

		bool has_rootfs = false;
		for (const auto &mp : mi.Enum()) {
			emplace_back();
			auto &e = back();
			e.path = mp.path;
			e.info = mp.filesystem;
			if (mp.bad) {
				e.usage = Opt.NoGraphics ? L"X_X" : L"❌_❌";

			} else {
				FileSizeToStr(e.usage, mp.avail, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX); //COLUMN_AUTOSIZE | COLUMN_SHOWBYTESINDEX
				while (e.usage.GetLength() < 4) {
					e.usage.Insert(0, L' ');
				}
				FARString tmp;
				FileSizeToStr(tmp, mp.total, -1, COLUMN_ECONOMIC | COLUMN_FLOATSIZE | COLUMN_SHOWBYTESINDEX); //COLUMN_AUTOSIZE | COLUMN_SHOWBYTESINDEX
				while (e.usage.GetLength() < 4) {
					tmp.Insert(0, L' ');
				}
				e.usage+= L"/";
				e.usage+= tmp;
			}

			if (e.path == L"/") {
				has_rootfs = true;
			} else {
				e.unmountable = true;
			}
			e.id = GenerateIdFromPath(e.path);
		}

		std::ifstream favis(InMyConfig("favorites"));
		if (favis.is_open()) {
			std::string line;
			while (std::getline(favis, line)) {
				StrTrim(line, " \t\r\n");
				if (line.empty() || line.front() == '#') {
					continue;
				}
				std::vector<std::string> parts;
				StrExplode(parts, line, "\t");
				if (!parts.empty()) {
					emplace_back();
					auto &e = back();
					e.path = parts.front();
					if (parts.size() > 1) {
						e.info = parts.back();
						if (parts.size() > 2) {
							e.usage = parts[1];
						}
					}
					e.id = GenerateIdFromPath(e.path);
					if (e.path == L"/") {
						has_rootfs = true;

					} else if (*e.path.CPtr() == L'/') {
						e.unmountable = true;
					}
				}
			}
		}

		if (!has_rootfs) {
			emplace(begin(), Entry(L"/", Msg::MountsRoot, false, ID_ROOT));
		}

		emplace(begin(), Entry( GetMyHome(), Msg::MountsHome, false, ID_HOME));
		emplace(begin(), Entry( another_curdir, Msg::MountsOther, false, ID_ANOTHER));

		ConfigReader cfg_reader(HOTKEYS_SECTION);
		for (auto &m : *this) {
			if (max_path < m.path.GetLength())
				max_path = m.path.GetLength();
			if (max_info < m.info.GetLength())
				max_info = m.info.GetLength();
			if (max_usage < m.usage.GetLength())
				max_usage = m.usage.GetLength();
			wchar_t def_hk[] = {DefaultHotKey(m.id, m.path), 0};
			auto hk = cfg_reader.GetString(SettingsKey(m.id), def_hk);
			m.hotkey = hk.IsEmpty() ? 0 : *hk.CPtr();
		}
	}

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
