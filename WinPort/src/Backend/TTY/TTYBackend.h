#pragma once
//#define __USE_BSD 
#include <termios.h> 
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "Backend.h"
#include "TTYOutput.h"
#include "TTYInput.h"
#include "../FSClipboardBackend.h"

class TTYBackend : IConsoleOutputBackend, ITTYInputSpecialSequenceHandler
{
	std::mutex _output_mutex;
	int _stdin = 0, _stdout = 1, _kickass[2] = {-1, -1};
	unsigned int _cur_width = 0, _cur_height = 0;
	unsigned int _prev_width = 0, _prev_height = 0;
	std::vector<CHAR_INFO> _cur_output, _prev_output;


	struct termios _ts;
	int _ts_r = -1;
	long _terminal_size_change_id;

	pthread_t _reader_trd = 0, _writer_trd = 0;
	volatile bool _exiting = false;
	std::atomic<bool> _status_updated, _vt_far2l;

	static void *sWriterThread(void *p) { ((TTYBackend *)p)->WriterThread(); return nullptr; }
	static void *sReaderThread(void *p) { ((TTYBackend *)p)->ReaderThread(); return nullptr; }

	void WriterThread();
	void ReaderThread();


	std::condition_variable _async_cond;
	std::mutex _async_mutex;

	union AsyncEvent
	{
		struct {
			bool term_resized : 1;
			bool output : 1;
		} flags;
		uint32_t all;
	} _ae;

	void DispatchTermResized(TTYOutput &tty_out);
	void DispatchOutput(TTYOutput &tty_out);
	void OnFar2lKey(bool down, const std::vector<uint32_t> &args);
	void OnFar2lMouse(const std::vector<uint32_t> &args);

	std::shared_ptr<IClipboardBackend> _clipboard_backend;

protected:
	// IConsoleOutputBackend
	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count);
	virtual void OnConsoleOutputResized();
	virtual void OnConsoleOutputTitleChanged();
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos);
	virtual COORD OnConsoleGetLargestWindowSize();
	virtual void OnConsoleAdhocQuickEdit();
	virtual DWORD OnConsoleSetTweaks(DWORD tweaks);
	virtual void OnConsoleChangeFont();
	virtual void OnConsoleSetMaximized(bool maximized);
	virtual void OnConsoleExit();
	virtual bool OnConsoleIsActive();

	// ITTYInputSpecialSequenceHandler
	virtual void OnFar2lEvent(char code, const std::vector<uint32_t> &args);

public:
	TTYBackend(int std_in, int std_out);
	~TTYBackend();
	void KickAss();
	bool Startup();
};

