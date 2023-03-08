#pragma once
#include <sys/stat.h>
#include <string>
#include <vector>
#include <map>

struct KeyFileCmp
{
	inline KeyFileCmp(bool case_insensitive = false) : _case_insensitive(case_insensitive) {}

	bool operator()(const std::string& a, const std::string& b) const;

private:
	bool _case_insensitive = false;
};

struct KeyFileValues : std::map<std::string, std::string, KeyFileCmp>
{
	KeyFileValues(bool case_insensitive = false);

	bool HasKey(const std::string &name) const;
	std::string GetString(const std::string &name, const char *def = "") const;
	std::wstring GetString(const std::string &name, const wchar_t *def) const;
	void GetChars(char *buffer, size_t maxchars, const std::string &name, const char *def = "") const;
	void GetChars(wchar_t *buffer, size_t maxchars, const std::string &name, const wchar_t *def = L"") const;
	int GetInt(const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(unsigned char *out, size_t len, const std::string &name, const unsigned char *def = nullptr) const;
	bool GetBytes(std::vector<unsigned char> &out, const std::string &name) const;
	std::vector<std::string> EnumKeys() const;
};

class KeyFileReadSection : public KeyFileValues
{
	bool _section_loaded;

public:
	KeyFileReadSection(const std::string &filename, const std::string &section, bool case_insensitive = false);

	bool SectionLoaded() const { return _section_loaded; }
};

class KeyFileReadHelper
{
protected:
	class Sections {
		typedef std::map<std::string, KeyFileValues, KeyFileCmp> Map;
		typedef std::vector<Map::iterator> IteratorsVec;

	public:
		Map _map;
		IteratorsVec _ordered;

		Map::const_iterator FindInMap(const std::string &name) const;
		Map::iterator FindInMap(const std::string &name);
		IteratorsVec::iterator FindInVec(Map::iterator it);

	public:
		Sections(bool case_insensitive);
		size_t Size() const { return _map.size(); }
		const KeyFileValues *Find(const std::string &section) const;
		KeyFileValues *Find(const std::string &section);
		KeyFileValues *Ensure(const std::string &section);
		bool Remove(const std::string &section);
		size_t RemoveAt(const std::string &section);
		bool Rename(const std::string &section, const std::string &new_name);
		const IteratorsVec &Enumeration() const;
	} _kf;

	struct stat _filestat {};
	bool _loaded = false;
	bool _case_insensitive;

public:
	KeyFileReadHelper(const std::string &filename, const char *load_only_section = nullptr, bool case_insensitive = false);

	bool IsLoaded() const { return _loaded; }

	inline const struct stat &LoadedFileStat() const { return _filestat; }

	size_t SectionsCount() const;

	bool HasSection(const std::string &section) const;

	// returned pointer valid until ~KeyFileReadHelper or next modify operation
	const KeyFileValues *GetSectionValues(const std::string &section) const;

	bool HasKey(const std::string &section, const std::string &name) const;
	std::string GetString(const std::string &section, const std::string &name, const char *def = "") const;
	std::wstring GetString(const std::string &section, const std::string &name, const wchar_t *def) const;
	int GetInt(const std::string &section, const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &section, const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &section, const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(unsigned char *out, size_t len, const std::string &section, const std::string &name, const unsigned char *def = nullptr) const;
	bool GetBytes(std::vector<unsigned char> &out, const std::string &section, const std::string &name) const;
	std::vector<std::string> EnumSections() const;
	std::vector<std::string> EnumSectionsAt(const std::string &parent_section, bool recursed = false) const;
	std::vector<std::string> EnumKeys(const std::string &section) const;
};

class KeyFileHelper : public KeyFileReadHelper
{
	std::string _filename;
	bool _dirty;

public:
	KeyFileHelper(const std::string &filename, bool load = true, bool case_insensitive = false);
	~KeyFileHelper();

	bool Save(bool only_if_dirty = true);

	void SetString(const std::string &section, const std::string &name, const std::string &value);
	void SetString(const std::string &section, const std::string &name, const char *value);
	void SetString(const std::string &section, const std::string &name, const wchar_t *value);
	void SetInt(const std::string &section, const std::string &name, int value);
	void SetUInt(const std::string &section, const std::string &name, unsigned int value);
	void SetULL(const std::string &section, const std::string &name, unsigned long long value);
	void SetBytes(const std::string &section, const std::string &name, const unsigned char *buf, size_t len, size_t space_interval = 0);
	bool RemoveSection(const std::string &section);
	size_t RemoveSectionsAt(const std::string &parent_section);
	void RemoveKey(const std::string &section, const std::string &name);
	void RenameSection(const std::string &src, const std::string &dst, bool recursed);
};
