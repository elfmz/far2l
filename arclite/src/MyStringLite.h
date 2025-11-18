#pragma once

static const unsigned kStartStringCapacity = 4;

#define MY_STRING_NEW(_T_, _size_) new _T_[_size_]
#define k_Alloc_Len_Limit (0x40000000 - 2)

#define MY_STRING_NEW_char(_size_) MY_STRING_NEW(char, (_size_))
#define MY_STRING_NEW_wchar_t(_size_) MY_STRING_NEW(wchar_t, (_size_))
#define MY_STRING_DELETE(_p_) { delete [](_p_); }

class UString
{
  wchar_t *_chars;
  unsigned _len;
  unsigned _limit;

public:
  UString();
  unsigned Len() const { return _len; }
//  unsigned Limit() const { return _limit; }
  bool IsEmpty() const { return _len == 0; }
  void Empty() { _len = 0; _chars[0] = 0; }
  void SetLen(unsigned len) { _len = len; _chars[len] = 0; }
  ~UString() { MY_STRING_DELETE(_chars) }

  operator const wchar_t *() const { return _chars; }
  wchar_t *Ptr_non_const() const { return _chars; }
  const wchar_t *Ptr() const { return _chars; }
  const wchar_t *Ptr(int pos) const { return _chars + (unsigned)pos; }
  const wchar_t *Ptr(unsigned pos) const { return _chars + pos; }
  const wchar_t *RightPtr(unsigned num) const { return _chars + _len - num; }
  wchar_t Back() const { return _chars[(size_t)_len - 1]; }

  void ReplaceOneCharAtPos(unsigned pos, wchar_t c) { _chars[pos] = c; }
  void SetNewSize(unsigned size) {
	MY_STRING_DELETE(_chars);
	_chars = NULL;
	_chars = MY_STRING_NEW_wchar_t(size);
	_len = 0;
	_limit = size - 1;
	_chars[0] = 0;
  }

  wchar_t *GetBuf() { return _chars; }
};

UString::UString()
{
  _chars = NULL;
  _chars = MY_STRING_NEW_wchar_t(kStartStringCapacity);
  _len = 0;
  _limit = kStartStringCapacity - 1;
  _chars[0] = 0;
}

class AString
{
	char *_chars;
	unsigned _len;
	unsigned _limit;

	void ReAlloc2(unsigned newLimit);
	void SetStartLen(unsigned len);

public:
	explicit AString();
	explicit AString(size_t ilimit);
	explicit AString(char c);
	explicit AString(const char *s);
	AString(const AString &s);

	~AString() { MY_STRING_DELETE(_chars) }

	unsigned Len() const { return _len; }
	bool IsEmpty() const { return _len == 0; }
	void Empty() { _len = 0; _chars[0] = 0; }
	void SetLen(unsigned len) { _len = len; _chars[len] = 0; }

	operator const char *() const { return _chars; }
	char *Ptr_non_const() const { return _chars; }
	const char *Ptr() const { return _chars; }
	const char *Ptr(unsigned pos) const { return _chars + pos; }
	const char *Ptr(int pos) const { return _chars + (unsigned)pos; }
	const char *RightPtr(unsigned num) const { return _chars + _len - num; }
	char Back() const { return _chars[(size_t)_len - 1]; }
};

AString::AString()
{
	_chars = NULL;
	_chars = MY_STRING_NEW_char(kStartStringCapacity);
	_len = 0;
	_limit = kStartStringCapacity - 1;
	_chars[0] = 0;
}

AString::AString(size_t ilimit)
{
	_chars = NULL;
	_chars = MY_STRING_NEW_char(ilimit);
	_len = 0;
	_limit = ilimit - 1;
	_chars[0] = 0;
}

AString::AString(char c)
{
  SetStartLen(1);
  char *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
}

inline unsigned MyStringLen(const char *s)
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

void AString::SetStartLen(unsigned len)
{
  _chars = NULL;
  _chars = MY_STRING_NEW_char((size_t)len + 1);
  _len = len;
  _limit = len;
}

inline void MyStringCopy(char *dest, const char *src)
{
  while ((*dest++ = *src++) != 0);
}

AString::AString(const char *s)
{
  SetStartLen(MyStringLen(s));
  MyStringCopy(_chars, s);
}

AString::AString(const AString &s)
{
  SetStartLen(s._len);
  MyStringCopy(_chars, s._chars);
}
