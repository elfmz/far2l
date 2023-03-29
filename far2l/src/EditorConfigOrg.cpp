#include "EditorConfigOrg.hpp"
#include <string>
#include <windows.h>
#include <farplug-wide.h>
#include <utils.h>
#include <string.h>
#include <KeyFileHelper.h>
#include <MatchWildcard.hpp>

/// see https://editorconfig.org/


static void MergeEditorConfigFileValues(const KeyFileValues &values, KeyFileValues &props)
{
	for (const auto &it : values) {
		if (!props.HasKey(it.first)) {
			props.emplace(it.first, it.second);
		}
	}
}

static bool MatchEditorConfigFileSection(const char *edited_file, const std::string &section)
{
	size_t expand_left = section.find('{');
	if (expand_left != std::string::npos) {
		size_t expand_right = section.find('}', expand_left + 1);
		if (expand_right != std::string::npos) {
			std::vector<std::string> parts;
			StrExplode(parts, section.substr(expand_left + 1, expand_right - expand_left - 1), ",");
			std::string expanded = section.substr(0, expand_left);
			for (const auto &part : parts) {
				expanded+= part;
				expanded+= section.substr(expand_right + 1);
				if (MatchEditorConfigFileSection(edited_file, expanded)) {
					return true;
				}
				expanded.resize(expand_left);
			}
			return false;
		}
	}

	if (section.find('/') != std::string::npos) {
		if (section.find("./") == 0) {
			return MatchWildcardICE(edited_file, section.c_str() + 2);
		} else {
			return MatchWildcardICE(edited_file, section.c_str());
		}
	}

	const char *edited_file_name = strrchr(edited_file, '/');
	edited_file_name = edited_file_name ? edited_file_name + 1 : edited_file;
	return MatchWildcardICE(edited_file_name, section.c_str());
}

static bool ParseEditorConfigFile(const char *edited_file, KeyFileReadHelper &kfh, KeyFileValues &props)
{
	const auto &sections = kfh.EnumSections();
	for (auto it = sections.rbegin(); it != sections.rend(); ++it) {
		if (!MatchEditorConfigFileSection(edited_file, *it)) {
			continue;
		}
		fprintf(stderr, "EditorConfigOrg: matched section '%s'\n", it->c_str());

		const KeyFileValues *values = kfh.GetSectionValues(*it);
		if (!values) {
			continue;
		}

		MergeEditorConfigFileValues(*values, props);
	}

	return kfh.GetString(std::string(), "root") == "true";
}

void EditorConfigOrg::Populate(const char *edited_file)
{
	KeyFileValues props;
	std::string path(edited_file);
	for (;;) {
		size_t slash_pos = path.rfind('/');
		if (slash_pos == std::string::npos) {
			break;
		}
		path.resize(slash_pos + 1);
		path+= ".editorconfig";
		KeyFileReadHelper kfh(path);
		if (kfh.IsLoaded()) {
			fprintf(stderr, "EditorConfigOrg: loaded '%s'\n", path.c_str());
			if (ParseEditorConfigFile(edited_file + slash_pos + 1, kfh, props)) {
				break;
			}
		}
		path.resize(slash_pos);
	}

	const std::string &indent_style = props.GetString("indent_style");
	if (indent_style == "tab") {
		ExpandTabs = EXPAND_NOTABS;
	} else if (indent_style == "space") {
		ExpandTabs = EXPAND_NEWTABS;
	} else if (indent_style != "") {
		fprintf(stderr, "EditorConfigOrg: bad indent_style='%s'\n", indent_style.c_str());
	}

	int indent_size = props.GetInt("indent_size");
	if (indent_size > 0) {
		TabSize = indent_size;
	}

	const std::string &end_of_line = props.GetString("end_of_line");
	if (end_of_line == "lf") {
		EndOfLine = L"\n";
	} else if (end_of_line == "cr") {
		EndOfLine = L"\r";
	} else if (end_of_line == "crlf") {
		EndOfLine = L"\r\n";
	} else if (end_of_line != "") {
		fprintf(stderr, "EditorConfigOrg: bad end_of_line='%s'\n", end_of_line.c_str());
	}

	const std::string &trim_trailing_whitespace = props.GetString("trim_trailing_whitespace");
	if (trim_trailing_whitespace == "true") {
		TrimTrailingWhitespace = 1;
	} else if (trim_trailing_whitespace == "false") {
		TrimTrailingWhitespace = 0;
	} else if (trim_trailing_whitespace != "") {
		fprintf(stderr, "EditorConfigOrg: bad trim_trailing_whitespace='%s'\n", trim_trailing_whitespace.c_str());
	}

	const std::string &insert_final_newline = props.GetString("insert_final_newline");
	if (insert_final_newline == "true") {
		InsertFinalNewline = 1;
	} else if (insert_final_newline == "false") {
		InsertFinalNewline = 0;
	} else if (insert_final_newline != "") {
		fprintf(stderr, "EditorConfigOrg: bad insert_final_newline='%s'\n", insert_final_newline.c_str());
	}

	const std::string &charset = props.GetString("charset");
	if (charset == "latin1") {
		CodePage = CP_ACP;
		CodePageBOM = 0;

	} else if (charset == "utf-8") {
		CodePage = CP_UTF8;
		CodePageBOM = 0;

	} else if (charset == "utf-8-bom") {
		CodePage = CP_UTF8;
		CodePageBOM = 1;

	} else if (charset == "utf-16be") {
		CodePage = CP_UTF16BE;
		CodePageBOM = 0;

	} else if (charset == "utf-16le") {
		CodePage = CP_UTF16LE;
		CodePageBOM = 0;

	} else if (charset == "utf-16be-bom") {
		CodePage = CP_UTF16BE;
		CodePageBOM = 1;

	} else if (charset == "utf-16le-bom") {
		CodePage = CP_UTF16LE;
		CodePageBOM = 1;

	} else if (charset != "") {
		fprintf(stderr, "EditorConfigOrg: bad charset='%s'\n", charset.c_str());
	}

	fprintf(stderr,
		"EditorConfigOrg: ExpandTabs=%d TabSize=%d EndOfLine='%ls' TrimTrailingWhitespace=%d InsertFinalNewline=%d CodePage=%d CodePageBOM=%d\n",
		ExpandTabs, TabSize, EndOfLine ? EndOfLine : L"", TrimTrailingWhitespace, InsertFinalNewline, CodePage, CodePageBOM);
}
