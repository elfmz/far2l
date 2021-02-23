#pragma once
#include <KeyFileHelper.h>
#include <memory>

class ConfigSection
{
protected:
	std::string _section;
	virtual void OnSectionSelected() {}

public:
	void SelectSection(const std::string &section);
	void SelectSection(const wchar_t *section);
	void SelectSectionFmt(const char *format, ...);
};

class ConfigReader : public ConfigSection
{
	std::map<std::string, std::unique_ptr<KeyFileReadHelper> > _ini2kfh;
	std::unique_ptr<KeyFileValues> _empty_values;
	KeyFileReadHelper *_selected_kfh = nullptr;
	const KeyFileValues *_selected_section_values = nullptr;
	bool _has_section;

	virtual void OnSectionSelected();

public:
	ConfigReader();
	ConfigReader(const std::string &preselect_section);

	std::vector<std::string> EnumKeys();
	std::vector<std::string> EnumSectionsAt();
	inline bool HasSection() const { return _has_section; };
	bool HasKey(const std::string &name) const;
	FARString GetString(const std::string &name, const wchar_t *def = L"") const;
	bool GetString(FARString &out, const std::string &name, const wchar_t *def) const;
	int GetInt(const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(const std::string &name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
	template <class POD> void GetPOD(const std::string &name, const POD &pod)
		{ GetBytes(name, sizeof(pod), (unsigned char *)&pod); }
};

class ConfigWriter : public ConfigSection
{
	std::map<std::string, std::unique_ptr<KeyFileHelper> > _ini2kfh;
	KeyFileHelper *_selected_kfh = nullptr;
	bool _nice_looking_section = false;

	virtual void OnSectionSelected();
public:
	ConfigWriter();
	ConfigWriter(const std::string &preselect_section);

	inline bool Save()
		{ return _selected_kfh->Save(); }

	void RemoveSection();
	void RenameSection(const std::string &new_section);
	void DefragIndexedSections(const char *indexed_prefix);

	void PutString(const std::string &name, const wchar_t *value);
	void PutInt(const std::string &name, int value);
	void PutUInt(const std::string &name, unsigned int value);
	void PutULL(const std::string &name, unsigned long long value);
	void PutBytes(const std::string &name, size_t len, const unsigned char *buf);
	template <class POD> void PutPOD(const std::string &name, const POD &pod)
		{ PutBytes(name, sizeof(pod), (const unsigned char *)&pod); }
	void RemoveKey(const std::string &name);
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

