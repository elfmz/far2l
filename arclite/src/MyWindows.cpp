// MyWindows.cpp

#include "headers.hpp"

#ifndef _WIN32

// #include <stdlib.h>
// #include <time.h>
// #ifdef __GNUC__
// #include <sys/time.h>
// #endif

// #include "MyWindows.h"

static inline void *AllocateForBSTR(size_t cb)
{
	return ::malloc(cb);
}
static inline void FreeForBSTR(void *pv)
{
	::free(pv);
}

/* Win32 uses DWORD (32-bit) type to store size of string before (OLECHAR *) string.
  We must select CBstrSizeType for another systems (not Win32):

	if (CBstrSizeType is UINT32),
		  then we support only strings smaller than 4 GB.
		  Win32 version always has that limitation.

	if (CBstrSizeType is UINT),
		  (UINT can be 16/32/64-bit)
		  We can support strings larger than 4 GB (if UINT is 64-bit),
		  but sizeof(UINT) can be different in parts compiled by
		  different compilers/settings,
		  and we can't send such BSTR strings between such parts.
*/

typedef UINT32 CBstrSizeType;
// typedef UINT CBstrSizeType;

#define k_BstrSize_Max 0xFFFFFFFF
// #define k_BstrSize_Max UINT_MAX
// #define k_BstrSize_Max ((UINT)(INT)-1)

BSTR SysAllocStringByteLen(LPCSTR s, UINT len)
{
	/* Original SysAllocStringByteLen in Win32 maybe fills only unaligned null OLECHAR at the end.
	   We provide also aligned null OLECHAR at the end. */

	if (len >= (k_BstrSize_Max - (UINT)sizeof(OLECHAR) - (UINT)sizeof(OLECHAR) - (UINT)sizeof(CBstrSizeType)))
		return NULL;

	UINT size = (len + (UINT)sizeof(OLECHAR) + (UINT)sizeof(OLECHAR) - 1) & ~((UINT)sizeof(OLECHAR) - 1);
	void *p = AllocateForBSTR(size + (UINT)sizeof(CBstrSizeType));
	if (!p)
		return NULL;
	*(CBstrSizeType *)p = (CBstrSizeType)len;
	BSTR bstr = (BSTR)((CBstrSizeType *)p + 1);
	if (s)
		memcpy(bstr, s, len);
	for (; len < size; len++)
		((Byte *)bstr)[len] = 0;
	return bstr;
}

BSTR SysAllocStringLen(const OLECHAR *s, UINT len)
{
	if (len >= (k_BstrSize_Max - (UINT)sizeof(OLECHAR) - (UINT)sizeof(CBstrSizeType)) / (UINT)sizeof(OLECHAR))
		return NULL;

	UINT size = len * (UINT)sizeof(OLECHAR);
	void *p = AllocateForBSTR(size + (UINT)sizeof(CBstrSizeType) + (UINT)sizeof(OLECHAR));
	if (!p)
		return NULL;
	*(CBstrSizeType *)p = (CBstrSizeType)size;
	BSTR bstr = (BSTR)((CBstrSizeType *)p + 1);
	if (s)
		memcpy(bstr, s, size);
	bstr[len] = 0;
	return bstr;
}

INT SysReAllocStringLen(BSTR *pbstr, const OLECHAR *s, UINT len)
{
	if (!pbstr)
		return FALSE;

	SysFreeString(*pbstr);

	if (len >= (k_BstrSize_Max - (UINT)sizeof(OLECHAR) - (UINT)sizeof(CBstrSizeType)) / (UINT)sizeof(OLECHAR))
		return FALSE;

	UINT size = len * (UINT)sizeof(OLECHAR);
	void *p = AllocateForBSTR(size + (UINT)sizeof(CBstrSizeType) + (UINT)sizeof(OLECHAR));
	if (!p)
		return FALSE;
	*(CBstrSizeType *)p = (CBstrSizeType)size;
	BSTR bstr = (BSTR)((CBstrSizeType *)p + 1);
	if (s)
		memcpy(bstr, s, size);
	bstr[len] = 0;

	*pbstr = bstr;

	return TRUE;
}

INT SysReAllocString(BSTR *pbstr, const OLECHAR *s)
{
	if (!s)
		return FALSE;

	const OLECHAR *s2 = s;
	while (*s2 != 0)
		s2++;
	return SysReAllocStringLen(pbstr, s, (UINT)(s2 - s));
}

BSTR SysAllocString(const OLECHAR *s)
{
	if (!s)
		return NULL;
	const OLECHAR *s2 = s;
	while (*s2 != 0)
		s2++;
	return SysAllocStringLen(s, (UINT)(s2 - s));
}

void SysFreeString(BSTR bstr)
{
	if (bstr)
		FreeForBSTR((CBstrSizeType *)(void *)bstr - 1);
}

UINT SysStringByteLen(BSTR bstr)
{
	if (!bstr)
		return 0;
	return *((CBstrSizeType *)(void *)bstr - 1);
}

UINT SysStringLen(BSTR bstr)
{
	if (!bstr)
		return 0;
	return *((CBstrSizeType *)(void *)bstr - 1) / (UINT)sizeof(OLECHAR);
}

HRESULT VariantClear(VARIANTARG *prop)
{
	if (prop->vt == VT_BSTR)
		SysFreeString(prop->bstrVal);
	prop->vt = VT_EMPTY;
	return S_OK;
}

HRESULT VariantCopy(VARIANTARG *dest, const VARIANTARG *src)
{
	HRESULT res = ::VariantClear(dest);
	if (res != S_OK)
		return res;
	if (src->vt == VT_BSTR) {
		dest->bstrVal = SysAllocStringByteLen((LPCSTR)src->bstrVal, SysStringByteLen(src->bstrVal));
		if (!dest->bstrVal)
			return E_OUTOFMEMORY;
		dest->vt = VT_BSTR;
	} else
		*dest = *src;
	return S_OK;
}

#endif
