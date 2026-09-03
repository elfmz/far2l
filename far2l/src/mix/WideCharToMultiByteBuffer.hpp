#pragma once
#include <vector>

struct WideCharToMultiByteBuffer : std::vector<char>
{
	WideCharToMultiByteBuffer() = default;
	WideCharToMultiByteBuffer(UINT codepage, const wchar_t *pws, size_t len)
	{
		assign(codepage, pws, len);
	}

	void assign(UINT cp, const wchar_t *pws, size_t len);
};
