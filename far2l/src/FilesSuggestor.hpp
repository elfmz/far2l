#pragma once
#include <sys/stat.h>
#include <mutex>
#include <string>
#include <vector>
#include <Threaded.h>
#include "vmenu.hpp"

class FilesSuggestor : protected Threaded
{
public:
	struct Suggestion {
		std::string name;
		bool dir;
		bool operator < (const Suggestion &another) const;
	};

private:
	std::string _dir_path;
	struct stat _dir_st{};
	std::vector<Suggestion> _suggestions;
	std::mutex _mtx;
	bool _stopping = false;

	bool StartEnum(const std::string &dir_path, const struct stat &dir_st);

protected:
	virtual void *ThreadProc();

public:
	virtual ~FilesSuggestor();
	void Suggest(const std::string &filter, std::vector<Suggestion> &result);
};


class MenuFilesSuggestor : protected FilesSuggestor
{
	std::vector<Suggestion> _suggestions;

public:
	void Suggest(const wchar_t *filter, VMenu& menu, bool escaping = true); // escaping = 0 - filter & results without escaping
};
