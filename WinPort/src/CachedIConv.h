#pragma once
#include <iconv.h>
#include <string>
#include <map>
#include <stdint.h>

typedef std::map<uint64_t, iconv_t> IConvCache;

class CachedIConv
{
	IConvCache *_icc;
	iconv_t _cd;

	CachedIConv(const CachedIConv&) = delete;

public:
	CachedIConv(bool wide2mb, uint32_t page);
	~CachedIConv();

	inline operator iconv_t()
	{
		return _cd;
	}
};

void CachedIConv_Initialize();
