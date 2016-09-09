/*
filefilterparams.cpp

Параметры Файлового фильтра
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

#include "colors.hpp"
#include "CFileMask.hpp"
#include "FileMasksWithExclude.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filelist.hpp"
#include "filefilterparams.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "setcolor.hpp"
#include "datetime.hpp"
#include "strmix.hpp"

FileFilterParams::FileFilterParams()
{
	SetMask(1,L"*");
	SetSize(0,L"",L"");
	memset(&FDate,0,sizeof(FDate));
	memset(&FAttr,0,sizeof(FAttr));
	memset(&FHighlight.Colors,0,sizeof(FHighlight.Colors));
	FHighlight.SortGroup=DEFAULT_SORT_GROUP;
	FHighlight.bContinueProcessing=false;
	ClearAllFlags();
}

const FileFilterParams &FileFilterParams::operator=(const FileFilterParams &FF)
{
	if (this != &FF)
	{
		SetTitle(FF.GetTitle());
		const wchar_t *Mask;
		FF.GetMask(&Mask);
		SetMask(FF.GetMask(nullptr),Mask);
		FSize=FF.FSize;
		FDate=FF.FDate;
		FAttr=FF.FAttr;
		FF.GetColors(&FHighlight.Colors);
		FHighlight.SortGroup=FF.GetSortGroup();
		FHighlight.bContinueProcessing=FF.GetContinueProcessing();
		memcpy(FFlags,FF.FFlags,sizeof(FFlags));
	}

	return *this;
}

void FileFilterParams::SetTitle(const wchar_t *Title)
{
	m_strTitle = Title;
}

// Преобразование корявого формата PATHEXT в ФАРовский :-)
// Функции передается нужные расширения, она лишь добавляет то, что есть
// в %PATHEXT%
// IS: Сравнений на совпадение очередной маски с тем, что имеется в Dest
// IS: не делается, т.к. дубли сами уберутся при компиляции маски
FARString &Add_PATHEXT(FARString &strDest)
{
	FARString strBuf;
	size_t curpos=strDest.GetLength()-1;
	UserDefinedList MaskList(0,0,ULF_UNIQUE);

	if (apiGetEnvironmentVariable(L"PATHEXT",strBuf) && MaskList.Set(strBuf))
	{
		/* $ 13.10.2002 IS проверка на '|' (маски исключения) */
		if (!strDest.IsEmpty() && (strDest.At(curpos)!=L',' && strDest.At(curpos)!=L';') && strDest.At(curpos)!=L'|')
			strDest += L",";

		const wchar_t *Ptr;
		MaskList.Reset();

		while (nullptr!=(Ptr=MaskList.GetNext()))
		{
			strDest += L"*";
			strDest += Ptr;
			strDest += L",";
		}
	}

	// лишняя запятая - в морг!
	curpos=strDest.GetLength()-1;

	if (strDest.At(curpos) == L',' || strDest.At(curpos)==L';')
		strDest.SetLength(curpos);

	return strDest;
}

void FileFilterParams::SetMask(bool Used, const wchar_t *Mask)
{
	FMask.Used = Used;
	FMask.strMask = Mask;
	/* Обработка %PATHEXT% */
	FARString strMask = FMask.strMask;
	size_t pos;

	// проверим
	if (strMask.PosI(pos,L"%PATHEXT%"))
	{
		{
			size_t IQ1=(strMask.At(pos+9) == L',' || strMask.At(pos+9) == L';')?10:9;
			wchar_t *Ptr = strMask.GetBuffer();
			// Если встречается %pathext%, то допишем в конец...
			wmemmove(Ptr+pos,Ptr+pos+IQ1,strMask.GetLength()-pos-IQ1+1);
			strMask.ReleaseBuffer();
		}
		size_t posSeparator;

		if (strMask.Pos(posSeparator, EXCLUDEMASKSEPARATOR))
		{
			if (pos > posSeparator) // PATHEXT находится в масках исключения
			{
				Add_PATHEXT(strMask); // добавляем то, чего нету.
			}
			else
			{
				FARString strTmp = strMask;
				strTmp.LShift(posSeparator+1);
				strMask.SetLength(posSeparator);
				Add_PATHEXT(strMask);
				strMask += strTmp;
			}
		}
		else
		{
			Add_PATHEXT(strMask); // добавляем то, чего нету.
		}
	}

	// Проверка на валидность текущих настроек фильтра
	if (!FMask.FilterMask.Set(strMask,FMF_SILENT))
	{
		FMask.strMask = L"*";
		FMask.FilterMask.Set(FMask.strMask,FMF_SILENT);
	}
}

void FileFilterParams::SetDate(bool Used, DWORD DateType, FILETIME DateAfter, FILETIME DateBefore, bool bRelative)
{
	FDate.Used=Used;
	FDate.DateType=(enumFDateType)DateType;

	if (DateType>=FDATE_COUNT)
		FDate.DateType=FDATE_MODIFIED;

	FDate.DateAfter.u.LowPart=DateAfter.dwLowDateTime;
	FDate.DateAfter.u.HighPart=DateAfter.dwHighDateTime;
	FDate.DateBefore.u.LowPart=DateBefore.dwLowDateTime;
	FDate.DateBefore.u.HighPart=DateBefore.dwHighDateTime;
	FDate.bRelative=bRelative;
}

void FileFilterParams::SetSize(bool Used, const wchar_t *SizeAbove, const wchar_t *SizeBelow)
{
	FSize.Used=Used;
	xwcsncpy(FSize.SizeAbove,SizeAbove,ARRAYSIZE(FSize.SizeAbove));
	xwcsncpy(FSize.SizeBelow,SizeBelow,ARRAYSIZE(FSize.SizeBelow));
	FSize.SizeAboveReal=ConvertFileSizeString(FSize.SizeAbove);
	FSize.SizeBelowReal=ConvertFileSizeString(FSize.SizeBelow);
}

void FileFilterParams::SetAttr(bool Used, DWORD AttrSet, DWORD AttrClear)
{
	FAttr.Used=Used;
	FAttr.AttrSet=AttrSet;
	FAttr.AttrClear=AttrClear;
}

void FileFilterParams::SetColors(HighlightDataColor *Colors)
{
	FHighlight.Colors=*Colors;
}

const wchar_t *FileFilterParams::GetTitle() const
{
	return m_strTitle;
}

bool FileFilterParams::GetMask(const wchar_t **Mask) const
{
	if (Mask)
		*Mask=FMask.strMask;

	return FMask.Used;
}

bool FileFilterParams::GetDate(DWORD *DateType, FILETIME *DateAfter, FILETIME *DateBefore, bool *bRelative) const
{
	if (DateType)
		*DateType=FDate.DateType;

	if (DateAfter)
	{
		DateAfter->dwLowDateTime=FDate.DateAfter.u.LowPart;
		DateAfter->dwHighDateTime=FDate.DateAfter.u.HighPart;
	}

	if (DateBefore)
	{
		DateBefore->dwLowDateTime=FDate.DateBefore.u.LowPart;
		DateBefore->dwHighDateTime=FDate.DateBefore.u.HighPart;
	}

	if (bRelative)
		*bRelative=FDate.bRelative;

	return FDate.Used;
}

