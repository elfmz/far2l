#include <algorithm>
#include <vector>
#include <mutex>
#include <utils.h>
#include <time.h>
#include <windows.h>
#include <StringConfig.h>
#include "SiteConnectionEditor.h"
#include "../../Globals.h"
#include "../../Protocol/Protocol.h"

/*                                                         62
345                      28         39                   60  64
 ============ Connection site settings ======================
| Display name:          [TEXTEDIT                       ][A]|
| Protocol:              [COMBOBOX                         ] |
| Hostname:              [TEXTEDIT                         ] |
| Port:                  [INTE]                              |
| Login:                 [TEXTEDIT                         ] |
| Password mode:         [COMBOBOX                         ] |
| Password:              [PSWDEDIT                         ] |
| Directory:             [TEXTEDIT                         ] |
|------------------------------------------------------------|
|   [Extra settings]  [Protocol settings] [Proxy settings]   |
|------------------------------------------------------------|
|  [     Save     ]  [    Connect   ]     [  Cancel      ]   |
 =============================================================
   6              21 24             39    45             60
*/

static int DefaultPortForProtocol(const char *protocol)
{
	if (protocol) {
		const auto pi = ProtocolInfoLookup(protocol);
		if (pi) {
			return pi->default_port;
		}
	}

	return -1;
}

void ConfigureExtraSiteSettings(std::string &options);
void ConfigureProxySettings(std::string &options);

SiteConnectionEditor::SiteConnectionEditor(const SitesConfigLocation &sites_cfg_location, const std::string &display_name)
	: _sites_cfg_location(sites_cfg_location), _initial_display_name(display_name), _display_name(display_name)
{
	if (!_display_name.empty()) {
		Load();
		_autogen_display_name = (DisplayNameAutogenerate() == _display_name);

	} else {
		_autogen_display_name = true;
	}

	for (auto pi = ProtocolInfoHead(); pi->name; ++pi) {
		_di_protocols.Add(pi->name);
	}

	if (_protocol.empty() || !_di_protocols.Select(_protocol.c_str())) {
		_protocol = ProtocolInfoHead()->name;
		_di_protocols.Select(_protocol.c_str());
	}

	_di_login_mode.Add(MPasswordModeNoPassword);
	_di_login_mode.Add(MPasswordModeAskPassword);
	_di_login_mode.Add(MPasswordModeSavedPassword);
	if (!_di_login_mode.SelectIndex(_login_mode)) {
		_login_mode = 0;
		_di_login_mode.SelectIndex(_login_mode);
	}

	if (_port == 0) {
		_port = DefaultPortForProtocol(_di_protocols.GetSelection());
	}

	char sz[32];

	_di.SetBoxTitleItem(MEditHost);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,27, 0, MDisplayName);
	_i_display_name = _di.AddAtLine(DI_EDIT, 28,58, 0, _display_name.c_str());
	_i_display_name_autogen = _di.AddAtLine(DI_BUTTON, 59,61, 0, "&A");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MProtocol);
	_i_protocol = _di.AddAtLine(DI_COMBOBOX, 28,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
	_di[_i_protocol].ListItems = _di_protocols.Get();

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MHost);
	_i_host = _di.AddAtLine(DI_EDIT, 28,62, DIF_HISTORY, _host.c_str(), "NetRocks_History_Host");

	_di.NextLine();
	_i_port_text = _di.AddAtLine(DI_TEXT, 5,27, 0, MPort);
	itoa(_port, sz, 10);
	_i_port = _di.AddAtLine(DI_FIXEDIT, 28,33, DIF_MASKEDIT, sz, "99999");

	_di.NextLine();
	_i_login_mode_text = _di.AddAtLine(DI_TEXT, 5,27, 0, MLoginMode);
	_i_login_mode = _di.AddAtLine(DI_COMBOBOX, 28,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
	_di[_i_login_mode].ListItems = _di_login_mode.Get();

	_di.NextLine();
	_i_username_text = _di.AddAtLine(DI_TEXT, 5,27, 0, MUserName);
	_i_username = _di.AddAtLine(DI_EDIT, 28,62, DIF_HISTORY, _username.c_str(), "NetRocks_History_User");

	_di.NextLine();
	_i_password_text = _di.AddAtLine(DI_TEXT, 5,27, 0, MPassword);
	_i_password = _di.AddAtLine(DI_PSWEDIT, 28,62, 0, _password.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MDirectory);
	_i_directory = _di.AddAtLine(DI_EDIT, 28,62, DIF_HISTORY, _directory.c_str(), "NetRocks_History_Dir");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_extra_options = _di.AddAtLine(DI_BUTTON, 10,50, DIF_CENTERGROUP, MExtraOptions);
	_i_protocol_options = _di.AddAtLine(DI_BUTTON, 10,50, DIF_CENTERGROUP, MProtocolOptions);
	_i_proxy_options = _di.AddAtLine(DI_BUTTON, 10,50, DIF_CENTERGROUP, MProxyOptions);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_save = _di.AddAtLine(DI_BUTTON, 61,21, DIF_CENTERGROUP, MSave);
	_i_connect = _di.AddAtLine(DI_BUTTON, 24,39, DIF_CENTERGROUP, MSaveConnect);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 45,60, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_protocol);
	SetDefaultDialogControl(_i_connect);
}

