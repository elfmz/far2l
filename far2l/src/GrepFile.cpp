#include "headers.hpp"
#include "mix.hpp"
#include "pathmix.hpp"
#include "GrepFile.hpp"
#include "dialog.hpp"
#include "DialogBuilder.hpp"

FileHolderPtr GrepFile(FileHolderPtr src)
{
	DialogBuilder Builder(Msg::ConfigGrepFilterTitle, L"GrepFilter");
	static int CaseSensitive = 0;
	static int Context = 0;
	static FARString Pattern, ExclPattern;

	DialogItemEx *PatternEdit = Builder.AddEditField(&Pattern, 30, L"GrepPattern",
		DIF_FOCUS | DIF_HISTORY | DIF_USELASTHISTORY | DIF_NOAUTOCOMPLETE);
	Builder.AddTextBefore(PatternEdit, Msg::ConfigGrepFilterPattern);

	DialogItemEx *ExclPatternEdit = Builder.AddEditField(&ExclPattern, 30, L"GrepExclPattern",
		DIF_HISTORY | DIF_USELASTHISTORY | DIF_NOAUTOCOMPLETE);
	Builder.AddTextBefore(ExclPatternEdit, Msg::ConfigGrepFilterExclPattern);

	DialogItemEx *ContextEdit = Builder.AddIntEditField(&Context, 3);
	Builder.AddTextBefore(ContextEdit, Msg::ConfigGrepFilterContext);

	Builder.AddCheckbox(Msg::ConfigGrepFilterCaseSensitive, &CaseSensitive);

	Builder.AddOKCancel();
	if (!Builder.ShowDialog()) {
		return FileHolderPtr();
	}

	FARString new_file_path_name;
	if (!FarMkTempEx(new_file_path_name, L"grep")) {
		fprintf(stderr, "GrepFile: mktemp failed\n");
		return FileHolderPtr();
	}
	apiCreateDirectory(new_file_path_name, nullptr);

	new_file_path_name+= L'/';
	new_file_path_name+= PointToName(src->GetPathName());

	std::string cmd_pattern = Pattern.GetMB();
	std::string cmd_excl_pattern = ExclPattern.GetMB();
	std::string cmd_in_file = src->GetPathName().GetMB();
	std::string cmd_new_file = new_file_path_name.GetMB();

	QuoteCmdArgIfNeed(cmd_pattern);
	QuoteCmdArgIfNeed(cmd_excl_pattern);
	QuoteCmdArgIfNeed(cmd_in_file);
	QuoteCmdArgIfNeed(cmd_new_file);

	std::string cmd = "grep ";
	if (!ExclPattern.IsEmpty()) {
		cmd+= "-v ";
		if (!CaseSensitive) {
			cmd+= "-i ";
		}
		cmd+= cmd_excl_pattern;
		cmd+= ' ';
		cmd+= cmd_in_file;
		cmd+= " | grep ";
	}
	if (!CaseSensitive) {
		cmd+= "-i ";
	}
	if (Context > 0) {
		cmd+= StrPrintf("-A %d -B %d ", Context, Context);
	}
	cmd+= cmd_pattern;
	if (ExclPattern.IsEmpty()) {
		cmd+= ' ';
		cmd+= cmd_in_file;
	}
	cmd+= '>';
	cmd+= cmd_new_file;

	fprintf(stderr, "GrepFile: '%s'\n", cmd.c_str());
	const int r = system(cmd.c_str());
	if (r != 0) {
		fprintf(stderr, "GrepFile: cmd returned %d\n", r);
	}

	return std::make_shared<TempFileHolder>(new_file_path_name, true);
}
