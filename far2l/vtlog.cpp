#include "headers.hpp"
#pragma hdrstop
#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>

#include "vtlog.h"


namespace VTLog
{
	class Lines
	{
		std::mutex _mutex;
		std::list<std::string> _memories;
		
		enum {
			LIMIT_NOT_IMPORTANT	= 100,
			LIMIT_IMPORTANT = 4000
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
				tmp = (wchar_t *)alloca(Width * sizeof(wchar_t));
				for (unsigned int i = 0; i<Width; ++i) 
					tmp[i] = Chars[i].Char.UnicodeChar ? Chars[i].Char.UnicodeChar : L' ';
				tmp[Width] = 0;
			} else
				tmp = NULL;
				
			std::lock_guard<std::mutex> lock(_mutex);
			while (_memories.size() >= LIMIT_IMPORTANT)
				_memories.pop_front();				

			if (tmp)
				_memories.emplace_back(Wide2MB(tmp));
			else
				_memories.emplace_back();
			
		}
		
		void OnNotImportant()
		{
			RemoveAllExcept(LIMIT_NOT_IMPORTANT);
		}
		
		void GetAsString(std::string &out) 
		{
			out.clear();
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto m : _memories) {
				out+= m;
				out+= NATIVE_EOL;
			}
		}
		
		void Reset()
		{
			RemoveAllExcept(0);
		}

		
	} g_lines;
	
	
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
	
	void  OnConsoleScroll(PVOID pContext, unsigned int Top, unsigned int Width, CHAR_INFO *Chars)
	{
		if (Top==0) {
			g_lines.Add( ActualLineWidth(Width, Chars), Chars);
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
	
	void GetAsString(std::string &s, bool append_screen_lines) 
	{
		g_lines.GetAsString(s);
		if (append_screen_lines) {
			CONSOLE_SCREEN_BUFFER_INFO csbi = { };
			if (WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 1) {
				--csbi.dwSize.Y;//dont grab keys
				std::vector<CHAR_INFO> line(csbi.dwSize.X);
				COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
				SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
				for (rc.Top = rc.Bottom = 0; rc.Top < csbi.dwSize.Y; rc.Top = ++rc.Bottom) {
					if (WINPORT(ReadConsoleOutput)(NULL, &line[0], buf_size, buf_pos, &rc)) {
						unsigned int width = ActualLineWidth(csbi.dwSize.X, &line[0]);
						for (unsigned int i = 0; i < width; ++i) {
							WCHAR wz[2] = {line[i].Char.UnicodeChar, 0};
							s+= Wide2MB(wz);
						}
						s+= NATIVE_EOL;
					}
				}
			}
		}
	}
	
	std::string GetAsFile(bool append_screen_lines)
	{
		const char *home = getenv("HOME");
		std::string path = home ? home : "/tmp";
		char name[128];
		SYSTEMTIME st;
		WINPORT(GetLocalTime)(&st);
		sprintf(name, "/farvt_%u-%u-%u_%u-%u-%u.txt", 
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		path+= name;
				
		std::string s;
		GetAsString(s, append_screen_lines);
		
		int fd = open(path.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
		if (fd==-1) {
			fprintf(stderr, "VTHistory: errno %u creating '%s'\n", errno, path.c_str() );
			return std::string();
		}
			
		for (size_t i = 0; i < s.size(); ) {
			size_t piece = s.size() - i;
			if (piece > 0x1000) piece = 0x1000;
			int r = write(fd, &s[i], piece);
			if (r<=0)
					perror("VTHistory: write");
			i+= r;
		}
		close(fd);
		return path;
	}
}


