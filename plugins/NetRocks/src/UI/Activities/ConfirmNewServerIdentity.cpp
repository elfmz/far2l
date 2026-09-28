#include <utils.h>
#include "ConfirmNewServerIdentity.h"
#include "../../Globals.h"



/*
345                                            60
 ======== Confirm new server identify =========
| Site:          [                           ] |
| New identity:                                |
|  [                                         ] |
|----------------------------------------------|
|   [       Ok      ]      [    Cancel    ]    |
 ==============================================
    6                     29       38
*/

ConfirmNewServerIdentity::ConfirmNewServerIdentity(const std::string &site, const std::string &identity, bool may_remember)
{
	_di.SetBoxTitleItem(MNewServerIdentityTitle);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,29, 0, MNewServerIdentitySite);
	_di.AddAtLine(DI_TEXT, 30,58, 0, site.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,58, 0, MNewServerIdentityText);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 6,58, 0, identity.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,59, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_allow_once = _di.AddAtLine(DI_BUTTON, 6,15, DIF_CENTERGROUP, MNewServerIdentityAllowOnce);
	if (may_remember) {
		_i_allow_always = _di.AddAtLine(DI_BUTTON, 16,25, DIF_CENTERGROUP, MNewServerIdentityAllowAlways);
	}
	_di.AddAtLine(DI_BUTTON, 26,35, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl();
	SetDefaultDialogControl();
}

ConfirmNewServerIdentity::Result ConfirmNewServerIdentity::Ask()
{
	const int r = Show("ConfirmNewServerIdentity", 6, 2, FDLG_WARNING);

	if (r != -1) {
		if (r == _i_allow_once)
			return R_ALLOW_ONCE;

		if (r == _i_allow_always)
			return R_ALLOW_ALWAYS;
	}

	return R_DENY;
}

