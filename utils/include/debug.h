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
	inline std::mutex logOutputMutex;


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
		static std::string home;
		static std::once_flag flag;
		std::call_once(flag, [] {
			const char* envHome = std::getenv("HOME");
			home = (envHome != nullptr) ? std::string(envHome) : std::string("/tmp");
		});
		return home;
	}


	template <typename T>
	inline void dump_value(
		std::ostringstream& logStream,
		std::string_view var_name,
		const T& value)
	{
		if constexpr (std::is_pointer_v<T>) {
			if (value == nullptr) {
				logStream << "|=> " << var_name << " = (nullptr)" << std::endl;
				return;
			}

			if constexpr ( std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, unsigned char> ) {
				dump_value(logStream, var_name, reinterpret_cast<const char*>(value));
				return;
			}
		}

		if constexpr (std::is_convertible_v<T, const wchar_t*>) {
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			try {
				dump_value(logStream, var_name, conv.to_bytes(value));
			} catch (const std::range_error& e) {
				logStream << "|=> " << var_name << " = [conversion error: "  << e.what() << "]" << std::endl;
			}
			return;
		}

		if constexpr (std::is_convertible_v<T, std::string_view> ||
						std::is_same_v<std::remove_cv_t<T>, char> ||
						std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
						std::is_same_v<std::remove_cv_t<T>, wchar_t>) {

			std::string s_value{ value };
			std::string escaped = EscapeString(s_value);
			logStream << "|=> " << var_name << " = " << escaped << std::endl;
		} else {
			logStream << "|=> " << var_name << " = " << value << std::endl;
		}
	}


	template <>
	inline void dump_value(
		std::ostringstream& logStream,
		std::string_view var_name,
		const std::wstring& value)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		try {
			dump_value(logStream, var_name, conv.to_bytes(value));
		} catch (const std::range_error& e) {
			logStream << "|=> " << var_name << " = [conversion error: "  << e.what() << "]" << std::endl;
		}
		return;
	}


	// ********** поддержка (unsigned) char[] и wchar_t[]
	template <typename CharT, std::size_t N>
	inline std::enable_if_t<
		std::is_same_v<std::remove_cv_t<CharT>, char> ||
		std::is_same_v<std::remove_cv_t<CharT>, unsigned char> ||
		std::is_same_v<std::remove_cv_t<CharT>, wchar_t>
		>
	dump_value(
		std::ostringstream& logStream,
		std::string_view var_name,
		const CharT (&value)[N])
	{
		constexpr std::size_t size = N;
		if constexpr (std::is_same_v<std::remove_cv_t<CharT>, wchar_t>) {
			dump_value(logStream, var_name, std::wstring(value, size));
		} else if constexpr (std::is_same_v<std::remove_cv_t<CharT>, unsigned char>) {
			dump_value(logStream, var_name, std::string(reinterpret_cast<const char*>(value), size));
		} else { // тип char
			dump_value(logStream, var_name, std::string(value, size));
		}
	}


	// ********** поддержка строковых буферов, доступных по указателю/размерности

	template <typename T>
	struct BufferWrapper {
		BufferWrapper(T* data, size_t length) : data(data), length(length) {}
		T* data;
		size_t length;
	};


	template <typename T>
	inline void dump_value(
		std::ostringstream& logStream,
		std::string_view var_name,
		const BufferWrapper<T>& buffer)
	{
		if (buffer.data == nullptr) {
			logStream << "|=> " << var_name << " = (nullptr)" << std::endl;
			return;
		}

		if constexpr (std::is_same_v<std::remove_cv_t<T>, char> || std::is_same_v<std::remove_cv_t<T>, unsigned char>) {
			std::string s_value ((char*)buffer.data, buffer.length);
			dump_value(logStream, var_name, s_value);
		} else if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>) {
			std::wstring ws_value (buffer.data, buffer.length);
			dump_value(logStream, var_name, ws_value);
		} else {
			logStream << "|=> " << var_name << " : ERROR, UNSUPPORTED TYPE!" << std::endl;
		}
	}

	// ********** поддержка контейнеров с итераторами

	template <typename Container, typename = decltype(std::begin(std::declval<Container>())),
		 typename = decltype(std::end(std::declval<Container>()))>
	struct ContainerWrapper {
	  ContainerWrapper(const Container& data, size_t maxElements)
			: data(data), maxElements(maxElements) {}
	  const Container& data;
	  size_t maxElements;
	};


	template <typename Container>
	inline void dump_value(
		std::ostringstream& logStream,
		std::string_view var_name,
		const ContainerWrapper<Container>& container)
	{
		std::size_t index = 0;
		for (const auto &item : container.data) {
			if (container.maxElements > 0 && index >= container.maxElements)
				break;
			auto itemName = std::string(var_name) + "[" + std::to_string(index++) + "]";
			dump_value(logStream, itemName, item);
		}
	}


	// ********** поддержка статических массивов

	template <typename T, std::size_t N>
	struct ContainerWrapper<T (&)[N]> {
	  ContainerWrapper(const T (&data)[N], size_t maxElements)
		: data(data), maxElements(maxElements) {}
	  const T (&data)[N];
	  size_t maxElements;
	};


	template <typename T, std::size_t N>
	inline void dump_value(std::ostringstream& logStream, std::string_view var_name,
						   const ContainerWrapper<T (&)[N]>& container)
	{
		size_t effective = (container.maxElements > 0 && container.maxElements < N ? container.maxElements : N);
		for (std::size_t index = 0; index < effective; ++index) {
			auto itemName = std::string(var_name) + "[" + std::to_string(index) + "]";
			dump_value(logStream, itemName, container.data[index]);
		}
	}


	template <typename T, std::size_t N>
	ContainerWrapper(const T (&)[N], size_t) -> ContainerWrapper<T (&)[N]>;


	// **********

	inline std::string FormatLogHeader(pid_t pID, unsigned int tID,
								  std::string_view func_name,
								  std::string_view location)
	{
		auto currentTime = std::chrono::system_clock::now();
		auto currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
		std::tm localTime{};
		localtime_r(&currentTimeT, &localTime);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()) % 1000;

		std::ostringstream headerStream;
		headerStream << std::endl << "/-----[PID:" << pID << ", TID:" << tID << "]-----[";

		headerStream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ',' << ms.count() << "]-----" << std::endl;
		headerStream << "|[" << location << "] in " << func_name << "()" << std::endl;
		return headerStream.str();
	}


	template<typename T, typename... Args>
	void dump(
		std::ostringstream& logStream, bool to_file, bool firstcall, std::string_view func_name,
		std::string_view location, pid_t pID, unsigned int tID, std::string_view var_name,
		const T& value, const Args&... args)
	{
		if (firstcall) {
			logStream << FormatLogHeader(pID, tID, func_name, location);
		}

		dump_value(logStream, var_name, value);

		if constexpr (sizeof...(args) > 0) {
			dump(logStream, to_file, false, func_name, location, pID, tID, args...);
		} else {
			std::string log_entry = logStream.str();

			std::lock_guard<std::mutex> lock(logOutputMutex);

			if (to_file) {
				std::ofstream(GetHomeDir() + "/far2l_debug.log", std::ios::app) << log_entry << std::endl;
			} else {
				std::clog << log_entry << std::endl;
			}
		}
	}


	// ********** поддержка дампинга только переменных

	template<typename ValuesTuple, std::size_t... I>
	void dumpWrapperImpl(
		std::ostringstream& logStream,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const std::vector<std::string>& varNames,
		ValuesTuple&& varValues,
		std::index_sequence<I...>)
	{
		auto nameValuePairs = std::tuple_cat(
			std::make_tuple(std::string_view(varNames[I]),
			std::cref(std::get<I>(std::forward<ValuesTuple>(varValues))))...
			);
		std::apply([&](auto&&... nameValuePairArgs) {
			dump(logStream, to_file, true, func_name, location, pID, tID, nameValuePairArgs...);
		}, nameValuePairs);
	}


	template<typename ValuesTuple>
	void dumpWrapper(
		std::ostringstream& logStream,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const std::vector<std::string>& varNames,
		ValuesTuple&& varValues)
	{
		constexpr auto N = std::tuple_size<std::decay_t<ValuesTuple>>::value;
		dumpWrapperImpl(logStream, to_file, func_name, location, pID, tID, varNames,
						std::forward<ValuesTuple>(varValues), std::make_index_sequence<N>{});
	}


	template<typename... Ts>
	void dumpv(
		std::ostringstream& logStream,
		bool to_file,
		std::string_view func_name,
		std::string_view location,
		pid_t pID,
		unsigned int tID,
		const char* varNamesStr,
		const Ts&... varValuesArgs)
	{
		std::vector<std::string> varNames;
		std::istringstream varNamesStream(varNamesStr);
		std::string nameToken;
		while (std::getline(varNamesStream, nameToken, ','))
		{
			size_t start = nameToken.find_first_not_of(" \t");
			size_t end = nameToken.find_last_not_of(" \t");
			if(start != std::string::npos && end != std::string::npos)
				varNames.push_back(nameToken.substr(start, end - start + 1));
		}
		constexpr auto varValuesCount = sizeof...(varValuesArgs);
		if (varNames.size() != varValuesCount) {
			std::string errorMessage =
				"dumpv: Mismatch between parsed variable names count (" + std::to_string(varNames.size()) +
				") and passed arguments (" + std::to_string(varValuesCount) + "). " +
				"Only simple variables are supported as arguments. " +
				"Function calls or complex expressions with internal commas are not supported.";

			std::vector<std::string> errorNames = { "ERROR" };
			auto errorTuple = std::make_tuple(errorMessage);
			dumpWrapper(logStream, to_file, func_name, location, pID, tID, errorNames, errorTuple);
			return;
		}

		auto varValues = std::forward_as_tuple(varValuesArgs...);
		dumpWrapper(logStream, to_file, func_name, location, pID, tID, varNames, varValues);
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

#define DUMP(to_file, ...) { std::ostringstream logStream; Dumper::dump(logStream, to_file, true, __func__, LOCATION, getpid(), DUMP_THREAD, __VA_ARGS__); }
#define DUMPV(to_file, ...) { std::ostringstream logStream; Dumper::dumpv(logStream, to_file, __func__, LOCATION, getpid(), DUMP_THREAD, #__VA_ARGS__, __VA_ARGS__); }

#define DVV(xxx) #xxx, xxx
#define DMSG(xxx) "msg", std::string(xxx)
#define DBUF(ptr,length) #ptr, Dumper::BufferWrapper(ptr,length)
#define DCONT(container,maxElements) #container, Dumper::ContainerWrapper(container,maxElements)
