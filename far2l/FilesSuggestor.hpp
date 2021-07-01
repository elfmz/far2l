#pragma once
#include <sys/stat.h>
#include <mutex>
#include <string>
#include <vector>
#include <Threaded.h>

class FilesSuggestor : protected Threaded
{
	std::string _dir_path;
	struct stat _dir_st{};
	std::vector<std::string> _files;
	std::mutex _mtx;
	enum {
		S_IDLE,
		S_ACTIVATED,
		S_STOPPING
	} _state = S_IDLE;

	void EnsureThreadStopped();
	bool Start(const std::string &dir_path, const struct stat &dir_st);

protected:
	virtual void *ThreadProc();

public:
	virtual ~FilesSuggestor();
	void Suggest(const std::string &filter, std::vector<std::string> &result);
};
