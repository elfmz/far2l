#pragma once
#include <stdexcept>
#include <vector>
#include <map>
#include <string>
#include <WinCompat.h>
#include "TTYInputSequenceParser.h"

class TTYInput
{
	std::vector<char> _buf;
	TTYInputSequenceParser _parser;

	void PostCharEvent(wchar_t ch);

	size_t BufTryDecodeUTF8();

	void OnBufUpdated();

public:
	TTYInput();
	void OnChar(char c);
};
