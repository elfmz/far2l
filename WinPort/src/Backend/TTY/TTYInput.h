#pragma once
#include <stdexcept>
#include <vector>
#include <WinCompat.h>

class TTYInput
{
	std::vector<char> _buf;

	void PostSimpleKeyEvent(wchar_t ch);

	bool BufParseIterationSimple();
	bool BufParseIterationCSI();

	void OnBufUpdated();

public:
	void OnChar(char c);
};
