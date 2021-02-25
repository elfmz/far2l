#pragma once
#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <vector>

struct KeyFileValues : std::unordered_map<std::string, std::string>
{
	bool HasKey(const std::string &name) const;
	std::string GetString(const std::string &name, const char *def = "") const;
	std::wstring GetString(const std::string &name, const wchar_t *def) const;
	int GetInt(const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(const std::string &name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
	bool GetBytes(const std::string &name, std::vector<unsigned char> &out) const;
	std::vector<std::string> EnumKeys() const;
};

class KeyFileReadSection : public KeyFileValues
{
	bool _section_loaded;

public:
	KeyFileReadSection(const std::string &filename, const std::string &section);

	bool SectionLoaded() const { return _section_loaded; }
};

class KeyFileReadHelper
{
protected:
	struct Sections : std::unordered_map<std::string, KeyFileValues> {} _kf;
	struct stat _filestat {};
	bool _loaded;

public:
	KeyFileReadHelper(const std::string &filename, const char *load_only_section = nullptr);

	bool IsLoaded() const { return _loaded; }

	inline const struct stat &LoadedFileStat() const { return _filestat; }

	size_t SectionsCount() const;

	bool HasSection(const std::string &section) const;

	// returned pointer valid until ~KeyFileReadHelper or next modify operation
	const KeyFileValues *GetSectionValues(const std::string &section) const;

	bool HasKey(const std::string &section, const std::string &name) const;
	std::string GetString(const std::string &section, const std::string &name, const char *def = "") const;
	std::wstring GetString(const std::string &section, const std::string &name, const wchar_t *def) const;
	void GetChars(char *buffer, size_t buf_size, const std::string &section, const std::string &name, const char *def = "") const;
	int GetInt(const std::string &section, const std::string &name, int def = 0) const;
	unsigned int GetUInt(const std::string &section, const std::string &name, unsigned int def = 0) const;
	unsigned long long GetULL(const std::string &section, const std::string &name, unsigned long long def = 0) const;
	size_t GetBytes(const std::string &section, const std::string &name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
	bool GetBytes(const std::string &section, const std::string &name, std::vector<unsigned char> &out) const;
	std::vector<std::string> EnumSections() const;
	std::vector<std::string> EnumSectionsAt(const std::string &parent_section, bool recursed = false) const;
	std::vector<std::string> EnumKeys(const std::string &section) const;
};

class KeyFileHelper : public KeyFileReadHelper
{
	std::string _filename;
	bool _dirty;

public:
	KeyFileHelper(const std::string &filename, bool load = true);
	~KeyFileHelper();

	bool Save(bool only_if_dirty = true);

	void PutString(const std::string &section, const std::string &name, const std::string &value);
	void PutString(const std::string &section, const std::string &name, const char *value);
	void PutString(const std::string &section, const std::string &name, const wchar_t *value);
	void PutInt(const std::string &section, const std::string &name, int value);
	void PutUInt(const std::string &section, const std::string &name, unsigned int value);
	void PutUIntAsHex(const std::string &section, const std::string &name, unsigned int value);
	void PutULL(const std::string &section, const std::string &name, unsigned long long value);
	void PutULLAsHex(const std::string &section, const std::string &name, unsigned long long value);
	void PutBytes(const std::string &section, const std::string &name, size_t len, const unsigned char *buf, size_t space_interval = 0);
	bool RemoveSection(const std::string &section);
	size_t RemoveSectionsAt(const std::string &parent_section);
	void RemoveKey(const std::string &section, const std::string &name);
	void RenameSection(const std::string &src, const std::string &dst, bool recursed);
};
