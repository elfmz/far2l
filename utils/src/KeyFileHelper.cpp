#include "KeyFileHelper.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <algorithm>

#include <string.h>
#include <stdlib.h>
#include <os_call.hpp>

#include "ScopeHelpers.h"
#include "utils.h"

// pseudo section name literal to be used internally by
// KeyFileHelper to tell KeyFileReadHelper don't load anything
static char sDontLoadLiteral[] = "][";

bool FN_NOINLINE KeyFileCmp::operator()(const std::string& a, const std::string& b) const
{
	const char *pa = a.c_str();
	const char *pb = b.c_str();
	for (size_t i = 0;; ++i) {
		if (UNLIKELY(i == a.size() || i == b.size())) {
			return a.size() < b.size();
		}
		auto ca = pa[i];
		auto cb = pb[i];
		if (ca != cb) {
			if (!_case_insensitive) {
				return ca < cb;
			}
			if (ca >= 'a' && ca <= 'z') {
				ca-= 'a' - 'A';
			}
			if (cb >= 'a' && cb <= 'z') {
				cb-= 'a' - 'A';
			}
			if (ca != cb) {
				return ca < cb;
			}
		}
	}

	return false;
}

static FN_NOINLINE KeyFileValues::const_iterator FindValue(const KeyFileValues *values, const std::string &name)
{ // stub to avoid excessive inlining
	return values->find(name);
}

static FN_NOINLINE KeyFileValues::iterator FindValue(KeyFileValues *values, const std::string &name)
{ // stub to avoid excessive inlining
	return values->find(name);
}

////
FN_NOINLINE KeyFileReadHelper::Sections::Sections(bool case_insensitive) : _map(KeyFileCmp(case_insensitive))
{
}

KeyFileReadHelper::Sections::Map::const_iterator FN_NOINLINE KeyFileReadHelper::Sections::FindInMap(const std::string &name) const
{ // stub to avoid excessive inlining
	return _map.find(name);
}

KeyFileReadHelper::Sections::Map::iterator FN_NOINLINE KeyFileReadHelper::Sections::FindInMap(const std::string &name)
{ // stub to avoid excessive inlining
	return _map.find(name);
}

const KeyFileValues * FN_NOINLINE KeyFileReadHelper::Sections::Find(const std::string &section) const
{
	auto it = FindInMap(section);
	return (it == _map.end()) ? nullptr : &it->second;
}

KeyFileValues * FN_NOINLINE KeyFileReadHelper::Sections::Find(const std::string &section)
{
	auto it = FindInMap(section);
	return (it == _map.end()) ? nullptr : &it->second;
}

KeyFileValues * FN_NOINLINE KeyFileReadHelper::Sections::Ensure(const std::string &section)
{
	auto ir = _map.emplace(section, KeyFileValues());
	if (ir.second) {
		if (_ordered.empty()) {
			_ordered.reserve(16);
		}
		_ordered.emplace_back(ir.first);
	}
	return &ir.first->second;
}

KeyFileReadHelper::Sections::IteratorsVec::iterator FN_NOINLINE KeyFileReadHelper::Sections::FindInVec(Map::iterator it)
{
	auto oit = std::find(_ordered.begin(), _ordered.end(), it);
	assert(oit != _ordered.end());
	return oit;
}

bool KeyFileReadHelper::Sections::Remove(const std::string &section)
{
	auto it = FindInMap(section);
	if (it == _map.end()) {
		return false;
	}

	auto oit = FindInVec(it);
	_ordered.erase(oit);
	_map.erase(it);
	return true;
}

size_t KeyFileReadHelper::Sections::RemoveAt(const std::string &section)
{
	size_t out = 0;
	for (auto it = _map.begin(); it != _map.end();) {
		if (it->first.size() > section.size()
				&& memcmp(it->first.c_str(), section.c_str(), section.size()) == 0) {
			auto oit = FindInVec(it);
			_ordered.erase(oit);
			it = _map.erase(it);
			++out;

		} else {
			++it;
		}
	}
	return out;

}

bool FN_NOINLINE KeyFileReadHelper::Sections::Rename(const std::string &section, const std::string &new_name)
{
	auto it = FindInMap(section);
	if (it == _map.end())
		return false;

//	if (section == new_name)
//		return true;

	auto oit = FindInVec(it);

	auto ir = _map.emplace(new_name, KeyFileValues());
	*oit = ir.first;
	ir.first->second = std::move(it->second);
	_map.erase(it);
	return true;
}

