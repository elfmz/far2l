#include "KeyFileHelper.h"
#include <fcntl.h>
#include <stdlib.h>
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

class KFEscaping
{
	std::string _result;

	void Encode(const std::string &s, bool key)
	{
		_result = '\"';
		for (const auto &c : s) switch (c) {
			case '\r': _result+= "\\r"; break;
			case '\n': _result+= "\\n"; break;
			case '\t': _result+= "\\t"; break;
			case '\\': _result+= "\\\\"; break;
			default: if (key && c == '=') {
				_result+= "\\E";
			} else {
				_result+= c;
			}
		}
		_result+= '\"';
	}

public:
	const std::string &EncodeSection(const std::string &s)
	{
		if (s.empty() || ( (s.front() != '\"' || s.back() != '\"')
				&& s.find_first_of("\r\n") == std::string::npos)) {
			return s;
		}
//fprintf(stderr, "%s: '%s'\n", __FUNCTION__, s.c_str());
		Encode(s, false);
		return _result;
	}

	const std::string &EncodeKey(const std::string &s)
	{
		if (s.empty() || ( (s.front() != '\"' || s.back() != '\"')
				&& s.find_first_of("\r\n=") == std::string::npos
				&& s.front() != ';' && s.front() != '#' && s.front() != '['
				&& s.front() != ' ' && s.front() != '\t'
				&& s.back() != ' ' && s.back() != '\t')) {
			return s;
		}
//fprintf(stderr, "%s: '%s'\n", __FUNCTION__, s.c_str());
		Encode(s, true);
		return _result;
	}

	const std::string &EncodeValue(const std::string &s)
	{
		if (s.empty() || ( (s.front() != '\"' || s.back() != '\"')
				&& s.find_first_of("\r\n") == std::string::npos
				&& s.front() != ' ' && s.front() != '\t'
				&& s.back() != ' ' && s.back() != '\t')) {
			return s;
		}
//fprintf(stderr, "%s: '%s'\n", __FUNCTION__, s.c_str());
		Encode(s, false);
		return _result;
	}

	const std::string &Decode(const std::string &s)
	{
		if (s.size() < 2 || s.front() != '\"' || s.back() != '\"') {
			return s;
		}

		_result.clear();

		for (size_t i = 1; i < s.size() - 1; ++i) {
			if (s[i] == '\\') {
				++i;
				switch (s[i]) {
					case 'E': _result+= '='; break;
					case 'r': _result+= '\r'; break;
					case 'n': _result+= '\n'; break;
					case 't': _result+= '\t'; break;
					case '\\': _result+= '\\'; break;
					default: // WTF???
						fprintf(stderr,
							"%s: bad escape sequence in '%s' at %ld\n",
							__FUNCTION__, s.c_str(), (unsigned long)i);
						_result+= '\\';
						_result+= s[i];
				}
			} else {
				_result+= s[i];
			}
		}
		return _result;
	}
};

////////////////////////////////////////////////////////////////



bool KeyFileValues::HasKey(const std::string &name) const
{
	return find(name) != end();
}

std::string KeyFileValues::GetString(const std::string &name, const char *def) const
{
	const auto &it = find(name);
	if (it != end()) {
		return it->second;
	}

	return def ? def : "";
}

std::wstring KeyFileValues::GetString(const std::string &name, const wchar_t *def) const
{
	const auto &it = find(name);
	if (it != end()) {
		return StrMB2Wide(it->second);
	}

	return def ? def : L"";
}

int KeyFileValues::GetInt(const std::string &name, int def) const
{
	const auto &it = find(name);
	if (it != end()) {
		const char *sz = it->second.c_str();
		if (sz[0] == '0' && sz[1] == 'x') {
			return (int)strtol(sz + 2, nullptr, 16);
		} else {
			return (int)strtol(sz, nullptr, 10);
		}
	}

	return def;
}

unsigned int KeyFileValues::GetUInt(const std::string &name, unsigned int def) const
{
	const auto &it = find(name);
	if (it != end()) {
		const char *sz = it->second.c_str();
		if (sz[0] == '0' && sz[1] == 'x') {
			return (unsigned int)strtoul(sz + 2, nullptr, 16);
		} else {
			return (unsigned int)strtoul(sz, nullptr, 10);
		}
	}

	return def;
}

