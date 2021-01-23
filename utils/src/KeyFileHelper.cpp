#include "KeyFileHelper.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <atomic>
#include <stdexcept>
#include <algorithm>

#include <WinCompat.h>
#include <string.h>
#include <mutex>
#include <stdlib.h>

#include "ScopeHelpers.h"
#include "utils.h"


KeyFileHelper::KeyFileHelper(const char *filename, bool load)
	: _filename(filename), _dirty(!load), _loaded(false)
{
	std::string content;

	for (size_t load_attempts = 0;; ++load_attempts) {
		struct stat s {};
		if (stat(_filename.c_str(), &s) == -1) {
			return;
		}

		_filemode = (s.st_mode | 0600) & 0777;

		if (!load) {
			return;
		}

		FDScope fd(open(_filename.c_str(), O_RDONLY, _filemode));
		if (!fd.Valid()) {
			fprintf(stderr, "KeyFileHelper: error=%d opening '%s'\n", errno , _filename.c_str());
			return;
		}

		if (s.st_size == 0) {
			_loaded = true;
			return;
		}

		content.resize(s.st_size);
		ssize_t r = ReadAll(fd, &content[0], content.size());
		if (r == (ssize_t)s.st_size) {
			struct stat s2 {};
			if (stat(_filename.c_str(), &s2) == -1 || s.st_mtime == s2.st_mtime) {
				break;
			}
		}

		if (load_attempts > 1000) {
			fprintf(stderr, "KeyFileHelper: to many attempts to load '%s'\n", _filename.c_str());
			content.resize((r <= 0) ? 0 : (size_t)r);
			break;
		}

		// seems tried to read at the moment when smbd else modifies file
		// usleep random time to effectively avoid long waits on mutual conflicts
		usleep(10000 + 1000 * (rand() % 100));
	}
	
	std::string line, value;
	Values *values = nullptr;
	for (size_t line_start = 0; line_start < content.size();) {
		size_t line_end = content.find('\n', line_start);
		if (line_end == std::string::npos) {
			line_end = content.size();
		}

		line = content.substr(line_start, line_end - line_start);
		StrTrim(line, " \t\r");
		if (!line.empty() && line[0] != ';' && line[0] != '#') {
			if (line[0] == '[' && line[line.size() - 1] == ']') {
				values = &_kf[line.substr(1, line.size() - 2)];

			} else if (values != nullptr) {
				size_t p = line.find('=');
				if (p != std::string::npos) {
					value = line.substr(p + 1);
					StrTrimLeft(value);
					line.resize(p);
					StrTrimRight(line);
					(*values)[line] = value;
				}
			}
		}

		line_start = line_end + 1;
	}

	_loaded = true;
}

KeyFileHelper::~KeyFileHelper()
{
	Save(true);
}

static std::atomic<unsigned int> s_tmp_uniq;
bool KeyFileHelper::Save(bool only_if_dirty)
{
	if (only_if_dirty && !_dirty)
		return true;

	std::string tmp = _filename;
	unsigned int  tmp_uniq = ++s_tmp_uniq;
	tmp+= StrPrintf(".%u-%u", getpid(), tmp_uniq);
	try {
		FDScope fd(creat(tmp.c_str(), _filemode));
		if (!fd.Valid()) {
			throw std::runtime_error("create file failed");
		}

		std::string content;
		std::vector<std::string> keys, sections;
		for (const auto &i_kf : _kf) {
			sections.emplace_back(i_kf.first);
		}
		std::sort(sections.begin(), sections.end());
		for (const auto &s : sections) {
			auto &kmap = _kf[s];
			keys.clear();
			for (const auto &i_kmap : kmap) {
				keys.emplace_back(i_kmap.first);
			}
			std::sort(keys.begin(), keys.end());
			content.append("[").append(s).append("]\n");
			for (const auto &k : keys) {
				content.append(k).append("=").append(kmap[k]).append("\n");
			}
			content.append("\n");
			if (content.size() >= 0x10000) {
				if (WriteAll(fd, content.c_str(), content.size()) != content.size()) {
					throw std::runtime_error("write file failed");
				}
				content.clear();
			}
		}

		if (WriteAll(fd, content.c_str(), content.size()) != content.size()) {
			throw std::runtime_error("write file failed");
		}

	} catch (std::exception &e) {
		fprintf(stderr,
			"KeyFileHelper::Save: exception '%s' errno=%u while saving '%s'\n",
				e.what(), errno, tmp.c_str());

		remove(tmp.c_str());
		return false;
	}

	if (rename(tmp.c_str(), _filename.c_str()) == -1) {
		fprintf(stderr,
			"KeyFileHelper::Save: errno=%u while renaming '%s' -> '%s'\n",
				errno, tmp.c_str(), _filename.c_str());
		remove(tmp.c_str());
		return false;
	}

	_dirty  = false;
	return true;
}

