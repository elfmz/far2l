#pragma once
#include <string>

class TestPath
{
	bool _exists, _directory, _regular, _executable;

	void Init(const char *path);

public:
	inline TestPath(const char *path) { Init(path); }
	inline TestPath(const std::string &path) { Init(path.c_str()); }

	inline bool Exists() const { return _exists; }
	inline bool Directory() const { return _directory; }
	inline bool Regular() const { return _regular; }
	inline bool Executable() const { return _executable; }
};