const KeyFileReadHelper::Sections::IteratorsVec &KeyFileReadHelper::Sections::Enumeration() const
{
	return _ordered;
}

////

class KFEscaping
{
	std::string _result;

	void Encode(const std::string &s, bool key_name)
	{
		_result = '\"';
		for (const auto &c : s) switch (c) {
			case '\r': _result+= "\\r"; break;
			case '\n': _result+= "\\n"; break;
			case '\t': _result+= "\\t"; break;
			case 0: _result+= "\\0"; break;
			case '\\': _result+= "\\\\"; break;
			default: if (c == '=' && key_name) {
				_result+= "\\E";
			} else {
				_result+= c;
			}
		}
		_result+= '\"';
	}

	bool ContainsEscRequiringChars(const std::string &s, bool key_name)
	{
		for (const auto &c : s) {
			if (c == '\r' || c == '\n' || c == 0 || (key_name && c == '=')) {
				return true;
			}
		}

		return false;
	}

public:
	const std::string &EncodeSection(const std::string &s)
	{
		if (s.empty() || ( (s.front() != '\"' || s.back() != '\"')
				&& !ContainsEscRequiringChars(s, false))) {
			return s;
		}
//fprintf(stderr, "%s: '%s'\n", __FUNCTION__, s.c_str());
		Encode(s, false);
		return _result;
	}

	const std::string &EncodeKey(const std::string &s)
	{
		if (s.empty() || ( (s.front() != '\"' || s.back() != '\"')
				&& !ContainsEscRequiringChars(s, true)
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
				&& !ContainsEscRequiringChars(s, false)
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
					case 'E': _result.append(1, '='); break;
					case 'r': _result.append(1, '\r'); break;
					case 'n': _result.append(1, '\n'); break;
					case 't': _result.append(1, '\t'); break;
					case '0': _result.append(1, 0); break;
					case '\\': _result.append(1, '\\'); break;
					default: // WTF???
						fprintf(stderr,
							"%s: bad escape sequence in '%s' at %ld\n",
							__FUNCTION__, s.c_str(), (unsigned long)i);
						_result.append(1, '\\');
						_result.append(1, s[i]);
				}
			} else {
				_result+= s[i];
			}
		}
		return _result;
	}
};

////////////////////////////////////////////////////////////////


KeyFileValues::KeyFileValues(bool case_insensitive)
	: std::map<std::string, std::string, KeyFileCmp>(KeyFileCmp(case_insensitive))
{
}

bool KeyFileValues::HasKey(const std::string &name) const
{
	return FindValue(this, name) != end();
}

std::string KeyFileValues::GetString(const std::string &name, const char *def) const
{
	const auto &it = FindValue(this, name);
	if (it != end()) {
		return it->second;
	}

	return def ? def : "";
}

std::wstring KeyFileValues::GetString(const std::string &name, const wchar_t *def) const
{
	const auto &it = FindValue(this, name);
	if (it != end()) {
		return StrMB2Wide(it->second);
	}

	return def ? def : L"";
}

void KeyFileValues::GetChars(char *buffer, size_t maxchars, const std::string &name, const char *def) const
{
	const std::string &out = GetString(name, def);
	strncpy(buffer, out.c_str(), maxchars);
	buffer[maxchars - 1] = 0;
}

void KeyFileValues::GetChars(wchar_t *buffer, size_t maxchars, const std::string &name, const wchar_t *def) const
{
	const std::wstring &out = GetString(name, def);
	wcsncpy(buffer, out.c_str(), maxchars);
	buffer[maxchars - 1] = 0;
}

