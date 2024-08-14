#include "headers.hpp"
#include <string>

//See: 
// http://www.manmrk.net/tutorials/ISPF/XE/xehelp/html/HID00000579.htm:

static __thread char s_translate_key_out_buffer[8] = {'\x1b', 0 };

const char *VT_TranslateSpecialKey(const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad, WCHAR uc)
{
	if (key==VK_CONTROL || key==VK_MENU || key==VK_SHIFT)
		return ""; //modifiers should not be sent as keys

	if ( ( (int)ctrl + (int)alt + (int)shift ) > 2) {
		fprintf(stderr, "VT_TranslateSpecialKey: too many modifiers: %u %u %u\n", ctrl, alt, shift);
		return ""; 
	}


		
	switch (key) {
		case VK_RETURN:
			return "\r";

		case VK_TAB:
			return "\t";

		case VK_ESCAPE:
			return "\x1b";

		case VK_F1: /*
		F1                 \x1bOP
		F1                 \x1b[[A
		F1                 \x1b[11~
		F1       Shift     \x1bO2P
		F1       Alt       \x1bO3P
		F1       Ctrl      \x1bO5P */
			if (ctrl && shift) return "\x1b[1;6P"; // \x1bO6P
			if (shift)         return "\x1b[1;2P"; // \x1bO2P
			if (alt)           return "\x1b[1;3P"; // \x1bO3P
			if (ctrl)          return "\x1b[1;5P"; // \x1bO5P
			//if (keypad==2) return "\x1b[11~";
			//if (keypad==1) return "\x1b[[A";
			return "\x1bOP";

		case VK_F2: /*
		F2                 \x1bOQ
		F2                 \x1b[[B
		F2                 \x1b[12~
		F2       Shift     \x1bO2Q
		F2       Alt       \x1bO3Q
		F2       Ctrl      \x1bO5Q */
			if (ctrl && shift) return "\x1b[1;6Q"; // \x1bO6Q
			if (shift)         return "\x1b[1;2Q"; // \x1bO2Q
			if (alt)           return "\x1b[1;3Q"; // \x1bO3Q
			if (ctrl)          return "\x1b[1;5Q"; // \x1bO5Q
			//if (keypad==2) return "\x1b[12~";
			//if (keypad==1) return "\x1b[[B";
			return "\x1bOQ";

		case VK_F3: /*
		F3                 \x1bOR
		F3                 \x1b[[C
		F3                 \x1b[13~
		F3       Shift     \x1bO2R
		F3       Alt       \x1bO3R
		F3       Ctrl      \x1bO5R */
			if (ctrl && shift) return "\x1b[1;6R"; // \x1bO6R
			if (shift)         return "\x1b[1;2R"; // \x1bO2R
			if (alt)           return "\x1b[1;3R"; // \x1bO3R
			if (ctrl)          return "\x1b[1;5R"; // \x1bO5R
			//if (keypad==2) return "\x1b[13~";
			//if (keypad==1) return "\x1b[[C";
			return "\x1bOR";

		case VK_F4: /*
		F4                 \x1bOS
		F4                 \x1b[[D
		F4                 \x1b[14~
		F4       Shift     \x1bO2S
		F4       Alt       \x1bO3S
		F4       Ctrl      \x1bO5S */
			if (ctrl && shift) return "\x1b[1;6S"; // \x1bO6S
			if (shift)         return "\x1b[1;2S"; // \x1bO2S
			if (alt)           return "\x1b[1;3S"; // \x1bO3S
			if (ctrl)          return "\x1b[1;5S"; // \x1bO5S
			//if (keypad==2) return "\x1b[14~";
			//if (keypad==1) return "\x1b[[D";
			return "\x1bOS";

		case VK_F5: /*
		F5                 \x1b[[E
		F5                 \x1b[15~
		F5       Shift     \x1b[15;2~
		F5       Alt       \x1b[15;3~
		F5       Ctrl      \x1b[15;5~ */
			if (ctrl && shift) return "\x1b[15;6~";
			if (shift) return "\x1b[15;2~";
			if (alt) return "\x1b[15;3~";
			if (ctrl) return "\x1b[15;5~";
			//if (keypad==1 || keypad==2) return "\x1b[[E";
			return "\x1b[15~";

		case VK_F6: /*
		F6                 \x1b[17~
		F6       Shift     \x1b[17;2~
		F6       Alt       \x1b[17;3~
		F6       Ctrl      \x1b[17;5~ */
			if (ctrl && shift) return "\x1b[17;6~";
			if (shift) return "\x1b[17;2~";
			if (alt) return "\x1b[17;3~";
			if (ctrl) return "\x1b[17;5~";
			return "\x1b[17~";

		case VK_F7: /*
		F7                 \x1b[18~
		F7       Shift     \x1b[18;2~
		F7       Alt       \x1b[18;3~
		F7       Ctrl      \x1b[18;5~ */
			if (ctrl && shift) return "\x1b[18;6~";
			if (shift) return "\x1b[18;2~";
			if (alt) return "\x1b[18;3~";
			if (ctrl) return "\x1b[18;5~";
			return "\x1b[18~";

		case VK_F8: /*
		F8                 \x1b[19~
		F8       Shift     \x1b[19;2~
		F8       Alt       \x1b[19;3~
		F8       Ctrl      \x1b[19;5~*/
			if (ctrl && shift) return "\x1b[19;6~";
			if (shift) return "\x1b[19;2~";
			if (alt) return "\x1b[19;3~";
			if (ctrl) return "\x1b[19;5~";
			return "\x1b[19~";

		case VK_F9: /*
		F9                 \x1b[20~ 
		F9       Shift     \x1b[20;2~
		F9       Alt       \x1b[20;3~
		F9       Ctrl      \x1b[20;5~ */
			if (ctrl && shift) return "\x1b[20;6~";
			if (shift) return "\x1b[20;2~";
			if (alt) return "\x1b[20;3~";
			if (ctrl) return "\x1b[20;5~";
			return "\x1b[20~";

		case VK_F10: /*
		F10                \x1b[21~
		F10      Shift     \x1b[21;2~
		F10      Alt       \x1b[21;3~
		F10      Ctrl      \x1b[21;5~ */
			if (ctrl && shift) return "\x1b[21;6~";
			if (shift) return "\x1b[21;2~";
			if (alt) return "\x1b[21;3~";
			if (ctrl) return "\x1b[21;5~";
			return "\x1b[21~";

		case VK_F11: /*
		F11                \x1b[23~
		F11      Shift     \x1b[23;2~
		F11      Alt       \x1b[23;3~
		F11      Ctrl      \x1b[23;5~ */
			if (ctrl && shift) return "\x1b[23;6~";
			if (shift) return "\x1b[23;2~";
			if (alt) return "\x1b[23;3~";
			if (ctrl) return "\x1b[23;5~";
			return "\x1b[23~";

		case VK_F12: /*
		F12                \x1b[24~
		F12      Shift     \x1b[24;2~
		F12      Alt       \x1b[24;3~
		F12      Ctrl      \x1b[24;5~ */
			if (ctrl && shift) return "\x1b[24;6~";
			if (shift) return "\x1b[24;2~";
			if (alt) return "\x1b[24;3~";
			if (ctrl) return "\x1b[24;5~";
			return "\x1b[24~";

		case VK_INSERT: /*
		Insert             \x1b[2~
		Insert   Shift     \x1b[2;2~
		Insert   Alt       \x1b[2;3~
		Insert   Ctrl      \x1b[2;5~ */
			if (ctrl && shift) return "\x1b[2;6~";
			if (shift) return "\x1b[2;2~";
			if (alt) return "\x1b[2;3~";
			if (ctrl) return "\x1b[2;5~";
			return "\x1b[2~";

		case VK_DELETE: /*
		Delete             \x1b[3~
		Delete   Shift     \x1b[3;2~
		Delete   Alt       \x1b[3;3~
		Delete   Ctrl      \x1b[3;5~ */
			if (ctrl && shift) return "\x1b[3;6~";
			if (shift) return "\x1b[3;2~";
			if (alt) return "\x1b[3;3~";
			if (ctrl) return "\x1b[3;5~";
			return "\x1b[3~";

		case VK_HOME: /*
		Home               \x1b[1~
		Home               \x1b[H
		Home     Shift     \x1b[2H
		Home     Shift     \x1b[1;2H
		Home     Alt       \x1b[3H
		Home     Alt       \x1b[1;3H
		Home     Ctrl      \x1b[5H
		Home     Ctrl      \x1b[1;5H */
				if (ctrl && shift) return "\x1b[1;6H";
				if (shift) return "\x1b[1;2H";
				if (alt) return "\x1b[1;3H";
				if (ctrl) return "\x1b[1;5H";
			if (keypad == 0) {
				return "\x1b[H";
			}
//			breaks mc editor [Ctrl+]Shift+Home
//			if (ctrl && shift) return "\x1b[6H";
//			if (shift) return "\x1b[2H";
//			if (alt) return "\x1b[3H";
//			if (ctrl) return "\x1b[5H";
			return "\x1bOH";
			//return "\x1b[1~";

		case VK_END: /*
		End                \x1b[4~
		End                \x1b[F
		End      Shift     \x1b[2F
		End      Shift     \x1b[1;2F
		End      Alt       \x1b[3F
		End      Alt       \x1b[1;3F
		End      Ctrl      \x1b[5F
		End      Ctrl      \x1b[1;5F */
				if (ctrl && shift) return "\x1b[1;6F";
				if (shift) return "\x1b[1;2F";
				if (alt) return "\x1b[1;3F";
				if (ctrl) return "\x1b[1;5F";
			if (keypad==0) {
				return "\x1b[F";
			}
//			breaks mc editor [Ctrl+]Shift+End
//			if (ctrl && shift) return "\x1b[6F";
//			if (shift) return "\x1b[2F";
//			if (alt) return "\x1b[3F";
//			if (ctrl) return "\x1b[5F";
			return "\x1bOF";
			//return "\x1b[4~";

		case VK_PRIOR: /*
		PgUp               \x1b[5~
		PgUp     Shift     \x1b[5;2~
		PgUp     Alt       \x1b[5;3~
		PgUp     Ctrl      \x1b[5;5~ */
			if (ctrl && shift) return "\x1b[5;6~";
			if (shift) return "\x1b[5;2~";
			if (alt) return "\x1b[5;3~";
			if (ctrl) return "\x1b[5;5~";
			return "\x1b[5~";

		case VK_NEXT: /*
		PgDn               \x1b[6~
		PgDn     Shift     \x1b[6;2~
		PgDn     Alt       \x1b[6;3~
		PgDn     Ctrl      \x1b[6;5~ */
			if (ctrl && shift) return "\x1b[6;6~";
			if (shift) return "\x1b[6;2~";
			if (alt) return "\x1b[6;3~";
			if (ctrl) return "\x1b[6;5~";
			return "\x1b[6~";

		case VK_UP: /*
		Up                 \x1bOA
		Up                 \x1b[A
		Up       Shift     \x1b[2A
		Up       Shift     \x1b[1;2A
		Up       Alt       \x1b[3A
		Up       Alt       \x1b[1;3A
		Up       Ctrl      \x1b[5A
		Up       Ctrl      \x1b[1;5A */
			if (ctrl && shift) return "\x1b[1;6A";
			if (shift) return "\x1b[1;2A";
			if (alt) return "\x1b[1;3A";
			if (ctrl) return "\x1b[1;5A";
			if (keypad == 1) {
				return "\x1bOA";
			}
//			breaks free pascal ide
//			if (ctrl && shift) return "\x1b[6A";
//			if (shift) return "\x1b[2A";
//			if (alt) return "\x1b[3A";
//			if (ctrl) return "\x1b[5A";
			return "\x1b[A";

		case VK_DOWN: /*
		Down               \x1bOB
		Down               \x1b[B
		Down     Shift     \x1b[2B
		Down     Shift     \x1b[1;2B
		Down     Alt       \x1b[3B
		Down     Alt       \x1b[1;3B
		Down     Ctrl      \x1b[5B
		Down     Ctrl      \x1b[1;5B */
			if (ctrl && shift) return "\x1b[1;6B";
			if (shift) return "\x1b[1;2B";
			if (alt) return "\x1b[1;3B";
			if (ctrl) return "\x1b[1;5B";
			if (keypad == 1) {
				return "\x1bOB";
			}
//			breaks free pascal ide
//			if (ctrl && shift) return "\x1b[6B";
//			if (shift) return "\x1b[2B";
//			if (alt) return "\x1b[3B";
//			if (ctrl) return "\x1b[5B";
			return "\x1b[B";

		case VK_LEFT: /*
		Left               \x1bOD
		Left               \x1b[D
		Left     Shift     \x1b[2D
		Left     Shift     \x1b[1;2D
		Left     Alt       \x1b[3D
		Left     Alt       \x1b[1;3D
		Left     Ctrl      \x1b[5D
		Left     Ctrl      \x1b[1;5D */
			if (ctrl && shift) return "\x1b[1;6D";
			if (shift) return "\x1b[1;2D";
			if (alt) return "\x1b[1;3D";
			if (ctrl) return "\x1b[1;5D";
			if (keypad == 1) {
				return "\x1bOD";
			}
//			breaks free pascal ide
//			if (ctrl && shift) return "\x1b[6D";
//			if (shift) return "\x1b[2D";
//			if (alt) return "\x1b[3D";
//			if (ctrl) return "\x1b[5D";
			return "\x1b[D";

		case VK_RIGHT: /*
		Right              \x1bOC
		Right              \x1b[C
		Right    Shift     \x1b[2C
		Right    Shift     \x1b[1;2C
		Right    Alt       \x1b[3C
		Right    Alt       \x1b[1;3C
		Right    Ctrl      \x1b[5C
		Right    Ctrl      \x1b[1;5C */
			if (ctrl && shift) return "\x1b[1;6C";
			if (shift) return "\x1b[1;2C";
			if (alt) return "\x1b[1;3C";
			if (ctrl) return "\x1b[1;5C";
			if (keypad == 1) {
				return "\x1bOC";
			}
//			breaks free pascal ide
//			if (ctrl && shift) return "\x1b[6C";
//			if (shift) return "\x1b[2C";
//			if (alt) return "\x1b[3C";
//			if (ctrl) return "\x1b[5C";
			return "\x1b[C";
	}

	if (ctrl && !alt && !shift) {
		switch (key) {
		case 'A': return "\x01";
		case 'B': return "\x02";
		case 'C': return "\x03";
		case 'D': return "\x04";
		case 'E': return "\x05";
		case 'F': return "\x06";
		case 'G': return "\x07";
		case 'H': return "\x08";
		case 'I': return "\x09";
		case 'J': return "\x0a";
		case 'K': return "\x0b";
		case 'L': return "\x0c";
		case 'M': return "\x0d";
		case 'N': return "\x0e";
		case 'O': return "\x0f";
		case 'P': return "\x10";
		case 'Q': return "\x11";
		case 'R': return "\x12";
		case 'S': return "\x13";
		case 'T': return "\x14";
		case 'U': return "\x15";
		case 'V': return "\x16";
		case 'W': return "\x17";
		case 'X': return "\x18";
		case 'Y': return "\x19";
		case 'Z': return "\x1a";
		case VK_OEM_4: return "\x1b";//'['
		case VK_OEM_5: return "\x1c";//'\\'
		case VK_OEM_6: return "\x1d";//']'
		}
	}

	if (alt && !ctrl) {
		// Alt+letter, try do process as many cases as possible.
		// Non-latin support not guaranteed on wx<3.1.3.
		// We check both key and unicodechar
		// as key is untrustable under wx>=3.1.3 for non-latin,
		// and unicodechar is untrustable under wx<3.1.3
		// (menu hack does not fill it by default).
		// Our wx Alt+letter hack[s] emulate[s] uppercase key events,
		// so let us translate to needed case depending on shift state (todo: check caps)

		if (uc) {

			// Unicode character can be absent (zero), but is not expected to be wrong.
			// And the key value can sometimes be '-' (even if other key is pressed),
			// this is caused by wx workaround. Not a big problem, as unicodechar should
			// be present in all such cases. So unicodechar should be checked first.

			wchar_t uc_right_case[] = {wchar_t(shift ? towupper(uc) : towlower(uc)), 0};
			wcstombs(&s_translate_key_out_buffer[1], &uc_right_case[0],
				sizeof(s_translate_key_out_buffer) - 1);

		} else if ((key >= 'A') && (key <= 'Z')) {
			s_translate_key_out_buffer[1] = shift ? key : key + 32;

		} else {
			return NULL;
		}

		return &s_translate_key_out_buffer[0];
	}
	
	return NULL;
}