std::vector<std::string> KeyFileHelper::EnumSections()
{
	std::vector<std::string> out;
	out.reserve(_kf.size());
	for (const auto &s : _kf) {
		out.push_back(s.first);
	}
	return out;
}

std::vector<std::string> KeyFileHelper::EnumSectionsAt(const char *parent_section, bool recursed)
{
	std::string prefix = parent_section;
	if (prefix == "/") {
		prefix.clear();

	} else if (!prefix.empty() && prefix.back() != '/') {
		prefix+= '/';
	}

	std::vector<std::string> out;
	for (const auto &s : _kf) {
		if (s.first.size() > prefix.size()
				&& memcmp(s.first.c_str(), prefix.c_str(), prefix.size()) == 0
				&& (recursed || strchr(s.first.c_str() + prefix.size(), '/') == nullptr))  {

			out.push_back(s.first);
		}
	}
	return out;
}

size_t KeyFileHelper::RemoveSection(const char *section)
{
	if (_kf.erase(section) != 0) {
		_dirty = true;
		return 1;
	}
	return 0;
}

size_t KeyFileHelper::RemoveSectionsAt(const char *parent_section)
{
	std::string prefix = parent_section;
	if (prefix == "/") {
		prefix.clear();

	} else if (!prefix.empty() && prefix.back() != '/') {
		prefix+= '/';
	}
	size_t out = 0;

	for (auto it = _kf.begin(); it != _kf.end();) {
		if (it->first.size() > prefix.size()
				&& memcmp(it->first.c_str(), prefix.c_str(), prefix.size()) == 0) {
			it = _kf.erase(it);
			_dirty = true;
			++out;

		} else {
			++it;
		}
	}
	return out;
}

void KeyFileHelper::RemoveKey(const char *section, const char *name)
{
	auto it = _kf.find(section);
	if (it != _kf.end() && it->second.erase(name) != 0) {
		_dirty = true;
//		if (it->second.empty()) {
//			_kf.erase(it);
//		}
	}
}

std::vector<std::string> KeyFileHelper::EnumKeys(const char *section)
{
	std::vector<std::string> out;
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		out.reserve(it->second.size());
		for (const auto &e : it->second) {
			out.emplace_back(e.first);
		}
	}
	return out;
}

bool KeyFileHelper::HasSection(const char *section)
{
	auto it = _kf.find(section);
	return (it != _kf.end());
}

bool KeyFileHelper::HasKey(const char *section, const char *name)
{
	auto it = _kf.find(section);
	if (it == _kf.end()) {
		return false;
	}

	auto s = it->second.find(name);
	return (s != it->second.end());
}

std::string KeyFileHelper::GetString(const char *section, const char *name, const char *def)
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		auto s = it->second.find(name);
		if (s != it->second.end()) {
			return s->second;
		}
	}

	return def ? def : "";
}

std::wstring KeyFileHelper::GetString(const char *section, const char *name, const wchar_t *def)
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		auto s = it->second.find(name);
		if (s != it->second.end()) {
			return StrMB2Wide(s->second);
		}
	}

	return def ? def : L"";
}

void KeyFileHelper::GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def)
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		auto s = it->second.find(name);
		if (s != it->second.end()) {
			strncpy(buffer, s->second.c_str(), buf_size);
			buffer[buf_size - 1] = 0;
			return;
		}
	}

	if (def && def != buffer) {
		strncpy(buffer, def, buf_size);
		buffer[buf_size - 1] = 0;
	} else {
		buffer[0] = 0;
	}
}

int KeyFileHelper::GetInt(const char *section, const char *name, int def)
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		auto s = it->second.find(name);
		if (s != it->second.end()) {
			return atoi(s->second.c_str());
		}
	}

	return def;
}

unsigned int KeyFileHelper::GetUInt(const char *section, const char *name, unsigned int def)
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		auto s = it->second.find(name);
		if (s != it->second.end()) {
			sscanf(s->second.c_str(), "%u", &def);
		}
	}

	return def;
}

///////////////////////////////////////////////
void KeyFileHelper::PutString(const char *section, const char *name, const char *value)
{
	_dirty = true;
	if (!value) {
		value = "";
	}
	_kf[section][name] = value;
}

void KeyFileHelper::PutString(const char *section, const char *name, const wchar_t *value)
{
	_dirty = true;
	if (!value) {
		value = L"";
	}
	_kf[section][name] = Wide2MB(value);
}

void KeyFileHelper::PutInt(const char *section, const char *name, int value)
{
	_dirty = true;
	char tmp[32];
	sprintf(tmp, "%d", value);
	PutString(section, name, tmp);
}

void KeyFileHelper::PutUInt(const char *section, const char *name, unsigned int value)
{
	_dirty = true;
	char tmp[32];
	sprintf(tmp, "%u", value);
	PutString(section, name, tmp);
}

