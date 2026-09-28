#include "OpChangeMode.h"
#include <utils.h>
#include "../UI/Activities/ConfirmChangeMode.h"
#include "../UI/Activities/SimpleOperationProgress.h"
#include "OpGetLinkTarget.h"
#include "../Globals.h"

OpChangeMode::OpChangeMode(std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		struct PluginPanelItem *items, int items_count)
	:
	OpBase(0, base_host, base_dir),
	_recurse(false),
	_mode_set(0),
	_mode_clear(0)
{
	bool has_dirs = false;
	mode_t mode_all = 07777, mode_any = 0;
	for (int i = 0; i < items_count; ++i) {
		if (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			has_dirs = true;
			/*mode_all = 0;
			mode_any = 07777;
			break;*/
		}
		mode_all&= items[i].FindData.dwUnixMode;
		mode_any|= (items[i].FindData.dwUnixMode & 07777);
	}
	std::wstring owner = items[0].Owner ? items[0].Owner : G.GetMsgWide(MOwnerUnknown);
	std::wstring group = items[0].Group ? items[0].Group : G.GetMsgWide(MGroupUnknown);
	for (int i = 1; i < items_count; ++i) {
		if ((items[i].Owner && owner != items[i].Owner) || (!items[i].Owner && owner != G.GetMsgWide(MOwnerUnknown))) {
			owner = G.GetMsgWide(MOwnerMultiple);
			break;
		}
	}
	for (int i = 1; i < items_count; ++i) {
		if ((items[i].Group && group != items[i].Group) || (!items[i].Group && group != G.GetMsgWide(MGroupUnknown))) {
			group = G.GetMsgWide(MOwnerMultiple);
			break;
		}
	}
	std::string display_path = base_dir, link_target;
	FILETIME ftCreationTime = {0};
	FILETIME ftLastAccessTime = {0};
	FILETIME ftLastWriteTime = {0};
	if (items_count == 1) {
		if (!display_path.empty() && display_path.back() != '/') {
			display_path+= '/';
		}
		Wide2MB(items->FindData.lpwszFileName, display_path, true);
		if (items->FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			if (!OpGetLinkTarget(0, base_host, base_dir, items).Do(link_target)) {
				link_target.clear();
			}
		}
		ftCreationTime = items->FindData.ftCreationTime;
		ftLastAccessTime = items->FindData.ftLastAccessTime;
		ftLastWriteTime = items->FindData.ftLastWriteTime;
	}

	if (!ConfirmChangeMode(items_count, display_path, link_target, owner, group,
			ftCreationTime, ftLastAccessTime, ftLastWriteTime,
			has_dirs, mode_all, mode_any).Ask(_recurse, _mode_set, _mode_clear)) {
		throw AbortError();
	}

	_enumer = std::make_shared<Enumer>(_entries, _base_host, _base_dir, items, items_count, true, _state, _wea_state);
}

void OpChangeMode::Do()
{
	if (!StartThread()) {
		;

	} else if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress p(SimpleOperationProgress::K_CHANGEMODE, _base_dir, _state);
		p.Show();
		WaitThread();
	}
}


void OpChangeMode::Process()
{
	_enumer->Scan(_recurse);
	for (const auto &entry : _entries) {
		ChangeModeOfPath(entry.first, entry.second.mode);
		ProgressStateUpdate psu(_state);
//		_state.path = path;
		_state.stats.count_complete++;
	}
}

void OpChangeMode::ChangeModeOfPath(const std::string &path, mode_t prev_mode)
{
	WhatOnErrorWrap<WEK_CHMODE>(_wea_state, _state, _base_host.get(), path,
		[&] () mutable
		{
			mode_t new_mode = prev_mode;
			new_mode&= (~_mode_clear);
			new_mode|= (_mode_set);
			if (S_ISDIR(prev_mode)) {
				// hacky workaround:
				// for directories  'x' means cd-ability, so treat it allowed if if 'r' allowed
				if (prev_mode & S_IRUSR) {
					new_mode|= (prev_mode & S_IXUSR);
				}
				if (prev_mode & S_IRGRP) {
					new_mode|= (prev_mode & S_IXGRP);
				}
				if (prev_mode & S_IROTH) {
					new_mode|= (prev_mode & S_IXOTH);
				}
			}
			if (new_mode != prev_mode) {
				_base_host->SetMode(path.c_str(), new_mode & 07777);
			}

//			fprintf(stderr, "%o -> %o '%s'\n", prev_mode, new_mode, path.c_str());
		}
	);
}

