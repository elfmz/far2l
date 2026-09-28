#pragma once

#include "error.hpp"
#include "noncopyable.hpp"

enum TriState
{
	triFalse = 0,
	triTrue = 1,
	triUndef = 2,
};

typedef std::vector<unsigned char> ByteVector;

template<typename CharT>
std::basic_string<CharT> make_absolute_symlink_target(
	const std::basic_string<CharT> &symlink_path,
	const std::basic_string<CharT> &raw_target);

template<typename CharT>
std::basic_string<CharT> get_parent_directory(const std::basic_string<CharT> &path);

template<typename CharT>
std::basic_string<CharT> normalize_path(const std::basic_string<CharT> &path);

template<typename CharT>
std::basic_string<CharT> make_relative_symlink_target(
	const std::basic_string<CharT> &symlink_path,
	const std::basic_string<CharT> &raw_target);

extern template std::basic_string<char> make_absolute_symlink_target<char>(
	const std::basic_string<char>&, const std::basic_string<char>&);
extern template std::basic_string<wchar_t> make_absolute_symlink_target<wchar_t>(
	const std::basic_string<wchar_t>&, const std::basic_string<wchar_t>&);

extern template std::basic_string<char> get_parent_directory<char>(const std::basic_string<char>&);
extern template std::basic_string<wchar_t> get_parent_directory<wchar_t>(const std::basic_string<wchar_t>&);

extern template std::basic_string<char> normalize_path<char>(const std::basic_string<char>&);
extern template std::basic_string<wchar_t> normalize_path<wchar_t>(const std::basic_string<wchar_t>&);

extern template std::basic_string<char> make_relative_symlink_target<char>(
	const std::basic_string<char>&, const std::basic_string<char>&);
extern template std::basic_string<wchar_t> make_relative_symlink_target<wchar_t>(
	const std::basic_string<wchar_t>&, const std::basic_string<wchar_t>&);

bool substr_match(const std::wstring &str, std::wstring::size_type pos, std::wstring::const_pointer mstr);
std::wstring word_wrap(const std::wstring &str, std::wstring::size_type wrap_bound);
std::wstring fit_str(const std::wstring &str, std::wstring::size_type size);
std::wstring center(const std::wstring &str, unsigned width);
std::string strip(const std::string &str);
std::wstring strip(const std::wstring &str);
int str_to_int(const std::string &str);
int str_to_int(const std::wstring &str);
std::wstring int_to_str(int val);
uint64_t str_to_uint(const std::wstring &str);
std::wstring uint_to_str(uint64_t val);
std::wstring widen(const std::string &str);
std::list<std::wstring> split(const std::wstring &str, wchar_t sep);
std::wstring combine(const std::list<std::wstring> &lst, wchar_t sep);
std::wstring format_data_size(uint64_t value, const wchar_t *suffixes[5]);
bool is_slash(wchar_t c);
std::wstring unquote(const std::wstring &str);
std::wstring
search_and_replace(const std::wstring &str, const std::wstring &search_str, const std::wstring &replace_str);
bool str_start_with(const std::wstring &str, const wchar_t *prefix, const bool ignore_case = true);
bool str_end_with(const std::wstring &str, const wchar_t *suffix, const bool ignore_case = true);

std::wstring long_path(const std::wstring &path);
std::wstring long_path_norm(const std::wstring &path);

std::wstring add_trailing_slash(const std::wstring &path);
std::wstring del_trailing_slash(const std::wstring &path);
std::wstring add_leading_slash(const std::wstring &path);

std::wstring extract_path_root(const std::wstring &path);
std::wstring extract_file_name(const std::wstring &path);
std::wstring extract_file_path(const std::wstring &path);
std::wstring extract_file_ext(const std::wstring &path);
bool is_root_path(const std::wstring &path);
bool is_unc_path(const std::wstring &path);
bool is_absolute_path(const std::wstring &path);
std::wstring remove_path_root(const std::wstring &path);
std::wstring correct_filename(const std::wstring &name, int mode, bool alt_stream);
std::string removeExtension(const std::string &filename);
void removeExtension(std::wstring &filename);
void removeExtension(std::vector<unsigned char> &filename);

template <class T>
inline const T *null_to_empty(const T *Str)
{
	static const T empty = T();
	return Str ? Str : &empty;
}

int al_round(double d);

#if 0
class NonCopyable {
protected:
  NonCopyable() {}
  ~NonCopyable() {}
private:
  NonCopyable(const NonCopyable&);
  NonCopyable& operator=(const NonCopyable&);
};
#endif

template <typename Type>
class Buffer : private NonCopyable
{
private:
	Type *buffer;
	size_t buf_size;

public:
	Buffer() : buffer(nullptr), buf_size(0) {}
	Buffer(size_t size)
	{
		buffer = new Type[size];
		buf_size = size;
	}
	~Buffer() { delete[] buffer; }
	void resize(size_t size)
	{
		delete[] buffer;
		buffer = new Type[size];
		buf_size = size;
	}
	Type *data() { return buffer; }
	size_t size() const { return buf_size; }
	void clear() { memset(buffer, 0, buf_size * sizeof(Type)); }
};
