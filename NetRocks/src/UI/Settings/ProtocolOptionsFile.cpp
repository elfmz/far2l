#include <algorithm>
#include <utils.h>
#include <StringConfig.h>
#include "../DialogUtils.h"
#include "../../Globals.h"
#include "ProtocolOptionsFile.h"


/*                                                         62
345                      28         39                   60  64
 ================ File Protocol options =====================
|------------------------------------------------------------|
|             [  OK    ]        [        Cancel       ]      |
 ============================================================
    6                     29       38                      60
*/

ProtocolOptionsFile::ProtocolOptionsFile()
{
	_di.Add(DI_DOUBLEBOX, 3,1,64,64 0, MSFTPOptionsTitle);
	_di.SetLine(2);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,61, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();

	_i_ok = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MOK);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_ok);
	SetDefaultDialogControl(_i_ok);
}


void ProtocolOptionsFile::Ask(std::string &options)
{
	StringConfig sc(options);
	if (Show(L"ProtocolOptionsFile", 6, 2) == _i_ok) {
		options = sc.Serialize();
	}
}