unsigned long long KeyFileValues::GetULL(const std::string &name, unsigned long long def) const
{
	const auto &it = find(name);
	if (it != end()) {
		const char *sz = it->second.c_str();
		if (sz[0] == '0' && sz[1] == 'x') {
			return strtoull(sz + 2, nullptr, 16);
		} else {
			return strtoull(sz, nullptr, 10);
		}
	}

	return def;
}


static size_t DecodeBytes(const std::string &str, size_t len, unsigned char *buf)
{
	size_t out;

	for (size_t i = out = 0; out != len && i != str.size(); ++out) {
		buf[out] = (digit_htob(str[i]) << 4);
		do {
			++i;
		} while (i != str.size() && (str[i] == ' ' || str[i] == '\t'));
		if (i == str.size()) {
			break;
		}
		buf[out]|= digit_htob(str[i]);
		do {
			++i;
		} while (i != str.size() && (str[i] == ' ' || str[i] == '\t'));
 	}

	return out;
}

size_t KeyFileValues::GetBytes(const std::string &name, size_t len, unsigned char *buf, const unsigned char *def) const
{
	const auto &it = find(name);
	if (it == end()) {
		if (def) {
			memcpy(buf, def, len);
			return len;
		}
		return 0;
	}

	return DecodeBytes(it->second, len, buf);
}

bool KeyFileValues::GetBytes(const std::string &name, std::vector<unsigned char> &out) const
{
	const auto &it = find(name);
	if (it == end()) {
		return false;
	}

	out.resize(it->second.size() / 2 + 1);
	size_t actual_size = DecodeBytes(it->second, out.size(), &out[0]);
    out.resize(actual_size);
	return true;
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

static inline mode_t MakeFileMode(struct stat &filestat)
{
	return (filestat.st_mode | 0600) & 0777;
}

template <class ValuesProviderT>
static bool LoadKeyFile(const std::string &filename, struct stat &filestat, ValuesProviderT values_provider)
{
	std::string content;

	for (size_t load_attempts = 0;; ++load_attempts) {
		if (stat(filename.c_str(), &filestat) == -1) {
			return false;
		}

		FDScope fd(open(filename.c_str(), O_RDONLY, MakeFileMode(filestat)));
		if (!fd.Valid()) {
			fprintf(stderr, "%s: error=%d opening '%s'\n", __FUNCTION__, errno, filename.c_str());
			return false;
		}

		if (filestat.st_size == 0) {
			return true;
		}

		content.resize(filestat.st_size);
		ssize_t r = ReadAll(fd, &content[0], content.size());
		if (r == (ssize_t)filestat.st_size) {
			struct stat s2 {};
			if (stat(filename.c_str(), &s2) == -1
				|| (filestat.st_mtime == s2.st_mtime
					&& filestat.st_ino == s2.st_ino
					&& filestat.st_size == s2.st_size)) {
				break;
			}
		}

		if (load_attempts > 1000) {
			fprintf(stderr, "KeyFileHelper: to many attempts to load '%s'\n", filename.c_str());
			content.resize((r <= 0) ? 0 : (size_t)r);
			break;
		}

		// seems tried to read at the moment when smbd else modifies file
		// usleep random time to effectively avoid long waits on mutual conflicts
		usleep(10000 + 1000 * (rand() % 100));
	}

	KFEscaping esc, esc_val;
	std::string line, value;
	KeyFileValues *values = nullptr;
	for (size_t line_start = 0; line_start < content.size();) {
		size_t line_end = content.find('\n', line_start);
		if (line_end == std::string::npos) {
			line_end = content.size();
		}

		line = content.substr(line_start, line_end - line_start);
		StrTrim(line, " \t\r");
		if (!line.empty()) {
			if (line.front() == '[') {
				if (line.back() == ']') {
					const auto &section = line.substr(1, line.size() - 2);
					values = values_provider(esc.Decode(section));

				} else {
					fprintf(stderr,
						"%s: leading section marker without trailing - '%s'\n",
						__FUNCTION__, line.c_str());
				}

			} else if (line.front() != ';' && line.front() != '#' && values != nullptr) {
				size_t p = line.find('=');
				if (p != std::string::npos) {
					value = line.substr(p + 1);
					StrTrimLeft(value);
					line.resize(p);
					StrTrimRight(line);
					// dedicated escaping for values to be used in single expression with esc
					(*values)[esc.Decode(line)] = esc_val.Decode(value);
				}
			}
		}

		line_start = line_end + 1;
	}

	return true;
}

///////////////////////////////////////////

KeyFileReadSection::KeyFileReadSection(const std::string &filename, const std::string &section)
	:
	_section_loaded(false)
{
	struct stat filestat{};
	LoadKeyFile(filename, filestat,
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

KeyFileReadHelper::KeyFileReadHelper(const std::string &filename, const char *load_only_section)
	: _loaded(false)
{
	_loaded = LoadKeyFile(filename, _filestat,
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

std::vector<std::string> KeyFileReadHelper::EnumSectionsAt(const std::string &parent_section, bool recursed) const
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

std::vector<std::string> KeyFileReadHelper::EnumKeys(const std::string &section) const
{
	std::vector<std::string> out;
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.EnumKeys();
	}
	return std::vector<std::string>();
}

size_t KeyFileReadHelper::SectionsCount() const
{
	return _kf.size();
}

bool KeyFileReadHelper::HasSection(const std::string &section) const
{
	auto it = _kf.find(section);
	return (it != _kf.end());
}

const KeyFileValues *KeyFileReadHelper::GetSectionValues(const std::string &section) const
{
	auto it = _kf.find(section);
	return (it != _kf.end()) ? &it->second : nullptr;
}

bool KeyFileReadHelper::HasKey(const std::string &section, const std::string &name) const
{
	auto it = _kf.find(section);
	if (it == _kf.end()) {
		return false;
	}

	return it->second.HasKey(name);
}

std::string KeyFileReadHelper::GetString(const std::string &section, const std::string &name, const char *def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetString(name, def);
	}

	return def ? def : "";
}

std::wstring KeyFileReadHelper::GetString(const std::string &section, const std::string &name, const wchar_t *def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetString(name, def);
	}

	return def ? def : L"";
}

void KeyFileReadHelper::GetChars(char *buffer, size_t buf_size, const std::string &section, const std::string &name, const char *def) const
{
	const std::string &out = GetString(section, name, def);
	strncpy(buffer, out.c_str(), buf_size);
	buffer[buf_size - 1] = 0;
}

int KeyFileReadHelper::GetInt(const std::string &section, const std::string &name, int def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetInt(name, def);
	}

	return def;
}

unsigned int KeyFileReadHelper::GetUInt(const std::string &section, const std::string &name, unsigned int def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetUInt(name, def);
	}

	return def;
}

