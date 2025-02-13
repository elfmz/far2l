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
	inline std::mutex dumper_mutex;


	inline std::string escape_string(std::string_view input)
	{
		std::ostringstream output;

		for (unsigned char c : input) {

			if (c > '\x1F') {
				output << c;
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
				// case '\\': output << "\\\\"; break;
				default:
					output << "\\x{" << std::setfill('0') << std::setw(2) << std::right<< std::hex << static_cast<unsigned int>(c) << "}";
					break;
				}
			}
		}
		return output.str();
	}


	inline const std::string& getHomeDir()
	{
		static std::string home;
		static std::once_flag flag;
		std::call_once(flag, [] {
			const char* envHome = std::getenv("HOME");
			home = (envHome != nullptr) ? std::string(envHome) : std::string("");
		});
		return home;
	}


	template <typename T>
	inline void dump_value(
		std::ostringstream& oss,
		std::string_view var_name,
		const T& value)
	{
		if constexpr (std::is_pointer_v<T>) {
			if (value == nullptr) {
				oss << "|=> " << var_name << " = (nullptr)" << std::endl;
				return;
			}

			if constexpr ( std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, unsigned char> ) {
				dump_value(oss, var_name, reinterpret_cast<const char*>(value));
				return;
			}

			if constexpr (std::is_convertible_v<T, const wchar_t*>) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
				try {
					dump_value(oss, var_name, conv.to_bytes(value));
				} catch (const std::range_error& e) {
					oss << "|=> " << var_name << " = [conversion error: "  << e.what() << "]" << std::endl;
				}
				return;
			}
		}

		if constexpr (std::is_convertible_v<T, std::string_view> ||
						std::is_same_v<std::remove_cv_t<T>, char> ||
						std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
						std::is_same_v<std::remove_cv_t<T>, wchar_t>) {

			std::string s_value{ value };
			std::string escaped = escape_string(s_value);
			oss << "|=> " << var_name << " = " << escaped << std::endl;
		} else {
			oss << "|=> " << var_name << " = " << value << std::endl;
		}
	}


	template <>
	inline void dump_value(
		std::ostringstream& oss,
		std::string_view var_name,
		const std::wstring& value)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		dump_value(oss, var_name, conv.to_bytes(value));
	}


	// ********** поддержка строковых буферов, доступных по указателю/размерности

	template <typename T>
	struct DumpBuffer {
		DumpBuffer(T* data, size_t length) : data(data), length(length) {}
		T* data;
		size_t length;
	};


	template <typename T>
	inline void dump_value(
		std::ostringstream& oss,
		std::string_view var_name,
		const DumpBuffer<T>& buffer)
	{

		if constexpr (std::is_same_v<std::remove_cv_t<T>, char> || std::is_same_v<std::remove_cv_t<T>, unsigned char>) {
			std::string s_value ((char*)buffer.data, buffer.length);
			dump_value(oss, var_name, s_value);
		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>) {
			std::wstring ws_value (buffer.data, buffer.length);
			dump_value(oss, var_name, ws_value);
		} else {
			oss << "|=> " << var_name << " : ERROR, UNSUPPORTED TYPE!" << std::endl;
		}
	}

	// ********** поддержка контейнеров с итераторами

	template <typename Container, typename = decltype(std::begin(std::declval<Container>())),
		 typename = decltype(std::end(std::declval<Container>()))>
	struct DumpContainer {
	  DumpContainer(const Container& data, size_t maxlength) : data(data), maxlength(maxlength) {}
	  const Container& data;
	  size_t maxlength;
	};


	template <typename Container>
	inline void dump_value(
	  std::ostringstream& oss,
	  std::string_view var_name,
	  const DumpContainer<Container>& container)
	{
	  std::size_t index = 0;
	  for (const auto &item : container.data) {
		if (container.maxlength > 0 && index >= container.maxlength)
		  break;
		auto itemName = std::string(var_name) + "[" + std::to_string(index++) + "]";
		dump_value(oss, itemName, item);
	  }
	}

	// ********** поддержка статических массивов

	template <typename T, std::size_t N>
	struct DumpContainer<T (&)[N]> {
	  DumpContainer(const T (&data)[N], size_t maxlength)
		: data(data), maxlength(maxlength) {}
	  const T (&data)[N];
	  size_t maxlength;
	};

	template <typename T, std::size_t N>
	inline void dump_value(std::ostringstream& oss, std::string_view var_name, const DumpContainer<T (&)[N]>& container)
	{
	  size_t effective = (container.maxlength > 0 && container.maxlength < N ? container.maxlength : N);
	  for (std::size_t index = 0; index < effective; ++index) {
		auto itemName = std::string(var_name) + "[" + std::to_string(index) + "]";
		dump_value(oss, itemName, container.data[index]);
	  }
	}


	template <typename T, std::size_t N>
	DumpContainer(const T (&)[N], size_t) -> DumpContainer<T (&)[N]>;

	// **********

	template<typename T, typename... Args>
	void dump(
		std::ostringstream& oss,
		bool to_file,
		bool firstcall,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		std::string_view var_name,
		const T& value,
		const Args&... args)
	{
		if (firstcall) {
			auto now = std::chrono::system_clock::now();
			auto time_t_now = std::chrono::system_clock::to_time_t(now);
			std::tm tm_now{};
			localtime_r(&time_t_now, &tm_now);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

			char buffer[80];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_now);
			std::string time_str = std::string(buffer);
			oss << std::endl << "/-----[PID:" << pID << ", TID:" << tID << "]-----[" << time_str << ","<< ms.count() << "]-----" << std::endl;
			oss << "|[" << location << "] in "  << func_name << "()"  << std::endl;
		}

		dump_value(oss, var_name, value);

		if constexpr (sizeof...(args) > 0) {
			dump(oss, to_file, false, func_name, location, pID, tID, args...);
		} else {
			std::string log_entry = oss.str();

			std::lock_guard<std::mutex> lock(dumper_mutex);

			if (to_file) {
				std::ofstream(getHomeDir() + "/far2l_debug.log", std::ios::app) << log_entry << std::endl;
			} else {
				std::clog << log_entry << std::endl;
			}
		}
	}

	// ********** поддержка дампинга только переменных

	template<typename ValuesTuple, std::size_t... I>
	void dumpWrapperImpl(
		std::ostringstream& oss,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const std::vector<std::string>& varNames,
		ValuesTuple&& varValues,
		std::index_sequence<I...>)
	{
		auto interleaved = std::tuple_cat(
			std::make_tuple(std::string_view(varNames[I]),
			std::get<I>(std::forward<ValuesTuple>(varValues)))...);
		std::apply([&](auto&&... interleavedArgs) {
			dump(oss, to_file, true, func_name, location, pID, tID, interleavedArgs...);
			}, interleaved);
	}


	template<typename ValuesTuple>
	void dumpWrapper(
		std::ostringstream& oss,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const std::vector<std::string>& varNames,
		ValuesTuple&& varValues)
	{
		constexpr auto N = std::tuple_size<std::decay_t<ValuesTuple>>::value;
		dumpWrapperImpl(oss, to_file, func_name, location, pID, tID, varNames,
						std::forward<ValuesTuple>(varValues), std::make_index_sequence<N>{});
	}


	template<typename... Ts>
	void dumpv(
		std::ostringstream& oss,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const char* varNamesStr,
		const Ts&... args)
	{
		std::vector<std::string> varNames;
		std::istringstream iss(varNamesStr);
		std::string token;
		while (std::getline(iss, token, ','))
		{
			size_t start = token.find_first_not_of(" \t");
			size_t end = token.find_last_not_of(" \t");
			if(start != std::string::npos && end != std::string::npos)
				varNames.push_back(token.substr(start, end - start + 1));
		}
		auto varValues = std::tie(args...);
		dumpWrapper(oss, to_file, func_name, location, pID, tID, varNames, varValues);
	}

} // end namespace Dumper

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define LOCATION (__FILE__ ":" STRINGIZE_VALUE_OF(__LINE__))

#ifdef GET_INTER_THREAD_ID
#define DUMP_THREAD GetInterThreadID()
#else
#define DUMP_THREAD 0 /*gettid()*/
#endif

#define DUMP(to_file, ...) { std::ostringstream oss; Dumper::dump(oss, to_file, true, __func__, LOCATION, getpid(), DUMP_THREAD, __VA_ARGS__); }
#define DUMPV(to_file, ...) { std::ostringstream oss; Dumper::dumpv(oss, to_file, __func__, LOCATION, getpid(), DUMP_THREAD, #__VA_ARGS__, __VA_ARGS__); }

#define DVV(xxx) #xxx, xxx
#define DMSG(xxx) "msg", xxx
#define DBUF(ptr,length) #ptr, Dumper::DumpBuffer(ptr,length)
#define DCONT(container,maxlength) #container, Dumper::DumpContainer(container,maxlength)
