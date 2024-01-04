#include <utils.h>
#include "ConfirmChangeMode.h"
#include "../../Globals.h"


/*                                                         62
345                      28         39                   60  64
 ================ Confirm change mode =======================
| Edit mode of files and directories in:                     |
| [TEXTBOX                                                 ] |
| That is symlink pointing to:                               |
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

void ConfirmChangeMode::StateFromModes(int ctl, bool recurse, mode_t mode_all, mode_t mode_any, mode_t bit)
{
	int state;
	if (recurse) {
		state = BSTATE_3STATE;

	} else if (bit & mode_all) {
		state = BSTATE_CHECKED;

	} else if (bit & mode_any) {
		state = BSTATE_3STATE;

	} else {
		state = BSTATE_UNCHECKED;
	}

	Set3StateDialogControl(ctl, state);
}

void ConfirmChangeMode::StateToModes(int ctl, mode_t &mode_set, mode_t &mode_clear, mode_t bit)
{
	switch (Get3StateDialogControl(ctl)) {
		case BSTATE_CHECKED: mode_set|= bit; break;
		case BSTATE_UNCHECKED: mode_clear|= bit; break;
	}
}

ConfirmChangeMode::ConfirmChangeMode(int selected_count, const std::string &display_path, const std::string &link_target, bool may_recurse, mode_t mode_all, mode_t mode_any)
{
	_di.SetBoxTitleItem(MConfirmChangeModeTitle);

	_di.SetLine(2);
	std::string text = G.GetMsgMB( (selected_count > 1) ? MConfirmChangeModeTextMany : MConfirmChangeModeTextOne);
	if (selected_count > 1) {
		text = StrPrintf(text.c_str(), selected_count);
	}
	_di.AddAtLine(DI_TEXT, 5,62, 0, text.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_EDIT, 5,62, DIF_READONLY, display_path.c_str());

	if (!link_target.empty()) {
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, 0, MThatIsSymlink);
		_di.NextLine();
		_di.AddAtLine(DI_EDIT, 5,62, DIF_READONLY, link_target.c_str());
	}

	if (may_recurse) {
		_di.NextLine();
		_i_recurse_subdirs = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MRecurseSubdirs);
		SetCheckedDialogControl(_i_recurse_subdirs);
	}

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,24, 0, MModeUser);
	_di.AddAtLine(DI_TEXT, 25,44, 0, MModeGroup);
	_di.AddAtLine(DI_TEXT, 45,62, 0, MModeOther);

	_di.NextLine();
	_i_mode_user_read = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeRead);
	StateFromModes(_i_mode_user_read, may_recurse, mode_all, mode_any, S_IRUSR);

	_i_mode_group_read = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeRead);
	StateFromModes(_i_mode_group_read, may_recurse, mode_all, mode_any, S_IRGRP);

	_i_mode_other_read = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeRead);
	StateFromModes(_i_mode_other_read, may_recurse, mode_all, mode_any, S_IROTH);

	_di.NextLine();
	_i_mode_user_write = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeWrite);
	StateFromModes(_i_mode_user_write, may_recurse, mode_all, mode_any, S_IWUSR);

	_i_mode_group_write = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeWrite);
	StateFromModes(_i_mode_group_write, may_recurse, mode_all, mode_any, S_IWGRP);

	_i_mode_other_write = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeWrite);
	StateFromModes(_i_mode_other_write, may_recurse, mode_all, mode_any, S_IWOTH);

	_di.NextLine();
	_i_mode_user_execute = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeExecute);
	StateFromModes(_i_mode_user_execute, may_recurse, mode_all, mode_any, S_IXUSR);

	_i_mode_group_execute = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeExecute);
	StateFromModes(_i_mode_group_execute, may_recurse, mode_all, mode_any, S_IXGRP);

	_i_mode_other_execute = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeExecute);
	StateFromModes(_i_mode_other_execute, may_recurse, mode_all, mode_any, S_IXOTH);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_mode_set_uid = _di.AddAtLine(DI_CHECKBOX, 5,24, DIF_3STATE, MModeSetUID);
	StateFromModes(_i_mode_set_uid, may_recurse, mode_all, mode_any, S_ISUID);

	_i_mode_set_gid = _di.AddAtLine(DI_CHECKBOX, 25,44, DIF_3STATE, MModeSetGID);
	StateFromModes(_i_mode_set_gid, may_recurse, mode_all, mode_any, S_ISGID);

	_i_mode_sticky = _di.AddAtLine(DI_CHECKBOX, 45,62, DIF_3STATE, MModeSticky);
	StateFromModes(_i_mode_sticky, may_recurse, mode_all, mode_any, S_ISVTX);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_proceed = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MProceedChangeMode);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_proceed);
	SetDefaultDialogControl(_i_proceed);
}

bool ConfirmChangeMode::Ask(bool &recurse, mode_t &mode_set, mode_t &mode_clear)
{
	if (Show(L"ConfirmChangeMode", 6, 2, 0) != _i_proceed) {
		return false;
	}

	if (_i_recurse_subdirs != -1) {
		recurse = IsCheckedDialogControl(_i_recurse_subdirs);
	}

	mode_set = 0;
	mode_clear = 0;

	StateToModes(_i_mode_user_read, mode_set, mode_clear, S_IRUSR);
	StateToModes(_i_mode_group_read, mode_set, mode_clear, S_IRGRP);
	StateToModes(_i_mode_other_read, mode_set, mode_clear, S_IROTH);

	StateToModes(_i_mode_user_write, mode_set, mode_clear, S_IWUSR);
	StateToModes(_i_mode_group_write, mode_set, mode_clear, S_IWGRP);
	StateToModes(_i_mode_other_write, mode_set, mode_clear, S_IWOTH);

	StateToModes(_i_mode_user_execute, mode_set, mode_clear, S_IXUSR);
	StateToModes(_i_mode_group_execute, mode_set, mode_clear, S_IXGRP);
	StateToModes(_i_mode_other_execute, mode_set, mode_clear, S_IXOTH);

	StateToModes(_i_mode_set_uid, mode_set, mode_clear, S_ISUID);
	StateToModes(_i_mode_set_gid, mode_set, mode_clear, S_ISGID);
	StateToModes(_i_mode_sticky, mode_set, mode_clear, S_ISVTX);

	return true;
}
