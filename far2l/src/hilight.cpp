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
#include "palette.hpp"
#include "message.hpp"
#include "config.hpp"
#include "interf.hpp"
#include "ConfigRW.hpp"
#include <Threaded.h>
#include <list>
#include <atomic>
#include <mutex>
#include <unordered_set>

struct HighlightStrings
{
	const char *Flags, *UseAttr, *IncludeAttributes, *ExcludeAttributes, *AttrSet, *AttrClear, *IgnoreMask, *UseMask,
			*Mask, *NormalColor, *SelectedColor, *CursorColor, *SelectedCursorColor, *MarkCharNormalColor, *MarkCharSelectedColor,
			*MarkCharCursorColor, *MarkCharSelectedCursorColor, *MarkChar, *ContinueProcessing, *UseDate, *DateType, *DateAfter,
			*DateBefore, *DateRelative, *UseSize, *SizeAbove, *SizeBelow, *HighlightEdit, *HighlightList, *MarkStr, *NormalColorMask,
			*SelectedColorMask, *CursorColorMask, *SelectedCursorColorMask, *MarkCharNormalColorMask, *MarkCharSelectedColorMask,
			*MarkCharCursorColorMask, *MarkCharSelectedCursorColorMask, *MaskIgnoreCase;
};

static const HighlightStrings HLS = {"Flags", "UseAttr", "IncludeAttributes", "ExcludeAttributes", "AttrSet", "AttrClear",
		"IgnoreMask", "UseMask", "Mask", "NormalColor", "SelectedColor", "CursorColor", "SelectedCursorColor", "MarkCharNormalColor",
		"MarkCharSelectedColor", "MarkCharCursorColor","MarkCharSelectedCursorColor", "MarkChar", "ContinueProcessing", "UseDate",
		"DateType", "DateAfter", "DateBefore", "DateRelative", "UseSize", "SizeAboveS", "SizeBelowS", "HighlightEdit", "HighlightList",
		"MarkStr", "NormalColorMask", "SelectedColorMask", "CursorColorMask", "SelectedCursorColorMask", "MarkCharNormalColorMask",
		"MarkCharSelectedColorMask", "MarkCharCursorColorMask", "MarkCharSelectedCursorColorMask", "MaskIgnoreCase"
};

static const char fmtFirstGroup[] = "Group%d";
static const char fmtUpperGroup[] = "UpperGroup%d";
static const char fmtLowerGroup[] = "LowerGroup%d";
static const char fmtLastGroup[] = "LastGroup%d";
static const char SortGroupsKeyName[] = "SortGroups";
static const char RegColorsHighlight[] = "Colors/Highlight";

