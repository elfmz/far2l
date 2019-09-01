#pragma once
#include <ScopeHelpers.h>
#include <string>
#include <fcntl.h>

class ExecCommandFIFO
{
	std::string _fifo;

	void CleanupFIFO()
	{
		{
			FDScope fd_ctl(open((_fifo + ".ctl").c_str(), O_WRONLY | O_NONBLOCK));
			FDScope fd_in(open((_fifo + ".in").c_str(), O_WRONLY | O_NONBLOCK));
			FDScope fd_out(open((_fifo + ".out").c_str(), O_RDONLY | O_NONBLOCK));
			FDScope fd_err(open((_fifo + ".err").c_str(), O_RDONLY | O_NONBLOCK));
		}

		unlink((_fifo + ".ctl").c_str());
		unlink((_fifo + ".in").c_str());
		unlink((_fifo + ".out").c_str());
		unlink((_fifo + ".err").c_str());
		unlink((_fifo + ".status").c_str());
	}

public:
	ExecCommandFIFO() throw (std::runtime_error)
	{
		char sz[64] = {};
		snprintf(sz, sizeof(sz) - 1, "NetRocks/fifo/%lx", (unsigned long)getpid());

		_fifo = InMyTemp(sz);

		CleanupFIFO();

		if (mkfifo((_fifo + ".ctl").c_str(), 0700) == -1 ) {
			throw std::runtime_error("mkfifo ctl");
		}
		if (mkfifo((_fifo + ".in").c_str(), 0700) == -1 ) {
			CleanupFIFO();
			throw std::runtime_error("mkfifo in");
		}
		if (mkfifo((_fifo + ".out").c_str(), 0700) == -1) {
			CleanupFIFO();
			throw std::runtime_error("mkfifo out");
		}
		if (mkfifo((_fifo + ".err").c_str(), 0700) == -1) {
			CleanupFIFO();
			throw std::runtime_error("mkfifo err");
		}
	}

	virtual ~ExecCommandFIFO()
	{
		CleanupFIFO();
	}

	inline const std::string &FileName() const { return _fifo; }


	static int sReadStatus(const std::string &fifo)
	{
		int status = -1;
		FDScope fd_status(open((fifo + ".status").c_str(), O_RDONLY));
		if (fd_status.Valid()) {
			ReadAll(fd_status, &status, sizeof(status));
		}

		return status;
	}

	int ReadStatus()
	{
		return sReadStatus(_fifo);
	}

};
