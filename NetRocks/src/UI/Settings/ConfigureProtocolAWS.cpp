#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                    26           39       48            60  64
 ===== WebDAV Protocol options ================
| User agent:          [                     ] |
| Region:              [                     ] |
| [x] Connect via proxy:                       |
|  Proxy host:         [                     ] |
|  Proxy port          [9999999              ] |
|  [x] Use proxy authentificaion:              |
|   Proxy user:         [                    ] |
|   Proxy password:     [                    ] |
|----------------------------------------------|
| [  OK    ]    [ Cancel ]                     |
 ==============================================
    6                     29       38
*/

class ProtocolOptionsAWS : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_user_agent = -1;
	int _i_region = -1;
	int _i_use_proxy = -1, _i_proxy_host = -1, _i_proxy_port = -1;
	int _i_auth_proxy = -1, _i_proxy_username = -1, _i_proxy_password = -1;

	void UpdateEnableds()
	{
		const bool use_proxy = IsCheckedDialogControl(_i_use_proxy);
		SetEnabledDialogControl(_i_proxy_host, use_proxy);
		SetEnabledDialogControl(_i_proxy_port, use_proxy);
		SetEnabledDialogControl(_i_auth_proxy, use_proxy);

		const bool auth_proxy = use_proxy && IsCheckedDialogControl(_i_auth_proxy);
		SetEnabledDialogControl(_i_proxy_username, auth_proxy);
		SetEnabledDialogControl(_i_proxy_password, auth_proxy);
	}

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_INITDIALOG
		 || (msg == DN_BTNCLICK)) {
			UpdateEnableds();
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	ProtocolOptionsAWS()
	{
		_di.SetBoxTitleItem(MAWSOprionTitle);

		_di.SetLine(2);
		_di.AddAtLine(DI_TEXT, 6,25, 0, MAWSUserAgent);
		_i_user_agent = _di.AddAtLine(DI_EDIT, 26,48, 0, "0", "0");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,25, 0, MAWSRegion);
		_i_region = _di.AddAtLine(DI_EDIT, 26,48, 0, "0", "0");

		_di.NextLine();
		_i_use_proxy = _di.AddAtLine(DI_CHECKBOX, 5,48, 0, MAWSUseProxy);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,25, 0, MAWSProxyHost);
		_i_proxy_host = _di.AddAtLine(DI_EDIT, 26,48, 0, "0", "0");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,25, 0, MAWSProxyPort);
		_i_proxy_port = _di.AddAtLine(DI_FIXEDIT, 26,31, DIF_MASKEDIT, "0", "99999");

		_di.NextLine();
		_i_auth_proxy = _di.AddAtLine(DI_CHECKBOX, 6,48, 0, MAWSAuthProxy);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 7,25, 0, MAWSProxyUsername);
		_i_proxy_username = _di.AddAtLine(DI_EDIT, 27,48, 0, "0", "0");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 7,25, 0, MAWSProxyPassword);
		_i_proxy_password = _di.AddAtLine(DI_EDIT, 27,48, 0, "0", "0");

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

		TextToDialogControl(_i_user_agent, sc.GetString("UserAgent"));
		TextToDialogControl(_i_region, sc.GetString("Region"));
		SetCheckedDialogControl( _i_use_proxy, sc.GetInt("UseProxy", 0) != 0);
		TextToDialogControl(_i_proxy_host, sc.GetString("ProxyHost"));
		LongLongToDialogControl(_i_proxy_port, sc.GetInt("ProxyPort", 8080));
		SetCheckedDialogControl( _i_auth_proxy, sc.GetInt("AuthProxy", 0) != 0);
		TextToDialogControl(_i_proxy_username, sc.GetString("ProxyUsername"));
		TextToDialogControl(_i_proxy_password, sc.GetString("ProxyPassword"));

		if (Show(L"ProtocolOptionsAWS", 6, 2) == _i_ok) {
			std::string str;

			TextFromDialogControl(_i_user_agent, str);
			sc.SetString("UserAgent", str);
			TextFromDialogControl(_i_region, str);
			sc.SetString("Region", str);

			sc.SetInt("UseProxy", IsCheckedDialogControl(_i_use_proxy) ? 1 : 0);
			TextFromDialogControl(_i_proxy_host, str);
			sc.SetString("ProxyHost", str);
			TextFromDialogControl(_i_proxy_port, str);
			sc.SetString("ProxyPort", str);

			sc.SetInt("AuthProxy", IsCheckedDialogControl(_i_auth_proxy) ? 1 : 0);
			TextFromDialogControl(_i_proxy_username, str);
			sc.SetString("ProxyUsername", str);
			TextFromDialogControl(_i_proxy_password, str);
			sc.SetString("ProxyPassword", str);

			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolAWS(std::string &options)
{
	ProtocolOptionsAWS().Configure(options);
}
