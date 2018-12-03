#pragma once
#include "../WinPort/WinCompat.h"
#include "../WinPort/WinPort.h"
#include <StackSerializer.h>
#include <string>
#include "vtansi.h"

class VTFar2lExtensios
{
	int _clipboard_opens = 0;
	int _ctrl_alt_c_counter = 0;

	DWORD _clipboard_read_allowance = 0;
	int _clipboard_read_allowance_prolongs = 0;

	std::string _tmp_input_event;
	IVTAnsiCommands *_ansi_commands;

	bool IsAllowedClipboardRead();
	void AllowClipboardRead(bool prolong);

	void OnInterract_ClipboardOpen(StackSerializer &stk_ser);
	void OnInterract_ClipboardClose(StackSerializer &stk_ser);
	void OnInterract_ClipboardEmpty(StackSerializer &stk_ser);
	void OnInterract_ClipboardIsFormatAvailable(StackSerializer &stk_ser);
	void OnInterract_ClipboardSetData(StackSerializer &stk_ser);
	void OnInterract_ClipboardGetData(StackSerializer &stk_ser);
	void OnInterract_ClipboardRegisterFormat(StackSerializer &stk_ser);
	void OnInterract_Clipboard(StackSerializer &stk_ser);

	void OnInterract_ChangeCursorHeigth(StackSerializer &stk_ser);

	void WriteInputEvent(const StackSerializer &stk_ser);
public:
	VTFar2lExtensios(IVTAnsiCommands *ansi_commands);
	~VTFar2lExtensios();

	bool OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent);
	bool OnInputKey(const KEY_EVENT_RECORD &KeyEvent);
	void OnInterract(StackSerializer &stk_ser);
};
