#pragma once
#include <string>

struct IVTShell
{
	virtual void OnApplicationProtocolCommand(const char *str) = 0;
	virtual void InjectInput(const char *str) = 0;
	virtual void OnKeypadChange(unsigned char keypad) = 0;
	virtual void OnTerminalResized() = 0;
};

class VTAnsi
{
	struct {
		std::string tail, tmp;
	} _incomplete;

	std::wstring _ws, _saved_title;
	public:
	VTAnsi(IVTShell *vt_shell);
	~VTAnsi();

	void DisableOutput();
	void EnableOutput();
	
	void Write(const char *str, size_t len);
	
	struct VTAnsiState *Suspend();
	void Resume(struct VTAnsiState* state);

	void OnStart();
	void OnStop();
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
