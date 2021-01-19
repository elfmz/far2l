#pragma once
#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <vector>

class KeyFileHelper
{
	typedef std::unordered_map<std::string, std::string>  Values;
	struct Sections : std::unordered_map<std::string, Values> {} _kf;
	std::string _filename;
	mode_t _filemode = 0640;
	bool _dirty, _loaded;

public:
	KeyFileHelper(const char *filename, bool load = true) ;
	~KeyFileHelper();

	bool IsLoaded() const { return _loaded; }
	bool Save();
	
	bool HasKey(const char *section, const char *name);
	std::string GetString(const char *section, const char *name, const char *def = "");
	void GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def = "");
	int GetInt(const char *section, const char *name, int def = 0);
	void PutString(const char *section, const char *name, const char *value);
	void PutInt(const char *section, const char *name, int value);
	std::vector<std::string> EnumSections();
	std::vector<std::string> EnumSectionsAt(const char *parent_section, bool recursed = false);
	std::vector<std::string> EnumKeys(const char *section);
	void RemoveSection(const char *section);
	void RemoveSectionsAt(const char *parent_section);
	void RemoveKey(const char *section, const char *name);
};

