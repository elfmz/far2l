#pragma once
#include <sys/stat.h>
#include <mutex>
#include <string>
#include <vector>
#include <Threaded.h>
#include "vmenu.hpp"

class FilesSuggestor : protected Threaded
{
	std::string _dir_path;
	struct stat _dir_st{};
	std::vector<std::string> _files;
	std::mutex _mtx;
	bool _stopping = false;

	bool StartEnum(const std::string &dir_path, const struct stat &dir_st);

protected:
	virtual void *ThreadProc();

public:
	virtual ~FilesSuggestor();
	void Suggest(const std::string &filter, std::vector<std::string> &result);
};


class MenuFilesSuggestor : protected FilesSuggestor
{
	std::vector<std::string> _suggestions;

public:
	void Suggest(const wchar_t *filter, VMenu& menu);
};
