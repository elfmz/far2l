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

// intentionally invalid section name to be used internally by
// KeyFileHelper to tell KeyFileReadHelper don't load anything
static const char *sDontLoadSectionName = "][";


bool KeyFileValues::HasKey(const char *name) const
{
	return find(name) != end();
}

std::string KeyFileValues::GetString(const char *name, const char *def) const
{
	const auto &it = find(name);
	if (it != end()) {
		return it->second;
	}

	return def ? def : "";
}

std::wstring KeyFileValues::GetString(const char *name, const wchar_t *def) const
{
	const auto &it = find(name);
	if (it != end()) {
		return StrMB2Wide(it->second);
	}

	return def ? def : L"";
}

int KeyFileValues::GetInt(const char *name, int def) const
{
	const auto &it = find(name);
	if (it != end()) {
		sscanf(it->second.c_str(), "%d", &def);
	}

	return def;
}

unsigned int KeyFileValues::GetUInt(const char *name, unsigned int def) const
{
	const auto &it = find(name);
	if (it != end()) {
		sscanf(it->second.c_str(), "%u", &def);
	}

	return def;
}

std::vector<std::string> KeyFileValues::EnumKeys() const
{
	std::vector<std::string> out;
	out.reserve(size());
	for (const auto &it : *this) {
		out.emplace_back(it.first);
	}
	return out;
}

///////////////////////////////////////////

template <class ValuesProviderT>
static bool LoadKeyFile(const char *filename, mode_t &filemode, ValuesProviderT values_provider)
{
	std::string content;

	for (size_t load_attempts = 0;; ++load_attempts) {
		struct stat s {};
		if (stat(filename, &s) == -1) {
			return false;
		}

		filemode = (s.st_mode | 0600) & 0777;

		FDScope fd(open(filename, O_RDONLY, filemode));
		if (!fd.Valid()) {
			fprintf(stderr, "%s: error=%d opening '%s'\n", __FUNCTION__, errno, filename);
			return false;
		}

		if (s.st_size == 0) {
			return true;
		}

		content.resize(s.st_size);
		ssize_t r = ReadAll(fd, &content[0], content.size());
		if (r == (ssize_t)s.st_size) {
			struct stat s2 {};
			if (stat(filename, &s2) == -1 || s.st_mtime == s2.st_mtime) {
				break;
			}
		}

		if (load_attempts > 1000) {
			fprintf(stderr, "KeyFileHelper: to many attempts to load '%s'\n", filename);
			content.resize((r <= 0) ? 0 : (size_t)r);
			break;
		}

		// seems tried to read at the moment when smbd else modifies file
		// usleep random time to effectively avoid long waits on mutual conflicts
		usleep(10000 + 1000 * (rand() % 100));
	}
	
	std::string line, value;
	KeyFileValues *values = nullptr;
	for (size_t line_start = 0; line_start < content.size();) {
		size_t line_end = content.find('\n', line_start);
		if (line_end == std::string::npos) {
			line_end = content.size();
		}

		line = content.substr(line_start, line_end - line_start);
		StrTrim(line, " \t\r");
		if (!line.empty() && line[0] != ';' && line[0] != '#') {
			if (line[0] == '[' && line[line.size() - 1] == ']') {
				values = values_provider(line.substr(1, line.size() - 2));

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

	return true;
}

///////////////////////////////////////////

KeyFileReadSection::KeyFileReadSection(const char *filename, const char *section)
	:
	_section_loaded(false)
{
	mode_t filemode;
	LoadKeyFile(filename, filemode,
		[&] (const std::string &section_name)->KeyFileValues *
		{
			if (section_name == section) {
				_section_loaded = true;
				return this;
			}

			return nullptr;
		}
	);
}

///////////////////////////////////

KeyFileReadHelper::KeyFileReadHelper(const char *filename, const char *load_only_section)
	: _loaded(false)
{
	_loaded = LoadKeyFile(filename, _filemode,
		[&] (const std::string &section_name)->KeyFileValues *
		{
			if (load_only_section == nullptr || section_name == load_only_section) {
				return &_kf[section_name];
			}

			// check may be its a nested section name
			size_t starts_len = StrStartsFrom(section_name, load_only_section);
			if (starts_len && section_name[starts_len] == '/') {
				// load also nested sections
				return &_kf[section_name];
			}

			return nullptr;
		}
	);
}

std::vector<std::string> KeyFileReadHelper::EnumSections() const
{
	std::vector<std::string> out;
	out.reserve(_kf.size());
	for (const auto &s : _kf) {
		out.push_back(s.first);
	}
	return out;
}

std::vector<std::string> KeyFileReadHelper::EnumSectionsAt(const char *parent_section, bool recursed) const
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

std::vector<std::string> KeyFileReadHelper::EnumKeys(const char *section) const
{
	std::vector<std::string> out;
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.EnumKeys();
	}
	return std::vector<std::string>();
}

bool KeyFileReadHelper::HasSection(const char *section) const
{
	auto it = _kf.find(section);
	return (it != _kf.end());
}

bool KeyFileReadHelper::HasKey(const char *section, const char *name) const
{
	auto it = _kf.find(section);
	if (it == _kf.end()) {
		return false;
	}

	return it->second.HasKey(name);
}

std::string KeyFileReadHelper::GetString(const char *section, const char *name, const char *def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetString(name, def);
	}

	return def ? def : "";
}

std::wstring KeyFileReadHelper::GetString(const char *section, const char *name, const wchar_t *def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetString(name, def);
	}

	return def ? def : L"";
}

void KeyFileReadHelper::GetChars(char *buffer, size_t buf_size, const char *section, const char *name, const char *def) const
{
	const std::string &out = GetString(section, name, def);
	strncpy(buffer, out.c_str(), buf_size);
	buffer[buf_size - 1] = 0;
}

int KeyFileReadHelper::GetInt(const char *section, const char *name, int def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetInt(name, def);
	}

	return def;
}

unsigned int KeyFileReadHelper::GetUInt(const char *section, const char *name, unsigned int def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetUInt(name, def);
	}

	return def;
}



/////////////////////////////////////////////////////////////
KeyFileHelper::KeyFileHelper(const char *filename, bool load)
	:
	KeyFileReadHelper(filename, load ? nullptr : sDontLoadSectionName),
	_filename(filename),
	_dirty(!load)
{
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

bool KeyFileHelper::RemoveSection(const char *section)
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

