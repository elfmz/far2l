#include "SiteConnectionEditor.h"
#include "../Globals.h"
#include <KeyFileHelper.h>
#include <algorithm>

/*                                                         62
345                      28         39                   60  64
 ============ Connection site settings ======================
| Display name:          [TEXTEDIT                       ][A]|
| Protocol:              [COMBOBOX                         ] |
| Hostname:              [TEXTEDIT                         ] |
| Port:                  [INTE]                              |
| Login:                 [TEXTEDIT                         ] |
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
	if (strcasecmp(protocol, "sftp") == 0)
		return 22;

	if (strcasecmp(protocol, "ftp") == 0)
		return 21;

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

	_di_protocols.Add("sftp");

	if (_protocol.empty() || !_di_protocols.Select(_protocol.c_str())) {
		_protocol = "sftp";
		_di_protocols.Select(_protocol.c_str());
	}

	if (_port == 0) {
		_port = DefaultPortForProtocol("sftp");
	}

	_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, MEditHost);

	_di.Add(DI_TEXT, 5,2,27,2, 0, MDisplayName);
	_i_display_name = _di.Add(DI_EDIT, 28,2,58,2, 0, _display_name.c_str());
	_i_display_name_autogen = _di.Add(DI_BUTTON, 59,2,61,2, 0, "&A");


	_di.Add(DI_TEXT, 5,3,27,3, 0, MProtocol);
	_i_protocol = _di.Add(DI_COMBOBOX, 28,3,62,3, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "", nullptr, FDIS_FOCUSED);
	_di[_i_protocol].ListItems = _di_protocols.Get();

	_di.Add(DI_TEXT, 5,4,27,4, 0, MHost);
	_i_host = _di.Add(DI_EDIT, 28,4,62,4, DIF_HISTORY, _host.c_str(), "NetRocks_History_Host");

	_di.Add(DI_TEXT, 5,5,27,5, 0, MPort);
	char sz[32]; itoa(_port, sz, 10);
	_i_port = _di.Add(DI_FIXEDIT, 28,5,33,5, DIF_MASKEDIT, sz, "99999");

	_di.Add(DI_TEXT, 5,6,27,6, 0, MUserName);
	_i_username = _di.Add(DI_EDIT, 28,6,62,6, DIF_HISTORY, _username.c_str(), "NetRocks_History_User");

	_di.Add(DI_TEXT, 5,7,27,7, 0, MPassword);
	_i_password = _di.Add(DI_PSWEDIT, 28,7,62,7, 0, _password.c_str());

	_di.Add(DI_TEXT, 5,8,27,8, 0, MDirectory);
	_i_directory = _di.Add(DI_EDIT, 28,8,62,8, DIF_HISTORY, _directory.c_str(), "NetRocks_History_Dir");

	_di.Add(DI_TEXT, 4,9,63,9, DIF_BOXCOLOR | DIF_SEPARATOR);

	_i_protocol_options = _di.Add(DI_BUTTON, 10,10,50,10, DIF_CENTERGROUP, MProtocolOptions);

	_i_save = _di.Add(DI_BUTTON, 6,11,21,11, DIF_CENTERGROUP, MSave);
	_i_connect = _di.Add(DI_BUTTON, 24,11,39,11, DIF_CENTERGROUP, MConnect, nullptr, FDIS_DEFAULT);
	_i_cancel = _di.Add(DI_BUTTON, 45,11,60,11, DIF_CENTERGROUP, MCancel);
}

void SiteConnectionEditor::Load()
{
	KeyFileHelper kfh(G.config.c_str());
	_initial_protocol = _protocol = kfh.GetString(_display_name.c_str(), "Protocol");
	_host = kfh.GetString(_display_name.c_str(), "Host");
	_initial_port = _port = (unsigned int)kfh.GetInt(_display_name.c_str(), "Port");
	_username = kfh.GetString(_display_name.c_str(), "Username");
	_password = kfh.GetString(_display_name.c_str(), "Password"); // TODO: de/obfuscation
	_directory = kfh.GetString(_display_name.c_str(), "Directory");
	_options = kfh.GetString(_display_name.c_str(), "Options");
}

bool SiteConnectionEditor::Save()
{
	if (_display_name.empty()) {
		_display_name = DisplayNameAutogenerate();
		if (_display_name.empty())
			return false;
	}

	KeyFileHelper kfh(G.config.c_str());
	kfh.PutString(_display_name.c_str(), "Protocol", _protocol.c_str());
	kfh.PutString(_display_name.c_str(), "Host", _host.c_str());
	kfh.PutInt(_display_name.c_str(), "Port", _port);
	kfh.PutString(_display_name.c_str(), "Username", _username.c_str());
	kfh.PutString(_display_name.c_str(), "Password", _password.c_str());
	kfh.PutString(_display_name.c_str(), "Directory", _directory.c_str());
	kfh.PutString(_display_name.c_str(), "Options", _options.c_str());
	return true;
}

bool SiteConnectionEditor::Edit()
{
	int result = Show(6, 2, _di[_i_dblbox].Data);

	if (result == _i_save || result == _i_connect) {
		if (!Save()) {
			return false;
		}
	}

	return (result == _i_connect);
}

LONG_PTR SiteConnectionEditor::DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	switch (msg) {
		case DN_BTNCLICK:
			if (param1 == _i_display_name_autogen) {
				DisplayNameAutogenerateAndApply(dlg);
				return TRUE;

			} else if (param1 == _i_save || param1 == _i_connect) {
				DataFromDialog(dlg);
			}
		break;

		case DN_EDITCHANGE:
			if (_autogen_pending) {
				;

			} else if (param1 == _i_display_name) {
				_autogen_display_name = false;

			} else if (_autogen_display_name && (
				param1 == _i_protocol || param1 == _i_host || param1 == _i_port
				|| param1 == _i_username || param1 == _i_directory) ) {
				if (param1 == _i_protocol) {
					AssignDefaultPortNumber(dlg);
				}
				DisplayNameAutogenerateAndApply(dlg);
			}

		break;
	}
	return BaseDialog::DlgProc(dlg, msg, param1, param2);
}

void SiteConnectionEditor::TextToDialogControl(HANDLE dlg, int ctl, const std::string &str)
{
	FarDialogItemData dd = { (int)str.size(), (char*)str.c_str() };
	G.info.SendDlgMessage(dlg, DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void SiteConnectionEditor::TextFromDialogControl(HANDLE dlg, int ctl, std::string &str)
{
	static char buf[ 0x1000 ] = {};
	FarDialogItemData dd = { sizeof(buf) - 1, buf };
	LONG_PTR rv = G.info.SendDlgMessage(dlg, DM_GETTEXT, ctl, (LONG_PTR)&dd);
	if (rv > 0 && rv < (LONG_PTR)sizeof(buf))
		buf[rv] = 0;
	str = buf;
}

void SiteConnectionEditor::DataFromDialog(HANDLE dlg)
{
	std::string str;
	TextFromDialogControl(dlg, _i_protocol, _protocol);
	TextFromDialogControl(dlg, _i_display_name, _display_name);
	TextFromDialogControl(dlg, _i_host, _host);
	TextFromDialogControl(dlg, _i_port, str); _port = atoi(str.c_str());
	TextFromDialogControl(dlg, _i_username, _username);
	TextFromDialogControl(dlg, _i_password, _password);
	TextFromDialogControl(dlg, _i_directory, _directory);
}

void SiteConnectionEditor::AssignDefaultPortNumber(HANDLE dlg)
{
	++_autogen_pending;
	std::string protocol;
	TextFromDialogControl(dlg, _i_protocol, protocol);
	unsigned int port = DefaultPortForProtocol(protocol.c_str());
	char sz[32];
	snprintf(sz, sizeof(sz), "%u", port);
	TextToDialogControl(dlg, _i_port, sz);
	--_autogen_pending;
}

void SiteConnectionEditor::DisplayNameAutogenerateAndApply(HANDLE dlg)
{
	++_autogen_pending;
	try {
		DataFromDialog(dlg);
		_display_name = DisplayNameAutogenerate();
		TextToDialogControl(dlg, _i_display_name, _display_name);
		_autogen_display_name = true;
	} catch (std::exception &) {
		;
	}
	--_autogen_pending;
}

std::string SiteConnectionEditor::DisplayNameAutogenerate()
{
	const auto &sections = KeyFileHelper(G.config.c_str()).EnumSections();

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

		for (auto ch : _directory) {
			str+= (ch == '/') ? '\\' : ch;
		}

		if (attempt) {
			snprintf(sz, sizeof(sz) - 1, " (%u)", attempt);
			str+= sz;
		}

		if (str == _display_name || str == _initial_display_name) {
			break;
		}

		if (std::find(sections.begin(), sections.end(), str) == sections.end()) {
			break;
		}
	}

	return str;
}
