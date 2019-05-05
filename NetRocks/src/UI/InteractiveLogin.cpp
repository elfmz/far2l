#include <utils.h>
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "../lng.h"
#include "../PooledStrings.h"

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
		_i_connect = _di.AddAtLine(DI_BUTTON, 6,23, DIF_CENTERGROUP, MConnect, nullptr, FDIS_DEFAULT);
		_di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MCancel);

		if (retry) {
			char sz[0x200] = {};
			snprintf(sz, sizeof(sz) - 1, "%ls (%u)", _di[_i_dblbox].PtrData, retry);
			_di[_i_dblbox].PtrData = MB2WidePooled(sz);
		}
	}

	bool Ask(std::string &username, std::string &password)
	{
		_di[_i_username].PtrData = MB2WidePooled(username);
		_di[_i_password].PtrData = MB2WidePooled(password);

		if (Show(L"InteractiveLoginDialog", 6, 2) != _i_connect)
			return false;

		Wide2MB(_di[_i_username].PtrData, username);
		Wide2MB(_di[_i_password].PtrData, password);
		return true;
	}
};

bool InteractiveLogin(const std::string &display_name, unsigned int retry, std::string &username, std::string &password)
{
	return InteractiveLoginDialog(display_name, retry).Ask(username, password);
}