class ProtocolOptionsScope
{
	SiteConnectionEditor &_sce;
	std::string _original_options, _options;

public:
	ProtocolOptionsScope(SiteConnectionEditor &sce)
		: _sce(sce)
	{
		auto it = _sce._protocols_options.find(_sce._protocol);
		if (it == _sce._protocols_options.end()) {
			SitesConfig sc(_sce._sites_cfg_location);
			_options = sc.GetProtocolOptions(_sce._display_name, _sce._protocol);
		} else {
			_options = it->second;
		}
		_original_options = _options;
	}

	~ProtocolOptionsScope()
	{
		if (_original_options != _options) {
			_sce._protocols_options[_sce._protocol] = _options;
		}
	}

	operator std::string &()
	{
		return _options;
	}
};

void SiteConnectionEditor::Load()
{
	SitesConfig sc(_sites_cfg_location);
	_protocol = sc.GetProtocol(_display_name);
	_host = sc.GetHost(_display_name);
	_initial_port = _port = sc.GetPort(_display_name);
	_login_mode = sc.GetLoginMode(_display_name);
	_username = sc.GetUsername(_display_name);
	_password = sc.GetPassword(_display_name);
	_directory = sc.GetDirectory(_display_name);
}

bool SiteConnectionEditor::Save()
{
	if (_display_name.empty()) {
		_display_name = DisplayNameAutogenerate();
		if (_display_name.empty())
			return false;
	}

	{
		ProtocolOptionsScope pos(*this);
		EnsureTimeStamp(pos);
	}

	SitesConfig sc(_sites_cfg_location);
	sc.SetProtocol(_display_name, _protocol);
	sc.SetHost(_display_name, _host);
	sc.SetPort(_display_name, _port);
	sc.SetLoginMode(_display_name, _login_mode);
	sc.SetUsername(_display_name, _username);
	sc.SetPassword(_display_name, _password);
	sc.SetDirectory(_display_name, _directory);

	for (const auto &it : _protocols_options) {
		sc.SetProtocolOptions(_display_name, it.first, it.second);
	}

	if (_display_name != _initial_display_name && !_initial_display_name.empty()) {
		sc.RemoveSite(_initial_display_name);
	}
	return true;
}

void SiteConnectionEditor::EnsureTimeStamp(std::string &protocol_options)
{
	StringConfig sc(protocol_options);
	unsigned long long ts = sc.GetHexULL("TS");
	if (!ts) {
		ts = time(NULL);
		sc.SetHexULL("TS", ts);
		protocol_options = sc.Serialize();
	}
}

bool SiteConnectionEditor::Edit()
{
	int result = Show(L"SiteConnectionEditor", 6, 2);

	if (result == _i_save || result == _i_connect) {
		if (!Save()) {
			return false;
		}
	}

	return (result == _i_connect);
}

void SiteConnectionEditor::UpdateEnabledButtons()
{
	++_autogen_pending;
	std::string protocol, host;
	TextFromDialogControl(_i_protocol, protocol);
	TextFromDialogControl(_i_host, host);
	auto pi = ProtocolInfoLookup(protocol.c_str());
	if (pi) {
		SetEnabledDialogControl(_i_save, !pi->require_server || !host.empty() );
		SetEnabledDialogControl(_i_connect, !pi->require_server || !host.empty() );
	}
	--_autogen_pending;
}

void SiteConnectionEditor::UpdatePerProtocolState(bool reset_port)
{
	++_autogen_pending;
	std::string protocol;
	TextFromDialogControl(_i_protocol, protocol);
	auto pi = ProtocolInfoLookup(protocol.c_str());
	if (pi) {
		_protocol = pi->name;
		if (reset_port && pi->default_port != -1) {
			LongLongToDialogControl(_i_port, pi->default_port);
		}
		SetVisibleDialogControl(_i_port, pi->default_port != -1);
		SetVisibleDialogControl(_i_port_text, pi->default_port != -1);
		SetVisibleDialogControl(_i_login_mode, pi->support_creds);
		SetVisibleDialogControl(_i_login_mode_text, pi->support_creds);
		SetVisibleDialogControl(_i_username, pi->support_creds);
		SetVisibleDialogControl(_i_username_text, pi->support_creds);
		SetVisibleDialogControl(_i_password, pi->support_creds);
		SetVisibleDialogControl(_i_password_text, pi->support_creds);
	}
	--_autogen_pending;
}

