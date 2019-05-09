#include <utils.h>
#include "ConfirmNewServerIdentity.h"
#include "../Globals.h"



/*                                               
345                                            50
 ======== Confirm new server identify =========
| Site:          [                           ] |
| New identity:                                |
|  [                                         ] |
|----------------------------------------------|
|   [       Ok      ]      [    Cancel    ]    |
 ==============================================
    6                     29       38            
*/

ConfirmNewServerIdentity::ConfirmNewServerIdentity(const std::string &site, const std::string &identity)
{
	_di.Add(DI_DOUBLEBOX, 3,1,50,7, 0, MNewServerIdentityTitle);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,19, 0, MNewServerIdentitySite);
	_di.AddAtLine(DI_TEXT, 20,48, 0, site.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,48, 0, MNewServerIdentityText);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 6,48, 0, identity.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,49, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_ok = _di.AddAtLine(DI_BUTTON, 6,23, DIF_CENTERGROUP, MOK);
	_di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MCancel, nullptr, FDIS_DEFAULT);
}

bool ConfirmNewServerIdentity::Ask()
{
	return (Show("ConfirmNewServerIdentity", 6, 2, FDLG_WARNING) == _i_ok);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
