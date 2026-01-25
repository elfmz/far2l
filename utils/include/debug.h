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
#include <array>
#include <map>
#include <dlfcn.h>
#include "WideMB.h"
#include <fcntl.h>

#if defined(__linux__)
#  define HAS_ELF_ENHANCEMENT
# include "LookupDebugSymbol.h"
#endif

// Platform-specific includes for stack trace functionality
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__MUSL__) && !defined(__UCLIBC__) && !defined(__HAIKU__) && !defined(__ANDROID__) // todo: pass to linker -lexecinfo under BSD and then may remove this ifndef
# include <execinfo.h>
# define HAS_BACKTRACE
#endif

#if defined(__has_include)
# if __has_include(<cxxabi.h>)
#   include <cxxabi.h>
#   define HAS_CXX_DEMANGLE
# endif
#else
# if defined(__GLIBCXX__) || defined(__GLIBCPP__) || ((defined(__GNUC__) || defined(__clang__)) && !defined(__APPLE__))
#   include <cxxabi.h>
#   define HAS_CXX_DEMANGLE
# endif
#endif


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

    struct DumperConfig
    {
        static constexpr bool WRITE_LOG_TO_FILE = false;
		static constexpr char LOG_FILENAME[] = "far2l_debug.log";

		static constexpr bool ENABLE_PID_TID = true;
		static constexpr bool ENABLE_TIMESTAMP = true;
		static constexpr bool ENABLE_LOCATION = true;

		static constexpr size_t HEXDUMP_BYTES_PER_LINE = 16;
		static constexpr size_t HEXDUMP_MAX_LENGTH = 1024 * 1024;

		static constexpr std::size_t CONTAINERS_MAX_INDENT_LEVEL = 32;
		enum class AdjustmentStrategy { Off, PreferAdjusted, PreferOriginal };
		enum class ResolutionStrategy { OnlyDynsym, PreferDynsym, PreferSymtab, OnlySymtab };

		static constexpr bool STACKTRACE_SHOW_ADDRESSES_ALWAYS = false;
		static constexpr bool STACKTRACE_DEMANGLE_NAMES = true;
		static constexpr AdjustmentStrategy STACKTRACE_RETADDR_ADJUSTMENT = AdjustmentStrategy::Off;
		static constexpr ResolutionStrategy STACKTRACE_SYMBOL_RESOLUTION = ResolutionStrategy::PreferDynsym;
		static constexpr bool STACKTRACE_SHOW_SYMBOL_SOURCE = false;
		static constexpr bool STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS = true;
		static constexpr size_t STACKTRACE_MAX_FRAMES = 64;
		static constexpr size_t STACKTRACE_SKIP_FRAMES = 2;
	};


	// ****************************************************************************************************
	// Helper variables, functions, and structures
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
					std::snprintf(buf, sizeof(buf), "\\x{%02X}", character_code);
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
			s_home += "/";
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


	// Compile-time metafunction that determines if T is a container

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
	// Stack trace support functionality
	// ****************************************************************************************************

	struct StackTrace
	{
		std::vector<std::string> frames;
		std::vector<std::string> cmdline_tool_invocations;

		StackTrace()
		{
			CaptureStackTrace();
		}

	private:

		struct DlAddrResult
		{
			bool success = false;
			Dl_info info = {};
			const void* used_address = nullptr;
			bool used_adjusted = false;
		};


		struct FrameInfo
		{
			std::string module_shortname {};
			std::string module_fullname {};
			std::string func_name {};

			uintptr_t original_address = 0;
			uintptr_t used_address = 0;
			uintptr_t module_base = 0;
			uintptr_t symbol_address = 0;
			uintptr_t offset_from_module = 0;
			uintptr_t offset_from_symbol = 0;

			bool used_adjusted = false;
			bool found_in_symtab = false;
		};


		static std::string DemangleName(const char* mangled_name)
		{
			if (!mangled_name || !*mangled_name) return "";
#ifdef HAS_CXX_DEMANGLE
			int status = 0;
			char* raw = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
			std::unique_ptr<char, decltype(&std::free)> demangled(raw, &std::free);
			if (status == 0 && demangled) {
				return std::string(demangled.get());
			}
#endif
			return std::string(mangled_name);
		}


		static std::string HexAddr(uintptr_t v)
		{
			std::ostringstream os;
			os << "0x" << std::hex << v;
			return os.str();
		}


		static bool TryAdjustReturnAddress(const void* in, const void*& out)
		{
			out = in;
			if (in == nullptr) return false;

			auto original = reinterpret_cast<uintptr_t>(in);
			uintptr_t adjusted = original;

#if defined(__arm__) && !defined(__aarch64__)
			if (original & 1u) {
				adjusted = original & ~uintptr_t(1);
			}
#elif defined(__aarch64__)
			if (original > 4) {
				adjusted = original - 4;
			}
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
			if (original > 1) {
				adjusted = original - 1;
			}
#endif

			if (adjusted != original && adjusted != 0) {
				out = reinterpret_cast<const void*>(adjusted);
				return true;
			}
			return false;
		}


		static DlAddrResult DlAddrWithAdjust(const void* original)
		{
			auto adjust_strategy = DumperConfig::STACKTRACE_RETADDR_ADJUSTMENT;

			DlAddrResult result;
			result.used_address = original;

			auto TryDlAddr = [&](const void* addr) -> bool {
				Dl_info info_local;
				if (dladdr(addr, &info_local)) {
					result.success = true;
					result.info = info_local;
					result.used_address = addr;
					return true;
				}
				return false;
			};

			if (adjust_strategy == DumperConfig::AdjustmentStrategy::Off) {
				TryDlAddr(original);
				return result;
			}

			const void* adjusted = nullptr;
			bool has_adjusted = TryAdjustReturnAddress(original, adjusted);

			if (adjust_strategy == DumperConfig::AdjustmentStrategy::PreferAdjusted) {
				if (has_adjusted) {
					if (TryDlAddr(adjusted)) { result.used_adjusted = true; return result; }
				}
				TryDlAddr(original);
				return result;
			}

			if (TryDlAddr(original)) { return result; }
			if (has_adjusted) {
				if (TryDlAddr(adjusted)) { result.used_adjusted = true; return result; }
			}
			return result;
		}


#ifdef HAS_ELF_ENHANCEMENT
		static bool TrySymtabResolve(DlAddrResult dladdr_result, FrameInfo &frameinfo_out)
		{
			if (!dladdr_result.success || !dladdr_result.info.dli_fname) {
				return false;
			}

			LookupDebugSymbol lds(dladdr_result.info.dli_fname, dladdr_result.info.dli_fbase, dladdr_result.used_address);
			if (lds.name.empty()) {
				return false;
			}

			frameinfo_out.func_name.swap(lds.name);
			frameinfo_out.offset_from_symbol = lds.offset;
			frameinfo_out.symbol_address =  frameinfo_out.used_address - (uintptr_t)lds.offset;
			frameinfo_out.found_in_symtab = true;
			return true;
		}
#endif


		static bool TryDynsymResolve(DlAddrResult dladdr_result, FrameInfo &frameinfo_out)
		{
			const char* sname = dladdr_result.info.dli_sname;
			const void *saddr = dladdr_result.info.dli_saddr;

			if (sname && *sname && saddr) {
				frameinfo_out.func_name = sname;
				frameinfo_out.symbol_address = reinterpret_cast<uintptr_t>(saddr);
				frameinfo_out.offset_from_symbol = CalculateOffset(frameinfo_out.used_address, frameinfo_out.symbol_address);
				return true;
			}
			return false;
		}


		static bool TryResolveSymbol(DlAddrResult& dladdr_result, FrameInfo& frameinfo) {
#ifdef HAS_ELF_ENHANCEMENT
			switch (DumperConfig::STACKTRACE_SYMBOL_RESOLUTION) {
			case DumperConfig::ResolutionStrategy::OnlyDynsym:
				return TryDynsymResolve(dladdr_result, frameinfo);

			case DumperConfig::ResolutionStrategy::OnlySymtab:
				return TrySymtabResolve(dladdr_result, frameinfo);

			case DumperConfig::ResolutionStrategy::PreferDynsym:
				return TryDynsymResolve(dladdr_result, frameinfo) || TrySymtabResolve(dladdr_result, frameinfo);

			case DumperConfig::ResolutionStrategy::PreferSymtab:
				return TrySymtabResolve(dladdr_result, frameinfo) || TryDynsymResolve(dladdr_result, frameinfo);
			}
#else
			return TryDynsymResolve(dladdr_result, frameinfo);
#endif
		}


		static FrameInfo GetFrameInfo(const void* raw_addr)
		{
			FrameInfo frameinfo;
			frameinfo.original_address = reinterpret_cast<uintptr_t>(raw_addr);
			auto dladdr_result = DlAddrWithAdjust(raw_addr);
			frameinfo.used_address = reinterpret_cast<uintptr_t>(dladdr_result.used_address);
			frameinfo.used_adjusted = dladdr_result.used_adjusted;

			if (dladdr_result.success) {

				frameinfo.module_base = reinterpret_cast<uintptr_t>(dladdr_result.info.dli_fbase);
				frameinfo.offset_from_module = CalculateOffset(frameinfo.used_address, frameinfo.module_base);

				const char* fname = dladdr_result.info.dli_fname;

				if (fname && *fname) {
					frameinfo.module_fullname = fname;
					const char* last_slash = strrchr(fname, '/');
					frameinfo.module_shortname = last_slash ? last_slash + 1 : fname;
				}

				if (TryResolveSymbol(dladdr_result, frameinfo) && DumperConfig::STACKTRACE_DEMANGLE_NAMES) {
					frameinfo.func_name = DemangleName(frameinfo.func_name.c_str());
				}
			}

			return frameinfo;
		}


		static uintptr_t CalculateOffset(uintptr_t address, uintptr_t base)
		{
			return (address >= base) ? (address - base) : 0;
		}


		static std::string FormatFrame(const FrameInfo& frameinfo)
		{
			std::ostringstream result;

			const bool func_name_resolved = !frameinfo.func_name.empty();
			const bool module_name_resolved = !frameinfo.module_shortname.empty();

			result << (module_name_resolved ? frameinfo.module_shortname.c_str() : "[unknown-module]");
			result << " :: ";
			result << (func_name_resolved ? frameinfo.func_name.c_str() : "[unknown-function]");

			if (func_name_resolved && !DumperConfig::STACKTRACE_SHOW_ADDRESSES_ALWAYS) {
				result << "+" << HexAddr(frameinfo.offset_from_symbol);
			}

			if constexpr (DumperConfig::STACKTRACE_SHOW_SYMBOL_SOURCE) {
				if (func_name_resolved) {
					result << (frameinfo.found_in_symtab ? "  [symtab]" : "  [dynsym]");
				}
			}

			if (DumperConfig::STACKTRACE_SHOW_ADDRESSES_ALWAYS || !func_name_resolved) {
				result << "  :: addr=" << HexAddr(frameinfo.used_address);

				if (frameinfo.used_adjusted) {
					result << " (*orig: " << HexAddr(frameinfo.original_address) << ")";
				}

				if (frameinfo.module_base) {
					result << ", mod_base=" << HexAddr(frameinfo.module_base)
					<< ", off_mod=+" << HexAddr(frameinfo.offset_from_module);
				}

				if (frameinfo.symbol_address) {
					result << ", sym_addr=" << HexAddr(frameinfo.symbol_address)
					<< ", off_sym=+" << HexAddr(frameinfo.offset_from_symbol);
				}
			}

			return result.str();
		}


		struct CmdLineGenerator
		{
		private:
			struct Group {
				std::string module_fullname {};
				uintptr_t module_base = 0;
				std::vector<uintptr_t> addresses {};

				Group(std::string name, uintptr_t base) : module_fullname(std::move(name)), module_base(base) {}
			};
			std::vector<Group> _groups;

		public:
			void AddFrame(const FrameInfo& frameinfo)
			{
				if (frameinfo.module_fullname.empty()) {
					return;
				}

				if (_groups.empty() || _groups.back().module_fullname != frameinfo.module_fullname || _groups.back().module_base != frameinfo.module_base) {
					_groups.emplace_back(frameinfo.module_fullname, frameinfo.module_base);
				}

#if defined(__APPLE__)
				if (frameinfo.symbol_address) {
					_groups.back().addresses.push_back(frameinfo.used_address);
				}
#else
				_groups.back().addresses.push_back(frameinfo.offset_from_module);
#endif
			}


			std::vector<std::string> GetCommands() const
			{
				std::vector<std::string> commands;
				commands.reserve(_groups.size());

				for (const auto& group : _groups) {
					if (group.addresses.empty()) {
						continue;
					}

					std::ostringstream cmdline_stream;
#if defined(__APPLE__)
					cmdline_stream << "atos -o '" << group.module_fullname << "' -l " << HexAddr(group.module_base);
#else
					cmdline_stream << "addr2line -e '" << group.module_fullname << "' -f -p -C -i";
#endif
					for (const auto& addr : group.addresses) {
						cmdline_stream << " " << HexAddr(addr);
					}
					commands.emplace_back(cmdline_stream.str());
				}
				return commands;
			}

		};


		void CaptureStackTrace()
		{
#ifdef HAS_BACKTRACE
			void* raw_frames[DumperConfig::STACKTRACE_MAX_FRAMES];
			auto frame_count = static_cast<size_t>(backtrace(raw_frames, DumperConfig::STACKTRACE_MAX_FRAMES));

			if (frame_count <= DumperConfig::STACKTRACE_SKIP_FRAMES) {
				frames.emplace_back("[no stack frames available]");
				return;
			}

			frames.reserve(frame_count - DumperConfig::STACKTRACE_SKIP_FRAMES);

			CmdLineGenerator cmd_generator;

			for (size_t i = DumperConfig::STACKTRACE_SKIP_FRAMES; i < frame_count; ++i) {
				auto frameinfo = GetFrameInfo(raw_frames[i]);
				frames.emplace_back(FormatFrame(frameinfo));

				if constexpr (DumperConfig::STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS) {
					cmd_generator.AddFrame(frameinfo);
				}
			}

			if constexpr (DumperConfig::STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS) {
				cmdline_tool_invocations = cmd_generator.GetCommands();
			}
#else
			frames.emplace_back("[stack trace not available on this platform]");
#endif
		}
	};



	// ****************************************************************************************************
	// Formatting variable names/values according to their nesting level when displaying containers
	// ****************************************************************************************************

	struct IndentInfo
	{
		std::bitset<DumperConfig::CONTAINERS_MAX_INDENT_LEVEL> levels;
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
			if (cur_level < static_cast<int>(DumperConfig::CONTAINERS_MAX_INDENT_LEVEL)) {
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
	// DumpValue() – the main dispatcher implementation handling the basic types
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

		} else if constexpr (std::is_same_v<std::remove_const_t<std::remove_reference_t<T>>, std::wstring>) {
			DumpValue(log_stream, var_name, StrWide2MB(value), indent_info);

		} else if constexpr (std::is_convertible_v<T, const wchar_t*>) {
			DumpValue(log_stream, var_name, Wide2MB(value), indent_info);

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
			LogVarWithIndentation(log_stream, var_name, 0, indent_info, false);
			auto vn = std::string( (var_name.size() >= 2 && var_name.front() == '{' && var_name.back() == '}') ? "" : var_name );
			std::size_t index = 0;
			auto it_begin = std::begin(value);
			auto it_end   = std::end(value);
			for (auto it = it_begin; it != it_end; ) {
				auto curr = it++;
				bool is_last = (it == it_end);
				auto child_indent_info = indent_info.CreateChild(!is_last);
				auto item_name = vn + "[" + std::to_string(index++) + "]";
				DumpValue(log_stream, item_name, *curr, child_indent_info);
			}

		} else {
			LogVarWithIndentation(log_stream, var_name, value, indent_info);
		}
	}


	template<typename FirstT, typename SecondT>
	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const std::pair<FirstT, SecondT>& value,
		const IndentInfo& indent_info = IndentInfo())
	{
		LogVarWithIndentation(log_stream, var_name, 0, indent_info, false);
		DumpValue(log_stream, std::string(var_name)+".first", value.first, indent_info.CreateChild(true));
		DumpValue(log_stream, std::string(var_name)+".second", value.second, indent_info.CreateChild(false));
	}


	// ****************************************************************************************************
	// Support for char[], unsigned char[], and wchar_t[] static arrays
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
	// Support for string buffers specified as (pointer, length) via DSTRBUF + DUMP macros
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
	// Support for binary buffers specified as (pointer, byte count) via DBINBUF + DUMP macros
	// ****************************************************************************************************

	inline std::string CreateHexDump(const std::uint8_t* data, size_t length)
	{
		constexpr std::string_view line_prefix = "|   ";
		constexpr auto bytes_per_line = DumperConfig::HEXDUMP_BYTES_PER_LINE;
		auto separator_pos = bytes_per_line / 2;
		std::ostringstream result;
		result << std::hex << std::setfill('0');

		result << line_prefix << "          ";
		for (size_t i = 0; i < bytes_per_line; ++i) {
			if (i == separator_pos) result << "| ";
			result << std::setw(2) << i << " ";
		}
		result << " | ASCII\n";
		result << line_prefix << "------------";
		result << std::string(3 * bytes_per_line, '-');
		result << "-+" << std::string(bytes_per_line + 1, '-') << "\n";

		for (size_t offset = 0; offset < length; offset += bytes_per_line) {
			result << line_prefix << std::setw(8) << offset << "  ";
			std::string ascii;
			for (size_t i = 0; i < bytes_per_line; ++i) {
				if (i == separator_pos) result << "| ";
				if (offset + i < length) {
					auto byte = data[offset + i];
					result << std::setw(2) << static_cast<unsigned int>(byte) << " ";
					ascii.push_back((byte >= 32 && byte < 127) ? byte : '.');
				} else {
					result << "   ";
					ascii.push_back(' ');
				}
			}
			result << " | " << ascii << '\n';
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

		size_t effective_length;
		if constexpr (DumperConfig::HEXDUMP_MAX_LENGTH == 0) {
			effective_length = bin_buf_wrapper.length;
		} else {
			effective_length = std::min(DumperConfig::HEXDUMP_MAX_LENGTH, bin_buf_wrapper.length);
		}

		std::string hexDump = CreateHexDump(reinterpret_cast<const uint8_t*>(bin_buf_wrapper.data), effective_length);

		log_stream << "|=> " << var_name << " =\n";
		log_stream << hexDump;

		if constexpr (DumperConfig::HEXDUMP_MAX_LENGTH != 0) {
			if (bin_buf_wrapper.length > DumperConfig::HEXDUMP_MAX_LENGTH) {
				log_stream << "|   Output truncated to " << effective_length
						   << " bytes (full length: " << bin_buf_wrapper.length << " bytes)\n";
			}
		}
	}


	// ****************************************************************************************************
	// Support for iterable STL containers and static arrays via DCONT + DUMP macros
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
		LogVarWithIndentation(log_stream, var_name, 0, indent_info, false);

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
	// Support for flags (bitmasks, etc.) via DFLAGS + DUMP; use Dumper::FlagsAs::... as the second argument
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
		if (flags == 0xFFFFFFFF) return "INVALID_FILE_ATTRIBUTES";

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


	inline std::string DecodeUnixMode(mode_t flags)
	{
		std::ostringstream result;

		char file_type_char = '?';
		if (S_ISREG(flags)) file_type_char = '-';
		else if (S_ISDIR(flags)) file_type_char = 'd';
		else if (S_ISLNK(flags)) file_type_char = 'l';
		else if (S_ISBLK(flags)) file_type_char = 'b';
		else if (S_ISCHR(flags)) file_type_char = 'c';
		else if (S_ISFIFO(flags)) file_type_char = 'p';
		else if (S_ISSOCK(flags)) file_type_char = 's';

		result << std::oct << flags << " (" << file_type_char
			<< ((flags & S_IRUSR) ? "r" : "-")
			<< ((flags & S_IWUSR) ? "w" : "-")
			<< ((flags & S_IXUSR) ? ((flags & S_ISUID) ? "s" : "x") : ((flags & S_ISUID) ? "S" : "-"))
			<< ((flags & S_IRGRP) ? "r" : "-")
			<< ((flags & S_IWGRP) ? "w" : "-")
			<< ((flags & S_IXGRP) ? ((flags & S_ISGID) ? "s" : "x") : ((flags & S_ISGID) ? "S" : "-"))
			<< ((flags & S_IROTH) ? "r" : "-")
			<< ((flags & S_IWOTH) ? "w" : "-")
			<< ((flags & S_IXOTH) ? ((flags & S_ISVTX) ? "t" : "x") : ((flags & S_ISVTX) ? "T" : "-"));
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
			decoded = DecodeUnixMode(static_cast<mode_t>(df.value));
			break;
		default:
			decoded = "[not implemented]";
		}
		log_stream << "|=> " << var_name << " = " << decoded << '\n';
	}


	// ****************************************************************************************************
	// Support for [STACKTRACE]
	// ****************************************************************************************************


	inline void DumpValue(
		std::ostringstream& log_stream,
		std::string_view var_name,
		const StackTrace& stack_trace,
		const IndentInfo& indent_info = IndentInfo())
	{
		LogVarWithIndentation(log_stream, "{STACKTRACE}", 0, indent_info, false);
		DumpValue(log_stream, "{FRAMES}", stack_trace.frames, indent_info.CreateChild(DumperConfig::STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS));
		if constexpr (DumperConfig::STACKTRACE_SHOW_CMDLINE_TOOL_COMMANDS) {
			DumpValue(log_stream, "{CMDLINE TOOL}", stack_trace.cmdline_tool_invocations, indent_info.CreateChild(false));
		}
	}


	// ****************************************************************************************************
	// Helper code for logging
	// ****************************************************************************************************


	inline std::string CreateLogHeader(std::string_view func_name, std::string_view location)
	{
		std::ostringstream header_stream;
		header_stream << "\n";

		if constexpr (DumperConfig::ENABLE_PID_TID || DumperConfig::ENABLE_TIMESTAMP || DumperConfig::ENABLE_LOCATION) {
			header_stream << "/-----";
		}

		if constexpr (DumperConfig::ENABLE_PID_TID) {
			header_stream << "[PID:" << getpid() << ", TID:" << GetNiceThreadId() << "]-----";
		}

		if constexpr (DumperConfig::ENABLE_TIMESTAMP) {
			auto current_time = std::chrono::system_clock::now();
			auto current_time_t = std::chrono::system_clock::to_time_t(current_time);
			std::tm local_time{};
			localtime_r(&current_time_t, &local_time);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()) % 1000;
			header_stream << "[" << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << ',' << ms.count() << "]-----";
		}

		if constexpr (DumperConfig::ENABLE_PID_TID || DumperConfig::ENABLE_TIMESTAMP) {
			header_stream << "\n";
		}

		if constexpr (DumperConfig::ENABLE_LOCATION) {
			if constexpr (DumperConfig::ENABLE_PID_TID || DumperConfig::ENABLE_TIMESTAMP) {
				header_stream << "|";
			}
			header_stream << "[" << location << "] in " << func_name << "()\n";
		}

		return header_stream.str();
	}


	inline void FlushLog(std::ostringstream& log_stream)
	{
		std::string log_entry = log_stream.str();
		std::lock_guard<std::mutex> lock(g_log_output_mutex);

		if constexpr (DumperConfig::WRITE_LOG_TO_FILE) {
			std::ofstream log_file(GetHomeDir() + DumperConfig::LOG_FILENAME, std::ios::app);
			if (log_file) {
				log_file << log_entry << std::endl;
			}
		} else {
			std::cerr << log_entry << std::endl;
		}
	}


	// ****************************************************************************************************
	// Backend for the DUMP macro: arguments must be wrapped in helper macros (DVV, DBINBUF, DCONT, DMSG...)
	// ****************************************************************************************************

	template<std::size_t... I, typename NameValueTupleT>
	void ProcessPairs(const NameValueTupleT& name_value_tuple, std::ostringstream & log_stream, std::index_sequence<I...>)
	{

		(DumpValue(log_stream, std::get<2 * I>(name_value_tuple), std::get<2 * I + 1>(name_value_tuple)), ...);
	}


	template<typename... Args>
	void Dump(std::string_view func_name, std::string_view location, const Args&... args)
	{
		static_assert(sizeof...(args) % 2 == 0, "Dump() expects arguments in pairs: name and value.");

		std::ostringstream log_stream;
		log_stream << CreateLogHeader(func_name, location) << std::boolalpha;

		auto args_tuple = std::forward_as_tuple(args...);
		constexpr std::size_t pair_count = sizeof...(args) / 2;
		ProcessPairs(args_tuple, log_stream, std::make_index_sequence<pair_count>{});
		FlushLog(log_stream);
	}


	// ****************************************************************************************************
	// Backend for the DUMPV macro: only simple variables support (no function calls, macros, or complex expressions)
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
			"Only simple variables are allowed as arguments. "
			"Function calls or complex expressions with internal commas are not supported.";
		DumpValue(log_stream, "{DUMPV ERROR}", error_message);
	}


	template<typename... Ts>
	void DumpV(
		std::string_view func_name,
		std::string_view location,
		const char *var_names_str,
		const Ts&... var_values)
	{
		std::ostringstream log_stream;
		log_stream << CreateLogHeader(func_name, location) << std::boolalpha;

		constexpr auto var_values_count = sizeof...(var_values);
		std::vector<std::string> var_names;

		if (TryParseVariableNames(var_names_str, var_names, var_values_count)) {
			auto values_tuple = std::forward_as_tuple(var_values...);
			DumpEachVariable(var_names, log_stream, values_tuple, std::make_index_sequence<var_values_count>{});
		} else {
			ReportDumpVError(log_stream);
		}

		FlushLog(log_stream);
	}

} // end namespace Dumper

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define LOCATION (__FILE__ ":" STRINGIZE_VALUE_OF(__LINE__))

#define DUMP(...) { Dumper::Dump(__func__, LOCATION, __VA_ARGS__); }
#define DUMPV(...) { Dumper::DumpV(__func__, LOCATION, #__VA_ARGS__, __VA_ARGS__); }

#define DVV(expr) #expr, expr
#define DMSG(msg) "{DMSG}", std::string(msg)
#define DBINBUF(ptr,length) #ptr, Dumper::BinBufWrapper(ptr,length)
#define DSTRBUF(ptr,length) #ptr, Dumper::StrBufWrapper(ptr,length)
#define DCONT(container,max_elements) #container, Dumper::ContainerWrapper(container,max_elements)
#define DFLAGS(var, treat_as) #var, Dumper::FlagsWrapper(var, treat_as)
#define DSTACKTRACE() "{STACKTRACE}", Dumper::StackTrace()