unsigned long long KeyFileReadHelper::GetULL(const std::string &section, const std::string &name, unsigned long long def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetULL(name, def);
	}

	return def;
}

size_t KeyFileReadHelper::GetBytes(const std::string &section, const std::string &name, size_t len, unsigned char *buf, const unsigned char *def) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetBytes(name, len, buf);

	} else if (def) {
		memcpy(buf, def, len);
		return len;
	}

	return 0;
}

bool KeyFileReadHelper::GetBytes(const std::string &section, const std::string &name, std::vector<unsigned char> &out) const
{
	auto it = _kf.find(section);
	if (it != _kf.end()) {
		return it->second.GetBytes(name, out);
	}

	return false;
}


/////////////////////////////////////////////////////////////
KeyFileHelper::KeyFileHelper(const std::string &filename, bool load)
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
		FDScope fd(creat(tmp.c_str(), MakeFileMode(_filestat)));
		if (!fd.Valid()) {
			throw std::runtime_error("create file failed");
		}

		std::string content;
		std::vector<std::string> keys, sections;
		for (const auto &i_kf : _kf) {
			sections.emplace_back(i_kf.first);
		}
		std::sort(sections.begin(), sections.end());
		KFEscaping esc;
		for (const auto &s : sections) {
			auto &kmap = _kf[s];
			keys.clear();
			for (const auto &i_kmap : kmap) {
				keys.emplace_back(i_kmap.first);
			}
			std::sort(keys.begin(), keys.end());

			content+= '[';
			content+= esc.EncodeSection(s);
			content+= "]\n";

			for (const auto &k : keys) {
				content+= esc.EncodeKey(k);
				content+= '=';
				content+= esc.EncodeValue(kmap[k]);
				content+= '\n';
			}
			content+= '\n';

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

	_dirty = false;
	return true;
}

