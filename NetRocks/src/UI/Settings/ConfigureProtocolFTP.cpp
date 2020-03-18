#include <algorithm>
#include <utils.h>
#include <WordExpansion.h>
#include <StringConfig.h>
#include <mutex>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================= FTP Protocol options =====================
| [ ] Enable explicit encryption                             |
| Minimal encryption protocol:           [COMBOBOX         ] |
| Server codepage:                       [COMBOBOX         ] |
| [ ] Use passive mode for transfers                         |
| [ ] Use MLSD/MLST if possible                              |
| [ ] Enable TCP_NODELAY option                              |
| [ ] Enable TCP_QUICKACK option                             |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

static std::mutex s_system_codepages_mutex;
static std::set<std::string> s_system_codepages;

static BOOL __stdcall ProtocolOptionsFTP_EnumCPProc(const wchar_t *lpwszCodePage)
{
	if (lpwszCodePage && *lpwszCodePage) {
		s_system_codepages.insert(Wide2MB(lpwszCodePage));
	}
	return TRUE;
}

class ProtocolOptionsFTP : protected BaseDialog
{
	bool _implicit_encryption;
	bool _encryption_protocol_enabled = false;

	int _i_ok = -1, _i_cancel = -1;

	int _i_explicit_encryption = -1;
	int _i_encryption_protocol = -1;
	int _i_server_codepage = -1;
	int _i_passive_mode = -1, _i_use_mlsd_mlst = -1;
	int _i_tcp_nodelay = -1, _i_tcp_quickack = -1;
	//int _i_enable_sandbox = -1;

	FarListWrapper _di_encryption_protocol, _di_server_codepage;

	void UpdateEnableds()
	{
		const bool encryption_protocol_enabled =
			(_implicit_encryption || IsCheckedDialogControl(_i_explicit_encryption));

		if (encryption_protocol_enabled != _encryption_protocol_enabled) {
			_encryption_protocol_enabled = encryption_protocol_enabled;
			SetEnabledDialogControl(_i_encryption_protocol, encryption_protocol_enabled);
		}
	}

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if ( msg == DN_INITDIALOG
		|| (msg == DN_BTNCLICK && param1 != -1 && param1 == _i_explicit_encryption) ) {
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

		_di_encryption_protocol.Add(MSFTPAuthModeKeyFile);

		_di_server_codepage.Add("UTF-8");
		{
			std::unique_lock<std::mutex> lock(s_system_codepages_mutex);
			if (s_system_codepages.empty()) {
				WINPORT(EnumSystemCodePages)((CODEPAGE_ENUMPROCW)ProtocolOptionsFTP_EnumCPProc, 0);//CP_INSTALLED
			}

			for (const auto &cp : s_system_codepages) {
				_di_server_codepage.Add(cp.c_str());
			}
		}

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

		_di.AddAtLine(DI_TEXT, 5,44, 0, MSFTPServerCodepage);
		_i_server_codepage = _di.AddAtLine(DI_COMBOBOX, 45,62, DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND, "");
		_di[_i_server_codepage].ListItems = _di_server_codepage.Get();
		_di.NextLine();

		_i_passive_mode = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPPassiveMode);
		_di.NextLine();

		_i_use_mlsd_mlst = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPUseMLSDMLST);
		_di.NextLine();

		_i_tcp_nodelay = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MFTPTCPNoDelay);
		_di.NextLine();

#ifndef __linux__
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
		if (!_di_server_codepage.Select(sc.GetString("ServerCodepage", "").c_str())) {
			_di_server_codepage.SelectIndex(0);
		}

		if (_i_explicit_encryption != -1) {
			SetCheckedDialogControl(_i_explicit_encryption, sc.GetInt("ExplicitEncryption", 0) != 0);
		}

		SetCheckedDialogControl(_i_passive_mode, sc.GetInt("Passive", 1) != 0);
		SetCheckedDialogControl(_i_use_mlsd_mlst, sc.GetInt("MLSDMLST", 1) != 0);

		SetCheckedDialogControl(_i_tcp_nodelay, sc.GetInt("TcpNoDelay", 1) != 0);
		if (_i_tcp_quickack != -1) {
			SetCheckedDialogControl(_i_tcp_quickack, sc.GetInt("TcpQuickAck", 0) != 0);
		}

		if (Show(L"ProtocolOptionsFTP", 6, 2) == _i_ok) {
			if (_i_explicit_encryption != -1) {
				sc.SetInt("ExplicitEncryption", IsCheckedDialogControl(_i_explicit_encryption));
			}
			sc.SetInt("EncryptionProtocol", GetDialogListPosition(_i_encryption_protocol));
			std::string str;
			TextFromDialogControl(_i_server_codepage, str);
			sc.SetString("ServerCodepage", str);
			sc.SetInt("Passive", IsCheckedDialogControl(_i_passive_mode) ? 1 : 0);
			sc.SetInt("MLSDMLST", IsCheckedDialogControl(_i_use_mlsd_mlst) ? 1 : 0);
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
