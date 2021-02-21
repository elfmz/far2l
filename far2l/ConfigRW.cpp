#include "headers.hpp"
#include "registry.hpp"
#include "ConfigRW.hpp"
#include <assert.h>
#include <algorithm>

const wchar_t *IMPOSSIBILIMO = L"!!!ImPoSsIbIlImO!!!";

#define CONFIG_INI "settings/config.ini"

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

static const char *Section2Ini(const std::string &section)
{
	if (IsSectionOrSubsection(section, "KeyMacros"))
		return "settings/key_macros.ini";

	if (IsSectionOrSubsection(section, "Associations"))
		return "settings/associations.ini";

	if (IsSectionOrSubsection(section, "Panel"))
		return "settings/panel.ini";

	if (IsSectionOrSubsection(section, "CodePages"))
		return "settings/codepages.ini";

	if (IsSectionOrSubsection(section, "Colors")
		|| IsSectionOrSubsection(section, "SortGroups") )
		return "settings/colors.ini";

	if (IsSectionOrSubsection(section, "UserMenu"))
		return "settings/user_menu.ini";

	if (IsSectionOrSubsection(section, "SavedDialogHistory"))
		return "history/dialogs.ini";

	if (IsSectionOrSubsection(section, "SavedHistory"))
		return "history/commands.ini";

	if (IsSectionOrSubsection(section, "SavedFolderHistory"))
		return "history/folders.ini";

	if (IsSectionOrSubsection(section, "SavedViewHistory"))
		return "history/view.ini";

	return CONFIG_INI;
}

static bool IsNiceLookingSection(const char *section)
{
	return (IsSectionOrSubsection(section, "Colors"));
}

void ConfigSection::SelectSection(const char *section)
{
	if (_section != section) {
		_section = section;
		OnSectionSelected();
	}
}

void ConfigSection::SelectSection(const wchar_t *section)
{
	SelectSection(Wide2MB(section).c_str());
}

void ConfigSection::SelectSectionFmt(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const std::string &section = StrPrintfV(format, args);
	va_end(args);

	SelectSection(section.c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////

ConfigReader::ConfigReader(const char *preselect_section)
{
	if (preselect_section) {
		SelectSection(preselect_section);
	}
}

void ConfigReader::OnSectionSelected()
{
	const char *ini = Section2Ini(_section.c_str());
	auto &selected_kfh = _ini2kfh[ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileReadHelper(InMyConfig(ini).c_str()));
	}
	
	_selected_section_values = selected_kfh->GetSectionValues(_section.c_str());

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
	assert(_selected_section_values != nullptr);
	std::vector<std::string> out = _selected_section_values->EnumKeys();
	std::sort(out.begin(), out.end());
	return out;
}

FARString ConfigReader::GetString(const char *name, const wchar_t *def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetString(name, def);
}

int ConfigReader::GetInt(const char *name, int def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetInt(name, def);
}

unsigned int ConfigReader::GetUInt(const char *name, unsigned int def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetUInt(name, def);
}

unsigned long long ConfigReader::GetULL(const char *name, unsigned long long def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetULL(name, def);
}

size_t ConfigReader::GetBytes(const char *name, size_t len, unsigned char *buf, const unsigned char *def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetBytes(name, len, buf, def);
}

////

ConfigWriter::ConfigWriter(const char *preselect_section)
{
	if (preselect_section) {
		SelectSection(preselect_section);
	}
}

void ConfigWriter::OnSectionSelected()
{
	const char *ini = Section2Ini(_section.c_str());
	auto &selected_kfh = _ini2kfh[ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileHelper(InMyConfig(ini).c_str()));
	}
	_selected_kfh =	selected_kfh.get();
	_nice_looking_section = IsNiceLookingSection(_section.c_str());
}

void ConfigWriter::RemoveSection()
{
	_selected_kfh->RemoveSectionsAt(_section.c_str());
	_selected_kfh->RemoveSection(_section.c_str());
}

void ConfigWriter::RenameSection(const char *new_section)
{
	if (_section != new_section) {
		const char *old_ini = Section2Ini(_section.c_str());
		const char *new_ini = Section2Ini(new_section);
		if (strcmp(old_ini, new_ini)) {
			fprintf(stderr,
				"%s: different ini files '%s' [%s] -> '%s' [%s]\n",
				__FUNCTION__,
				_section.c_str(), old_ini, new_section, new_ini);
			abort();
		}
		_selected_kfh->RenameSection(_section.c_str(), new_section, true);
	}
}

