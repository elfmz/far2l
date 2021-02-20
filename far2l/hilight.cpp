/*
hilight.cpp

Files highlighting
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


#include "colors.hpp"
#include "hilight.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "registry.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "config.hpp"
#include "interf.hpp"
#include "KeyFileHelper.h"

static const struct HighlightStrings
{
	const char *UseAttr,*IncludeAttributes,*ExcludeAttributes,*AttrSet,*AttrClear,
	*IgnoreMask,*UseMask,*Mask,
	*NormalColor,*SelectedColor,*CursorColor,*SelectedCursorColor,
	*MarkCharNormalColor,*MarkCharSelectedColor,*MarkCharCursorColor,*MarkCharSelectedCursorColor,
	*MarkChar,
	*ContinueProcessing,
	*UseDate,*DateType,*DateAfter,*DateBefore,*DateRelative,
	*UseSize,*SizeAbove,*SizeBelow;
//	*HighlightEdit,*HighlightList;
} HLS=
{
	"UseAttr", "IncludeAttributes", "ExcludeAttributes", "AttrSet", "AttrClear",
	"IgnoreMask", "UseMask", "Mask",
	"NormalColor", "SelectedColor", "CursorColor", "SelectedCursorColor",
	"MarkCharNormalColor", "MarkCharSelectedColor", "MarkCharCursorColor", "MarkCharSelectedCursorColor",
	"MarkChar",
	"ContinueProcessing",
	"UseDate", "DateType", "DateAfter", "DateBefore", "DateRelative",
	"UseSize", "SizeAboveS", "SizeBelowS"
//	"HighlightEdit", "HighlightList"
};

static const char fmtFirstGroup[]= "Group%d";
static const char fmtUpperGroup[]= "UpperGroup%d";
static const char fmtLowerGroup[]= "LowerGroup%d";
static const char fmtLastGroup[]= "LastGroup%d";
static const char SortGroupsKeyName[]= "SortGroups";
static const char HighlightKeyName[]= "Highlight";
static const char RegColorsHighlight[]= "Colors/Highlight";

static std::string HighlightIni()
{
	return InMyConfig("highlight.ini");
}

static std::string SortgroupsIni()
{
	return InMyConfig("sortgroups.ini");
}


static void SetDefaultHighlighting()
{
	static const wchar_t *Masks[]=
	{
		/* 0 */ L"*.*",
		/* 1 */ L"*.rar,*.zip,*.[zj],*.[bxg7]z,*.[bg]zip,*.tar,*.t[agbx]z,*.ar[cj],*.r[0-9][0-9],*.a[0-9][0-9],*.bz2,*.cab,*.msi,*.jar,*.lha,*.lzh,*.ha,*.ac[bei],*.pa[ck],*.rk,*.cpio,*.rpm,*.zoo,*.hqx,*.sit,*.ice,*.uc2,*.ain,*.imp,*.777,*.ufa,*.boa,*.bs[2a],*.sea,*.hpk,*.ddi,*.x2,*.rkv,*.[lw]sz,*.h[ay]p,*.lim,*.sqz,*.chz",
		/* 2 */ L"*.bak,*.tmp",                                                                                                                                                                                //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ -> может к терапевту? ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		/* $ 25.09.2001  IS
		    Эта маска для каталогов: обрабатывать все каталоги, кроме тех, что
		    являются родительскими (их имена - две точки).
		*/
		/* 3 */ L"*.*|..", // маска для каталогов
		/* 4 */ L"..",     // такие каталоги окрашивать как простые файлы
	};
	static struct DefaultData
	{
		const wchar_t *Mask;
		int IgnoreMask;
		DWORD IncludeAttr;
		BYTE NormalColor;
		BYTE CursorColor;
	}
	StdHighlightData[]=
	    { /*
             Mask                NormalColor
                          IncludeAttributes
                       IgnoreMask       CursorColor             */
	        /* 7 */{Masks[0], 1, FILE_ATTRIBUTE_BROKEN, 0x10 | F_LIGHTRED, 0x30 | F_LIGHTRED }, 
	        /* 0 */{Masks[0], 0, 0x0002, 0x13, 0x38},
	        /* 1 */{Masks[0], 0, 0x0004, 0x13, 0x38},
	        /* 2 */{Masks[3], 0, 0x0010, 0x1F, 0x3F},
	        /* 3 */{Masks[4], 0, 0x0010, 0x00, 0x00},
	        /* 4 */{L"*.sh,*.py,*.pl,*.cmd,*.exe,*.bat,*.com",0, 0x0000, 0x1A, 0x3A},
	        /* 5 */{Masks[1], 0, 0x0000, 0x1D, 0x3D},
	        /* 6 */{Masks[2], 0, 0x0000, 0x16, 0x36},
	        // это настройка для каталогов на тех панелях, которые должны раскрашиваться
	        // без учета масок (например, список хостов в "far navigator")
	        /* 7 */{Masks[0], 1, FILE_ATTRIBUTE_EXECUTABLE | FILE_ATTRIBUTE_REPARSE_POINT, 0x10 | F_GREEN, 0x30 | F_GREEN }, 
	        /* 7 */{Masks[0], 1, FILE_ATTRIBUTE_DIRECTORY, 0x10 | F_WHITE, 0x30 | F_WHITE},
	        /* 7 */{Masks[0], 1, FILE_ATTRIBUTE_EXECUTABLE, 0x10 | F_LIGHTGREEN, 0x30 | F_LIGHTGREEN}, 
	    };


	KeyFileHelper kfh(HighlightIni().c_str());
	for (size_t I=0; I < ARRAYSIZE(StdHighlightData); I++)
	{
		const std::string &Section = StrPrintf("Group%d", I);
		kfh.PutString(Section.c_str(), HLS.Mask, StdHighlightData[I].Mask);
		kfh.PutUInt(Section.c_str(), HLS.IgnoreMask, StdHighlightData[I].IgnoreMask);
		kfh.PutUInt(Section.c_str(), HLS.IncludeAttributes, StdHighlightData[I].IncludeAttr);
		kfh.PutUInt(Section.c_str(), HLS.NormalColor, StdHighlightData[I].NormalColor);
		kfh.PutUInt(Section.c_str(), HLS.CursorColor, StdHighlightData[I].CursorColor);
	}
}

