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
| Owner:      [TEXTBOX                                     ] |
| Group:      [TEXTBOX                                     ] |
|------------------------------------------------------------|
| User:     [x] R   [x] W   [x] X   [ ] SUID    Octal: SUGO  |
| Group:    [x] R   [ ] W   [x] X   [ ] SGID          [TEXT] |
| Other:    [x] R   [ ] W   [ ] X   [ ] Sticky  [ Original ] |
|------------------------------------------------------------|
| [ ] Recurse into subdirectories                            |
|------------------------------------------------------------|
|   [  Proceed  ]                  [        Cancel       ]   |
 ============================================================
    6                     29       38                      60
*/

void ConfirmChangeMode::StateFromModes(int ctl, bool recurse, mode_t mode_all, mode_t mode_any, mode_t bit)
{
	int state;
	/*if (recurse) {
		state = BSTATE_3STATE;

	} else*/ if (bit & mode_all) {
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

ConfirmChangeMode::ConfirmChangeMode(int selected_count, const std::string &display_path, const std::string &link_target,
	const std::wstring &owner, const std::wstring &group,
	FILETIME ftCreationTime, FILETIME ftLastAccessTime, FILETIME ftLastWriteTime,
	bool may_recurse, mode_t mode_all, mode_t mode_any)
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

	if (ftCreationTime.dwHighDateTime != 0 || ftCreationTime.dwLowDateTime != 0
			|| ftLastAccessTime.dwHighDateTime != 0 || ftLastAccessTime.dwLowDateTime != 0
			|| ftLastWriteTime.dwHighDateTime != 0 || ftLastWriteTime.dwLowDateTime != 0) {
		int i;
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR, MTimeTitle);
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,35, 0, MTimeAccess);
		i = _di.AddAtLine(DI_EDIT, 37,62, DIF_READONLY);
		DateTimeToDialogControl(i, &ftLastAccessTime);
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,35, 0, MTimeModification);
		i = _di.AddAtLine(DI_EDIT, 37,62, DIF_READONLY);
		DateTimeToDialogControl(i, &ftLastWriteTime);
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,35, 0, MTimeChange);
		i = _di.AddAtLine(DI_EDIT, 37,62, DIF_READONLY);
		DateTimeToDialogControl(i, &ftCreationTime);
	}

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR, MOwnerTitle);
	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,16, 0, MOwner);
	_di.AddAtLine(DI_EDIT, 17,62, DIF_READONLY, owner.c_str());
	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,16, 0, MGroup);
	_di.AddAtLine(DI_EDIT, 17,62, DIF_READONLY, group.c_str());

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR, MPermissionsTtitle);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,18, 0, MModeUser);
	_i_mode_user_read = _di.AddAtLine(DI_CHECKBOX, 19,23, DIF_3STATE, MModeReadHot);
	StateFromModes(_i_mode_user_read, may_recurse, mode_all, mode_any, S_IRUSR);
	_i_mode_user_write = _di.AddAtLine(DI_CHECKBOX, 26,30, DIF_3STATE, MModeWriteHot);
	StateFromModes(_i_mode_user_write, may_recurse, mode_all, mode_any, S_IWUSR);
	_i_mode_user_execute = _di.AddAtLine(DI_CHECKBOX, 33,37, DIF_3STATE, MModeExecuteHot);
	StateFromModes(_i_mode_user_execute, may_recurse, mode_all, mode_any, S_IXUSR);
	_i_mode_set_uid = _di.AddAtLine(DI_CHECKBOX, 40,52, DIF_3STATE, MModeSetUID);
	StateFromModes(_i_mode_set_uid, may_recurse, mode_all, mode_any, S_ISUID);
	_di.AddAtLine(DI_TEXT, 52,62, 0, "Octal: SUGO");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,18, 0, MModeGroup);
	_i_mode_group_read = _di.AddAtLine(DI_CHECKBOX, 19,23, DIF_3STATE, MModeRead);
	StateFromModes(_i_mode_group_read, may_recurse, mode_all, mode_any, S_IRGRP);
	_i_mode_group_write = _di.AddAtLine(DI_CHECKBOX, 26,30, DIF_3STATE, MModeWrite);
	StateFromModes(_i_mode_group_write, may_recurse, mode_all, mode_any, S_IWGRP);
	_i_mode_group_execute = _di.AddAtLine(DI_CHECKBOX, 33,37, DIF_3STATE, MModeExecute);
	StateFromModes(_i_mode_group_execute, may_recurse, mode_all, mode_any, S_IXGRP);
	_i_mode_set_gid = _di.AddAtLine(DI_CHECKBOX, 40,52, DIF_3STATE, MModeSetGID);
	StateFromModes(_i_mode_set_gid, may_recurse, mode_all, mode_any, S_ISGID);
	_i_octal = _di.AddAtLine(DI_FIXEDIT, 59,62, DIF_MASKEDIT, "", "####");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,18, 0, MModeOther);
	_i_mode_other_read = _di.AddAtLine(DI_CHECKBOX, 19,23, DIF_3STATE, MModeRead);
	StateFromModes(_i_mode_other_read, may_recurse, mode_all, mode_any, S_IROTH);
	_i_mode_other_write = _di.AddAtLine(DI_CHECKBOX, 26,30, DIF_3STATE, MModeWrite);
	StateFromModes(_i_mode_other_write, may_recurse, mode_all, mode_any, S_IWOTH);
	_i_mode_other_execute = _di.AddAtLine(DI_CHECKBOX, 33,37, DIF_3STATE, MModeExecute);
	StateFromModes(_i_mode_other_execute, may_recurse, mode_all, mode_any, S_IXOTH);
	_i_mode_sticky = _di.AddAtLine(DI_CHECKBOX, 40,52, DIF_3STATE, MModeSticky);
	StateFromModes(_i_mode_sticky, may_recurse, mode_all, mode_any, S_ISVTX);
	_i_original = _di.AddAtLine(DI_BUTTON, 52,62, DIF_BTNNOCLOSE, MModeOriginal);

	_original_mode_user_read     = Get3StateDialogControl(_i_mode_user_read);
	_original_mode_user_write    = Get3StateDialogControl(_i_mode_user_write);
	_original_mode_user_execute  = Get3StateDialogControl(_i_mode_user_execute);
	_original_mode_group_read    = Get3StateDialogControl(_i_mode_group_read);
	_original_mode_group_write   = Get3StateDialogControl(_i_mode_group_write);
	_original_mode_group_execute = Get3StateDialogControl(_i_mode_group_execute);
	_original_mode_other_read    = Get3StateDialogControl(_i_mode_other_read);
	_original_mode_other_write   = Get3StateDialogControl(_i_mode_other_write);
	_original_mode_other_execute = Get3StateDialogControl(_i_mode_other_execute);
	_original_mode_set_uid       = Get3StateDialogControl(_i_mode_set_uid);
	_original_mode_set_gid       = Get3StateDialogControl(_i_mode_set_gid);
	_original_mode_sticky        = Get3StateDialogControl(_i_mode_sticky);

	if (may_recurse) {
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,62, DIF_DISABLE, "(X will be automatically added to dirs having original R)");
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);
		_di.NextLine();
		_i_recurse_subdirs = _di.AddAtLine(DI_CHECKBOX, 5,62, 0, MRecurseSubdirs);
		//SetCheckedDialogControl(_i_recurse_subdirs);
	}

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_proceed = _di.AddAtLine(DI_BUTTON, 7,29, DIF_CENTERGROUP, MProceedChangeMode);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 38,58, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_proceed);
	SetDefaultDialogControl(_i_proceed);

	CalcBitsCharFromModeCheckBoxes();
}