static void SetDefaultHighlighting()
{
	fprintf(stderr, "SetDefaultHighlighting\n");

	ConfigWriter cfg_writer;
	static const wchar_t *MasksArchives = /* 1 */
			L"*.rar,*.zip,*.[zj],*.[7bglx]z,*.[bg]zip,*.tar,*.t[agbx]z,*.ar[cj],*.r[0-9][0-9],*.a[0-9][0-9],*."
			L"bz2,*.cab,*.msi,*.jar,*.lha,*.lzh,*.ha,*.ac[bei],*.pa[ck],*.rk,*.cpio,*.rpm,*.tbz2,*.zoo,*.zst,*.hqx,*.sit,*"
			L".ice,*.uc2,*.ain,*.imp,*.777,*.ufa,*.boa,*.bs[2a],*.sea,*.hpk,*.ddi,*.x2,*.rkv,*.[lw]sz,*.h[ay]"
			L"p,*.lim,*.sqz,*.chz";
	static const wchar_t *MasksTemporary = /* 2 */ L"*.bak,*.tmp";
										/*
											$ 25.09.2001  IS
											Эта маска для каталогов: обрабатывать все каталоги, кроме тех, что
											являются родительскими (их имена - две точки).
										*/
	static const wchar_t *MasksScripts = L"*.sh,*.py,*.pl,*.cmd,*.exe,*.bat,*.com,*.run,*.elf";
	static const wchar_t *MasksSoundFiles = 
			L"*.aif,*.cda,*.mid,*.midi,*.mp3,*.mpa,*.ogg,*.wma,*.flac,*.wav,*.ape,*.wv,*.voc,*.669,*.digi,*.amf,*.ams,*.dbm,*.dmf,*.dsm,*.gdm,*.imf,"
			L"*.it,*.itg,*.itp,*.j2b,*.mdl,*.med,*.mo3,*.mod,*.mt2,*.mtm,*.okt,*.plm,*.psm,*.ptm,*.s3m,*.sfx,*.stm,*.stp,*.uax,*.ult,*.xm";
	static const wchar_t *MaskSharedObjects = L"*.dll,*.so,*.dll.*,*.so.*,*.obj,*.o,*.a,*.lib,*.sys,*.pyo,*.vim";
	static const wchar_t *MaskVideoFiles = L"*.mkv,*.webm,*.mpg,*.mp2,*.mpeg,*.mpe,*.mpv,*.mp4,*.m4p,*.m4v,*.avi,*.wmv,*.mov,*.qt,*.flv,*.swf,*.avchd,*.3gp,*.vob";
	static const wchar_t *MaskImageFiles = L"*.avif,*.jpg,*.jpeg,*.jpeg2000,*.ico,*.gif,*.png,*.webp,*.tga,*.bmp,*.pcx,*.tiff,*.tif,*.psd,*.eps,*.indd,*.svg,*.ai,*.cpt,*.kra,*.pdn,*.psp,*.xcf,*.sai,*.cgm,*.mpo,*.pns,*.jps";
	static const wchar_t *MaskSourceFiles = L"*.c,*.cpp,*.c++,*.h,*.hpp,*.h++,*.asm,*.inc,*.src,*.css,*.glsl,*.lua,*.java,*.php,*.go,*.perl,*.r,*.bas,*.pas,*.jsm,*.qml,"
											L"*.js,*.kt,*.sample,*.vs,*.fs,*.fx,*.hlsl,*.fsh,*.vsh,*.pixel,*.vertex,*.fragmentshader,*.fragment,*.vertexshader,"
											L"*.ml,*.frag,*.geom,*.vert,*.rs,*.ts,*.jam,*.tcl, *.swift";
	static const wchar_t *MaskModelFiles =  L"*.ma,*.mb,*.opengex,*.ply,*.pov-ray,*.prc,*.step,*.skp,*.stl,*.u3d,*.vrml,*.xaml,*.xgl,*.xvl,*.xvrml,*.x3d,*.3d,*.3df,*.3dm,*.3ds,*.3dxml,*.x3d,"
											L"*.dds,*.sdkmesh,*.x,*.hdr,*.ktx,*.amf,*.asymptote,*.blend,*.collada,*.dgn,*.dwf,*.dwg,*.dxf,*.drawings,*.flt,*.fvrml,*.gltf,*.hsf,*.iges,*.imml,*.ipa,*.jt";
	static const wchar_t *MaskTextFiles =  	L"*.docx,*.odt,*.pdf,*.rtf,*.tex,*.wpd,*.htm,*.html,*.key,*.odp,*.pps,*.ppt,*.pptx,*.ods,*.xls,*.xlsm,*.xlsx,*.srt,*.nfo,*.rst,*.man,"
											L"read.me,readme*,*.txt,*.chm,*.hlp,*.doc,*.md,NEWS";

	static struct DefaultData
	{
		const wchar_t *Mask;
		int IgnoreMask;
		DWORD IncludeAttr;
		DWORD ExcludeAttr;
		uint64_t NormalColor;
		uint64_t NormalMask;
		uint64_t CursorColor;
		uint64_t CursorMask;
		DWORD dwMarkChar;
		DWORD continueProcessing;
		DWORD Flags;

	} StdHighlightData[] = {

			//           Mask                NormalColor
			//                        IncludeAttributes
			//                     IgnoreMask       CursorColor
	{L"*", 1, FILE_ATTRIBUTE_BROKEN, 0x00, 0x10 | F_LIGHTRED, 0xFFFFFFFD0F, 0x30 | F_LIGHTRED, 0xFFFFFFFD0F, 0xFF0021 /*!*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY, 0x00, 0x10 | F_WHITE, 0xFFFFFFFD0F, 0x30 | F_WHITE, 0xFFFFFFFD0F, 0xFF007E /*~*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_REPARSE_POINT, FILE_ATTRIBUTE_DIRECTORY, 0x90 | F_LIGHTCYAN, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF0040 /*@*/, 0, 0},

	{L"*", 1, FILE_ATTRIBUTE_DEVICE_CHAR, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF002D /*-*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_BLOCK, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF002B /*+*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_FIFO, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF007C /*|*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_SOCK, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF003D /*=*/, 0, 0},

	{L"*", 1, FILE_ATTRIBUTE_HARDLINKS, FILE_ATTRIBUTE_DIRECTORY, 0x10 | F_LIGHTCYAN, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF00AB, 0, 0},

	{L"*", 1, FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_DIRECTORY, 0x10 | F_CYAN, 0xFFFFFFFD0F, 0x30 | F_DARKGRAY, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY, 0x00, 0x10 | F_CYAN, 0xFFFFFFFD0F, 0x30 | F_DARKGRAY, 0xFFFFFFFD0F, 0xFF002F /*/*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_SYSTEM, 0x00, 0x10 | F_CYAN, 0xFFFFFFFD0F, 0x30 | F_DARKGRAY, 0xFFFFFFFD0F, 0xFF263C, 1, 0},
	{L"*|..", 0, FILE_ATTRIBUTE_DIRECTORY, 0x00, 0x10 | F_WHITE, 0xFFFFFFFD0F, 0x30 | F_WHITE, 0xFFFFFFFD0F, 0xFF002F /*/*/, 0, 0},
	{L"..", 0, FILE_ATTRIBUTE_DIRECTORY, 0x00, 0x00, 0xFFFFFFFD0F, 0x00, 0xFFFFFFFD0F, 0x00002F /*/*/, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_EXECUTABLE, 0, 0x10 | F_GREEN, 0xFFFFFFFD0F, 0x30 | F_GREEN, 0xFFFFFFFD0F, 0xFF002A /***/, 0, 0},

	{MasksSoundFiles,0, 0x00,0x00, (0xAAFF00ull << 16) | (0x10 | F_LIGHTGREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x005500ull << 16) | (0x30 | F_LIGHTGREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF266A, 0, FFF_DISABLED},
	{MaskSharedObjects,0, 0x00,0x00, (0x00b800ull << 16) | (0x10 | F_GREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x005500ull << 16) | (0x30 | F_GREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MaskSourceFiles,0, 0x00,0x00, (0xffbcacull << 16) | (0x10 | F_GREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x8F0C00ull << 16) | (0x30 | F_GREEN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MaskVideoFiles,0, 0x00,0x00, (0x30b8ffull << 16) | (0x10 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x006767ull << 16) | (0x30 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MaskImageFiles,0, 0x00,0x00, (0x00ffaeull << 16) | (0x10 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x00432eull << 16) | (0x30 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MaskModelFiles,0, 0x00,0x00, (0x00ffaeull << 16) | (0x10 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x00432eull << 16) | (0x30 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MaskTextFiles,0, 0x00,0x00, (0xccccccull << 16) | (0x10 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, (0x767676ull << 16) | (0x30 | F_BROWN) | FOREGROUND_TRUECOLOR, 0xFFFFFFFD0F, 0xFF0000, 0, 0},

	{MasksScripts,   0, 0x00,0x00, 0x10 | F_LIGHTGREEN, 0xFFFFFFFD0F, 0x30 | F_LIGHTGREEN, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MasksArchives,  0, 0x00,0x00, 0x10 | F_LIGHTMAGENTA, 0xFFFFFFFD0F, 0x30 | F_LIGHTMAGENTA, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{MasksTemporary, 0, 0x00,0x00, 0x10 | F_BROWN, 0xFFFFFFFD0F, 0x30 | F_BROWN, 0xFFFFFFFD0F, 0xFF0000, 0, 0},

	// это настройка для каталогов на тех панелях, которые должны раскрашиваться
	// без учета масок (например, список хостов в "far navigator")
	{L"*", 1, FILE_ATTRIBUTE_EXECUTABLE | FILE_ATTRIBUTE_REPARSE_POINT, 0, 0x10 | F_GREEN, 0xFFFFFFFD0F, 0x30 | F_GREEN, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DIRECTORY, 0x00, 0x10 | F_WHITE, 0xFFFFFFFD0F, 0x30 | F_WHITE, 0xFFFFFFFD0F, 0xFF0000, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_CHAR, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_BLOCK, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_FIFO, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0, 0, 0},
	{L"*", 1, FILE_ATTRIBUTE_DEVICE_SOCK, 0x00, 0x10 | F_LIGHTBLUE, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0, 0, 0},
//	{L"*.*", 0, 0x00, 0x00, 0x10 | F_LIGHTCYAN, 0xFFFFFFFD0F, 0x30 | F_BLUE, 0xFFFFFFFD0F, 0xFF0000, 0}, // Чтобы не раскрашивало на NTFS все файлы в зеленый
	{L"*", 1, FILE_ATTRIBUTE_EXECUTABLE, 0x00, 0x10 | F_LIGHTGREEN, 0xFFFFFFFD0F, 0x30 | F_LIGHTGREEN, 0xFFFFFFFD0F, 0xFF0000, 0, 0}

	};

	for (size_t I = 0; I < ARRAYSIZE(StdHighlightData); I++) {
		cfg_writer.SelectSectionFmt("%s/Group%d", RegColorsHighlight, (int)I);
		cfg_writer.SetString(HLS.Mask, StdHighlightData[I].Mask);
		cfg_writer.SetInt(HLS.IgnoreMask, StdHighlightData[I].IgnoreMask);
		cfg_writer.SetUInt(HLS.IncludeAttributes, StdHighlightData[I].IncludeAttr);
		cfg_writer.SetUInt(HLS.ExcludeAttributes, StdHighlightData[I].ExcludeAttr);
		cfg_writer.SetULL(HLS.NormalColor, StdHighlightData[I].NormalColor);
		cfg_writer.SetULL(HLS.NormalColorMask, StdHighlightData[I].NormalMask);
		cfg_writer.SetULL(HLS.CursorColor, StdHighlightData[I].CursorColor);
		cfg_writer.SetULL(HLS.CursorColorMask, StdHighlightData[I].CursorMask);
		cfg_writer.SetUInt(HLS.MarkChar, StdHighlightData[I].dwMarkChar);
		cfg_writer.SetInt(HLS.ContinueProcessing, StdHighlightData[I].continueProcessing);
		cfg_writer.SetUInt(HLS.Flags, StdHighlightData[I].Flags);
	}

	cfg_writer.SelectSection(RegColorsHighlight);
	cfg_writer.SetInt("Initialized", 1);
}

HighlightFiles::HighlightFiles()
{
	InitHighlightFiles();
	UpdateCurrentTime();
}

static void LoadFilter(FileFilterParams *HData, ConfigReader &cfg_reader, const wchar_t *Mask, int SortGroup,
		bool bSortGroup)
{
	// Дефолтные значения выбраны так чтоб как можно правильней загрузить
	// настройки старых версий фара.
	if (bSortGroup)
		HData->SetMask(cfg_reader.GetInt(HLS.UseMask, 1) != 0, Mask);
	else
		HData->SetMask(cfg_reader.GetInt(HLS.IgnoreMask, 0) == 0, Mask,
					   cfg_reader.GetInt(HLS.MaskIgnoreCase, 1) == 1);

	FILETIME DateAfter{}, DateBefore{};
	cfg_reader.GetPOD(HLS.DateAfter, DateAfter);
	cfg_reader.GetPOD(HLS.DateBefore, DateBefore);
	HData->SetDate(cfg_reader.GetInt(HLS.UseDate, 1) != 0, (DWORD)cfg_reader.GetUInt(HLS.DateType, 0),
			DateAfter, DateBefore, cfg_reader.GetInt(HLS.DateRelative, 0) != 0);
	FARString strSizeAbove = cfg_reader.GetString(HLS.SizeAbove, L"");
	FARString strSizeBelow = cfg_reader.GetString(HLS.SizeBelow, L"");
	HData->SetSize(cfg_reader.GetInt(HLS.UseSize, 0) != 0, strSizeAbove, strSizeBelow);

	if (bSortGroup) {
		HData->SetAttr(cfg_reader.GetInt(HLS.UseAttr, 1) != 0, (DWORD)cfg_reader.GetUInt(HLS.AttrSet, 0),
				(DWORD)cfg_reader.GetUInt(HLS.AttrClear, FILE_ATTRIBUTE_DIRECTORY));
	} else {
		HData->SetAttr(cfg_reader.GetInt(HLS.UseAttr, 1) != 0,
				(DWORD)cfg_reader.GetUInt(HLS.IncludeAttributes, 0),
				(DWORD)cfg_reader.GetUInt(HLS.ExcludeAttributes, 0));
	}
	HData->SetSortGroup(SortGroup);

	HData->SetFlags(FFFT_CUSTOM, cfg_reader.GetUInt(HLS.Flags, 0));

	HighlightDataColor hl;
	hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL] = cfg_reader.GetULL(HLS.NormalColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED] = cfg_reader.GetULL(HLS.SelectedColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR] = cfg_reader.GetULL(HLS.CursorColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = cfg_reader.GetULL(HLS.SelectedCursorColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_NORMAL] = cfg_reader.GetULL(HLS.MarkCharNormalColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTED] = cfg_reader.GetULL(HLS.MarkCharSelectedColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_UNDERCURSOR] = cfg_reader.GetULL(HLS.MarkCharCursorColor, 0);
	hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = cfg_reader.GetULL(HLS.MarkCharSelectedCursorColor, 0);

	hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL] = cfg_reader.GetULL(HLS.NormalColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED] = cfg_reader.GetULL(HLS.SelectedColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR] = cfg_reader.GetULL(HLS.CursorColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = cfg_reader.GetULL(HLS.SelectedCursorColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_NORMAL] = cfg_reader.GetULL(HLS.MarkCharNormalColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTED] = cfg_reader.GetULL(HLS.MarkCharSelectedColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_UNDERCURSOR] = cfg_reader.GetULL(HLS.MarkCharCursorColorMask, 0);
	hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR] = cfg_reader.GetULL(HLS.MarkCharSelectedCursorColorMask, 0);

	{ // Load Mark str
		FARString strMark = cfg_reader.GetString(HLS.MarkStr, L"");
		DWORD dwMarkLen = strMark.GetLength();
		DWORD dwMarkChar = cfg_reader.GetUInt(HLS.MarkChar, 0);

		hl.Flags = (dwMarkChar & 0xFF0000) >> 23;
		dwMarkChar &= 0x0000FFFF;

		if (dwMarkLen) {
			if (dwMarkLen > HIGHLIGHT_MAX_MARK_LENGTH)
				dwMarkLen = HIGHLIGHT_MAX_MARK_LENGTH;

			memcpy(&hl.Mark[0], strMark.GetBuffer(), sizeof(wchar_t) * dwMarkLen);
			strMark.ReleaseBuffer();
		}
		else if (dwMarkChar) {
			hl.Mark[0] = dwMarkChar;
			dwMarkLen = 1;
		}

		hl.Mark[dwMarkLen] = 0; // terminate
		hl.MarkLen = dwMarkLen;
	}


#if 1
	  // FIXME: Temporary code for compatibility with old settings where there are no transparency masks for colors

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 4; i++) {

			if (hl.Mask[j][i])
				continue;

			hl.Mask[j][i] = 0xFFFFFFFFFFFFFFFF;

			if (hl.Color[j][i] & 0x0000000000000F00) // transparent foreground
				hl.Mask[j][i] ^= (0x000000FFFFFF000F | FOREGROUND_TRUECOLOR);

			if (hl.Color[j][i] & 0x000000000000F000) // transparent background
				hl.Mask[j][i] ^= (0xFFFFFF00000000F0 | BACKGROUND_TRUECOLOR);

			hl.Color[j][i] &= 0xFFFFFFFFFFFF00FF;

			if (hl.Color[j][i] & 0xFFFFFF0000000000) // background true color
				hl.Color[j][i] |= BACKGROUND_TRUECOLOR;

			if (hl.Color[j][i] & 0x000000FFFFFF0000) // foreground true color
				hl.Color[j][i] |= FOREGROUND_TRUECOLOR;
		}
	}

	   // FIXME: Temporary code for compatibility with old settings where there are no transparency masks for colors
#endif

	HData->SetColors(&hl);
	HData->SetContinueProcessing(cfg_reader.GetInt(HLS.ContinueProcessing, 0) != 0);
}

void HighlightFiles::InitHighlightFiles()
{
	FARString strMask;
	std::string strGroupName, strRegKey;
	const int GroupDelta[4] = {DEFAULT_SORT_GROUP, 0, DEFAULT_SORT_GROUP + 1, DEFAULT_SORT_GROUP};
	const char *KeyNames[4] = {RegColorsHighlight, SortGroupsKeyName, SortGroupsKeyName, RegColorsHighlight};
	const char *GroupNames[4] = {fmtFirstGroup, fmtUpperGroup, fmtLowerGroup, fmtLastGroup};
	int *Count[4] = {&FirstCount, &UpperCount, &LowerCount, &LastCount};
	HiData.Free();
	FirstCount = UpperCount = LowerCount = LastCount = 0;

	std::unique_ptr<ConfigReader> cfg_reader(new ConfigReader(RegColorsHighlight));
	// fast check if has root section that is created by SetDefaultHighlighting
	// if no - slower check that has no any subsections
	// if no root section and no subsections - apply default settings
	if (!cfg_reader->HasSection() && cfg_reader->EnumSectionsAt().empty()) {
		SetDefaultHighlighting();
		cfg_reader.reset(new ConfigReader);
	}

	for (int j = 0; j < 4; j++) {
		for (int i = 0;; i++) {
			strGroupName = StrPrintf(GroupNames[j], i);
			strRegKey = KeyNames[j];
			strRegKey+= '/';
			strRegKey+= strGroupName;
			if (GroupDelta[j] != DEFAULT_SORT_GROUP) {
				cfg_reader->SelectSection(KeyNames[j]);
				if (!cfg_reader->GetString(strMask, strGroupName, L""))
					break;
				cfg_reader->SelectSection(strRegKey);
			} else {
				cfg_reader->SelectSection(strRegKey);
				if (!cfg_reader->GetString(strMask, HLS.Mask, L""))
					break;
			}
			FileFilterParams *HData = HiData.addItem();

			if (HData) {
				LoadFilter(HData, *cfg_reader, strMask,
						GroupDelta[j] + (GroupDelta[j] == DEFAULT_SORT_GROUP ? 0 : i),
						(GroupDelta[j] == DEFAULT_SORT_GROUP ? false : true));
				(*(Count[j]))++;
			} else
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
	FirstCount = UpperCount = LowerCount = LastCount = 0;
}

static const DWORD FarColor[] = {COL_PANELTEXT, COL_PANELSELECTEDTEXT, COL_PANELCURSOR,
		COL_PANELSELECTEDCURSOR};

static const HighlightDataColor DefaultStartingColors =
	{
		{	{0x0, 0x0, 0x0, 0x0},	// Color[0][4] 0 = Black on black = default theme color
			{0x0, 0x0, 0x0, 0x0}},	// Color[1][4]
		{	{0x0, 0x0, 0x0, 0x0},	// Mask[0][4] // Transparency Masks 0 = fully transparent
			{0x0, 0x0, 0x0, 0x0}},	// Mask[1][4]
		0,     						// size_t	MarkLen;
		1,   						// flags;
		{ 0 }, 						// wchar_t	Mark
	};

const HighlightDataColor ZeroColors =
	{
		{	{0x0, 0x0, 0x0, 0x0},	// Color[0][4] 0 = Black on black = default theme color
			{0x0, 0x0, 0x0, 0x0}},	// Color[1][4]
		{	{0x0, 0x0, 0x0, 0x0},	// Mask[0][4] // Transparency Masks 0 = fully transparent
			{0x0, 0x0, 0x0, 0x0}},	// Mask[1][4]
		0,     						// size_t	MarkLen;
		0,   						// flags;
		{ 0 }, 						// wchar_t	Mark
	};

//const HighlightDataColor ZeroColors{{{0}}};

static void ApplyBlackOnBlackColors(HighlightDataColor *hl)
{
	for (int i = 0; i < 4; i++) {

		// Применим black on black.
		// Для файлов возьмем цвета панели не изменяя прозрачность.
		// Для пометки возьмем цвета файла включая прозрачность.
		if (!(hl->Color[HIGHLIGHTCOLORTYPE_FILE][i] & 0xFF)) {

			hl->Color[HIGHLIGHTCOLORTYPE_FILE][i] = Palette[FarColor[i]];
		}

		// Если у метки black on black, то возьмем ей цвета и маску от имени
		if (!(hl->Color[HIGHLIGHTCOLORTYPE_MARKSTR][i] & 0xFF)) {

			hl->Color[HIGHLIGHTCOLORTYPE_MARKSTR][i] = hl->Color[HIGHLIGHTCOLORTYPE_FILE][i];
			hl->Mask[HIGHLIGHTCOLORTYPE_MARKSTR][i] = hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i];
		}
	}
}

static void ApplyStartColors(HighlightDataColor *hl)
{
	// Начинаем с цветов "по умолчанию" и далее их раскрашиваем
	for (int i = 0; i < 4; i++) {
		hl->Color[HIGHLIGHTCOLORTYPE_FILE][i] = Palette[FarColor[i]];
		hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i] |= 0xFFFFFFFFFFFFFFFF;
		hl->Color[HIGHLIGHTCOLORTYPE_MARKSTR][i] = hl->Color[HIGHLIGHTCOLORTYPE_FILE][i];
		hl->Mask[HIGHLIGHTCOLORTYPE_MARKSTR][i] = hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i];
	}

	hl->Flags   = 0;
	hl->MarkLen = 0;
	hl->Mark[0] = 0;
}

static void ApplyColors(HighlightDataColor *hlDst, HighlightDataColor *hlSrc)
{
	// Обработаем black on black чтоб наследовать правильные цвета
	// и чтоб после наследования были правильные цвета.
	ApplyBlackOnBlackColors(hlDst);
	ApplyBlackOnBlackColors(hlSrc);

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 4; i++) {

			// Если текущие цвета в Src (fore и/или back) не прозрачные
			// то унаследуем их в Dest. (включая флаги стилей)

			if (hlSrc->Mask[j][i]) {
				hlDst->Color[j][i] = (hlDst->Color[j][i] & (~hlSrc->Mask[j][i])) | (hlSrc->Color[j][i] & hlSrc->Mask[j][i]);
			}
		}
	}

	// Унаследуем пометку из Src в Dst если она есть
	if (hlSrc->MarkLen) {
		// Если нет наследования в Src, то просто заменим метку на новую в Dst
		if (!(hlSrc->Flags & HL_FLAGS_MARK_INHERIT)) {
			hlDst->MarkLen = hlSrc->MarkLen;
			memcpy(hlDst->Mark, hlSrc->Mark, sizeof(wchar_t) * hlSrc->MarkLen);
		}
		else { // Если есть наследование, добавим метку к старой
			uint32_t freespace = (HIGHLIGHT_MAX_MARK_LENGTH - hlDst->MarkLen);
			if (freespace) { // Если есть хоть какое то место, добавим что влезет
				uint32_t copylen = (freespace < hlSrc->MarkLen) ? freespace : hlSrc->MarkLen;
				memcpy(hlDst->Mark + hlDst->MarkLen, hlSrc->Mark, sizeof(wchar_t) * copylen);
				hlDst->MarkLen += copylen;
			}
		}
	}
	else if (!(hlSrc->Flags & HL_FLAGS_MARK_INHERIT)) { // Если нет наследования и метка пустая, то убираем метку совсем
		hlDst->MarkLen = 0;
	}
}

void HighlightFiles::UpdateCurrentTime()
{
	SYSTEMTIME cst;
	FILETIME cft;
	WINPORT(GetSystemTime)(&cst);
	WINPORT(SystemTimeToFileTime)(&cst, &cft);
	ULARGE_INTEGER current;
	current.u.LowPart = cft.dwLowDateTime;
	current.u.HighPart = cft.dwHighDateTime;
	CurrentTime = current.QuadPart;
}

////

class HighlightFilesChunk : protected Threaded
{
	uint64_t _CurrentTime;
	const TPointerArray<FileFilterParams> &_HiData;
	FileListItem **_FileItem;
	size_t _FileCount, _MarkLM;
	bool _UseAttrHighlighting;

	virtual void *ThreadProc()
	{
		return DoNow();
	}

public:
	HighlightFilesChunk(uint64_t CurrentTime, const TPointerArray<FileFilterParams> &HiData,
			FileListItem **FileItem, size_t FileCount, bool UseAttrHighlighting)
		:
		_CurrentTime(CurrentTime),
		_HiData(HiData),
		_FileItem(FileItem),
		_FileCount(FileCount),
		_MarkLM(0),
		_UseAttrHighlighting(UseAttrHighlighting)
	{}

	virtual ~HighlightFilesChunk() { WaitThread(); }

	void *DoNow()
	{
		for (size_t FCnt = 0; FCnt < _FileCount; ++FCnt) {
			FileListItem &fli = *_FileItem[FCnt];
			HighlightDataColor Colors = DefaultStartingColors;
			ApplyStartColors(&Colors);

			for (size_t i = 0; i < _HiData.getCount(); i++) {
				const FileFilterParams *CurHiData = _HiData.getConstItem(i);
				if (CurHiData->GetFlags(FFFT_CUSTOM) & FFF_DISABLED)
					continue;

				if (_UseAttrHighlighting && CurHiData->GetMask(nullptr))
					continue;

				if (CurHiData->FileInFilter(fli, _CurrentTime)) {
					HighlightDataColor TempColors;
					CurHiData->GetColors(&TempColors);
					ApplyColors(&Colors, &TempColors);

					if (!CurHiData->GetContinueProcessing())
						break;
				}
			}

			if (Colors.MarkLen) {
				size_t ncells = StrCellsCount( Colors.Mark, Colors.MarkLen );

				if (ncells > _MarkLM && ncells <= Opt.MaxFilenameIndentation)
					_MarkLM = ncells;
			}

			fli.ColorsPtr = PooledHighlightDataColor(Colors);
		}

		return (void *)_MarkLM;
	}

	void *GetResult() { return GetThreadResult(); }

	bool DoAsync()
	{
		return StartThread();
	}
};

void HighlightFiles::GetHiColor(FileListItem **FileItem, size_t FileCount, bool UseAttrHighlighting, size_t *_MarkLM)
{
	if (!FileItem || !FileCount)
		return;

	size_t MarkLM = 0;
	size_t BestThreadsNum = std::max(BestThreadsCount(), 1u);
	std::list<HighlightFilesChunk> async_hfc;

	const int sFileCountTrh = 0x1000;	// empirically found, can be subject of (dynamic) adjustment

	if (FileCount >= sFileCountTrh && BestThreadsNum > 1) {
		size_t FilePerCPU = std::max(FileCount / BestThreadsNum, (size_t)0x400u);

		while (FileCount > FilePerCPU && async_hfc.size() + 1 < BestThreadsNum) {
			async_hfc.emplace_back(CurrentTime, HiData, FileItem, FilePerCPU, UseAttrHighlighting);
			if (!async_hfc.back().DoAsync()) {
				async_hfc.pop_back();
				break;
			}
			FileItem += FilePerCPU;
			FileCount -= FilePerCPU;
		}
	}

	MarkLM = (size_t)HighlightFilesChunk(CurrentTime, HiData, FileItem, FileCount, UseAttrHighlighting).DoNow();

	while(!async_hfc.empty( )) {
		size_t len = (size_t)async_hfc.back().GetResult();
		if (len > MarkLM)
			MarkLM = len;
		async_hfc.pop_back();
	}

	if (_MarkLM && *_MarkLM < MarkLM )
		*_MarkLM = MarkLM;
}

int HighlightFiles::GetGroup(const FileListItem *fli)
{
	for (int i = FirstCount; i < FirstCount + UpperCount; i++) {
		FileFilterParams *CurGroupData = HiData.getItem(i);

		if (CurGroupData->FileInFilter(*fli, CurrentTime))
			return (CurGroupData->GetSortGroup());
	}

	for (int i = FirstCount + UpperCount; i < FirstCount + UpperCount + LowerCount; i++) {
		FileFilterParams *CurGroupData = HiData.getItem(i);

		if (CurGroupData->FileInFilter(*fli, CurrentTime))
			return (CurGroupData->GetSortGroup());
	}

	return DEFAULT_SORT_GROUP;
}

void HighlightFiles::FillMenu(VMenu *HiMenu, int MenuPos)
{
	MenuItemEx HiMenuItem;
	const int Count[4][2] = {
		{0,                                    FirstCount                                      },
		{FirstCount,                           FirstCount + UpperCount                         },
		{FirstCount + UpperCount,              FirstCount + UpperCount + LowerCount            },
		{FirstCount + UpperCount + LowerCount, FirstCount + UpperCount + LowerCount + LastCount}
	};
	HiMenu->DeleteItems();
	HiMenuItem.Clear();

	for (int j = 0; j < 4; j++) {
		for (int i = Count[j][0]; i < Count[j][1]; i++) {

			FileFilterParams *ffp = HiData.getItem(i);
			MenuString(HiMenuItem.strName, ffp, true);

			if (ffp->GetFlags(FFFT_CUSTOM) & FFF_DISABLED) {
				HiMenuItem.Flags |= LIF_GRAYED;
				HiMenuItem.SetCheck(L'X');
			}

			HiMenu->AddItem(&HiMenuItem);
			HiMenuItem.Clear();
		}

		HiMenuItem.strName.Clear();
		HiMenu->AddItem(&HiMenuItem);

		if (j < 3) {
			if (!j)
				HiMenuItem.strName = Msg::HighlightUpperSortGroup;
			else if (j == 1)
				HiMenuItem.strName = Msg::HighlightLowerSortGroup;
			else
				HiMenuItem.strName = Msg::HighlightLastGroup;

			HiMenuItem.Flags|= LIF_SEPARATOR;
			HiMenu->AddItem(&HiMenuItem);
			HiMenuItem.Flags = 0;
		}
	}

	HiMenu->SetSelectPos(MenuPos, 1);
}

void HighlightFiles::ProcessGroups()
{
	for (int i = 0; i < FirstCount; i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);

	for (int i = FirstCount; i < FirstCount + UpperCount; i++)
		HiData.getItem(i)->SetSortGroup(i - FirstCount);

	for (int i = FirstCount + UpperCount; i < FirstCount + UpperCount + LowerCount; i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP + 1 + i - FirstCount - UpperCount);

	for (int i = FirstCount + UpperCount + LowerCount; i < FirstCount + UpperCount + LowerCount + LastCount;
			i++)
		HiData.getItem(i)->SetSortGroup(DEFAULT_SORT_GROUP);
}

int HighlightFiles::MenuPosToRealPos(int MenuPos, int **Count, bool Insert)
{
	int Pos = MenuPos;
	*Count = nullptr;
	int x = Insert ? 1 : 0;

	if (MenuPos < FirstCount + x) {
		*Count = &FirstCount;
	} else if (MenuPos > FirstCount + 1 && MenuPos < FirstCount + UpperCount + 2 + x) {
		Pos = MenuPos - 2;
		*Count = &UpperCount;
	} else if (MenuPos > FirstCount + UpperCount + 3
			&& MenuPos < FirstCount + UpperCount + LowerCount + 4 + x) {
		Pos = MenuPos - 4;
		*Count = &LowerCount;
	} else if (MenuPos > FirstCount + UpperCount + LowerCount + 5
			&& MenuPos < FirstCount + UpperCount + LowerCount + LastCount + 6 + x) {
		Pos = MenuPos - 6;
		*Count = &LastCount;
	}

	return Pos;
}

void HighlightFiles::HiEdit(int MenuPos)
{
	VMenu HiMenu(Msg::HighlightTitle, nullptr, 0, ScrY - 4);
	HiMenu.SetHelp(FARString(HLS.HighlightList));
	HiMenu.SetFlags(VMENU_WRAPMODE | VMENU_SHOWAMPERSAND ); //
	HiMenu.SetPosition(-1, -1, 0, 0);
	HiMenu.SetBottomTitle(Msg::HighlightBottom);
	FillMenu(&HiMenu, MenuPos);
	int NeedUpdate;
	Panel *LeftPanel = CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel = CtrlObject->Cp()->RightPanel;
	HiMenu.Show();

	while (1) {
		while (!HiMenu.Done()) {
			FarKey Key = HiMenu.ReadInput();
			int SelectPos = HiMenu.GetSelectPos();
			NeedUpdate = FALSE;

			switch (Key) {
					/*
						$ 07.07.2000 IS
						Если нажали ctrl+r, то восстановить значения по умолчанию.
					*/

				case KEY_SUBTRACT:
				case KEY_ADD:
				case KEY_SPACE: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && RealSelectPos < (int)HiData.getCount()) {

						FileFilterParams *ffp = HiData.getItem(RealSelectPos);
						uint32_t flags = ffp->GetFlags(FFFT_CUSTOM);
						if (Key == KEY_SUBTRACT && (flags & FFF_DISABLED))
							break;
						if (Key == KEY_ADD && !(flags & FFF_DISABLED))
							break;

						ffp->SetFlags(FFFT_CUSTOM, flags ^ FFF_DISABLED);

						HiMenu.SetUpdateRequired(TRUE);
						HiMenu.FastShow();
						HiMenu.ProcessKey(KEY_DOWN);
						NeedUpdate = true;
					}
				}
				break;

				case KEY_CTRLR:
					if (Message(MSG_WARNING, 2, Msg::HighlightTitle, Msg::HighlightWarning,
								Msg::HighlightAskRestore, Msg::Yes, Msg::Cancel))
						break;

					{
						ConfigWriter(RegColorsHighlight).RemoveSection();
					}
					HiMenu.Hide();
					ClearData();
					InitHighlightFiles();
					NeedUpdate = TRUE;
					break;
				case KEY_NUMDEL:
				case KEY_DEL: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && RealSelectPos < (int)HiData.getCount()) {
						const wchar_t *Mask;
						HiData.getItem(RealSelectPos)->GetMask(&Mask);

						if (Message(MSG_WARNING, 2, Msg::HighlightTitle, Msg::HighlightAskDel, Mask,
									Msg::Delete, Msg::Cancel))
							break;

						HiData.deleteItem(RealSelectPos);
						(*Count)--;
						NeedUpdate = TRUE;
					}

					break;
				}
				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && RealSelectPos < (int)HiData.getCount())
						if (FileFilterConfig(HiData.getItem(RealSelectPos), true))
							NeedUpdate = TRUE;

					break;
				}
				case KEY_INS:
				case KEY_NUMPAD0: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count, true);

					if (Count) {
						FileFilterParams *NewHData = HiData.insertItem(RealSelectPos);

						if (!NewHData)
							break;

						if (FileFilterConfig(NewHData, true)) {
							(*Count)++;
							NeedUpdate = TRUE;
						} else
							HiData.deleteItem(RealSelectPos);
					}

					break;
				}
				case KEY_F5: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && RealSelectPos < (int)HiData.getCount()) {
						FileFilterParams *HData = HiData.insertItem(RealSelectPos);

						if (HData) {
							*HData = *HiData.getItem(RealSelectPos + 1);
							HData->SetTitle(L"");

							if (FileFilterConfig(HData, true)) {
								NeedUpdate = TRUE;
								(*Count)++;
							} else
								HiData.deleteItem(RealSelectPos);
						}
					}

					break;
				}
				case KEY_CTRLUP:
				case KEY_CTRLNUMPAD8: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && SelectPos > 0) {
						if (UpperCount && RealSelectPos == FirstCount
								&& RealSelectPos < FirstCount + UpperCount) {
							FirstCount++;
							UpperCount--;
							SelectPos--;
						} else if (LowerCount && RealSelectPos == FirstCount + UpperCount
								&& RealSelectPos < FirstCount + UpperCount + LowerCount) {
							UpperCount++;
							LowerCount--;
							SelectPos--;
						} else if (LastCount && RealSelectPos == FirstCount + UpperCount + LowerCount) {
							LowerCount++;
							LastCount--;
							SelectPos--;
						} else
							HiData.swapItems(RealSelectPos, RealSelectPos - 1);

						HiMenu.SetCheck(--SelectPos);
						NeedUpdate = TRUE;
						break;
					}

					HiMenu.ProcessInput();
					break;
				}
				case KEY_CTRLDOWN:
				case KEY_CTRLNUMPAD2: {
					int *Count = nullptr;
					int RealSelectPos = MenuPosToRealPos(SelectPos, &Count);

					if (Count && SelectPos < (int)HiMenu.GetItemCount() - 2) {
						if (FirstCount && RealSelectPos == FirstCount - 1) {
							FirstCount--;
							UpperCount++;
							SelectPos++;
						} else if (UpperCount && RealSelectPos == FirstCount + UpperCount - 1) {
							UpperCount--;
							LowerCount++;
							SelectPos++;
						} else if (LowerCount && RealSelectPos == FirstCount + UpperCount + LowerCount - 1) {
							LowerCount--;
							LastCount++;
							SelectPos++;
						} else
							HiData.swapItems(RealSelectPos, RealSelectPos + 1);

						HiMenu.SetCheck(++SelectPos);
						NeedUpdate = TRUE;
					}

					HiMenu.ProcessInput();
					break;
				}
				default:
					HiMenu.ProcessInput();
					break;
			}

			// повторяющийся кусок!
			if (NeedUpdate) {
				ScrBuf.Lock();	// отменяем всякую прорисовку
				HiMenu.Hide();
				ProcessGroups();

				if (Opt.AutoSaveSetup)
					SaveHiData();

				// FrameManager->RefreshFrame(); // рефрешим
				LeftPanel->Update(UPDATE_KEEP_SELECTION);
				LeftPanel->Redraw();
				RightPanel->Update(UPDATE_KEEP_SELECTION);
				RightPanel->Redraw();
				FillMenu(&HiMenu, MenuPos = SelectPos);
				HiMenu.SetPosition(-1, -1, 0, 0);
				HiMenu.Show();
				ScrBuf.Unlock();	// разрешаем прорисовку
			}
		}

		if (HiMenu.Modal::GetExitCode() != -1) {
			HiMenu.ClearDone();
			HiMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}

