#pragma once
#include <WinCompat.h>
#include <WinPort.h>
#include <map>
#include <StackSerializer.h>

struct TTYInputKey
{
	WORD vk;
	DWORD control_keys;
};

template <size_t N>
	struct NChars
{
	char chars[N];

	NChars() = default;
	NChars(const NChars&) = default;

	NChars(const char *src)
	{
		for (size_t i = 0; i < N; ++i) {
			chars[i] = src[i];
		}
	}

	bool operator <(const NChars<N> &other) const
	{
		for (size_t i = 0;; ++i) {
			if (i == N || chars[i] > other.chars[i])
				return false;
			if (chars[i] < other.chars[i])
				return true;
		}
	}
};

template <size_t N, class V>
	struct NCharsMap : std::map<NChars<N>, V>
{
	void Add(const char *s, const V &v)
	{
		NChars<N> nc(s);
		auto ir = std::map<NChars<N>, V>::emplace(nc, v);
		ASSERT_MSG(ir.second, "can't add '%s'", s);
	}

	bool Lookup(const char *s, V &out) const
	{
		NChars<N> nc(s);
		auto it = std::map<NChars<N>, V>::find(nc);
		if (it == std::map<NChars<N>, V>::end())
			return false;

		out = it->second;
		return true;
	}
};

template <size_t N> using NChars2Key = NCharsMap<N, TTYInputKey>;

struct ITTYInputSpecialSequenceHandler
{
	virtual void OnInspectKeyEvent(KEY_EVENT_RECORD &event) = 0;
	virtual void OnFar2lEvent(StackSerializer &stk_ser) = 0;
	virtual void OnFar2lReply(StackSerializer &stk_ser) = 0;
	virtual void OnInputBroken() = 0;
};

#define TTY_PARSED_WANTMORE         ((size_t)0)
#define TTY_PARSED_PLAINCHARS       ((size_t)-1)
#define TTY_PARSED_BADSEQUENCE      ((size_t)-2)

class TTYInputSequenceParser
{
	NChars2Key<1> _nch2key1;
	NChars2Key<2> _nch2key2;
	NChars2Key<3> _nch2key3;
	NChars2Key<4> _nch2key4;
	NChars2Key<5> _nch2key5;
	NChars2Key<6> _nch2key6;
	NChars2Key<7> _nch2key7;
	NChars2Key<8> _nch2key8;


	struct {
		bool left = false;
		bool middle = false;
		bool right = false;
		DWORD left_ts = 0;
		DWORD middle_ts = 0;
		DWORD right_ts = 0;
	} _mouse;

	ITTYInputSpecialSequenceHandler *_handler;
	StackSerializer _tmp_stk_ser;
	DWORD _extra_control_keys = 0;
	std::vector<INPUT_RECORD> _ir_pending;

	void AssertNoConflicts();

	void AddStr(WORD vk, DWORD control_keys, const char *fmt, ...);
	void AddStr_ControlsThenCode(WORD vk, const char *fmt, const char *code);
	void AddStr_CodeThenControls(WORD vk, const char *fmt, const char *code);
	void AddStrTilde(WORD vk, int code);
	void AddStrTilde_Controls(WORD vk, int code);
	void AddStrF1F5(WORD vk, const char *code);
	void AddStrCursors(WORD vk, const char *code);

	size_t ParseNChars2Key(const char *s, size_t l);
	void ParseMouse(char action, char col, char raw);
	void ParseAPC(const char *s, size_t l);
	size_t ParseEscapeSequence(const char *s, size_t l);
	void OnBracketedPaste(bool start);

	void AddPendingKeyEvent(const TTYInputKey &k);
	size_t ParseIntoPending(const char *s, size_t l);


public:
	TTYInputSequenceParser(ITTYInputSpecialSequenceHandler *handler);

	size_t Parse(const char *s, size_t l, bool idle_expired); // 0 - need more, -1 - not sequence, -2 - unrecognized sequence, >0 - sequence
};
