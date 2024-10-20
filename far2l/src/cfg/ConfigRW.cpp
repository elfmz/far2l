#include "headers.hpp"
#include "ConfigRW.hpp"
#include <algorithm>

static bool IsSectionOrSubsection(const std::string &haystack, const char *needle)
{
	size_t l = strlen(needle);
	if (haystack.size() < l) {
		return false;
	}
	if (memcmp(haystack.c_str(), needle, l) != 0) {
		return false;
	}
	if (haystack.size() > l && haystack[l] != '/') {
		return false;
	}
	return true;
}

static const struct SectionProps
{
	const char *name;
	const char *ini;
	bool case_insensitive;
} s_default_section_props = { "", CONFIG_INI, false };

static const SectionProps s_section_props [] = {
	{"KeyMacros", "settings/key_macros.ini", true},
	{"Associations", "settings/associations.ini", false},
	{"Panel", "settings/panel.ini", false},
	{"CodePages", "settings/codepages.ini", false},
	{"XLat", "settings/xlat.ini", false},
	{"MaskGroups", "settings/maskgroups.ini"},
	{"Colors", "settings/colors.ini", false},
	{"SortGroups", "settings/colors.ini", false},
	{"UserMenu", "settings/user_menu.ini", false},
	{"SavedDialogHistory", "history/dialogs.hst", false},
	{"SavedHistory", "history/commands.hst", false},
	{"SavedFolderHistory", "history/folders.hst", false},
	{"SavedViewHistory", "history/view.hst", false}
};

static const SectionProps &GetSectionProps(const std::string &section)
{
	for (const auto &sp : s_section_props) {
		if (IsSectionOrSubsection(section, sp.name)) {
			return sp;
		}
	}

	return s_default_section_props;
}

void ConfigSection::SelectSection(const std::string &section)
{
	if (_section != section) {
		_section = section;
		OnSectionSelected();
	}
}

void ConfigSection::SelectSection(const wchar_t *section)
{
	SelectSection(Wide2MB(section));
}

void ConfigSection::SelectSectionFmt(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const std::string &section = StrPrintfV(format, args);
	va_end(args);

	SelectSection(section);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
ConfigReader::ConfigReader()
{
}

ConfigReader::ConfigReader(const std::string &preselect_section)
{
	SelectSection(preselect_section);
}

struct stat ConfigReader::SavedSectionStat(const std::string &section)
{
	struct stat out;
	if (stat(InMyConfig(GetSectionProps(section).ini).c_str(), &out) == -1) {
		memset(&out, 0, sizeof(out));
	}
	return out;
}

void ConfigReader::OnSectionSelected()
{
	const auto &sp = GetSectionProps(_section);
	auto &selected_kfh = _ini2kfh[sp.ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileReadHelper(InMyConfig(sp.ini), nullptr, sp.case_insensitive));
	}
	_selected_kfh = selected_kfh.get();
	_selected_section_values = _selected_kfh->GetSectionValues(_section);

	if (!_selected_section_values) {
		_has_section = false;
		if (!_empty_values) {
			_empty_values.reset(new KeyFileValues);
		}
		_selected_section_values = _empty_values.get();

	} else {
		_has_section = true;
	}
}

std::vector<std::string> ConfigReader::EnumKeys()
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->EnumKeys();
}

std::vector<std::string> ConfigReader::EnumSectionsAt(bool recursed)
{
	ASSERT(_selected_kfh != nullptr);
	return _selected_kfh->EnumSectionsAt(_section, recursed);
}

bool ConfigReader::HasKey(const std::string &name) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->HasKey(name);
}

FARString ConfigReader::GetString(const std::string &name, const wchar_t *def) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetString(name, def);
}

bool ConfigReader::GetString(FARString &out, const std::string &name, const wchar_t *def) const
{
	ASSERT(_selected_section_values != nullptr);
	if (!_selected_section_values->HasKey(name)) {
		return false;
	}
	out = _selected_section_values->GetString(name, def);
	return true;
}

bool ConfigReader::GetString(std::string &out, const std::string &name, const char *def) const
{
	ASSERT(_selected_section_values != nullptr);
	if (!_selected_section_values->HasKey(name)) {
		return false;
	}
	out = _selected_section_values->GetString(name, def);
	return true;
}

int ConfigReader::GetInt(const std::string &name, int def) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetInt(name, def);
}

unsigned int ConfigReader::GetUInt(const std::string &name, unsigned int def) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetUInt(name, def);
}

unsigned long long ConfigReader::GetULL(const std::string &name, unsigned long long def) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetULL(name, def);
}

size_t ConfigReader::GetBytes(unsigned char *out, size_t len, const std::string &name, const unsigned char *def) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetBytes(out, len, name, def);
}

bool ConfigReader::GetBytes(std::vector<unsigned char> &out, const std::string &name) const
{
	ASSERT(_selected_section_values != nullptr);
	return _selected_section_values->GetBytes(out, name);
}

////
ConfigWriter::ConfigWriter()
{
}

ConfigWriter::ConfigWriter(const std::string &preselect_section)
{
	SelectSection(preselect_section);
}

bool ConfigWriter::Save()
{
	bool out = true;

	for (const auto &it : _ini2kfh) {
		if (!it.second->Save()) {
			out = false;
		}
	}

	return out;
}