void ConfigWriter::DefragIndexedSections(const char *indexed_prefix)
{
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

	for (size_t i = 0; i < sections.size(); ++i) {
		const std::string &expected_section = StrPrintf("%s%lu", indexed_prefix, (unsigned long)i);
		if (sections[i] != expected_section) {
			_selected_kfh->RenameSection(sections[i].c_str(), expected_section.c_str(), true);
			if (_section == sections[i]) {
				_section = expected_section;
			}
		}
	}
}


void ConfigWriter::PutString(const char *name, const wchar_t *value)
{
	_selected_kfh->PutString(_section.c_str(), name, value);
}

void ConfigWriter::PutInt(const char *name, int value)
{
	_selected_kfh->PutInt(_section.c_str(), name, value);
}

void ConfigWriter::PutUInt(const char *name, unsigned int value)
{
	_selected_kfh->PutUIntAsHex(_section.c_str(), name, value);
}

void ConfigWriter::PutULL(const char *name, unsigned long long value)
{
	_selected_kfh->PutULLAsHex(_section.c_str(), name, value);
}

void ConfigWriter::PutBytes(const char *name, size_t len, const unsigned char *buf)
{
	_selected_kfh->PutBytes(_section.c_str(), name, len, buf, _nice_looking_section);
}

void ConfigWriter::RemoveKey(const char *name)
{
	_selected_kfh->RemoveKey(_section.c_str(), name);
}

//////////////////////////////////////////////////////////////////////////////////////

static bool ShouldImportRegSettings(const std::string &virtual_path)
{
	// skip stuff that goes to Plugins.ini
	return (virtual_path != "Plugins" && virtual_path != "PluginHotkeys");
}

static void ConfigUgrade_RegKey(ConfigWriter &cfg_writer, HKEY root, const wchar_t *subpath, const std::string &virtual_path)
{
	HKEY key = 0;
	LONG r = WINPORT(RegOpenKeyEx)(root, subpath, 0, GENERIC_READ, &key);
	std::string virtual_subpath;
	if (r == ERROR_SUCCESS) {
		std::vector<wchar_t> namebuf(0x400);
		for (DWORD i = 0; ;++i) {
			r = WINPORT(RegEnumKey)(key, i, &namebuf[0], namebuf.size() - 1);
			if (r != ERROR_SUCCESS) break;
			virtual_subpath = virtual_path;
			if (!virtual_subpath.empty()) {
				virtual_subpath+= '/';
			}
			virtual_subpath+= Wide2MB(&namebuf[0]);
			if (ShouldImportRegSettings(virtual_subpath)) {
				ConfigUgrade_RegKey(cfg_writer, key, &namebuf[0], virtual_subpath);
			}
		}

		cfg_writer.SelectSection(virtual_path.c_str());

		std::vector<BYTE> databuf(0x400);
		for (DWORD i = 0; ;) {
			DWORD namelen = namebuf.size() - 1;
			DWORD datalen = databuf.size();
			DWORD tip = 0;
            r = WINPORT(RegEnumValue)(key, i,
				&namebuf[0], &namelen, NULL, &tip, &databuf[0], &datalen);
			if (r != ERROR_SUCCESS) {
				if (r != ERROR_MORE_DATA) {
					break;
				}
				namebuf.resize(namebuf.size() + 0x400);
				databuf.resize(databuf.size() + 0x400);			
			} else {
				std::string name = Wide2MB(&namebuf[0]);
				switch (tip) {
					case REG_DWORD: {
						cfg_writer.PutUInt(name.c_str(), *(const unsigned int *)&databuf[0]);
					} break;
					case REG_QWORD: {
						cfg_writer.PutULL(name.c_str(), *(const unsigned long long *)&databuf[0]);
					} break;
					case REG_SZ: case REG_EXPAND_SZ: {
						std::wstring str((const WCHAR *)&databuf[0], datalen / sizeof(WCHAR));
						if (!str.empty() && !str.back()) {
							str.resize(str.size() - 1);
						}
						cfg_writer.PutString(name.c_str(), str.c_str());
					} break;
					default: {
						cfg_writer.PutBytes(name.c_str(), datalen, &databuf[0]);
					} break;
				}
				++i;
			}
		}
		WINPORT(RegCloseKey)(key);
	}
}

void CheckForConfigUpgrade()
{
	const std::string &cfg_ini = InMyConfig(CONFIG_INI);
	struct stat s{};
	if (stat(cfg_ini.c_str(), &s) != 0) {
		ConfigWriter cfg_writer;
		ConfigUgrade_RegKey(cfg_writer, HKEY_CURRENT_USER, L"Software/Far2", "");
	}
}
