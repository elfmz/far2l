#include <stdarg.h>
#include "TTYInputSequenceParser.h"

//See: 
// http://www.manmrk.net/tutorials/ISPF/XE/xehelp/html/HID00000579.htm
// http://www.leonerd.org.uk/hacks/fixterms/

#if 1 // change to 1 to enable self-contradiction check on startup

template <typename Last> static void AssertNoConflictsBetween(const Last &last) { }

template <typename First, typename Second, class ... Types>
	static void AssertNoConflictsBetween(const First &first, const Second &second, Types ... others)
{
	for (const auto &f_i : first) {
		for (const auto &s_i : second) {
			if (memcmp(f_i.first.chars, s_i.first.chars,
				std::min(sizeof(f_i.first.chars), sizeof(s_i.first.chars)) ) == 0) {
				fprintf(stderr, "AssertNoConflictsBetween: '%s' vs '%s'\n",
					std::string(f_i.first.chars, sizeof(f_i.first.chars)).c_str(),
					std::string(s_i.first.chars, sizeof(s_i.first.chars)).c_str());
				abort();
			}
		}
	}
	
	AssertNoConflictsBetween(second, others...);
}


void TTYInputSequenceParser::AssertNoConflicts()
{
	AssertNoConflictsBetween(_nch2key1, _nch2key2, _nch2key3,
		_nch2key4, _nch2key5, _nch2key6, _nch2key7, _nch2key8);
}

#else
void TTYInputSequenceParser::AssertNoConflicts()
{
}
#endif


void TTYInputSequenceParser::AddStr(WORD vk, DWORD control_keys, const char *fmt, ...)
{
	char tmp[32];
	va_list va;
	va_start(va, fmt);
	int r = vsnprintf (&tmp[0], sizeof(tmp), fmt, va);
	va_end(va);

	fprintf(stderr, "TTYInputSequenceParser::AddStr(0x%x, 0x%x, '%s'): '%s' r=%d\n", vk, control_keys, fmt, tmp, r);

	TTYInputKey k = {vk, control_keys};
	switch (r) {
		case 1: _nch2key1.Add(tmp, k); break;
		case 2: _nch2key2.Add(tmp, k); break;
		case 3: _nch2key3.Add(tmp, k); break;
		case 4: _nch2key4.Add(tmp, k); break;
		case 5: _nch2key5.Add(tmp, k); break;
		case 6: _nch2key6.Add(tmp, k); break;
		case 7: _nch2key7.Add(tmp, k); break;
		case 8: _nch2key8.Add(tmp, k); break;
		default:
			fprintf(stderr,
				"TTYInputSequenceParser::AddStr(0x%x, 0x%x, '%s') - unexpected: %d\n",
				vk, control_keys, fmt, r);
			abort();
	}

	

	
}

void TTYInputSequenceParser::AddStrTilde(WORD vk, int code)
{
	AddStr(vk, 0, "[%d~", code);
	for (int i = 0; i <= 7; ++i) {		
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, "[%d;%d~", code, 1 + i);
	}
}

void TTYInputSequenceParser::AddStr_ControlsThenCode(WORD vk, const char *fmt, const char *code)
{
	for (int i = 0; i <= 7; ++i) {		
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, fmt, 1 + i, code);
	}
}

void TTYInputSequenceParser::AddStr_CodeThenControls(WORD vk, const char *fmt, const char *code)
{
	for (int i = 0; i <= 7; ++i) {		
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, fmt, code, 1 + i);
	}
}

void TTYInputSequenceParser::AddStrF1F5(WORD vk, const char *code)
{
	AddStr(vk, 0, "O%s", code);
	AddStr_ControlsThenCode(vk, "O%d%s", code);
}

void TTYInputSequenceParser::AddStrCursors(WORD vk, const char *code)
{
	AddStr(vk, 0, "O%s", code);
	AddStr(vk, 0, "[%s", code);
	AddStr_ControlsThenCode(vk, "[%d%s", code);
	AddStr_ControlsThenCode(vk, "[1;%d%s", code);
}

