#pragma once
//#define __USE_BSD 
#include <termios.h> 
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <Event.h>
#include <StackSerializer.h>
#include "Backend.h"
#include "TTYOutput.h"
#include "TTYInput.h"
#include "IFar2lInteractor.h"
#include "TTYXGlue.h"
#include "OSC52ClipboardBackend.h"

class TTYBackend : IConsoleOutputBackend, ITTYInputSpecialSequenceHandler, IFar2lInteractor, IOSC52Interactor
{
	const char *_full_exe_path;
	int _stdin = 0, _stdout = 1;
	bool _ext_clipboard;
	bool _norgb;
	DWORD _nodetect = NODETECT_NONE;
	bool _far2l_tty = false;
	bool _osc52clip_set = false;

	std::mutex _palette_mtx;
	TTYBasePalette _palette;
	bool _override_default_palette = false;
	std::condition_variable _palette_changed_cond;

	enum {
		FKS_UNKNOWN,
		FKS_SUPPORTED,
		FKS_NOT_SUPPORTED
	} _fkeys_support = FKS_UNKNOWN;

	unsigned int _esc_expiration = 0;
	int _notify_pipe = -1;
	int *_result = nullptr;
	int _kickass[2] = {-1, -1};
	int _far2l_cursor_height = -1;
	unsigned int _cur_width = 0, _cur_height = 0;
	unsigned int _prev_width = 0, _prev_height = 0;
	std::vector<CHAR_INFO> _cur_output, _prev_output;

	long _terminal_size_change_id = 0;

	pthread_t _reader_trd = 0;
	volatile bool _exiting = false;
	volatile bool _deadio = false;

	static void *sReaderThread(void *p) { ((TTYBackend *)p)->ReaderThread(); return nullptr; }
	static void *sWriterThread(void *p) { ((TTYBackend *)p)->WriterThread(); return nullptr; }

	void ReaderThread();
	void ReaderLoop();
	void WriterThread();
	void UpdateBackendIdentification();

	std::condition_variable _async_cond;
	std::mutex _async_mutex;
	ITTYXGluePtr _ttyx;
	char _using_extension = 0;

	COORD _largest_window_size{};
	std::atomic<bool> _largest_window_size_ready{false};
	std::atomic<bool> _flush_input_queue{false};


	struct Far2lInteractData
	{
		Event evnt;
		StackSerializer stk_ser;
		bool waited;
	};

	struct Far2lInteractV : std::vector<std::shared_ptr<Far2lInteractData> > {} _far2l_interacts_queued;
	struct Far2lInteractsM : std::map<uint8_t, std::shared_ptr<Far2lInteractData> >, std::mutex
	{
		uint8_t _id_counter = 0;
	} _far2l_interacts_sent;

	struct AsyncEvent
	{
		bool term_resized : 1;
		bool output : 1;
		bool title_changed : 1;
		bool far2l_interact : 1;
		bool go_background : 1;
		bool osc52clip_set : 1;
		bool palette : 1;

		inline bool HasAny() const
		{
			return term_resized || output || title_changed || far2l_interact || go_background || osc52clip_set || palette;
		}
	} _ae{};

	std::string _osc52clip;

	ClipboardBackendSetter _clipboard_backend_setter;

	void GetWinSize(struct winsize &w);
	void ChooseSimpleClipboardBackend();
	void DispatchTermResized(TTYOutput &tty_out);
	void DispatchOutput(TTYOutput &tty_out);
	void DispatchFar2lInteract(TTYOutput &tty_out);
	void DispatchOSC52ClipSet(TTYOutput &tty_out);
	void DispatchPalette(TTYOutput &tty_out);

	void DetachNotifyPipe();

protected:
	// IOSC52Interactor
	virtual void OSC52SetClipboard(const char *text);

	// IFar2lInteractor
	virtual bool Far2lInteract(StackSerializer &stk_ser, bool wait);

	// IConsoleOutputBackend
	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count);
	virtual void OnConsoleOutputResized();
	virtual void OnConsoleOutputTitleChanged();
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos);
	virtual COORD OnConsoleGetLargestWindowSize();
	virtual void OnConsoleAdhocQuickEdit();
	virtual DWORD64 OnConsoleSetTweaks(DWORD64 tweaks);
	virtual void OnConsoleChangeFont();
	virtual void OnConsoleSaveWindowState();
	virtual void OnConsoleSetMaximized(bool maximized);
	virtual void OnConsoleExit();
	virtual bool OnConsoleIsActive();
	virtual void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text);
	virtual bool OnConsoleBackgroundMode(bool TryEnterBackgroundMode);
	virtual bool OnConsoleSetFKeyTitles(const char **titles);
	virtual BYTE OnConsoleGetColorPalette();
	virtual void OnConsoleGetBasePalette(void *pbuff);
	virtual bool OnConsoleSetBasePalette(void *pbuff);
	virtual void OnConsoleOverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK);
	virtual void OnConsoleSetCursorBlinkTime(DWORD interval);

	// ITTYInputSpecialSequenceHandler
	virtual void OnUsingExtension(char extension);
	virtual void OnInspectKeyEvent(KEY_EVENT_RECORD &event);
	virtual void OnFar2lEvent(StackSerializer &stk_ser);
	virtual void OnFar2lReply(StackSerializer &stk_ser);
	virtual void OnInputBroken();

	DWORD QueryControlKeys();

public:
	TTYBackend(const char *full_exe_path, int std_in, int std_out, bool ext_clipboard, bool norgb, DWORD nodetect, bool far2l_tty, unsigned int esc_expiration, int notify_pipe, int *result);
	~TTYBackend();
	void KickAss(bool flush_input_queue = false);
	bool Startup();
};

