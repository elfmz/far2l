#pragma once
#include <KeyFileHelper.h>
#include <memory>

class ConfigSection
{
protected:
	std::string _section;
	virtual void OnSectionSelected() {}

public:
	void SelectSection(const char *section);
	void SelectSectionFmt(const char *format, ...);
	const char *SelectedSection() const;
};

class ConfigReader : public ConfigSection
{
	std::unique_ptr<KeyFileReadHelper> _kfh;
	std::unique_ptr<KeyFileValues> _empty_values;
	const KeyFileValues *_selected_section_values = nullptr;

	virtual void OnSectionSelected();

public:
	ConfigReader(const char *name);
	FARString GetString(const char *name, const wchar_t *def = L"") const;
	int GetInt(const char *name, int def = 0) const;
	unsigned int GetUInt(const char *name, unsigned int def = 0) const;
	size_t GetBytes(const char *name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
};

class ConfigWriter : public ConfigSection
{
	KeyFileHelper _kfh;

public:
	ConfigWriter(const char *name);
	void RemoveSectionAndSubsections(const char *name);
	void PutString(const char *name, const wchar_t *value);
	void PutInt(const char *name, int value);
	void PutUInt(const char *name, unsigned int value);
	void PutBytes(const char *name, size_t len, const unsigned char *buf);
};
