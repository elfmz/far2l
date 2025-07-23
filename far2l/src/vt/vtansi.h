#pragma once

#define MAX_ARG 32					// max number of args in an escape sequence
#define BUFFER_SIZE 2048

#include <string>
#include <memory>
#include <map>
#include "IVTShell.h"

struct VTAnsiContext;

class VTAnsi
{
	struct {
		std::string tail, tmp;
	} _incomplete;

	std::wstring _ws, _saved_title;
	std::unique_ptr<VTAnsiContext> _ctx;
	struct DetachedState
	{
		SHORT scrl_top{0}, scrl_bottom{MAXSHORT};
		std::map<DWORD, std::pair<DWORD, DWORD> > palette;
	} _detached_state;

	int   es_argc;				// escape sequence args count
	int   es_argv[MAX_ARG]; 		// escape sequence args
	std::string os_cmd_arg;		// text parameter for Operating System Command
	int   screen_top = -1;		// initial window top when cleared
	TCHAR blank_character = L' ';

	int   chars_in_buffer;
	WCHAR char_buffer[BUFFER_SIZE];

	void RevertConsoleState(HANDLE con_hnd);

	public:
	VTAnsi(IVTShell *vtsh);
	~VTAnsi();

	void DisableOutput();
	void EnableOutput();
	
	void Write(const char *str, size_t len);
	
	struct VTAnsiState *Suspend();
	void Resume(struct VTAnsiState* state);

	void OnStart();
	void OnStop();
	void OnDetached();
	void OnReattached();
	std::string GetTitle();
};

class VTAnsiSuspend
{
	VTAnsi &_vta;
	struct VTAnsiState *_ansi_state;

	public:
	VTAnsiSuspend(VTAnsi &vta) 
		: _vta(vta), _ansi_state(_vta.Suspend())
	{
	}

	~VTAnsiSuspend()
	{
		if (_ansi_state)
			_vta.Resume(_ansi_state);
	}
	
	inline operator bool() const
	{
		return _ansi_state != nullptr;
	}
};
