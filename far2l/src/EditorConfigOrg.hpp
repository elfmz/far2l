#pragma once

struct EditorConfigOrg
{
	int ExpandTabs = -1;
	int TabSize = -1;
	const wchar_t *EndOfLine = nullptr;
	int TrimTrailingWhitespace = -1;
	int InsertFinalNewline = -1;
	int CodePage = -1;
	int CodePageBOM = -1;

	int pos_trim_dir_nearest = -1;
	int pos_trim_dir_root = -1;

	void Populate(const char *edited_file);
};
