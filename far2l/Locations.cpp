#include "headers.hpp"


#include "Locations.hpp"
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
#include "interf.hpp"
#include "execute.hpp"
#include "dialog.hpp"
#include "DlgGuid.hpp"


namespace Locations
{
	void Enum(Entries &out, const FARString &curdir, const FARString &another_curdir)
	{
		out.clear();
		std::string cmd = GetMyScriptQuoted("locations.sh");
		cmd+= " enum \"";
		cmd+= EscapeCmdStr(Wide2MB(curdir));
		cmd+= "\" \"";
		cmd+= EscapeCmdStr(Wide2MB(another_curdir));
		cmd+= '\"';

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
			
				out.emplace_back();
				auto &e = out.back();
				e.text.Copy(&buf[0]);

				for (bool first = true;;) {
					size_t t;
					if (!e.text.Pos(t, L'\t')) break;
					if (first) {
						first = false;
						e.path = e.text.SubStr(0, t);
						e.text.Remove(0, t + 1);
					} else {
						e.text.Replace(t, 1, BoxSymbols[BS_V1]);
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
	}

	bool Unmount(FARString &path, bool force)
	{
		std::string cmd = GetMyScriptQuoted("locations.sh");
		cmd+= " umount \"";
		cmd+= EscapeCmdStr(Wide2MB(path));
		cmd+= "\"";
		if (force) {
			cmd+= " force";
		}
		int r = farExecuteA(cmd.c_str(), 0);
		return r == 0;
	}
}
