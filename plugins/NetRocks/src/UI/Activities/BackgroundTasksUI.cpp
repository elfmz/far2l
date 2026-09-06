#include <utils.h>
#include "BackgroundTasksUI.h"
#include "../../Globals.h"
#include "../../BackgroundTasks.h"


void BackgroundTasksList()
{
	if (CountOfAllBackgroundTasks() == 0) {
		const wchar_t *msg[] = { G.GetMsgWide(MTitle), G.GetMsgWide(MNoBackgroundTasks), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, 0, nullptr, msg, ARRAYSIZE(msg), 1);
	}

	std::wstring item_text;
	std::set<std::wstring> str_pool;
	std::wstring prefix_active = G.GetMsgWide(MBackgroundTasksMenuActive);
	std::wstring prefix_paused = G.GetMsgWide(MBackgroundTasksMenuPaused);
	std::wstring prefix_complete = G.GetMsgWide(MBackgroundTasksMenuComplete);
	std::wstring prefix_aborted = G.GetMsgWide(MBackgroundTasksMenuAborted);
	size_t prefix_len = std::max(prefix_active.size(), std::max(prefix_paused.size(), std::max(prefix_complete.size(), prefix_aborted.size())));
	prefix_active.resize(prefix_len, ' ');   prefix_active+= L" │ ";
	prefix_paused.resize(prefix_len, ' ');   prefix_paused+= L" │ ";
	prefix_complete.resize(prefix_len, ' '); prefix_complete+= L" │ ";
	prefix_aborted.resize(prefix_len, ' ');  prefix_aborted+= L" │ ";
	for (unsigned long selected_id = 0;;) {
		BackgroundTasksInfo info;
		GetBackgroundTasksInfo(info);
		if (info.empty())
			break;

		std::vector<FarMenuItem> menu_items(info.size());
		for (size_t i = 0; i < info.size(); ++i) {
			switch (info[i].status) {
				case BTS_ACTIVE:   item_text = prefix_active; break;
				case BTS_PAUSED:   item_text = prefix_paused; break;
				case BTS_COMPLETE: item_text = prefix_complete; break;
				case BTS_ABORTED:  item_text = prefix_aborted; break;

				default:
					ABORT_MSG("unexpected status %u", info[i].status);
			}
			item_text+= StrMB2Wide(info[i].information);
			menu_items[i].Text = str_pool.emplace(item_text).first->c_str();
			if (selected_id == 0 && (info[i].status == BTS_ACTIVE || info[i].status == BTS_PAUSED)) {
				selected_id = info[i].id;
			}
			if (selected_id == info[i].id) {
				menu_items[i].Selected = 1;
			}
		}

		int choice = G.info.Menu(G.info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, G.GetMsgWide(MBackgroundTasksTitle),
				NULL, L"BackgroundTasksMenu", NULL, NULL, &menu_items[0], menu_items.size());
		if (choice >= 0 && (size_t)choice < menu_items.size()) {
			ShowBackgroundTask(info[choice].id);
			selected_id = info[choice].id;
		} else {
			break;
		}
	}
}

/*
345                                            50
 ============== Confirm exit FAR ===========================
| Exiting FAR will disrupt %u pending background tasks.     |
| Are you sure you want to exit far?                        |
|-----------------------------------------------------------|
|   [Background Tasks ] [    Ok   ]      [    Cancel    ]   |
 ===========================================================
    6                     29       38
*/

ConfirmExitFAR::ConfirmExitFAR(size_t background_ops_count)
{
	_di.SetBoxTitleItem(MConfirmExitFARTitle);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,62, 0, StrPrintf(G.GetMsgMB(MConfirmExitFARText), background_ops_count).c_str());
	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,62, 0, MConfirmExitFARQuestion);

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	_i_background_tasks = _di.AddAtLine(DI_BUTTON, 6,15, DIF_CENTERGROUP, MConfirmExitFARBackgroundTasks);
	_i_ok = _di.AddAtLine(DI_BUTTON, 16,25, DIF_CENTERGROUP, MOK);
	_di.AddAtLine(DI_BUTTON, 26,35, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl();
	SetDefaultDialogControl();
}

LONG_PTR ConfirmExitFAR::DlgProc(int msg, int param1, LONG_PTR param2)
{
	if (msg == DN_BTNCLICK && param1 == _i_background_tasks) {
		BackgroundTasksList();
		return TRUE;
	}

	return BaseDialog::DlgProc(msg, param1, param2);
}

bool ConfirmExitFAR::Ask()
{
	return Show("ConfirmExitFAR", 6, 2, FDLG_WARNING) == _i_ok;
}

