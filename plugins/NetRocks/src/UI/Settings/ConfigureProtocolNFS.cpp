#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ===== NFS Protocol options ================
| [x] Override user identity:               |
|  Host:     [9999999                     ] |
|  UID:      [9999999                     ] |
|  GID:      [9999999                     ] |
|  Groups:   [9999999                     ] |
|-------------------------------------------|
| [  OK    ]    [ Cancel ]                  |
 ===========================================
    6                     29       38
*/

class ProtocolOptionsNFS : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_override = -1, _i_host = -1, _i_uid = -1, _i_gid = -1, _i_groups = -1;

	void UpdateEnableds()
	{
		const bool en = IsCheckedDialogControl(_i_override);
		SetEnabledDialogControl(_i_host, en);
		SetEnabledDialogControl(_i_uid, en);
		SetEnabledDialogControl(_i_gid, en);
		SetEnabledDialogControl(_i_groups, en);
	}

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_INITDIALOG
		 || (msg == DN_BTNCLICK && param1 == _i_override)) {
			UpdateEnableds();
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	ProtocolOptionsNFS()
	{
		_di.SetBoxTitleItem(MNFSOptionsTitle);

		_di.SetLine(2);
		_i_override = _di.AddAtLine(DI_CHECKBOX, 5,48, 0, MNFSOverride);

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,15, 0, MNFSHost);
		_i_host = _di.AddAtLine(DI_EDIT, 16,48, 0, "0", "0");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,15, 0, MNFSUID);
		_i_uid = _di.AddAtLine(DI_FIXEDIT, 16,48, DIF_MASKEDIT, "0", "9999999999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,15, 0, MNFSGID);
		_i_gid = _di.AddAtLine(DI_FIXEDIT, 16,48, DIF_MASKEDIT, "0", "9999999999");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 6,15, 0, MNFSGroups);
		_i_groups = _di.AddAtLine(DI_EDIT, 16,48, 0, "0", "0");

		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

		_di.NextLine();
		_i_ok = _di.AddAtLine(DI_BUTTON, 7,11, DIF_CENTERGROUP, MOK);
		_i_cancel = _di.AddAtLine(DI_BUTTON, 12,23, DIF_CENTERGROUP, MCancel);

		SetFocusedDialogControl(_i_ok);
		SetDefaultDialogControl(_i_ok);
	}


	void Configure(std::string &options)
	{
		StringConfig sc(options);
		SetCheckedDialogControl( _i_override, sc.GetInt("Override", 0) != 0);
		TextToDialogControl(_i_host, sc.GetString("Host"));
		TextToDialogControl(_i_groups, sc.GetString("Groups"));
		LongLongToDialogControl(_i_uid, sc.GetInt("UID", 65534));
		LongLongToDialogControl(_i_gid, sc.GetInt("GID", 65534));
		if (Show(L"ProtocolOptionsNFS", 6, 2) == _i_ok) {
			sc.SetInt("Override", IsCheckedDialogControl(_i_override) ? 1 : 0);
			std::string str;
			TextFromDialogControl(_i_host, str);
			sc.SetString("Host", str);
			TextFromDialogControl(_i_groups, str);
			sc.SetString("Groups", str);
			sc.SetInt("UID", (int)LongLongFromDialogControl(_i_uid));
			sc.SetInt("GID", (int)LongLongFromDialogControl(_i_gid));
			options = sc.Serialize();
		}
	}
};

void ConfigureProtocolNFS(std::string &options)
{
	ProtocolOptionsNFS().Configure(options);
}

