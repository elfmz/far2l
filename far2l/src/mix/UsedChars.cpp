#include "headers.hpp"
#include "keyboard.hpp"
#include "UsedChars.hpp"

UsedChars::UsedChars()
	: _base(MAX_VKEY_CODE)
{
}

UsedChars::~UsedChars()
{
}

void UsedChars::UnsetOtherCase(wchar_t c)
{
	wchar_t other = Upper(c);
	if (other == c) {
		other = Lower(c);
	}
	if (other != c) {
		if (LIKELY(size_t(other) < _base.size())) {
			_base[size_t(other)] = false;
		} else {
			_extended.erase(other);
		}
	}
}
void UsedChars::Unset(wchar_t c)
{
	if (LIKELY(size_t(c) < _base.size())) {
		_base[size_t(c)] = false;
	} else {
		_extended.erase(c);
	}
	UnsetOtherCase(c);
	wchar_t xlated = KeyToKeyLayout(c);
	if (xlated && xlated != c) {
		UnsetOtherCase(xlated);
	}
}


void UsedChars::SetOtherCase(wchar_t c)
{
	wchar_t other = Upper(c);
	if (other == c) {
		other = Lower(c);
	}
	if (other != c) {
		if (LIKELY(size_t(other) < _base.size())) {
			_base[size_t(other)] = true;
		} else {
			_extended.emplace(other);
		}
	}
}

bool UsedChars::Set(wchar_t c)
{
	if (LIKELY(size_t(c) < _base.size())) {
		if (_base[size_t(c)]) {
			return false;
		}
		_base[size_t(c)] = true;
	} else if (!_extended.insert(c).second) {
		return false;
	}
	SetOtherCase(c);
	wchar_t xlated = KeyToKeyLayout(c);
	if (xlated && xlated != c) {
		SetOtherCase(xlated);
	}
	return true;
}

bool UsedChars::IsSet(wchar_t c) const
{
	if (LIKELY(size_t(c) < _base.size())) {
		return _base[size_t(c)];
	}
	return _extended.find(c) != _extended.end();
}
