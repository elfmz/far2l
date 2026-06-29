#include <utils.h>
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../../lng.h"
#include "../../PooledStrings.h"

/*
345                                            50
 ============== Login credentials =============
| Connecting to :     [TEXTBOX               ] |
| YUsername:          [EDITBOX               ] |
| UPassword:          [EDITBOX               ] |
|----------------------------------------------|
|   [       Ok      ]      [    Cancel    ]    |
 ==============================================
    6                     29       38
*/

class InteractiveLoginDialog : BaseDialog
{
	int _i_dblbox, _i_username, _i_password, _i_connect;
public:
	InteractiveLoginDialog(const std::string &display_name, unsigned int retry)
	{
		_i_dblbox = _di.SetBoxTitleItem(retry ? MLoginAuthRetryTitle : MLoginAuthTitle);
		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,24, 0, retry ? MLoginAuthRetryTo : MLoginAuthTo);
		_di.AddAtLine(DI_TEXT, 25,48, 0, display_name.c_str());

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,24, 0, MUserName);
		_i_username = _di.AddAtLine(DI_EDIT, 28,48, DIF_HISTORY, "", "NetRocks_History_User");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,24, 0, MPassword);
		_i_password = _di.AddAtLine(DI_PSWEDIT, 28,48, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_connect = _di.AddAtLine(DI_BUTTON, 6,23, DIF_CENTERGROUP, MOK);
		_di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MCancel);

		if (retry) {
			std::string title;
			TextFromDialogControl(_i_dblbox, title);
			title+= StrPrintf(" (%u)", retry);
			TextToDialogControl(_i_dblbox, title);
		}

		SetFocusedDialogControl(_i_username);
		SetDefaultDialogControl(_i_connect);
	}

	bool Ask(std::string &username, std::string &password)
	{
		TextToDialogControl(_i_username, username);
		TextToDialogControl(_i_password, password);

		SetFocusedDialogControl((username.empty() || !password.empty()) ? _i_username : _i_password);

		if (Show(L"InteractiveLoginDialog", 6, 2) != _i_connect)
			return false;

		TextFromDialogControl(_i_username, username);
		TextFromDialogControl(_i_password, password);
		//fprintf(stderr, "username='%s', password='%s'\n", username.c_str(), password.c_str());
		return true;
	}
};

bool InteractiveLogin(const std::string &display_name, unsigned int retry, std::string &username, std::string &password)
{
	return InteractiveLoginDialog(display_name, retry).Ask(username, password);
}

/*
345                                            50
 =========== Private key passphrase ===========
| Unlocking key for:                             |
| <sftp:91.188.214.92 - keyfile with pass>      |
| Passphrase:                                    |
| [EDITBOX                                     ] |
|-----------------------------------------------|
|   [       Ok      ]      [    Cancel    ]      |
 ================================================
    6                     29       38
*/

class InteractivePassphraseDialog : BaseDialog
{
	int _i_dblbox, _i_password, _i_connect;
	bool _select_all = false;
public:
	InteractivePassphraseDialog(const std::string &display_name, unsigned int retry, const std::string &info)
	{
		_i_dblbox = _di.SetBoxTitleItem(retry ? MKeyPassphraseRetryTitle : MKeyPassphraseTitle);
		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,26, 0, retry ? MKeyPassphraseRetryTo : MKeyPassphraseTo);
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 7,48, 0, display_name.c_str());
		if (!info.empty()) {
			_di.NextLine();
			_di.AddAtLine(DI_TEXT, 7,48, 0, info.c_str());
		}

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,26, 0, MPassphrase);
		_di.NextLine();
		_i_password = _di.AddAtLine(DI_PSWEDIT, 7,48, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_connect = _di.AddAtLine(DI_BUTTON, 6,23, DIF_CENTERGROUP, MOK);
		_di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MCancel);

		if (retry) {
			std::string title;
			TextFromDialogControl(_i_dblbox, title);
			title += StrPrintf(" (%u)", retry);
			TextToDialogControl(_i_dblbox, title);
		}

		SetFocusedDialogControl(_i_password);
		SetDefaultDialogControl(_i_connect);
	}

	bool Ask(std::string &password)
	{
		TextToDialogControl(_i_password, password);
		_select_all = !password.empty();

		if (Show(L"InteractivePassphraseDialog", 6, 2) != _i_connect)
			return false;

		TextFromDialogControl(_i_password, password);
		return true;
	}

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override
	{
		if (msg == DN_INITDIALOG && _select_all) {
			EditorSelect es;
			es.BlockType = BTYPE_STREAM;
			es.BlockStartLine = 0;
			es.BlockStartPos = 0;
			es.BlockWidth = 99999;
			es.BlockHeight = 1;
			SendDlgMessage(DM_SETSELECTION, _i_password, (LONG_PTR)&es);
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}
};

bool InteractivePassphrase(const std::string &display_name, unsigned int retry, const std::string &info, std::string &password)
{
	return InteractivePassphraseDialog(display_name, retry, info).Ask(password);
}
