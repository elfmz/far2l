#pragma once
#include <string>
#include <memory>
#include <map>
#include "IVTShell.h"

#define FOREGROUND_RGB (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define BACKGROUND_RGB (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)

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
	void AddToLogicalLines(const std::wstring& str);
	void DumpLogicalLines(std::string &s, bool colored) const;
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
