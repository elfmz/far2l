#include "headers.hpp"

#include "mix.hpp"
#include <mutex>
#include <vector>
#include <deque>
#include <fcntl.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
# include <alloca.h>
#endif

#include "vtlog.h"




namespace VTLog
{
	struct DumpState
	{
		DumpState() : nonempty(false) {}
		
		bool nonempty;
	};
	
	class Lines
	{
		std::mutex _mutex;
		std::deque<std::string> _memories;
		
		enum {
			LIMIT_NOT_IMPORTANT	= 1000,
			LIMIT_IMPORTANT = 5000
		};
		
		void RemoveAllExcept(size_t leave_count)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			while (_memories.size() > leave_count )
				_memories.pop_front();
		}
		
	public:
		~Lines()
		{
			Reset();
		}
		
		void Add(unsigned int Width, const CHAR_INFO *Chars)
		{
			wchar_t *tmp;
			if (Width) {
				tmp = (wchar_t *)alloca((Width + 1) * sizeof(wchar_t));
				for (unsigned int i = 0; i<Width; ++i) 
					tmp[i] = Chars[i].Char.UnicodeChar ? Chars[i].Char.UnicodeChar : L' ';
				tmp[Width] = 0;
			} else
				tmp = NULL;
				
			std::lock_guard<std::mutex> lock(_mutex);
			//a little hustling to reduce reallocations
			_memories.emplace_back();
			
			if (_memories.size() >= LIMIT_IMPORTANT) {
				_memories.back().swap(_memories.front());
				do {
					_memories.pop_front();
				} while (_memories.size() >= LIMIT_IMPORTANT);
			}

			if (tmp) {
				Wide2MB(tmp, _memories.back());
			} else
				_memories.back().clear();
		}
		
		void OnNotImportant()
		{
			RemoveAllExcept(LIMIT_NOT_IMPORTANT);
		}
		
		void DumpToString(std::string &out, DumpState &ds) 
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (const auto &m : _memories) {
				if (ds.nonempty || !m.empty()) {
					ds.nonempty = true;
					out+= m;
					out+= NATIVE_EOL;
				}
			}
		}
		
		void DumpToFile(int fd, DumpState &ds) 
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto m : _memories) {
				if (ds.nonempty || !m.empty()) {
					ds.nonempty = true;
					m+= NATIVE_EOL;
					if (write(fd, m.c_str(), m.size()) != (int)m.size())
						perror("VTLog: WriteToFile");
				}
			}
		}
		
		void Reset()
		{
			RemoveAllExcept(0);
		}

		
	} g_lines;

	static unsigned int g_pause_cnt = 0;
	
	static unsigned int ActualLineWidth(unsigned int Width, const CHAR_INFO *Chars)
	{
		for (;;) {
			if (!Width)
				return 0;
				
			--Width;
			if (Chars[Width].Char.UnicodeChar && Chars[Width].Char.UnicodeChar != L' ') {
				return Width + 1;
			}
		}		
	}
	
	void  OnConsoleScroll(PVOID pContext, unsigned int Width, CHAR_INFO *Chars)
	{
		if (g_pause_cnt == 0) {
			g_lines.Add( ActualLineWidth(Width, Chars), Chars);
		}
	}

	void Pause()
	{
		__sync_add_and_fetch(&g_pause_cnt, 1);
	}

	void Resume()
	{
		if (__sync_sub_and_fetch(&g_pause_cnt, 1) < 0) {
			abort();
		}
	}
	
	void Start()
	{
		g_lines.OnNotImportant();
		WINPORT(SetConsoleScrollCallback) (NULL, OnConsoleScroll, NULL);
	}

	void Stop()
	{
		WINPORT(SetConsoleScrollCallback) (NULL, NULL, NULL);
	}
	
	void Reset()
	{
		g_lines.Reset();
	}
	
	static void AppendScreenLines(std::string &s, DumpState &ds)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> line(csbi.dwSize.X);
			COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
			SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
			for (rc.Top = rc.Bottom = 0; rc.Top < csbi.dwSize.Y; rc.Top = ++rc.Bottom) {
				if (WINPORT(ReadConsoleOutput)(NULL, &line[0], buf_size, buf_pos, &rc)) {
					unsigned int width = ActualLineWidth(csbi.dwSize.X, &line[0]);
					if (width || ds.nonempty) {
						ds.nonempty = true;
						for (unsigned int i = 0; i < width; ++i) {
							WCHAR wz[2] = {line[i].Char.UnicodeChar, 0};
							if (!wz[0]) wz[0] = L' ';
							s+= Wide2MB(wz);
						}
						s+= NATIVE_EOL;					
					}
				}
			}
		}		
	}
	
	void GetAsString(std::string &s, bool append_screen_lines) 
	{
		s.clear();
		DumpState ds;
		g_lines.DumpToString(s, ds);
		if (append_screen_lines) 
			AppendScreenLines(s, ds);
	}
	
	std::string GetAsFile(bool append_screen_lines)
	{
		char name[128];
		SYSTEMTIME st;
		WINPORT(GetLocalTime)(&st);
		sprintf(name, "farvt_%u-%u-%u_%u-%u-%u.log", 
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		std::string path = InMyTemp(name);
				
		std::string s;
		GetAsString(s, append_screen_lines);
		
		int fd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
		if (fd==-1) {
			fprintf(stderr, "VTLog: errno %u creating '%s'\n", errno, path.c_str() );
			return std::string();
		}
			
		DumpState ds;
		g_lines.DumpToFile(fd, ds);
		if (append_screen_lines) {
			std::string s;
			AppendScreenLines(s, ds);
			if (!s.empty()) {
				if (write(fd, s.c_str(), s.size()) != (int)s.size())
					perror("VTLog: write");				
			}
		}
		close(fd);
		return path;
	}
}



