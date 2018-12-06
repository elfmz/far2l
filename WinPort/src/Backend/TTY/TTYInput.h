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
	ITTYInputSpecialSequenceHandler *_handler;

	void PostCharEvent(wchar_t ch);

	size_t BufTryDecodeUTF8();

	void OnBufUpdated();

public:
	TTYInput(ITTYInputSpecialSequenceHandler *handler);
	void OnInput(const char *data, size_t len);
};
