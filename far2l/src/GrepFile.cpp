#include "headers.hpp"
#include "mix.hpp"
#include "pathmix.hpp"
#include "GrepFile.hpp"
#include "dialog.hpp"
#include "DialogBuilder.hpp"

FileHolderPtr GrepFile(FileHolderPtr src)
{
	DialogBuilder dlg_builder(Msg::ConfigGrepFilterTitle, L"GrepFilter");
	static int s_context = 0;
	static FARString s_pattern, s_exclude_pattern;
	static int s_case_sensitive = 0;
	static int s_whole_words = 0;

	DialogItemEx *pattern_edit = dlg_builder.AddEditField(&s_pattern, 30, L"GrepPattern", DIF_FOCUS | DIF_HISTORY);
	dlg_builder.AddTextBefore(pattern_edit, Msg::ConfigGrepFilterPattern);

	DialogItemEx *exclude_pattern_edit = dlg_builder.AddEditField(&s_exclude_pattern, 30, L"GrepExclPattern", DIF_HISTORY);
	dlg_builder.AddTextBefore(exclude_pattern_edit, Msg::ConfigGrepFilterExclPattern);

	DialogItemEx *context_edit = dlg_builder.AddIntEditField(&s_context, 3);
	dlg_builder.AddTextBefore(context_edit, Msg::ConfigGrepFilterContext);

	dlg_builder.AddCheckbox(Msg::ConfigGrepFilterCaseSensitive, &s_case_sensitive);
	dlg_builder.AddCheckbox(Msg::ConfigGrepFilterWholeWords, &s_whole_words);


	dlg_builder.AddOKCancel();
	if (!dlg_builder.ShowDialog()) {
		return FileHolderPtr();
	}

	if (s_pattern.IsEmpty() && s_exclude_pattern.IsEmpty()) {
		fprintf(stderr, "%s: empty patterns\n", __FUNCTION__);
		return FileHolderPtr();
	}

	FARString new_file_path_name;
	if (!FarMkTempEx(new_file_path_name, L"grep")) {
		fprintf(stderr, "%s: mktemp failed\n", __FUNCTION__);
		return FileHolderPtr();
	}
	apiCreateDirectory(new_file_path_name, nullptr);

	new_file_path_name+= LGOOD_SLASH;
	new_file_path_name+= PointToName(src->GetPathName());

	std::string cmd_pattern = s_pattern.GetMB();
	std::string cmd_excl_pattern = s_exclude_pattern.GetMB();
	std::string cmd_in_file = src->GetPathName().GetMB();
	std::string cmd_new_file = new_file_path_name.GetMB();

	QuoteCmdArgIfNeed(cmd_pattern);
	QuoteCmdArgIfNeed(cmd_excl_pattern);
	QuoteCmdArgIfNeed(cmd_in_file);
	QuoteCmdArgIfNeed(cmd_new_file);

	if (cmd_pattern.empty()) {
		cmd_pattern = "''"; // otherwise grep expects input from stdin and stucks
	}

	std::string cmd = "grep ";
	if (!s_exclude_pattern.IsEmpty()) {
		cmd+= "-v ";
		if (!s_case_sensitive) {
			cmd+= "-i ";
		}
		if (s_whole_words) {
			cmd+= "-w ";
		}
		cmd+= "-- ";
		cmd+= cmd_excl_pattern;
		cmd+= ' ';
		cmd+= cmd_in_file;
		cmd+= " | grep ";
	}
	if (!s_case_sensitive) {
		cmd+= "-i ";
	}
	if (s_whole_words) {
		cmd+= "-w ";
	}
	if (s_context > 0) {
		cmd+= StrPrintf("-A %d -B %d ", s_context, s_context);
	}
	cmd+= "-- ";
	cmd+= cmd_pattern;
	if (s_exclude_pattern.IsEmpty()) {
		cmd+= ' ';
		cmd+= cmd_in_file;
	}
	cmd+= '>';
	cmd+= cmd_new_file;

	fprintf(stderr, "%s: '%s'\n", __FUNCTION__, cmd.c_str());
	const int r = system(cmd.c_str());
	if (r != 0) {
		fprintf(stderr, "%s: cmd returned %d\n", __FUNCTION__, r);
	}

	return std::make_shared<TempFileHolder>(new_file_path_name, true);
}