HighlightFiles::HighlightFiles()
{
	struct stat s{};

	if (stat(HighlightIni().c_str(), &s) == 0) {
		InitHighlightFiles();

	} else if (CheckRegKey(FARString(RegColorsHighlight))) {
		InitHighlightFilesFromReg();
		SaveHiData();

	} else {
		SetDefaultHighlighting();
		InitHighlightFiles();
	}

	UpdateCurrentTime();
}

static void LoadFilter(FileFilterParams *HData,
		const KeyFileValues *Values, const std::string &Mask, int SortGroup, bool bSortGroup)
{
	//Дефолтные значения выбраны так чтоб как можно правильней загрузить
	//настройки старых версий фара.
	if (bSortGroup)
		HData->SetMask(Values->GetUInt(HLS.UseMask, 1) != 0, FARString(Mask));
	else
		HData->SetMask(Values->GetUInt(HLS.IgnoreMask, 0) == 0, FARString(Mask));

	FILETIME DateAfter{}, DateBefore{};
	Values->GetBytes(HLS.DateAfter, sizeof(DateAfter), (unsigned char *)&DateAfter);
	Values->GetBytes(HLS.DateBefore, sizeof(DateBefore), (unsigned char *)&DateBefore);
	HData->SetDate(Values->GetUInt(HLS.UseDate, 0) != 0,
					(DWORD)Values->GetUInt(HLS.DateType, 0),
					DateAfter, DateBefore,
					Values->GetUInt(HLS.DateRelative, 0) != 0);
	FARString strSizeAbove(Values->GetString(HLS.SizeAbove, L""));
	FARString strSizeBelow(Values->GetString(HLS.SizeBelow, L""));
	HData->SetSize(Values->GetUInt(HLS.UseSize, 0) != 0, strSizeAbove, strSizeBelow);

	if (bSortGroup)
	{
		HData->SetAttr(Values->GetUInt(HLS.UseAttr, 1) != 0,
						(DWORD)Values->GetUInt(HLS.AttrSet, 0),
						(DWORD)Values->GetUInt(HLS.AttrClear, FILE_ATTRIBUTE_DIRECTORY));
	}
	else
	{
		HData->SetAttr(Values->GetUInt(HLS.UseAttr, 1) != 0,
						(DWORD)Values->GetUInt(HLS.IncludeAttributes, 0),
						(DWORD)Values->GetUInt(HLS.ExcludeAttributes, 0));
	}

	HData->SetSortGroup(SortGroup);
	HighlightDataColor Colors;
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL] = (WORD)Values->GetUInt(HLS.NormalColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED] = (WORD)Values->GetUInt(HLS.SelectedColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR] = (WORD)Values->GetUInt(HLS.CursorColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = (WORD)Values->GetUInt(HLS.SelectedCursorColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_NORMAL] = (WORD)Values->GetUInt(HLS.MarkCharNormalColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTED] = (WORD)Values->GetUInt(HLS.MarkCharSelectedColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_UNDERCURSOR] = (WORD)Values->GetUInt(HLS.MarkCharCursorColor, 0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = (WORD)Values->GetUInt(HLS.MarkCharSelectedCursorColor, 0);
	Colors.MarkChar = Values->GetUInt(HLS.MarkChar, 0);
	HData->SetColors(&Colors);
	HData->SetContinueProcessing(Values->GetUInt(HLS.ContinueProcessing, 0) != 0);
}


/// TODO: Remove this function and its invokation after 2022/02/20
static void LoadFilterFromReg(FileFilterParams *HData, const wchar_t *RegKey, const wchar_t *Mask, int SortGroup, bool bSortGroup)
{
	//Дефолтные значения выбраны так чтоб как можно правильней загрузить
	//настройки старых версий фара.
	if (bSortGroup)
		HData->SetMask(GetRegKey(RegKey,FARString(HLS.UseMask),1)!=0, Mask);
	else
		HData->SetMask(!GetRegKey(RegKey,FARString(HLS.IgnoreMask),0), Mask);
	FILETIME DateAfter, DateBefore;
	GetRegKey(RegKey,FARString(HLS.DateAfter),(BYTE *)&DateAfter,nullptr,sizeof(DateAfter));
	GetRegKey(RegKey,FARString(HLS.DateBefore),(BYTE *)&DateBefore,nullptr,sizeof(DateBefore));
	HData->SetDate(GetRegKey(RegKey,FARString(HLS.UseDate),0)!=0,
	               (DWORD)GetRegKey(RegKey,FARString(HLS.DateType),0),
	               DateAfter, DateBefore,
	               GetRegKey(RegKey,FARString(HLS.DateRelative),0)!=0);
	FARString strSizeAbove;
	FARString strSizeBelow;
	GetRegKey(RegKey,FARString(HLS.SizeAbove),strSizeAbove,L"");
	GetRegKey(RegKey,FARString(HLS.SizeBelow),strSizeBelow,L"");
	HData->SetSize(GetRegKey(RegKey,FARString(HLS.UseSize),0)!=0, strSizeAbove, strSizeBelow);

	if (bSortGroup)
	{
		HData->SetAttr(GetRegKey(RegKey,FARString(HLS.UseAttr),1)!=0,
		               (DWORD)GetRegKey(RegKey,FARString(HLS.AttrSet),0),
		               (DWORD)GetRegKey(RegKey,FARString(HLS.AttrClear),FILE_ATTRIBUTE_DIRECTORY));
	}
	else
	{
		HData->SetAttr(GetRegKey(RegKey,FARString(HLS.UseAttr),1)!=0,
		               (DWORD)GetRegKey(RegKey,FARString(HLS.IncludeAttributes),0),
		               (DWORD)GetRegKey(RegKey,FARString(HLS.ExcludeAttributes),0));
	}

	HData->SetSortGroup(SortGroup);
	HighlightDataColor Colors;
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]=(WORD)GetRegKey(RegKey,FARString(HLS.NormalColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]=(WORD)GetRegKey(RegKey,FARString(HLS.SelectedColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]=(WORD)GetRegKey(RegKey,FARString(HLS.CursorColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]=(WORD)GetRegKey(RegKey,FARString(HLS.SelectedCursorColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_NORMAL]=(WORD)GetRegKey(RegKey,FARString(HLS.MarkCharNormalColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTED]=(WORD)GetRegKey(RegKey,FARString(HLS.MarkCharSelectedColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_UNDERCURSOR]=(WORD)GetRegKey(RegKey,FARString(HLS.MarkCharCursorColor),0);
	Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]=(WORD)GetRegKey(RegKey,FARString(HLS.MarkCharSelectedCursorColor),0);
	Colors.MarkChar=GetRegKey(RegKey,FARString(HLS.MarkChar),0);
	HData->SetColors(&Colors);
	HData->SetContinueProcessing(GetRegKey(RegKey,FARString(HLS.ContinueProcessing),0)!=0);
}

void HighlightFiles::InitHighlightFiles()
{
	KeyFileReadHelper kfh_highlight(HighlightIni().c_str());
	KeyFileReadHelper kfh_sortgroups(SortgroupsIni().c_str());

	const int GroupDelta[4] = {DEFAULT_SORT_GROUP, 0, DEFAULT_SORT_GROUP + 1, DEFAULT_SORT_GROUP};
	const char *GroupNames[4] = {fmtFirstGroup, fmtUpperGroup, fmtLowerGroup, fmtLastGroup};
	int *Count[4] = {&FirstCount, &UpperCount, &LowerCount, &LastCount};
	HiData.Free();
	FirstCount = UpperCount = LowerCount = LastCount=0;
			
	std::unique_ptr<KeyFileValues> emptyValues;
	for (int j=0; j<4; j++)
	{
		KeyFileReadHelper &kfh = (j == 1 || j == 2) ? kfh_sortgroups : kfh_highlight;
		for (int i=0;; i++)
		{
			std::string strGroupName = StrPrintf(GroupNames[j], i), strMask;
			const KeyFileValues *Values = kfh.GetSectionValues(strGroupName.c_str());
			if (!Values)
			{
				if (!emptyValues)
					emptyValues.reset(new KeyFileValues);
				Values = emptyValues.get();
			}

			if (GroupDelta[j] != DEFAULT_SORT_GROUP)
			{
				if (!kfh.HasKey(".", strGroupName.c_str()))
					break;
				strMask = kfh.GetString(".", strGroupName.c_str(), "");
			}
			else
			{
				if (!Values->HasKey(HLS.Mask))
					break;
				strMask = Values->GetString(HLS.Mask, "");
			}
			FileFilterParams *HData = HiData.addItem();

			if (!HData)
				break;

			LoadFilter(HData, Values, strMask,
				GroupDelta[j] + (GroupDelta[j] == DEFAULT_SORT_GROUP ? 0 : i),
				(GroupDelta[j] == DEFAULT_SORT_GROUP ? false : true));
			(*(Count[j]))++;
		}
	}
}

void HighlightFiles::InitHighlightFilesFromReg()
{
	FARString strRegKey, strGroupName, strMask;
	const int GroupDelta[4]={DEFAULT_SORT_GROUP,0,DEFAULT_SORT_GROUP+1,DEFAULT_SORT_GROUP};
	const char *KeyNames[4]={RegColorsHighlight,SortGroupsKeyName,SortGroupsKeyName,RegColorsHighlight};
	const char *GroupNames[4]={fmtFirstGroup,fmtUpperGroup,fmtLowerGroup,fmtLastGroup};
	int  *Count[4] = {&FirstCount,&UpperCount,&LowerCount,&LastCount};
	HiData.Free();
	FirstCount=UpperCount=LowerCount=LastCount=0;
			
	for (int j=0; j<4; j++)
	{
		for (int i=0;; i++)
		{
			strGroupName.Format(FARString(GroupNames[j]), i);
			strRegKey=KeyNames[j];
			strRegKey+=L"/"+strGroupName;
			if (GroupDelta[j]!=DEFAULT_SORT_GROUP)
			{
				if (!GetRegKey(FARString(KeyNames[j]),strGroupName,strMask,L""))
					break;
			}
			else
			{
				if (!GetRegKey(strRegKey,FARString(HLS.Mask),strMask,L""))
					break;
			}
			FileFilterParams *HData = HiData.addItem();

			if (HData)
			{
				LoadFilterFromReg(HData,strRegKey,strMask,GroupDelta[j]+(GroupDelta[j]==DEFAULT_SORT_GROUP?0:i),(GroupDelta[j]==DEFAULT_SORT_GROUP?false:true));
				(*(Count[j]))++;
			}
			else
				break;
		}
	}
}


HighlightFiles::~HighlightFiles()
{
	ClearData();
}

void HighlightFiles::ClearData()
{
	HiData.Free();
	FirstCount=UpperCount=LowerCount=LastCount=0;
}

static const DWORD FarColor[] = {COL_PANELTEXT,COL_PANELSELECTEDTEXT,COL_PANELCURSOR,COL_PANELSELECTEDCURSOR};

void ApplyDefaultStartingColors(HighlightDataColor *Colors)
{
	for (int j=0; j<2; j++)
		for (int i=0; i<4; i++)
			Colors->Color[j][i]=0xFF00;

	Colors->MarkChar=0x00FF0000;
}

void ApplyBlackOnBlackColors(HighlightDataColor *Colors)
{
	for (int i=0; i<4; i++)
	{
		//Применим black on black.
		//Для файлов возьмем цвета панели не изменяя прозрачность.
		//Для пометки возьмем цвета файла включая прозрачность.
		if (!(Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]&0x00FF))
			Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]=(Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i]&0xFF00)|(0x00FF&Palette[FarColor[i]-COL_FIRSTPALETTECOLOR]);

		if (!(Colors->Color[HIGHLIGHTCOLORTYPE_MARKCHAR][i]&0x00FF))
			Colors->Color[HIGHLIGHTCOLORTYPE_MARKCHAR][i]=Colors->Color[HIGHLIGHTCOLORTYPE_FILE][i];
	}
}

void ApplyColors(HighlightDataColor *DestColors, HighlightDataColor *SrcColors)
{
	//Обработаем black on black чтоб наследовать правильные цвета
	//и чтоб после наследования были правильные цвета.
	ApplyBlackOnBlackColors(DestColors);
	ApplyBlackOnBlackColors(SrcColors);

	for (int j=0; j<2; j++)
	{
		for (int i=0; i<4; i++)
		{
			//Если текущие цвета в Src (fore и/или back) не прозрачные
			//то унаследуем их в Dest.
			if (!(SrcColors->Color[j][i]&0xF000))
				DestColors->Color[j][i]=(DestColors->Color[j][i]&0x0F0F)|(SrcColors->Color[j][i]&0xF0F0);

			if (!(SrcColors->Color[j][i]&0x0F00))
				DestColors->Color[j][i]=(DestColors->Color[j][i]&0xF0F0)|(SrcColors->Color[j][i]&0x0F0F);
		}
	}

	//Унаследуем пометку из Src если она не прозрачная
	if (!(SrcColors->MarkChar&0x00FF0000))
		DestColors->MarkChar=SrcColors->MarkChar;
}

/*
bool HasTransparent(HighlightDataColor *Colors)
{
  for (int j=0; j<2; j++)
    for (int i=0; i<4; i++)
      if (Colors->Color[j][i]&0xFF00)
        return true;

  if (Colors->MarkChar&0x00FF0000)
    return true;

  return false;
}
*/

void ApplyFinalColors(HighlightDataColor *Colors)
{
	//Обработаем black on black чтоб после наследования были правильные цвета.
	ApplyBlackOnBlackColors(Colors);

	for (int j=0; j<2; j++)
		for (int i=0; i<4; i++)
		{
			//Если какой то из текущих цветов (fore или back) прозрачный
			//то унаследуем соответствующий цвет с панелей.
			BYTE temp=(BYTE)((Colors->Color[j][i]&0xFF00)>>8);
			Colors->Color[j][i]=((~temp)&(BYTE)Colors->Color[j][i])|(temp&(BYTE)Palette[FarColor[i]-COL_FIRSTPALETTECOLOR]);
		}

	//Если символ пометки прозрачный то его как бы и нет вообще.
	if (Colors->MarkChar&0x00FF0000)
		Colors->MarkChar=0;

	//Параноя но случится может:
	//Обработаем black on black снова чтоб обработались унаследованые цвета.
	ApplyBlackOnBlackColors(Colors);
}

void HighlightFiles::UpdateCurrentTime()
{
	SYSTEMTIME cst;
	FILETIME cft;
	WINPORT(GetSystemTime)(&cst);
	WINPORT(SystemTimeToFileTime)(&cst, &cft);
	ULARGE_INTEGER current;
	current.u.LowPart  = cft.dwLowDateTime;
	current.u.HighPart = cft.dwHighDateTime;
	CurrentTime = current.QuadPart;
}

void HighlightFiles::GetHiColor(FileListItem **FileItem,int FileCount,bool UseAttrHighlighting)
{
	if (!FileItem || !FileCount)
		return;

	FileFilterParams *CurHiData;

	for (int FCnt=0; FCnt < FileCount; ++FCnt)
	{
		FileListItem& fli = *FileItem[FCnt];
		ApplyDefaultStartingColors(&fli.Colors);

		for (size_t i=0; i < HiData.getCount(); i++)
		{
			CurHiData = HiData.getItem(i);

			if (UseAttrHighlighting && CurHiData->GetMask(nullptr))
				continue;

			if (CurHiData->FileInFilter(fli, CurrentTime))
			{
				HighlightDataColor TempColors;
				CurHiData->GetColors(&TempColors);
				ApplyColors(&fli.Colors,&TempColors);

				if (!CurHiData->GetContinueProcessing())// || !HasTransparent(&fli->Colors))
					break;
			}
		}

		ApplyFinalColors(&fli.Colors);
	}
}

int HighlightFiles::GetGroup(const FileListItem *fli)
{
	for (int i=FirstCount; i<FirstCount+UpperCount; i++)
	{
		FileFilterParams *CurGroupData=HiData.getItem(i);

		if (CurGroupData->FileInFilter(*fli, CurrentTime))
			return(CurGroupData->GetSortGroup());
	}

	for (int i=FirstCount+UpperCount; i<FirstCount+UpperCount+LowerCount; i++)
	{
		FileFilterParams *CurGroupData=HiData.getItem(i);

		if (CurGroupData->FileInFilter(*fli, CurrentTime))
			return(CurGroupData->GetSortGroup());
	}

	return DEFAULT_SORT_GROUP;
}

void HighlightFiles::FillMenu(VMenu *HiMenu,int MenuPos)
{
	MenuItemEx HiMenuItem;
	const int Count[4][2] =
	{
		{0,                               FirstCount},
		{FirstCount,                      FirstCount+UpperCount},
		{FirstCount+UpperCount,           FirstCount+UpperCount+LowerCount},
		{FirstCount+UpperCount+LowerCount,FirstCount+UpperCount+LowerCount+LastCount}
	};
	HiMenu->DeleteItems();
	HiMenuItem.Clear();

	for (int j=0; j<4; j++)
	{
		for (int i=Count[j][0]; i<Count[j][1]; i++)
		{
			MenuString(HiMenuItem.strName,HiData.getItem(i),true);
			HiMenu->AddItem(&HiMenuItem);
		}

		HiMenuItem.strName.Clear();
		HiMenu->AddItem(&HiMenuItem);

		if (j<3)
		{
			if (!j)
				HiMenuItem.strName = MSG(MHighlightUpperSortGroup);
			else if (j==1)
				HiMenuItem.strName = MSG(MHighlightLowerSortGroup);
			else
				HiMenuItem.strName = MSG(MHighlightLastGroup);

			HiMenuItem.Flags|=LIF_SEPARATOR;
			HiMenu->AddItem(&HiMenuItem);
			HiMenuItem.Flags=0;
		}
	}

	HiMenu->SetSelectPos(MenuPos,1);
}

void HighlightFiles::ProcessGroups()
{
	for (int i=0; i<FirstCount; i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);

	for (int i=FirstCount; i<FirstCount+UpperCount; i++)
		HiData.getItem(i)->SetSortGroup(i-FirstCount);

	for (int i=FirstCount+UpperCount; i<FirstCount+UpperCount+LowerCount; i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP+1+i-FirstCount-UpperCount);

	for (int i=FirstCount+UpperCount+LowerCount; i<FirstCount+UpperCount+LowerCount+LastCount; i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);
}

int HighlightFiles::MenuPosToRealPos(int MenuPos, int **Count, bool Insert)
{
	int Pos=MenuPos;
	*Count=nullptr;
	int x = Insert ? 1 : 0;

	if (MenuPos<FirstCount+x)
	{
		*Count=&FirstCount;
	}
	else if (MenuPos>FirstCount+1 && MenuPos<FirstCount+UpperCount+2+x)
	{
		Pos=MenuPos-2;
		*Count=&UpperCount;
	}
	else if (MenuPos>FirstCount+UpperCount+3 && MenuPos<FirstCount+UpperCount+LowerCount+4+x)
	{
		Pos=MenuPos-4;
		*Count=&LowerCount;
	}
	else if (MenuPos>FirstCount+UpperCount+LowerCount+5 && MenuPos<FirstCount+UpperCount+LowerCount+LastCount+6+x)
	{
		Pos=MenuPos-6;
		*Count=&LastCount;
	}

	return Pos;
}

void HighlightFiles::HiEdit(int MenuPos)
{
	VMenu HiMenu(MSG(MHighlightTitle),nullptr,0,ScrY-4);
	HiMenu.SetHelp(L"HighlightList");
	HiMenu.SetFlags(VMENU_WRAPMODE|VMENU_SHOWAMPERSAND);
	HiMenu.SetPosition(-1,-1,0,0);
	HiMenu.SetBottomTitle(MSG(MHighlightBottom));
	FillMenu(&HiMenu,MenuPos);
	int NeedUpdate;
	Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel=CtrlObject->Cp()->RightPanel;
	HiMenu.Show();

	while (1)
	{
		while (!HiMenu.Done())
		{
			int Key=HiMenu.ReadInput();
			int SelectPos=HiMenu.GetSelectPos();
			NeedUpdate=FALSE;

			switch (Key)
			{
					/* $ 07.07.2000 IS
					  Если нажали ctrl+r, то восстановить значения по умолчанию.
					*/
				case KEY_CTRLR:

					if (Message(MSG_WARNING,2,MSG(MHighlightTitle),
					            MSG(MHighlightWarning),MSG(MHighlightAskRestore),
					            MSG(MYes),MSG(MCancel)))
						break;

					remove(HighlightIni().c_str());
					//DeleteKeyTree(RegColorsHighlight);
					SetDefaultHighlighting();
					HiMenu.Hide();
					ClearData();
					InitHighlightFiles();
					NeedUpdate=TRUE;
					break;
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);

					if (Count && RealSelectPos<(int)HiData.getCount())
					{
						const wchar_t *Mask;
						HiData.getItem(RealSelectPos)->GetMask(&Mask);

						if (Message(MSG_WARNING,2,MSG(MHighlightTitle),
						            MSG(MHighlightAskDel),Mask,
						            MSG(MDelete),MSG(MCancel)))
							break;

						HiData.deleteItem(RealSelectPos);
						(*Count)--;
						NeedUpdate=TRUE;
					}

					break;
				}
				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);

					if (Count && RealSelectPos<(int)HiData.getCount())
						if (FileFilterConfig(HiData.getItem(RealSelectPos),true))
							NeedUpdate=TRUE;

					break;
				}
				case KEY_INS: case KEY_NUMPAD0:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count,true);

					if (Count)
					{
						FileFilterParams *NewHData = HiData.insertItem(RealSelectPos);

						if (!NewHData)
							break;

						if (FileFilterConfig(NewHData,true))
						{
							(*Count)++;
							NeedUpdate=TRUE;
						}
						else
							HiData.deleteItem(RealSelectPos);
					}

					break;
				}
				case KEY_F5:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);

					if (Count && RealSelectPos<(int)HiData.getCount())
					{
						FileFilterParams *HData = HiData.insertItem(RealSelectPos);

						if (HData)
						{
							*HData = *HiData.getItem(RealSelectPos+1);
							HData->SetTitle(L"");

							if (FileFilterConfig(HData,true))
							{
								NeedUpdate=TRUE;
								(*Count)++;
							}
							else
								HiData.deleteItem(RealSelectPos);
						}
					}

					break;
				}
				case KEY_CTRLUP: case KEY_CTRLNUMPAD8:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);

					if (Count && SelectPos > 0)
					{
						if (UpperCount && RealSelectPos==FirstCount && RealSelectPos<FirstCount+UpperCount)
						{
							FirstCount++;
							UpperCount--;
							SelectPos--;
						}
						else if (LowerCount && RealSelectPos==FirstCount+UpperCount && RealSelectPos<FirstCount+UpperCount+LowerCount)
						{
							UpperCount++;
							LowerCount--;
							SelectPos--;
						}
						else if (LastCount && RealSelectPos==FirstCount+UpperCount+LowerCount)
						{
							LowerCount++;
							LastCount--;
							SelectPos--;
						}
						else
							HiData.swapItems(RealSelectPos,RealSelectPos-1);

						HiMenu.SetCheck(--SelectPos);
						NeedUpdate=TRUE;
						break;
					}

					HiMenu.ProcessInput();
					break;
				}
				case KEY_CTRLDOWN: case KEY_CTRLNUMPAD2:
				{
					int *Count=nullptr;
					int RealSelectPos=MenuPosToRealPos(SelectPos,&Count);

					if (Count && SelectPos < (int)HiMenu.GetItemCount()-2)
					{
						if (FirstCount && RealSelectPos==FirstCount-1)
						{
							FirstCount--;
							UpperCount++;
							SelectPos++;
						}
						else if (UpperCount && RealSelectPos==FirstCount+UpperCount-1)
						{
							UpperCount--;
							LowerCount++;
							SelectPos++;
						}
						else if (LowerCount && RealSelectPos==FirstCount+UpperCount+LowerCount-1)
						{
							LowerCount--;
							LastCount++;
							SelectPos++;
						}
						else
							HiData.swapItems(RealSelectPos,RealSelectPos+1);

						HiMenu.SetCheck(++SelectPos);
						NeedUpdate=TRUE;
					}

					HiMenu.ProcessInput();
					break;
				}
				default:
					HiMenu.ProcessInput();
					break;
			}

			// повторяющийся кусок!
			if (NeedUpdate)
			{
				ScrBuf.Lock(); // отменяем всякую прорисовку
				HiMenu.Hide();
				ProcessGroups();

				if (Opt.AutoSaveSetup)
					SaveHiData();

				//FrameManager->RefreshFrame(); // рефрешим
				LeftPanel->Update(UPDATE_KEEP_SELECTION);
				LeftPanel->Redraw();
				RightPanel->Update(UPDATE_KEEP_SELECTION);
				RightPanel->Redraw();
				FillMenu(&HiMenu,MenuPos=SelectPos);
				HiMenu.SetPosition(-1,-1,0,0);
				HiMenu.Show();
				ScrBuf.Unlock(); // разрешаем прорисовку
			}
		}

		if (HiMenu.Modal::GetExitCode()!=-1)
		{
			HiMenu.ClearDone();
			HiMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}