void ConfigWriter::OnSectionSelected()
{
	const auto &sp = GetSectionProps(_section);
	auto &selected_kfh = _ini2kfh[sp.ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileHelper(InMyConfig(sp.ini), true, sp.case_insensitive));
	}
	_selected_kfh =	selected_kfh.get();

	if (IsSectionOrSubsection(_section, "Colors")) {
		_bytes_space_interval = 1;

	} else if (IsSectionOrSubsection(_section, "SavedHistory")
		|| IsSectionOrSubsection(_section, "SavedDialogHistory")
		|| IsSectionOrSubsection(_section, "SavedFolderHistory")
		|| IsSectionOrSubsection(_section, "SavedViewHistory")) {

		_bytes_space_interval = sizeof(FILETIME);

	} else {
		_bytes_space_interval = 0;
	}
}

void ConfigWriter::RemoveSection()
{
	_selected_kfh->RemoveSectionsAt(_section);
	_selected_kfh->RemoveSection(_section);
}

std::vector<std::string> ConfigWriter::EnumIndexedSections(const char *indexed_prefix)
{
	std::string saved_section = _section;
	SelectSectionFmt("%s0", indexed_prefix);

	const size_t indexed_prefix_len = strlen(indexed_prefix);
	std::vector<std::string> sections = _selected_kfh->EnumSections();
	// exclude sections not prefixed by indexed_prefix
	// and sections that are nested from prefixed by indexed_prefix
	for (auto it = sections.begin(); it != sections.end();) {
		if (it->size() <= indexed_prefix_len
				|| memcmp(it->c_str(), indexed_prefix, indexed_prefix_len) != 0
				|| strchr(it->c_str() + indexed_prefix_len, '/') != nullptr) {
			it = sections.erase(it);
		} else {
			++it;
		}
	}

	std::sort(sections.begin(), sections.end(),
		[&](const std::string &a, const std::string &b) -> bool {
		return atoi(a.c_str() + indexed_prefix_len) < atoi(b.c_str() + indexed_prefix_len);
	});

	if (!saved_section.empty()) {
		SelectSection(saved_section);
	}
	return sections;
}

void ConfigWriter::DefragIndexedSections(const char *indexed_prefix)
{
	std::string saved_section = _section;
	SelectSectionFmt("%s0", indexed_prefix);

	std::vector<std::string> sections = EnumIndexedSections(indexed_prefix);

	for (size_t i = 0; i < sections.size(); ++i) {
		const std::string &expected_section = StrPrintf("%s%lu", indexed_prefix, (unsigned long)i);
		if (sections[i] != expected_section) {
			_selected_kfh->RenameSection(sections[i], expected_section, true);
			if (_section == sections[i]) {
				_section = expected_section;
			}
		}
	}

	if (!saved_section.empty()) {
		SelectSection(saved_section);
	}
}

void ConfigWriter::ReserveIndexedSection(const char *indexed_prefix, unsigned int index)
{
	DefragIndexedSections(indexed_prefix);

	std::vector<std::string> sections = EnumIndexedSections(indexed_prefix);

	for (size_t i = sections.size(); i > index;) {
		const std::string &new_name = StrPrintf("%s%lu", indexed_prefix, (unsigned long)i);
		--i;
		const std::string &old_name = StrPrintf("%s%lu", indexed_prefix, (unsigned long)i);
		_selected_kfh->RenameSection(old_name, new_name, true);
	}
}

void ConfigWriter::MoveIndexedSection(const char *indexed_prefix, unsigned int old_index, unsigned int new_index)
{
	if (old_index == new_index) {
		return;
	}

	const std::string &old_section_tmp = StrPrintf("%s%u.tmp-%llx",
		indexed_prefix, old_index, (unsigned long long)time(NULL));

	SelectSection(old_section_tmp);

	_selected_kfh->RenameSection(
		StrPrintf("%s%u", indexed_prefix, old_index),
		old_section_tmp,
		true);

	if (old_index < new_index) {
		// rename [old_index + 1, new_index] -> [old_index, new_index - 1]
		for (unsigned int i = old_index + 1; i <= new_index; ++i) {
			_selected_kfh->RenameSection(
				StrPrintf("%s%u", indexed_prefix, i),
				StrPrintf("%s%u", indexed_prefix, i - 1),
				true);
		}
	} else {
		// rename [new_index, old_index - 1] -> [new_index + 1, old_index]
		for (unsigned int i = old_index; i > new_index; --i) {
			_selected_kfh->RenameSection(
				StrPrintf("%s%u", indexed_prefix, i - 1),
				StrPrintf("%s%u", indexed_prefix, i),
				true);
		}
	}

	_selected_kfh->RenameSection(
		old_section_tmp,
		StrPrintf("%s%u", indexed_prefix, new_index),
		true);

	DefragIndexedSections(indexed_prefix);
}


void ConfigWriter::SetString(const std::string &name, const wchar_t *value)
{
	_selected_kfh->SetString(_section, name, value);
}

void ConfigWriter::SetString(const std::string &name, const std::string &value)
{
	_selected_kfh->SetString(_section, name, value);
}

void ConfigWriter::SetInt(const std::string &name, int value)
{
	_selected_kfh->SetInt(_section, name, value);
}

void ConfigWriter::SetUInt(const std::string &name, unsigned int value)
{
	_selected_kfh->SetUInt(_section, name, value);
}

void ConfigWriter::SetULL(const std::string &name, unsigned long long value)
{
	_selected_kfh->SetULL(_section, name, value);
}

void ConfigWriter::SetBytes(const std::string &name, const unsigned char *buf, size_t len)
{
	_selected_kfh->SetBytes(_section, name, buf, len, _bytes_space_interval);
}

void ConfigWriter::RemoveKey(const std::string &name)
{
	_selected_kfh->RemoveKey(_section, name);
}
