#include "headers.hpp"
#include "registry.hpp"
#include "ConfigRW.hpp"
#include <assert.h>

void ConfigSection::SelectSection(const char *section)
{
	if (_section != section) {
		_section = section;
		OnSectionSelected();
	}
}

void ConfigSection::SelectSectionFmt(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const std::string &section = StrPrintfV(format, args);
	va_end(args);

	SelectSection(section.c_str());
}

const char *ConfigSection::SelectedSection() const
{
	assert(!_section.empty());
	return _section.c_str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

ConfigReader::ConfigReader(const char *name)
{
	const std::string &cfg_ini = InMyConfig(name);
	struct stat s{};
	if (stat(cfg_ini.c_str(), &s) == 0) {
		_kfh.reset(new KeyFileReadHelper(cfg_ini.c_str()));
	}
}

void ConfigReader::OnSectionSelected()
{
	if (_kfh) {
		_selected_section_values = _kfh->GetSectionValues(SelectedSection());
		if (!_selected_section_values) {
			if (!_empty_values) {
				_empty_values.reset(new KeyFileValues);
			}
			_selected_section_values = _empty_values.get();
		}
	}
}

FARString ConfigReader::GetString(const char *name, const wchar_t *def) const
{
	FARString out;
	if (_kfh) {
		assert(_selected_section_values != nullptr);
		out = _selected_section_values->GetString(name, def);
	} else {
		GetRegKey(FARString(SelectedSection()), FARString(name), out, def);
	}
	return out;
}

int ConfigReader::GetInt(const char *name, int def) const
{
	if (_kfh) {
		assert(_selected_section_values != nullptr);
		return _selected_section_values->GetInt(name, def);
	}

	int out = def;
	GetRegKey(FARString(SelectedSection()), FARString(name), out, (DWORD)def);
	return out;
}

unsigned int ConfigReader::GetUInt(const char *name, unsigned int def) const
{
	if (_kfh) {
		assert(_selected_section_values != nullptr);
		return _selected_section_values->GetUInt(name, def);
	}

	int out = def;
	GetRegKey(FARString(SelectedSection()), FARString(name), out, (DWORD)def);
	return (unsigned int)out;
}

size_t ConfigReader::GetBytes(const char *name, size_t len, unsigned char *buf, const unsigned char *def) const
{
	if (_kfh) {
		assert(_selected_section_values != nullptr);
		return _selected_section_values->GetBytes(name, len, buf, def);
	}

	return GetRegKey(FARString(SelectedSection()), FARString(name), (BYTE*)buf, (BYTE*)def, len);
}

////

ConfigWriter::ConfigWriter(const char *name)
	: _kfh(InMyConfig(name).c_str())
{
}

void ConfigWriter::RemoveSectionAndSubsections(const char *name)
{
	_kfh.RemoveSectionsAt(name);
	_kfh.RemoveSection(name);
}

void ConfigWriter::PutString(const char *name, const wchar_t *value)
{
	_kfh.PutString(SelectedSection(), name, value);
}

void ConfigWriter::PutInt(const char *name, int value)
{
	_kfh.PutInt(SelectedSection(), name, value);
}

void ConfigWriter::PutUInt(const char *name, unsigned int value)
{
	_kfh.PutUInt(SelectedSection(), name, value);
}

void ConfigWriter::PutBytes(const char *name, size_t len, const unsigned char *buf)
{
	_kfh.PutBytes(SelectedSection(), name, len, buf, true);
}