TTYInputSequenceParser::TTYInputSequenceParser()
{
	AddStrCursors(VK_UP, "A");
	AddStrCursors(VK_DOWN, "B");
	AddStrCursors(VK_RIGHT, "C");
	AddStrCursors(VK_LEFT, "D");
	AddStrCursors(VK_END, "F");
	AddStrCursors(VK_HOME, "H");
	AddStrF1F5(VK_F1, "P"); AddStr(VK_F1, 0, "[[A");
	AddStrF1F5(VK_F2, "Q"); AddStr(VK_F2, 0, "[[B");
	AddStrF1F5(VK_F3, "R"); AddStr(VK_F3, 0, "[[C");
	AddStrF1F5(VK_F4, "S"); AddStr(VK_F4, 0, "[[D");
	AddStrF1F5(VK_F5, "E"); AddStr(VK_F5, 0, "[[E");

	AddStrTilde(VK_HOME, 1);
	AddStrTilde(VK_INSERT, 2);
	AddStrTilde(VK_DELETE, 3);
	AddStrTilde(VK_END, 4);
	AddStrTilde(VK_PRIOR, 5);
	AddStrTilde(VK_NEXT, 6);

	AddStrTilde(VK_F1, 11);
	AddStrTilde(VK_F2, 12);
	AddStrTilde(VK_F3, 13);
	AddStrTilde(VK_F4, 14);
	AddStrTilde(VK_F5, 16);
	AddStrTilde(VK_F6, 17);
	AddStrTilde(VK_F7, 18);
	AddStrTilde(VK_F8, 19);
	AddStrTilde(VK_F9, 20);
	AddStrTilde(VK_F10, 21);
	AddStrTilde(VK_F11, 23);
	AddStrTilde(VK_F12, 24);

	AddStr(VK_ESCAPE, 0, "\x1b");

	AssertNoConflicts();
}

size_t TTYInputSequenceParser::Parse(TTYInputKey &k, const char *s, size_t l)
{
	switch (*s) {
		case 0x1b:
			++s;
			--l;

			switch (l >= 8 ? 8 : l) {
				case 8: if (_nch2key8.Lookup(s, k)) return 8 + 1;
				case 7: if (_nch2key7.Lookup(s, k)) return 7 + 1;
				case 6: if (_nch2key6.Lookup(s, k)) return 6 + 1;
				case 5: if (_nch2key5.Lookup(s, k)) return 5 + 1;
				case 4: if (_nch2key4.Lookup(s, k)) return 4 + 1;
				case 3: if (_nch2key3.Lookup(s, k)) return 3 + 1;
				case 2: if (_nch2key2.Lookup(s, k)) return 2 + 1;
				case 1: if (_nch2key1.Lookup(s, k)) return 1 + 1;
			}

			return (l >= 8) ? (size_t)-2 : 0;

		case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
		case 0x0a: case 0x0b: case 0x0c: case 0x0e: case 0x0f: case 0x10: case 0x11: case 0x12:
		case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a:
			k = TTYInputKey{WORD('A' + (*s - 0x01)), LEFT_CTRL_PRESSED};
			return 1;

		case 0x09:
			k = TTYInputKey{VK_TAB, 0};
			return 1;

		case 0x0d:
			k = TTYInputKey{VK_RETURN, 0};
			return 1;

//		case 0x1b:
//			k = TTYInputKey{VK_OEM_4, 0};
//			return 1;

		case 0x1c:
			k = TTYInputKey{VK_OEM_5, 0};
			return 1;

		case 0x1d:
			k = TTYInputKey{VK_OEM_6, 0};
			return 1;


		case 0x7f:
			k = TTYInputKey{VK_BACK, 0};
			return 1;

		default:
			return (size_t)-1;
	}

	abort();
}
