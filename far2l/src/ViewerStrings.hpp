#pragma once
#include <stdint.h>
#include <wchar.h>
#include <vector>

struct ViewerString
{
	int64_t nFilePos{0};
	int64_t nSelStart{0};
	int64_t nSelEnd{0};
	bool bSelection{false};
	bool WrapsToNext{false};
	bool ContinuesFromPrev{false};

	ViewerString() = default;
	ViewerString(const ViewerString &src) = default;
	ViewerString &operator=(const ViewerString &src) = default;

	ViewerString(ViewerString &&src);
	ViewerString &operator=(ViewerString &&src);

	bool IsEmpty() const;
	size_t Capacity() const;

	const wchar_t *Chars() const;
	const wchar_t *Chars();
	const wchar_t *Chars(size_t x) const;
	const wchar_t *Chars(size_t x);

	void SetChar(size_t x, wchar_t c);
	void SetChars(size_t x, const wchar_t *cs, size_t cnt);

private:
	std::vector<wchar_t> _data;
};

class ViewerStrings
{
	std::vector<ViewerString> _lines;

public:
	~ViewerStrings();

	ViewerString &operator[](int y);
	void ScrollUp();
	void ScrollDown();
};
