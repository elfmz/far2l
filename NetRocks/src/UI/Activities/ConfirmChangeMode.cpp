#include <utils.h>
#include "ConfirmChangeMode.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ Confirm change mode =======================
| Edit mode of files and directories in:                     |
| [TEXTBOX                                                 ] |
| [x] Recurse into subdirectories                            |
|------------------------------------------------------------|
| User's access       Group's access      Other's access     |
| [x] Read            [x] Read            [x] Read           |
| [x] Write           [x] Write           [ ] Write          |
| [ ] Execute         [ ] Execute         [ ] Execute        |
|------------------------------------------------------------|
| [ ] Set UID         [ ] Set GID         [ ] Sticky         |
|------------------------------------------------------------|
|   [  Proceed  ]                  [        Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

ConfirmChangeMode::ConfirmChangeMode(const std::string &site_dir, bool may_recurse, mode_t mode_all, mode_t mode_any)
{
	_di.SetBoxTitleItem(MConfirmChangeModeTitle);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,62, 0, MConfirmChangeModeText);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,62, 0, site_dir.c_str());

	_di.NextLine();
	_i_recurse_subdirs = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MRecurseSubdirs);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,24, DIF_3STATE, MModeUser);
	_di.AddAtLine(DI_TEXT, 25,44, DIF_3STATE, MModeGroup);
	_di.AddAtLine(DI_TEXT, 45,62, DIF_3STATE, MModeOther);

	_di.NextLine();
	_i_mode_user_read = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeRead);
	_i_mode_group_read = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeRead);
	_i_mode_other_read = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeRead);

	_di.NextLine();
	_i_mode_user_write = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeWrite);
	_i_mode_group_write = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeWrite);
	_i_mode_other_write = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeWrite);

	_di.NextLine();
	_i_mode_user_execute = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeExecute);
	_i_mode_group_execute = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeExecute);
	_i_mode_other_execute = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeExecute);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_mode_set_uid = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeSetUID);
	_i_mode_set_gid = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeSetGID);
	_i_mode_sticky = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeSticky);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_proceed = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MProceedRemoval);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_proceed);
	SetDefaultDialogControl(_i_proceed);
}


bool ConfirmChangeMode::Ask(bool &recurse, mode_t &mode_set, mode_t &mode_clear)
{
	if (Show(L"ConfirmChangeMode", 6, 2, 0) != _i_proceed) {
		return false;
	}

	recurse = false;
	mode_set = 0;
	mode_clear = 0;
	return true;
}
