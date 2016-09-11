#pragma once
#include <string>
#include <vector>

class KeyFileHelper
{
	struct _GKeyFile *_kf;
	std::string _filename;
	bool _dirty;
public:
	KeyFileHelper(const char *filename, bool load = true) ;
	~KeyFileHelper();
	
	std::string GetString(const char *section, const char *name, const char *def = "");
	void GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def = "");
	int GetInt(const char *section, const char *name, int def = 0);
	void PutString(const char *section, const char *name, const char *value);
	void PutInt(const char *section, const char *name, int value);
	std::vector<std::string> EnumSections();
};

