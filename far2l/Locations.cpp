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
	static std::map<FARString, FARString> gLastFavorites;

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
			gLastFavorites.clear();
			char buf[0x2100] = { };
			std::wstring s, tmp;
			while (fgets(buf, sizeof(buf)-1, f)!=NULL) {
				for (;;) {
					size_t l = strlen(buf);
					if (!l) break;
					if (buf[l-1]!='\r' && buf[l-1]!='\n') break;
					buf[l-1] = 0;
				}
			
				out.emplace_back();
				auto &e = out.back();

				if (buf[0] == 'F' && buf[1] == '^') {
					e.kind = FAVORITE;
					e.text.Copy(&buf[2]);

				} else if (buf[0] == 'M' && buf[1] == '^') {
					e.kind = MOUNTPOINT;
					e.text.Copy(&buf[2]);

				} else {
					e.text.Copy(&buf[0]);
				}

				for (bool first = true;;) {
					size_t t;
					if (!e.text.Pos(t, L'\t')) break;
					if (first) {
						first = false;
						e.path = e.text.SubStr(0, t);
						e.text.Remove(0, t + 1);
						if (e.kind == FAVORITE) {
							gLastFavorites[e.path] = e.text;
						}
					} else {
						e.text.Replace(t, 1, BoxSymbols[BS_V1]);
						if (e.kind == FAVORITE) {
							gLastFavorites[e.path] = e.text.SubStr(t + 1);
						}
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
		cmd+= " unmount \"";
		cmd+= EscapeCmdStr(Wide2MB(path));
		cmd+= "\"";
		int r = farExecuteA(cmd.c_str(), EF_HIDEOUT | EF_NOWAIT);
		return r == 0;
	}

	static bool SetFavorite(const wchar_t *path)
	{
		const wchar_t *HistoryNamePath = L"LocationsPath";
		const wchar_t *HistoryNameText = L"LocationsText";
		DialogDataEx DlgData[]=
		{
			{DI_DOUBLEBOX,3,1,72,8,{},0,path ? MSG(MLocationsEditFavoriteTitle) : MSG(MLocationsAddFavoriteTitle)},
			{DI_TEXT,     5,2, 0,2,{},0,MSG(MLocationsFavoritePath)},
			{DI_EDIT,     5,3,70,3,{(DWORD_PTR)HistoryNamePath},
				(path ? DIF_READONLY : DIF_FOCUS) | DIF_HISTORY | DIF_EDITEXPAND | DIF_EDITPATH, L""},
			{DI_TEXT,     5,4, 0,4,{},0,MSG(MLocationsFavoriteText)},
			{DI_EDIT,     5,5,70,5,{(DWORD_PTR)HistoryNameText}, (path ? DIF_FOCUS : 0) | DIF_HISTORY, L""},
			{DI_TEXT,     3,6, 0,6,{},DIF_SEPARATOR,L""},
			{DI_BUTTON,   0,7, 0,7,{},DIF_DEFAULT|DIF_CENTERGROUP,MSG(MOk)},
			{DI_BUTTON,   0,7, 0,7,{},DIF_CENTERGROUP,MSG(MCancel)}
		};
		MakeDialogItemsEx(DlgData, Dlg);
		if (path) {
			Dlg[2].strData = path;
			auto known_it = gLastFavorites.find(path);
			if (known_it != gLastFavorites.end()) {
				const wchar_t *trimmed_text = known_it->second.CPtr();
				while (*trimmed_text == ' ') {
					++trimmed_text;
				}
				Dlg[4].strData = trimmed_text;
			}
		}

		Dialog d(Dlg, ARRAYSIZE(Dlg), (FARWINDOWPROC)DefDlgProc, 0);
		d.SetPosition(-1,-1,76,10);
		d.SetHelp(L"LocationsFavorite");
		d.SetId(LocationsFavoriteId);
		d.Process();
		if (d.GetExitCode() != 6 || !Dlg[2].strData || !*Dlg[2].strData)
		{
			return false;
		}

		std::string cmd = GetMyScriptQuoted("locations.sh");
		cmd+= " favorite \"";
		cmd+= EscapeCmdStr(Wide2MB(Dlg[2].strData));
		cmd+= "\" \"";
		if (Dlg[4].strData && *Dlg[4].strData)
		{
			cmd+= EscapeCmdStr(Wide2MB(Dlg[4].strData));
		}
		else
		{
			cmd+= EscapeCmdStr(Wide2MB(PointToName(Dlg[2].strData)));
		}
		cmd+= "\"";
		int r = farExecuteA(cmd.c_str(), EF_HIDEOUT);
		return r == 0;
	}

	bool AddFavorite()
	{
		return SetFavorite(nullptr);
	}

	bool EditFavorite(FARString &path)
	{
		return SetFavorite(path.CPtr());
	}

	bool RemoveFavorite(FARString &path)
	{
		std::string cmd = GetMyScriptQuoted("locations.sh");
		cmd+= " favorite-remove \"";
		cmd+= EscapeCmdStr(Wide2MB(path));
		cmd+= "\"";
		int r = farExecuteA(cmd.c_str(), EF_HIDEOUT);
		return r == 0;
	}

	
	void Show()
	{
		//TODO
	}
}
