#include "headers.hpp"

#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>
#include "config.hpp"
#include <WideMB.h>

#include "vtlog.h"


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


	static unsigned int ActualLineWidth(unsigned int Width, const CHAR_INFO *Chars)
	{
		for (;;) {
			if (!Width)
				return 0;

			--Width;
			const auto &CI = Chars[Width];
			if ((CI.Char.UnicodeChar && CI.Char.UnicodeChar != L' ') || (CI.Attributes & BACKGROUND_RGB) != 0) {
				return Width + 1;
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
				out+= "\033[";
				if ( attr_prev == (DWORD64)-1
				|| (attr_prev&FOREGROUND_INTENSITY) != (attr_now&FOREGROUND_INTENSITY)) {
					out+= (attr_now&FOREGROUND_INTENSITY) ? "1;" : "22;";
				}
				if ( attr_prev == (DWORD64)-1 || (tc_fore_prev && !tc_fore_now)
				|| (attr_prev&(FOREGROUND_INTENSITY|FOREGROUND_RGB)) != (attr_now&(FOREGROUND_INTENSITY|FOREGROUND_RGB))) {
					out+= (attr_now&FOREGROUND_INTENSITY) ? '9' : '3';
					out+= '0' + TranslateForegroundColor(attr_now);
					out+= ';';
				}
				if ( attr_prev == (DWORD64)-1 || (tc_back_prev && !tc_back_now)
				|| (attr_prev&(BACKGROUND_INTENSITY|BACKGROUND_RGB)) != (attr_now&(BACKGROUND_INTENSITY|BACKGROUND_RGB))) {
					out+= (attr_now&BACKGROUND_INTENSITY) ? "10" : "4";
					out+= '0' + TranslateBackgroundColor(attr_now);
					out+= ';';
				}

				if (tc_fore_now && (!tc_fore_prev || GET_RGB_FORE(attr_prev) != GET_RGB_FORE(attr_now))) {
					const DWORD rgb = GET_RGB_FORE(attr_now);
					out+= StrPrintf("38;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (tc_back_now && (!tc_back_prev || GET_RGB_BACK(attr_prev) != GET_RGB_BACK(attr_now))) {
					const DWORD rgb = GET_RGB_BACK(attr_now);
					out+= StrPrintf("48;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (out.back() == ';') {
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
		
	public:
		void Add(HANDLE con_hnd, unsigned int Width, const CHAR_INFO *Chars)
		{
			const size_t limit = (size_t)std::max(Opt.CmdLine.VTLogLimit, 2);
			std::lock_guard<std::mutex> lock(_mutex);
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
			g_lines.Add(hConsole, ActualLineWidth(Width, Chars), Chars);
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
	
	static void AppendScreenLines(HANDLE con_hnd, std::string &s, DumpState &ds, bool colored)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> line(csbi.dwSize.X);
			COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
			SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
			for (rc.Top = rc.Bottom = 0; rc.Top < csbi.dwSize.Y; rc.Top = ++rc.Bottom) {
				if (WINPORT(ReadConsoleOutput)(con_hnd, &line[0], buf_size, buf_pos, &rc)) {
					unsigned int width = ActualLineWidth(csbi.dwSize.X, &line[0]);
					if (width || ds.nonempty) {
						ds.nonempty = true;
						EncodeLine(s, width, &line[0], colored);
						s+= NATIVE_EOL;
					}
				}
			}
		}		
	}
	
	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines)
	{
		SYSTEMTIME st;
		WINPORT(GetLocalTime)(&st);
		const auto &path = InMyTempFmt("farvt_%u-%u-%u_%u-%u-%u.%s",
			 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			 colored ? "ans" : "log");
				
		int fd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
		if (fd==-1) {
			fprintf(stderr, "VTLog: errno %u creating '%s'\n", errno, path.c_str() );
			return std::string();
		}
			
		DumpState ds;
		g_lines.DumpToFile(con_hnd, fd, ds, colored);
		if (append_screen_lines) {
			std::string s;
			AppendScreenLines(con_hnd, s, ds, colored);
			if (!s.empty()) {
				if (write(fd, s.c_str(), s.size()) != (int)s.size())
					perror("VTLog: write");				
			}
		}
		close(fd);
		return path;
	}
}



