#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ SFTP Protocol options =====================
| Max IO block size, bytes:                   [9999999]      |
| [ ] Enable TCP_NODELAY option (libssh >= v0.8.0)           |
| [ ] Enable sandbox (if you dont trust server)              |
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

class ProtocolOptionsSFTP : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_max_io_block_size = -1;
	int _i_tcp_nodelay = -1;
	//int _i_enable_sandbox = -1;

public:
	ProtocolOptionsSFTP()
	{
		_di.Add(DI_DOUBLEBOX, 3,1,64,6, 0, MSFTPOptionsTitle);
		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,50, 0, MSFTPMaxBlockSize);
		_i_max_io_block_size = _di.AddAtLine(DI_FIXEDIT, 51,60, DIF_MASKEDIT, "32768", "9999999999");

		_di.NextLine();
		_i_tcp_nodelay = _di.AddAtLine(DI_CHECKBOX, 5,60, 0, MSFTPTCPNodelay);

	//	_di.NextLine();
	//	_i_enable_sandbox = _di.AddAtLine(DI_CHECKBOX, 5,60, 0, MSFTPEnableSandbox);

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
		LongLongToDialogControl(_i_max_io_block_size, std::max((int)512, sc.GetInt("MaxIOBlock", 32768)));
		SetCheckedDialogControl(_i_tcp_nodelay, sc.GetInt("TcpNoDelay", 0) != 0);
	//	SetCheckedDialogControl(_i_enable_sandbox, sc.GetInt("Sandbox", 0) != 0);

		if (Show(L"ProtocolOptionsSFTP", 6, 2) == _i_ok) {
			sc.SetInt("MaxIOBlock", std::max((int)512, (int)LongLongFromDialogControl(_i_max_io_block_size)));
			sc.SetInt("TcpNoDelay", IsCheckedDialogControl(_i_tcp_nodelay) ? 1 : 0);
	//		sc.SetInt("Sandbox", IsCheckedDialogControl(_i_enable_sandbox) ? 1 : 0);
			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolSFTP(std::string &options)
{
	ProtocolOptionsSFTP().Configure(options);
}
