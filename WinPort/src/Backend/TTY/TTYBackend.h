#pragma once
#include <mutex>
#include <condition_variable>
#include "Backend.h"
#include "TTYWriter.h"

class TTYBackend : IConsoleOutputBackend, IClipboardBackend
{
	std::mutex _output_mutex;
	unsigned int _cur_width = 0, _cur_height = 0;
	unsigned int _last_width = 0, _last_height = 0;
	std::vector<CHAR_INFO> _last_output;


	TTYWriter _tty_writer;

	pthread_t _reader_trd = 0, _writer_trd = 0;
	volatile bool _exiting = false;

	static void *sWriterThread(void *p) { ((TTYBackend *)p)->WriterThread(); return nullptr; }
	static void *sReaderThread(void *p) { ((TTYBackend *)p)->ReaderThread(); return nullptr; }

	void WriterThread();
	void ReaderThread();


	std::condition_variable _async_cond;
	std::mutex _async_mutex;

	struct AsyncEvent
	{
		bool output = false;
	} _ae;

	void DispatchOutput();

protected:
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
//
	virtual bool OnClipboardOpen();
	virtual void OnClipboardClose();
	virtual void OnClipboardEmpty();
	virtual bool OnClipboardIsFormatAvailable(UINT format);
	virtual void *OnClipboardSetData(UINT format, void *data);
	virtual void *OnClipboardGetData(UINT format);
	virtual UINT OnClipboardRegisterFormat(const wchar_t *lpszFormat);

public:
	~TTYBackend();
	bool Startup();
};

