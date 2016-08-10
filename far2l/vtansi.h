#pragma once

class VTAnsi 
{
	public:
	VTAnsi();
	~VTAnsi();
	size_t Write(const WCHAR *str, size_t len);
	size_t Read(WCHAR *str, size_t len);
};

