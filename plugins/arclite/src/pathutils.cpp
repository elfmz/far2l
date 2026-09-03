#include "headers.hpp"

#include "utils.hpp"

#if 0
std::string make_absolute_symlink_target(const std::string &symlink_path, const std::string &raw_target)
{
	if (!raw_target.empty() && raw_target[0] == '/') {
		return raw_target;
	}

	size_t last_slash = symlink_path.find_last_of('/');

	if (last_slash == 0) {
		return "/" + raw_target;
	}

	std::string base_dir;
	if (last_slash == std::string::npos) {
		base_dir = ".";
	} else {
		base_dir = symlink_path.substr(0, last_slash);
		if (base_dir.empty()) base_dir = "/";
	}

	std::vector<std::string> parts;
	parts.reserve(16);

	auto add_parts = [&parts](const std::string &path) {
		size_t start = 0;
		size_t end = 0;

		if (!path.empty() && path[0] == '/') {
			start = 1;
		}

		while (start < path.length()) {
			end = path.find('/', start);
			if (end == std::string::npos) {
				end = path.length();
			}

			if (end > start) {
				std::string part = path.substr(start, end - start);
				if (part == "..") {
					if (!parts.empty()) parts.pop_back();
				} else if (part != ".") {
					parts.push_back(std::move(part));
				}
			}

			start = end + 1;
		}
	};

	add_parts(base_dir);
	add_parts(raw_target);

	if (parts.empty()) {
		return "/";
	}

	std::string result;
	result.reserve(parts.size() * 10 + 1);

	result = "/";
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) result += "/";
		result += parts[i];
	}

	return result;
}

std::string get_parent_directory(const std::string& path) {
	size_t last_slash = path.find_last_of('/');
	if (last_slash == std::string::npos) {
		return ".";
	} else if (last_slash == 0) {
		return "/";
	} else {
		return path.substr(0, last_slash);
	}
}

std::string normalize_path(const std::string& path) {
	if (path.empty()) return "/";
	
	std::vector<std::string> parts;
	size_t start = (path[0] == '/') ? 1 : 0;
	size_t end = 0;
	
	while (start < path.length()) {
		end = path.find('/', start);
		if (end == std::string::npos) end = path.length();
		
		if (end > start) {
			std::string part = path.substr(start, end - start);
			if (part == "..") {
				if (!parts.empty()) parts.pop_back();
			} else if (part != ".") {
				parts.push_back(part);
			}
		}
		start = end + 1;
	}

	if (parts.empty()) return "/";
	
	std::string result = "/";
	for (size_t i = 0; i < parts.size(); i++) {
		if (i > 0) result += "/";
		result += parts[i];
	}
	
	return result;
}

std::string make_relative_symlink_target(const std::string &symlink_path, const std::string &raw_target)
{
	if (raw_target.empty() || raw_target[0] != '/') {
		return raw_target;
	}

	if (symlink_path.empty() || symlink_path[0] != '/') {
		return raw_target;
	}

	std::string from_dir = get_parent_directory(symlink_path);
	if (from_dir.empty()) from_dir = "/";

	std::string to = raw_target;

	from_dir = normalize_path(from_dir);
	to = normalize_path(to);

	auto split_path = [](const std::string& path) -> std::vector<std::string> {
		std::vector<std::string> parts;
		if (path.empty() || path == "/") return parts;
		
		size_t start = 1;
		size_t end = 0;
		
		while (start < path.length()) {
			end = path.find('/', start);
			if (end == std::string::npos) end = path.length();
			
			if (end > start) {
				parts.push_back(path.substr(start, end - start));
			}
			start = end + 1;
		}
		return parts;
	};

	auto from_parts = split_path(from_dir);
	auto to_parts = split_path(to);

	size_t common = 0;
	size_t min_size = std::min(from_parts.size(), to_parts.size());
	while (common < min_size && from_parts[common] == to_parts[common]) {
		common++;
	}

	std::string result;
	
	for (size_t i = common; i < from_parts.size(); i++) {
		if (!result.empty()) result += "/";
		result += "..";
	}

	for (size_t i = common; i < to_parts.size(); i++) {
		if (!result.empty()) result += "/";
		result += to_parts[i];
	}

	return result.empty() ? "." : result;
}
#endif