LONG_PTR SiteConnectionEditor::DlgProc(int msg, int param1, LONG_PTR param2)
{
	switch (msg) {
		case DN_INITDIALOG: {
				UpdatePerProtocolState(false);
				UpdateEnabledButtons();
			} break;

		case DN_CLOSE: {
			if (param1 == _i_save || param1 == _i_connect) {
				DataFromDialog();
			}
		} break;

		case DN_BTNCLICK:
			if (param1 == _i_extra_options) {
				ProtocolOptionsScope pos(*this);
				ConfigureExtraSiteSettings(pos);
				return TRUE;
			}
			if (param1 == _i_proxy_options) {
				ProtocolOptionsScope pos(*this);
				ConfigureProxySettings(pos);
				return TRUE;
			}

			if (param1 == _i_protocol_options) {
				ProtocolOptions();
				return TRUE;
			}

			if (param1 == _i_display_name_autogen) {
				DisplayNameAutogenerateAndApply();
				return TRUE;
			}

		break;

		case DN_EDITCHANGE:
			if (_autogen_pending)
				break;

			if (param1 == _i_protocol) {
				UpdatePerProtocolState(true);
				UpdateEnabledButtons();
			}
			else if (param1 == _i_login_mode) {
				OnLoginModeChanged();
			}
			else if (param1 == _i_host) {
				UpdateEnabledButtons();
			}


			if (param1 == _i_display_name) {
				_autogen_display_name = false;
				DisplayNameInputRefine();

			} else if (_autogen_display_name && (
				param1 == _i_protocol || param1 == _i_host || param1 == _i_port
				|| param1 == _i_login_mode || param1 == _i_username || param1 == _i_directory) ) {
				DisplayNameAutogenerateAndApply();
			}

		break;
	}
	return BaseDialog::DlgProc(msg, param1, param2);
}

void SiteConnectionEditor::DataFromDialog()
{
	std::string str;
	TextFromDialogControl(_i_protocol, _protocol);
	TextFromDialogControl(_i_display_name, _display_name);
	TextFromDialogControl(_i_host, _host);
	TextFromDialogControl(_i_port, str); _port = atoi(str.c_str());
	TextFromDialogControl(_i_username, _username);
	TextFromDialogControl(_i_password, _password);
	TextFromDialogControl(_i_directory, _directory);
}

void SiteConnectionEditor::OnLoginModeChanged()
{
	++_autogen_pending;
	_login_mode = (unsigned long)GetDialogListPosition(_i_login_mode);
	if (_login_mode > 2) {
		_login_mode = 0;
	}
//	fprintf(stderr, "_login_mode=%u\n", _login_mode);

	if (_login_mode == 0) {
		TextToDialogControl(_i_username, "anonymous");
	}
	SetEnabledDialogControl(_i_password, (_login_mode == 2));
	--_autogen_pending;
}

void SiteConnectionEditor::DisplayNameInputRefine()
{
	++_autogen_pending;
	try {
		DataFromDialog();
		bool changed = false;
		for (auto &c : _display_name) {
			if (c == '/') {
				c = '\\';
				changed = true;
			}
		}
		if (changed)
			TextToDialogControl(_i_display_name, _display_name);
	} catch (std::exception &) {
		;
	}
	--_autogen_pending;
}


void SiteConnectionEditor::DisplayNameAutogenerateAndApply()
{
	++_autogen_pending;
	try {
		DataFromDialog();
		_display_name = DisplayNameAutogenerate();
		TextToDialogControl(_i_display_name, _display_name);
		_autogen_display_name = true;
	} catch (std::exception &) {
		;
	}
	--_autogen_pending;
}

std::string SiteConnectionEditor::DisplayNameAutogenerate()
{
	auto existing = SitesConfig(_sites_cfg_location).EnumSites();
	existing.emplace_back(Wide2MB(G.GetMsgWide(MCreateSiteConnection)));

	const int def_port = DefaultPortForProtocol(_protocol.c_str());

	std::string str;
	char sz[32];
	for (unsigned int attempt = 0; attempt < 0x10000000; ++attempt) {
		str = _protocol;
		str+= ':';

		if (!_username.empty()) {
			str+= _username;
			str+= '@';
		}

		if (!_host.empty()) {
			str+= _host;
		} else {
			str+= "*";
		}

		if (def_port != -1 && _port != (unsigned int)def_port) {
			snprintf(sz, sizeof(sz) - 1, ":%u", _port);
			str+= sz;
		}

//		for (auto ch : _directory) {
//			str+= (ch == '/') ? '\\' : ch;
//		}

		if (attempt) {
			snprintf(sz, sizeof(sz) - 1, " (%u)", attempt);
			str+= sz;
		}

		for (auto &ch : str) {
			if (ch == '/') ch = '\\';
		}

		if (str == _display_name || str == _initial_display_name) {
			break;
		}

		if (std::find(existing.begin(), existing.end(), str) == existing.end()) {
			break;
		}
	}

	return str;
}

void SiteConnectionEditor::ProtocolOptions()
{
	auto pi = ProtocolInfoLookup(_protocol.c_str());
	if (pi && pi->Configure) {
		ProtocolOptionsScope pos(*this);
		pi->Configure(pos);
	}
}