bool FileFilterParams::GetSize(const wchar_t **SizeAbove, const wchar_t **SizeBelow) const
{
	if (SizeAbove)
		*SizeAbove=FSize.SizeAbove;

	if (SizeBelow)
		*SizeBelow=FSize.SizeBelow;

	return FSize.Used;
}

bool FileFilterParams::GetAttr(DWORD *AttrSet, DWORD *AttrClear) const
{
	if (AttrSet)
		*AttrSet=FAttr.AttrSet;

	if (AttrClear)
		*AttrClear=FAttr.AttrClear;

	return FAttr.Used;
}

void FileFilterParams::GetColors(HighlightDataColor *Colors) const
{
	*Colors=FHighlight.Colors;
}

int FileFilterParams::GetMarkChar() const
{
	return FHighlight.Colors.MarkChar;
}

bool FileFilterParams::FileInFilter(const FileListItem& fli, uint64_t CurrentTime)
{
	FAR_FIND_DATA_EX fde;
	fde.dwFileAttributes=fli.FileAttr;
	fde.dwUnixMode=fli.FileMode;
	fde.ftCreationTime=fli.CreationTime;
	fde.ftLastAccessTime=fli.AccessTime;
	fde.ftLastWriteTime=fli.WriteTime;
	fde.ftChangeTime=fli.ChangeTime;
	fde.nFileSize=fli.UnpSize;
	fde.nPackSize=fli.PackSize;
	fde.strFileName=fli.strName;
	return FileInFilter(fde, CurrentTime);
}

bool FileFilterParams::FileInFilter(const FAR_FIND_DATA_EX& fde, uint64_t CurrentTime)
{
	// Режим проверки атрибутов файла включен?
	if (FAttr.Used)
	{
		// Проверка попадания файла по установленным атрибутам
		if ((fde.dwFileAttributes & FAttr.AttrSet) != FAttr.AttrSet)
			return false;

		// Проверка попадания файла по отсутствующим атрибутам
		if (fde.dwFileAttributes & FAttr.AttrClear)
			return false;
	}

	// Режим проверки размера файла включен?
	if (FSize.Used)
	{
		if (*FSize.SizeAbove)
		{
			if (fde.nFileSize < FSize.SizeAboveReal) // Размер файла меньше минимального разрешённого по фильтру?
				return false;                          // Не пропускаем этот файл
		}

		if (*FSize.SizeBelow)
		{
			if (fde.nFileSize > FSize.SizeBelowReal) // Размер файла больше максимального разрешённого по фильтру?
				return false;                          // Не пропускаем этот файл
		}
	}

	// Режим проверки времени файла включен?
	if (FDate.Used)
	{
		// Преобразуем FILETIME в беззнаковый int64_t
		uint64_t after  = FDate.DateAfter.QuadPart;
		uint64_t before = FDate.DateBefore.QuadPart;

		if (after || before)
		{
			const FILETIME *ft;

			switch (FDate.DateType)
			{
				case FDATE_CREATED:
					ft=&fde.ftCreationTime;
					break;
				case FDATE_OPENED:
					ft=&fde.ftLastAccessTime;
					break;
				case FDATE_CHANGED:
					ft=&fde.ftChangeTime;
					break;
				default: //case FDATE_MODIFIED:
					ft=&fde.ftLastWriteTime;
			}

			ULARGE_INTEGER ftime;
			ftime.u.LowPart  = ft->dwLowDateTime;
			ftime.u.HighPart = ft->dwHighDateTime;

			if (FDate.bRelative)
			{
				if (after)
					after = CurrentTime - after;

				if (before)
					before = CurrentTime - before;
			}

			// Есть введённая пользователем начальная дата?
			if (after)
			{
				// Дата файла меньше начальной даты по фильтру?
				if (ftime.QuadPart<after)
					// Не пропускаем этот файл
					return false;
			}

			// Есть введённая пользователем конечная дата?
			if (before)
			{
				// Дата файла больше конечной даты по фильтру?
				if (ftime.QuadPart>before)
					return false;
			}
		}
	}

	// Режим проверки маски файла включен?
	if (FMask.Used)
	{
		// Файл не попадает под маску введённую в фильтре?
		if (!FMask.FilterMask.Compare(fde.strFileName))
			// Не пропускаем этот файл
			return false;
	}

	// Да! Файл выдержал все испытания и будет допущен к использованию
	// в вызвавшей эту функцию операции.
	return true;
}

bool FileFilterParams::FileInFilter(const FAR_FIND_DATA& fd, uint64_t CurrentTime)
{
	FAR_FIND_DATA_EX fde;
	apiFindDataToDataEx(&fd,&fde);
	return FileInFilter(fde, CurrentTime);
}

