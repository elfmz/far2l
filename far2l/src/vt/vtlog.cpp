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
	// Эта структура используется для отслеживания состояния при выгрузке в файл.
	struct DumpState
	{
		DumpState() : nonempty(false) {}
		bool nonempty;
	};

	static inline unsigned char TranslateForegroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes & FOREGROUND_RED) out |= 1;
		if (attributes & FOREGROUND_GREEN) out |= 2;
		if (attributes & FOREGROUND_BLUE) out |= 4;
		return out;
	}

	static inline unsigned char TranslateBackgroundColor(WORD attributes)
	{
		unsigned char out = 0;
		if (attributes & BACKGROUND_RED) out |= 1;
		if (attributes & BACKGROUND_GREEN) out |= 2;
		if (attributes & BACKGROUND_BLUE) out |= 4;
		return out;
	}

	// Эта функция остаётся, но теперь не используется, так как строки хранятся целиком.
	// Оставляем на случай, если понадобится для анализа активного экрана.
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

	// Кодирует одну строку (вектор CHAR_INFO) в ANSI-последовательность.
	static void EncodeLine(std::string &out, const std::vector<CHAR_INFO>& line, bool colored)
	{
		DWORD64 attr_prev = (DWORD64)-1;
		for (const auto& Char : line) {
			if (Char.Char.UnicodeChar == 0) continue;

			const DWORD64 attr_now = Char.Attributes;
			if (colored && attr_now != attr_prev) {
				const bool tc_back_now = (attr_now & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_back_prev = (attr_prev & BACKGROUND_TRUECOLOR) != 0;
				const bool tc_fore_now = (attr_now & FOREGROUND_TRUECOLOR) != 0;
				const bool tc_fore_prev = (attr_prev & FOREGROUND_TRUECOLOR) != 0;

				const size_t out_len_before_attr = out.size();
				out += "\033[";
				if (attr_prev == (DWORD64)-1
					|| (attr_prev & FOREGROUND_INTENSITY) != (attr_now & FOREGROUND_INTENSITY)) {
					out += (attr_now & FOREGROUND_INTENSITY) ? "1:" : "22;";
				}
				if (attr_prev == (DWORD64)-1 || (tc_fore_prev && !tc_fore_now)
					|| (attr_prev & (FOREGROUND_INTENSITY | FOREGROUND_RGB)) != (attr_now & (FOREGROUND_INTENSITY | FOREGROUND_RGB))) {
					out += (attr_now & FOREGROUND_INTENSITY) ? '9' : '3';
					out += '0' + TranslateForegroundColor(attr_now);
					out += ';';
				}
				if (attr_prev == (DWORD64)-1 || (tc_back_prev && !tc_back_now)
					|| (attr_prev & (BACKGROUND_INTENSITY | BACKGROUND_RGB)) != (attr_now & (BACKGROUND_INTENSITY | BACKGROUND_RGB))) {
					out += (attr_now & BACKGROUND_INTENSITY) ? "10" : "4";
					out += '0' + TranslateBackgroundColor(attr_now);
					out += ';';
				}

				if (tc_fore_now && (!tc_fore_prev || GET_RGB_FORE(attr_prev) != GET_RGB_FORE(attr_now))) {
					const DWORD rgb = GET_RGB_FORE(attr_now);
					out += StrPrintf("38;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (tc_back_now && (!tc_back_prev || GET_RGB_BACK(attr_prev) != GET_RGB_BACK(attr_now))) {
					const DWORD rgb = GET_RGB_BACK(attr_now);
					out += StrPrintf("48;2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
				}

				if (out.back() == ':') {
					out.back() = 'm';
					attr_prev = attr_now;
				} else {
					out.resize(out_len_before_attr);
				}
			}

			if (CI_USING_COMPOSITE_CHAR(Char)) {
				const wchar_t *pwc = WINPORT(CompositeCharLookup)(Char.Char.UnicodeChar);
				Wide2MB_UnescapedAppend(pwc, wcslen(pwc), out);
			} else if (Char.Char.UnicodeChar > 0x80) {
				Wide2MB_UnescapedAppend(Char.Char.UnicodeChar, out);
			} else {
				out += (char)(unsigned char)Char.Char.UnicodeChar;
			}
		}
		if (colored && attr_prev != (DWORD64)-1) {
			out += "\033[m";
		}
	}

	// Кодирует одну строку (указатель на CHAR_INFO) в ANSI-последовательность.
	// Используется для захвата активного экрана.
	static void EncodeLine(std::string &out, unsigned int Width, const CHAR_INFO *Chars, bool colored)
	{
		std::vector<CHAR_INFO> line_vec(Chars, Chars + Width);
		EncodeLine(out, line_vec, colored);
	}


	// Новый менеджер для хранения истории (scrollback).
	static class ScrollbackManager
	{
		std::mutex _mutex;
		// Каждая запись - это пара из хендла консоли и самой логической строки.
		std::list<std::pair<HANDLE, std::vector<CHAR_INFO>>> _logical_lines;
		unsigned int _pause_cnt = 0;

	public:
		void Add(HANDLE con_hnd, const std::vector<CHAR_INFO>& line)
		{
			if (_pause_cnt != 0 || line.empty()) {
				return;
			}

			const size_t limit = (size_t)std::max(Opt.CmdLine.VTLogLimit, 10000);
			std::lock_guard<std::mutex> lock(_mutex);

			_logical_lines.emplace_back(con_hnd, line);

			while (_logical_lines.size() > limit) {
				_logical_lines.pop_front();
			}
		}

		void DumpToFile(HANDLE con_hnd, int fd, DumpState &ds, bool colored)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (const auto& mem : _logical_lines) {
				// Отображаем строки для текущей активной команды (con_hnd) И все предыдущие (NULL)
				if (mem.first == con_hnd || mem.first == NULL) {
					std::string encoded_line;
					EncodeLine(encoded_line, mem.second, colored);

					if (ds.nonempty || !encoded_line.empty()) {
						ds.nonempty = true;
						if (!colored) {
							// Упрощенное удаление ANSI-кодов для текстового лога.
							for (;;) {
								size_t i = encoded_line.find('\033');
								if (i == std::string::npos) break;
								size_t j = encoded_line.find('m', i + 1);
								if (j == std::string::npos) break;
								encoded_line.erase(i, j + 1 - i);
							}
						}
						encoded_line += NATIVE_EOL;
						if (write(fd, encoded_line.c_str(), encoded_line.size()) != (int)encoded_line.size())
							perror("VTLog: WriteToFile");
					}
				}
			}
		}

		void Reset(HANDLE con_hnd)
		{
			// Никогда не очищаем общую историю (которая хранится с меткой NULL)
			if (con_hnd == NULL)
				return;

			std::lock_guard<std::mutex> lock(_mutex);
			_logical_lines.remove_if([con_hnd](const auto& val) {
				return val.first == con_hnd;
			});
		}

		void ConsoleJoined(HANDLE con_hnd)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto& line : _logical_lines) {
				if (line.first == con_hnd) {
					line.first = NULL;
				}
			}
		}

		void Pause()
		{
			__sync_add_and_fetch(&_pause_cnt, 1);
		}

		void Resume()
		{
			if (__sync_sub_and_fetch(&_pause_cnt, 1) < 0) {
				ABORT();
			}
		}

	} g_scrollback;

	void AddLogicalLine(HANDLE con_hnd, const std::vector<CHAR_INFO>& line)
	{
		g_scrollback.Add(con_hnd, line);
	}

	void Pause()
	{
		g_scrollback.Pause();
	}

	void Resume()
	{
		g_scrollback.Resume();
	}

	void Start()
	{
		// OnConsoleScroll больше не используется, эта функция теперь пустая.
	}

	void Stop()
	{
		// OnConsoleScroll больше не используется, эта функция теперь пустая.
	}

	void ConsoleJoined(HANDLE con_hnd)
	{
		g_scrollback.ConsoleJoined(con_hnd);
	}

	void Reset(HANDLE con_hnd)
	{
		g_scrollback.Reset(con_hnd);
	}

	static void AppendScreenLine(const CHAR_INFO *line, unsigned int width, std::string &s, DumpState &ds, bool colored)
	{
		// Используем старую функцию для определения реальной ширины строки на экране.
		width = ActualLineWidth(width, line);
		if (width || ds.nonempty) {
			ds.nonempty = true;
			std::vector<CHAR_INFO> line_vec(line, line + width);
			EncodeLine(s, line_vec, colored);
			s += NATIVE_EOL;
		}
	}

	static void AppendActiveScreenLines(HANDLE con_hnd, std::string &s, DumpState &ds, bool colored)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { };
		if (WINPORT(GetConsoleScreenBufferInfo)(con_hnd, &csbi) && csbi.dwSize.X > 0 && csbi.dwSize.Y > 0) {
			std::vector<CHAR_INFO> line(csbi.dwSize.X);
			COORD buf_pos = { }, buf_size = {csbi.dwSize.X, 1};
			SMALL_RECT rc = {0, 0, (SHORT)(csbi.dwSize.X - 1), 0};
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
					ci += w;
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
		if (fd == -1) {
			fprintf(stderr, "VTLog: errno %u creating '%s'\n", errno, path.c_str());
			return std::string();
		}

		DumpState ds;
		// Выгружаем историю из нашего нового менеджера.
		g_scrollback.DumpToFile(con_hnd, fd, ds, colored);

		// Добавляем содержимое активного экрана ТОЛЬКО если наша история пуста.
		// Это гарантирует, что мы не смешаем длинные логические строки с "нарезанными" экранными.
		if (append_screen_lines && !ds.nonempty) {
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