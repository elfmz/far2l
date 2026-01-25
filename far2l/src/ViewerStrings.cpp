#include <stdio.h>
#include <utils.h>
#include "ViewerStrings.hpp"

ViewerString::ViewerString(ViewerString &&src)
{
	nFilePos = src.nFilePos;
	nSelStart = src.nSelStart;
	nSelEnd = src.nSelEnd;
	bSelection = src.bSelection;
	WrapsToNext = src.WrapsToNext;
	ContinuesFromPrev = src.ContinuesFromPrev;
	_data.swap(src._data);
}

ViewerString &ViewerString::operator=(ViewerString &&src)
{
	nFilePos = src.nFilePos;
	nSelStart = src.nSelStart;
	nSelEnd = src.nSelEnd;
	bSelection = src.bSelection;
	WrapsToNext = src.WrapsToNext;
	ContinuesFromPrev = src.ContinuesFromPrev;
	_data.swap(src._data);
	return *this;
}

bool ViewerString::IsEmpty() const
{
	return _data.empty() || !_data.front();
}

size_t ViewerString::Capacity() const
{
	return _data.capacity();
}

//////

const wchar_t *ViewerString::Chars(size_t x) const
{
	return LIKELY(x < _data.size()) ? Chars() + x : L"";
}

const wchar_t *ViewerString::Chars(size_t x)
{
	return LIKELY(x < _data.size()) ? Chars() + x : L"";
}

const wchar_t *ViewerString::Chars() const
{
	if (_data.empty())
		return L"";
	return _data.data();
}

const wchar_t *ViewerString::Chars()
{
	if (UNLIKELY(_data.empty() || _data.back() != 0)) {
		_data.emplace_back(0);
	}

	return _data.data();
}

void ViewerString::SetChar(size_t x, wchar_t c)
{
	if (x >= _data.size()) {
		_data.resize(x + 1);
	}
	_data[x] = c;
}

void ViewerString::SetChars(size_t x, const wchar_t *cs, size_t cnt)
{
	if (x + cnt > _data.size()) {
		_data.resize(x + cnt);
	}
	wmemcpy(_data.data() + x, cs, cnt);
}

///////////////////////////////////////////////////////////////

ViewerStrings::~ViewerStrings()
{
	size_t bytes = 0;
	for (const auto &line : _lines) {
		bytes+= line.Capacity() * sizeof(wchar_t);
	}
	bytes+= _lines.capacity() * sizeof(ViewerString);
	fprintf(stderr, "~ViewerStrings: %lu lines, %lu bytes\n", _lines.size(), bytes);
}

ViewerString &ViewerStrings::operator[](int y)
{
	if (UNLIKELY(y < 0)) {
		fprintf(stderr, "ViewerStrings: negative index %d ???\n", y);
		y = 0;
	}
	if ((size_t)y >= _lines.size()) {
		_lines.resize(y + 1);
	}
	return _lines[y];
}

void ViewerStrings::ScrollUp()
{
	if (!_lines.empty()) {
		for (size_t i = _lines.size() - 1; i > 0; --i) {
			_lines[i] = std::move(_lines[i - 1]);
		}
	}
}

void ViewerStrings::ScrollDown()
{
	for (size_t i = 0; i + 1 < _lines.size(); ++i) {
		_lines[i] = std::move(_lines[i + 1]);
	}
}
