#include "headers.hpp"
#include "WideCharToMultiByteBuffer.hpp"

void WideCharToMultiByteBuffer::assign(UINT cp, const wchar_t *pws, size_t len)
{
	const auto n = WINPORT(WideCharToMultiByte)(cp, 0, pws, len, nullptr, 0, nullptr, nullptr);
	if (n > 0) {
		resize(n);
		const auto n2 = WINPORT(WideCharToMultiByte)(cp, 0, pws, len, data(), size(), nullptr, nullptr);
		if (n2 != n) {
			clear();
		}
	} else {
		clear();
	}
}
