#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================= FTP Protocol options =====================
| [ ] Enable explicit encryption                             |
| Minimal encryption protocol:           [COMBOBOX         ] |
| List command:                          [COMBOBOX         ] |
| [ ] Use MLSD/MLST if possible                              |
| [ ] Use passive mode for transfers                         |
| [ ] Enable commands pipelining                             |
| [ ] Ensure data connection peer matches server             |
| [ ] Enable TCP_NODELAY option                              |
| [ ] Enable TCP_QUICKACK option                             |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

class ProtocolOptionsFTP : protected BaseDialog
{
	bool _implicit_encryption;
	bool _encryption_protocol_enabled = true;
	bool _restrict_data_peer_enabled = true;

	int _i_ok = -1, _i_cancel = -1;

	int _i_explicit_encryption = -1;
	int _i_encryption_protocol = -1;
	int _i_list_command = -1;
	int _i_restrict_data_peer = -1;
	int _i_passive_mode = -1;
	int _i_use_mlsd_mlst = -1;
	int _i_commands_pipelining = -1;
	int _i_tcp_nodelay = -1, _i_tcp_quickack = -1;

	FarListWrapper _di_encryption_protocol, _di_list_command;

	void UpdateEnableds()
	{
		const bool encryption_protocol_enabled =
			(_implicit_encryption || IsCheckedDialogControl(_i_explicit_encryption));

		const bool restrict_data_peer_enabled =
			(encryption_protocol_enabled || !IsCheckedDialogControl(_i_passive_mode));

		if (encryption_protocol_enabled != _encryption_protocol_enabled) {
			_encryption_protocol_enabled = encryption_protocol_enabled;
			SetEnabledDialogControl(_i_encryption_protocol, encryption_protocol_enabled);
		}

		if (restrict_data_peer_enabled != _restrict_data_peer_enabled) {
			_restrict_data_peer_enabled = restrict_data_peer_enabled;
			SetEnabledDialogControl(_i_restrict_data_peer, restrict_data_peer_enabled);
		}
	}

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if ( msg == DN_INITDIALOG || (msg == DN_BTNCLICK && param1 != -1
			&& (param1 == _i_explicit_encryption || param1 == _i_passive_mode)) )
		{
			UpdateEnableds();
		}

		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:

	ProtocolOptionsFTP(bool implicit_encryption)
		: _implicit_encryption(implicit_encryption)
	{
		_di_encryption_protocol.Add("SSL2");
		_di_encryption_protocol.Add("SSL3");
		_di_encryption_protocol.Add("TLS1.0");
		_di_encryption_protocol.Add("TLS1.1");
		_di_encryption_protocol.Add("TLS1.2");

		_di_list_command.Add(MFTPListAutodetect);
		_di_list_command.Add("LIST -la");
		_di_list_command.Add("LIST");

		_di.SetBoxTitleItem(implicit_encryption ? MFTPSOptionsTitle : MFTPOptionsTitle);

		_di.SetLine(2);

		if (!_implicit_encryption) {
			_i_explicit_encryption = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPExplicitEncryption);
			_di.NextLine();
		}

		_di.AddAtLine(DI_TEXT, 5,44, 0, MSFTPEncryptionProtocol);
		_i_encryption_protocol = _di.AddAtLine(DI_COMBOBOX, 45,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_encryption_protocol].ListItems = _di_encryption_protocol.Get();
		_di.NextLine();

		_di.AddAtLine(DI_TEXT, 5,44, 0, MSFTPListCommand);
		_i_list_command = _di.AddAtLine(DI_COMBOBOX, 45,62, 0, MFTPListAutodetect);
		_di[_i_list_command].ListItems = _di_list_command.Get();
		_di.NextLine();

		_i_use_mlsd_mlst = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPUseMLSDMLST);
		_di.NextLine();

		_i_passive_mode = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPPassiveMode);
		_di.NextLine();

		_i_commands_pipelining = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPCommandsPipelining);
		_di.NextLine();

		_i_restrict_data_peer = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPRestrictDataPeer);
		_di.NextLine();

		_i_tcp_nodelay = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPTCPNoDelay);
		_di.NextLine();

#ifdef __linux__
		_i_tcp_quickack = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPQuickAck);
		_di.NextLine();
#endif

		_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);
		_di.NextLine();

		_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}


	void Configure(std::string &options)
	{
		StringConfig sc(options);

		//GetDialogListPosition(_i_auth_mode)

		_di_encryption_protocol.SelectIndex(sc.GetInt("EncryptionProtocol", 3)); // default: TLS1.1
		if (_i_explicit_encryption != -1) {
			SetCheckedDialogControl(_i_explicit_encryption, sc.GetInt("ExplicitEncryption", 0) != 0);
		}
		const auto &list_cmd = sc.GetString("ListCommand", "");
		if (!list_cmd.empty())
			TextToDialogControl(_i_list_command, list_cmd);

		SetCheckedDialogControl(_i_passive_mode, sc.GetInt("Passive", 1) != 0);
		SetCheckedDialogControl(_i_use_mlsd_mlst, sc.GetInt("MLSDMLST", 1) != 0);
		SetCheckedDialogControl(_i_commands_pipelining, sc.GetInt("CommandsPipelining", 0) != 0);
		SetCheckedDialogControl(_i_restrict_data_peer, sc.GetInt("RestrictDataPeer", 1) != 0);
		SetCheckedDialogControl(_i_tcp_nodelay, sc.GetInt("TcpNoDelay", 1) != 0);
		if (_i_tcp_quickack != -1) {
			SetCheckedDialogControl(_i_tcp_quickack, sc.GetInt("TcpQuickAck", 0) != 0);
		}

		if (Show(L"ProtocolOptionsFTP", 6, 2) == _i_ok) {
			if (_i_explicit_encryption != -1) {
				sc.SetInt("ExplicitEncryption", IsCheckedDialogControl(_i_explicit_encryption));
			}
			std::string str;
			TextFromDialogControl(_i_list_command, str);
			if (!str.empty() && str.front() == '<' && str.back() == '>') {
				str.clear();
			}
			sc.SetString("ListCommand", str);
			sc.SetInt("EncryptionProtocol", GetDialogListPosition(_i_encryption_protocol));
			sc.SetInt("Passive", IsCheckedDialogControl(_i_passive_mode) ? 1 : 0);
			sc.SetInt("MLSDMLST", IsCheckedDialogControl(_i_use_mlsd_mlst) ? 1 : 0);
			sc.SetInt("CommandsPipelining", IsCheckedDialogControl(_i_commands_pipelining) ? 1 : 0);
			sc.SetInt("RestrictDataPeer", IsCheckedDialogControl(_i_restrict_data_peer) ? 1 : 0);
			sc.SetInt("TcpNoDelay", IsCheckedDialogControl(_i_tcp_nodelay) ? 1 : 0);
			if (_i_tcp_quickack != -1) {
				sc.SetInt("TcpQuickAck", IsCheckedDialogControl(_i_tcp_quickack) ? 1 : 0);
			}

			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolFTP(std::string &options)
{
	ProtocolOptionsFTP(false).Configure(options);
}

void ConfigureProtocolFTPS(std::string &options)
{
	ProtocolOptionsFTP(true).Configure(options);
}
