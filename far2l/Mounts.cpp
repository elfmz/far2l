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

namespace Mounts
{
	static FARString strRootPath(L"/"), strRootName(L"&/");
	static FARString strHomeName(L"&~"), strSameName(L"&-");

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
						e.path.SetLength(t);
					}
					if (e.path == L"/") {
						has_rootfs = true;
						size_t l;
						if(!e.info.Pos(l, L'&')) e.info+= " &/";
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
			emplace(begin(), Entry{ strRootPath, strRootName});
		}

		emplace(begin(), Entry(GetMyHome(), strHomeName));
		emplace(begin(), Entry(another_curdir, strSameName));

		for (const auto &m : *this) {
			if (max_path < m.path.GetLength())
				max_path = m.path.GetLength();
			if (max_info < m.info.GetLength())
				max_info = m.info.GetLength();
		}
	}

	bool Unmount(FARString &path, bool force)
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
}
