/*
strmix.cpp

Куча разных вспомогательных функций по работе со строками
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
#pragma hdrstop

#include "strmix.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "config.hpp"
#include "pathmix.hpp"

FARString &FormatNumber(const wchar_t *Src, FARString &strDest, int NumDigits)
{
	strDest = Src;
	while ((int)strDest.GetSize() < NumDigits)strDest.Insert(0, L'0');
	return strDest;
	/*
	static bool first = true;
	static NUMBERFMT fmt;
	static wchar_t DecimalSep[4];
	static wchar_t ThousandSep[4];

	if (first)
	{
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STHOUSAND,ThousandSep,ARRAYSIZE(ThousandSep));
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,DecimalSep,ARRAYSIZE(DecimalSep));
		DecimalSep[1]=0;  //В винде сепараторы цифр могут быть больше одного символа
		ThousandSep[1]=0; //но для нас это будет не очень хорошо

		if (LOWORD(Opt.FormatNumberSeparators))
			*DecimalSep=LOWORD(Opt.FormatNumberSeparators);

		if (HIWORD(Opt.FormatNumberSeparators))
			*ThousandSep=HIWORD(Opt.FormatNumberSeparators);

		fmt.LeadingZero = 1;
		fmt.Grouping = 3;
		fmt.lpDecimalSep = DecimalSep;
		fmt.lpThousandSep = ThousandSep;
		fmt.NegativeOrder = 1;
		first = false;
	}

	fmt.NumDigits = NumDigits;
	FARString strSrc=Src;
	int Size=GetNumberFormat(LOCALE_USER_DEFAULT,0,strSrc,&fmt,nullptr,0);
	wchar_t* lpwszDest=strDest.GetBuffer(Size);
	GetNumberFormat(LOCALE_USER_DEFAULT,0,strSrc,&fmt,lpwszDest,Size);
	strDest.ReleaseBuffer();
	return strDest;*/
}

FARString &InsertCommas(uint64_t li,FARString &strDest)
{
	strDest.Format(L"%llu", li);
	return FormatNumber(strDest,strDest);
}

static wchar_t * WINAPI InsertCustomQuote(wchar_t *Str,wchar_t QuoteChar)
{
	size_t l = StrLength(Str);

	if (*Str != QuoteChar)
	{
		wmemmove(Str+1,Str,++l);
		*Str=QuoteChar;
	}

	if (l==1 || Str[l-1] != QuoteChar)
	{
		Str[l++] = QuoteChar;
		Str[l] = 0;
	}

	return Str;
}

static FARString& InsertCustomQuote(FARString &strStr,wchar_t QuoteChar)
{
	size_t l = strStr.GetLength();

	if (strStr.At(0) != QuoteChar)
	{
		strStr.Insert(0,QuoteChar);
		l++;
	}

	if (l==1 || strStr.At(l-1) != QuoteChar)
	{
		strStr += QuoteChar;
	}

	return strStr;
}

wchar_t * WINAPI InsertQuote(wchar_t *Str)
{
	return InsertCustomQuote(Str,L'\"');
}

wchar_t * WINAPI InsertRegexpQuote(wchar_t *Str)
{
	if (Str && *Str != L'/')
		return InsertCustomQuote(Str,L'/');
	else          //выражение вида /regexp/i не дополняем слэшем
		return Str;
}

FARString& InsertQuote(FARString &strStr)
{
	return InsertCustomQuote(strStr,L'\"');
}

FARString& InsertRegexpQuote(FARString &strStr)
{
	if (strStr.IsEmpty() || strStr[0] != L'/')
		return InsertCustomQuote(strStr,L'/');
	else          //выражение вида /regexp/i не дополняем слэшем
		return strStr;
}


static FARString escapeSpace(const wchar_t* str) {
	if (*str == L'\0')
		return "''";
	FARString result;
	for (const wchar_t *cur = str; *cur; ++cur) {
		if (wcschr(Opt.strQuotedSymbols, *cur) != NULL)
			result.Append('\\');
		result.Append(*cur);
	}
	return result;
}