char ConfirmChangeMode::GetBitCharFromModeCheckBoxes(int _i1, int _i2, int _i3)
{
	int i1, i2, i3;

	i1 = Get3StateDialogControl(_i1);
	i2 = Get3StateDialogControl(_i2);
	i3 = Get3StateDialogControl(_i3);
	if (i1 == BSTATE_3STATE || i2 == BSTATE_3STATE || i3 == BSTATE_3STATE)
		return '-';
	else {
		int i = (i1 == BSTATE_CHECKED ? 1 : 0) + (i2 == BSTATE_CHECKED ? 2 : 0) + (i3 == BSTATE_CHECKED ? 4 : 0);
		return '0' + i;
	}
}

void ConfirmChangeMode::CalcBitsCharFromModeCheckBoxes()
{
	char str_octal[5] = {0};
	str_octal[0] = GetBitCharFromModeCheckBoxes(_i_mode_sticky, _i_mode_set_gid, _i_mode_set_uid);
	str_octal[1] = GetBitCharFromModeCheckBoxes(_i_mode_user_execute,  _i_mode_user_write,  _i_mode_user_read);
	str_octal[2] = GetBitCharFromModeCheckBoxes(_i_mode_group_execute, _i_mode_group_write, _i_mode_group_read);
	str_octal[3] = GetBitCharFromModeCheckBoxes(_i_mode_other_execute, _i_mode_other_write, _i_mode_other_read);
	TextToDialogControl(_i_octal, str_octal);
}

