/*
clipboard.cpp

������ � ������� ������.
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

#include "clipboard.hpp"
#include "console.hpp"

const wchar_t FAR_VerticalBlock[] = L"FAR_VerticalBlock";
const wchar_t FAR_VerticalBlock_Unicode[] = L"FAR_VerticalBlock_Unicode";

/* ------------------------------------------------------------ */
// CF_OEMTEXT CF_TEXT CF_UNICODETEXT CF_HDROP
HGLOBAL Clipboard::hInternalClipboard[5] = {0};
UINT    Clipboard::uInternalClipboardFormat[5] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};

bool Clipboard::UseInternalClipboard = false;
bool Clipboard::InternalClipboardOpen = false;

//Sets UseInternalClipboard to State, and returns previous state
bool Clipboard::SetUseInternalClipboardState(bool State)
{
	bool OldState = UseInternalClipboard;
	UseInternalClipboard = State;
	return OldState;
}

bool Clipboard::GetUseInternalClipboardState()
{
	return UseInternalClipboard;
}

UINT Clipboard::RegisterFormat(LPCWSTR lpszFormat)
{
	if (UseInternalClipboard)
	{
		if (!StrCmp(lpszFormat,FAR_VerticalBlock))
		{
			return 0xFEB0;
		}
		else if (!StrCmp(lpszFormat,FAR_VerticalBlock_Unicode))
		{
			return 0xFEB1;
		}

		return 0;
	}

	return WINPORT(RegisterClipboardFormat)(lpszFormat);
}


BOOL Clipboard::Open()
{
	if (UseInternalClipboard)
	{
		if (!InternalClipboardOpen)
		{
			InternalClipboardOpen=true;
			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(OpenClipboard)(NULL);//Console.GetWindow());
}

BOOL Clipboard::Close()
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			InternalClipboardOpen=false;
			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(CloseClipboard)();
}

BOOL Clipboard::Empty()
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (hInternalClipboard[I])
				{
					WINPORT(GlobalFree)(hInternalClipboard[I]);
					hInternalClipboard[I]=0;
					uInternalClipboardFormat[I]=0xFFFF;
				}
			}

			return TRUE;
		}

		return FALSE;
	}

	return WINPORT(EmptyClipboard)();
}

HANDLE Clipboard::GetData(UINT uFormat)
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (uInternalClipboardFormat[I] != 0xFFFF && uInternalClipboardFormat[I] == uFormat)
				{
					return hInternalClipboard[I];
				}
			}
		}

		return (HANDLE)nullptr;
	}

	return WINPORT(GetClipboardData)(uFormat);
}

/*
UINT Clipboard::EnumFormats(UINT uFormat)
{
  if(UseInternalClipboard)
  {
    if(InternalClipboardOpen)
    {
      for(size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
      {
        if(uInternalClipboardFormat[I] xFFFF && uInternalClipboardFormat[I] == uFormat)
        {
          return I+1 < ARRAYSIZE(hInternalClipboard)?uInternalClipboardFormat[I+1]:0;
        }
      }
    }
    return 0;
  }
  return EnumClipboardFormats(uFormat);
}
*/

HANDLE Clipboard::SetData(UINT uFormat,HANDLE hMem)
{
	if (UseInternalClipboard)
	{
		if (InternalClipboardOpen)
		{
			for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
			{
				if (!hInternalClipboard[I])
				{
					hInternalClipboard[I]=hMem;
					uInternalClipboardFormat[I]=uFormat;
					return hMem;
				}
			}
		}

		return (HANDLE)nullptr;
	}

	HANDLE hData=WINPORT(SetClipboardData)(uFormat,hMem);
	/*
	if (hData)
	{
		HANDLE hLC=WINPORT(GlobalAlloc)(GMEM_MOVEABLE,sizeof(LCID));

		if (hLC)
		{
			PLCID pLc=(PLCID)WINPORT(GlobalLock)(hLC);

			if (pLc)
			{
				*pLc=LOCALE_USER_DEFAULT;
				WINPORT(GlobalUnlock)(hLC);

				if (!WINPORT(SetClipboardData)(CF_LOCALE,pLc))
					WINPORT(GlobalFree)(hLC);
			}
			else
			{
				WINPORT(GlobalFree)(hLC);
			}
		}
	}*/

	return hData;
}