template<typename CharT>
static std::basic_string<CharT> make_string(const char* s) {
	if constexpr (std::is_same_v<CharT, char>) {
		return std::string(s);
	} else {
		return std::wstring(s, s + strlen(s));
	}
}

template<typename CharT>
static CharT slash_char() {
	if constexpr (std::is_same_v<CharT, char>) return '/';
	else return L'/';
}

template<typename CharT>
static std::basic_string<CharT> dot_str() {
	if constexpr (std::is_same_v<CharT, char>) return ".";
	else return L".";
}

template<typename CharT>
static std::basic_string<CharT> dotdot_str() {
	if constexpr (std::is_same_v<CharT, char>) return "..";
	else return L"..";
}

template<typename CharT>
std::basic_string<CharT> make_absolute_symlink_target(
	const std::basic_string<CharT>& symlink_path,
	const std::basic_string<CharT>& raw_target)
{
	const CharT slash = slash_char<CharT>();
	if (!raw_target.empty() && raw_target[0] == slash) {
		return raw_target;
	}

	size_t last_slash = symlink_path.find_last_of(slash);
	if (last_slash == 0) {
		return make_string<CharT>("/") + raw_target;
	}

	std::basic_string<CharT> base_dir;
	if (last_slash == std::basic_string<CharT>::npos) {
		base_dir = dot_str<CharT>();
	} else {
		base_dir = symlink_path.substr(0, last_slash);
		if (base_dir.empty()) base_dir = make_string<CharT>("/");
	}

	std::vector<std::basic_string<CharT>> parts;
	parts.reserve(16);

	auto add_parts = [&parts, slash](const std::basic_string<CharT>& path) {
		size_t start = 0;
		if (!path.empty() && path[0] == slash) start = 1;

		while (start < path.length()) {
			size_t end = path.find(slash, start);
			if (end == std::basic_string<CharT>::npos) end = path.length();
			if (end > start) {
				auto part = path.substr(start, end - start);
				if (part == dotdot_str<CharT>()) {
					if (!parts.empty()) parts.pop_back();
				} else if (part != dot_str<CharT>()) {
					parts.push_back(std::move(part));
				}
			}
			start = end + 1;
		}
	};

	add_parts(base_dir);
	add_parts(raw_target);

	if (parts.empty()) {
		return make_string<CharT>("/");
	}

	std::basic_string<CharT> result;
	result.reserve(parts.size() * 10 + 1);
	result = make_string<CharT>("/");
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) result += slash;
		result += parts[i];
	}
	return result;
}

template<typename CharT>
std::basic_string<CharT> get_parent_directory(const std::basic_string<CharT>& path) {
	const CharT slash = slash_char<CharT>();
	size_t last_slash = path.find_last_of(slash);
	if (last_slash == std::basic_string<CharT>::npos) {
		return dot_str<CharT>();
	} else if (last_slash == 0) {
		return make_string<CharT>("/");
	} else {
		return path.substr(0, last_slash);
	}
}

template<typename CharT>
std::basic_string<CharT> normalize_path(const std::basic_string<CharT>& path) {
	if (path.empty()) return make_string<CharT>("/");

	const CharT slash = slash_char<CharT>();
	std::vector<std::basic_string<CharT>> parts;
	size_t start = (path[0] == slash) ? 1 : 0;

	while (start < path.length()) {
		size_t end = path.find(slash, start);
		if (end == std::basic_string<CharT>::npos) end = path.length();
		if (end > start) {
			auto part = path.substr(start, end - start);
			if (part == dotdot_str<CharT>()) {
				if (!parts.empty()) parts.pop_back();
			} else if (part != dot_str<CharT>()) {
				parts.push_back(part);
			}
		}
		start = end + 1;
	}

	if (parts.empty()) return make_string<CharT>("/");
	std::basic_string<CharT> result = make_string<CharT>("/");
	for (size_t i = 0; i < parts.size(); i++) {
		if (i > 0) result += slash;
		result += parts[i];
	}
	return result;
}

