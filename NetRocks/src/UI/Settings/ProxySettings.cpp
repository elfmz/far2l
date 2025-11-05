#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include <ScopeHelpers.h>
#include "../DialogUtils.h"
#include "../../Globals.h"

#define DEFAULT_CONFIG_TSOCKS       "/etc/tsocks.conf"
#define DEFAULT_CONFIG_PROXYCHAINS  "/etc/proxychains.conf"
#define DEFAULT_CONFIG_PROXYCHAINS4 "/etc/proxychains4.conf"

static const char *s_Proxifiers[] = {"tsocks", "proxychains"};

/*                                                         62
345                      28         39                   60  64
 ==================== Proxy settings ========================
| Proxifier:                       [COMBOBOX               ] |
|------------------------------------------------------------|
|                [Edit Proxy configuration]                  |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

static void LoadDefaultProxifierConfig(const std::string &kind, std::string &cfg)
{
	const char *cfg_src = nullptr;
	if (kind == "tsocks") {
		if (ReadWholeFile(DEFAULT_CONFIG_TSOCKS, cfg)) {
			cfg_src = DEFAULT_CONFIG_TSOCKS;
		}
	} else if (kind == "proxychains") {
		if (ReadWholeFile(DEFAULT_CONFIG_PROXYCHAINS4, cfg)) {
			cfg_src = DEFAULT_CONFIG_PROXYCHAINS4;
		} else if (ReadWholeFile(DEFAULT_CONFIG_PROXYCHAINS, cfg)) {
			cfg_src = DEFAULT_CONFIG_PROXYCHAINS;
		}

	} else {
		cfg = "# No known configuration for this proxifier kind\n";
		return;
	}

	if (!cfg_src) {
		cfg = "# Unable to load default configuration\n";
		return;
	}
	std::string header = "# Default configuration loaded from:\n#";
	header+= cfg_src;
	header+= "\n";
	cfg.insert(0, header);
}

static void RefineProxifierConfig(std::string &cfg)
{
	// Remove comments and empty lines to do not waist space in site connection
	// TODO: dont do this if really lots of users will complain
	std::vector<std::string> lines;
	StrExplode(lines, cfg, "\r\n");
	cfg.clear();
	for (const auto &line : lines) {
		if (!line.empty() && line.front() != '#') {
			cfg+= line;
			cfg+= '\n';
		}
	}
}


class ProxySettings : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_kind = -1, _i_edit = -1;
	FarListWrapper _di_kinds;
	StringConfig _sc;

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_BTNCLICK && param1 == _i_edit) {
			EditProxyConfig();
			return TRUE;
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}

	std::string GetSelectedProxifier()
	{
		int pos = GetDialogListPosition(_i_kind);
		if (!pos)
			return std::string();

		std::string out;
		TextFromDialogControl(_i_kind, out);
		return out;
	}

	std::string ProxifierCfgKey(const std::string &prxf)
	{
		return std::string("Proxifier_").append(prxf);
	}

	void EditProxyConfig()
	{
		const auto &kind = GetSelectedProxifier();
		if (kind.empty())
			return;

		UnlinkScope cfg_file(InMyTempFmt("NetRocks/proxy/edit_%u.cfg", getpid()));

		const std::string &key = ProxifierCfgKey(kind);
		std::string cfg = _sc.GetString(key.c_str());

		if (cfg.empty()) {
			LoadDefaultProxifierConfig(kind, cfg);
		}

		if (!WriteWholeFile(cfg_file.c_str(), cfg)) {
			fprintf(stderr, "%s: failed to make '%s'\n", __FUNCTION__, cfg_file.c_str());
			return;
		}

		G.info.Editor(StrMB2Wide(cfg_file).c_str(),
			NULL, -1, -1, -1, -1, EF_DISABLEHISTORY, 1, 1, CP_UTF8);

		cfg.clear();
		if (!ReadWholeFile(cfg_file.c_str(), cfg)) {
			fprintf(stderr, "%s: failed to read '%s'\n", __FUNCTION__, cfg_file.c_str());
			return;
		}

		RefineProxifierConfig(cfg);

		for (const auto &prxf : s_Proxifiers) {
			_sc.Delete(ProxifierCfgKey(prxf).c_str());
		}

		_sc.SetString(key.c_str(), cfg);
	}

public:
	ProxySettings(const std::string &options) :
		_sc(options)
	{
		_di_kinds.Add(MProxySettingsDisabled);
		for (const auto &prxf : s_Proxifiers) {
			_di_kinds.Add(prxf);
		}

		_di_kinds.SelectIndex(0);
		const std::string &kind = _sc.GetString("Proxifier");
		if (!kind.empty()) {
			_di_kinds.Select(kind.c_str());
		}

		_di.SetBoxTitleItem(MProxySettingsTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,37, 0, MProxySettingsKind);
		_i_kind = _di.AddAtLine(DI_COMBOBOX, 38,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_kind].ListItems = _di_kinds.Get();

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_edit = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MProxySettingsEdit);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}

	void Configure(std::string &options)
	{
		if (Show(L"ProxySettings", 6, 2) == _i_ok) {
			_sc.SetString("Proxifier", GetSelectedProxifier());
			options = _sc.Serialize();
		}
	}
};

void ConfigureProxySettings(std::string &options)
{
	ProxySettings(options).Configure(options);
}
