#pragma once
#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <vector>

class KeyFileReadHelper
{
protected:
	typedef std::unordered_map<std::string, std::string>  Values;
	struct Sections : std::unordered_map<std::string, Values> {} _kf;
	std::string _filename;
	mode_t _filemode = 0640;
	bool _loaded;

public:
	KeyFileReadHelper(const char *filename, const char *load_only_section = nullptr);

	bool IsLoaded() const { return _loaded; }

	bool HasSection(const char *section);
	bool HasKey(const char *section, const char *name);
	std::string GetString(const char *section, const char *name, const char *def = "");
	std::wstring GetString(const char *section, const char *name, const wchar_t *def);
	void GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def = "");
	int GetInt(const char *section, const char *name, int def = 0);
	unsigned int GetUInt(const char *section, const char *name, unsigned int def = 0);
	std::vector<std::string> EnumSections();
	std::vector<std::string> EnumSectionsAt(const char *parent_section, bool recursed = false);
	std::vector<std::string> EnumKeys(const char *section);
};


class KeyFileHelper : public KeyFileReadHelper
{
	bool _dirty;

public:
	KeyFileHelper(const char *filename, bool load = true);
	~KeyFileHelper();

	bool Save(bool only_if_dirty = true);

	void PutString(const char *section, const char *name, const char *value);
	void PutString(const char *section, const char *name, const wchar_t *value);
	void PutInt(const char *section, const char *name, int value);
	void PutUInt(const char *section, const char *name, unsigned int value);
	bool RemoveSection(const char *section);
	size_t RemoveSectionsAt(const char *parent_section);
	void RemoveKey(const char *section, const char *name);
};
