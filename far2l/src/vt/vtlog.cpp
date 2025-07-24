#include "headers.hpp"

#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>
#include "config.hpp"
#include <WideMB.h>

#include "vtlog.h"

#include "vtshell.h"
#include "ctrlobj.hpp"
#include "cmdline.hpp"


#define FOREGROUND_RGB (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define BACKGROUND_RGB (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)

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
		for (auto x = Width;;) {
			if (x == 0) {
				return std::pair<unsigned int, bool>(0, true);
			}
			--x;
			const auto &ci = Chars[x];
			if ((ci.Char.UnicodeChar && ci.Char.UnicodeChar != L' ') || (ci.Attributes & (BACKGROUND_RGB|EXPLICIT_LINE_WRAP)) != 0) {
				return std::pair<unsigned int, bool>(x + 1, (ci.Attributes & EXPLICIT_LINE_WRAP) != 0 || x != Width - 1);
			}
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
		std::list< std::pair<HANDLE, std::string> > _memories;

		void AddInner(HANDLE con_hnd, unsigned int Width, const CHAR_INFO *Chars)
		{
			const size_t limit = (size_t)std::max(Opt.CmdLine.VTLogLimit, 2);
			// a little hustling to reduce reallocations
			if (_memories.size() >= limit) {
				while (_memories.size() > limit) {
					_memories.pop_front();
				}
				_memories.splice(_memories.end(), _memories, _memories.begin());
			} else {
				_memories.emplace_back();
			}
			auto &last = _memories.back();
			last.first = con_hnd;
			last.second.clear();
			if (Width) {
				EncodeLine(last.second, Width, Chars, true);
			}
		}
		
	public:
		void Add(HANDLE con_hnd, unsigned int Width, const CHAR_INFO *Chars)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			AddInner(con_hnd, Width, Chars);
		}

		void Append(HANDLE con_hnd, unsigned int Width, const CHAR_INFO *Chars)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_memories.empty()) {
				AddInner(con_hnd, Width, Chars);
			} else if (Width) {
				EncodeLine(_memories.back().second, Width, Chars, true);
			}
		}

		void DumpToFile(HANDLE con_hnd, int fd, DumpState &ds, bool colored)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto m : _memories) {
				if (m.first == con_hnd && (ds.nonempty || !m.second.empty())) {
					ds.nonempty = true;
					if (!colored) {
						for (;;) {
							size_t i = m.second.find('\033');
							if (i == std::string::npos) break;
							size_t j = m.second.find('m', i + 1);
							if (j == std::string::npos) break;
							m.second.erase(i, j + 1 - i);
						}
					}
					m.second+= NATIVE_EOL;
					if (write(fd, m.second.c_str(), m.second.size()) != (int)m.second.size())
						perror("VTLog: WriteToFile");
				}
			}
		}
		
		void Reset(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto it = _memories.begin(); it != _memories.end();) {
				if (it->first == con_hnd) {
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

	static unsigned int g_pause_cnt = 0;

	void OnConsoleScroll(PVOID pContext, HANDLE hConsole, unsigned int Width, CHAR_INFO *Chars)
	{
		if (g_pause_cnt == 0) {
			auto width_eol = ActualLineWidth(Width, Chars);
			if (width_eol.second) {
				g_lines.Add(hConsole, width_eol.first, Chars);
			} else {
				g_lines.Append(hConsole, width_eol.first, Chars);
			}
		}
	}

	void Pause()
	{
		__sync_add_and_fetch(&g_pause_cnt, 1);
	}

	void Resume()
	{
		if (__sync_sub_and_fetch(&g_pause_cnt, 1) < 0) {
			ABORT();
		}
	}
	
	void Start()
	{
		WINPORT(SetConsoleScrollCallback) (NULL, OnConsoleScroll, NULL);
	}

	void Stop()
	{
		WINPORT(SetConsoleScrollCallback) (NULL, NULL, NULL);
	}

	void ConsoleJoined(HANDLE con_hnd)
	{
		g_lines.ConsoleJoined(con_hnd);
	}
	
	void Reset(HANDLE con_hnd)
	{
		g_lines.Reset(con_hnd);
	}
	
	static void AppendScreenLine(const CHAR_INFO *line, unsigned int width, std::string &s, DumpState &ds, bool colored)
	{
		auto width_eol = ActualLineWidth(width, line);
		if (width_eol.first || ds.nonempty) {
			ds.nonempty = true;
			EncodeLine(s, width_eol.first, line, colored);
			if (width_eol.second) {
				s+= NATIVE_EOL;
			}
		}
	}

	static void AppendActiveScreenLines(HANDLE con_hnd, std::string &s, DumpState &ds, bool colored)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> line(csbi.dwSize.X);
			COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
			SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
			for (rc.Top = rc.Bottom = 0; rc.Top < csbi.dwSize.Y; rc.Top = ++rc.Bottom) {
				if (WINPORT(ReadConsoleOutput)(con_hnd, &line[0], buf_size, buf_pos, &rc)) {
					AppendScreenLine(&line[0], (unsigned int)csbi.dwSize.X, s, ds, colored);
				}
			}
		}		
	}

	static void AppendSavedScreenLines(std::string &s, DumpState &ds, bool colored)
	{
		if (CtrlObject->CmdLine) {
			int w = 0, h = 0;
			const CHAR_INFO *ci = CtrlObject->CmdLine->GetBackgroundScreen(w, h);
			if (ci && w > 0 && h > 0) {
				while (h--) {
					AppendScreenLine(ci, (unsigned int)w, s, ds, colored);
					ci+= w;
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
			if (!con_hnd && !VTShell_Busy()) {
				AppendSavedScreenLines(s, ds, colored);
			} else {
				AppendActiveScreenLines(con_hnd, s, ds, colored);
			}
			if (!s.empty()) {
				if (write(fd, s.c_str(), s.size()) != (int)s.size())
					perror("VTLog: write");				
			}
		}
		close(fd);
		return path;
	}
}



