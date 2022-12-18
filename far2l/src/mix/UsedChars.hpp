#pragma once
#include <vector>
#include <set>

class UsedChars
{
	std::vector<bool> _base;
	std::set<wchar_t> _extended;
	void SetOtherCase(wchar_t c);
	void UnsetOtherCase(wchar_t c);

public:
	UsedChars();
	~UsedChars();

	void Unset(wchar_t c);
	bool Set(wchar_t c);
	bool IsSet(wchar_t c) const;
};