//Централизованная функция для создания строк меню различных фильтров.
void MenuString(FARString &strDest, FileFilterParams *FF, bool bHighlightType, int Hotkey, bool bPanelType, const wchar_t *FMask, const wchar_t *Title)
{
	const wchar_t AttrC[] = L"RAHSDCEI$TLOV";
	const DWORD   AttrF[] =
	{
		FILE_ATTRIBUTE_READONLY,
		FILE_ATTRIBUTE_ARCHIVE,
		FILE_ATTRIBUTE_HIDDEN,
		FILE_ATTRIBUTE_SYSTEM,
		FILE_ATTRIBUTE_DIRECTORY,
		FILE_ATTRIBUTE_COMPRESSED,
		FILE_ATTRIBUTE_ENCRYPTED,
		FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
		FILE_ATTRIBUTE_SPARSE_FILE,
		FILE_ATTRIBUTE_TEMPORARY,
		FILE_ATTRIBUTE_REPARSE_POINT,
		FILE_ATTRIBUTE_OFFLINE,
		FILE_ATTRIBUTE_VIRTUAL,
	};
	const wchar_t Format1a[] = L"%-21.21ls %lc %-26.26ls %-2.2ls %lc %ls";
	const wchar_t Format1b[] = L"%-22.22ls %lc %-26.26ls %-2.2ls %lc %ls";
	const wchar_t Format1c[] = L"&%lc. %-18.18ls %lc %-26.26ls %-2.2ls %lc %ls";
	const wchar_t Format1d[] = L"   %-18.18ls %lc %-26.26ls %-2.2ls %lc %ls";
	const wchar_t Format2[]  = L"%-3.3ls %lc %-26.26ls %-3.3ls %lc %ls";
	const wchar_t DownArrow=0x2193;
	const wchar_t *Name, *Mask;
	wchar_t MarkChar[]=L"\" \"";
	DWORD IncludeAttr, ExcludeAttr;
	bool UseMask, UseSize, UseDate, RelativeDate;

	if (bPanelType)
	{
		Name=Title;
		UseMask=true;
		Mask=FMask;
		IncludeAttr=0;
		ExcludeAttr=FILE_ATTRIBUTE_DIRECTORY;
		RelativeDate=UseDate=UseSize=false;
	}
	else
	{
		MarkChar[1]=(wchar_t)FF->GetMarkChar();

		if (!MarkChar[1])
			*MarkChar=0;

		Name=FF->GetTitle();
		UseMask=FF->GetMask(&Mask);

		if (!FF->GetAttr(&IncludeAttr,&ExcludeAttr))
			IncludeAttr=ExcludeAttr=0;

		UseSize=FF->GetSize(nullptr,nullptr);
		UseDate=FF->GetDate(nullptr,nullptr,nullptr,&RelativeDate);
	}

	wchar_t Attr[ARRAYSIZE(AttrC)*2] = {0};

	for (size_t i=0; i<ARRAYSIZE(AttrF); i++)
	{
		wchar_t *Ptr=Attr+i*2;
		*Ptr=AttrC[i];

		if (IncludeAttr&AttrF[i])
			*(Ptr+1)=L'+';
		else if (ExcludeAttr&AttrF[i])
			*(Ptr+1)=L'-';
		else
			*Ptr=*(Ptr+1)=L'.';
	}

	wchar_t SizeDate[4] = L"...";

	if (UseSize)
	{
		SizeDate[0]=L'S';
	}

	if (UseDate)
	{
		if (RelativeDate)
			SizeDate[1]=L'R';
		else
			SizeDate[1]=L'D';
	}

	if (bHighlightType)
	{
		if (FF->GetContinueProcessing())
			SizeDate[2]=DownArrow;

		strDest.Format(Format2, MarkChar, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
	}
	else
	{
		SizeDate[2]=0;

		if (!Hotkey && !bPanelType)
		{
			strDest.Format(wcschr(Name, L'&') ? Format1b : Format1a, Name, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
		}
		else
		{
			if (Hotkey)
				strDest.Format(Format1c, Hotkey, Name, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
			else
				strDest.Format(Format1d, Name, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
		}
	}

	RemoveTrailingSpaces(strDest);
}

enum enumFileFilterConfig
{
	ID_FF_TITLE,

	ID_FF_NAME,
	ID_FF_NAMEEDIT,

	ID_FF_SEPARATOR1,

	ID_FF_MATCHMASK,
	ID_FF_MASKEDIT,

	ID_FF_SEPARATOR2,

	ID_FF_MATCHSIZE,
	ID_FF_SIZEFROMSIGN,
	ID_FF_SIZEFROMEDIT,
	ID_FF_SIZETOSIGN,
	ID_FF_SIZETOEDIT,

	ID_FF_MATCHDATE,
	ID_FF_DATETYPE,
	ID_FF_DATERELATIVE,
	ID_FF_DATEBEFORESIGN,
	ID_FF_DATEBEFOREEDIT,
	ID_FF_DAYSBEFOREEDIT,
	ID_FF_TIMEBEFOREEDIT,
	ID_FF_DATEAFTERSIGN,
	ID_FF_DATEAFTEREDIT,
	ID_FF_DAYSAFTEREDIT,
	ID_FF_TIMEAFTEREDIT,
	ID_FF_CURRENT,
	ID_FF_BLANK,

	ID_FF_SEPARATOR3,
	ID_FF_SEPARATOR4,

	ID_FF_MATCHATTRIBUTES,
	ID_FF_READONLY,
	ID_FF_ARCHIVE,
	ID_FF_HIDDEN,
	ID_FF_SYSTEM,
	ID_FF_DIRECTORY,
	ID_FF_COMPRESSED,
	ID_FF_ENCRYPTED,
	ID_FF_NOTINDEXED,
	ID_FF_REPARSEPOINT,
	ID_FF_SPARSE,
	ID_FF_TEMP,
	ID_FF_OFFLINE,
	ID_FF_VIRTUAL,

	ID_HER_SEPARATOR1,
	ID_HER_MARK_TITLE,
	ID_HER_MARKEDIT,
	ID_HER_MARKTRANSPARENT,

	ID_HER_NORMALFILE,
	ID_HER_NORMALMARKING,
	ID_HER_SELECTEDFILE,
	ID_HER_SELECTEDMARKING,
	ID_HER_CURSORFILE,
	ID_HER_CURSORMARKING,
	ID_HER_SELECTEDCURSORFILE,
	ID_HER_SELECTEDCURSORMARKING,

	ID_HER_COLOREXAMPLE,
	ID_HER_CONTINUEPROCESSING,

	ID_FF_SEPARATOR5,

	ID_FF_OK,
	ID_FF_RESET,
	ID_FF_CANCEL,
	ID_FF_MAKETRANSPARENT,
};

void HighlightDlgUpdateUserControl(CHAR_INFO *VBufColorExample,HighlightDataColor &Colors)
{
	const wchar_t *ptr;
	DWORD Color;
	const DWORD FarColor[] = {COL_PANELTEXT,COL_PANELSELECTEDTEXT,COL_PANELCURSOR,COL_PANELSELECTEDCURSOR};

	for (int j=0; j<4; j++)
	{
		Color=(DWORD)(Colors.Color[HIGHLIGHTCOLORTYPE_FILE][j]&0x00FF);

		if (!Color)
			Color=FarColorToReal(FarColor[j]);

		if (Colors.MarkChar&0x0000FFFF)
			ptr=MSG(MHighlightExample2);
		else
			ptr=MSG(MHighlightExample1);

		for (int k=0; k<15; k++)
		{
			VBufColorExample[15*j+k].Char.UnicodeChar=ptr[k];
			VBufColorExample[15*j+k].Attributes=(WORD)Color;
		}

		if (Colors.MarkChar&0x0000FFFF)
		{
			VBufColorExample[15*j+1].Char.UnicodeChar=(WCHAR)Colors.MarkChar&0x0000FFFF;

			if (Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][j]&0x00FF)
				VBufColorExample[15*j+1].Attributes=Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][j]&0x00FF;
		}

		VBufColorExample[15*j].Attributes=FarColorToReal(COL_PANELBOX);
		VBufColorExample[15*j+14].Attributes=FarColorToReal(COL_PANELBOX);
	}
}

void FilterDlgRelativeDateItemsUpdate(HANDLE hDlg, bool bClear)
{
	SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);

	if (SendDlgMessage(hDlg,DM_GETCHECK,ID_FF_DATERELATIVE,0))
	{
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DATEBEFOREEDIT,0);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DATEAFTEREDIT,0);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_CURRENT,0);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DAYSBEFOREEDIT,1);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DAYSAFTEREDIT,1);
	}
	else
	{
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DAYSBEFOREEDIT,0);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DAYSAFTEREDIT,0);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DATEBEFOREEDIT,1);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_DATEAFTEREDIT,1);
		SendDlgMessage(hDlg,DM_SHOWITEM,ID_FF_CURRENT,1);
	}

	if (bClear)
	{
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_DATEAFTEREDIT,(LONG_PTR)L"");
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_DAYSAFTEREDIT,(LONG_PTR)L"");
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_TIMEAFTEREDIT,(LONG_PTR)L"");
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_DATEBEFOREEDIT,(LONG_PTR)L"");
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_TIMEBEFOREEDIT,(LONG_PTR)L"");
		SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_DAYSBEFOREEDIT,(LONG_PTR)L"");
	}

	SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
}