void ConfirmChangeMode::GetModeCheckBoxesFromChar(char c, int _i1, int _i2, int _i3)
{
	switch(c) {
		case '0':
			Set3StateDialogControl(_i1, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i2, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i3, BSTATE_UNCHECKED);
			break;
		case '1':
			Set3StateDialogControl(_i1, BSTATE_CHECKED);
			Set3StateDialogControl(_i2, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i3, BSTATE_UNCHECKED);
			break;
		case '2':
			Set3StateDialogControl(_i1, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i2, BSTATE_CHECKED);
			Set3StateDialogControl(_i3, BSTATE_UNCHECKED);
			break;
		case '3':
			Set3StateDialogControl(_i1, BSTATE_CHECKED);
			Set3StateDialogControl(_i2, BSTATE_CHECKED);
			Set3StateDialogControl(_i3, BSTATE_UNCHECKED);
			break;
		case '4':
			Set3StateDialogControl(_i1, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i2, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i3, BSTATE_CHECKED);
			break;
		case '5':
			Set3StateDialogControl(_i1, BSTATE_CHECKED);
			Set3StateDialogControl(_i2, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i3, BSTATE_CHECKED);
			break;
		case '6':
			Set3StateDialogControl(_i1, BSTATE_UNCHECKED);
			Set3StateDialogControl(_i2, BSTATE_CHECKED);
			Set3StateDialogControl(_i3, BSTATE_CHECKED);
			break;
		case '7':
			Set3StateDialogControl(_i1, BSTATE_CHECKED);
			Set3StateDialogControl(_i2, BSTATE_CHECKED);
			Set3StateDialogControl(_i3, BSTATE_CHECKED);
			break;
		default:
			Set3StateDialogControl(_i1, BSTATE_3STATE);
			Set3StateDialogControl(_i2, BSTATE_3STATE);
			Set3StateDialogControl(_i3, BSTATE_3STATE);
			break;
	}
}

LONG_PTR ConfirmChangeMode::DlgProc(int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_EDITCHANGE) {
		if (param1 == _i_octal && !_b_check_or_edit_process) {
			_b_check_or_edit_process = true;
			std::string str_octal;
			TextFromDialogControl(_i_octal, str_octal);
			int length = str_octal.length();
			GetModeCheckBoxesFromChar(length<1 ? ' ' : str_octal[0], _i_mode_sticky, _i_mode_set_gid, _i_mode_set_uid);
			GetModeCheckBoxesFromChar(length<2 ? ' ' : str_octal[1], _i_mode_user_execute,  _i_mode_user_write,  _i_mode_user_read);
			GetModeCheckBoxesFromChar(length<3 ? ' ' : str_octal[2], _i_mode_group_execute, _i_mode_group_write, _i_mode_group_read);
			GetModeCheckBoxesFromChar(length<4 ? ' ' : str_octal[3], _i_mode_other_execute, _i_mode_other_write, _i_mode_other_read);
			_b_check_or_edit_process = false;
			return TRUE;
		}
	}
	else if (msg == DN_BTNCLICK) {
		if ( param1 == _i_original ) {
			_b_check_or_edit_process = true;
			Set3StateDialogControl(_i_mode_sticky,        _original_mode_sticky);
			Set3StateDialogControl(_i_mode_set_gid,       _original_mode_set_gid);
			Set3StateDialogControl(_i_mode_set_uid,       _original_mode_set_uid);
			Set3StateDialogControl(_i_mode_user_execute,  _original_mode_user_execute);
			Set3StateDialogControl(_i_mode_user_write,    _original_mode_user_write);
			Set3StateDialogControl(_i_mode_user_read,     _original_mode_user_read);
			Set3StateDialogControl(_i_mode_group_execute, _original_mode_group_execute);
			Set3StateDialogControl(_i_mode_group_write,   _original_mode_group_write);
			Set3StateDialogControl(_i_mode_group_read,    _original_mode_group_read);
			Set3StateDialogControl(_i_mode_other_execute, _original_mode_other_execute);
			Set3StateDialogControl(_i_mode_other_write,   _original_mode_other_write);
			Set3StateDialogControl(_i_mode_other_read,    _original_mode_other_read);
			CalcBitsCharFromModeCheckBoxes();
			_b_check_or_edit_process = false;
			return TRUE;
		}
		else if ( param1 == _i_recurse_subdirs ) {
			if (IsCheckedDialogControl(_i_recurse_subdirs)) {
				_b_check_or_edit_process = true;
				Set3StateDialogControl(_i_mode_sticky, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_set_gid, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_set_uid, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_user_execute, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_user_write, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_user_read, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_group_execute, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_group_write, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_group_read, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_other_execute, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_other_write, BSTATE_3STATE);
				Set3StateDialogControl(_i_mode_other_read, BSTATE_3STATE);
				CalcBitsCharFromModeCheckBoxes();
				_b_check_or_edit_process = false;
			}
			return TRUE;
		}
		else if ( !_b_check_or_edit_process &&
					(param1 == _i_mode_user_read || param1 == _i_mode_user_write || param1 == _i_mode_user_execute
					|| param1 == _i_mode_group_read || param1 == _i_mode_group_write || param1 == _i_mode_group_execute
					|| param1 == _i_mode_other_read || param1 == _i_mode_other_write || param1 == _i_mode_other_execute
					|| param1 == _i_mode_set_uid || param1 == _i_mode_set_gid || param1 == _i_mode_sticky )) {
			_b_check_or_edit_process = true;
			CalcBitsCharFromModeCheckBoxes();
			_b_check_or_edit_process = false;
			return TRUE;
		}
	}

	return BaseDialog::DlgProc(msg, param1, param2);
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