int KeyFileValues::GetInt(const std::string &name, int def) const
{
	const auto &it = FindValue(this, name);
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
	const auto &it = FindValue(this, name);
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
	const auto &it = FindValue(this, name);
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


static size_t DecodeBytes(unsigned char *out, size_t len, const std::string &str)
{
	size_t cnt;

	for (size_t i = cnt = 0; cnt != len && i != str.size(); ++cnt) {
		out[cnt] = (ParseHexDigit(str[i]) << 4);
		do {
			++i;
		} while (i != str.size() && (str[i] == ' ' || str[i] == '\t'));
		if (i == str.size()) {
			break;
		}
		out[cnt]|= ParseHexDigit(str[i]);
		do {
			++i;
		} while (i != str.size() && (str[i] == ' ' || str[i] == '\t'));
 	}

	return cnt;
}

size_t KeyFileValues::GetBytes(unsigned char *out, size_t len, const std::string &name, const unsigned char *def) const
{
	const auto &it = FindValue(this, name);
	if (it == end()) {
		if (def) {
			memcpy(out, def, len);
			return len;
		}
		return 0;
	}

	return DecodeBytes(out, len, it->second);
}

bool KeyFileValues::GetBytes(std::vector<unsigned char> &out, const std::string &name) const
{
	const auto &it = FindValue(this, name);
	if (it == end()) {
		return false;
	}

	out.resize(it->second.size() / 2 + 1);
	size_t actual_size = DecodeBytes(&out[0], out.size(), it->second);
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

static bool LoadKeyFileContent(const std::string &filename, struct stat &filestat, std::string &content)
{
	for (size_t load_attempts = 0;; ++load_attempts) {
		if (stat(filename.c_str(), &filestat) == -1) {
			return false;
		}

		FDScope fd(filename.c_str(), O_RDONLY | O_CLOEXEC);
		if (!fd.Valid()) {
			fprintf(stderr, "%s: error=%d opening '%s'\n", __FUNCTION__, errno, filename.c_str());
			return false;
		}

		content.resize(filestat.st_size);

		if (filestat.st_size == 0) {
			break;
		}

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

	return true;
}

template <class ValuesProviderT>
	static bool LoadKeyFile(const std::string &filename, struct stat &filestat, ValuesProviderT values_provider)
{
	std::string content;
	if (!LoadKeyFileContent(filename, filestat,content)) {
		return false;
	}

	KFEscaping esc, esc_val;
	std::string line, value;
	KeyFileValues *values = nullptr;
	bool reached_named_section = false;
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
					reached_named_section = true;
					const auto &section = line.substr(1, line.size() - 2);
					values = values_provider(esc.Decode(section));

				} else {
					fprintf(stderr,
						"%s: leading section marker without trailing - '%s'\n",
						__FUNCTION__, line.c_str());
				}

			} else if (line.front() != ';' && line.front() != '#') {
				size_t p = line.find('=');
				if (p != std::string::npos) {
					value = line.substr(p + 1);
					StrTrimLeft(value);
					line.resize(p);
					StrTrimRight(line);
					if (!values && !reached_named_section) {
						values = values_provider(std::string());
					}
					if (values) {
						// dedicated escaping for values to be used in single expression with esc
						(*values)[esc.Decode(line)] = esc_val.Decode(value);
					}
				}
			}
		}

		line_start = line_end + 1;
	}

	return true;
}

///////////////////////////////////////////

KeyFileReadSection::KeyFileReadSection(const std::string &filename, const std::string &section, bool case_insensitive)
	:
	_section_loaded(false)
{
	struct stat filestat{};
	LoadKeyFile(filename, filestat,
		[&] (const std::string &section_name)->KeyFileValues *
		{
			if (section_name == section || (case_insensitive && CaseIgnoreEngStrMatch(section_name, section))) {
				_section_loaded = true;
				return this;
			}

			return nullptr;
		}
	);
}

///////////////////////////////////

KeyFileReadHelper::KeyFileReadHelper(const std::string &filename, const char *load_only_section, bool case_insensitive)
	: _kf(case_insensitive), _case_insensitive(case_insensitive)
{
	// intentionally comparing pointer values
	if (load_only_section == &sDontLoadLiteral[0]) {
		return;
	}

	_loaded = LoadKeyFile(filename, _filestat,
		[&] (const std::string &section_name)->KeyFileValues *
		{
			if (load_only_section == nullptr || section_name == load_only_section
					|| (case_insensitive && CaseIgnoreEngStrMatch(section_name, load_only_section))) {
				return _kf.Ensure(section_name);
			}

			// check may be its a nested section name
			size_t starts_len = StrStartsFrom(section_name, load_only_section);
			if (starts_len && section_name[starts_len] == '/') {
				// load also nested sections
				return _kf.Ensure(section_name);
			}

			return nullptr;
		}
	);

	if (!_loaded) {
		ZeroFill(_filestat);
	}
}