LONG_PTR WINAPI FileFilterConfigDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DN_INITDIALOG:
		{
			FilterDlgRelativeDateItemsUpdate(hDlg, false);
			return TRUE;
		}
		case DN_BTNCLICK:
		{
			if (Param1==ID_FF_CURRENT || Param1==ID_FF_BLANK) //Current и Blank
			{
				FILETIME ft;
				FARString strDate, strTime;

				if (Param1==ID_FF_CURRENT)
				{
					WINPORT(GetSystemTimeAsFileTime)(&ft);
					ConvertDate(ft,strDate,strTime,12,FALSE,FALSE,2);
				}
				else
				{
					strDate.Clear();
					strTime.Clear();
				}

				SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
				int relative = (int)SendDlgMessage(hDlg,DM_GETCHECK,ID_FF_DATERELATIVE,0);
				int db = relative ? ID_FF_DAYSBEFOREEDIT : ID_FF_DATEBEFOREEDIT;
				int da = relative ? ID_FF_DAYSAFTEREDIT  : ID_FF_DATEAFTEREDIT;
				SendDlgMessage(hDlg,DM_SETTEXTPTR,da,(LONG_PTR)strDate.CPtr());
				SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_TIMEAFTEREDIT,(LONG_PTR)strTime.CPtr());
				SendDlgMessage(hDlg,DM_SETTEXTPTR,db,(LONG_PTR)strDate.CPtr());
				SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_TIMEBEFOREEDIT,(LONG_PTR)strTime.CPtr());
				SendDlgMessage(hDlg,DM_SETFOCUS,da,0);
				COORD r;
				r.X=r.Y=0;
				SendDlgMessage(hDlg,DM_SETCURSORPOS,da,(LONG_PTR)&r);
				SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
				break;
			}
			else if (Param1==ID_FF_RESET) // Reset
			{
				SendDlgMessage(hDlg,DM_ENABLEREDRAW,FALSE,0);
				LONG_PTR ColorConfig = SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
				SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_MASKEDIT,(LONG_PTR)L"*");
				SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_SIZEFROMEDIT,(LONG_PTR)L"");
				SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_FF_SIZETOEDIT,(LONG_PTR)L"");

				for (int I=ID_FF_READONLY; I <= ID_FF_VIRTUAL; ++I)
				{
					SendDlgMessage(hDlg,DM_SETCHECK,I,BSTATE_3STATE);
				}

				if (!ColorConfig)
					SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_DIRECTORY,BSTATE_UNCHECKED);

				FarListPos LPos={0,0};
				SendDlgMessage(hDlg,DM_LISTSETCURPOS,ID_FF_DATETYPE,(LONG_PTR)&LPos);
				SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_MATCHMASK,BSTATE_CHECKED);
				SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_MATCHSIZE,BSTATE_UNCHECKED);
				SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_MATCHDATE,BSTATE_UNCHECKED);
				SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_DATERELATIVE,BSTATE_UNCHECKED);
				FilterDlgRelativeDateItemsUpdate(hDlg, true);
				SendDlgMessage(hDlg,DM_SETCHECK,ID_FF_MATCHATTRIBUTES,ColorConfig?BSTATE_UNCHECKED:BSTATE_CHECKED);
				SendDlgMessage(hDlg,DM_ENABLEREDRAW,TRUE,0);
				break;
			}
			else if (Param1==ID_FF_MAKETRANSPARENT)
			{
				HighlightDataColor *Colors = (HighlightDataColor *) SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

				for (int i=0; i<2; i++)
					for (int j=0; j<4; j++)
						Colors->Color[i][j]|=0xFF00;

				SendDlgMessage(hDlg,DM_SETCHECK,ID_HER_MARKTRANSPARENT,BSTATE_CHECKED);
				break;
			}
			else if (Param1==ID_FF_DATERELATIVE)
			{
				FilterDlgRelativeDateItemsUpdate(hDlg, true);
				break;
			}
		}
		case DN_MOUSECLICK:

			if ((Msg==DN_BTNCLICK && Param1 >= ID_HER_NORMALFILE && Param1 <= ID_HER_SELECTEDCURSORMARKING)
			        || (Msg==DN_MOUSECLICK && Param1==ID_HER_COLOREXAMPLE && ((MOUSE_EVENT_RECORD *)Param2)->dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED))
			{
				HighlightDataColor *EditData = (HighlightDataColor *) SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

				if (Msg==DN_MOUSECLICK)
				{
					Param1 = ID_HER_NORMALFILE + ((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.Y*2;

					if (((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.X==1 && (EditData->MarkChar&0x0000FFFF))
						Param1 = ID_HER_NORMALMARKING + ((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.Y*2;
				}

				//Color[0=file, 1=mark][0=normal,1=selected,2=undercursor,3=selectedundercursor]
				WORD Color=EditData->Color[(Param1-ID_HER_NORMALFILE)&1][(Param1-ID_HER_NORMALFILE)/2];
				GetColorDialog(Color,true,true);
				EditData->Color[(Param1-ID_HER_NORMALFILE)&1][(Param1-ID_HER_NORMALFILE)/2]=(WORD)Color;
				FarDialogItem *ColorExample = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_HER_COLOREXAMPLE,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_HER_COLOREXAMPLE,(LONG_PTR)ColorExample);
				wchar_t MarkChar[2];
				//MarkChar это FIXEDIT размером в 1 символ так что проверять размер строки не надо
				SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_HER_MARKEDIT,(LONG_PTR)MarkChar);
				EditData->MarkChar=*MarkChar;
				HighlightDlgUpdateUserControl(ColorExample->Param.VBuf,*EditData);
				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_HER_COLOREXAMPLE,(LONG_PTR)ColorExample);
				xf_free(ColorExample);
				return TRUE;
			}

			break;
		case DN_EDITCHANGE:

			if (Param1 == ID_HER_MARKEDIT)
			{
				HighlightDataColor *EditData = (HighlightDataColor *) SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
				FarDialogItem *ColorExample = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_HER_COLOREXAMPLE,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_HER_COLOREXAMPLE,(LONG_PTR)ColorExample);
				wchar_t MarkChar[2];
				//MarkChar это FIXEDIT размером в 1 символ так что проверять размер строки не надо
				SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_HER_MARKEDIT,(LONG_PTR)MarkChar);
				EditData->MarkChar=*MarkChar;
				HighlightDlgUpdateUserControl(ColorExample->Param.VBuf,*EditData);
				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_HER_COLOREXAMPLE,(LONG_PTR)ColorExample);
				xf_free(ColorExample);
				return TRUE;
			}

			break;
		case DN_CLOSE:

			if (Param1 == ID_FF_OK && SendDlgMessage(hDlg,DM_GETCHECK,ID_FF_MATCHSIZE,0))
			{
				FARString strTemp;
				wchar_t *temp;
				temp = strTemp.GetBuffer(SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_FF_SIZEFROMEDIT,0)+1);
				SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_FF_SIZEFROMEDIT,(LONG_PTR)temp);
				bool bTemp = !*temp || CheckFileSizeStringFormat(temp);
				temp = strTemp.GetBuffer(SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_FF_SIZETOEDIT,0)+1);
				SendDlgMessage(hDlg,DM_GETTEXTPTR,ID_FF_SIZETOEDIT,(LONG_PTR)temp);
				bTemp = bTemp && (!*temp || CheckFileSizeStringFormat(temp));

				if (!bTemp)
				{
					LONG_PTR ColorConfig = SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
					Message(MSG_WARNING,1,ColorConfig?MSG(MFileHilightTitle):MSG(MFileFilterTitle),MSG(MBadFileSizeFormat),MSG(MOk));
					return FALSE;
				}
			}

			break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