template<typename CharT>
std::basic_string<CharT> make_relative_symlink_target(
	const std::basic_string<CharT>& symlink_path,
	const std::basic_string<CharT>& raw_target)
{
	const CharT slash = slash_char<CharT>();
	if (raw_target.empty() || raw_target[0] != slash) {
		return raw_target;
	}
	if (symlink_path.empty() || symlink_path[0] != slash) {
		return raw_target;
	}

	auto from_dir = get_parent_directory(symlink_path);
	if (from_dir.empty()) from_dir = make_string<CharT>("/");

	auto to = raw_target;
	from_dir = normalize_path(from_dir);
	to = normalize_path(to);

	auto split_path = [slash](const std::basic_string<CharT>& path) {
		std::vector<std::basic_string<CharT>> parts;
		if (path.empty() || path == make_string<CharT>("/")) return parts;
		size_t start = 1;
		while (start < path.length()) {
			size_t end = path.find(slash, start);
			if (end == std::basic_string<CharT>::npos) end = path.length();
			if (end > start) {
				parts.push_back(path.substr(start, end - start));
			}
			start = end + 1;
		}
		return parts;
	};

	auto from_parts = split_path(from_dir);
	auto to_parts = split_path(to);

	size_t common = 0;
	size_t min_size = std::min(from_parts.size(), to_parts.size());
	while (common < min_size && from_parts[common] == to_parts[common]) {
		++common;
	}

	std::basic_string<CharT> result;
	const auto dotdot = dotdot_str<CharT>();
	for (size_t i = common; i < from_parts.size(); ++i) {
		if (!result.empty()) result += slash;
		result += dotdot;
	}
	for (size_t i = common; i < to_parts.size(); ++i) {
		if (!result.empty()) result += slash;
		result += to_parts[i];
	}

	return result.empty() ? dot_str<CharT>() : result;
}

template std::basic_string<char> make_absolute_symlink_target<char>(
	const std::basic_string<char>&, const std::basic_string<char>&);
template std::basic_string<wchar_t> make_absolute_symlink_target<wchar_t>(
	const std::basic_string<wchar_t>&, const std::basic_string<wchar_t>&);

template std::basic_string<char> get_parent_directory<char>(const std::basic_string<char>&);
template std::basic_string<wchar_t> get_parent_directory<wchar_t>(const std::basic_string<wchar_t>&);

template std::basic_string<char> normalize_path<char>(const std::basic_string<char>&);
template std::basic_string<wchar_t> normalize_path<wchar_t>(const std::basic_string<wchar_t>&);

template std::basic_string<char> make_relative_symlink_target<char>(
	const std::basic_string<char>&, const std::basic_string<char>&);
template std::basic_string<wchar_t> make_relative_symlink_target<wchar_t>(
	const std::basic_string<wchar_t>&, const std::basic_string<wchar_t>&);

std::wstring long_path(const std::wstring &path)
{
#if 0
	if (substr_match(path, 0, L"\\\\")) {
		if (substr_match(path, 2, L"?\\") || substr_match(path, 2, L".\\")) {
			return path;
	}
	else {
		return std::wstring(path).replace(0, 1, L"\\\\?\\UNC");
	}
	}
	else {
		return std::wstring(path).insert(0, L"\\\\?\\");
	}
#endif
	return path;
}

std::wstring long_path_norm(const std::wstring &path)
{
	auto nt_path = long_path(path);
	auto p = nt_path.find(L'\\');
	while (p != std::wstring::npos) {
		nt_path[p] = L'/';
		p = nt_path.find(L'\\', p + 1);
	}
	return nt_path;
}

std::wstring add_trailing_slash(const std::wstring &path)
{
	if ((path.size() == 0) || (path[path.size() - 1] == L'/')) {
		return path;
	} else {
		return path + L'/';
	}
}

std::wstring add_leading_slash(const std::wstring &path)
{
	if (path.empty() || path[0] == L'/') {
		return path;
	}
	return L'/' + path;
}

std::wstring del_trailing_slash(const std::wstring &path)
{
	if ((path.size() < 2) || (path[path.size() - 1] != L'/')) {
		return path;
	} else {
		return path.substr(0, path.size() - 1);
	}
}

