#pragma once
#include <KeyFileHelper.h>
#include <memory>

extern const wchar_t *IMPOSSIBILIMO;

class ConfigSection
{
protected:
	std::string _section;
	virtual void OnSectionSelected() {}

public:
	void SelectSection(const char *section);
	void SelectSection(const wchar_t *section);
	void SelectSectionFmt(const char *format, ...);
};

class ConfigReader : public ConfigSection
{
	std::map<std::string, std::unique_ptr<KeyFileReadHelper> > _ini2kfh;
	std::unique_ptr<KeyFileValues> _empty_values;
	const KeyFileValues *_selected_section_values = nullptr;
	bool _has_section;

	virtual void OnSectionSelected();

public:
	ConfigReader(const char *preselect_section = nullptr);
	std::vector<std::string> EnumKeys();
	inline bool HasSection() const { return _has_section; };
	FARString GetString(const char *name, const wchar_t *def = L"") const;
	int GetInt(const char *name, int def = 0) const;
	unsigned int GetUInt(const char *name, unsigned int def = 0) const;
	unsigned long long GetULL(const char *name, unsigned long long def = 0) const;
	size_t GetBytes(const char *name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
};

class ConfigWriter : public ConfigSection
{
	std::map<std::string, std::unique_ptr<KeyFileHelper> > _ini2kfh;
	KeyFileHelper *_selected_kfh = nullptr;
	bool _nice_looking_section = false;

	virtual void OnSectionSelected();
public:
	ConfigWriter(const char *preselect_section = nullptr);
	void RemoveSection();
	void RenameSection(const char *new_section);
	void DefragIndexedSections(const char *indexed_prefix);

	void PutString(const char *name, const wchar_t *value);
	void PutInt(const char *name, int value);
	void PutUInt(const char *name, unsigned int value);
	void PutULL(const char *name, unsigned long long value);
	void PutBytes(const char *name, size_t len, const unsigned char *buf);
	void RemoveKey(const char *name);
};

void CheckForConfigUpgrade();

class GlobalConfigReader
{
	std::unique_ptr<ConfigReader> &_cfg_reader;

public:
	GlobalConfigReader(std::unique_ptr<ConfigReader> &cfg_reader)
		: _cfg_reader(cfg_reader)
	{
		_cfg_reader.reset(new ConfigReader);
	}

	~GlobalConfigReader()
	{
		_cfg_reader.reset();
	}

	static void Update(std::unique_ptr<ConfigReader> &cfg_reader)
	{
		cfg_reader.reset();
		cfg_reader.reset(new ConfigReader);
	}
};

