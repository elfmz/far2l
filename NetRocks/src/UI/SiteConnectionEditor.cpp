#include "SiteConnectionEditor.h"
#include "DialogUtils.h"
#include "../Globals.h"
#include <KeyFileHelper.h>

/*                                                         62
345                      28         39                   60  64
 ============ Connection site settings ======================
| Display name:          [TEXTEDIT                         ] |
| Protocol:              [COMBOBOX                         ] |
| Hostname:              [TEXTEDIT                         ] |
| Port:                  [INTE]                              |
| Login:                 [TEXTEDIT                         ] |
| Password:              [PSWDEDIT                         ] |
|------------------------------------------------------------|
|      [             Protocol settings         ]             |
|  [     Save     ]  [    Connect   ]     [  Cancel      ]   |
 =============================================================
   6              21 24             39    45             60
*/

bool SiteConnectionEditor::Edit()
{
	std::string protocol, host, username, password, options;
	unsigned int port = 0;
	if (!_site.empty()) {
		KeyFileHelper kfh(G.config.c_str());
		protocol = kfh.GetString(_site.c_str(), "Protocol");
		host = kfh.GetString(_site.c_str(), "Host");
		port = (unsigned int)kfh.GetInt(_site.c_str(), "Port");
		options = kfh.GetString(_site.c_str(), "Options");
		username = kfh.GetString(_site.c_str(), "Username");
		password = kfh.GetString(_site.c_str(), "Password"); // TODO: de/obfuscation
		//directory = kfh.GetString(_site.c_str(), "Directory");
	}

	FarDialogItems di;
	FarListWrapper di_protocols;
	di_protocols.Add("SFTP");

	if (!di_protocols.Select(protocol.c_str())) {
		di_protocols.Select("SFTP");
	}

	size_t i_dblbox = di.Add(DI_DOUBLEBOX, 3,1,64,11, 0, MEditHost);

	di.Add(DI_TEXT, 5,2,27,2, 0, MDisplayName);
	size_t i_display_name = di.Add(DI_EDIT, 28,2,62,2, 0, _site.c_str());


	di.Add(DI_TEXT, 5,3,27,3, 0, MProtocol);
	size_t i_protocol = di.Add(DI_COMBOBOX, 28,3,62,3, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
	di[i_protocol].ListItems = di_protocols.Get();

	di.Add(DI_TEXT, 5,4,27,4, 0, MHost);
	size_t i_host = di.Add(DI_EDIT, 28,4,62,4, DIF_HISTORY, host.c_str(), "NetRocks_History_Host");

	di.Add(DI_TEXT, 5,5,27,5, 0, MPort);
	char sz[32]; itoa(port, sz, 10);
	size_t i_port = di.Add(DI_FIXEDIT, 28,5,33,5, DIF_MASKEDIT, sz, "99999");

	di.Add(DI_TEXT, 5,6,27,6, 0, MUserName);
	size_t i_username = di.Add(DI_EDIT, 28,6,62,6, DIF_HISTORY, username.c_str(), "NetRocks_History_User");

	di.Add(DI_TEXT, 5,7,27,7, 0, MPassword);
	size_t i_password = di.Add(DI_PSWEDIT, 28,7,62,7, 0, password.c_str());

	di.Add(DI_TEXT, 4,8,63,8, DIF_BOXCOLOR | DIF_SEPARATOR);

	size_t i_protocol_options = di.Add(DI_BUTTON, 10,9,50,9, DIF_CENTERGROUP, MProtocolOptions);

	size_t i_save = di.Add(DI_BUTTON, 6,10,21,10, DIF_CENTERGROUP, MSave);
	size_t i_connect = di.Add(DI_BUTTON, 24,10,39,10, DIF_CENTERGROUP, MConnect, nullptr, true);
	size_t i_cancel = di.Add(DI_BUTTON, 45,10,60,10, DIF_CENTERGROUP, MCancel);

	int result = G.info.DialogEx(G.info.ModuleNumber, -1, -1, di.EstimateWidth() + 6, di.EstimateHeight() + 2,
		di[i_dblbox].Data, &di[0], di.size(), 0, 0, &sDlgProc, (LONG_PTR)(uintptr_t)this);

	if (result == i_save || result == i_connect) {
		// TODO: save
	}

	return (result == i_connect);
}

LONG_PTR WINAPI SiteConnectionEditor::sDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	SiteConnectionEditor *it = (SiteConnectionEditor *)G.info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	if (it) {
		return it->DlgProc(hDlg, Msg, Param1, Param2);
	}

	return G.info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

LONG_PTR SiteConnectionEditor::DlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return G.info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

SiteConnectionEditor::SiteConnectionEditor(const std::string &site)
	: _site(site)
{
}
