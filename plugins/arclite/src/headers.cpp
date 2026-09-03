#include "headers.hpp"

#define ___DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)                                      \
	GUID name = {                                                                                            \
			l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8} \
	   }

___DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
