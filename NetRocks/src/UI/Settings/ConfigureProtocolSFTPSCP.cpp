#include <algorithm>
#include <utils.h>
#include <Environment.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ SFTP Protocol options =====================
| Authentification:     [COMBOBOX Private key file path:   ] |
| [EDIT....................................................] |
| [ ] Custom subsystem request:                              |
| [EDIT....................................................] |
| Compression:          [COMBOBOX Compressed traffic       ] |
| Max read block size, bytes:                 [9999999]      |
| Max write block size, bytes:                [9999999]      |
| Automatically retry connect, times:         [##]           |
| Connection timeout, seconds:                [###]          |
| [ ] Enable TCP_NODELAY option                              |
| [ ] Enable TCP_QUICKACK option                             |
| [ ] Use OpenSSH config files                               |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

class ProtocolOptionsSFTPSCP : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_auth_mode = -1, _i_privkey_path = -1;
	int _i_use_custom_subsystem = -1, _i_custom_subsystem = -1;
	int _i_compression = -1;
	int _i_max_read_block_size = -1, _i_max_write_block_size = -1;
	int _i_connect_retries = -1, _i_connect_timeout = -1;
	int _i_tcp_nodelay = -1, _i_tcp_quickack = -1;
	int _i_use_openssh_configs = -1;

	FarListWrapper _di_authmode;
	FarListWrapper _di_compression;

	bool _keypath_enabled = true, _subsystem_enabled = true, _ok_enabled = true;

	void UpdateEnableds(bool due_authmode_changed)
	{
		bool ok_enabled = true, keypath_enabled = false, subsystem_enabled = false;
		if (GetDialogListPosition(_i_auth_mode) == 2) { // keyfile
			ok_enabled = false;
			keypath_enabled = true;
			std::string str;
			TextFromDialogControl(_i_privkey_path, str);
			if (str.empty() && due_authmode_changed) {
				str = "~/.ssh/id_rsa";
				TextToDialogControl(_i_privkey_path, str);
			}
			Environment::ExplodeCommandLine ecl(str);
			for (const auto &part : ecl) {
				struct stat s{};
				if (!part.empty() && stat(part.c_str(), &s) == 0 && S_ISREG(s.st_mode)) {
					ok_enabled = true;
					break;
				}
			}
		}

		subsystem_enabled = IsCheckedDialogControl(_i_use_custom_subsystem);

		if (ok_enabled && subsystem_enabled) {
			std::string str;
			TextFromDialogControl(_i_privkey_path, str);
			if (str.empty()) {
				ok_enabled = false;
			}
		}

		if (ok_enabled != _ok_enabled) {
			_ok_enabled = ok_enabled;
			SetEnabledDialogControl(_i_ok, ok_enabled);
		}

		if (keypath_enabled != _keypath_enabled) {
			_keypath_enabled = keypath_enabled;
			SetEnabledDialogControl(_i_privkey_path, keypath_enabled);
		}

		if (_i_custom_subsystem != -1 && subsystem_enabled != _subsystem_enabled) {
			_subsystem_enabled = subsystem_enabled;
			SetEnabledDialogControl(_i_custom_subsystem, subsystem_enabled);
		}

	}

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
#ifndef __linux__
		if ( msg == DN_INITDIALOG) {
			SetEnabledDialogControl(_i_tcp_quickack, false);
		}
#endif

		if ( msg == DN_INITDIALOG
		|| (msg == DN_BTNCLICK && (param1 == _i_use_custom_subsystem))
		|| (msg == DN_EDITCHANGE && (param1 == _i_auth_mode || param1 == _i_privkey_path || param1 == _i_custom_subsystem)) ) {
			UpdateEnableds(msg == DN_EDITCHANGE && param1 == _i_auth_mode);
		}

		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	ProtocolOptionsSFTPSCP(bool scp)
	{
		_di_authmode.Add(MSFTPAuthModeUserPassword);
		_di_authmode.Add(MSFTPAuthModeUserPasswordInteractive);
		_di_authmode.Add(MSFTPAuthModeKeyFile);
		_di_authmode.Add(MSFTPAuthModeSSHAgent);

		_di_compression.Add(MSFTPCompressionNone);
		_di_compression.Add(MSFTPCompressionIncoming);
		_di_compression.Add(MSFTPCompressionOutgoing);
		_di_compression.Add(MSFTPCompressionAll);

		_di.SetBoxTitleItem(scp ? MSCPOptionsTitle : MSFTPOptionsTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,26, 0, MSFTPAuthMode);
		_i_auth_mode = _di.AddAtLine(DI_COMBOBOX, 27,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_auth_mode].ListItems = _di_authmode.Get();
		_di.NextLine();
		_i_privkey_path = _di.AddAtLine(DI_EDIT, 5,62, 0, "");
		_di.NextLine();

		_di.AddAtLine(DI_TEXT, 5,26, 0, MSFTPCompression);
		_i_compression = _di.AddAtLine(DI_COMBOBOX, 27,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_compression].ListItems = _di_compression.Get();
		_di.NextLine();

		if (!scp)  {
			_i_use_custom_subsystem = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MSFTPCustomSubsystem);
			_di.NextLine();
			_i_custom_subsystem = _di.AddAtLine(DI_EDIT, 5,62, 0, "");

			_di.NextLine();
			_di.AddAtLine(DI_TEXT, 5,50, 0, MSFTPMaxReadBlockSize);
			_i_max_read_block_size = _di.AddAtLine(DI_FIXEDIT, 51,60, DIF_MASKEDIT, "32768", "9999999999");

			_di.NextLine();
			_di.AddAtLine(DI_TEXT, 5,50, 0, MSFTPMaxWriteBlockSize);
			_i_max_write_block_size = _di.AddAtLine(DI_FIXEDIT, 51,60, DIF_MASKEDIT, "32768", "9999999999");
			_di.NextLine();
		}

		_di.AddAtLine(DI_TEXT, 5,50, 0, MSFTPConnectRetries);
		_i_connect_retries = _di.AddAtLine(DI_FIXEDIT, 51,52, DIF_MASKEDIT, "1", "99");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,50, 0, MSFTPConnectTimeout);
		_i_connect_timeout = _di.AddAtLine(DI_FIXEDIT, 51,53, DIF_MASKEDIT, "20", "999");

		_di.NextLine();
		_i_tcp_nodelay = _di.AddAtLine(DI_CHECKBOX, 5,60, 0, MSFTPTCPNodelay);

		_di.NextLine();
		_i_tcp_quickack = _di.AddAtLine(DI_CHECKBOX, 5,60, 0, MSFTPTCPQuickAck);

		_di.NextLine();
		_i_use_openssh_configs = _di.AddAtLine(DI_CHECKBOX, 5,60, 0, MSFTPUseOpenSSHConfigs);

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
		StringConfig sc(options);
		//GetDialogListPosition(_i_auth_mode)
		if (sc.GetInt("SSHAgentEnable", 0) != 0) {
			SetDialogListPosition(_i_auth_mode, 3);

		} else if (sc.GetInt("PrivKeyEnable", 0) != 0) {
			SetDialogListPosition(_i_auth_mode, 2);

		} else if (sc.GetInt("InteractiveLogin", 0) != 0) {
			SetDialogListPosition(_i_auth_mode, 1);

		} else {
			SetDialogListPosition(_i_auth_mode, 0);
		}

		SetDialogListPosition(_i_compression, // 0 - none, 1 - incoming, 2 - outgoing, 3 - all
			std::min(std::max(0, sc.GetInt("Compression", 0)), 3));

		TextToDialogControl(_i_privkey_path, sc.GetString("PrivKeyPath"));

		if (_i_max_read_block_size != -1) {
			LongLongToDialogControl(_i_max_read_block_size, std::max((int)512, sc.GetInt("MaxReadBlock", 32768)));
		}
		if (_i_max_write_block_size != -1) {
			LongLongToDialogControl(_i_max_write_block_size, std::max((int)512, sc.GetInt("MaxWriteBlock", 32768)));
		}

		SetCheckedDialogControl(_i_tcp_nodelay, sc.GetInt("TcpNoDelay", 1) != 0);
		SetCheckedDialogControl(_i_tcp_quickack, sc.GetInt("TcpQuickAck", 0) != 0);
		SetCheckedDialogControl(_i_use_openssh_configs, sc.GetInt("UseOpenSSHConfigs", 0) != 0);

		LongLongToDialogControl(_i_connect_retries, std::max((int)1, sc.GetInt("ConnectRetries", 2)));
		LongLongToDialogControl(_i_connect_timeout, std::max((int)1, sc.GetInt("ConnectTimeout", 20)));

		if (_i_use_custom_subsystem != -1) {
			SetCheckedDialogControl(_i_use_custom_subsystem, sc.GetInt("UseCustomSubsystem", 0) != 0);
		}
		if (_i_custom_subsystem != -1) {
			TextToDialogControl(_i_custom_subsystem, sc.GetString("CustomSubsystem"));
		}
	//	SetCheckedDialogControl(_i_enable_sandbox, sc.GetInt("Sandbox", 0) != 0);
		if (Show(L"ProtocolOptionsSFTPSCP", 6, 2) == _i_ok) {
			sc.Delete("SSHAgentEnable");
			sc.Delete("PrivKeyEnable");
			sc.Delete("InteractiveLogin");
			switch (GetDialogListPosition(_i_auth_mode)) {
				case 1: sc.SetInt("InteractiveLogin", 1); break;
				case 2: sc.SetInt("PrivKeyEnable", 1); break;
				case 3: sc.SetInt("SSHAgentEnable", 1); break;
			}

			sc.SetInt("Compression", GetDialogListPosition(_i_compression));

			std::string str;
			TextFromDialogControl(_i_privkey_path, str);
			sc.SetString("PrivKeyPath", str);
			if (_i_max_read_block_size != -1) {
				sc.SetInt("MaxReadBlock", std::max((int)512, (int)LongLongFromDialogControl(_i_max_read_block_size)));
			}
			if (_i_max_write_block_size != -1) {
				sc.SetInt("MaxWriteBlock", std::max((int)512, (int)LongLongFromDialogControl(_i_max_write_block_size)));
			}
			sc.SetInt("TcpNoDelay", IsCheckedDialogControl(_i_tcp_nodelay) ? 1 : 0);
			sc.SetInt("TcpQuickAck", IsCheckedDialogControl(_i_tcp_quickack) ? 1 : 0);
			sc.SetInt("UseOpenSSHConfigs", IsCheckedDialogControl(_i_use_openssh_configs) ? 1 : 0);

			sc.SetInt("ConnectRetries", std::max((int)1, (int)LongLongFromDialogControl(_i_connect_retries)));
			sc.SetInt("ConnectTimeout", std::max((int)1, (int)LongLongFromDialogControl(_i_connect_timeout)));

			if (_i_use_custom_subsystem != -1) {
				sc.SetInt("UseCustomSubsystem", IsCheckedDialogControl(_i_use_custom_subsystem) ? 1 : 0);
			}
			if (_i_custom_subsystem != -1) {
				TextFromDialogControl(_i_custom_subsystem, str);
				sc.SetString("CustomSubsystem", str);
			}

	//		sc.SetInt("Sandbox", IsCheckedDialogControl(_i_enable_sandbox) ? 1 : 0);
			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolSFTP(std::string &options)
{
	ProtocolOptionsSFTPSCP(false).Configure(options);
}

void ConfigureProtocolSCP(std::string &options)
{
	ProtocolOptionsSFTPSCP(true).Configure(options);
}
