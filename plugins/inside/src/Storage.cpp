#include "Storage.h"
#include <crc64.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <utils.h>
#include <sudo.h>

namespace Storage
{
#define STORAGE_DIR "plugins/inside/stg"

	static std::string StoragePath(const std::string &key_file, const std::string &key_string)
	{
		uint64_t crc = crc64(0, (const unsigned char *)key_string.c_str(), key_string.size());
		crc = crc64(crc, (const unsigned char *)key_file.c_str(), key_file.size());
		char buf[128] = {};
		snprintf(buf, sizeof(buf) - 1, "%s/%llx", STORAGE_DIR, (unsigned long long)crc);
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

	void Clear()
	{
		std::string stg_path = InMyConfig(STORAGE_DIR);
		DIR *dir = opendir(stg_path.c_str());
		if (!dir)
			return;

		std::vector<std::string> entries;
		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
				entries.emplace_back(de->d_name);
		}
		closedir(dir);

		stg_path+= '/';
		const size_t dir_length = stg_path.size();
		for (const auto &entry : entries) {
			stg_path.resize(dir_length);
			stg_path+= entry;
			if (remove(stg_path.c_str()) == 0) {
				fprintf(stderr, "Inside: removed entry '%s'\n", entry.c_str());
			} else {
				fprintf(stderr, "Inside: error %u removing entry '%s'\n", errno, entry.c_str());
			}
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
			&& s_key.st_dev == s_recall.st_dev && s_key.st_ino == s_recall.st_ino)
		{
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