static void locate_path_root(const std::wstring &path, size_t &path_root_len, bool &is_unc_path)
{
	unsigned prefix_len = 0;
	is_unc_path = false;

	path_root_len = 0;
	return;
#if 0
  if (substr_match(path, 0, L"\\\\")) {
	if (substr_match(path, 2, L"?\\UNC\\")) {
	  prefix_len = 8;
	  is_unc_path = true;
	}
	else if (substr_match(path, 2, L"?\\") || substr_match(path, 2, L".\\")) {
	  prefix_len = 4;
	}
	else {
	  prefix_len = 2;
	  is_unc_path = true;
	}
  }

  if ((prefix_len == 0) && !substr_match(path, 1, L":\\")) {
	path_root_len = 0;
  }
  else {
	std::wstring::size_type p = path.find(L'\\', prefix_len);
	if (p == std::wstring::npos) {
	  p = path.size();
	}
	if (is_unc_path) {
	  p = path.find(L'\\', p + 1);
	  if (p == std::wstring::npos) {
		p = path.size();
	  }
	}
	path_root_len = p;
  }
#endif
	std::wstring::size_type p = path.find(L'/', prefix_len);
	if (p == std::wstring::npos) {
		p = path.size();
	}
	path_root_len = p;
}

std::wstring extract_path_root(const std::wstring &path)
{
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if (path_root_len)
		return path.substr(0, path_root_len).append(1, L'/');
	else
		return std::wstring();
}

std::wstring extract_file_name(const std::wstring &path)
{
	size_t pos = path.rfind(L'/');
	if (pos == std::wstring::npos) {
		pos = 0;
	} else {
		pos++;
	}
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if ((pos <= path_root_len) && (path_root_len != 0))
		return std::wstring();
	else
		return path.substr(pos);
}

std::wstring extract_file_path(const std::wstring &path)
{
	size_t pos = path.rfind(L'/');
	if (pos == std::wstring::npos) {
		pos = 0;
	}
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if ((pos <= path_root_len) && (path_root_len != 0))
		return path.substr(0, path_root_len).append(1, L'/');
	else
		return path.substr(0, pos);
}

std::wstring extract_file_ext(const std::wstring &path)
{
	size_t ext_pos = path.rfind(L'.');
	if (ext_pos == std::wstring::npos) {
		return std::wstring();
	}
	size_t name_pos = path.rfind(L'/');
	if (name_pos == std::wstring::npos) {
		name_pos = 0;
	} else {
		name_pos++;
	}
	if (ext_pos <= name_pos)
		return std::wstring();
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if ((ext_pos <= path_root_len) && (path_root_len != 0))
		return std::wstring();
	else
		return path.substr(ext_pos);
}

bool is_root_path(const std::wstring &path)
{
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	return (path.size() == path_root_len)
			|| ((path.size() == path_root_len + 1) && (path[path.size() - 1] == L'/'));
}

bool is_unc_path(const std::wstring &path)
{
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	return is_unc_path;
}

bool is_absolute_path(const std::wstring &path)
{
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if (path_root_len == 0)
		return false;
	std::wstring::size_type p1 = path_root_len;
	while (p1 != path.size()) {
		p1 += 1;
		std::wstring::size_type p2 = path.find(L'/', p1);
		if (p2 == std::wstring::npos)
			p2 = path.size();
		std::wstring::size_type sz = p2 - p1;
		if (sz == 1 && path[p1] == L'.')
			return false;
		if (sz == 2 && path[p1] == L'.' && path[p1 + 1] == L'.')
			return false;
		p1 = p2;
	}
	return true;
}

std::wstring remove_path_root(const std::wstring &path)
{
	size_t path_root_len;
	bool is_unc_path;
	locate_path_root(path, path_root_len, is_unc_path);
	if ((path_root_len < path.size()) && (path[path_root_len] == L'/'))
		path_root_len++;
	return path.substr(path_root_len);
}

std::string removeExtension(const std::string &filename) {
	size_t dotPos = filename.find_last_of('.');

	if (dotPos == std::string::npos || dotPos == 0) {
		return filename;
	}

	if (dotPos == filename.length() - 1) {
		return filename;
	}

	if (dotPos == 0) {
		return filename;
	}

	return filename.substr(0, dotPos);
}

void removeExtension(std::wstring &filename) {
	size_t dotPos = filename.find_last_of('.');

	if (dotPos == std::wstring::npos || dotPos == 0) {
		return;
	}
	if (dotPos == filename.length() - 1) {
		return;
	}
	filename = filename.substr(0, dotPos);
}

