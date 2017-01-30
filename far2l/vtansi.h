#pragma once

struct IVTAnsiCommands
{
	virtual int OnApplicationProtocolCommand(const char *str) = 0;
	virtual void WriteRawInput(const char *str) = 0;
};

class VTAnsi
{
	public:
	VTAnsi(IVTAnsiCommands *ansi_commands);
	~VTAnsi();
	
	size_t Write(const WCHAR *str, size_t len);
	
	struct VTAnsiState *Pause();
	void Continue(struct VTAnsiState* state);
};

