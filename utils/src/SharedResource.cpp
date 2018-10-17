#include "SharedResource.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <utils.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

SharedResource::SharedResource(const char *group, uint64_t id) :
	_modify_id(0),
	_modify_counter(0),
	_fd(-1)
{
	char buf[128];
	snprintf(buf, sizeof(buf) - 1, "sr/%s/%llx", group, (unsigned long long)id);

	_fd = open(InMyConfig(buf).c_str(), O_CREAT | O_RDWR, 0640);
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

void SharedResource::GenerateModifyId()
{
	++_modify_counter;
	_modify_id = (uint64_t)(((uintptr_t)this) & 0xffff000) << 12;
	_modify_id^= getpid();
	_modify_id<<= 32;
	_modify_id^= (uint64_t)time(NULL);
	_modify_id+= _modify_counter;
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
			if (tn - ts >= timeout) {
				fprintf(stderr,
					"SharedResource::Lock(%d, %d): timeout\n", op, timeout);
				break;
			}
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
			perror("SharedResource::UnlockRead: pread");
			_modify_id = 0;
		}
		flock(_fd, LOCK_UN);
	}
}

void SharedResource::UnlockWrite()
{
	if (_fd != -1) {
		GenerateModifyId();
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
//		perror("SharedResource::IsModified: pread");
		return false;
	}

	return modify_id != _modify_id;
}
