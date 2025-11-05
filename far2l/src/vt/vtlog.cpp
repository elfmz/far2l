#include "headers.hpp"

#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>
#include "config.hpp"
#include <WideMB.h>

#include "vtlog.h"
#include "shoco.h"

#include "vtshell.h"
#include "ctrlobj.hpp"
#include "cmdline.hpp"

namespace VTLog
{
	struct DumpState
	{
		DumpState() : nonempty(false) {}

		bool nonempty;
	};

	static inline unsigned char TranslateForegroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes&FOREGROUND_RED) out|= 1;
		if (attributes&FOREGROUND_GREEN) out|= 2;
		if (attributes&FOREGROUND_BLUE) out|= 4;
		return out;
	}

	static inline unsigned char TranslateBackgroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes&BACKGROUND_RED) out|= 1;
		if (attributes&BACKGROUND_GREEN) out|= 2;
		if (attributes&BACKGROUND_BLUE) out|= 4;
		return out;
	}


	// rv.first: actual line width
	// rv.second: if line actually ended or subject for tail-merging with following line(s)
	static std::pair<unsigned int, bool> ActualLineWidth(unsigned int Width, const CHAR_INFO *Chars)
	{
		const CHAR_INFO *prev_ci = nullptr;
		for (auto x = Width;;) {
			if (x == 0) {
				return std::pair<unsigned int, bool>(0, true);
			}
			--x;
			const auto &ci = Chars[x];
			// essentially same condition logic as applied in ConsoleBuffer::SetSize
			if ((ci.Char.UnicodeChar && ci.Char.UnicodeChar != L' ')
					|| (ci.Attributes & (IMPORTANT_LINE_CHAR | EXPLICIT_LINE_BREAK)) != 0
					|| (prev_ci && (ci.Attributes & BACKGROUND_RGB) != (prev_ci->Attributes & BACKGROUND_RGB)))  {
				return std::pair<unsigned int, bool>(x + 1, (ci.Attributes & EXPLICIT_LINE_BREAK) != 0); //  || x != Width - 1
			}
			prev_ci = &ci;
		}
	}

	static void EncodeLine(std::string &out, unsigned int Width, const CHAR_INFO *Chars, bool colored)
	{
		DWORD64 attr_prev = (DWORD64)-1;
		for (unsigned int i = 0; i < Width; ++i) if (Chars[i].Char.UnicodeChar) {
			const DWORD64 attr_now = Chars[i].Attributes;
			if ( colored && attr_now != attr_prev) {
				const bool tc_back_now = (attr_now & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_back_prev = (attr_prev & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_fore_now = (attr_now & FOREGROUND_TRUECOLOR) != 0;
				const bool tc_fore_prev = (attr_prev & FOREGROUND_TRUECOLOR) != 0;

				const size_t out_len_before_attr = out.size();
				out += "\033[";
				if (attr_prev == (DWORD64)-1
					|| (attr_prev & FOREGROUND_INTENSITY) != (attr_now & FOREGROUND_INTENSITY)) {
					out += (attr_now & FOREGROUND_INTENSITY) ? "1:" : "22:";
				}
				if (attr_prev == (DWORD64)-1 || (tc_fore_prev && !tc_fore_now)
					|| (attr_prev & (FOREGROUND_INTENSITY | FOREGROUND_RGB)) != (attr_now & (FOREGROUND_INTENSITY | FOREGROUND_RGB))) {
					out += (attr_now & FOREGROUND_INTENSITY) ? '9' : '3';
					out += '0' + TranslateForegroundColor(attr_now);
					out += ':';
				}
				if (attr_prev == (DWORD64)-1 || (tc_back_prev && !tc_back_now)
					|| (attr_prev & (BACKGROUND_INTENSITY | BACKGROUND_RGB)) != (attr_now & (BACKGROUND_INTENSITY | BACKGROUND_RGB))) {
					out += (attr_now & BACKGROUND_INTENSITY) ? "10" : "4";
					out += '0' + TranslateBackgroundColor(attr_now);
					out += ':';
				}

				if (tc_fore_now && (!tc_fore_prev || GET_RGB_FORE(attr_prev) != GET_RGB_FORE(attr_now))) {
					const DWORD rgb = GET_RGB_FORE(attr_now);
					out += StrPrintf("38:2:%u:%u:%u:", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (tc_back_now && (!tc_back_prev || GET_RGB_BACK(attr_prev) != GET_RGB_BACK(attr_now))) {
					const DWORD rgb = GET_RGB_BACK(attr_now);
					out += StrPrintf("48:2:%u:%u:%u:", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (out.back() == ':') {
					out.back() = 'm';
					attr_prev = attr_now;

				} else { // no visible attributes changed --> dismiss sequence start
					out.resize(out_len_before_attr);
				}
			}

			if (CI_USING_COMPOSITE_CHAR(Chars[i])) {
				const wchar_t *pwc = WINPORT(CompositeCharLookup)(Chars[i].Char.UnicodeChar);
				Wide2MB_UnescapedAppend(pwc, wcslen(pwc), out);

			} else if (Chars[i].Char.UnicodeChar > 0x80) {
				Wide2MB_UnescapedAppend(Chars[i].Char.UnicodeChar, out);

			} else {
				out+= (char)(unsigned char)Chars[i].Char.UnicodeChar;
			}
		}
		if (colored && attr_prev != 0xffff) {
			out+= "\033[m";
		}
	}


	static class Lines
	{
		std::mutex _mutex;
		struct Memories : std::list< std::pair<HANDLE, std::vector<char> > > {} _memories;
		size_t _memories_size{0}; // size in bytes of memory occupied by _memories
		std::string _encoded_line;
		std::vector<char> _compressed_line;
		enum
		{
			FLAG_HAS_EOL = 0x01,
			FLAG_IS_COMPRESSED = 0x80
		};

	public:
		void Add(HANDLE con_hnd, const CHAR_INFO *Chars, unsigned int Width, bool EOL)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			// attach to tail of last non-empty non-EOLed string if there is such one
			if (!_memories.empty() && _memories.back().first == con_hnd
					&& !_memories.back().second.empty() && _memories.back().second[0] == 0) {
				_encoded_line.assign((char *)&_memories.back().second[1], _memories.back().second.size() - 1);
				_memories_size-= sizeof(Memories::value_type) + _memories.back().second.size();
				_memories.pop_back();
			} else {
				_encoded_line.clear();
			}
			if (Width) {
				EncodeLine(_encoded_line, Width, Chars, true);
			}

			if (_encoded_line.empty() && EOL) {
				_compressed_line.clear(); // optimization for storing empty lines
			} else {
				_compressed_line.resize(_encoded_line.size() + 1);
				_compressed_line[0] = EOL ? FLAG_HAS_EOL : 0;
				if (EOL) {
					const size_t sz = shoco_compress(_encoded_line.c_str(),
						_encoded_line.size(), &_compressed_line[1], _compressed_line.size() - 1);
					if (sz < _compressed_line.size() - 1) {
						_compressed_line[0] |= FLAG_IS_COMPRESSED;
						_compressed_line.resize(sz + 1);
					}
				}
				if (!(_compressed_line[0] & FLAG_IS_COMPRESSED)) { // compression inefficient - store uncompressed
					memcpy(&_compressed_line[1], _encoded_line.c_str(), _encoded_line.size());
				}
			}

			const size_t limit = size_t(std::max(Opt.CmdLine.VTLogLimit, 1)) * 1024;

			_memories_size+= sizeof(Memories::value_type) + _compressed_line.size();
			_memories.emplace_back(con_hnd, _compressed_line);
			while (_memories_size > limit && !_memories.empty()) {
				_memories_size-= sizeof(Memories::value_type) + _memories.front().second.size();
				_memories.pop_front();
			}

//			fprintf(stderr, "VTLog count=%lu size=%lu\n", (unsigned long)_memories.size(), (unsigned long)_memories_size);
		}

		void DumpToFile(HANDLE con_hnd, int fd, DumpState &ds, bool colored)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			std::string s;
			for (auto m : _memories) {
				if (m.first == con_hnd && (ds.nonempty || m.second.size() > 1)) {
					if (m.second.size() <= 1) {
						s.clear();
					} else if ((m.second[0] & FLAG_IS_COMPRESSED) == 0) { // uncompressed
						s.assign((char *)&m.second[1], m.second.size() - 1);
					} else for (s.resize(m.second.size() * 2);; s.resize(s.size() * 3 / 2 + 32)) {
						size_t sz = shoco_decompress(&m.second[1], m.second.size() - 1, s.data(), s.size());
						if (sz <= s.size()) {
							while (sz != 0 && !s[sz - 1]) {
								--sz;
							}
							s.resize(sz);
							break;
						}
					}

					ds.nonempty = true;
					if (!colored) {
						for (;;) {
							size_t i = s.find('\033');
							if (i == std::string::npos) break;
							size_t j = s.find('m', i + 1);
							if (j == std::string::npos) break;
							s.erase(i, j + 1 - i);
						}
					}

					// empty means empty line terminated with EOL
					if (m.second.empty() || (m.second[0] & FLAG_HAS_EOL) != 0) {
						s+= NATIVE_EOL;
					}

					if (write(fd, s.c_str(), s.size()) != (int)s.size())
						perror("VTLog: WriteToFile");
				}
			}
		}

		void Reset(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto it = _memories.begin(); it != _memories.end();) {
				if (it->first == con_hnd) {
					_memories_size-= sizeof(Memories::value_type) + it->second.size();
					it = _memories.erase(it);
				} else {
					++it;
				}
			}
		}

		void ConsoleJoined(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			size_t remain = _memories.size();
			for (auto it = _memories.begin(); remain; --remain) {
				if (it->first == con_hnd) {
					auto next = it;
					++next;
					it->first = NULL;
					_memories.splice(_memories.end(), _memories, it);
					it = next;
				} else {
					++it;
				}
			}
		}

	} g_lines;


	void OnConsoleScroll(PVOID pContext, HANDLE hConsole, unsigned int Width, CHAR_INFO *Chars)
	{
		auto state = VTShell_LookupState(hConsole);
//std::wstring s;
//for (auto i = 0; i < Width; ++i)s+= Chars[i].Char.UnicodeChar;
//fprintf(stderr, " : %p %d '%ls'\n", hConsole, state, s.c_str());
		if (state != VT_INVALID && state != VT_ALTERNATE_SCREEN) {
			auto width_eol = ActualLineWidth(Width, Chars);
			g_lines.Add(hConsole, Chars, width_eol.first, width_eol.second);
		}
	}

	void Register(HANDLE con_hnd)
	{
		WINPORT(SetConsoleScrollCallback) (con_hnd, OnConsoleScroll, NULL);
	}

	void Unregister(HANDLE con_hnd)
	{
		WINPORT(SetConsoleScrollCallback) (con_hnd, NULL, NULL);
	}

	void ConsoleJoined(HANDLE con_hnd)
	{
		g_lines.ConsoleJoined(con_hnd);
	}

	void Reset(HANDLE con_hnd)
	{
		g_lines.Reset(con_hnd);
	}

	static void AppendScreenLine(const CHAR_INFO *line, unsigned int width, std::string &s, DumpState &ds, bool colored, bool no_line_recompose)
	{
		auto width_eol = ActualLineWidth(width, line);
		if (width_eol.first || ds.nonempty) {
			ds.nonempty = true;
			EncodeLine(s, width_eol.first, line, colored);
			if (width_eol.second || no_line_recompose) {
				s+= NATIVE_EOL;
			}
		}
	}

	static void AppendConsoleScreenLines(HANDLE con_hnd, std::string &s, DumpState &ds, bool colored)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> line(csbi.dwSize.X);
			COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
			SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
			// alternate VT screen mode typically used by rich UI terminal apps like MC
			// which need identical screen copy without line recomposition
			const bool no_line_recompose = (VTShell_LookupState(con_hnd) == VT_ALTERNATE_SCREEN);
			if (no_line_recompose && !s.empty() && !strchr(NATIVE_EOL, s.back())) {
				s+= NATIVE_EOL;
			}
			for (rc.Top = rc.Bottom = 0; rc.Top < csbi.dwSize.Y; rc.Top = ++rc.Bottom) {
				if (WINPORT(ReadConsoleOutput)(con_hnd, &line[0], buf_size, buf_pos, &rc)) {
					AppendScreenLine(&line[0], (unsigned int)csbi.dwSize.X, s, ds, colored, no_line_recompose);
				}
			}
		}
	}

	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines, const char *wanted_path)
	{
		std::string path;
		if (wanted_path && *wanted_path) {
			path = wanted_path;
		} else {
			SYSTEMTIME st;
			WINPORT(GetLocalTime)(&st);
			path = InMyTempFmt("farvt_%u-%u-%u_%u-%u-%u.%s",
				 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
				 colored ? "ans" : "log");
		}

		int fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0600);
		if (fd==-1) {
			fprintf(stderr, "VTLog: errno %u creating '%s'\n", errno, path.c_str() );
			return std::string();
		}

		DumpState ds;
		g_lines.DumpToFile(con_hnd, fd, ds, colored);
		if (append_screen_lines) {
			std::string s;
			if (con_hnd || VTShell_Busy()) {
				AppendConsoleScreenLines(con_hnd, s, ds, colored);
			} else if (CtrlObject->CmdLine) {
				AppendConsoleScreenLines(CtrlObject->CmdLine->GetBackgroundConsole(), s, ds, colored);
			}
			if (!s.empty()) {
				for (;;) { // remove excessive empty lines at the end
					if (StrEndsBy(s, "\n\n")) {
						s.pop_back();
					} else if (StrEndsBy(s, "\n\x1b[m\n")) {
						s.resize(s.size() - 4);
					} else {
						break;
					}
				}
				if (write(fd, s.c_str(), s.size()) != (int)s.size()) {
					perror("VTLog: write");
				}
			}
		}
		close(fd);
		return path;
	}
}
