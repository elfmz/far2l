#include "Storage.h"
#include <crc64.h>
#include <fcntl.h>
#include <utils.h>
#include <sudo.h>

namespace Storage
{
	static std::string StoragePath(const std::string &key_file, const std::string &key_string)
	{
		uint64_t crc = crc64(0, (const unsigned char *)key_string.c_str(), key_string.size());
		crc = crc64(crc, (const unsigned char *)key_file.c_str(), key_file.size());
		char buf[128] = {};
		snprintf(buf, sizeof(buf) - 1, "inside/stg/%llx", (unsigned long long)crc);
		return InMyConfig(buf);
	}

	static bool sdc_fd2fd(int fd_src, int fd_dst)
	{
		char buf[0x1000];
		for (;;) {
			ssize_t r = sdc_read(fd_src, buf, sizeof(buf));
			if (r <= 0)
				return true;
			if (sdc_write(fd_dst, buf, (size_t)r) != r)
				return false;
		}
	}

	void Clear(const std::string &key_file, const std::string &key_string)
	{
		const std::string &stg = StoragePath(key_file, key_string);
		sdc_unlink(stg.c_str());
	}

	void Put(const std::string &key_file, const std::string &key_string, const std::string &data_file)
	{
		const std::string &stg = StoragePath(key_file, key_string);

		int fd_data = sdc_open(data_file.c_str(), O_RDONLY);
		if (fd_data == -1)
			return;

		int fd_stg = sdc_open(stg.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0640);
		if (fd_stg != -1) {
			struct stat s = {};
			sdc_stat(key_file.c_str(), &s);
			bool ok = (sdc_write(fd_stg, &s, sizeof(s)) == sizeof(s)
				&& sdc_fd2fd(fd_data, fd_stg));

			sdc_close(fd_stg);
			if (!ok)
				sdc_unlink(stg.c_str());
		}

		sdc_close(fd_data);
	}

	bool Get(const std::string &key_file, const std::string &key_string, const std::string &data_file)
	{
		const std::string &stg = StoragePath(key_file, key_string);
		int fd_stg = sdc_open(stg.c_str(), O_RDONLY);
		if (fd_stg == -1)
			return false;

		bool out = false;

		struct stat s_key = {}, s_recall = {};
		sdc_stat(key_file.c_str(), &s_key);

		if (sdc_read(fd_stg, &s_recall, sizeof(s_recall)) == sizeof(s_recall)
		  && s_key.st_mtime == s_recall.st_mtime && s_key.st_size == s_recall.st_size
		  && s_key.st_dev == s_recall.st_dev && s_key.st_ino == s_recall.st_ino) {
			int fd_data = sdc_open(data_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0640);
			if (fd_data != -1) {
				out = sdc_fd2fd(fd_stg, fd_data);
				sdc_close(fd_data);
			}			
		}

		sdc_close(fd_stg);
		return out;
	}
}
