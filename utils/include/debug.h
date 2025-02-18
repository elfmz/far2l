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
#include <unordered_map>

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
	inline std::mutex g_log_output_mutex;

	inline std::size_t g_thread_idx = 0;
	inline std::mutex g_thread_mutex;
	inline std::unordered_map<std::thread::id, std::size_t> g_thread_ids;


	inline std::string EscapeString(std::string_view input)
	{
		std::ostringstream output;

		for (unsigned char c : input) {
			if (c >= '\x20') {
				switch (c) {
				case '"':  output << "\\\""; break;
				case '\\': output << "\\\\"; break;
				default:
					output << c;
					break;
				}
			} else {
				switch (c) {
				case '\t': output << "\\t"; break;
				case '\r': output << "\\r"; break;
				case '\n': output << "\\n"; break;
				case '\a': output << "\\a"; break;
				case '\b': output << "\\b"; break;
				case '\v': output << "\\v"; break;
				case '\f': output << "\\f"; break;
				case '\e': output << "\\e"; break;
				default:
					output << "\\x{" << std::setfill('0') << std::setw(2) << std::right
						   << std::hex << static_cast<unsigned int>(c) << "}";
					break;
				}
			}
		}
		return output.str();
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


	inline std::size_t GetNiceThreadId() noexcept {
		static thread_local std::size_t s_nice_thread_id = [] {
			std::lock_guard<std::mutex> lock(g_thread_mutex);
			std::thread::id thread_id = std::this_thread::get_id();
			auto iter = g_thread_ids.find(thread_id);
			if (iter == g_thread_ids.end()) {
				iter = g_thread_ids.insert({ thread_id, g_thread_idx++ }).first;
			}
			return iter->second;
		}();
		return s_nice_thread_id;
	}


	template <typename T>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const T& value)
	{
		if constexpr (std::is_pointer_v<T>) {
			if (value == nullptr) {
				log_stream << "|=> " << var_name << " = (nullptr)" << std::endl;
				return;
			}

			if constexpr ( std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, unsigned char> ) {
				DumpValue(log_stream, var_name, reinterpret_cast<const char*>(value));
				return;
			}
		}

		if constexpr (std::is_convertible_v<T, const wchar_t*>) {
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			try {
				DumpValue(log_stream, var_name, conv.to_bytes(value));
			} catch (const std::range_error& e) {
				log_stream << "|=> " << var_name << " = [conversion error: "  << e.what() << "]" << std::endl;
			}
			return;
		}

		if constexpr (std::is_convertible_v<T, std::string_view> ||
						std::is_same_v<std::remove_cv_t<T>, char> ||
						std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
						std::is_same_v<std::remove_cv_t<T>, wchar_t>) {

			std::string str_value{ value };
			std::string escaped = EscapeString(str_value);
			log_stream << "|=> " << var_name << " = " << escaped << std::endl;
		} else {
			log_stream << "|=> " << var_name << " = " << value << std::endl;
		}
	}


	template <>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const std::wstring& value)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		try {
			DumpValue(log_stream, var_name, conv.to_bytes(value));
		} catch (const std::range_error& e) {
			log_stream << "|=> " << var_name << " = [conversion error: "  << e.what() << "]" << std::endl;
		}
		return;
	}


	// Поддержка статических массивов char[], unsigned char[] и wchar_t[]

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


	// Поддержка строковых буферов, доступных по паре (указатель, размер): через макросы DBUF + DUMP

	template <typename T>
	struct BufferWrapper {
		BufferWrapper(T *data, size_t length) : data(data), length(length) {}
		T *data;
		size_t length;
	};


	template <typename T>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const BufferWrapper<T>& buffer_wrapper)
	{
		if (buffer_wrapper.data == nullptr) {
			log_stream << "|=> " << var_name << " = (nullptr)" << std::endl;
			return;
		}

		if constexpr (std::is_same_v<std::remove_cv_t<T>, char> ||
					  std::is_same_v<std::remove_cv_t<T>, unsigned char>) {
			std::string str_value ((char*)buffer_wrapper.data, buffer_wrapper.length);
			DumpValue(log_stream, var_name, str_value);
		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>) {
			std::wstring wstr_value (buffer_wrapper.data, buffer_wrapper.length);
			DumpValue(log_stream, var_name, wstr_value);
		} else {
			log_stream << "|=> " << var_name << " : ERROR, UNSUPPORTED TYPE!" << std::endl;
		}
	}

	// Поддержка контейнеров с итераторами: через макросы DCONT + DUMP

	template <typename ContainerT, typename = decltype(std::begin(std::declval<ContainerT>())),
		 typename = decltype(std::end(std::declval<ContainerT>()))>
	struct ContainerWrapper {
	  ContainerWrapper(const ContainerT& data, size_t max_elements)
			: data(data), max_elements(max_elements) {}
	  const ContainerT& data;
	  size_t max_elements;
	};


	template <typename ContainerT>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const ContainerWrapper<ContainerT>& container_wrapper)
	{
		std::size_t index = 0;
		for (const auto &item : container_wrapper.data) {
			if (container_wrapper.max_elements > 0 && index >= container_wrapper.max_elements)
				break;
			auto item_name = std::string(var_name) + "[" + std::to_string(index++) + "]";
			DumpValue(log_stream, item_name, item);
		}
	}


	// Поддержка статических массивов: через макросы DCONT + DUMP

	template <typename T, std::size_t N>
	struct ContainerWrapper<T (&)[N]> {
	  ContainerWrapper(const T (&data)[N], size_t max_elements)
		: data(data), max_elements(max_elements) {}
	  const T (&data)[N];
	  size_t max_elements;
	};


	template <typename T, std::size_t N>
	inline void DumpValue(std::ostringstream& log_stream, std::string_view var_name,
						   const ContainerWrapper<T (&)[N]>& container_wrapper)
	{
		size_t effective = (container_wrapper.max_elements > 0 && container_wrapper.max_elements < N
								? container_wrapper.max_elements : N);

		for (std::size_t index = 0; index < effective; ++index) {
			auto item_name = std::string(var_name) + "[" + std::to_string(index) + "]";
			DumpValue(log_stream, item_name, container_wrapper.data[index]);
		}
	}


	template <typename T, std::size_t N>
	ContainerWrapper(const T (&)[N], size_t) -> ContainerWrapper<T (&)[N]>;




	inline std::string FormatLogHeader(pid_t pid, unsigned long int tid,
								  std::string_view func_name,
								  std::string_view location)
	{
		auto current_time = std::chrono::system_clock::now();
		auto current_time_t = std::chrono::system_clock::to_time_t(current_time);
		std::tm local_time{};
		localtime_r(&current_time_t, &local_time);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()) % 1000;

		std::ostringstream header_stream;
		header_stream << std::endl << "/-----[PID:" << pid << ", TID:" << tid << "]-----[";

		header_stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << ',' << ms.count() << "]-----" << std::endl;
		header_stream << "|[" << location << "] in " << func_name << "()" << std::endl;
		return header_stream.str();
	}


	inline void FlushLog(std::ostringstream& log_stream, bool to_file)
	{
		std::string log_entry = log_stream.str();

		std::lock_guard<std::mutex> lock(g_log_output_mutex);

		if (to_file) {
			std::ofstream(GetHomeDir() + "/far2l_debug.log", std::ios::app) << log_entry << std::endl;
		} else {
			std::clog << log_entry << std::endl;
		}
	}

	// Бэкенд для макроса DUMP: аргументы заключаются в дополнительные макросы DVV, DBUF, DCONT, DMSG...

	template<std::size_t... I, typename NameValueTupleT>
	void ProcessPairs(const NameValueTupleT& name_value_tuple, std::ostringstream & log_stream, std::index_sequence<I...>)
	{

		(DumpValue(log_stream, std::get<2 * I>(name_value_tuple), std::get<2 * I + 1>(name_value_tuple)), ...);
	}


	template<typename... Args>
	void Dump(bool to_file, std::string_view func_name, std::string_view location,
			  pid_t pid, unsigned long int tid, const Args&... args)
	{
		static_assert(sizeof...(args) % 2 == 0, "Dump() expects arguments in pairs: name and value.");

		std::ostringstream log_stream;
		log_stream << FormatLogHeader(pid, tid, func_name, location);

		auto args_tuple = std::forward_as_tuple(args...);
		constexpr std::size_t pair_count = sizeof...(args) / 2;
		ProcessPairs(args_tuple, log_stream, std::make_index_sequence<pair_count>{});
		FlushLog(log_stream, to_file);
	}


	// Бэкенд для макроса DUMPV: поддержка дампинга только переменных (без вызовов функций, макросов и сложных выражений)

	template <std::size_t... I, typename ValuesTupleT>
	void DumpEachVariable(const std::vector<std::string>& var_names, std::ostringstream& log_stream,
						  ValuesTupleT& values_tuple, std::index_sequence<I...>)
	{
		(DumpValue(log_stream, var_names[I], std::get<I>(values_tuple)), ...);
	}


	inline bool TryParseVariableNames(const char *var_names_str, std::vector<std::string> &var_names, size_t var_values_count)
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

		if (var_names.size() != var_values_count) {
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
		pid_t pid,
		unsigned long int tid,
		const char *var_names_str,
		const Ts&... var_values)
	{
		std::ostringstream log_stream;
		log_stream << FormatLogHeader(pid, tid, func_name, location);

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

#define NICE_THREAD_ID
#ifdef NICE_THREAD_ID
#define DUMP_THREAD Dumper::GetNiceThreadId()
#else
#define DUMP_THREAD std::hash<std::thread::id>{}(std::this_thread::get_id())
#endif

#define DUMP(to_file, ...) { Dumper::Dump(to_file, __func__, LOCATION, getpid(), DUMP_THREAD, __VA_ARGS__); }
#define DUMPV(to_file, ...) { Dumper::DumpV(to_file, __func__, LOCATION, getpid(), DUMP_THREAD, #__VA_ARGS__, __VA_ARGS__); }

#define DVV(xxx) #xxx, xxx
#define DMSG(xxx) "msg", std::string(xxx)
#define DBUF(ptr,length) #ptr, Dumper::BufferWrapper(ptr,length)
#define DCONT(container,max_elements) #container, Dumper::ContainerWrapper(container,max_elements)
