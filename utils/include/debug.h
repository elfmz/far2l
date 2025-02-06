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

static std::mutex dumper_mutex;

inline std::string dump_escape_string(const std::string &input)
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

template <typename T>
inline void dump_value(
	std::ostringstream& oss,
	std::string_view var_name,
	const T& value)
{

	if constexpr (std::is_convertible_v<T, const wchar_t*>) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		dump_value(oss, var_name, conv.to_bytes(value));
		return;
	}

	if constexpr (std::is_convertible_v<T, std::string_view> || std::is_same_v<T, char> || std::is_same_v<T, wchar_t>) {
		std::string s_value{ value };
		std::string escaped = dump_escape_string(s_value);
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
			std::ofstream(std::string(std::getenv("HOME")) + "/far2l_debug.log", std::ios::app) << log_entry << std::endl;
		} else {
			std::clog << log_entry << std::endl;
		}
	}
}

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define LOCATION (__FILE__ ":" STRINGIZE_VALUE_OF(__LINE__))

#ifdef _FAR2L_PROJECT
#define DUMP(to_file, ...) { std::ostringstream oss; dump(oss, to_file, true, __func__, LOCATION, getpid(), GetInterThreadID(), __VA_ARGS__); }
#else
#define DUMP(to_file, ...) { std::ostringstream oss; dump(oss, to_file, true, __func__, LOCATION, getpid(), 0, __VA_ARGS__); }
#endif

#define DVV(xxx) #xxx, xxx
#define DMSG(xxx) "msg", xxx
