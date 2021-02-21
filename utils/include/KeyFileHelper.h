#pragma once
#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <vector>

struct KeyFileValues : std::unordered_map<std::string, std::string>
{
	bool HasKey(const char *name) const;
	std::string GetString(const char *name, const char *def = "") const;
	std::wstring GetString(const char *name, const wchar_t *def) const;
	int GetInt(const char *name, int def = 0) const;
	unsigned int GetUInt(const char *name, unsigned int def = 0) const;
	unsigned long long GetULL(const char *name, unsigned long long def = 0) const;
	size_t GetBytes(const char *name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
	std::vector<std::string> EnumKeys() const;
};

class KeyFileReadSection : public KeyFileValues
{
	bool _section_loaded;

public:
	KeyFileReadSection(const char *filename, const char *section);

	bool SectionLoaded() const { return _section_loaded; }
};

class KeyFileReadHelper
{
protected:
	struct Sections : std::unordered_map<std::string, KeyFileValues> {} _kf;
	mode_t _filemode = 0640;
	bool _loaded;

public:
	KeyFileReadHelper(const char *filename, const char *load_only_section = nullptr);

	bool IsLoaded() const { return _loaded; }

	size_t SectionsCount() const;

	bool HasSection(const char *section) const;

	// returned value valid only until next modify operation
	const KeyFileValues *GetSectionValues(const char *section) const;

	bool HasKey(const char *section, const char *name) const;
	std::string GetString(const char *section, const char *name, const char *def = "") const;
	std::wstring GetString(const char *section, const char *name, const wchar_t *def) const;
	void GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def = "") const;
	int GetInt(const char *section, const char *name, int def = 0) const;
	unsigned int GetUInt(const char *section, const char *name, unsigned int def = 0) const;
	unsigned long long GetULL(const char *section, const char *name, unsigned long long def = 0) const;
	size_t GetBytes(const char *section, const char *name, size_t len, unsigned char *buf, const unsigned char *def = nullptr) const;
	std::vector<std::string> EnumSections() const;
	std::vector<std::string> EnumSectionsAt(const char *parent_section, bool recursed = false) const;
	std::vector<std::string> EnumKeys(const char *section) const;
};

class KeyFileHelper : public KeyFileReadHelper
{
	std::string _filename;
	bool _dirty;

public:
	KeyFileHelper(const char *filename, bool load = true);
	~KeyFileHelper();

	bool Save(bool only_if_dirty = true);

	void PutString(const char *section, const char *name, const char *value);
	void PutString(const char *section, const char *name, const wchar_t *value);
	void PutInt(const char *section, const char *name, int value);
	void PutUInt(const char *section, const char *name, unsigned int value);
	void PutUIntAsHex(const char *section, const char *name, unsigned int value);
	void PutULL(const char *section, const char *name, unsigned long long value);
	void PutULLAsHex(const char *section, const char *name, unsigned long long value);
	void PutBytes(const char *section, const char *name, size_t len, const unsigned char *buf, bool spaced = false);
	bool RemoveSection(const char *section);
	size_t RemoveSectionsAt(const char *parent_section);
	void RemoveKey(const char *section, const char *name);
	void RenameSection(const char *src, const char *dst, bool recursed);
};