void removeExtension(std::vector<unsigned char> &filename) {
	size_t dotPos = std::string::npos;
	for (size_t i = 0; i < filename.size(); ++i) {
		if (filename[i] == '.') {
			dotPos = i;
		}
	}

	if (dotPos == std::string::npos || dotPos == 0) {
		return;
	}
	if (dotPos == filename.size() - 1) {
		return;
	}
	filename.erase(filename.begin() + dotPos, filename.end());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef TOOLS_TOOL

static const wchar_t simple_replace_char = L'_';
static const wchar_t simple_replace_str[] = L"_";

// '<'
static const wchar_t LeftPointing_Double_Angle_Quotation_Mark = L'\u00AB';	  // '«'
// static const wchar_t Single_LeftPointing_Angle_Quotation_Mark  = L'\u2039'; // '‹'

// '>'
static const wchar_t RightPointing_Double_Angle_Quotation_Mark = L'\u00BB';	   // '»'
// static const wchar_t Single_RightPointing_Angle_Quotation_Mark = L'\u203A'; // '›'

static wchar_t quotes2[2] = {LeftPointing_Double_Angle_Quotation_Mark,
		RightPointing_Double_Angle_Quotation_Mark};
// static wchar_t quotes1[2] = { Single_LeftPointing_Angle_Quotation_Mark,
// Single_RightPointing_Angle_Quotation_Mark };

// ':'
// static const wchar_t Division_Sign                             = L'\u00F7'; // '÷'
// static const wchar_t Identical_To                              = L'\u2216'; // '≡'
static const wchar_t Horizontal_Ellipsis = L'\u2026';	 // '…'
static const wchar_t American_Full_Stop = L'\u0589';	 // '։' --missed in Lucida Console font
// static const wchar_t Raised Colon                              = L'\u02F8'; // '˸' --missed in Lucida
// Console font
static wchar_t colons[2] = {Horizontal_Ellipsis, American_Full_Stop};

// '*'
static const wchar_t Currency_Sign = L'\u00A4';				// '¤'
static const wchar_t Six_Pointed_Black_Star = L'\u0589';	// '✶' --missed in Lucida Console font
static wchar_t asterisks[2] = {Currency_Sign, Six_Pointed_Black_Star};

// '?'
static const wchar_t Inverted_Question_Mark = L'\u00BF';	// '¿'
// static const wchar_t Interrobang                               = L'\u203D'; // '‽' --missed in Lucida
// Console font

// '|'
static const wchar_t Broken_Bar = L'\u00A6';	// '¦'
// static const wchar_t Dental_Click                              = L'\u01C0'; // 'ǀ' --missed in Lucida
// Console font

// '"'
static const wchar_t Left_Double_Quotation_Mark = L'\u201C';	 // '“'
static const wchar_t Right_Double_Quotation_Mark = L'\u201D';	 // '”'
static wchar_t quotes3[2] = {Left_Double_Quotation_Mark, Right_Double_Quotation_Mark};

// '/'
static const wchar_t Fraction_Slash = L'\u2044';	// '⁄'

// '\'
static const wchar_t Not_Sign = L'\u00AC';	  // '¬'

// static const wchar_t Middle_Dot                              = L'\u00B7'; // '·'

static const wchar_t control_chars[32 - 1] = {
		L'\u263A'	 // \x01  '☺'  (White Smiling Face)
		,
		L'\u263B'	 // \x02  '☻'  (Black Smiling Face)
		,
		L'\u2665'	 // \x03  '♥'  (Black Heart Suit)
		,
		L'\u2666'	 // \x04  '♦'  (Black Diamond Suit)
		,
		L'\u2663'	 // \x05  '♣'  (Black Club Suit)
		,
		L'\u2660'	 // \x06  '♠'  (Black Spade Suit)
		,
		L'\u2022'	 // \x07  '•'  (Bullet)
		,
		L'\u25D8'	 // \x08  '◘'  (Inverse Bullet)
		,
		L'\u25CB'	 // \x09  '○'  (White Circle)
		,
		L'\u25D9'	 // \x0A  '◙'  (Inverse White Circle)
		,
		L'\u2642'	 // \x0B  '♂'  (Male Sign)
		,
		L'\u2640'	 // \x0C  '♀'  (Female Sign)
		,
		L'\u266A'	 // \x0D  '♪'  (Eight Note)
		,
		L'\u266B'	 // \x0E  '♫'  (Beamed Eight Note)
		,
		L'\u263C'	 // \x0F  '☼'  (White Sun With Rays)
		,
		L'\u25BA'	 // \x10  '►'  (Black Right-Pointing Pointer)
		,
		L'\u25C4'	 // \x11  '◄'  (Black Left-Pointing Pointer)
		,
		L'\u2195'	 // \x12  '↕'  (Up Down Arrow)
		,
		L'\u203C'	 // \x13  '‼'  (Double Exclamation Mark)
		,
		L'\u00B6'	 // \x14  '¶'  (Pilcrow Sign)
		,
		L'\u00A7'	 // \x15  '§'  (Section Sign)
		,
		L'\u25AC'	 // \x16  '▬'  (Black Rectangle)
		,
		L'\u21A8'	 // \x17  '↨'  (Up Down Arrow With Base)
		,
		L'\u2191'	 // \x18  '↑'  (Upwards Arrow)
		,
		L'\u2193'	 // \x19  '↓'  (Downwards Arrow)
		,
		L'\u2192'	 // \x1A  '→'  (Rightwards Arrow)
		,
		L'\u2190'	 // \x1B  '←'  (Leftwards Arrow)
		,
		L'\u221F'	 // \x1C  '∟'  (Right Angle)
		,
		L'\u2194'	 // \x1D  '↔'  (Left Right Arrow)
		,
		L'\u25B2'	 // \x1E  '▲'  (Black Up-Pointing Triangle)
		,
		L'\u25BC'	 // \x1F  '▼'  (Black Down-Pointing Triangle)
};

//-----------------------------------------------------------------------------

static const wchar_t *reserved_names[] = {L"CON", L"PRN", L"AUX", L"NUL", L"COM9", L"LPT9"};

static bool is_matched(const std::wstring &name, const wchar_t *res)
{
	size_t i = 0, len = name.size();
	while (i < len) {
		wchar_t c1 = res[i];
		wchar_t c2 = name[i];
		if (!c1) {
			break;
		} else if (c1 == L'9') {
			if (c2 < '0' || c2 > L'9')
				return false;
		} else {
			//      if (c1 != (wchar_t)(size_t)CharUpperW((LPWSTR)(size_t)c2))
			if (c1 != Upper(c2))
				return false;
		}
		++i;
	}
	if (res[i])
		return false;

	while (i < len) {
		if (name[i] == L'.')
			return true;
		else if (name[i] != L' ')
			return false;
		++i;
	}
	return true;
}

static bool is_reserved_name(const std::wstring &name)
{
	for (const auto &res : reserved_names) {
		if (is_matched(name, res))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

std::wstring correct_filename(const std::wstring &orig_name, int mode, bool alt_stream)
{
	bool correct_empty = (mode & 0x10) != 0;
	bool remove_final_dotsp = (mode & 0x20) != 0;
	bool correct_reserved = (mode & 0x40) != 0;
	int m = std::min(mode & 0x0f, 3);

	auto name(orig_name);
	if (m > 0) {
		int i = 0, q = 0;
		for (const auto c : name) {
			switch (c) {
				case L'<':
					name[i] = m > 1 ? quotes2[0] : simple_replace_char;
					break;
				case L'>':
					name[i] = m > 1 ? quotes2[1] : simple_replace_char;
					break;
				case L':':
					name[i] = alt_stream ? c : (m > 1 ? colons[m - 2] : simple_replace_char);
					break;
				case L'*':
					name[i] = m > 1 ? asterisks[m - 2] : simple_replace_char;
					break;
				case L'?':
					name[i] = m > 1 ? Inverted_Question_Mark : simple_replace_char;
					break;
				case L'|':
					name[i] = m > 1 ? Broken_Bar : simple_replace_char;
					break;
				case L'"':
					name[i] = m > 1 ? quotes3[q] : simple_replace_char;
					q = 1 - q;
					break;
				case L'/':
					name[i] = m > 1 ? Fraction_Slash : simple_replace_char;
					break;
				case L'\\':
					name[i] = m > 1 ? Not_Sign : simple_replace_char;
					break;
				default:
					if (c < L' ' && c > L'\0')
						name[i] = m > 1 ? control_chars[c - L'\x01'] : simple_replace_char;
					break;
			}
			++i;
		}
	}

	if (correct_reserved) {
		if (is_reserved_name(name))
			name = simple_replace_str + name;
	}

	if (remove_final_dotsp) {
		while (!name.empty() && (name.back() == L'.' || name.back() == L' '))
			name.pop_back();
	}

	if (name.empty() && correct_empty) {
		name = simple_replace_str;
	}

	return name;
}
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