std::vector<std::string> KeyFileReadHelper::EnumSections() const
{
	std::vector<std::string> out;
	const auto &vec = _kf.Enumeration();
	out.reserve(vec.size());
	for (const auto &s : vec) {
		out.push_back(s->first);
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
	for (const auto &s : _kf.Enumeration()) {
		if (s->first.size() > prefix.size()
				&& ( (!_case_insensitive && memcmp(s->first.c_str(), prefix.c_str(), prefix.size()) == 0) ||
					(_case_insensitive && CaseIgnoreEngStrMatch(s->first.c_str(), prefix.c_str(), prefix.size())) )
				&& (recursed || strchr(s->first.c_str() + prefix.size(), '/') == nullptr))  {

			out.push_back(s->first);
		}
	}

	return out;
}

std::vector<std::string> KeyFileReadHelper::EnumKeys(const std::string &section) const
{
	const auto *values = _kf.Find(section);
	if (values) {
		return values->EnumKeys();
	}
	return std::vector<std::string>();
}

size_t KeyFileReadHelper::SectionsCount() const
{
	return _kf.Size();
}

bool KeyFileReadHelper::HasSection(const std::string &section) const
{
	return _kf.Find(section) != nullptr;
}

const KeyFileValues *KeyFileReadHelper::GetSectionValues(const std::string &section) const
{
	return _kf.Find(section);
}

bool KeyFileReadHelper::HasKey(const std::string &section, const std::string &name) const
{
	const auto *vals = _kf.Find(section);

	return vals && vals->HasKey(name);
}

std::string KeyFileReadHelper::GetString(const std::string &section, const std::string &name, const char *def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetString(name, def);
	}

	return def ? def : "";
}

std::wstring KeyFileReadHelper::GetString(const std::string &section, const std::string &name, const wchar_t *def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetString(name, def);
	}

	return def ? def : L"";
}

int KeyFileReadHelper::GetInt(const std::string &section, const std::string &name, int def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetInt(name, def);
	}

	return def;
}

unsigned int KeyFileReadHelper::GetUInt(const std::string &section, const std::string &name, unsigned int def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetUInt(name, def);
	}

	return def;
}

unsigned long long KeyFileReadHelper::GetULL(const std::string &section, const std::string &name, unsigned long long def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetULL(name, def);
	}

	return def;
}

size_t KeyFileReadHelper::GetBytes(unsigned char *buf, size_t len, const std::string &section, const std::string &name, const unsigned char *def) const
{
	const auto *vals = _kf.Find(section);
	if (vals) {
		return vals->GetBytes(buf, len, name);
	}

	if (def) {
		memcpy(buf, def, len);
		return len;
	}

	return 0;
}

bool KeyFileReadHelper::GetBytes(std::vector<unsigned char> &out, const std::string &section, const std::string &name) const
{
	const auto *vals = _kf.Find(section);
	return vals && vals->GetBytes(out, name);
}


/////////////////////////////////////////////////////////////
KeyFileHelper::KeyFileHelper(const std::string &filename, bool load, bool case_insensitive)
	:
	KeyFileReadHelper(filename, load ? nullptr : &sDontLoadLiteral[0], case_insensitive),
	_filename(filename),
	_dirty(!load)
{
	// for symlinks need to save later into final destination path to avoid symlink disruption
	// also relative path should be translated to absolute to avoid wrong file saving if current directory changed in a way
	char *real_filename = realpath(_filename.c_str(), NULL);
	if (real_filename) {
		if (*real_filename == '/') {
			_filename = real_filename;
		}
		free(real_filename);
	}
}

KeyFileHelper::~KeyFileHelper()
{
	Save(true);
}

