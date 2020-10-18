#include "WinCompat.h"
#include "WinPort.h"
#include "CachedIConv.h"
#include <utils.h>
#include <ThreadSpecific.h>
#include <stdexcept>

#if (__WCHAR_MAX__ > 0xffff)
# define CP_ICONV_WCHAR "UTF-32LE" // //IGNORE
#else
# define CP_ICONV_WCHAR "UTF-16LE" // //IGNORE
#endif

static const char *iconv_codepage_name(uint32_t page, char *tmpbuf)
{
	switch (page) {
       		case CP_KOI8R: return "CP_KOI8R-7";
       		case CP_UTF8:  return "UTF-8";
       		case CP_UTF7:  return "UTF-7";
		default: {
			sprintf(tmpbuf, "CP%d", page);
			return tmpbuf;
		}
	}
}



static void CachedIConv_ThreadCleanup(void *p)
{
	if (!p)
		return;

	IConvCache *p_icc = (IConvCache *)p;
	for (auto &it : *p_icc) {
		iconv_close(it.second);
	}
	delete p_icc;
}

static ThreadSpecific s_icc_ts(&CachedIConv_ThreadCleanup);

CachedIConv::CachedIConv(bool wide2mb, uint32_t page)
{
	uint64_t key = page;
	if (wide2mb) {
		key|= 0x8000000000000000;
	}

	_icc = (IConvCache *)s_icc_ts.Get();
	if (!_icc) {
		_icc = new IConvCache;
		try {
			s_icc_ts.Set(_icc);

		} catch (std::exception &) {
			delete _icc;
			throw;
		}

	} else {
		auto it = _icc->find(key);
		if (it != _icc->end()) {
			_cd = it->second;
			return;
		}
	}

	char tmpbuf[32];
	if (wide2mb) {
		_cd = iconv_open(iconv_codepage_name(page, tmpbuf), CP_ICONV_WCHAR);
	} else {
		_cd = iconv_open(CP_ICONV_WCHAR, iconv_codepage_name(page, tmpbuf));
	}

	if (_cd == (iconv_t)-1) {
		throw std::runtime_error(
			StrPrintf("CachedIConv(%d, %u) - error %d", wide2mb, page, errno));
	}

	_icc->emplace(key, _cd);
}

CachedIConv::~CachedIConv()
{
	// its already in cache, dont care
}