FARString &EscapeSpace(FARString &strStr)
{
	if (strStr.IsEmpty() || wcspbrk(strStr, Opt.strQuotedSymbols)) {
		strStr.Copy(escapeSpace(strStr.CPtr()));
	}

	return strStr;
}


wchar_t*  WINAPI QuoteSpaceOnly(wchar_t *Str)
{
	if (wcschr(Str,L' '))
		InsertQuote(Str);

	return Str;
}


FARString& WINAPI QuoteSpaceOnly(FARString &strStr)
{
	if (strStr.Contains(L' '))
		InsertQuote(strStr);

	return(strStr);
}


FARString& WINAPI TruncStrFromEnd(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStrFromEnd(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* WINAPI TruncStrFromEnd(wchar_t *Str,int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	if (Str)
	{
		int Length = StrLength(Str);

		if (Length > MaxLength)
		{
			if (MaxLength>3)
				wmemcpy(Str+MaxLength-3, L"...", 3);

			Str[MaxLength]=0;
		}
	}

	return Str;
}


wchar_t* WINAPI TruncStr(wchar_t *Str,int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	if (Str)
	{
		int Length=StrLength(Str);

		if (MaxLength<0)
			MaxLength=0;

		if (Length > MaxLength)
		{
			if (MaxLength>3)
			{
				wchar_t *MovePos = Str+Length-MaxLength+3;
				wmemmove(Str+3, MovePos, StrLength(MovePos)+1);
				wmemcpy(Str,L"...",3);
			}

			Str[MaxLength]=0;
		}
	}

	return Str;
}


FARString& WINAPI TruncStr(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStr(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* TruncStrFromCenter(wchar_t *Str, int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	if (Str)
	{
		int Length = StrLength(Str);

		if (MaxLength < 0)
			MaxLength=0;

		if (Length > MaxLength)
		{
			const int DotsLen = 3;

			if (MaxLength > DotsLen)
			{
				int Len1 = (MaxLength - DotsLen) / 2;
				int Len2 = MaxLength - DotsLen - Len1;
				wmemcpy(Str + Len1, L"...", DotsLen);
				wmemmove(Str + Len1 + DotsLen, Str + Length - Len2, Len2);
			}

			Str[MaxLength] = 0;
		}
	}

	return Str;
}

FARString& TruncStrFromCenter(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszBuffer = strStr.GetBuffer();
	TruncStrFromCenter(lpwszBuffer, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}

wchar_t* WINAPI TruncPathStr(wchar_t *Str, int MaxLength)
{
	assert(MaxLength >= 0);

	MaxLength=Max(0, MaxLength);

	if (Str)
	{
		int nLength = (int)wcslen(Str);

		if ((MaxLength > 0) && (nLength > MaxLength) && (nLength >= 2))
		{
			wchar_t *lpStart = nullptr;

/*			if (*Str && (Str[1] == L':') && IsSlash(Str[2]))
				lpStart = Str+3;
			else*/
			{
				if ((Str[0] == GOOD_SLASH) && (Str[1] == GOOD_SLASH))
				{
					if ((lpStart = const_cast<wchar_t*>(FirstSlash(Str+2))) )
					{
						wchar_t *lpStart2=lpStart;

						if ((lpStart-Str < nLength) && ((lpStart=const_cast<wchar_t*>(FirstSlash(lpStart2+1)))))
							lpStart++;
					}
				}
			}

			if (!lpStart || (lpStart-Str > MaxLength-5))
				return TruncStr(Str, MaxLength);

			wchar_t *lpInPos = lpStart+3+(nLength-MaxLength);
			wmemmove(lpStart+3, lpInPos, (wcslen(lpInPos)+1));
			wmemcpy(lpStart, L"...", 3);
		}
	}

	return Str;
}


FARString& WINAPI TruncPathStr(FARString &strStr, int MaxLength)
{
	wchar_t *lpwszStr = strStr.GetBuffer();
	TruncPathStr(lpwszStr, MaxLength);
	strStr.ReleaseBuffer();
	return strStr;
}


wchar_t* WINAPI RemoveLeadingSpaces(wchar_t *Str)
{
	wchar_t *ChPtr = Str;

	if (!ChPtr)
		return nullptr;

	for (; IsSpace(*ChPtr) || IsEol(*ChPtr); ChPtr++)
		;

	if (ChPtr!=Str)
		wmemmove(Str,ChPtr,StrLength(ChPtr)+1);

	return Str;
}


FARString& WINAPI RemoveLeadingSpaces(FARString &strStr)
{
	const wchar_t *ChPtr = strStr;

	for (; IsSpace(*ChPtr) || IsEol(*ChPtr); ChPtr++)
		;

	strStr.Remove(0,ChPtr-strStr.CPtr());
	return strStr;
}


// удалить конечные пробелы
wchar_t* WINAPI RemoveTrailingSpaces(wchar_t *Str)
{
	if (!Str)
		return nullptr;

	if (!*Str)
		return Str;

	for (wchar_t *ChPtr=Str+StrLength(Str)-1; ChPtr >= Str; ChPtr--)
	{
		if (IsSpace(*ChPtr) || IsEol(*ChPtr))
			*ChPtr=0;
		else
			break;
	}

	return Str;
}


FARString& WINAPI RemoveTrailingSpaces(FARString &strStr)
{
	if (strStr.IsEmpty())
		return strStr;

	const wchar_t *Str = strStr;
	const wchar_t *ChPtr = Str + strStr.GetLength() - 1;

	for (; ChPtr >= Str && (IsSpace(*ChPtr) || IsEol(*ChPtr)); ChPtr--)
		;

	strStr.SetLength(ChPtr < Str ? 0 : ChPtr-Str+1);
	return strStr;
}


wchar_t* WINAPI RemoveExternalSpaces(wchar_t *Str)
{
	return RemoveTrailingSpaces(RemoveLeadingSpaces(Str));
}

FARString&  WINAPI RemoveExternalSpaces(FARString &strStr)
{
	return RemoveTrailingSpaces(RemoveLeadingSpaces(strStr));
}


/* $ 02.02.2001 IS
   Заменяет пробелами непечатные символы в строке. В настоящий момент
   обрабатываются только cr и lf.
*/
FARString& WINAPI RemoveUnprintableCharacters(FARString &strStr)
{
	wchar_t *p = strStr.GetBuffer();

	while (*p)
	{
		if (IsEol(*p))
			*p=L' ';

		p++;
	}

	strStr.ReleaseBuffer(strStr.GetLength());
	return RemoveExternalSpaces(strStr);
}


// Удалить символ Target из строки Str (везде!)
FARString &RemoveChar(FARString &strStr,wchar_t Target,BOOL Dup)
{
	wchar_t *Ptr = strStr.GetBuffer();
	wchar_t *Str = Ptr, Chr;

	while ((Chr=*Str++) )
	{
		if (Chr == Target)
		{
			if (Dup && *Str == Target)
			{
				*Ptr++ = Chr;
				++Str;
			}

			continue;
		}

		*Ptr++ = Chr;
	}

	*Ptr = L'\0';
	strStr.ReleaseBuffer();
	return strStr;
}

FARString& CenterStr(const wchar_t *Src, FARString &strDest, int Length)
{
	int SrcLength=StrLength(Src);
	FARString strTempStr = Src; //если Src == strDest, то надо копировать Src!

	if (SrcLength >= Length)
	{
		/* Здесь не надо отнимать 1 от длины, т.к. strlen не учитывает \0
		   и мы получали обрезанные строки */
		strDest = strTempStr;
		strDest.SetLength(Length);
	}
	else
	{
		int Space=(Length-SrcLength)/2;
		FormatString FString;
		FString<<fmt::Width(Space)<<L""<<strTempStr<<fmt::Width(Length-Space-SrcLength)<<L"";
		strDest=FString.strValue();
	}

	return strDest;
}


const wchar_t *GetCommaWord(const wchar_t *Src, FARString &strWord,wchar_t Separator)
{
	if (!*Src)
		return nullptr;

	const wchar_t *StartPtr = Src;
	size_t WordLen;
	bool SkipBrackets=false;

	for (WordLen=0; *Src; Src++,WordLen++)
	{
		if (*Src==L'[' && wcschr(Src+1,L']'))
			SkipBrackets=true;

		if (*Src==L']')
			SkipBrackets=false;

		if (*Src==Separator && !SkipBrackets)
		{
			Src++;

			while (IsSpace(*Src))
				Src++;

			strWord.Copy(StartPtr,WordLen);
			return Src;
		}
	}

	strWord.Copy(StartPtr,WordLen);
	return Src;
}


BOOL IsCaseMixed(const FARString &strSrc)
{
	const wchar_t *lpwszSrc = strSrc;

	while (*lpwszSrc && !IsAlpha(*lpwszSrc))
		lpwszSrc++;

	int Case = IsLower(*lpwszSrc);

	while (*(lpwszSrc++))
		if (IsAlpha(*lpwszSrc) && (IsLower(*lpwszSrc) != Case))
			return TRUE;

	return FALSE;
}

BOOL IsCaseLower(const FARString &strSrc)
{
	const wchar_t *lpwszSrc = strSrc;

	while (*lpwszSrc)
	{
		if (!IsLower(*lpwszSrc))
			return FALSE;

		lpwszSrc++;
	}

	return TRUE;
}



void WINAPI Unquote(wchar_t *Str)
{
	if (!Str)
		return;

	wchar_t *Dst=Str;

	while (*Str)
	{
		if (*Str!=L'\"')
			*Dst++=*Str;

		Str++;
	}

	*Dst=0;
}


void WINAPI Unquote(FARString &strStr)
{
	wchar_t *Dst = strStr.GetBuffer();
	const wchar_t *Str = Dst;
	const wchar_t *StartPtr = Dst;

	while (*Str)
	{
		if (*Str!=L'\"')
			*Dst++=*Str;

		Str++;
	}

	strStr.ReleaseBuffer(Dst-StartPtr);
}


void UnquoteExternal(FARString &strStr)
{
	size_t len = strStr.GetLength();

	if (len > 1 && strStr.At(0) == L'\"' && strStr.At(len-1) == L'\"')
	{
		strStr.SetLength(len-1);
		strStr.LShift(1);
	}
}


/* FileSizeToStr()
   Форматирование размера файла в удобочитаемый вид.
*/
#define MAX_UNITSTR_SIZE 16

#define UNIT_COUNT 7 // byte, kilobyte, megabyte, gigabyte, terabyte, petabyte, exabyte.

static wchar_t UnitStr[UNIT_COUNT][2][MAX_UNITSTR_SIZE]={0};

void PrepareUnitStr()
{
	for (int i=0; i<UNIT_COUNT; i++)
	{
		xwcsncpy(UnitStr[i][0],MSG(MListBytes+i),MAX_UNITSTR_SIZE);
		wcscpy(UnitStr[i][1],UnitStr[i][0]);
		WINPORT(CharLower)(UnitStr[i][0]);
		WINPORT(CharUpper)(UnitStr[i][1]);
	}
}

FARString & WINAPI FileSizeToStr(FARString &strDestStr, uint64_t Size, int Width, int ViewFlags)
{
	FARString strStr;
	uint64_t Divider;
	int IndexDiv, IndexB;

	// подготовительные мероприятия
	if (!UnitStr[0][0][0])
	{
		PrepareUnitStr();
	}

	int Commas=(ViewFlags & COLUMN_COMMAS);
	int FloatSize=(ViewFlags & COLUMN_FLOATSIZE);
	int Economic=(ViewFlags & COLUMN_ECONOMIC);
	int UseMinSizeIndex=(ViewFlags & COLUMN_MINSIZEINDEX);
	int MinSizeIndex=(ViewFlags & COLUMN_MINSIZEINDEX_MASK)+1;
	int ShowBytesIndex=(ViewFlags & COLUMN_SHOWBYTESINDEX);

	if (ViewFlags & COLUMN_THOUSAND)
	{
		Divider=1000;
		IndexDiv=0;
	}
	else
	{
		Divider=1024;
		IndexDiv=1;
	}

	uint64_t Sz = Size, Divider2 = Divider/2, Divider64 = Divider, OldSize;

	if (FloatSize)
	{
		uint64_t Divider64F = 1, Divider64F_mul = 1000, Divider64F2 = 1, Divider64F2_mul = Divider;

		//выравнивание идёт по 1000 но само деление происходит на Divider
		//например 999 bytes покажутся как 999 а вот 1000 bytes уже покажутся как 0.97 K
		for (IndexB=0; IndexB<UNIT_COUNT-1; IndexB++)
		{
			if (Sz < Divider64F*Divider64F_mul)
				break;

			Divider64F = Divider64F*Divider64F_mul;
			Divider64F2  = Divider64F2*Divider64F2_mul;
		}

		if (!IndexB)
			strStr.Format(L"%d", (DWORD)Sz);
		else
		{
			Sz = (OldSize=Sz) / Divider64F2;
			OldSize = (OldSize % Divider64F2) / (Divider64F2 / Divider64F2_mul);
			DWORD Decimal = (DWORD)(0.5+(double)(DWORD)OldSize/(double)Divider*100.0);

			if (Decimal >= 100)
			{
				Decimal -= 100;
				Sz++;
			}

			strStr.Format(L"%d.%02d", (DWORD)Sz,Decimal);
			FormatNumber(strStr,strStr,2);
		}

		if (IndexB>0 || ShowBytesIndex)
		{
			Width-=(Economic?1:2);

			if (Width<0)
				Width=0;

			if (Economic)
				strDestStr.Format(L"%*.*ls%1.1s",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
			else
				strDestStr.Format(L"%*.*ls %1.1s",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
		}
		else
			strDestStr.Format(L"%*.*ls",Width,Width,strStr.CPtr());

		return strDestStr;
	}

	if (Commas)
		InsertCommas(Sz,strStr);
	else
		strStr.Format(L"%llu", Sz);

	if ((!UseMinSizeIndex && strStr.GetLength()<=static_cast<size_t>(Width)) || Width<5)
	{
		if (ShowBytesIndex)
		{
			Width-=(Economic?1:2);

			if (Width<0)
				Width=0;

			if (Economic)
				strDestStr.Format(L"%*.*ls%1.1ls",Width,Width,strStr.CPtr(),UnitStr[0][IndexDiv]);
			else
				strDestStr.Format(L"%*.*ls %1.1ls",Width,Width,strStr.CPtr(),UnitStr[0][IndexDiv]);
		}
		else
			strDestStr.Format(L"%*.*ls",Width,Width,strStr.CPtr());
	}
	else
	{
		Width-=(Economic?1:2);
		IndexB=0;

		do
		{
			//Sz=(Sz+Divider2)/Divider64;
			Sz = (OldSize=Sz) / Divider64;

			if ((OldSize % Divider64) > Divider2)
				++Sz;

			IndexB++;

			if (Commas)
				InsertCommas(Sz,strStr);
			else
				strStr.Format(L"%llu",Sz);
		}
		while ((UseMinSizeIndex && IndexB<MinSizeIndex) || strStr.GetLength() > static_cast<size_t>(Width));

		if (Economic)
			strDestStr.Format(L"%*.*ls%1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
		else
			strDestStr.Format(L"%*.*ls %1.1ls",Width,Width,strStr.CPtr(),UnitStr[IndexB][IndexDiv]);
	}

	return strDestStr;
}



// вставить с позиции Pos в Str строку InsStr (размером InsSize байт)
// если InsSize = 0, то... вставлять все строку InsStr
// возвращает указатель на Str

wchar_t *InsertString(wchar_t *Str,int Pos,const wchar_t *InsStr,int InsSize)
{
	int InsLen=StrLength(InsStr);

	if (InsSize && InsSize < InsLen)
		InsLen=InsSize;

	wmemmove(Str+Pos+InsLen, Str+Pos, (StrLength(Str+Pos)+1));
	wmemcpy(Str+Pos, InsStr, InsLen);
	return Str;
}


// Заменить в строке Str Count вхождений подстроки FindStr на подстроку ReplStr
// Если Count < 0 - заменять "до полной победы"
// Return - количество замен
int ReplaceStrings(FARString &strStr,const wchar_t *FindStr,const wchar_t *ReplStr,int Count,BOOL IgnoreCase)
{
	const int LenFindStr=StrLength(FindStr);
	if ( !LenFindStr || !Count )
		return 0;
	const int LenReplStr=StrLength(ReplStr);
	size_t L=strStr.GetLength();

	const int Delta = LenReplStr-LenFindStr;
	const int AllocDelta = Delta > 0 ? Delta*10 : 0;

	size_t I=0;
	int J=0;
	while (I+LenFindStr <= L)
	{
		int Res=IgnoreCase?StrCmpNI(&strStr[I], FindStr, LenFindStr):StrCmpN(&strStr[I], FindStr, LenFindStr);

		if (!Res)
		{
			wchar_t *Str;
			if (L+Delta+1 > strStr.GetSize())
				Str = strStr.GetBuffer(L+AllocDelta);
			else
				Str = strStr.GetBuffer();

			if (Delta > 0)
				wmemmove(Str+I+Delta,Str+I,L-I+1);
			else if (Delta < 0)
				wmemmove(Str+I,Str+I-Delta,L-I+Delta+1);

			wmemcpy(Str+I,ReplStr,LenReplStr);
			I += LenReplStr;

			L+=Delta;
			strStr.ReleaseBuffer(L);

			if (++J == Count && Count > 0)
				break;
		}
		else
		{
			I++;
		}
	}

	return J;
}

/*
From PHP 4.x.x
Форматирует исходный текст по заданной ширине, используя
разделительную строку. Возвращает строку SrcText свёрнутую
в колонке, заданной параметром Width. Строка рубится при
помощи строки Break.

Разбивает на строки с выравниваением влево.

Если параметр Flahs & FFTM_BREAKLONGWORD, то строка всегда
сворачивается по заданной ширине. Так если у вас есть слово,
которое больше заданной ширины, то оно будет разрезано на части.

Example 1.
FarFormatText("Пример строки, которая будет разбита на несколько строк по ширине в 20 символов.", 20 ,Dest, "\n", 0);
Этот пример вернет:
---
Пример строки,
которая будет
разбита на
несколько строк по
ширине в 20
символов.
---

Example 2.
FarFormatText( "Эта строка содержит оооооооооооооччччччччеееень длиное слово", 9, Dest, nullptr, FFTM_BREAKLONGWORD);
Этот пример вернет:

---
Эта
строка
содержит
ооооооооо
ооооччччч
чччеееень
длиное
слово
---

*/

enum FFTMODE
{
	FFTM_BREAKLONGWORD = 0x00000001,
};

FARString& WINAPI FarFormatText(const wchar_t *SrcText,     // источник
                             int Width,               // заданная ширина
                             FARString &strDestText,          // приемник
                             const wchar_t* Break,       // брик, если = nullptr, то принимается '\n'
                             DWORD Flags)             // один из FFTM_*
{
	const wchar_t *breakchar;
	breakchar = Break?Break:L"\n";

	if (!SrcText || !*SrcText)
	{
		strDestText.Clear();
		return strDestText;
	}

	FARString strSrc = SrcText; //copy FARString in case of SrcText == strDestText

	if (!wcspbrk(strSrc,breakchar) && strSrc.GetLength() <= static_cast<size_t>(Width))
	{
		strDestText = strSrc;
		return strDestText;
	}

	long i=0, l=0, pgr=0, last=0;
	wchar_t *newtext;
	const wchar_t *text= strSrc;
	long linelength = Width;
	int breakcharlen = StrLength(breakchar);
	int docut = Flags&FFTM_BREAKLONGWORD?1:0;
	/* Special case for a single-character break as it needs no
	   additional storage space */

	if (breakcharlen == 1 && !docut)
	{
		newtext = xf_wcsdup(text);

		if (!newtext)
		{
			strDestText.Clear();
			return strDestText;
		}

		while (newtext[i] != L'\0')
		{
			/* prescan line to see if it is greater than linelength */
			l = 0;

			while (newtext[i+l] != breakchar[0])
			{
				if (newtext[i+l] == L'\0')
				{
					l--;
					break;
				}

				l++;
			}

			if (l >= linelength)
			{
				pgr = l;
				l = linelength;

				/* needs breaking; work backwards to find previous word */
				while (l >= 0)
				{
					if (newtext[i+l] == L' ')
					{
						newtext[i+l] = breakchar[0];
						break;
					}

					l--;
				}

				if (l == -1)
				{
					/* couldn't break is backwards, try looking forwards */
					l = linelength;

					while (l <= pgr)
					{
						if (newtext[i+l] == L' ')
						{
							newtext[i+l] = breakchar[0];
							break;
						}

						l++;
					}
				}
			}

			i += l+1;
		}
	}
	else
	{
		/* Multiple character line break */
		newtext = (wchar_t*)xf_malloc((strSrc.GetLength() * (breakcharlen+1)+1)*sizeof(wchar_t));

		if (!newtext)
		{
			strDestText.Clear();
			return strDestText;
		}

		newtext[0] = L'\0';
		i = 0;

		while (text[i] != L'\0')
		{
			/* prescan line to see if it is greater than linelength */
			l = 0;

			while (text[i+l] != L'\0')
			{
				if (text[i+l] == breakchar[0])
				{
					if (breakcharlen == 1 || !StrCmpN(text+i+l, breakchar, breakcharlen))
						break;
				}

				l++;
			}

			if (l >= linelength)
			{
				pgr = l;
				l = linelength;

				/* needs breaking; work backwards to find previous word */
				while (l >= 0)
				{
					if (text[i+l] == L' ')
					{
						wcsncat(newtext, text+last, i+l-last);
						wcscat(newtext, breakchar);
						last = i + l + 1;
						break;
					}

					l--;
				}

				if (l == -1)
				{
					/* couldn't break it backwards, try looking forwards */
					l = linelength - 1;

					while (l <= pgr)
					{
						if (!docut)
						{
							if (text[i+l] == L' ')
							{
								wcsncat(newtext, text+last, i+l-last);
								wcscat(newtext, breakchar);
								last = i + l + 1;
								break;
							}
						}

						if (docut == 1)
						{
							if (text[i+l] == L' ' || l > i-last)
							{
								wcsncat(newtext, text+last, i+l-last+1);
								wcscat(newtext, breakchar);
								last = i + l + 1;
								break;
							}
						}

						l++;
					}
				}

				i += l+1;
			}
			else
			{
				i += (l ? l : 1);
			}
		}

		if (i+l > last)
		{
			wcscat(newtext, text+last);
		}
	}

	strDestText = newtext;
	xf_free(newtext);
	return strDestText;
}

/*
  Ptr=CalcWordFromString(Str,I,&Start,&End);
  xstrncpy(Dest,Ptr,End-Start+1);
  Dest[End-Start+1]=0;

// Параметры:
//   WordDiv  - набор разделителей слова в кодировке OEM
  возвращает указатель на начало слова
*/
const wchar_t * const CalcWordFromString(const wchar_t *Str,int CurPos,int *Start,int *End, const wchar_t *WordDiv0)
{
	int I, J, StartWPos, EndWPos;
	DWORD DistLeft, DistRight;
	int StrSize=StrLength(Str);

	if (CurPos >= StrSize)
		return nullptr;

	FARString strWordDiv(WordDiv0);
	strWordDiv += L" \t\n\r";

	if (IsWordDiv(strWordDiv,Str[CurPos]))
	{
		// вычисляем дистанцию - куда копать, где ближе слово - слева или справа
		I=J=CurPos;
		// копаем влево
		DistLeft=-1;

		while (I >= 0 && IsWordDiv(strWordDiv,Str[I]))
		{
			DistLeft++;
			I--;
		}

		if (I < 0)
			DistLeft=-1;

		// копаем вправо
		DistRight=-1;

		while (J < StrSize && IsWordDiv(strWordDiv,Str[J]))
		{
			DistRight++;
			J++;
		}

		if (J >= StrSize)
			DistRight=-1;

		if (DistLeft > DistRight) // ?? >=
			EndWPos=StartWPos=J;
		else
			EndWPos=StartWPos=I;
	}
	else // здесь все оби, т.е. стоим на буковке
		EndWPos=StartWPos=CurPos;

	if (StartWPos < StrSize)
	{
		while (StartWPos >= 0)
			if (IsWordDiv(strWordDiv,Str[StartWPos]))
			{
				StartWPos++;
				break;
			}
			else
				StartWPos--;

		while (EndWPos < StrSize)
			if (IsWordDiv(strWordDiv,Str[EndWPos]))
			{
				EndWPos--;
				break;
			}
			else
				EndWPos++;
	}

	if (StartWPos < 0)
		StartWPos=0;

	if (EndWPos >= StrSize)
		EndWPos=StrSize;

	*Start=StartWPos;
	*End=EndWPos;
	return Str+StartWPos;
}


bool CheckFileSizeStringFormat(const wchar_t *FileSizeStr)
{
//проверяет если формат строки такой: [0-9]+[BbKkMmGgTtPpEe]?
	const wchar_t *p = FileSizeStr;

	while (iswdigit(*p))
		p++;

	if (p == FileSizeStr)
		return false;

	if (*p)
	{
		if (*(p+1))
			return false;

		if (!StrStrI(L"BKMGTPE", p))
			return false;
	}

	return true;
}

uint64_t ConvertFileSizeString(const wchar_t *FileSizeStr)
{
	if (!CheckFileSizeStringFormat(FileSizeStr))
		return 0;

	uint64_t n = _wtoi64(FileSizeStr);
	wchar_t c = Upper(FileSizeStr[StrLength(FileSizeStr)-1]);

	// http://en.wikipedia.org/wiki/SI_prefix
	switch (c)
	{
		case L'K':		// kilo 10x3
			n <<= 10;
			break;
		case L'M':		// mega 10x6
			n <<= 20;
			break;
		case L'G':		// giga 10x9
			n <<= 30;
			break;
		case L'T':		// tera 10x12
			n <<= 40;
			break;
		case L'P':		// peta 10x15
			n <<= 50;
			break;
		case L'E':		// exa  10x18
			n <<= 60;
			break;
			// Z - zetta 10x21
			// Y - yotta 10x24
	}

	return n;
}

/* $ 21.09.2003 KM
   Трансформация строки по заданному типу.
*/
void Transform(FARString &strBuffer,const wchar_t *ConvStr,wchar_t TransformType)
{
	FARString strTemp;

	switch (TransformType)
	{
		case L'X': // Convert common FARString to hexadecimal FARString representation
		{
			FARString strHex;

			while (*ConvStr)
			{
				strHex.Format(L"%02X",*ConvStr);
				strTemp += strHex;
				ConvStr++;
			}

			break;
		}
		case L'S': // Convert hexadecimal FARString representation to common string
		{
			const wchar_t *ptrConvStr=ConvStr;

			while (*ptrConvStr)
			{
				if (*ptrConvStr != L' ')
				{
					WCHAR Hex[]={ptrConvStr[0],ptrConvStr[1],0};
					size_t l=strTemp.GetLength();
					wchar_t *Temp=strTemp.GetBuffer(l+2);
					Temp[l]=(wchar_t)wcstoul(Hex,nullptr,16)&0xFFFF;
					strTemp.ReleaseBuffer(l+1);
					ptrConvStr++;
				}

				ptrConvStr++;
			}

			break;
		}
		default:
			break;
	}

	strBuffer=strTemp;
}

wchar_t GetDecimalSeparator()
{
	//wchar_t Separator[4];
	//GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,Separator,ARRAYSIZE(Separator));
	//return *Separator;
	return L'.';
}

FARString ReplaceBrackets(const FARString& SearchStr,const FARString& ReplaceStr,RegExpMatch* Match,int Count)
{
	FARString result;
	size_t pos=0,length=ReplaceStr.GetLength();

	while (pos<length)
	{
		bool common=true;

		if (ReplaceStr[pos]=='$')
		{
			++pos;

			if (pos>length) break;

			wchar_t symbol=Upper(ReplaceStr[pos]);
			int index=-1;

			if (symbol>='0'&&symbol<='9')
			{
				index=symbol-'0';
			}
			else if (symbol>='A'&&symbol<='Z')
			{
				index=symbol-'A'+10;
			}

			if (index>=0)
			{
				if (index<Count)
				{
					FARString bracket(SearchStr.CPtr()+Match[index].start,Match[index].end-Match[index].start);
					result+=bracket;
				}

				common=false;
			}
		}

		if (common)
		{
			result+=ReplaceStr[pos];
		}

		++pos;
	}

	return result;
}