static void SaveFilter(FileFilterParams *CurHiData, ConfigWriter &cfg_writer, bool bSortGroup)
{
	if (bSortGroup) {
		cfg_writer.SetInt(HLS.UseMask, CurHiData->GetMask(nullptr) ? 1 : 0);
	} else {
		const wchar_t *Mask = nullptr;
		cfg_writer.SetInt(HLS.IgnoreMask, (CurHiData->GetMask(&Mask) ? 0 : 1));
		cfg_writer.SetString(HLS.Mask, Mask);
		cfg_writer.SetInt(HLS.MaskIgnoreCase, CurHiData->GetMaskIgnoreCase() ? 1 : 0);
	}

	DWORD DateType;
	FILETIME DateAfter, DateBefore;
	bool bRelative;

	cfg_writer.SetInt(HLS.UseDate,
			CurHiData->GetDate(&DateType, &DateAfter, &DateBefore, &bRelative) ? 1 : 0);
	cfg_writer.SetUInt(HLS.DateType, DateType);
	cfg_writer.SetPOD(HLS.DateAfter, DateAfter);
	cfg_writer.SetPOD(HLS.DateBefore, DateBefore);
	cfg_writer.SetInt(HLS.DateRelative, bRelative ? 1 : 0);

	const wchar_t *SizeAbove = nullptr, *SizeBelow = nullptr;
	cfg_writer.SetInt(HLS.UseSize, CurHiData->GetSize(&SizeAbove, &SizeBelow) ? 1 : 0);
	cfg_writer.SetString(HLS.SizeAbove, SizeAbove);
	cfg_writer.SetString(HLS.SizeBelow, SizeBelow);

	DWORD AttrSet = 0, AttrClear = 0;
	cfg_writer.SetInt(HLS.UseAttr, CurHiData->GetAttr(&AttrSet, &AttrClear) ? 1 : 0);
	cfg_writer.SetUInt((bSortGroup ? HLS.AttrSet : HLS.IncludeAttributes), AttrSet);
	cfg_writer.SetUInt((bSortGroup ? HLS.AttrClear : HLS.ExcludeAttributes), AttrClear);

	cfg_writer.SetUInt(HLS.Flags, CurHiData->GetFlags(FFFT_CUSTOM));

	HighlightDataColor hl;
	CurHiData->GetColors(&hl);

	cfg_writer.SetULL(HLS.NormalColor,
			hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]);
	cfg_writer.SetULL(HLS.SelectedColor,
			hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]);
	cfg_writer.SetULL(HLS.CursorColor,
			hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]);
	cfg_writer.SetULL(HLS.SelectedCursorColor,
			hl.Color[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
	cfg_writer.SetULL(HLS.MarkCharNormalColor,
			hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_NORMAL]);
	cfg_writer.SetULL(HLS.MarkCharSelectedColor,
			hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTED]);
	cfg_writer.SetULL(HLS.MarkCharCursorColor,
			hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_UNDERCURSOR]);
	cfg_writer.SetULL(HLS.MarkCharSelectedCursorColor,
			hl.Color[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);

	cfg_writer.SetULL(HLS.NormalColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_NORMAL]);
	cfg_writer.SetULL(HLS.SelectedColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTED]);
	cfg_writer.SetULL(HLS.CursorColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_UNDERCURSOR]);
	cfg_writer.SetULL(HLS.SelectedCursorColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_FILE][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);
	cfg_writer.SetULL(HLS.MarkCharNormalColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_NORMAL]);
	cfg_writer.SetULL(HLS.MarkCharSelectedColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTED]);
	cfg_writer.SetULL(HLS.MarkCharCursorColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_UNDERCURSOR]);
	cfg_writer.SetULL(HLS.MarkCharSelectedCursorColorMask,
			hl.Mask[HIGHLIGHTCOLORTYPE_MARKSTR][HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR]);

	{ // Save Mark str
		FARString strMark = L"";
//		DWORD dwMarkChar = (hl.MarkLen == 1) ? hl.Mark[0] : 0;
		DWORD dwMarkChar = 0;
		dwMarkChar |= (0xFF0000 * (hl.Flags & HL_FLAGS_MARK_INHERIT));

		cfg_writer.SetUInt(HLS.MarkChar, dwMarkChar);

//		if (hl.MarkLen > 1)
		strMark = hl.Mark;
		cfg_writer.SetString(HLS.MarkStr, strMark);
	}

	cfg_writer.SetInt(HLS.ContinueProcessing, (CurHiData->GetContinueProcessing() ? 1 : 0));
}