bool KeyFileHelper::RemoveSection(const std::string &section)
{
	if (_kf.erase(section) != 0) {
		_dirty = true;
		return 1;
	}
	return 0;
}

size_t KeyFileHelper::RemoveSectionsAt(const std::string &parent_section)
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

void KeyFileHelper::RemoveKey(const std::string &section, const std::string &name)
{
	auto it = _kf.find(section);
	if (it != _kf.end() && it->second.erase(name) != 0) {
		_dirty = true;
//		if (it->second.empty()) {
//			_kf.erase(it);
//		}
	}
}

void KeyFileHelper::PutRaw(const std::string &section, const std::string &name, const char *value)
{
	auto &s = _kf[section];

	auto it = s.find(name);
	if (it != s.end()) {
		if (it->second == value) {
			return;
		}
		it->second = value;

	} else {
		s.emplace(name, value);
	}

	_dirty = true;
}

void KeyFileHelper::PutString(const std::string &section, const std::string &name, const char *value)
{
	if (!value) {
		value = "";
	}
	PutRaw(section, name, value);
}

void KeyFileHelper::PutString(const std::string &section, const std::string &name, const wchar_t *value)
{
	if (!value) {
		value = L"";
	}
	PutRaw(section, name, Wide2MB(value).c_str());
}

void KeyFileHelper::PutInt(const std::string &section, const std::string &name, int value)
{
	char tmp[32];
	sprintf(tmp, "%d", value);
	PutRaw(section, name, tmp);
}

void KeyFileHelper::PutUInt(const std::string &section, const std::string &name, unsigned int value)
{
	char tmp[32];
	sprintf(tmp, "%u", value);
	PutRaw(section, name, tmp);
}

void KeyFileHelper::PutUIntAsHex(const std::string &section, const std::string &name, unsigned int value)
{
	char tmp[32];
	sprintf(tmp, "0x%x", value);
	PutRaw(section, name, tmp);
}

void KeyFileHelper::PutULL(const std::string &section, const std::string &name, unsigned long long value)
{
	char tmp[64];
	sprintf(tmp, "%llu", value);
	PutRaw(section, name, tmp);
}

void KeyFileHelper::PutULLAsHex(const std::string &section, const std::string &name, unsigned long long value)
{
	char tmp[64];
	sprintf(tmp, "0x%llx", value);
	PutRaw(section, name, tmp);
}

void KeyFileHelper::PutBytes(const std::string &section, const std::string &name, size_t len, const unsigned char *buf, bool spaced)
{
	const size_t slen = (len * 2 + ((spaced && len > 1) ? len - 1 : 0));
	std::string &v = _kf[section][name];
	if (slen != v.size()) {
		v.resize(slen);
		_dirty = true;
	}

	for (size_t i = 0, j = 0; i != len; ++i) {
		if (spaced && i != 0) {
			if (v[j] != ' ') {
				v[j] = ' ';
				_dirty = true;
			}
			++j;
		}

		char c = digit_btoh(buf[i] >> 4);
		if (v[j] != c) {
			v[j] = c;
			_dirty = true;
		}
		++j;

		c = digit_btoh(buf[i] & 0xf);
		if (v[j] != c) {
			v[j] = c;
			_dirty = true;
		}
		++j;
	}
}

void KeyFileHelper::RenameSection(const std::string &src, const std::string &dst, bool recursed)
{
	auto it = _kf.find(src);
	if (it != _kf.end()) {
		auto section_values = it->second;
		_kf.erase(it);
		_kf[dst] = section_values;
		_dirty = true;
	}

	if (recursed) {
		const auto subsections = EnumSectionsAt(src, true);
		for (auto subsection : subsections) {
			auto section_values = _kf[subsection];
			_kf.erase(subsection);
			subsection.replace(0, src.size(), dst);
			_kf[subsection] = section_values;
			_dirty = true;
		}
	}
}
