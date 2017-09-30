#include "SharedResource.hpp"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <utils.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

SharedResource::SharedResource(uint64_t id) :
	_modify_id(0),
	_modify_counter(0),
	_fd(-1)
{
	char buf[64];
	snprintf(buf, sizeof(buf) - 1, "sr/%llx", (unsigned long long)id);

	const std::string &path = InMyTemp(buf);
	_fd = open(InMyTemp(buf).c_str(), O_CREAT | O_RDWR, 0640);
	if (_fd == -1) {
		perror("SharedResource::SharedResource: open");

	} else if (pread(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
		_modify_id = 0;
	}
}

SharedResource::~SharedResource()
{
	if (_fd != -1)
		close(_fd);
}

bool SharedResource::Lock(int op, int timeout)
{
	if (_fd == -1)
		return false;

	bool out = false;
	if (timeout != -1) {
		for (const time_t ts = time(NULL);;) {
			out = flock(_fd, op | LOCK_NB) != -1;
			if (out)
				break;
			const time_t tn = time(NULL);
			if (tn - ts >= timeout)
				break;
			if (tn - ts > 1) {
				sleep(1);
			} else if (tn - ts == 1) {
				usleep(100000);
			} else
				usleep(10000);
		}
		
	} else {
		out = flock(_fd, op) != -1;
	}
	if (!out) {
		fprintf(stderr, "SharedResource::Lock(%d, %d): err=%d\n", op, timeout, errno);
	}
	return out;
}

bool SharedResource::LockRead(int timeout)
{
	return Lock(LOCK_SH, timeout);
}

bool SharedResource::LockWrite(int timeout)
{
	return Lock(LOCK_EX, timeout);
}

void SharedResource::UnlockRead()
{
	if (_fd != -1) {
		if (pread(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
			perror("SharedResource::UnlockWrite: pread");
			_modify_id = 0;
		}
		flock(_fd, LOCK_UN);
	}
}

void SharedResource::UnlockWrite()
{
	if (_fd != -1) {
		_modify_id = ++_modify_counter;
		_modify_id<<= 32;
		_modify_id^= (uint64_t)getpid();
		if (pwrite(_fd, &_modify_id, sizeof(_modify_id), 0) != sizeof(_modify_id)) {
			perror("SharedResource::UnlockWrite: pwrite");
			_modify_id = 0;
		}
		flock(_fd, LOCK_UN);
	}
}

bool SharedResource::IsModified()
{
	if (_fd == -1)
		return false;

	uint64_t modify_id = 0;
	if (pread(_fd, &modify_id, sizeof(modify_id), 0) != sizeof(modify_id)) {
		perror("SharedResource::IsModified: pread");
		return false;
	}

	return modify_id != _modify_id;
}