void HighlightFiles::SaveHiData()
{
	std::string strRegKey, strGroupName;
	const char *KeyNames[4] = {RegColorsHighlight, SortGroupsKeyName, SortGroupsKeyName, RegColorsHighlight};
	const char *GroupNames[4] = {fmtFirstGroup, fmtUpperGroup, fmtLowerGroup, fmtLastGroup};
	const int Count[4][2] = {
		{0,                                    FirstCount                                      },
		{FirstCount,                           FirstCount + UpperCount                         },
		{FirstCount + UpperCount,              FirstCount + UpperCount + LowerCount            },
		{FirstCount + UpperCount + LowerCount, FirstCount + UpperCount + LowerCount + LastCount}
	};

	ConfigWriter cfg_writer;
	for (int j = 0; j < 4; j++) {
		for (int i = Count[j][0]; i < Count[j][1]; i++) {
			strGroupName = StrPrintf(GroupNames[j], i - Count[j][0]);
			strRegKey = KeyNames[j];
			strRegKey+= '/';
			strRegKey+= strGroupName;
			FileFilterParams *CurHiData = HiData.getItem(i);

			if (j == 1 || j == 2) {
				const wchar_t *Mask = nullptr;
				CurHiData->GetMask(&Mask);
				cfg_writer.SelectSection(KeyNames[j]);
				cfg_writer.SetString(strGroupName, Mask);
			}
			cfg_writer.SelectSection(strRegKey);
			SaveFilter(CurHiData, cfg_writer, (j == 1 || j == 2));
		}

		for (int i = 0; i < 5; i++) {
			strGroupName = StrPrintf(GroupNames[j], Count[j][1] - Count[j][0] + i);
			strRegKey = KeyNames[j];
			strRegKey+= '/';
			strRegKey+= strGroupName;

			if (j == 1 || j == 2) {
				cfg_writer.SelectSection(KeyNames[j]);
				cfg_writer.RemoveKey(strGroupName);
			}
			cfg_writer.SelectSection(strRegKey);
			cfg_writer.RemoveSection();
		}
	}
}


