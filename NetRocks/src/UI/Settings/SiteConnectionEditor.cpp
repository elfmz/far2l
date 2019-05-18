#include <algorithm>
#include <utils.h>
#include "SiteConnectionEditor.h"
#include "../../Globals.h"
#include "../../SitesConfig.h"
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
|      [             Protocol settings         ]             |
|  [     Save     ]  [    Connect   ]     [  Cancel      ]   |
 =============================================================
   6              21 24             39    45             60
*/

static unsigned int DefaultPortForProtocol(const char *protocol)
{
	if (protocol) {
		for (auto pi = g_protocols; pi->name; ++pi) {
			if (strcasecmp(protocol, pi->name) == 0) {
				return pi->default_port;
			}
		}
	}

	return 0;
}


SiteConnectionEditor::SiteConnectionEditor(const std::string &display_name)
	: _initial_display_name(display_name), _display_name(display_name)
{
	if (!_display_name.empty()) {
		Load();
		_autogen_display_name = (DisplayNameAutogenerate() == _display_name);

	} else {
		_autogen_display_name = true;
	}

	for (auto pi = g_protocols; pi->name; ++pi) {
		_di_protocols.Add(pi->name);
	}

	if (_protocol.empty() || !_di_protocols.Select(_protocol.c_str())) {
		_protocol = g_protocols->name;
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

	_di.Add(DI_DOUBLEBOX, 3,1,64,13, 0, MEditHost);

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
	_di.AddAtLine(DI_TEXT, 5,27, 0, MPort);
	char sz[32]; itoa(_port, sz, 10);
	_i_port = _di.AddAtLine(DI_FIXEDIT, 28,33, DIF_MASKEDIT, sz, "99999");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MLoginMode);
	_i_login_mode = _di.AddAtLine(DI_COMBOBOX, 28,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
	_di[_i_login_mode].ListItems = _di_login_mode.Get();

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MUserName);
	_i_username = _di.AddAtLine(DI_EDIT, 28,62, DIF_HISTORY, _username.c_str(), "NetRocks_History_User");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MPassword);
	_i_password = _di.AddAtLine(DI_PSWEDIT, 28,62, 0, _password.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,27, 0, MDirectory);
	_i_directory = _di.AddAtLine(DI_EDIT, 28,62, DIF_HISTORY, _directory.c_str(), "NetRocks_History_Dir");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_protocol_options = _di.AddAtLine(DI_BUTTON, 10,50, DIF_CENTERGROUP, MProtocolOptions);

	_di.NextLine();
	_i_save = _di.AddAtLine(DI_BUTTON, 61,21, DIF_CENTERGROUP, MSave);
	_i_connect = _di.AddAtLine(DI_BUTTON, 24,39, DIF_CENTERGROUP, MConnect);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 45,60, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_protocol);
	SetDefaultDialogControl(_i_connect);
}

void SiteConnectionEditor::Load()
{
	SitesConfig sc;
	_initial_protocol = _protocol = sc.GetProtocol(_display_name);
	_host = sc.GetHost(_display_name);
	_initial_port = _port = sc.GetPort(_display_name);
	_login_mode = sc.GetLoginMode(_display_name);
	_username = sc.GetUsername(_display_name);
	_password = sc.GetPassword(_display_name);
	_directory = sc.GetDirectory(_display_name);
	_protocol_options = sc.GetProtocolOptions(_display_name, _protocol);
}

bool SiteConnectionEditor::Save()
{
	if (_display_name.empty()) {
		_display_name = DisplayNameAutogenerate();
		if (_display_name.empty())
			return false;
	}

	SitesConfig sc;
	sc.PutProtocol(_display_name, _protocol);
	sc.PutHost(_display_name, _host);
	sc.PutPort(_display_name, _port);
	sc.PutLoginMode(_display_name, _login_mode);
	sc.PutUsername(_display_name, _username);
	sc.PutPassword(_display_name, _password);
	sc.PutDirectory(_display_name, _directory);
	sc.PutProtocolOptions(_display_name, _protocol, _protocol_options);

	if (_display_name != _initial_display_name && !_initial_display_name.empty()) {
		sc.RemoveSite(_initial_display_name);
	}
	return true;
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

LONG_PTR SiteConnectionEditor::DlgProc(int msg, int param1, LONG_PTR param2)
{
	switch (msg) {
		case DN_BTNCLICK:
			if (param1 == _i_protocol_options) {
				ProtocolOptions();
				return TRUE;
			}

			if (param1 == _i_display_name_autogen) {
				DisplayNameAutogenerateAndApply();
				return TRUE;

			}

			if (param1 == _i_save || param1 == _i_connect) {
				DataFromDialog();
			}
		break;

		case DN_EDITCHANGE:
			if (_autogen_pending)
				break;

			if (param1 == _i_protocol) {
				OnProtocolChanged();
			}
			if (param1 == _i_login_mode) {
				OnLoginModeChanged();
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

void SiteConnectionEditor::OnProtocolChanged()
{
	++_autogen_pending;
	std::string protocol;
	TextFromDialogControl(_i_protocol, protocol);
	unsigned int port = DefaultPortForProtocol(protocol.c_str());
	char sz[32];
	snprintf(sz, sizeof(sz), "%u", port);
	TextToDialogControl(_i_port, sz);
	--_autogen_pending;
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
	auto existing = SitesConfig().EnumSites();
	existing.emplace_back(Wide2MB(G.GetMsgWide(MCreateSiteConnection)));

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
			str+= "-";
		}

		if (_port != DefaultPortForProtocol(_protocol.c_str())) {
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
	for (auto pi = g_protocols; pi->name; ++pi) {
		if (strcasecmp(_protocol.c_str(), pi->name) == 0) {
			if (pi->Configure) {
				pi->Configure(_protocol_options);
			}
			break;
		}
	}
}
