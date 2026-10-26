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
	bool _include_flavor;

	int _i_ok = -1, _i_cancel = -1, _i_way = -1, _i_flavor = -1;

	FarListWrapper _di_ways;
	FarListWrapper _di_flavor;
	struct Option
	{
		FarListWrapper di_items;
		int i_cb;
	};
	std::list<Option> _opts;

	// The flavors on offer for the current way, in list order. A way that
	// declares which shell it arrives at gets a shortened list - offering a
	// helper the way cannot run only invites a connect that cannot work - so
	// list positions are not fixed and the values are kept alongside rather
	// than recomputed from an index. ProtocolFISHPLUS reads these strings
	// back verbatim in Initialize().
	std::vector<const char *> _flavor_values;

	void BuildFlavorList(const std::string &way_flavor, const std::string &current)
	{
		_flavor_values.clear();
		_di_flavor.Add(MFISHPLUSHelperFlavorAuto);
		_flavor_values.emplace_back("auto");
		if (way_flavor != "pwsh") {
			_di_flavor.Add(MFISHPLUSHelperFlavorPosix);
			_flavor_values.emplace_back("posix");
		}
		if (way_flavor != "posix") {
			_di_flavor.Add(MFISHPLUSHelperFlavorPwsh);
			_flavor_values.emplace_back("pwsh");
		}
		// A value carried over from another way may not be on offer here; the
		// site then falls back to Auto rather than keeping a setting the way
		// cannot honor. This is what used to let Flavor=PowerShell survive a
		// switch to a POSIX-only way and produce a connect that never worked.
		size_t sel = 0;
		for (size_t i = 0; i < _flavor_values.size(); ++i) {
			if (current == _flavor_values[i]) {
				sel = i;
				break;
			}
		}
		_di_flavor.SelectIndex(sel);
	}

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
	ProtocolOptionsSHELL(std::string &way, StringConfig &sc,
			const char *ways_ini_subpath, int box_title, bool include_flavor)
		: _way(way), _sc(sc), _include_flavor(include_flavor)
	{
		_ways_ini = StrWide2MB(G.plugin_path);
		CutToSlash(_ways_ini, true);
		_ways_ini+= ways_ini_subpath;
		TranslateInstallPath_Lib2Share(_ways_ini);

		InitializeWays();

		_di.SetBoxTitleItem(box_title);

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

		// The flavor is not a way-specific option like the OPTs are - a
		// Windows peer may be reached by any way that arrives at a PowerShell
		// prompt - so it gets its own row. What the row offers depends on what
		// the way declares. A way that lands in PowerShell leaves nothing to
		// choose, since Auto means the same thing there and POSIX cannot work:
		// it gets no row at all and Configure() stores Auto for it. A way that
		// lands in a POSIX shell keeps Auto - that is what lets the probe jump
		// away to a PowerShell way - and only drops PowerShell from the list.
		if (_include_flavor && cfg.flavor != "pwsh") {
			_di.NextLine();
			_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

			_di.NextLine();
			_di.AddAtLine(DI_TEXT, 5, 34, 0, MFISHPLUSHelperFlavor);
			BuildFlavorList(cfg.flavor, _sc.GetString("Flavor"));
			_i_flavor = _di.AddAtLine(DI_COMBOBOX, 35, 53,
				DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
			_di[_i_flavor].ListItems = _di_flavor.Get();
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
			if (_include_flavor) {
				const int pos = (_i_flavor >= 0) ? GetDialogListPosition(_i_flavor) : -1;
				_sc.SetString("Flavor",
					(pos >= 0 && size_t(pos) < _flavor_values.size())
						? _flavor_values[pos] : "auto");
			}
		}
		return r == _i_way;
	}
};

static void ConfigureWayBasedProtocol(std::string &options,
	const char *ways_ini_subpath, int box_title, bool include_flavor)
{
	try {
		StringConfig sc(options);
		std::string way = sc.GetString("Way");
		while (ProtocolOptionsSHELL(way, sc, ways_ini_subpath, box_title, include_flavor).Configure()) {
			;
		}
		options = sc.Serialize();
	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}
}

void ConfigureProtocolSHELL(std::string &options)
{
	ConfigureWayBasedProtocol(options, "SHELL/ways.ini", MSHELLOptionsTitle, false);
}

// FISH+ reaches its remote shell exactly the way SHELL does, so it gets the
// same dialog driven by its own ways.ini rather than a copy of this file. It
// adds a Flavor combobox on top: which helper (helper.sh or helper.ps1) to
// upload once the transport is up. See ProtocolFISHPLUS::Initialize.
void ConfigureProtocolFISHPLUS(std::string &options)
{
	ConfigureWayBasedProtocol(options, "FISHPLUS/ways.ini", MFISHPLUSOptionsTitle, true);
}