////////

static bool operator==(const HighlightDataColor &hl1, const HighlightDataColor &hl2)
{
	if (hl1.Flags != hl2.Flags || hl1.MarkLen != hl2.MarkLen)
		return false;

	if (hl1.MarkLen && wmemcmp(&hl1.Mark[0], &hl2.Mark[0], hl1.MarkLen) != 0)
		return false;

	if (memcmp(hl1.Color, hl2.Color, sizeof(hl2.Color)) != 0)
		return false;

	if (memcmp(hl1.Mask, hl2.Mask, sizeof(hl2.Mask)) != 0)
		return false;

	return true;
}


struct HighlightDataColorHash
{
	size_t operator()(const HighlightDataColor &hl) const
	{
		size_t out = hl.Flags ^ (hl.MarkLen * 0xFFFF);

		for (size_t i = 0; i < ARRAYSIZE(hl.Color); ++i) {
			for (size_t j = 0; j < ARRAYSIZE(hl.Color[i]); ++j) {
				out ^= hl.Color[i][j] + hl.Mask[i][j] + ((i ^ j) << 16);
			}
		}
		return out;
	}
};

static std::unordered_set<HighlightDataColor, HighlightDataColorHash> s_highlight_color_pool;
static std::mutex s_highlight_color_pool_mutex;
static std::atomic<const HighlightDataColor *> s_last_color{&DefaultStartingColors};

const HighlightDataColor *PooledHighlightDataColor(const HighlightDataColor &color)
{
	const HighlightDataColor *last_color = s_last_color.load(std::memory_order_relaxed);

	if (color == *last_color) {
		return last_color;
	}

	std::lock_guard<std::mutex> lock(s_highlight_color_pool_mutex);
	const auto &ir = s_highlight_color_pool.insert(color);
	const HighlightDataColor *out = &(*ir.first);
	s_last_color = out;
	return out;
}
