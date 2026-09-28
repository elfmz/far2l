#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class ConfirmChangeMode : protected BaseDialog
{
	int _i_recurse_subdirs = -1;
	int _i_mode_user_read = -1;
	int _i_mode_group_read = -1;
	int _i_mode_other_read = -1;
	int _i_mode_user_write = -1;
	int _i_mode_group_write = -1;
	int _i_mode_other_write = -1;
	int _i_mode_user_execute = -1;
	int _i_mode_group_execute = -1;
	int _i_mode_other_execute = -1;
	int _i_mode_set_uid = -1;
	int _i_mode_set_gid = -1;
	int _i_mode_sticky = -1;

	int _i_octal = -1;
	int _i_original = -1;

	int _i_proceed = -1, _i_cancel = -1;

	int _original_mode_user_read = BSTATE_3STATE;
	int _original_mode_group_read = BSTATE_3STATE;
	int _original_mode_other_read = BSTATE_3STATE;
	int _original_mode_user_write = BSTATE_3STATE;
	int _original_mode_group_write = BSTATE_3STATE;
	int _original_mode_other_write = BSTATE_3STATE;
	int _original_mode_user_execute = BSTATE_3STATE;
	int _original_mode_group_execute = BSTATE_3STATE;
	int _original_mode_other_execute = BSTATE_3STATE;
	int _original_mode_set_uid = BSTATE_3STATE;
	int _original_mode_set_gid = BSTATE_3STATE;
	int _original_mode_sticky = BSTATE_3STATE;

	bool _b_check_or_edit_process = false;

	void StateFromModes(int ctl, bool recurse, mode_t mode_all, mode_t mode_any, mode_t bit);
	void StateToModes(int ctl, mode_t &mode_set, mode_t &mode_clear, mode_t bit);

	char GetBitCharFromModeCheckBoxes(int _i1, int _i2, int _i3);
	void CalcBitsCharFromModeCheckBoxes();
	void GetModeCheckBoxesFromChar(char c, int _i1, int _i2, int _i3);

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);
public:
	ConfirmChangeMode(int selected_count, const std::string &display_path, const std::string &link_target,
		const std::wstring &owner, const std::wstring &group,
		FILETIME ftCreationTime, FILETIME ftLastAccessTime, FILETIME ftLastWriteTime,
		bool may_recurse, mode_t mode_all, mode_t mode_any);

	bool Ask(bool &recurse, mode_t &mode_set, mode_t &mode_clear);
};
