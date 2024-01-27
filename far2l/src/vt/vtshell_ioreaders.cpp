#include "headers.hpp"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <os_call.hpp>
#include "vtshell_ioreaders.h"
#include "InterThreadCall.hpp"

WithThread::WithThread()
	: _started(false), _thread(0)
{
}
	
WithThread::~WithThread()
{
	ASSERT(!_started);
}

bool WithThread::Start()
{
	ASSERT(!_started);
	_started = true;
	if (pthread_create(&_thread, NULL, sThreadProc, this)) {
		perror("VT: pthread_create");
		_started = false;
		return false;
	}		
	return true;
}

void WithThread::Join()
{
	if (_started) {
		_started = false;
		OnJoin();
		pthread_join(_thread, NULL);
		_thread = 0;
	}
}

void WithThread::OnJoin()
{
}

void *WithThread::sThreadProc(void *p)
{
	return ((WithThread *)p)->ThreadProc();
}

//////////////////////

VTOutputReader::VTOutputReader(IProcessor *processor) 
	: _processor(processor), _fd_out(-1), _deactivated(false)
{
	_pipe[0] = _pipe[1] = -1;
}
	
VTOutputReader::~VTOutputReader()
{
	Stop();
	CheckedCloseFDPair(_pipe);
}
	
void VTOutputReader::Start(int fd_out)
{
	if (fd_out != -1 ) {
		_fd_out = fd_out;
		InterThreadLock itl;
		_deactivated = false;
	}

	if (_pipe[0] == -1) {
		if (pipe_cloexec(_pipe)==-1) {
			perror("VTOutputReader: pipe_cloexec 1");
			_pipe[0] = _pipe[1] = -1;
			return;
		}
	}

	if (!WithThread::Start()) {
		perror("VTOutputReader::Start");
	}
}
	
void VTOutputReader::Stop()
{
	if (_started) {
		Join();
		CheckedCloseFDPair(_pipe);
	}
}

void VTOutputReader::KickAss()
{
	char c = 0;
	if (os_call_ssize(write, _pipe[1], (const void *)&c, sizeof(c)) != sizeof(c))
		perror("VTOutputReader::Stop - write");
}

void VTOutputReader::OnJoin()
{
	KickAss();
	WithThread::OnJoin();
}

void *VTOutputReader::ThreadProc()
{
	char buf[0x1000];
	fd_set rfds;
		
	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(_fd_out, &rfds);
		FD_SET(_pipe[0], &rfds);
			
		int r = os_call_int(select, std::max(_fd_out, _pipe[0]) + 1, &rfds, (fd_set *)nullptr, (fd_set *)nullptr, (timeval *)nullptr);
		if (r <= 0) {
			perror("VTOutputReader select");
			break;
		}
		if (FD_ISSET(_fd_out, &rfds)) {
			r = os_call_ssize(read, _fd_out, (void *)buf, sizeof(buf));
			if (r <= 0) break;
#if 1 //set to 0 to test extremely fragmented output processing 
			if (!_processor->OnProcessOutput(buf, r)) break;
#else 
			for (int i = 0; r > 0;) {
				int n = 1 + (rand()%7);
				if (n > r) n = r;
				if (!_processor->OnProcessOutput(&buf[i], n)) break;
				i+= n;
				r-= n;
			}
			if (r) break;
#endif
		}
		if (FD_ISSET(_pipe[0], &rfds)) {
			r = os_call_ssize(read, _pipe[0], (void *)buf, sizeof(buf));
			if (r < 0) {
				perror("VTOutputReader read pipe[0]");
				break;
			}
		}
		if (!_started)
			return NULL; //stop thread requested
	}

	//thread stopped due to output deactivated
	InterThreadLockAndWake itlw;
	_deactivated = true;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

VTInputReader::VTInputReader(IProcessor *processor) : _stop(false), _processor(processor)
{
}
	
void VTInputReader::Start(HANDLE con_hnd)
{
	if (!_started) {
		_con_hnd = con_hnd;
		_stop = false;
		WithThread::Start();
	}
}

void VTInputReader::Stop()
{
	if (_started) {
		_stop = true;
		Join();
	}
}

void VTInputReader::InjectInput(const char *str, size_t len)
{
	{
		std::lock_guard<std::mutex> locker(_pending_injected_inputs_mutex);
		_pending_injected_inputs.emplace_back(str, len);
	}
	KickInputThread();
}

void VTInputReader::OnJoin()
{
	KickInputThread();
	WithThread::OnJoin();
}

void VTInputReader::KickInputThread()
{
	// write some dummy console input to kick pending ReadConsoleInput
	INPUT_RECORD ir = {};
	ir.EventType = NOOP_EVENT;
	DWORD dw = 0;
	WINPORT(WriteConsoleInput)(_con_hnd, &ir, 1, &dw);
}

void *VTInputReader::ThreadProc()
{
	std::list<std::string> pending_injected_inputs;
	while (!_stop) {
		INPUT_RECORD ir = {0};
		DWORD dw = 0;
		if (!WINPORT(ReadConsoleInput)(_con_hnd, &ir, 1, &dw)) {
			perror("VT: ReadConsoleInput");
			usleep(100000);
		} else if (ir.EventType == MOUSE_EVENT) {
			_processor->OnInputMouse(ir.Event.MouseEvent);

		} else if (ir.EventType == KEY_EVENT) {
			_processor->OnInputKey(ir.Event.KeyEvent);

		} else if (ir.EventType == BRACKETED_PASTE_EVENT) {
			_processor->OnBracketedPaste(ir.Event.BracketedPaste.bStartPaste != FALSE);

		} else if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
			_processor->OnInputResized(ir);
		}
		{
			std::lock_guard<std::mutex> locker(_pending_injected_inputs_mutex);
			pending_injected_inputs.swap(_pending_injected_inputs);
		}
		for(const auto &pri : pending_injected_inputs) {
			_processor->OnInputInjected(pri);
		}

		pending_injected_inputs.clear();

		if (CloseFAR) {
			_processor->OnRequestShutdown();
			break;
		}
	}

	return nullptr;
}
