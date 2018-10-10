#pragma once
#include <stdexcept>
#include <vector>
#include <map>
#include <string>
#include <WinCompat.h>

class TTYInput
{
	struct Key
	{
		WORD key_code;
		DWORD control_keys;
	};

	std::vector<char> _buf;
	std::map<std::string, Key>	_csi2key;
	std::map<char, Key>		_spec_char2key;

	void PostCharEvent(wchar_t ch);
	void PostKeyEvent(const Key &k);

	bool BufParseIterationSimple();
	bool BufParseIterationCSI();

	void OnBufUpdated();

public:
	TTYInput();
	void OnChar(char c);
};