static void SaveFilter(FileFilterParams *CurHiData, KeyFileHelper &kfh, const char *Section, bool bSortGroup)
{
	if (bSortGroup)
	{
		kfh.PutUInt(Section, HLS.UseMask, CurHiData->GetMask(nullptr));
	}
	else
	{
		const wchar_t *Mask = nullptr;
		kfh.PutUInt(Section, HLS.IgnoreMask, (CurHiData->GetMask(&Mask) ? 0 : 1));
		kfh.PutString(Section, HLS.Mask, Mask);
	}

	DWORD DateType = 0;
	FILETIME DateAfter{}, DateBefore{};
	bool bRelative = false;
	kfh.PutUInt(Section, HLS.UseDate, CurHiData->GetDate(&DateType, &DateAfter, &DateBefore, &bRelative) ? 1 : 0);
	kfh.PutUInt(Section, HLS.DateType, DateType);
	kfh.PutBytes(Section, HLS.DateAfter, sizeof(DateAfter), (BYTE *)&DateAfter);
	kfh.PutBytes(Section, HLS.DateBefore, sizeof(DateBefore), (BYTE *)&DateBefore);
	kfh.PutUInt(Section, HLS.DateRelative, bRelative ? 1 : 0);
	const wchar_t *SizeAbove = nullptr, *SizeBelow = nullptr;
	kfh.PutUInt(Section, HLS.UseSize, CurHiData->GetSize(&SizeAbove, &SizeBelow) ? 1 : 0);
	kfh.PutString(Section, HLS.SizeAbove, SizeAbove);
	kfh.PutString(Section, HLS.SizeBelow, SizeBelow);
	DWORD AttrSet = 0, AttrClear = 0;
	kfh.PutUInt(Section, HLS.UseAttr, CurHiData->GetAttr(&AttrSet, &AttrClear) ? 1 : 0);
	kfh.PutUIntAsHex(Section, bSortGroup ? HLS.AttrSet : HLS.IncludeAttributes, AttrSet);
	kfh.PutUIntAsHex(Section, bSortGroup ? HLS.AttrClear : HLS.ExcludeAttributes, AttrClear);
	HighlightDataColor Colors;
	CurHiData->GetColors(&Colors);
	kfh.PutUIntAsHex(Section, HLS.NormalColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]);
	kfh.PutUIntAsHex(Section, HLS.SelectedColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]);
	kfh.PutUIntAsHex(Section, HLS.CursorColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]);
	kfh.PutUIntAsHex(Section, HLS.SelectedCursorColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
	kfh.PutUIntAsHex(Section, HLS.MarkCharNormalColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_NORMAL]);
	kfh.PutUIntAsHex(Section, HLS.MarkCharSelectedColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTED]);
	kfh.PutUIntAsHex(Section, HLS.MarkCharCursorColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_UNDERCURSOR]);
	kfh.PutUIntAsHex(Section, HLS.MarkCharSelectedCursorColor, (DWORD)Colors.Color[HIGHLIGHTCOLORTYPE_MARKCHAR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
	kfh.PutUIntAsHex(Section, HLS.MarkChar, Colors.MarkChar);
	kfh.PutUInt(Section, HLS.ContinueProcessing, (CurHiData->GetContinueProcessing() ? 1 : 0));
}

void HighlightFiles::SaveHiData()
{
	KeyFileHelper kfh_highlight(HighlightIni().c_str());
	KeyFileHelper kfh_sortgroups(SortgroupsIni().c_str());

//	const wchar_t *KeyNames[4]={RegColorsHighlight,SortGroupsKeyName,SortGroupsKeyName,RegColorsHighlight};
	const char *GroupNames[4]={fmtFirstGroup,fmtUpperGroup,fmtLowerGroup,fmtLastGroup};
	const int Count[4][2] =
	{
		{0,                               FirstCount},
		{FirstCount,                      FirstCount+UpperCount},
		{FirstCount+UpperCount,           FirstCount+UpperCount+LowerCount},
		{FirstCount+UpperCount+LowerCount,FirstCount+UpperCount+LowerCount+LastCount}
	};

	for (int j=0; j<4; j++)
	{
		KeyFileHelper &kfh = (j == 1 || j == 2) ? kfh_sortgroups : kfh_highlight;

		for (int i=Count[j][0]; i<Count[j][1]; i++)
		{
			std::string strGroupName = StrPrintf(GroupNames[j], i - Count[j][0]);
			FileFilterParams *CurHiData = HiData.getItem(i);

			if (j == 1 || j == 2)
			{
				const wchar_t *Mask = nullptr;
				CurHiData->GetMask(&Mask);
				kfh.PutString(".", strGroupName.c_str(), Mask);
			}

			SaveFilter(CurHiData, kfh, strGroupName.c_str(), (j == 1 || j == 2) );
		}

		for (int i=0; i<5; i++)
		{
			std::string strGroupName = StrPrintf(GroupNames[j], Count[j][1] - Count[j][0] + i);

			if (j == 1 || j == 2)
			{
				kfh.RemoveKey(".", strGroupName.c_str());
			}

			kfh.RemoveSection(strGroupName.c_str());
		}
	}
}

