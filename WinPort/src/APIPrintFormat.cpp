#include <mutex>
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"

SHAREDSYMBOL int vswprintf_ws2ls(wchar_t * ws, size_t len, const wchar_t * format, va_list arg )
{
	std::wstring converted_format;
	for (bool percent = false; *format; ++format) {
		if (percent) {
			if (*format==L's') {
				if (!converted_format.empty()) {
					if (converted_format.back() == L'w')
						converted_format.back() = L'l';
					else if (converted_format.back() == L'h')
						converted_format.pop_back();
					else
						converted_format+= L'l';
				} else
					converted_format+= L'l';
			}
			if (wcschr(L"diouxXeEfFgGcCsSpnm%", *format)) {
				percent = false;
			}
		} else if (*format==L'%') {
			percent = true;
		}
		converted_format+= *format;
	}

	return vswprintf(ws, len, converted_format.c_str(), arg );
}


SHAREDSYMBOL int swprintf_ws2ls (wchar_t* ws, size_t len, const wchar_t* format, ...)
{
	va_list arg;
	va_start(arg,format);
	int r = vswprintf_ws2ls(ws, len, format, arg );
	va_end(arg);
	return r;
}
