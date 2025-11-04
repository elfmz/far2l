#include <algorithm>
#include <memory>
#include <list>
#include <utils.h>
#include <StringConfig.h>
#include <KeyFileHelper.h>
#include "../DialogUtils.h"
#include "../../Globals.h"
#include "../../Protocol/SHELL/WayToShellConfig.h"
#include <stdexcept>

/*                                                  55
345                      28     35                53
 ===== SHELL Protocol options ======================
| Client application:           [COMBOBOX         ] |
|---------------------------------------------------|
| Option1        :              [                 ] |
| Option2        :              [                 ] |
| Option3        :              [                 ] |
| Option4        :              [                 ] |
| Option5        :              [                 ] |
| Option6        :              [                 ] |
| Option7        :              [                 ] |
| Option8        :              [                 ] |
|---------------------------------------------------|
| [  OK    ]    [ Cancel ]                          |
 ===================================================
    6                     29       38
*/

class ProtocolOptionsSHELL : protected BaseDialog
{
	std::string &_way;
	StringConfig &_sc;

	std::string _ways_ini;

	int _i_ok = -1, _i_cancel = -1, _i_way = -1;

	FarListWrapper _di_ways;
	struct Option
	{
		FarListWrapper di_items;
		int i_cb;
	};
	std::list<Option> _opts;

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_EDITCHANGE && param1 == _i_way) {
			Close(param1);
		}

		return BaseDialog::DlgProc(msg, param1, param2);
	}

	void InitializeWays()
	{
		WaysToShell ways(_ways_ini);
		if (ways.empty()) {
			throw std::runtime_error("no way");
		}
		for (const auto &w : ways) {
			_di_ways.Add(w.c_str());
		}
		if (_way.empty() || !_di_ways.Select(_way.c_str())) {
			_di_ways.SelectIndex(0);
			_way = ways.front();
		}
	}

	void InitializeOption(const WayToShellConfig::Option &opt, const std::string &value)
	{
		if (opt.items.empty()) {
			throw std::runtime_error("option missing items");
		}
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5, 34, 0, opt.name.c_str());

		_opts.emplace_back();
		auto &added_opt = _opts.back();

		ssize_t select_index = 0;
		for (unsigned i = 0; i < opt.items.size(); ++i) {
			added_opt.di_items.Add(opt.items[i].info.c_str());
			if (opt.items[i].value == value) {
				select_index = i;
			}
		}
		added_opt.di_items.SelectIndex(select_index);

		added_opt.i_cb = _di.AddAtLine(DI_COMBOBOX, 35, 53, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[added_opt.i_cb].ListItems = added_opt.di_items.Get();
	}

public:
	ProtocolOptionsSHELL(std::string &way, StringConfig &sc)
		: _way(way), _sc(sc)
	{
		_ways_ini = StrWide2MB(G.plugin_path);
		CutToSlash(_ways_ini, true);
		_ways_ini+= "SHELL/ways.ini";
		TranslateInstallPath_Lib2Share(_ways_ini);

		InitializeWays();

		_di.SetBoxTitleItem(MSHELLOptionsTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5, 34, 0, MSHELLWay);
		_i_way = _di.AddAtLine(DI_COMBOBOX, 35, 53, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_way].ListItems = _di_ways.Get();

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR, MSHELLWaySettings);

		WayToShellConfig cfg(_ways_ini, _way);
		for (unsigned i = 0; i < cfg.options.size(); ++i) {
			const auto &opt = cfg.options[i];
			const auto &value = _sc.GetString(StrPrintf("OPT%u", i).c_str(), opt.items[opt.def].value.c_str());
			InitializeOption(opt, value);
		}

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_ok = _di.AddAtLine(DI_BUTTON, 7,11, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 12,23, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}


	bool Configure()
	{
		const int r = Show(L"ProtocolOptionsSHELL", 6, 2);
		TextFromDialogControl(_i_way, _way);
		if (r == _i_ok) {
			_sc.SetString("Way", _way);
			WayToShellConfig cfg(_ways_ini, _way);
			unsigned i = 0;
			for (auto &opt : _opts) {
				const int pos = GetDialogListPosition(opt.i_cb);
				if (pos >= 0 && size_t(pos) < cfg.options[i].items.size()) {
					_sc.SetString(StrPrintf("OPT%u", i).c_str(), cfg.options[i].items[pos].value);
				}
				++i;
			}
		}
		return r == _i_way;
	}
};

void ConfigureProtocolSHELL(std::string &options)
{
	try {
		StringConfig sc(options);
		std::string way = sc.GetString("Way");
		while (ProtocolOptionsSHELL(way, sc).Configure()) {
			;
		}
		options = sc.Serialize();
	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}
}