static std::atomic<unsigned int> s_tmp_uniq{0};
bool KeyFileHelper::Save(bool only_if_dirty)
{
	if (only_if_dirty && !_dirty)
		return true;

	std::string tmp = _filename;
	unsigned int  tmp_uniq = ++s_tmp_uniq;
	tmp+= StrPrintf(".%u-%u", getpid(), tmp_uniq);
	try {
		FDScope fd(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, MakeFileMode(_filestat));
		if (!fd.Valid()) {
			throw std::runtime_error("create file failed");
		}

		std::string content;
		KFEscaping esc;
		for (const auto &si : _kf.Enumeration()) {
			content+= '[';
			content+= esc.EncodeSection(si->first);
			content+= "]\n";

			const auto &kmap = si->second;
			for (const auto &ki : kmap) {
				content+= esc.EncodeKey(ki.first);
				content+= '=';
				content+= esc.EncodeValue(ki.second);
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

		if (os_call_int(remove, tmp.c_str()) == -1) {
			perror("remove");
		}
		return false;
	}

	if (os_call_int(rename, tmp.c_str(), _filename.c_str()) == -1) {
		fprintf(stderr,
			"KeyFileHelper::Save: errno=%u while renaming '%s' -> '%s'\n",
				errno, tmp.c_str(), _filename.c_str());
		if (os_call_int(remove, tmp.c_str()) == -1) {
			perror("remove");
		}
		return false;
	}

	_dirty = false;
	return true;
}

bool KeyFileHelper::RemoveSection(const std::string &section)
{
	if (!_kf.Remove(section)) {
		return false;
	}

	_dirty = true;
	return true;
}

size_t KeyFileHelper::RemoveSectionsAt(const std::string &parent_section)
{
	std::string prefix = parent_section;
	if (prefix == "/") {
		prefix.clear();

	} else if (!prefix.empty() && prefix.back() != '/') {
		prefix+= '/';
	}

	const size_t out = _kf.RemoveAt(prefix);
	if (out != 0) {
		_dirty = true;
	}

	return out;
}

void KeyFileHelper::RemoveKey(const std::string &section, const std::string &name)
{
	auto *vals = _kf.Find(section);
	if (vals && vals->erase(name) != 0) {
		_dirty = true;
//		if (it->second.empty()) {
//			_kf.erase(it);
//		}
	}
}

void KeyFileHelper::SetString(const std::string &section, const std::string &name, const std::string &value)
{
	auto *s = _kf.Ensure(section);

	auto it = FindValue(s, name);
	if (it != s->end()) {
		if (it->second == value) {
			return;
		}
		it->second = value;

	} else {
		s->emplace(name, value);
	}

	_dirty = true;
}

void KeyFileHelper::SetString(const std::string &section, const std::string &name, const char *value)
{
	if (!value) {
		value = "";
	}

	auto *s = _kf.Ensure(section);

	auto it = FindValue(s, name);
	if (it != s->end()) {
		if (it->second.compare(value) == 0) {
			return;
		}
		it->second = value;

	} else {
		s->emplace(name, value);
	}

	_dirty = true;
}

void KeyFileHelper::SetString(const std::string &section, const std::string &name, const wchar_t *value)
{
	if (!value) {
		value = L"";
	}
	SetString(section, name, Wide2MB(value));
}

void KeyFileHelper::SetInt(const std::string &section, const std::string &name, int value)
{
	SetString(section, name, ToDec(value));
}

void KeyFileHelper::SetUInt(const std::string &section, const std::string &name, unsigned int value)
{
	SetString(section, name, ToPrefixedHex(value));
}

void KeyFileHelper::SetULL(const std::string &section, const std::string &name, unsigned long long value)
{
	SetString(section, name, ToPrefixedHex(value));
}

void KeyFileHelper::SetBytes(const std::string &section,
	const std::string &name, const unsigned char *buf, size_t len, size_t space_interval)
{
	std::string str;
	str.reserve(len * 2 + (space_interval ? (len / space_interval) + 1 : 0) );
	for (size_t i = 0; i != len; ++i) {
		if (i && space_interval && (i % space_interval) == 0) {
			str+= ' ';
		}
		str+= MakeHexDigit(buf[i] >> 4);
		str+= MakeHexDigit(buf[i] & 0xf);
	}

	SetString(section, name, str);
}

void KeyFileHelper::RenameSection(const std::string &src, const std::string &dst, bool recursed)
{
	if (src == dst) {
		return;
	}

	if (_kf.Rename(src, dst)) {
		_dirty = true;
	}

	if (recursed) {
		std::string subname;
		const auto &subsections = EnumSectionsAt(src, true);
		for (const auto &subsection : subsections) {
			subname = subsection;
			subname.replace(0, src.size(), dst);
			if (_kf.Rename(subsection, subname)) {
				_dirty = true;
			}
		}
	}
}
