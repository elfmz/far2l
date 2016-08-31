#include "headers.hpp"
#pragma hdrstop
#include "mix.hpp"
#include <mutex>
#include <vector>
#include <list>
#include <fcntl.h>

#include "vthistory.h"


namespace VTHistory
{
	class Lines
	{
		std::mutex _mutex;
		std::list<wchar_t *> _memories;
		
		enum {
			LIMIT_NOT_IMPORTANT	= 100,
			LIMIT_IMPORTANT = 1000
		};
		
	public:
		~Lines()
		{
			for (auto m : _memories) free(m);
			_memories.clear();
		}
		
		void Add(unsigned int Width, const CHAR_INFO *Chars)
		{
			wchar_t *line;
			if (Width) {
				line = (wchar_t *)malloc(sizeof(wchar_t) * (Width + 1));
				for (unsigned int i = 0; i<Width; ++i) 
					line[i] = Chars[i].Char.UnicodeChar;

				line[Width] = 0;
			} else
				line = NULL;
				
			std::lock_guard<std::mutex> lock(_mutex);
			while (_memories.size() >= LIMIT_IMPORTANT) {
				free(_memories.front());
				_memories.pop_front();				
			}
			_memories.push_back(line);
			
		}
		
		void OnNotImportant()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			while (_memories.size() > LIMIT_NOT_IMPORTANT) {
				free(_memories.front());
				_memories.pop_front();
			}
		}
		
		void GetAsString(std::string &out) {
			out.clear();
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto m : _memories) {
				if (m) out+= Wide2MB(m);
				out+= NATIVE_EOL;
			}
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
	
	void GetAsString(std::string &s, unsigned short append_screen_lines) 
	{
		g_lines.GetAsString(s);
		if (append_screen_lines) {
			CONSOLE_SCREEN_BUFFER_INFO csbi = { };
			if (WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi) && csbi.dwSize.X > 0) {
				std::vector<CHAR_INFO> line(csbi.dwSize.X);
				COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
				SMALL_RECT rc = {0, 0, (SHORT) (csbi.dwSize.X - 1), 0};
				for (rc.Top = rc.Bottom = 0; (rc.Top < csbi.dwSize.Y && rc.Top < append_screen_lines); rc.Top = ++rc.Bottom) {
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
	
	string GetAsFile(unsigned short append_screen_lines)
	{
		string path;
		if (!FarMkTempEx(path))
				return string();
				
		std::string s;
		GetAsString(s, append_screen_lines);
		
		int fd = open(Wide2MB(path).c_str(), O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0600);
		if (fd!=-1) {
			for (size_t i = 0; i < s.size(); ) {
				size_t piece = s.size() - i;
				if (piece > 0x1000) piece = 0x1000;
				int r = write(fd, &s[i], piece);
				if (r<=0)
						perror("write");
				i+= r;
			}
			close(fd);		
		}
		return path;
	}
}


