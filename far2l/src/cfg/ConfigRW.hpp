#pragma once
#include <KeyFileHelper.h>
#include <memory>

#include "FARString.hpp"

class ConfigSection
{
protected:
	std::string _section;
	virtual void OnSectionSelected() {}

public:
	virtual ~ConfigSection() {}

	void SelectSection(const std::string &section);
	void SelectSection(const wchar_t *section);
	void FN_PRINTF_ARGS(2) SelectSectionFmt(const char *format, ...);
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
	ConfigReader(const std::string &section);

	static struct stat SavedSectionStat(const std::string &section);
	inline const struct stat &LoadedSectionStat() const { return _selected_kfh->LoadedFileStat(); }

	std::vector<std::string> EnumKeys();
	std::vector<std::string> EnumSectionsAt();
	inline bool HasSection() const { return _has_section; };
	bool HasKey(const std::string &name) const;
	FARString GetString(const std::string &name, const wchar_t *def = L"") const;
	bool GetString(FARString &out, const std::string &name, const wchar_t *def) const;
	bool GetString(std::string &out, const std::string &name, const char *def) const;
	int GetInt(const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(unsigned char *out, size_t len, const std::string &name, const unsigned char *def = nullptr) const;
	bool GetBytes(std::vector<unsigned char> &out, const std::string &name) const;
	template <class POD> void GetPOD(const std::string &name, POD &pod)
		{ GetBytes((unsigned char *)&pod, sizeof(pod), name); }
};

class ConfigWriter : public ConfigSection
{
	std::map<std::string, std::unique_ptr<KeyFileHelper> > _ini2kfh;
	KeyFileHelper *_selected_kfh = nullptr;
	size_t _bytes_space_interval = 0;

	virtual void OnSectionSelected();

	std::vector<std::string> EnumIndexedSections(const char *indexed_prefix);

public:
	ConfigWriter();
	ConfigWriter(const std::string &preselect_section);

	bool Save();

	void RemoveSection();

	void DefragIndexedSections(const char *indexed_prefix);
	void MoveIndexedSection(const char *indexed_prefix, unsigned int old_index, unsigned int new_index);
	void ReserveIndexedSection(const char *indexed_prefix, unsigned int index);

	void SetString(const std::string &name, const wchar_t *value);
	void SetString(const std::string &name, const std::string &value);
	void SetInt(const std::string &name, int value);
	void SetUInt(const std::string &name, unsigned int value);
	void SetULL(const std::string &name, unsigned long long value);
	void SetBytes(const std::string &name, const unsigned char *buf, size_t len);
	template <class POD> void SetPOD(const std::string &name, const POD &pod)
		{ SetBytes(name, (const unsigned char *)&pod, sizeof(pod)); }
	void RemoveKey(const std::string &name);
};

void CheckForConfigUpgrade();

class ConfigReaderScope
{
	std::unique_ptr<ConfigReader> &_cfg_reader;

public:
	ConfigReaderScope(std::unique_ptr<ConfigReader> &cfg_reader)
		: _cfg_reader(cfg_reader)
	{
		_cfg_reader.reset(new ConfigReader);
	}

	~ConfigReaderScope()
	{
		_cfg_reader.reset();
	}

	static void Update(std::unique_ptr<ConfigReader> &cfg_reader)
	{
		if (cfg_reader) {
			cfg_reader.reset();
			cfg_reader.reset(new ConfigReader);
		}
	}
};

