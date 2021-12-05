#include "headers.hpp"


#include "Mounts.hpp"
#include "lang.hpp"
#include "language.hpp"
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

namespace Mounts
{
//	static FARString strRootPath(L"/"), strRootName(L"&/");
//	static FARString strHomeName(L"&~"), strSameName(L"&-");
//	static FARString strRootPath(L"/"), strRootName(L"&/");
//	static FARString strHomeName(L"&~"), strSameName(L"&-");
	#define HOTKEYS_SECTION "MountsHotkeys"

	const wchar_t GetDefaultHotKey(const FARString &path, const wchar_t *another_curdir = nullptr)
	{
		if (path == L"/")
			return L'/';

		if (path == GetMyHome())
			return L'~';

		if (another_curdir && path == another_curdir)
			return L'-';

		return 0;
	}

	Enum::Enum(FARString &another_curdir)
	{
		std::string cmd = GetMyScriptQuoted("mounts.sh");
		cmd+= " enum";

		bool has_rootfs = false;

		FILE *f = popen(cmd.c_str(), "r");
		if (f) {
			char buf[0x2100] = { };
			std::wstring s, tmp;
			while (fgets(buf, sizeof(buf) - 1, f)!=NULL) {
				for (;;) {
					size_t l = strlen(buf);
					if (!l) break;
					if (buf[l-1]!='\r' && buf[l-1]!='\n') break;
					buf[l-1] = 0;
				}
				if (buf[0]) {
					emplace_back();
					auto &e = back();
					e.path.Copy(&buf[0]);
					size_t t;
					if (e.path.Pos(t, L'\t')) {
						e.info = e.path.SubStr(t + 1);
						e.path.Truncate(t);
						if (e.info.Pos(t, L'\t')) {
							e.usage = e.info.SubStr(0, t);
							e.info = e.info.SubStr(t + 1);
						}
					}
					if (e.path == L"/") {
						has_rootfs = true;
					} else {
						e.unmountable = true;
					}
				}
			}
			int r = pclose(f);
			if (r != 0) {
				fprintf(stderr, "Exit code %u executing '%s'\n", r, cmd.c_str());
			}
		} else {
			fprintf(stderr, "Error %u executing '%s'\n", errno, cmd.c_str());
		}

		if (!has_rootfs) {
			emplace(begin(), Entry(L"/", MSG(MMountsRoot)));
		}

		emplace(begin(), Entry( GetMyHome(), MSG(MMountsHome)));
		emplace(begin(), Entry( another_curdir, MSG(MMountsOther)));

		ConfigReader cfg_reader(HOTKEYS_SECTION);
		for (auto &m : *this) {
			if (max_path < m.path.GetLength())
				max_path = m.path.GetLength();
			if (max_info < m.info.GetLength())
				max_info = m.info.GetLength();
			if (max_usage < m.usage.GetLength())
				max_usage = m.usage.GetLength();
			wchar_t def_hk[] = {GetDefaultHotKey(m.path, another_curdir), 0};
			auto hk = cfg_reader.GetString(m.path.GetMB(), def_hk);
			m.hotkey = hk.IsEmpty() ? 0 : *hk.CPtr();
		}
	}

	bool Unmount(const FARString &path, bool force)
	{
		std::string cmd = GetMyScriptQuoted("mounts.sh");
		cmd+= " umount \"";
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

	void EditHotkey(const FARString &path)
	{
		wchar_t def_hk[] = {GetDefaultHotKey(path), 0};
		const auto &Setting = ConfigReader(HOTKEYS_SECTION).GetString(path.GetMB(), def_hk);
		WCHAR Letter[2] = {Setting.IsEmpty() ? 0 : Setting[0], 0};
		if (!HotkeyLetterDialog(MSG(MLocationHotKeyTitle), path.CPtr(), Letter[0]))
			return;

		ConfigWriter cw(HOTKEYS_SECTION);
		if (Letter[0])
			cw.SetString(path.GetMB(), Letter);
		else
			cw.RemoveKey(path.GetMB());
	}
}
