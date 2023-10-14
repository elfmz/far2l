#pragma once
#include <WinCompat.h>
#include <WinPort.h>
#include <StackSerializer.h>
#include <string>
#include <set>
#include "IVTShell.h"

class VTFar2lExtensios
{
	uint64_t _xfeatures = 0;
	int _clipboard_opens = 0;
	int _ctrl_alt_c_counter = 0;

	DWORD _clipboard_read_allowance = 0;
	int _clipboard_read_allowance_prolongs = 0;

	std::string _tmp_input_event;
	IVTShell *_vt_shell;
	std::string _client_id_prefix;

	std::set<std::string> _autheds;
	std::vector<unsigned char> _clipboard_chunks;

	char ClipboardAuthorize(std::string client_id);

	bool IsAllowedClipboardRead();
	void AllowClipboardRead(bool prolong);

	void OnInteract_ClipboardOpen(StackSerializer &stk_ser);
	void OnInteract_ClipboardClose(StackSerializer &stk_ser);
	void OnInteract_ClipboardEmpty(StackSerializer &stk_ser);
	void OnInteract_ClipboardIsFormatAvailable(StackSerializer &stk_ser);
	void OnInteract_ClipboardSetDataChunk(StackSerializer &stk_ser);
	void OnInteract_ClipboardSetData(StackSerializer &stk_ser);
	void OnInteract_ClipboardGetData(StackSerializer &stk_ser);
	void OnInteract_ClipboardGetDataID(StackSerializer &stk_ser);
	void OnInteract_ClipboardRegisterFormat(StackSerializer &stk_ser);
	void OnInteract_Clipboard(StackSerializer &stk_ser);
	void OnInteract_GetLargestWindowSize(StackSerializer &stk_ser);
	void OnInteract_ChangeCursorHeight(StackSerializer &stk_ser);
	void OnInteract_DisplayNotification(StackSerializer &stk_ser);
	void OnInteract_SetFKeyTitles(StackSerializer &stk_ser);
	void OnInteract_GetColorPalette(StackSerializer &stk_ser);

	void WriteInputEvent(const StackSerializer &stk_ser);
public:
	VTFar2lExtensios(IVTShell *vt_shell, const std::string &host_id);
	~VTFar2lExtensios();

	void OnTerminalResized();
	bool OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent);
	bool OnInputKey(const KEY_EVENT_RECORD &KeyEvent);
	void OnInteract(StackSerializer &stk_ser);
};