bool FileFilterConfig(FileFilterParams *FF, bool ColorConfig)
{
	const wchar_t VerticalLine[] = {BoxSymbols[BS_T_H1V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_B_H1V1],0};
	// Временная маска.
	CFileMask FileMask;
	// История для маски файлов
	const wchar_t FilterMasksHistoryName[] = L"FilterMasks";
	// История для имени фильтра
	const wchar_t FilterNameHistoryName[] = L"FilterName";
	// Маски для диалога настройки
	// Маска для ввода дней для относительной даты
	const wchar_t DaysMask[] = L"9999";
	FARString strDateMask, strTimeMask;
	// Определение параметров даты и времени в системе.
	int DateSeparator=GetDateSeparator();
	int TimeSeparator=GetTimeSeparator();
	wchar_t DecimalSeparator=GetDecimalSeparator();
	int DateFormat=GetDateFormat();

	switch (DateFormat)
	{
		case 0:
			// Маска даты для форматов DD.MM.YYYYY и MM.DD.YYYYY
			strDateMask.Format(L"99%c99%c99999",DateSeparator,DateSeparator);
			break;
		case 1:
			// Маска даты для форматов DD.MM.YYYYY и MM.DD.YYYYY
			strDateMask.Format(L"99%c99%c99999",DateSeparator,DateSeparator);
			break;
		default:
			// Маска даты для формата YYYYY.MM.DD
			strDateMask.Format(L"99999%c99%c99",DateSeparator,DateSeparator);
			break;
	}

	// Маска времени
	strTimeMask.Format(L"99%c99%c99%c999",TimeSeparator,TimeSeparator,DecimalSeparator);
	DialogDataEx FilterDlgData[]=
	{
		DI_DOUBLEBOX,3,1,76,18,0,DIF_SHOWAMPERSAND,MSG(MFileFilterTitle),

		DI_TEXT,5,2,0,2,0,DIF_FOCUS,MSG(MFileFilterName),
		DI_EDIT,5,2,74,2,(DWORD_PTR)FilterNameHistoryName,DIF_HISTORY,L"",

		DI_TEXT,0,3,0,3,0,DIF_SEPARATOR,L"",

		DI_CHECKBOX,5,4,0,4,0,DIF_AUTOMATION,MSG(MFileFilterMatchMask),
		DI_EDIT,5,4,74,4,(DWORD_PTR)FilterMasksHistoryName,DIF_HISTORY,L"",

		DI_TEXT,0,5,0,5,0,DIF_SEPARATOR,L"",

		DI_CHECKBOX,5,6,0,6,0,DIF_AUTOMATION,MSG(MFileFilterSize),
		DI_TEXT,7,7,8,7,0,0,MSG(MFileFilterSizeFromSign),
		DI_EDIT,10,7,20,7,0,0,L"",
		DI_TEXT,7,8,8,8,0,0,MSG(MFileFilterSizeToSign),
		DI_EDIT,10,8,20,8,0,0,L"",

		DI_CHECKBOX,24,6,0,6,0,DIF_AUTOMATION,MSG(MFileFilterDate),
		DI_COMBOBOX,26,7,41,7,0,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND,L"",
		DI_CHECKBOX,26,8,0,8,0,0,MSG(MFileFilterDateRelative),
		DI_TEXT,48,7,50,7,0,0,MSG(MFileFilterDateBeforeSign),
		DI_FIXEDIT,51,7,61,7,(DWORD_PTR)strDateMask.CPtr(),DIF_MASKEDIT,L"",
		DI_FIXEDIT,51,7,61,7,(DWORD_PTR)DaysMask,DIF_MASKEDIT,L"",
		DI_FIXEDIT,63,7,74,7,(DWORD_PTR)strTimeMask.CPtr(),DIF_MASKEDIT,L"",
		DI_TEXT,48,8,50,8,0,0,MSG(MFileFilterDateAfterSign),
		DI_FIXEDIT,51,8,61,8,(DWORD_PTR)strDateMask.CPtr(),DIF_MASKEDIT,L"",
		DI_FIXEDIT,51,8,61,8,(DWORD_PTR)DaysMask,DIF_MASKEDIT,L"",
		DI_FIXEDIT,63,8,74,8,(DWORD_PTR)strTimeMask.CPtr(),DIF_MASKEDIT,L"",
		DI_BUTTON,0,6,0,6,0,DIF_BTNNOCLOSE,MSG(MFileFilterCurrent),
		DI_BUTTON,0,6,74,6,0,DIF_BTNNOCLOSE,MSG(MFileFilterBlank),

		DI_TEXT,0,9,0,9,0,DIF_SEPARATOR,L"",
		DI_VTEXT,22,5,22,9,0,DIF_BOXCOLOR,VerticalLine,

		DI_CHECKBOX, 5,10,0,10,0,DIF_AUTOMATION,MSG(MFileFilterAttr),
		DI_CHECKBOX, 7,11,0,11,0,DIF_3STATE,MSG(MFileFilterAttrR),
		DI_CHECKBOX, 7,12,0,12,0,DIF_3STATE,MSG(MFileFilterAttrA),
		DI_CHECKBOX, 7,13,0,13,0,DIF_3STATE,MSG(MFileFilterAttrH),
		DI_CHECKBOX, 7,14,0,14,0,DIF_3STATE,MSG(MFileFilterAttrS),

		DI_CHECKBOX,29,11,0,11,0,DIF_3STATE,MSG(MFileFilterAttrD),
		DI_CHECKBOX,29,12,0,12,0,DIF_3STATE,MSG(MFileFilterAttrC),
		DI_CHECKBOX,29,13,0,13,0,DIF_3STATE,MSG(MFileFilterAttrE),
		DI_CHECKBOX,29,14,0,14,0,DIF_3STATE,MSG(MFileFilterAttrNI),
		DI_CHECKBOX,29,15,0,15,0,DIF_3STATE,MSG(MFileFilterAttrReparse),

		DI_CHECKBOX,51,11,0,11,0,DIF_3STATE,MSG(MFileFilterAttrSparse),
		DI_CHECKBOX,51,12,0,12,0,DIF_3STATE,MSG(MFileFilterAttrT),
		DI_CHECKBOX,51,13,0,13,0,DIF_3STATE,MSG(MFileFilterAttrOffline),
		DI_CHECKBOX,51,14,0,14,0,DIF_3STATE,MSG(MFileFilterAttrVirtual),

		DI_TEXT,-1,14,0,14,0,DIF_SEPARATOR,MSG(MHighlightColors),
		DI_TEXT,7,15,0,15,0,0,MSG(MHighlightMarkChar),
		DI_FIXEDIT,5,15,5,15,0,0,L"",
		DI_CHECKBOX,0,15,0,15,0,0,MSG(MHighlightTransparentMarkChar),

		DI_BUTTON,5,16,0,16,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightFileName1),
		DI_BUTTON,0,16,0,16,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightMarking1),
		DI_BUTTON,5,17,0,17,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightFileName2),
		DI_BUTTON,0,17,0,17,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightMarking2),
		DI_BUTTON,5,18,0,18,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightFileName3),
		DI_BUTTON,0,18,0,18,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightMarking3),
		DI_BUTTON,5,19,0,19,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightFileName4),
		DI_BUTTON,0,19,0,19,0,DIF_BTNNOCLOSE|DIF_NOBRACKETS,MSG(MHighlightMarking4),

		DI_USERCONTROL,73-15-1,16,73-2,19,0,DIF_NOFOCUS,L"",
		DI_CHECKBOX,5,20,0,20,0,0,MSG(MHighlightContinueProcessing),

		DI_TEXT,0,16,0,16,0,DIF_SEPARATOR,L"",

		DI_BUTTON,0,17,0,17,0,DIF_DEFAULT|DIF_CENTERGROUP,MSG(MOk),
		DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP|DIF_BTNNOCLOSE,MSG(MFileFilterReset),
		DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP,MSG(MFileFilterCancel),
		DI_BUTTON,0,17,0,17,0,DIF_CENTERGROUP|DIF_BTNNOCLOSE,MSG(MFileFilterMakeTransparent),
	};
	FilterDlgData[0].Data=MSG(ColorConfig?MFileHilightTitle:MFileFilterTitle);
	MakeDialogItemsEx(FilterDlgData,FilterDlg);

	if (ColorConfig)
	{
		FilterDlg[ID_FF_TITLE].Y2+=5;

		for (int i=ID_FF_NAME; i<=ID_FF_SEPARATOR1; i++)
			FilterDlg[i].Flags|=DIF_HIDDEN;

		for (int i=ID_FF_MATCHMASK; i<=ID_FF_VIRTUAL; i++)
		{
			FilterDlg[i].Y1-=2;
			FilterDlg[i].Y2-=2;
		}

		for (int i=ID_FF_SEPARATOR5; i<=ID_FF_MAKETRANSPARENT; i++)
		{
			FilterDlg[i].Y1+=5;
			FilterDlg[i].Y2+=5;
		}
	}
	else
	{
		for (int i=ID_HER_SEPARATOR1; i<=ID_HER_CONTINUEPROCESSING; i++)
			FilterDlg[i].Flags|=DIF_HIDDEN;

		FilterDlg[ID_FF_MAKETRANSPARENT].Flags=DIF_HIDDEN;
	}

	FilterDlg[ID_FF_NAMEEDIT].X1=FilterDlg[ID_FF_NAME].X1+(int)FilterDlg[ID_FF_NAME].strData.GetLength()-(FilterDlg[ID_FF_NAME].strData.Contains(L'&')?1:0)+1;
	FilterDlg[ID_FF_MASKEDIT].X1=FilterDlg[ID_FF_MATCHMASK].X1+(int)FilterDlg[ID_FF_MATCHMASK].strData.GetLength()-(FilterDlg[ID_FF_MATCHMASK].strData.Contains(L'&')?1:0)+5;
	FilterDlg[ID_FF_BLANK].X1=FilterDlg[ID_FF_BLANK].X2-(int)FilterDlg[ID_FF_BLANK].strData.GetLength()+(FilterDlg[ID_FF_BLANK].strData.Contains(L'&')?1:0)-3;
	FilterDlg[ID_FF_CURRENT].X2=FilterDlg[ID_FF_BLANK].X1-2;
	FilterDlg[ID_FF_CURRENT].X1=FilterDlg[ID_FF_CURRENT].X2-(int)FilterDlg[ID_FF_CURRENT].strData.GetLength()+(FilterDlg[ID_FF_CURRENT].strData.Contains(L'&')?1:0)-3;
	FilterDlg[ID_HER_MARKTRANSPARENT].X1=FilterDlg[ID_HER_MARK_TITLE].X1+(int)FilterDlg[ID_HER_MARK_TITLE].strData.GetLength()-(FilterDlg[ID_HER_MARK_TITLE].strData.Contains(L'&')?1:0)+1;

	for (int i=ID_HER_NORMALMARKING; i<=ID_HER_SELECTEDCURSORMARKING; i+=2)
		FilterDlg[i].X1=FilterDlg[ID_HER_NORMALFILE].X1+(int)FilterDlg[ID_HER_NORMALFILE].strData.GetLength()-(FilterDlg[ID_HER_NORMALFILE].strData.Contains(L'&')?1:0)+1;

	CHAR_INFO VBufColorExample[15*4]={0};
	HighlightDataColor Colors;
	FF->GetColors(&Colors);
	HighlightDlgUpdateUserControl(VBufColorExample,Colors);
	FilterDlg[ID_HER_COLOREXAMPLE].VBuf=VBufColorExample;
	wchar_t MarkChar[] = {(wchar_t)Colors.MarkChar&0x0000FFFF, 0};
	FilterDlg[ID_HER_MARKEDIT].strData=MarkChar;
	FilterDlg[ID_HER_MARKTRANSPARENT].Selected=(Colors.MarkChar&0xFF0000?1:0);
	FilterDlg[ID_HER_CONTINUEPROCESSING].Selected=(FF->GetContinueProcessing()?1:0);
	FilterDlg[ID_FF_NAMEEDIT].strData = FF->GetTitle();
	const wchar_t *FMask;
	FilterDlg[ID_FF_MATCHMASK].Selected=FF->GetMask(&FMask)?1:0;
	FilterDlg[ID_FF_MASKEDIT].strData=FMask;

	if (!FilterDlg[ID_FF_MATCHMASK].Selected)
		FilterDlg[ID_FF_MASKEDIT].Flags|=DIF_DISABLE;

	const wchar_t *SizeAbove, *SizeBelow;
	FilterDlg[ID_FF_MATCHSIZE].Selected=FF->GetSize(&SizeAbove,&SizeBelow)?1:0;
	FilterDlg[ID_FF_SIZEFROMEDIT].strData=SizeAbove;
	FilterDlg[ID_FF_SIZETOEDIT].strData=SizeBelow;

	if (!FilterDlg[ID_FF_MATCHSIZE].Selected)
		for (int i=ID_FF_SIZEFROMSIGN; i <= ID_FF_SIZETOEDIT; i++)
			FilterDlg[i].Flags|=DIF_DISABLE;

	// Лист для комбобокса времени файла
	FarList DateList;
	FarListItem TableItemDate[FDATE_COUNT]={0};
	// Настройка списка типов дат файла
	DateList.Items=TableItemDate;
	DateList.ItemsNumber=FDATE_COUNT;

	for (int i=0; i < FDATE_COUNT; ++i)
		TableItemDate[i].Text=MSG(MFileFilterWrited+i);

	DWORD DateType;
	FILETIME DateAfter, DateBefore;
	bool bRelative;
	FilterDlg[ID_FF_MATCHDATE].Selected=FF->GetDate(&DateType,&DateAfter,&DateBefore,&bRelative)?1:0;
	FilterDlg[ID_FF_DATERELATIVE].Selected=bRelative?1:0;
	FilterDlg[ID_FF_DATETYPE].ListItems=&DateList;
	TableItemDate[DateType].Flags=LIF_SELECTED;

	if (bRelative)
	{
		ConvertRelativeDate(DateAfter, FilterDlg[ID_FF_DAYSAFTEREDIT].strData, FilterDlg[ID_FF_TIMEAFTEREDIT].strData);
		ConvertRelativeDate(DateBefore, FilterDlg[ID_FF_DAYSBEFOREEDIT].strData, FilterDlg[ID_FF_TIMEBEFOREEDIT].strData);
	}
	else
	{
		ConvertDate(DateAfter,FilterDlg[ID_FF_DATEAFTEREDIT].strData,FilterDlg[ID_FF_TIMEAFTEREDIT].strData,12,FALSE,FALSE,2);
		ConvertDate(DateBefore,FilterDlg[ID_FF_DATEBEFOREEDIT].strData,FilterDlg[ID_FF_TIMEBEFOREEDIT].strData,12,FALSE,FALSE,2);
	}

	if (!FilterDlg[ID_FF_MATCHDATE].Selected)
		for (int i=ID_FF_DATETYPE; i <= ID_FF_BLANK; i++)
			FilterDlg[i].Flags|=DIF_DISABLE;

	DWORD AttrSet, AttrClear;
	FilterDlg[ID_FF_MATCHATTRIBUTES].Selected=FF->GetAttr(&AttrSet,&AttrClear)?1:0;
	FilterDlg[ID_FF_READONLY].Selected=(AttrSet & FILE_ATTRIBUTE_READONLY?1:AttrClear & FILE_ATTRIBUTE_READONLY?0:2);
	FilterDlg[ID_FF_ARCHIVE].Selected=(AttrSet & FILE_ATTRIBUTE_ARCHIVE?1:AttrClear & FILE_ATTRIBUTE_ARCHIVE?0:2);
	FilterDlg[ID_FF_HIDDEN].Selected=(AttrSet & FILE_ATTRIBUTE_HIDDEN?1:AttrClear & FILE_ATTRIBUTE_HIDDEN?0:2);
	FilterDlg[ID_FF_SYSTEM].Selected=(AttrSet & FILE_ATTRIBUTE_SYSTEM?1:AttrClear & FILE_ATTRIBUTE_SYSTEM?0:2);
	FilterDlg[ID_FF_COMPRESSED].Selected=(AttrSet & FILE_ATTRIBUTE_COMPRESSED?1:AttrClear & FILE_ATTRIBUTE_COMPRESSED?0:2);
	FilterDlg[ID_FF_ENCRYPTED].Selected=(AttrSet & FILE_ATTRIBUTE_ENCRYPTED?1:AttrClear & FILE_ATTRIBUTE_ENCRYPTED?0:2);
	FilterDlg[ID_FF_DIRECTORY].Selected=(AttrSet & FILE_ATTRIBUTE_DIRECTORY?1:AttrClear & FILE_ATTRIBUTE_DIRECTORY?0:2);
	FilterDlg[ID_FF_NOTINDEXED].Selected=(AttrSet & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED?1:AttrClear & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED?0:2);
	FilterDlg[ID_FF_SPARSE].Selected=(AttrSet & FILE_ATTRIBUTE_SPARSE_FILE?1:AttrClear & FILE_ATTRIBUTE_SPARSE_FILE?0:2);
	FilterDlg[ID_FF_TEMP].Selected=(AttrSet & FILE_ATTRIBUTE_TEMPORARY?1:AttrClear & FILE_ATTRIBUTE_TEMPORARY?0:2);
	FilterDlg[ID_FF_REPARSEPOINT].Selected=(AttrSet & FILE_ATTRIBUTE_REPARSE_POINT?1:AttrClear & FILE_ATTRIBUTE_REPARSE_POINT?0:2);
	FilterDlg[ID_FF_OFFLINE].Selected=(AttrSet & FILE_ATTRIBUTE_OFFLINE?1:AttrClear & FILE_ATTRIBUTE_OFFLINE?0:2);
	FilterDlg[ID_FF_VIRTUAL].Selected=(AttrSet & FILE_ATTRIBUTE_VIRTUAL?1:AttrClear & FILE_ATTRIBUTE_VIRTUAL?0:2);

	if (!FilterDlg[ID_FF_MATCHATTRIBUTES].Selected)
	{
		for (int i=ID_FF_READONLY; i <= ID_FF_VIRTUAL; i++)
			FilterDlg[i].Flags|=DIF_DISABLE;
	}

	Dialog Dlg(FilterDlg,ARRAYSIZE(FilterDlg),FileFilterConfigDlgProc,(LONG_PTR)(ColorConfig?&Colors:nullptr));
	Dlg.SetHelp(ColorConfig?L"HighlightEdit":L"Filter");
	Dlg.SetPosition(-1,-1,FilterDlg[ID_FF_TITLE].X2+4,FilterDlg[ID_FF_TITLE].Y2+2);
	Dlg.SetAutomation(ID_FF_MATCHMASK,ID_FF_MASKEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE,ID_FF_SIZEFROMSIGN,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE,ID_FF_SIZEFROMEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE,ID_FF_SIZETOSIGN,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE,ID_FF_SIZETOEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATETYPE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATERELATIVE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATEAFTERSIGN,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATEAFTEREDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DAYSAFTEREDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_TIMEAFTEREDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATEBEFORESIGN,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DATEBEFOREEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_DAYSBEFOREEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_TIMEBEFOREEDIT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_CURRENT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE,ID_FF_BLANK,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_READONLY,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_ARCHIVE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_HIDDEN,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_SYSTEM,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_COMPRESSED,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_ENCRYPTED,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_NOTINDEXED,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_SPARSE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_TEMP,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_REPARSEPOINT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_OFFLINE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_VIRTUAL,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES,ID_FF_DIRECTORY,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);

	for (;;)
	{
		Dlg.ClearDone();
		Dlg.Process();
		int ExitCode=Dlg.GetExitCode();

		if (ExitCode==ID_FF_OK) // Ok
		{
			// Если введённая пользователем маска не корректна, тогда вернёмся в диалог
			if (FilterDlg[ID_FF_MATCHMASK].Selected && !FileMask.Set(FilterDlg[ID_FF_MASKEDIT].strData,0))
				continue;

			if (FilterDlg[ID_HER_MARKTRANSPARENT].Selected)
				Colors.MarkChar|=0x00FF0000;
			else
				Colors.MarkChar&=0x0000FFFF;

			FF->SetColors(&Colors);
			FF->SetContinueProcessing(FilterDlg[ID_HER_CONTINUEPROCESSING].Selected!=0);
			FF->SetTitle(FilterDlg[ID_FF_NAMEEDIT].strData);
			FF->SetMask(FilterDlg[ID_FF_MATCHMASK].Selected!=0,
			            FilterDlg[ID_FF_MASKEDIT].strData);
			FF->SetSize(FilterDlg[ID_FF_MATCHSIZE].Selected!=0,
			            FilterDlg[ID_FF_SIZEFROMEDIT].strData,
			            FilterDlg[ID_FF_SIZETOEDIT].strData);
			bRelative = FilterDlg[ID_FF_DATERELATIVE].Selected!=0;

			LPWSTR TimeBefore = FilterDlg[ID_FF_TIMEBEFOREEDIT].strData.GetBuffer();
			TimeBefore[8] = TimeSeparator;
			FilterDlg[ID_FF_TIMEBEFOREEDIT].strData.ReleaseBuffer(FilterDlg[ID_FF_TIMEBEFOREEDIT].strData.GetLength());

			LPWSTR TimeAfter = FilterDlg[ID_FF_TIMEAFTEREDIT].strData.GetBuffer();
			TimeAfter[8] = TimeSeparator;
			FilterDlg[ID_FF_TIMEAFTEREDIT].strData.ReleaseBuffer(FilterDlg[ID_FF_TIMEAFTEREDIT].strData.GetLength());

			StrToDateTime(FilterDlg[bRelative?ID_FF_DAYSAFTEREDIT:ID_FF_DATEAFTEREDIT].strData,FilterDlg[ID_FF_TIMEAFTEREDIT].strData,DateAfter,DateFormat,DateSeparator,TimeSeparator,bRelative);
			StrToDateTime(FilterDlg[bRelative?ID_FF_DAYSBEFOREEDIT:ID_FF_DATEBEFOREEDIT].strData,FilterDlg[ID_FF_TIMEBEFOREEDIT].strData,DateBefore,DateFormat,DateSeparator,TimeSeparator,bRelative);
			FF->SetDate(FilterDlg[ID_FF_MATCHDATE].Selected!=0,
			            FilterDlg[ID_FF_DATETYPE].ListPos,
			            DateAfter,
			            DateBefore,
			            bRelative);
			AttrSet=0;
			AttrClear=0;
			AttrSet|=(FilterDlg[ID_FF_READONLY].Selected==1?FILE_ATTRIBUTE_READONLY:0);
			AttrSet|=(FilterDlg[ID_FF_ARCHIVE].Selected==1?FILE_ATTRIBUTE_ARCHIVE:0);
			AttrSet|=(FilterDlg[ID_FF_HIDDEN].Selected==1?FILE_ATTRIBUTE_HIDDEN:0);
			AttrSet|=(FilterDlg[ID_FF_SYSTEM].Selected==1?FILE_ATTRIBUTE_SYSTEM:0);
			AttrSet|=(FilterDlg[ID_FF_COMPRESSED].Selected==1?FILE_ATTRIBUTE_COMPRESSED:0);
			AttrSet|=(FilterDlg[ID_FF_ENCRYPTED].Selected==1?FILE_ATTRIBUTE_ENCRYPTED:0);
			AttrSet|=(FilterDlg[ID_FF_DIRECTORY].Selected==1?FILE_ATTRIBUTE_DIRECTORY:0);
			AttrSet|=(FilterDlg[ID_FF_NOTINDEXED].Selected==1?FILE_ATTRIBUTE_NOT_CONTENT_INDEXED:0);
			AttrSet|=(FilterDlg[ID_FF_SPARSE].Selected==1?FILE_ATTRIBUTE_SPARSE_FILE:0);
			AttrSet|=(FilterDlg[ID_FF_TEMP].Selected==1?FILE_ATTRIBUTE_TEMPORARY:0);
			AttrSet|=(FilterDlg[ID_FF_REPARSEPOINT].Selected==1?FILE_ATTRIBUTE_REPARSE_POINT:0);
			AttrSet|=(FilterDlg[ID_FF_OFFLINE].Selected==1?FILE_ATTRIBUTE_OFFLINE:0);
			AttrSet|=(FilterDlg[ID_FF_VIRTUAL].Selected==1?FILE_ATTRIBUTE_VIRTUAL:0);
			AttrClear|=(FilterDlg[ID_FF_READONLY].Selected==0?FILE_ATTRIBUTE_READONLY:0);
			AttrClear|=(FilterDlg[ID_FF_ARCHIVE].Selected==0?FILE_ATTRIBUTE_ARCHIVE:0);
			AttrClear|=(FilterDlg[ID_FF_HIDDEN].Selected==0?FILE_ATTRIBUTE_HIDDEN:0);
			AttrClear|=(FilterDlg[ID_FF_SYSTEM].Selected==0?FILE_ATTRIBUTE_SYSTEM:0);
			AttrClear|=(FilterDlg[ID_FF_COMPRESSED].Selected==0?FILE_ATTRIBUTE_COMPRESSED:0);
			AttrClear|=(FilterDlg[ID_FF_ENCRYPTED].Selected==0?FILE_ATTRIBUTE_ENCRYPTED:0);
			AttrClear|=(FilterDlg[ID_FF_DIRECTORY].Selected==0?FILE_ATTRIBUTE_DIRECTORY:0);
			AttrClear|=(FilterDlg[ID_FF_NOTINDEXED].Selected==0?FILE_ATTRIBUTE_NOT_CONTENT_INDEXED:0);
			AttrClear|=(FilterDlg[ID_FF_SPARSE].Selected==0?FILE_ATTRIBUTE_SPARSE_FILE:0);
			AttrClear|=(FilterDlg[ID_FF_TEMP].Selected==0?FILE_ATTRIBUTE_TEMPORARY:0);
			AttrClear|=(FilterDlg[ID_FF_REPARSEPOINT].Selected==0?FILE_ATTRIBUTE_REPARSE_POINT:0);
			AttrClear|=(FilterDlg[ID_FF_OFFLINE].Selected==0?FILE_ATTRIBUTE_OFFLINE:0);
			AttrClear|=(FilterDlg[ID_FF_VIRTUAL].Selected==0?FILE_ATTRIBUTE_VIRTUAL:0);
			FF->SetAttr(FilterDlg[ID_FF_MATCHATTRIBUTES].Selected!=0,
			            AttrSet,
			            AttrClear);
			return true;
		}
		else
			break;
	}

	return false;
}
