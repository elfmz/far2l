#include "headers.hpp"
#include <StackSerializer.h>
#include <string>


static void VT_OnFar2lInterract_ClipboardOpen(StackSerializer &stk_ser)
{
	char out = WINPORT(OpenClipboard)(NULL) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

static void VT_OnFar2lInterract_ClipboardClose(StackSerializer &stk_ser)
{
	char out = WINPORT(CloseClipboard)() ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

static void VT_OnFar2lInterract_ClipboardEmpty(StackSerializer &stk_ser)
{
	char out = WINPORT(EmptyClipboard)() ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

static void VT_OnFar2lInterract_ClipboardIsFormatAvailable(StackSerializer &stk_ser)
{
	UINT fmt;
	stk_ser.PopPOD(fmt);
	char out = WINPORT(IsClipboardFormatAvailable)(fmt) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

static void VT_OnFar2lInterract_ClipboardSetData(StackSerializer &stk_ser)
{
	UINT fmt;
	uint32_t len;
	void *data;

	stk_ser.PopPOD(fmt);
	stk_ser.PopPOD(len);
	if (len) {
		data = malloc(len);
		stk_ser.Pop(data, len);
	} else
		data = nullptr;

	char out = WINPORT(SetClipboardData)(fmt, data) ? 1 : 0;
	stk_ser.Clear();
	stk_ser.PushPOD(out);

}

static void VT_OnFar2lInterract_ClipboardGetData(StackSerializer &stk_ser)
{
	UINT fmt;
	stk_ser.PopPOD(fmt);
	void *ptr = WINPORT(GetClipboardData)(fmt);
	stk_ser.Clear();
	uint32_t len = ptr ? GetMallocSize(ptr) : 0;
	if (len)
		stk_ser.Push(ptr, len);
	stk_ser.PushPOD(len);
}

static void VT_OnFar2lInterract_ClipboardRegisterFormat(StackSerializer &stk_ser)
{
	const std::wstring &fmt_name = StrMB2Wide(stk_ser.PopStr());
	UINT out = WINPORT(RegisterClipboardFormat)(fmt_name.c_str());
	stk_ser.Clear();
	stk_ser.PushPOD(out);
}

///////////

static void VT_OnFar2lInterract_Clipboard(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case 'o': VT_OnFar2lInterract_ClipboardOpen(stk_ser); break;
		case 'c': VT_OnFar2lInterract_ClipboardClose(stk_ser); break;
		case 'e': VT_OnFar2lInterract_ClipboardEmpty(stk_ser); break;
		case 'a': VT_OnFar2lInterract_ClipboardIsFormatAvailable(stk_ser); break;
		case 's': VT_OnFar2lInterract_ClipboardSetData(stk_ser); break;
		case 'g': VT_OnFar2lInterract_ClipboardGetData(stk_ser); break;
		case 'r': VT_OnFar2lInterract_ClipboardRegisterFormat(stk_ser); break;

		default:
			fprintf(stderr, "VT_OnFar2lInterract_Clipboard: wrong code %c\n", code);
	}
}

void VT_OnFar2lInterract(StackSerializer &stk_ser)
{
	const char code = stk_ser.PopChar();

	switch (code) {
		case 't': {
			std::string title;
			stk_ser.PopStr(title);
			WINPORT(SetConsoleTitle)( StrMB2Wide(title).c_str() );
			stk_ser.Clear();
		} break;

		case 'e':
			WINPORT(BeginConsoleAdhocQuickEdit)();
			stk_ser.Clear();
		break;

		case 'M':
			WINPORT(SetConsoleWindowMaximized)(TRUE);
			stk_ser.Clear();
		break;

		case 'm':
			WINPORT(SetConsoleWindowMaximized)(FALSE);
			stk_ser.Clear();
		break;

		case 'c':
			VT_OnFar2lInterract_Clipboard(stk_ser);
		break;

		default:
			stk_ser.Clear();
	}
}

