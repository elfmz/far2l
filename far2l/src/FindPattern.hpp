#pragma once
#include <vector>
#include <memory>
#include <stdint.h>


typedef std::unique_ptr<struct IScannedPattern> ScannedPatternPtr;

class FindPattern
{
	bool _case_sensitive{false};
	bool _whole_words{false};

	size_t _min_pattern_size{0};
	size_t _look_behind{0};

	std::vector<ScannedPatternPtr> _patterns;
	void AddPattern(ScannedPatternPtr &&new_p);

public:
	FindPattern(bool case_sensitive, bool whole_words);
	~FindPattern();

	void AddBytesPattern(const uint8_t *pattern, size_t len);
	void AddTextPattern(const wchar_t *pattern, unsigned int codepage);

	/**
		Call this once after all patterns added but before using any other method below.
	*/
	void GetReady();

	/**
		Minimal size (in bytes) of content that can match any of added pattern.
	*/
	inline size_t MinPatternSize() const noexcept { return _min_pattern_size; }

	/**
		Size (in bytes) of substring that should be prepended from tail of previous scan window
		to ensure matching substring that happen to be crossed by scan windows boundary.
		Returned value guaranteed to be aligned by size of largest searched codeunit.
	*/
	inline size_t LookBehind() const noexcept { return _look_behind; }

	/**
		Searches given data array for substring matching any of added pattern.
		Data array pointer expected to be aligned by size of largest searched codeunit.
		Following arguments needed by whole_words to correctly treat edging scan windows:
			first_fragment indicates if this is very first scan window of actually checked content.
			last_fragment indicates if this is very last scan window of actually checked content.
		Returns {start, len} of matching region or {-1, 0} if no match found.
	*/
	std::pair<size_t, size_t> FindMatch(const void *data, size_t len, bool first_fragment, bool last_fragment) const noexcept;
};
