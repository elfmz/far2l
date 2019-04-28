#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "../lng.h"

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
	int _i_dblbox, _i_username, _i_password, _i_ok;
public:
	InteractiveLoginDialog(const std::string &display_name, unsigned int retry)
	{
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,50,7, 0, retry ? MLoginAuthRetryTitle : MLoginAuthTitle);
		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 5,24, 0, retry ? MLoginAuthRetryTo : MLoginAuthTo);
		_di.AddAtLine(DI_TEXT, 25,48, 0, display_name.c_str());

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,24, 0, MUserName);
		_i_username = _di.AddAtLine(DI_EDIT, 28,48, DIF_HISTORY, "", "NetRocks_History_User", FDIS_FOCUSED);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,24, 0, MPassword);
		_i_password = _di.AddAtLine(DI_PSWEDIT, 28,48, 0, "");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_ok = _di.AddAtLine(DI_BUTTON, 6,23, DIF_CENTERGROUP, MOK, nullptr, FDIS_DEFAULT);
		_di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MCancel);

		if (retry) {
			char sz[32] = {};
			snprintf(sz, sizeof(sz) - 1, " (%u)", retry);
			strncat(_di[_i_dblbox].Data, sz, sizeof(_di[_i_username].Data));
		}
	}

	bool Ask(std::string &username, std::string &password)
	{
		strncpy(_di[_i_username].Data, username.c_str(), sizeof(_di[_i_username].Data));
		strncpy(_di[_i_password].Data, password.c_str(), sizeof(_di[_i_password].Data));

		if (Show(_di[_i_dblbox].Data, 6, 2) != _i_ok)
			return false;

		username = _di[_i_username].Data;
		password = _di[_i_password].Data;
		return true;
	}
};

bool InteractiveLogin(const std::string &display_name, unsigned int retry, std::string &username, std::string &password)
{
	return InteractiveLoginDialog(display_name, retry).Ask(username, password);
}
