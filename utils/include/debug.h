#pragma once

#include "cctweaks.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <codecvt>
#include <string_view>
#include <unistd.h>
#include <iomanip>
#include <mutex>
#include <vector>
#include <tuple>
#include <utility>
#include <functional>
#include <cstring>
#include <thread>
#include <atomic>
#include <cstddef>
#include <sys/stat.h>
#include <cstdint>
#include <bitset>
#include <iterator>
#include <algorithm>


/** This ABORT_* / ASSERT_* have following distinctions comparing to abort/assert:
  * - Errors logged into ~/.config/far2l/crash.log
  * - Error printed in WinPort UI if possible, so user will see it unlike abort/assert that typically cause silent exit
  * - Have possibility to show customized message
  * - ASSERT_* evaluated _always_ regardless of _NDEBUG macro
  * - Use with caution in WinPort project as may deadlock if called from UI drawing/input handling code there
  */
#define ABORT_MSG(FMT, ...) \
	Panic("%d@%s: " FMT, __LINE__, __FUNCTION__, ##__VA_ARGS__ )

#define ABORT() ABORT_MSG("ABORT")

#define ASSERT_MSG(COND, FMT, ...) if (UNLIKELY(!(COND))) { ABORT_MSG(FMT, ##__VA_ARGS__); }

#define ASSERT(COND) if (UNLIKELY(!(COND))) { ABORT_MSG("ASSERT"); }

void FN_NORETURN FN_PRINTF_ARGS(1) Panic(const char *format, ...) noexcept;

#define DBGLINE fprintf(stderr, "%d %d @%s\n", getpid(), __LINE__, __FILE__)


namespace Dumper {

	// ****************************************************************************************************
	// Вспомогательные переменные, функции и структуры
	// ****************************************************************************************************

	inline std::mutex g_log_output_mutex;
	inline std::atomic<std::size_t> g_thread_counter {0};
	inline std::once_flag g_escape_table_flag;
	inline std::array<std::string, 256> g_escape_table;


	inline std::array<std::string, 256> BuildEscapeTable()
	{
		std::array<std::string, 256> table{};
		char buf[8];
		for (size_t i = 0; i < 256; ++i) {
			auto character_code = static_cast<unsigned char>(i);
			if (character_code >= 0x20) {
				switch (character_code) {
				case '"': table[i] = "\\\""; break;
				case '\\': table[i] = "\\\\"; break;
				default:
					table[i] = std::string(1, character_code);
					break;
				}
			} else {
				switch (character_code) {
				case '\t': table[i] = "\\t"; break;
				case '\r': table[i] = "\\r"; break;
				case '\n': table[i] = "\\n"; break;
				case '\a': table[i] = "\\a"; break;
				case '\b': table[i] = "\\b"; break;
				case '\v': table[i] = "\\v"; break;
				case '\f': table[i] = "\\f"; break;
				case '\x1B': table[i] = "\\e"; break;
				default:
					std::snprintf(buf, sizeof(buf), "\\x{%02x}", character_code);
					table[i] = buf;
					break;
				}
			}
		}
		return table;
	}


	inline std::string EscapeString(std::string_view input)
	{
		std::call_once(g_escape_table_flag, []{
			g_escape_table = BuildEscapeTable();
		});

		std::string output;
		for (unsigned char c : input) {
			output.append(g_escape_table[c]);
		}
		return output;
	}


	inline const std::string& GetHomeDir()
	{
		static std::string s_home;
		static std::once_flag s_flag;
		std::call_once(s_flag, [] {
			const char *env_home = std::getenv("HOME");
			s_home = (env_home != nullptr) ? std::string(env_home) : std::string("/tmp");
		});
		return s_home;
	}


	inline std::size_t GetNiceThreadId() noexcept
	{
		static thread_local std::size_t s_nice_thread_id = 0;
		if (UNLIKELY(!s_nice_thread_id)) {
			s_nice_thread_id = g_thread_counter.fetch_add(1, std::memory_order_relaxed) + 1;
		}
		return s_nice_thread_id;
	}


	// Метафункция для проверки на этапе компиляции: является ли тип T контейнером?

	template <typename T, typename = void>
	struct is_container : std::false_type { };

	template <typename T>
	struct is_container<T, std::void_t<
							   decltype(std::begin(std::declval<T&>())),
							   decltype(std::end(std::declval<T&>()))
							   >> : std::true_type { };

	template <typename T>
	inline constexpr bool is_container_v = is_container<T>::value;

	// ****************************************************************************************************
	// Форматирование имён/значений переменных с учётом уровня их возможной вложенности при отображении контейнеров
	// ****************************************************************************************************

	constexpr std::size_t MAX_INDENT_LEVEL = 32;

	struct IndentInfo
	{
		std::bitset<MAX_INDENT_LEVEL> levels;
		int cur_level;

		IndentInfo() : levels(), cur_level(0) {}

		std::string MakeIndent() const
		{
			std::string result;
			result.reserve(4 * (cur_level + 1));
			result.push_back('|');
			if (UNLIKELY(cur_level != 0)) {
				constexpr std::string_view indent = "   ";
				result.append(indent);
				for (int i = 0; i < cur_level - 1; ++i) {
					result.push_back(levels.test(i) ? '|' : ' ');
					result.append(indent);
				}
				result.push_back(levels.test(cur_level - 1) ? '|' : '\\');
			}
			result.append("=> ");
			return result;
		}

		IndentInfo CreateChild(bool current_branch_has_next_elements) const
		{
			auto child = *this;
			if (cur_level < static_cast<int>(MAX_INDENT_LEVEL)) {
				child.levels.set(cur_level, current_branch_has_next_elements);
				++child.cur_level;
			}
			return child;
		}
	};


	template <typename T>
	inline void LogVarWithIndentation(std::ostringstream& log_stream,
								   std::string_view var_name,
								   const T& var_value,
								   const IndentInfo& indent_info,
								   bool print_value = true)
	{
		log_stream << indent_info.MakeIndent();
		log_stream << var_name;
		if (print_value)
			log_stream << " = " << var_value;
		log_stream << '\n';
	}



	// ****************************************************************************************************
	// DumpValue() - главная реализация-"диспетчер", обрабатывающая основные типы
	// ****************************************************************************************************

	template <typename T>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const T& value,
		const IndentInfo& indent_info = IndentInfo())
	{
		if constexpr (std::is_pointer_v<T>) {
			if (!value) {
				LogVarWithIndentation(log_stream, var_name, "(nullptr)", indent_info);
				return;
			}
		}

		if constexpr (std::is_pointer_v<T> &&
					  std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, unsigned char>) {
			DumpValue(log_stream, var_name, reinterpret_cast<const char*>(value), indent_info);

		} else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::wstring> ||
							 std::is_convertible_v<T, const wchar_t*>) {
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			try {
				DumpValue(log_stream, var_name, conv.to_bytes(value), indent_info);
			} catch (const std::range_error& e) {
				LogVarWithIndentation(log_stream, var_name, std::string("[conversion error: ") + e.what() + "]", indent_info);
			}

		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, char> ||
							 std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
							 std::is_same_v<std::remove_cv_t<T>, signed char>) {
			auto code = static_cast<unsigned char>(value);
			if (code >= 0x20 && code < 0x80) {
				LogVarWithIndentation(log_stream, var_name, static_cast<char>(value), indent_info);
			} else {
				char buffer[5];
				std::snprintf(buffer, sizeof(buffer), "0x%02X", code);
				LogVarWithIndentation(log_stream, var_name, std::string(buffer), indent_info);
			}

		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>) {
			std::wstring wstr_value {value };
			DumpValue(log_stream, var_name, wstr_value, indent_info);

		} else if constexpr (std::is_convertible_v<T, std::string_view>) {
			LogVarWithIndentation(log_stream, var_name, EscapeString(value), indent_info);

		} else if constexpr (is_container_v<T>) {
			LogVarWithIndentation(log_stream, var_name, nullptr, indent_info, false);
			std::size_t index = 0;
			auto it_begin = std::begin(value);
			auto it_end   = std::end(value);
			for (auto it = it_begin; it != it_end; ) {
				auto curr = it++;
				bool is_last = (it == it_end);
				auto child_indent_info = indent_info.CreateChild(!is_last);
				auto item_name = std::string(var_name) + "[" + std::to_string(index++) + "]";
				DumpValue(log_stream, item_name, *curr, child_indent_info);
			}

		} else {
			LogVarWithIndentation(log_stream, var_name, value, indent_info);
		}
	}

	// ****************************************************************************************************
	// Поддержка статических массивов char[], unsigned char[] и wchar_t[]
	// ****************************************************************************************************

	template <typename CharT, std::size_t N>
	inline std::enable_if_t<
		std::is_same_v<std::remove_cv_t<CharT>, char> ||
		std::is_same_v<std::remove_cv_t<CharT>, unsigned char> ||
		std::is_same_v<std::remove_cv_t<CharT>, wchar_t>
		>
	DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const CharT (&value)[N])
	{
		constexpr std::size_t size = N;
		if constexpr (std::is_same_v<std::remove_cv_t<CharT>, wchar_t>) {
			DumpValue(log_stream, var_name, std::wstring(value, size));
		} else if constexpr (std::is_same_v<std::remove_cv_t<CharT>, unsigned char>) {
			DumpValue(log_stream, var_name, std::string(reinterpret_cast<const char*>(value), size));
		} else {
			DumpValue(log_stream, var_name, std::string(value, size));
		}
	}


	// ****************************************************************************************************
	// Поддержка строковых буферов, доступных по паре (указатель, размер): через макросы DSTRBUF + DUMP
	// ****************************************************************************************************

	template <typename T>
	struct StrBufWrapper
	{
		StrBufWrapper(T *data, size_t length) : data(data), length(length) {}
		T *data;
		size_t length;
	};


	template <typename T>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const StrBufWrapper<T>& str_buf_wrapper)
	{
		if (!str_buf_wrapper.data) {
			log_stream << "|=> " << var_name << " = (nullptr)\n";
			return;
		}

		if constexpr (std::is_same_v<std::remove_cv_t<T>, char> ||
					  std::is_same_v<std::remove_cv_t<T>, unsigned char>) {
			std::string str_value ((char*)str_buf_wrapper.data, str_buf_wrapper.length);
			DumpValue(log_stream, var_name, str_value);
		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>) {
			std::wstring wstr_value (str_buf_wrapper.data, str_buf_wrapper.length);
			DumpValue(log_stream, var_name, wstr_value);
		} else {
			log_stream << "|=> " << var_name << " : ERROR, UNSUPPORTED TYPE!\n";
		}
	}

	// ****************************************************************************************************
	// Поддержка бинарных буферов, доступных по паре (указатель, размер в байтах): через макросы DBINBUF + DUMP
	// ****************************************************************************************************

	inline std::string CreateHexDump(const std::uint8_t* data, size_t length,
									 std::string_view line_prefix, size_t bytes_per_line = 16)
	{
		auto separator_pos = bytes_per_line / 2;

		std::ostringstream result;
		result << std::hex << std::setfill('0');

		for (size_t offset = 0; offset < length; offset += bytes_per_line) {
			result << line_prefix << std::setw(8)  << offset << "  ";
			for (size_t i = 0; i < bytes_per_line && (offset + i) < length; ++i) {
				if (i == separator_pos) {
					result << "| ";
				}
				result << std::setw(2) << static_cast<unsigned int>(data[offset + i]) << " ";
			}
			result << '\n';
		}

		return result.str();
	}


	template <typename T, typename = std::enable_if_t<std::is_pointer_v<T>>>
	struct BinBufWrapper
	{
		BinBufWrapper(T data, size_t length) : data(data), length(length) {}
		const T data;
		size_t length;
	};


	template <typename T, typename = std::enable_if_t<std::is_pointer_v<T>>>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const BinBufWrapper<T>& bin_buf_wrapper)
	{
		if (!bin_buf_wrapper.data) {
			log_stream << "|=> " << var_name << " = (nullptr)\n";
			return;
		}

		if (bin_buf_wrapper.length == 0) {
			log_stream << "|=> " << var_name << " = (empty)\n";
			return;
		}

		constexpr size_t MAX_LENGTH = 1024 * 1024;
		size_t effective_length = (bin_buf_wrapper.length > MAX_LENGTH)
									  ? MAX_LENGTH : bin_buf_wrapper.length;

		std::string hexDump = CreateHexDump(reinterpret_cast<const uint8_t*>(bin_buf_wrapper.data),
											effective_length, "|   ");

		log_stream << "|=> " << var_name << " =\n";
		log_stream << hexDump;

		if (bin_buf_wrapper.length > MAX_LENGTH) {
			log_stream << "|   Output truncated to " << effective_length
					   << " bytes (full length: " << bin_buf_wrapper.length << " bytes)\n";
		}
	}

	// ****************************************************************************************************
	// Поддержка итерируемых STL контейнеров и статических массивов: через макросы DCONT + DUMP
	// ****************************************************************************************************

	template <typename ContainerT, typename = std::enable_if_t<is_container_v<ContainerT>>>
	struct ContainerWrapper
	{
		ContainerWrapper(const ContainerT &data, size_t max_elements)
			: data(data), max_elements(max_elements) {}
		const ContainerT &data;
		size_t max_elements;
	};


	template <typename ContainerT>
	inline void DumpValue(
		std::ostringstream &log_stream,
		std::string_view var_name,
		const ContainerWrapper<ContainerT> &container_wrapper)
	{
		auto indent_info = IndentInfo();
		LogVarWithIndentation(log_stream, var_name, nullptr, indent_info, false);

		auto container_size = std::size(container_wrapper.data);
		size_t effective_size = (container_wrapper.max_elements == 0)
									  ? container_size
									  : std::min(container_size, container_wrapper.max_elements);

		std::size_t index = 0;
		for (const auto &item : container_wrapper.data) {
			bool is_last = (index == effective_size - 1);
			auto child_indent_info = indent_info.CreateChild(!is_last);
			auto item_name = std::string(var_name) + "[" + std::to_string(index++) + "]";
			DumpValue(log_stream, item_name, item, child_indent_info);
			if (is_last) break;
		}
		if (effective_size != container_size) {
			log_stream << "|   Output limited to " << effective_size
					   << " elements (total elements: " << container_size << ")\n";
		}
	}

	// ****************************************************************************************************
	// Поддержка флагов (битовые маски, etc): через макросы DFLAGS + DUMP; второй аргумент - Dumper::FlagsAs::...
	// ****************************************************************************************************

	enum class FlagsAs
	{
		FILE_ATTRIBUTES,
		UNIX_MODE
	};


	struct FlagsWrapper
	{
		unsigned long int value;
		FlagsAs type;
		FlagsWrapper(unsigned long int v, FlagsAs t)
			: value(v), type(t) {}
	};


	template <std::size_t N>
	std::string FlagsToString(unsigned long int input,
							  const unsigned long int (&flag_values)[N],
							  const std::string_view (&flag_names)[N])
	{
		std::string result;
		bool first_in_list = true;
		for (std::size_t i = 0; i < N; ++i) {
			if (input & flag_values[i]) {
				if (!first_in_list)
					result.append(", ");
				result.append(flag_names[i]);
				first_in_list = false;
			}
		}
		if (result.empty())
			return "[none]";
		return result;
	}


	inline std::string DecodeFileAttributes(unsigned long int flags)
	{
		constexpr unsigned long int f_attr[] = {
			0x00000001, 0x00000002, 0x00000004, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000100,
			0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000,
			0x00020000, 0x00200000, 0x00400000, 0x00800000, 0x01000000, 0x02000000, 0x08000000};

		constexpr std::string_view s_attr[] = {
			"READONLY", "HIDDEN", "SYSTEM", "DIRECTORY", "ARCHIVE", "DEVICE_BLOCK", "NORMAL", "TEMPORARY",
			"SPARSE_FILE", "REPARSE_POINT", "COMPRESSED", "OFFLINE", "NOT_CONTENT_INDEXED", "ENCRYPTED",
			"INTEGRITY_STREAM", "VIRTUAL", "NO_SCRUB_DATA", "BROKEN", "EXECUTABLE", "DEVICE_CHAR", "DEVICE_FIFO",
			"DEVICE_SOCK", "HARDLINKS"};

		return FlagsToString(flags, f_attr, s_attr);
	}


	inline std::string DecodeUnixMode(unsigned long int flags)
	{
		std::ostringstream result;
		result << std::oct << flags << " ("
			<< ((flags & S_IRUSR) ? "r" : "-")
			<< ((flags & S_IWUSR) ? "w" : "-")
			<< ((flags & S_IXUSR) ? "x" : "-")
			<< ((flags & S_IRGRP) ? "r" : "-")
			<< ((flags & S_IWGRP) ? "w" : "-")
			<< ((flags & S_IXGRP) ? "x" : "-")
			<< ((flags & S_IROTH) ? "r" : "-")
			<< ((flags & S_IWOTH) ? "w" : "-")
			<< ((flags & S_IXOTH) ? "x" : "-");
		if(flags & S_ISUID) result << " suid";
		if(flags & S_ISGID) result << " sgid";
		if(flags & S_ISVTX) result << " sticky";
		result << ")";
		return result.str();
	}


	inline void DumpValue(std::ostringstream& log_stream, std::string_view var_name, const FlagsWrapper& df)
	{
		std::string decoded;
		switch (df.type) {
		case FlagsAs::FILE_ATTRIBUTES:
			decoded = DecodeFileAttributes(df.value);
			break;
		case FlagsAs::UNIX_MODE:
			decoded = DecodeUnixMode(df.value);
			break;
		default:
			decoded = "[not implemented]";
		}
		log_stream << "|=> " << var_name << " = " << decoded << '\n';
	}

	// ****************************************************************************************************
	// Вспомогательный код для работы с логом
	// ****************************************************************************************************


	inline std::string CreateLogHeader(std::string_view func_name, std::string_view location)
	{
		auto current_time = std::chrono::system_clock::now();
		auto current_time_t = std::chrono::system_clock::to_time_t(current_time);
		std::tm local_time{};
		localtime_r(&current_time_t, &local_time);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()) % 1000;

		std::ostringstream header_stream;
		header_stream << "\n/-----[PID:" << getpid() << ", TID:" << GetNiceThreadId() << "]-----[";
		header_stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << ',' << ms.count() << "]-----\n";
		header_stream << "|[" << location << "] in " << func_name << "()\n";
		return header_stream.str();
	}


	inline void FlushLog(std::ostringstream& log_stream, bool to_file)
	{
		std::string log_entry = log_stream.str();
		std::lock_guard<std::mutex> lock(g_log_output_mutex);

		if (to_file) {
			std::ofstream log_file(GetHomeDir() + "/far2l_debug.log", std::ios::app);
			if (log_file) {
				log_file << log_entry << std::endl;
			}
		} else {
			std::clog << log_entry << std::endl;
		}
	}

	// ****************************************************************************************************
	// Бэкенд для макроса DUMP: аргументы заключаются в дополнительные макросы DVV, DBUF, DCONT, DMSG...
	// ****************************************************************************************************

	template<std::size_t... I, typename NameValueTupleT>
	void ProcessPairs(const NameValueTupleT& name_value_tuple, std::ostringstream & log_stream, std::index_sequence<I...>)
	{

		(DumpValue(log_stream, std::get<2 * I>(name_value_tuple), std::get<2 * I + 1>(name_value_tuple)), ...);
	}


	template<typename... Args>
	void Dump(bool to_file, std::string_view func_name, std::string_view location, const Args&... args)
	{
		static_assert(sizeof...(args) % 2 == 0, "Dump() expects arguments in pairs: name and value.");

		std::ostringstream log_stream;
		log_stream << CreateLogHeader(func_name, location);

		auto args_tuple = std::forward_as_tuple(args...);
		constexpr std::size_t pair_count = sizeof...(args) / 2;
		ProcessPairs(args_tuple, log_stream, std::make_index_sequence<pair_count>{});
		FlushLog(log_stream, to_file);
	}

	// ****************************************************************************************************
	// Бэкенд для макроса DUMPV: поддержка дампинга только переменных (без вызовов функций, макросов и сложных выражений)
	// ****************************************************************************************************

	template <std::size_t... I, typename ValuesTupleT>
	void DumpEachVariable(const std::vector<std::string>& var_names, std::ostringstream& log_stream,
						  ValuesTupleT& values_tuple, std::index_sequence<I...>)
	{
		(DumpValue(log_stream, var_names[I], std::get<I>(values_tuple)), ...);
	}


	inline bool TryParseVariableNames(const char *var_names_str, std::vector<std::string> &var_names, size_t expected_count)
	{
		if (!var_names_str || std::strchr(var_names_str, '(') != nullptr) {
			return false;
		}

		std::istringstream var_names_stream(var_names_str);
		std::string name_token;
		while (std::getline(var_names_stream, name_token, ',')) {
			size_t start = name_token.find_first_not_of(" \t");
			size_t end = name_token.find_last_not_of(" \t");
			if(start != std::string::npos && end != std::string::npos)
				var_names.emplace_back(name_token.substr(start, end - start + 1));
		}

		if (var_names.size() != expected_count) {
			return false;
		}

		return true;
	}


	inline void ReportDumpVError(std::ostringstream &log_stream)
	{
		const std::string error_message =
			"dumpv: Only simple variables are allowed as arguments. "
			"Function calls or complex expressions with internal commas are not supported.";
		DumpValue(log_stream, "ERROR", error_message);
	}


	template<typename... Ts>
	void DumpV(
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		const char *var_names_str,
		const Ts&... var_values)
	{
		std::ostringstream log_stream;
		log_stream << CreateLogHeader(func_name, location);

		constexpr auto var_values_count = sizeof...(var_values);
		std::vector<std::string> var_names;

		if (TryParseVariableNames(var_names_str, var_names, var_values_count)) {
			auto values_tuple = std::forward_as_tuple(var_values...);
			DumpEachVariable(var_names, log_stream, values_tuple, std::make_index_sequence<var_values_count>{});
		} else {
			ReportDumpVError(log_stream);
		}

		FlushLog(log_stream, to_file);
	}

} // end namespace Dumper

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define LOCATION (__FILE__ ":" STRINGIZE_VALUE_OF(__LINE__))

#define DUMP(to_file, ...) { Dumper::Dump(to_file, __func__, LOCATION, __VA_ARGS__); }
#define DUMPV(to_file, ...) { Dumper::DumpV(to_file, __func__, LOCATION, #__VA_ARGS__, __VA_ARGS__); }

#define DVV(expr) #expr, expr
#define DMSG(msg) "msg", std::string(msg)
#define DBINBUF(ptr,length) #ptr, Dumper::BinBufWrapper(ptr,length)
#define DSTRBUF(ptr,length) #ptr, Dumper::StrBufWrapper(ptr,length)
#define DCONT(container,max_elements) #container, Dumper::ContainerWrapper(container,max_elements)
#define DFLAGS(var, treat_as) #var, Dumper::FlagsWrapper(var, treat_as)
