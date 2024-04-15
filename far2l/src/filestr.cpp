/*
filestr.cpp

Класс GetFileString
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "filestr.hpp"
#include "DetectCodepage.h"
#include "codepage.hpp"

#define DELTA 1024

enum EolType
{
	FEOL_NONE,
	// \r\n
	FEOL_WINDOWS,
	// \n
	FEOL_UNIX,
	// \r
	FEOL_MAC,
	// \r\r (это не реальное завершение строки, а состояние парсера)
	FEOL_MAC2,
	// \r\r\n (появление таких завершений строк вызвано багом Notepad-а)
	FEOL_NOTEPAD
};

OldGetFileString::OldGetFileString(FILE *SrcFile)
	:
	SrcFile(SrcFile),
	ReadPos(0),
	ReadSize(0),
	m_nStrLength(DELTA),
	Str(reinterpret_cast<char *>(malloc(m_nStrLength))),
	m_nwStrLength(DELTA),
	wStr(reinterpret_cast<wchar_t *>(malloc(m_nwStrLength * sizeof(wchar_t)))),
	SomeDataLost(false),
	bCrCr(false)
{}

OldGetFileString::~OldGetFileString()
{
	if (Str)
		free(Str);

	if (wStr)
		free(wStr);
}

int OldGetFileString::GetString(wchar_t **DestStr, int nCodePage, int &Length)
{
	int nExitCode;

	if (nCodePage == CP_WIDE_LE)
		nExitCode = GetUnicodeString(DestStr, Length, false);
	else if (nCodePage == CP_WIDE_BE)
		nExitCode = GetUnicodeString(DestStr, Length, true);
	else {
		char *Str;
		nExitCode = GetAnsiString(&Str, Length);

		if (nExitCode == 1) {
			DWORD ret = ERROR_SUCCESS;
			int nResultLength = 0;
			bool bGet = false;
			*wStr = L'\0';

			if (!SomeDataLost) {
				// при CP_UTF7 dwFlags должен быть 0, см. MSDN
				nResultLength = WINPORT(MultiByteToWideChar)(nCodePage,
						(SomeDataLost || nCodePage == CP_UTF7) ? 0 : MB_ERR_INVALID_CHARS, Str, Length, wStr,
						m_nwStrLength - 1);

				ret = WINPORT(GetLastError)();

				if (ERROR_NO_UNICODE_TRANSLATION == ret) {
					SomeDataLost = true;
					if (!nResultLength) {
						bGet = true;
					}
				}
			} else
				bGet = true;

			if (bGet) {
				nResultLength =
						WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, wStr, m_nwStrLength - 1);

				if (!nResultLength)
					ret = WINPORT(GetLastError)();
			}

			if (ERROR_INSUFFICIENT_BUFFER == ret) {
				nResultLength = WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, nullptr, 0);
				free(wStr);
				wStr = (wchar_t *)malloc((nResultLength + 1) * sizeof(wchar_t));
				*wStr = L'\0';
				m_nwStrLength = nResultLength + 1;
				nResultLength = WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, wStr, nResultLength);
			}

			if (nResultLength)
				wStr[nResultLength] = L'\0';

			Length = nResultLength;
			*DestStr = wStr;
		}
	}

	return nExitCode;
}

int OldGetFileString::GetAnsiString(char **DestStr, int &Length)
{
	int CurLength = 0;
	int ExitCode = 1;
	EolType Eol = FEOL_NONE;
	int x = 0;
	char *ReadBufPtr = ReadPos < ReadSize ? ReadBuf + ReadPos : nullptr;

	// Обработка ситуации, когда у нас пришёл двойной \r\r, а потом не было \n.
	// В этом случаем считаем \r\r двумя MAC окончаниями строк.
	if (bCrCr) {
		*Str = '\r';
		CurLength = 1;
		bCrCr = false;
	} else {
		while (1) {
			if (ReadPos >= ReadSize) {
				if (!(ReadSize = (int)fread(ReadBuf, 1, sizeof(ReadBuf), SrcFile))) {
					if (!CurLength)
						ExitCode = 0;

					break;
				}

				ReadPos = 0;
				ReadBufPtr = ReadBuf;
			}

			if (Eol == FEOL_NONE) {
				// UNIX
				if (*ReadBufPtr == '\n')
					Eol = FEOL_UNIX;
				// MAC / Windows? / Notepad?
				else if (*ReadBufPtr == '\r')
					Eol = FEOL_MAC;
			} else if (Eol == FEOL_MAC) {
				// Windows
				if (*ReadBufPtr == '\n')
					Eol = FEOL_WINDOWS;
				// Notepad?
				else if (*ReadBufPtr == '\r')
					Eol = FEOL_MAC2;
				else
					break;
			} else if (Eol == FEOL_WINDOWS || Eol == FEOL_UNIX)
				break;
			else if (Eol == FEOL_MAC2) {
				// Notepad
				if (*ReadBufPtr == '\n')
					Eol = FEOL_NOTEPAD;
				else {
					// Пришёл \r\r, а \n не пришёл, поэтому считаем \r\r двумя MAC окончаниями строк
					--CurLength;
					bCrCr = true;
					break;
				}
			} else
				break;

			ReadPos++;

			if (CurLength >= m_nStrLength - 1) {
				char *NewStr = (char *)realloc(Str, m_nStrLength + (DELTA << x));

				if (!NewStr)
					return (-1);

				Str = NewStr;
				m_nStrLength+= DELTA << x;
				x++;
			}

			Str[CurLength++] = *ReadBufPtr;
			ReadBufPtr++;
		}
	}

	Str[CurLength] = 0;
	*DestStr = Str;
	Length = CurLength;
	return (ExitCode);
}

int OldGetFileString::GetUnicodeString(wchar_t **DestStr, int &Length, bool bBigEndian)
{
	int CurLength = 0;
	int ExitCode = 1;
	EolType Eol = FEOL_NONE;
	int x = 0;
	wchar_t *ReadBufPtr = ReadPos < ReadSize ? wReadBuf + ReadPos / sizeof(wchar_t) : nullptr;

	// Обработка ситуации, когда у нас пришёл двойной \r\r, а потом не было \n.
	// В этом случаем считаем \r\r двумя MAC окончаниями строк.
	if (bCrCr) {
		*wStr = L'\r';
		CurLength = 1;
		bCrCr = false;
	} else {
		while (1) {
			if (ReadPos >= ReadSize) {
				if (!(ReadSize = (int)fread(wReadBuf, 1, sizeof(wReadBuf), SrcFile))) {
					if (!CurLength)
						ExitCode = 0;

					break;
				}

				if (bBigEndian)
					RevBytes(wReadBuf, ReadSize / sizeof(wReadBuf[0]));

				ReadPos = 0;
				ReadBufPtr = wReadBuf;
			}

			if (Eol == FEOL_NONE) {
				// UNIX
				if (*ReadBufPtr == L'\n')
					Eol = FEOL_UNIX;
				// MAC / Windows? / Notepad?
				else if (*ReadBufPtr == L'\r')
					Eol = FEOL_MAC;
			} else if (Eol == FEOL_MAC) {
				// Windows
				if (*ReadBufPtr == L'\n')
					Eol = FEOL_WINDOWS;
				// Notepad?
				else if (*ReadBufPtr == L'\r')
					Eol = FEOL_MAC2;
				else
					break;
			} else if (Eol == FEOL_WINDOWS || Eol == FEOL_UNIX)
				break;
			else if (Eol == FEOL_MAC2) {
				// Notepad
				if (*ReadBufPtr == L'\n')
					Eol = FEOL_NOTEPAD;
				else {
					// Пришёл \r\r, а \n не пришёл, поэтому считаем \r\r двумя MAC окончаниями строк
					--CurLength;
					bCrCr = true;
					break;
				}
			} else
				break;

			ReadPos+= sizeof(wchar_t);

			if (CurLength >= m_nwStrLength - 1) {
				wchar_t *NewStr = (wchar_t *)realloc(wStr, (m_nwStrLength + (DELTA << x)) * sizeof(wchar_t));

				if (!NewStr)
					return (-1);

				wStr = NewStr;
				m_nwStrLength+= DELTA << x;
				x++;
			}

			wStr[CurLength++] = *ReadBufPtr;
			ReadBufPtr++;
		}
	}

	wStr[CurLength] = 0;
	*DestStr = wStr;
	Length = CurLength;
	return (ExitCode);
}

static bool IsTextUTF8(const LPBYTE Buffer, size_t Length)
{
	bool Ascii = true;
	UINT Octets = 0;

	for (size_t i = 0; i < Length; i++) {
		BYTE c = Buffer[i];

		if (c & 0x80)
			Ascii = false;

		if (Octets) {
			if ((c & 0xC0) != 0x80)
				return false;

			Octets--;
		} else {
			if (c & 0x80) {
				while (c & 0x80) {
					c<<= 1;
					Octets++;
				}

				Octets--;

				if (!Octets)
					return false;
			}
		}
	}

	return (Octets > 0 || Ascii) ? false : true;
}

bool DetectFileMagic(FILE *file, UINT &nCodePage)
{
	uint32_t dwHeading = 0;
	size_t n = fread(&dwHeading, 1, 4, file);

	if (n == 4 && dwHeading == SIGN_UTF32LE) {
		nCodePage = CP_UTF32LE;
		return true;
	}
	if (n == 4 && dwHeading == SIGN_UTF32BE) {
		nCodePage = CP_UTF32BE;
		return true;
	}
	if (n >= 2 && LOWORD(dwHeading) == SIGN_UTF16LE) {
		nCodePage = CP_UTF16LE;
		fseek(file, -(off_t)(n - 2), SEEK_CUR);
		return true;
	}
	if (n >= 2 && LOWORD(dwHeading) == SIGN_UTF16BE) {
		nCodePage = CP_UTF16BE;
		fseek(file, -(off_t)(n - 2), SEEK_CUR);
		return true;
	}
	if (n >= 3 && (dwHeading & 0x00FFFFFF) == SIGN_UTF8) {
		nCodePage = CP_UTF8;
		fseek(file, -(off_t)(n - 3), SEEK_CUR);
		return true;
	}

	if (n)
		fseek(file, -(off_t)n, SEEK_CUR);

	return false;
}

wchar_t *StringReader::Read(FILE *file, wchar_t *lpwszDest, size_t nDestLength, int nCodePage)
{
	if (!nDestLength)
		return nullptr;

	if (nCodePage == CP_WIDE_LE || nCodePage == CP_WIDE_BE) {
		if (nCodePage == CP_WIDE_BE) {
			size_t nLength;
			for (nLength = 0; nLength + 1 < nDestLength; ++nLength) {
				if (feof(file)) {
					if (!nLength)
						return nullptr;

					break;
				}
				lpwszDest[nLength] = RevBytes((wchar_t)fgetwc(file));
				if (lpwszDest[nLength] == '\n')
					break;
			}
			lpwszDest[nLength] = 0;

		} else if (!fgetws(lpwszDest, nDestLength, file)) {
			return nullptr;
		}

		return lpwszDest;
	}

	if (_tmp.size() < (nDestLength + 1) * 4)	// UTF-8, up to 4 bytes per char support
		_tmp.resize((nDestLength + 1) * 4);

	if (!fgets(_tmp.data(), _tmp.size(), file))
		return nullptr;

	int n = strnlen(_tmp.data(), _tmp.size());

	n = WINPORT(MultiByteToWideChar)(CP_UTF8, 0, _tmp.data(), n, lpwszDest, nDestLength);
	if (n < 0)
		return nullptr;

	lpwszDest[std::min(size_t(n), nDestLength)] = 0;

	return lpwszDest;
}

//-----------------------------------------------------------------------------

template <class CHAR_T>
class TypedStringReader
{
	CHAR_T ReadBuf[8192];
	std::vector<CHAR_T> Str;

	GetFileStringContext &context;

public:
	TypedStringReader(GetFileStringContext &context_)
		:
		Str(DELTA), context(context_)
	{}

	int GetString(CHAR_T **DestStr, int &Length, bool be = false)
	{
		int CurLength = 0;
		int ExitCode = 1;
		EolType Eol = FEOL_NONE;
		int x = 0;
		CHAR_T cr = '\r', lf = '\n';
		if (be) {
			cr<<= (sizeof(CHAR_T) - 1) * 8;
			lf<<= (sizeof(CHAR_T) - 1) * 8;
		}

		CHAR_T *ReadBufPtr =
				context.ReadPos < context.ReadSize ? &ReadBuf[context.ReadPos / sizeof(CHAR_T)] : nullptr;

		// Обработка ситуации, когда у нас пришёл двойной \r\r, а потом не было \n.
		// В этом случаем считаем \r\r двумя MAC окончаниями строк.
		if (context.bCrCr) {
			Str[0] = cr;
			CurLength = 1;
			context.bCrCr = false;
		} else {
			for (;;) {
				if (context.ReadPos >= context.ReadSize) {
					if (!(context.SrcFile.Read(ReadBuf, sizeof(ReadBuf), &context.ReadSize)
								&& context.ReadSize)) {
						if (!CurLength) {
							ExitCode = 0;
						}
						break;
					}

					context.ReadPos = 0;
					ReadBufPtr = ReadBuf;
				}
				if (Eol == FEOL_NONE) {
					// UNIX
					if (*ReadBufPtr == lf) {
						Eol = FEOL_UNIX;
					}
					// MAC / Windows? / Notepad?
					else if (*ReadBufPtr == cr) {
						Eol = FEOL_MAC;
					}
				} else if (Eol == FEOL_MAC) {
					// Windows
					if (*ReadBufPtr == lf) {
						Eol = FEOL_WINDOWS;
					}
					// Notepad?
					else if (*ReadBufPtr == cr) {
						Eol = FEOL_MAC2;
					} else {
						break;
					}
				} else if (Eol == FEOL_WINDOWS || Eol == FEOL_UNIX) {
					break;
				} else if (Eol == FEOL_MAC2) {
					// Notepad
					if (*ReadBufPtr == lf) {
						Eol = FEOL_NOTEPAD;
					} else {
						// Пришёл \r\r, а \n не пришёл, поэтому считаем \r\r двумя MAC окончаниями строк
						CurLength--;
						context.bCrCr = true;
						break;
					}
				} else {
					break;
				}
				context.ReadPos+= sizeof(CHAR_T);
				if ((CurLength + 1) >= (int)Str.size()) {
					Str.resize(Str.size() + (DELTA << x));
					x++;
				}
				Str[CurLength++] = *ReadBufPtr;
				ReadBufPtr++;
			}
		}
		Str[CurLength] = 0;
		*DestStr = &Str[0];
		Length = CurLength;
		return ExitCode;
	}
};

//////
GetFileString::GetFileString(File &SrcFile)
	:
	context(SrcFile), Peek(false), LastLength(0), LastString(nullptr), LastResult(0), Buffer(128, L'\0')
{}

GetFileString::~GetFileString() {}

int GetFileString::PeekString(LPWSTR *DestStr, UINT nCodePage, int &Length)
{
	if (!Peek) {
		LastResult = GetString(DestStr, nCodePage, Length);
		Peek = true;
		LastString = DestStr;
		LastLength = Length;
	} else {
		DestStr = LastString;
		Length = LastLength;
	}
	return LastResult;
}

static wchar_t s_wchnul = 0;

int GetFileString::GetString(LPWSTR *DestStr, UINT nCodePage, int &Length)
{
	if (Peek) {
		Peek = false;
		DestStr = LastString;
		Length = LastLength;
		return LastResult;
	}

	int nExitCode;
	if (nCodePage == CP_UTF32LE || nCodePage == CP_UTF32BE) {
		if (sizeof(wchar_t) != 4)
			return 0;

		if (!UTF32Reader)
			UTF32Reader.reset(new UTF32_StringReader(context));

		nExitCode = UTF32Reader->GetString((uint32_t **)DestStr, Length, nCodePage == CP_UTF32BE);
		if (nExitCode && nCodePage == CP_UTF32BE)
			RevBytes(*DestStr, Length);

		return nExitCode;
	}

	char *Str;
	if (nCodePage == CP_UTF16LE || nCodePage == CP_UTF16BE) {
		if (!UTF16Reader)
			UTF16Reader.reset(new UTF16_StringReader(context));

		uint16_t *u16 = NULL;
		nExitCode = UTF16Reader->GetString(&u16, Length, nCodePage == CP_UTF16BE);
		if (sizeof(wchar_t) == 2) {
			*DestStr = (LPWSTR)u16;
			if (nExitCode && nCodePage == CP_UTF32BE)
				RevBytes(*DestStr, Length);
			return nExitCode;
		}
		Str = (char *)u16;
		Length*= 2;
	} else {
		if (!CharReader)
			CharReader.reset(new Char_StringReader(context));
		nExitCode = CharReader->GetString(&Str, Length);
	}

	if (nExitCode != 1)
		return nExitCode;

	if (nCodePage == CP_UTF8) {
		MB2Wide(Str, Length, Buffer);
		Length = Buffer.size();
	} else {
		DWORD Result = ERROR_SUCCESS;
		int nResultLength = 0;
		bool bGet = false;

		Buffer[0] = L'\0';

		if (!context.SomeDataLost) {
			// при CP_UTF7 dwFlags должен быть 0, см. MSDN
			nResultLength = WINPORT(MultiByteToWideChar)(nCodePage,
					(context.SomeDataLost || nCodePage == CP_UTF7) ? 0 : MB_ERR_INVALID_CHARS, Str, Length,
					&Buffer[0], Buffer.size() - 1);

			Result = WINPORT(GetLastError)();
			if (Result == ERROR_NO_UNICODE_TRANSLATION) {
				context.SomeDataLost = true;
				if (!nResultLength) {
					bGet = true;
				}
			}
		} else {
			bGet = true;
		}

		if (bGet) {
			nResultLength =
					WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, &Buffer[0], Buffer.size() - 1);
			if (!nResultLength) {
				Result = WINPORT(GetLastError)();
			}
		}
		if (Result == ERROR_INSUFFICIENT_BUFFER) {
			nResultLength = WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, nullptr, 0);
			if (nResultLength >= (int)Buffer.size())
				Buffer.resize(nResultLength + 128);
			Buffer[0] = 0;
			nResultLength =
					WINPORT(MultiByteToWideChar)(nCodePage, 0, Str, Length, &Buffer[0], Buffer.size() - 1);
		}
		if (nResultLength) {
			Buffer[nResultLength] = L'\0';
		}
		Length = nResultLength;
	}

	*DestStr = Length ? &Buffer[0] : &s_wchnul;

	return 1;
}

static bool GetFileFormatBySignature(File &file, UINT &nCodePage)
{
	DWORD dwTemp = 0;
	DWORD Readed = 0;
	if (!file.Read(&dwTemp, sizeof(dwTemp), &Readed)) {
		fprintf(stderr, "%s: read error %u\n", __FUNCTION__, WINPORT(GetLastError)());
		return false;
	}
	if (Readed >= 4 && dwTemp == SIGN_UTF32LE) {
		nCodePage = CP_UTF32LE;
		file.SetPointer(4, nullptr, FILE_BEGIN);
		return true;
	}
	if (Readed >= 4 && dwTemp == SIGN_UTF32BE) {
		nCodePage = CP_UTF32BE;
		file.SetPointer(4, nullptr, FILE_BEGIN);
		return true;
	}
	if (Readed >= 2 && LOWORD(dwTemp) == SIGN_UTF16LE) {
		nCodePage = CP_UTF16LE;
		file.SetPointer(2, nullptr, FILE_BEGIN);
		return true;
	}
	if (Readed >= 2 && LOWORD(dwTemp) == SIGN_UTF16BE) {
		nCodePage = CP_UTF16BE;
		file.SetPointer(2, nullptr, FILE_BEGIN);
		return true;
	}
	if (Readed >= 3 && (dwTemp & 0x00FFFFFF) == SIGN_UTF8) {
		nCodePage = CP_UTF8;
		file.SetPointer(3, nullptr, FILE_BEGIN);
		return true;
	}

	file.SetPointer(0, nullptr, FILE_BEGIN);
	return false;
}

template <class T>
size_t EstimateEncodingValidity(bool rev, const void *buf, size_t size)
{
	/// if text in given codepage contains only valid unicode characters then
	/// return count of well-known control characters widely used in texts
	/// like '\r' '\n' '\t' ' '
	const size_t count = size / sizeof(T);
	if (count * sizeof(T) != size) {	// expecting aligned size
		return 0;
	}
	const T *data = (const T *)buf;
	size_t out = 0;
	for (size_t i = 0; i < count; ++i) {
		T v = rev ? RevBytes(data[i]) : data[i];
		if (v == (T)'\r' || v == (T)'\n' || v == (T)'\t' || v == (T)' ') {
			++out;
		}
		if (!WCHAR_IS_VALID(v)) {	// may be try to transcode?
			return 0;
		}
	}
	if (out < count / 32) {		// too few controls for normal text
		return 0;
	}
	return out;
}

static bool GetFileFormatByHeuristics(File &file, UINT &nCodePage)
{
	file.SetPointer(0, nullptr, FILE_BEGIN);
	BYTE Buffer[0x8000];	// Must be aligned by 32 bits
	DWORD ReadSize = 0;
	bool ReadResult = file.Read(Buffer, sizeof(Buffer), &ReadSize);
	file.SetPointer(0, nullptr, FILE_BEGIN);
	if (!ReadResult || ReadSize < 2)
		return false;

	const auto Val16LE = EstimateEncodingValidity<uint16_t>(false, Buffer, ReadSize);
	const auto Val16BE = EstimateEncodingValidity<uint16_t>(true, Buffer, ReadSize);
	const auto Val32LE = EstimateEncodingValidity<uint32_t>(false, Buffer, ReadSize);
	const auto Val32BE = EstimateEncodingValidity<uint32_t>(true, Buffer, ReadSize);
	if (Val16LE > Val16BE && Val16LE > Val32LE && Val16LE > Val32BE) {
		nCodePage = CP_UTF16LE;
		return true;
	}
	if (Val16BE > Val16LE && Val16BE > Val32LE && Val16BE > Val32BE) {
		nCodePage = CP_UTF16BE;
		return true;
	}
	if (Val32LE > Val16BE && Val32LE >= Val16LE && Val32LE > Val32BE) {
		nCodePage = CP_UTF32LE;
		return true;
	}
	if (Val32BE >= Val16BE && Val32BE > Val16LE && Val32BE > Val32LE) {
		nCodePage = CP_UTF32BE;
		return true;
	}
	if (IsTextUTF8(Buffer, ReadSize)) {
		nCodePage = CP_UTF8;
		return true;
	}
	const int cp = DetectCodePage(reinterpret_cast<LPCSTR>(Buffer), ReadSize);
	if (cp != -1) {
		nCodePage = cp;
		return true;
	}
	return false;
}

bool GetFileFormat(File &file, UINT &nCodePage, bool *pSignatureFound, bool bUseHeuristics)
{
	const bool bSignatureFound = GetFileFormatBySignature(file, nCodePage);

	if (pSignatureFound)
		*pSignatureFound = bSignatureFound;

	if (bSignatureFound)
		return true;

	if (!bUseHeuristics)
		return false;

	return GetFileFormatByHeuristics(file, nCodePage);
}

bool GetFileFormat2(FARString strFileName, UINT &nCodePage, bool *pSignatureFound, bool bUseHeuristics,
		bool bCheckIfSupported)
{
	File f;
	if (!f.Open(strFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL))
		return false;

	UINT detectedCodePage = nCodePage;
	if (!GetFileFormat(f, detectedCodePage, pSignatureFound, bUseHeuristics))
		return false;

	// Проверяем поддерживается или нет задетектированная кодовая страница
	if (bCheckIfSupported && !IsCodePageSupported(detectedCodePage))
		return false;

	nCodePage = detectedCodePage;
	return true;
}
