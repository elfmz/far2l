#pragma once
#include <string>

struct IVTAnsiCommands
{
	virtual int OnApplicationProtocolCommand(const char *str) = 0;
	virtual void WriteRawInput(const char *str) = 0;
};

class VTAnsi
{
	std::wstring _ws;
	public:
	VTAnsi(IVTAnsiCommands *ansi_commands);
	~VTAnsi();
	
	size_t Write(const char *str, size_t len);
	
	struct VTAnsiState *Suspend();
	void Resume(struct VTAnsiState* state);
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