BOOL Clipboard::IsFormatAvailable(UINT Format)
{
	if (UseInternalClipboard)
	{
		for (size_t I=0; I < ARRAYSIZE(hInternalClipboard); ++I)
		{
			if (uInternalClipboardFormat[I] != 0xFFFF && uInternalClipboardFormat[I]==Format)
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	return WINPORT(IsClipboardFormatAvailable)(Format);
}

// ����� �������� ������������ ������� ������
bool Clipboard::Copy(const wchar_t *Data)
{
	Empty();

	if (Data && *Data)
	{
		HGLOBAL hData;
		void *GData;
		int BufferSize=(StrLength(Data)+1)*sizeof(wchar_t);

		if ((hData=WINPORT(GlobalAlloc)(GMEM_MOVEABLE,BufferSize)))
		{
			if ((GData=WINPORT(GlobalLock)(hData)))
			{
				memcpy(GData,Data,BufferSize);
				WINPORT(GlobalUnlock)(hData);

				if (!SetData(CF_UNICODETEXT,(HANDLE)hData))
					WINPORT(GlobalFree)(hData);
			}
			else
			{
				WINPORT(GlobalFree)(hData);
			}
		}
	}

	return true;
}

// ������� ��� ������� ������ - �� ����������
bool Clipboard::CopyFormat(const wchar_t *Format, const wchar_t *Data)
{
	UINT FormatType=RegisterFormat(Format);

	if (!FormatType)
		return false;

	if (Data && *Data)
	{
		HGLOBAL hData;
		void *GData;

		int BufferSize=(StrLength(Data)+1)*sizeof(wchar_t);

		if ((hData=WINPORT(GlobalAlloc)(GMEM_MOVEABLE,BufferSize)))
		{
			if ((GData=WINPORT(GlobalLock)(hData)))
			{
				memcpy(GData,Data,BufferSize);
				WINPORT(GlobalUnlock)(hData);

				if (!SetData(FormatType,(HANDLE)hData))
					WINPORT(GlobalFree)(hData);
			}
			else
			{
				WINPORT(GlobalFree)(hData);
			}
		}
	}

	return true;
}

wchar_t *Clipboard::Paste()
{
	wchar_t *ClipText=nullptr;
	HANDLE hClipData=GetData(CF_UNICODETEXT);

	if (hClipData)
	{
		wchar_t *ClipAddr=(wchar_t *)WINPORT(GlobalLock)(hClipData);

		if (ClipAddr)
		{
			int BufferSize;
			BufferSize=StrLength(ClipAddr)+1;
			ClipText=(wchar_t *)xf_malloc(BufferSize*sizeof(wchar_t));

			if (ClipText)
				wcscpy(ClipText, ClipAddr);

			WINPORT(GlobalUnlock)(hClipData);
		}
	}
	return ClipText;
}

// max - ��� ����� ������� ����� ������!
wchar_t *Clipboard::PasteEx(int max)
{
	wchar_t *ClipText=nullptr;
	HANDLE hClipData=GetData(CF_UNICODETEXT);

	if (hClipData)
	{
		wchar_t *ClipAddr=(wchar_t *)WINPORT(GlobalLock)(hClipData);

		if (ClipAddr)
		{
			int BufferSize;
			BufferSize=StrLength(ClipAddr);

			if (BufferSize>max)
				BufferSize=max;

			ClipText=(wchar_t *)xf_malloc((BufferSize+1)*sizeof(wchar_t));

			if (ClipText)
			{
				wmemset(ClipText,0,BufferSize+1);
				xwcsncpy(ClipText,ClipAddr,BufferSize+1);
			}

			WINPORT(GlobalUnlock)(hClipData);
		}
	}

	return ClipText;
}

wchar_t *Clipboard::PasteFormat(const wchar_t *Format)
{
	bool isOEMVBlock=false;
	UINT FormatType=RegisterFormat(Format);

	if (!FormatType)
		return nullptr;

	if (!StrCmp(Format,FAR_VerticalBlock_Unicode) && !IsFormatAvailable(FormatType))
	{
		FormatType=RegisterFormat(FAR_VerticalBlock);
		isOEMVBlock=true;
	}

	if (!FormatType || !IsFormatAvailable(FormatType))
		return nullptr;

	wchar_t *ClipText=nullptr;
	HANDLE hClipData=GetData(FormatType);

	if (hClipData)
	{
		wchar_t *ClipAddr=(wchar_t *)WINPORT(GlobalLock)(hClipData);

		if (ClipAddr)
		{
			size_t BufferSize;

			if (isOEMVBlock)
				BufferSize=strlen((LPCSTR)ClipAddr)+1;
			else
				BufferSize=wcslen(ClipAddr)+1;

			ClipText=(wchar_t *)xf_malloc(BufferSize*sizeof(wchar_t));

			if (ClipText)
			{
				if (isOEMVBlock) {
					WINPORT(MultiByteToWideChar)(CP_OEMCP,0,(LPCSTR)ClipAddr,-1,ClipText,(int)BufferSize);
				} else
					wcscpy(ClipText,ClipAddr);
			}

			WINPORT(GlobalUnlock)(hClipData);
		}
	}

	return ClipText;
}

/* ------------------------------------------------------------ */
int WINAPI CopyToClipboard(const wchar_t *Data)
{
	Clipboard clip;

	if (!clip.Open())
		return FALSE;

	BOOL ret = clip.Copy(Data);

	clip.Close();

	return ret;
}

int CopyFormatToClipboard(const wchar_t *Format,const wchar_t *Data)
{
	Clipboard clip;

	if (!clip.Open())
		return FALSE;

	BOOL ret = clip.CopyFormat(Format,Data);

	clip.Close();

	return ret;
}

wchar_t * WINAPI PasteFromClipboard()
{
	Clipboard clip;

	if (!clip.Open())
		return nullptr;

	wchar_t *ClipText = clip.Paste();

	clip.Close();

	return ClipText;
}

// max - ��� ����� ������� ����� ������!
wchar_t *PasteFromClipboardEx(int max)
{
	Clipboard clip;

	if (!clip.Open())
		return nullptr;

	wchar_t *ClipText = clip.PasteEx(max);

	clip.Close();

	return ClipText;
}

wchar_t *PasteFormatFromClipboard(const wchar_t *Format)
{
	Clipboard clip;

	if (!clip.Open())
		return nullptr;

	wchar_t *ClipText = clip.PasteFormat(Format);

	clip.Close();

	return ClipText;
}

BOOL EmptyInternalClipboard()
{
	bool OldState = Clipboard::SetUseInternalClipboardState(true);

	Clipboard clip;

	if (!clip.Open())
		return FALSE;

	BOOL ret = clip.Empty();

	clip.Close();

	Clipboard::SetUseInternalClipboardState(OldState);

	return ret;
}
