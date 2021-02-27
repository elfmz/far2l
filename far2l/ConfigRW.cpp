#include "headers.hpp"
#include "ConfigRW.hpp"
#include <assert.h>
#include <algorithm>

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

	if (IsSectionOrSubsection(section, "XLat"))
		return "settings/xlat.ini";

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
	if (stat(InMyConfig(Section2Ini(section)).c_str(), &out) == -1) {
		memset(&out, 0, sizeof(out));
	}
	return out;
}

void ConfigReader::OnSectionSelected()
{
	const char *ini = Section2Ini(_section);
	auto &selected_kfh = _ini2kfh[ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileReadHelper(InMyConfig(ini)));
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
	assert(_selected_section_values != nullptr);
	return _selected_section_values->EnumKeys();
}

std::vector<std::string> ConfigReader::EnumSectionsAt()
{
	assert(_selected_kfh != nullptr);
	return _selected_kfh->EnumSectionsAt(_section);
}

bool ConfigReader::HasKey(const std::string &name) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->HasKey(name);
}

FARString ConfigReader::GetString(const std::string &name, const wchar_t *def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetString(name, def);
}

bool ConfigReader::GetString(FARString &out, const std::string &name, const wchar_t *def) const
{
	assert(_selected_section_values != nullptr);
	if (!_selected_section_values->HasKey(name)) {
		return false;
	}
	out = _selected_section_values->GetString(name, def);
	return true;
}

bool ConfigReader::GetString(std::string &out, const std::string &name, const char *def) const
{
	assert(_selected_section_values != nullptr);
	if (!_selected_section_values->HasKey(name)) {
		return false;
	}
	out = _selected_section_values->GetString(name, def);
	return true;
}

int ConfigReader::GetInt(const std::string &name, int def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetInt(name, def);
}

unsigned int ConfigReader::GetUInt(const std::string &name, unsigned int def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetUInt(name, def);
}

unsigned long long ConfigReader::GetULL(const std::string &name, unsigned long long def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetULL(name, def);
}

size_t ConfigReader::GetBytes(const std::string &name, size_t len, unsigned char *buf, const unsigned char *def) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetBytes(name, len, buf, def);
}

bool ConfigReader::GetBytes(const std::string &name, std::vector<unsigned char> &out) const
{
	assert(_selected_section_values != nullptr);
	return _selected_section_values->GetBytes(name, out);
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
	const char *ini = Section2Ini(_section);
	auto &selected_kfh = _ini2kfh[ini];
	if (!selected_kfh) {
		selected_kfh.reset(new KeyFileHelper(InMyConfig(ini)));
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

void ConfigWriter::RenameSection(const std::string &new_section)
{
	if (_section != new_section) {
		const char *old_ini = Section2Ini(_section);
		const char *new_ini = Section2Ini(new_section);
		if (strcmp(old_ini, new_ini)) {
			fprintf(stderr,
				"%s: different ini files '%s' [%s] -> '%s' [%s]\n",
				__FUNCTION__,
				_section.c_str(), old_ini, new_section.c_str(), new_ini);
			abort();
		}
		_selected_kfh->RenameSection(_section, new_section, true);
	}
}

std::vector<std::string> ConfigWriter::EnumIndexedSections(const char *indexed_prefix)
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

	return sections;
}

void ConfigWriter::DefragIndexedSections(const char *indexed_prefix)
{
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


void ConfigWriter::PutString(const std::string &name, const wchar_t *value)
{
	_selected_kfh->PutString(_section, name, value);
}

void ConfigWriter::PutString(const std::string &name, const std::string &value)
{
	_selected_kfh->PutString(_section, name, value);
}

void ConfigWriter::PutInt(const std::string &name, int value)
{
	_selected_kfh->PutInt(_section, name, value);
}

void ConfigWriter::PutUInt(const std::string &name, unsigned int value)
{
	_selected_kfh->PutUIntAsHex(_section, name, value);
}

void ConfigWriter::PutULL(const std::string &name, unsigned long long value)
{
	_selected_kfh->PutULLAsHex(_section, name, value);
}

void ConfigWriter::PutBytes(const std::string &name, size_t len, const unsigned char *buf)
{
	_selected_kfh->PutBytes(_section, name, len, buf, _bytes_space_interval);
}

void ConfigWriter::RemoveKey(const std::string &name)
{
	_selected_kfh->RemoveKey(_section, name);
}

//////////////////////////////////////////////////////////////////////////////////////
#ifdef WINPORT_REGISTRY
static bool ShouldImportRegSettings(const std::string &virtual_path)
{
	// skip stuff that goes to plugins/state.ini
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

		cfg_writer.SelectSection(virtual_path);
		const bool macro_type_prefix =
			(virtual_path == "KeyMacros/Vars" || virtual_path == "KeyMacros/Consts");

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
				std::string name(Wide2MB(&namebuf[0]));
				FARString tmp_str;
				switch (tip) {
					case REG_DWORD: {
						if (macro_type_prefix) {
							tmp_str.Format(L"INT:%ld", (long)*(int32_t *)&databuf[0]);
							cfg_writer.PutString(name, tmp_str);
						} else {
							cfg_writer.PutUInt(name, (long)*(uint32_t *)&databuf[0]);
						}
					} break;
					case REG_QWORD: {
						if (macro_type_prefix) {
							tmp_str.Format(L"INT:%lld", (long long)*(int64_t *)&databuf[0]);
							cfg_writer.PutString(name, tmp_str);
						} else {
							cfg_writer.PutULL(name, *(uint64_t *)&databuf[0]);
						}
					} break;
					case REG_SZ: case REG_EXPAND_SZ: case REG_MULTI_SZ: {
							if (macro_type_prefix) {
								tmp_str = L"STR:";
							}
							for (size_t i = 0; i + sizeof(WCHAR) <= datalen ; i+= sizeof(WCHAR)) {
								WCHAR wc = *(const WCHAR *)&databuf[i];
								if (!wc) {
									if (i + sizeof(WCHAR) >= datalen) {
										break;
									}
									// REG_MULTI_SZ was used only in macroses and history.
									// Macroses did inter-strings zeroes translated to \n.
									// Now macroses code doesn't do that translation, just need
									// to one time translate data imported from legacy registry.
									// History code now also uses '\n' chars as string separators.
									if (tip == REG_MULTI_SZ) {
										if (i + 2 * sizeof(WCHAR) >= datalen) {
											// skip last string terminator translation
											break;
										}
										tmp_str+= L'\n';
									} else {
										tmp_str.Append(wc);
									}
								} else {
									tmp_str.Append(wc);
								}
							}
							cfg_writer.PutString(name, tmp_str);
					} break;

					default:
						cfg_writer.PutBytes(name, datalen, &databuf[0]);
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
	if (stat(cfg_ini.c_str(), &s) == -1) {
		ConfigWriter cfg_writer;
		ConfigUgrade_RegKey(cfg_writer, HKEY_CURRENT_USER, L"Software/Far2", "");
		rename(InMyConfig("plugins.ini").c_str(), InMyConfig("plugins/state.ini").c_str());
	}
}

#else // WINPORT_REGISTRY

void CheckForConfigUpgrade() { }

#endif // WINPORT_REGISTRY
