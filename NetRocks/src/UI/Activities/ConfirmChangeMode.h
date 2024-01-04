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

	int _i_proceed = -1, _i_cancel = -1;

	void StateFromModes(int ctl, bool recurse, mode_t mode_all, mode_t mode_any, mode_t bit);
	void StateToModes(int ctl, mode_t &mode_set, mode_t &mode_clear, mode_t bit);
public:
	ConfirmChangeMode(int selected_count, const std::string &display_path, const std::string &link_target, bool may_recurse, mode_t mode_all, mode_t mode_any);

	bool Ask(bool &recurse, mode_t &mode_set, mode_t &mode_clear);
};
