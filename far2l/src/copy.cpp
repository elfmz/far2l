/*
copy.cpp

Копирование файлов
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

#include "copy.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "dialog.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "foldtree.hpp"
#include "treelist.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "constitle.hpp"
#include "filefilter.hpp"
#include "fileview.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "config.hpp"
#include "stddlg.hpp"
#include "fileattr.hpp"
#include "datetime.hpp"
#include "dirinfo.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"
#include "processname.hpp"
#include "MountInfo.h"
#include "mix.hpp"
#include "DlgGuid.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include <unistd.h>
#include <algorithm>

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_12) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#include <sys/attr.h>
#include <sys/clonefile.h>
#define COW_SUPPORTED
#endif

#elif defined(__linux__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 27))
#define COW_SUPPORTED

#endif

/* Общее время ожидания пользователя */
extern long WaitUserTime;
/* Для того, что бы время при ожидании пользователя тикало, а remaining/speed нет */
static long OldCalcTime;

#define PROGRESS_REFRESH_THRESHOLD 200		// msec

enum
{
	COPY_BUFFER_SIZE   = 0x800000,
	COPY_PIECE_MINIMAL = 0x10000
};

enum
{
	COPY_RULE_NUL   = 0x0001,
	COPY_RULE_FILES = 0x0002,
};

static int TotalFiles, TotalFilesToProcess;

static clock_t CopyStartTime;

static int OrigScrX, OrigScrY;

static uint64_t TotalCopySize, TotalCopiedSize;		// Общий индикатор копирования
static uint64_t CurCopiedSize;						// Текущий индикатор копирования
static uint64_t TotalSkippedSize;					// Общий размер пропущенных файлов
static size_t CountTarget;							// всего целей.
static bool ShowTotalCopySize;
static FARString strTotalCopySizeText;

static FileFilter *Filter;
static int UseFilter = FALSE;

static clock_t ProgressUpdateTime;	// Last progress bar update time

ShellCopyFileExtendedAttributes::ShellCopyFileExtendedAttributes(File &f)
{
	_apply = (f.QueryFileExtendedAttributes(_xattr) != FB_NO && !_xattr.empty());
}

void ShellCopyFileExtendedAttributes::ApplyToCopied(File &f)
{
	if (_apply) {
		f.SetFileExtendedAttributes(_xattr);
	}
}

struct CopyDlgParam
{
	ShellCopy *thisClass;
	int AltF10;
	DWORD FileAttr;
	int SelCount;
	bool FolderPresent;
	bool FilesPresent;
	FARString strPluginFormat;
	bool AskRO;
};

enum enumShellCopy
{
	ID_SC_TITLE,
	ID_SC_TARGETTITLE,
	ID_SC_TARGETEDIT,
	ID_SC_SEPARATOR1,
	ID_SC_COMBOTEXT,
	ID_SC_COMBO,
	ID_SC_MULTITARGET,
	ID_SC_COPYACCESSMODE,
	ID_SC_COPYXATTR,
	ID_SC_WRITETHROUGH,
	ID_SC_SPARSEFILES,
	ID_SC_USECOW,
	ID_SC_COPYSYMLINK_TEXT,
	ID_SC_COPYSYMLINK_COMBO,
	ID_SC_COPYSYMLINK_EXPLAIN_TEXT,
	ID_SC_SEPARATOR3,
	ID_SC_USEFILTER,
	ID_SC_SEPARATOR4,
	ID_SC_BTNCOPY,
	ID_SC_BTNTREE,
	ID_SC_BTNFILTER,
	ID_SC_BTNCANCEL,
	ID_SC_SOURCEFILENAME,
};

enum CopyMode
{
	CM_ASK,
	CM_OVERWRITE,
	CM_SKIP,
	CM_RENAME,
	CM_APPEND,
	CM_ONLYNEWER,
	CM_SEPARATOR,
	CM_ASKRO,
};

// CopyProgress start
// гнать это отсюда в отдельный файл после разбора кучи глобальных переменных вверху
class CopyProgress
{
	ConsoleTitle CopyTitle;
	wakeful W;
	SMALL_RECT Rect{};
	wchar_t Bar[100]{};
	size_t BarSize;
	bool Move, Total, Time;
	bool BgInit, ScanBgInit;
	bool IsCancelled;
	uint64_t Color;
	int Percents;
	DWORD LastWriteTime;
	FARString strSrc, strDst, strFiles, strTime;
	bool Timer();
	void Flush();
	void DrawNames();
	void CreateScanBackground();
	void SetProgress(bool TotalProgress, UINT64 CompletedSize, UINT64 TotalSize);

public:
	CopyProgress(bool Move, bool Total, bool Time);
	void CreateBackground();
	bool Cancelled() { return IsCancelled; }
	void SetScanName(const wchar_t *Name);
	void SetNames(const wchar_t *Src, const wchar_t *Dst);
	void SetProgressValue(UINT64 CompletedSize, UINT64 TotalSize)
	{
		return SetProgress(false, CompletedSize, TotalSize);
	}
	void SetTotalProgressValue(UINT64 CompletedSize, UINT64 TotalSize)
	{
		return SetProgress(true, CompletedSize, TotalSize);
	}
};

static void GetTimeText(DWORD Time, FARString &strTimeText)
{
	DWORD Sec = Time;
	DWORD Min = Sec / 60;
	Sec-= (Min * 60);
	DWORD Hour = Min / 60;
	Min-= (Hour * 60);
	strTimeText.Format(L"%02u:%02u:%02u", Hour, Min, Sec);
}

bool CopyProgress::Timer()
{
	bool Result = false;
	DWORD Time = GetProcessUptimeMSec();

	if (!LastWriteTime || (Time - LastWriteTime >= RedrawTimeout)) {
		LastWriteTime = Time;
		Result = true;
	}

	return Result;
}

void CopyProgress::Flush()
{
	if (Timer()) {
		if (Total || (TotalFilesToProcess == 1)) {
			CopyTitle.Set(L"{%d%%} %ls",
					Total ? ToPercent64(TotalCopiedSize >> 8, TotalCopySize >> 8) : Percents,
					(Move ? Msg::CopyMovingTitle : Msg::CopyCopyingTitle).CPtr());
		}
		if (!IsCancelled) {
			if (CheckForEscSilent()) {
				LockFrame LF((*FrameManager)[0]);
				IsCancelled = ConfirmAbortOp();
			}
		}
	}
}

CopyProgress::CopyProgress(bool Move, bool Total, bool Time)
	:
	BarSize(52),
	Move(Move),
	Total(Total),
	Time(Time),
	BgInit(false),
	ScanBgInit(false),
	IsCancelled(false),
	Color(FarColorToReal(COL_DIALOGTEXT)),
	Percents(0),
	LastWriteTime(0)
{}

void CopyProgress::SetScanName(const wchar_t *Name)
{
	if (!ScanBgInit) {
		CreateScanBackground();
	}

	GotoXY(Rect.Left + 5, Rect.Top + 3);
	FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(Rect.Right - Rect.Left - 9) << Name;
	Flush();
}

void CopyProgress::CreateScanBackground()
{
	for (size_t i = 0; i < BarSize; i++) {
		Bar[i] = L' ';
	}

	Bar[BarSize] = 0;
	Message(MSG_LEFTALIGN, 0, (Move ? Msg::MoveDlgTitle : Msg::CopyDlgTitle), Msg::CopyScanning, Bar);
	int MX1, MY1, MX2, MY2;
	GetMessagePosition(MX1, MY1, MX2, MY2);
	Rect.Left = MX1;
	Rect.Right = MX2;
	Rect.Top = MY1;
	Rect.Bottom = MY2;
	ScanBgInit = true;
}

void CopyProgress::CreateBackground()
{
	for (size_t i = 0; i < BarSize; i++) {
		Bar[i] = L' ';
	}

	Bar[BarSize] = 0;

	if (!Total) {
		if (!Time) {
			Message(MSG_LEFTALIGN, 0, (Move ? Msg::MoveDlgTitle : Msg::CopyDlgTitle),
					(Move ? Msg::CopyMoving : Msg::CopyCopying), L"", Msg::CopyTo, L"", Bar, L"\x1", L"");
		} else {
			Message(MSG_LEFTALIGN, 0, (Move ? Msg::MoveDlgTitle : Msg::CopyDlgTitle),
					(Move ? Msg::CopyMoving : Msg::CopyCopying), L"", Msg::CopyTo, L"", Bar, L"\x1", L"",
					L"\x1", L"");
		}
	} else {
		FARString strTotalSeparator(L"\x1 ");
		strTotalSeparator+= Msg::CopyDlgTotal;
		strTotalSeparator+= L": ";
		strTotalSeparator+= strTotalCopySizeText;
		strTotalSeparator+= L" ";

		if (!Time) {
			Message(MSG_LEFTALIGN, 0, (Move ? Msg::MoveDlgTitle : Msg::CopyDlgTitle),
					(Move ? Msg::CopyMoving : Msg::CopyCopying), L"", Msg::CopyTo, L"", Bar,
					strTotalSeparator, Bar, L"\x1", L"");
		} else {
			Message(MSG_LEFTALIGN, 0, (Move ? Msg::MoveDlgTitle : Msg::CopyDlgTitle),
					(Move ? Msg::CopyMoving : Msg::CopyCopying), L"", Msg::CopyTo, L"", Bar,
					strTotalSeparator, Bar, L"\x1", L"", L"\x1", L"");
		}
	}

	int MX1, MY1, MX2, MY2;
	GetMessagePosition(MX1, MY1, MX2, MY2);
	Rect.Left = MX1;
	Rect.Right = MX2;
	Rect.Top = MY1;
	Rect.Bottom = MY2;
	BgInit = true;
	DrawNames();
}

void CopyProgress::DrawNames()
{
	Text(Rect.Left + 5, Rect.Top + 3, Color, strSrc);
	Text(Rect.Left + 5, Rect.Top + 5, Color, strDst);
	Text(Rect.Left + 5, Rect.Top + (Total ? 10 : 8), Color, strFiles);
}

void CopyProgress::SetNames(const wchar_t *Src, const wchar_t *Dst)
{
	if (!BgInit) {
		CreateBackground();
	}

	FormatString FString;
	FString << fmt::Cells() << fmt::LeftAlign() << fmt::Size(Rect.Right - Rect.Left - 9) << Src;
	strSrc = std::move(FString.strValue());
	FString.Clear();
	FString << fmt::Cells() << fmt::LeftAlign() << fmt::Size(Rect.Right - Rect.Left - 9) << Dst;
	strDst = std::move(FString.strValue());

	if (Total) {
		strFiles.Format(Msg::CopyProcessedTotal, TotalFiles, TotalFilesToProcess);
	} else {
		strFiles.Format(Msg::CopyProcessed, TotalFiles);
	}

	DrawNames();
	Flush();
}

void CopyProgress::SetProgress(bool TotalProgress, UINT64 CompletedSize, UINT64 TotalSize)
{
	if (!BgInit) {
		CreateBackground();
	}

	if (Total == TotalProgress) {
	}

	UINT64 OldCompletedSize = CompletedSize;
	UINT64 OldTotalSize = TotalSize;
	CompletedSize>>= 8;
	TotalSize>>= 8;
	CompletedSize = Min(CompletedSize, TotalSize);
	COORD BarCoord = {(SHORT)(Rect.Left + 5), (SHORT)(Rect.Top + (TotalProgress ? 8 : 6))};
	size_t BarLength = Rect.Right - Rect.Left - 9 - 5;	//-5 для процентов
	size_t Length = TotalSize
			? static_cast<size_t>((TotalSize < 1000000 ? CompletedSize : CompletedSize / 100) * BarLength
					/ (TotalSize < 1000000 ? TotalSize : TotalSize / 100))
			: BarLength;

	for (size_t i = 0; i < BarLength; i++) {
		Bar[i] = BoxSymbols[BS_X_B0];
	}

	if (TotalSize) {
		for (size_t i = 0; i < Length; i++) {
			Bar[i] = BoxSymbols[BS_X_DB];
		}
	}

	Bar[BarLength] = 0;
	Percents = ToPercent64(CompletedSize, TotalSize);
	FormatString strPercents;
	strPercents << fmt::Expand(4) << Percents << L"%";
	Text(BarCoord.X, BarCoord.Y, Color, Bar);
	Text(static_cast<int>(BarCoord.X + BarLength), BarCoord.Y, Color, strPercents);

	if (Time && (!Total || TotalProgress)) {
		DWORD WorkTime = GetProcessUptimeMSec() - CopyStartTime;
		UINT64 SizeLeft = (OldTotalSize > OldCompletedSize) ? (OldTotalSize - OldCompletedSize) : 0;
		long CalcTime = OldCalcTime;

		if (WaitUserTime != -1)		// -1 => находимся в процессе ожидания ответа юзера
		{
			OldCalcTime = CalcTime = WorkTime - WaitUserTime;
		}

		WorkTime/= 1000;
		CalcTime/= 1000;

		if (!WorkTime) {
			strTime.Format(Msg::CopyTimeInfo, L" ", L" ", L" ");
		} else {
			if (TotalProgress) {
				OldCompletedSize = OldCompletedSize - TotalSkippedSize;
			}

			UINT64 CPS = CalcTime ? OldCompletedSize / CalcTime : 0;
			DWORD TimeLeft = static_cast<DWORD>(CPS ? SizeLeft / CPS : 0);
			FARString strSpeed;
			FileSizeToStr(strSpeed, CPS, 8, COLUMN_FLOATSIZE | COLUMN_COMMAS);
			FARString strWorkTimeStr, strTimeLeftStr;
			GetTimeText(WorkTime, strWorkTimeStr);
			GetTimeText(TimeLeft, strTimeLeftStr);
			if (strSpeed.At(0) == L' ' && strSpeed.At(strSpeed.GetLength() - 1) >= L'0'
					&& strSpeed.At(strSpeed.GetLength() - 1) <= L'9') {
				strSpeed.LShift(1);
				strSpeed+= L" ";
			}
			strTime.Format(Msg::CopyTimeInfo, strWorkTimeStr.CPtr(), strTimeLeftStr.CPtr(), strSpeed.CPtr());
		}

		Text(Rect.Left + 5, Rect.Top + (Total ? 12 : 10), Color, strTime);
	}

	Flush();
}
// CopyProgress end

static CopyProgress *CP = nullptr;

/*
	$ 25.05.2002 IS
	+ Всегда работаем с реальными _длинными_ именами, в результате чего
	отлавливается ситуация, когда
	Src="D:\Program Files\filename"
	Dest="D:\PROGRA~1\filename"
	("D:\PROGRA~1" - короткое имя для "D:\Program Files")
	считается, что имена тоже одинаковые, а раньше считалось,
	что они разные (функция не знала, что и в первом, и во втором случае
	путь один и тот же)
	! Оптимизация - "велосипед" заменен на DeleteEndSlash
	! Убираем всю самодеятельность по проверке имен с разным
	регистром из функции прочь, потому что это нужно делать только при
	переименовании, а функция вызывается и при копировании тоже.
	Теперь функция вернет 1, для случая имен src=path\filename,
	dest=path\filename (раньше возвращала 2 - т.е. сигнал об ошибке).
*/

bool ShellCopy::CmpFullNames(const wchar_t *Src, const wchar_t *Dest) const
{
	FARString strSrcFullName, strDestFullName;

	// получим полные пути с учетом символических связей
	ConvertNameToFull(Src, strSrcFullName);
	ConvertNameToFull(Dest, strDestFullName);
	DeleteEndSlash(strSrcFullName);
	DeleteEndSlash(strDestFullName);

	return CmpNames(strSrcFullName, strDestFullName);
}

bool ShellCopy::CmpNames(const wchar_t *Src, const wchar_t *Dest) const
{
	if (CaseInsensitiveFS)
		return StrCmpI(Src, Dest) == 0;

	return StrCmp(Src, Dest) == 0;
}

static FARString &GetParentFolder(const wchar_t *Src, FARString &strDest)
{
	strDest = Src;
	CutToSlash(strDest, true);
	return strDest;
}

static int CmpFullPath(const wchar_t *Src, const wchar_t *Dest)
{
	FARString strSrcFullName, strDestFullName;

	GetParentFolder(Src, strSrcFullName);
	GetParentFolder(Dest, strDestFullName);
	DeleteEndSlash(strSrcFullName);
	DeleteEndSlash(strDestFullName);

	return !StrCmp(strSrcFullName, strDestFullName);
}

static void GenerateName(FARString &strName, const wchar_t *Path = nullptr)
{
	if (Path && *Path) {
		FARString strTmp = Path;
		AddEndSlash(strTmp);
		strTmp+= PointToName(strName);
		strName = strTmp;
	}

	FARString strExt = PointToExt(strName);
	size_t NameLength = strName.GetLength() - strExt.GetLength();

	for (int i = 1; apiPathExists(strName); i++) {
		WCHAR Suffix[20] = L"_";
		_itow(i, Suffix + 1, 10);
		strName.Truncate(NameLength);
		strName+= Suffix;
		strName+= strExt;
	}
}

void PR_ShellCopyMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();

	if (preRedrawItem.Param.Param1) {
		((CopyProgress *)preRedrawItem.Param.Param1)->CreateBackground();
	}
}

#define USE_PAGE_SIZE 0x1000
template <class T>
static T AlignPageUp(T v)
{
	// todo: use actual system page size
	uintptr_t al = ((uintptr_t)v) & (USE_PAGE_SIZE - 1);
	if (al) {
		v = (T)(((uintptr_t)v) + (USE_PAGE_SIZE - al));
	}
	return v;
}

ShellCopyBuffer::ShellCopyBuffer()
	:
	Capacity(AlignPageUp((DWORD)COPY_BUFFER_SIZE)),
	Size(std::min((DWORD)COPY_PIECE_MINIMAL, Capacity)),
	// allocate page-aligned memory: IO works faster on that, also direct-io requires buffer to be aligned sometimes
	// OSX lacks aligned_malloc so do it manually
	Buffer(new char[Capacity + USE_PAGE_SIZE]),
	Ptr(AlignPageUp(Buffer))
{}

ShellCopyBuffer::~ShellCopyBuffer()
{
	delete[] Buffer;
}

ShellCopy::ShellCopy(Panel *SrcPanel,		// исходная панель (активная)
		int Move,							// =1 - операция Move
		int Link,							// =1 - Sym/Hard Link
		int CurrentOnly,					// =1 - только текущий файл, под курсором
		int Ask,							// =1 - выводить диалог?
		int &ToPlugin,						// =?
		const wchar_t *PluginDestPath, bool ToSubdir)
	:
	RPT(RP_EXACTCOPY)
{
	Flags.ErrorMessageFlags = MSG_WARNING | MSG_ERRORTYPE;
	if (Opt.NotifOpt.OnFileOperation) {
		Flags.ErrorMessageFlags|= MSG_DISPLAYNOTIFY;
	}
	Filter = nullptr;
	DestList.SetParameters(0, 0, ULF_UNIQUE);
	CopyDlgParam CDP{};
	if (!(CDP.SelCount = SrcPanel->GetSelCount()))
		return;

	FARString strSelName;

	if (CDP.SelCount == 1) {
		SrcPanel->GetSelNameCompat(nullptr, CDP.FileAttr);	//????
		SrcPanel->GetSelNameCompat(&strSelName, CDP.FileAttr);

		if (TestParentFolderName(strSelName))
			return;
	}

	// Создадим объект фильтра
	Filter = new FileFilter(SrcPanel, FFT_COPY);
	// $ 26.05.2001 OT Запретить перерисовку панелей во время копирования
	_tran(SysLog(L"call (*FrameManager)[0]->LockRefresh()"));
	(*FrameManager)[0]->Lock();

	// Progress bar update threshold
	CDP.thisClass = this;
	CDP.AltF10 = 0;
	CDP.FolderPresent = false;
	CDP.FilesPresent = false;
	if (Move)
		Flags.MOVE = true;
	if (Link)
		Flags.LINK = true;
	if (CurrentOnly)
		Flags.CURRENTONLY = true;
	ShowTotalCopySize = Opt.CMOpt.CopyShowTotal != 0;
	strTotalCopySizeText.Clear();
	SelectedFolderNameLength = 0;
	int DestPlugin = ToPlugin;
	ToPlugin = FALSE;
	this->SrcPanel = SrcPanel;
	DestPanel = CtrlObject->Cp()->GetAnotherPanel(SrcPanel);
	DestPanelMode = DestPlugin ? DestPanel->GetMode() : NORMAL_PANEL;
	SrcPanelMode = SrcPanel->GetMode();

	/*
	 ***********************************************************************
	 *** Prepare Dialog Controls
	 ***********************************************************************
	 */
	int msh = Move ? 1 : 0;
	int DLG_HEIGHT = 19 + msh, DLG_WIDTH = 76;

	DialogDataEx CopyDlgData[] = {
		{DI_DOUBLEBOX, 3,  1,  (SHORT)(DLG_WIDTH - 4), (SHORT)(DLG_HEIGHT - 2), {}, 0, Msg::CopyDlgTitle},
		{DI_TEXT,      5,  2,  0,  2,  {}, 0, (Link ? Msg::CMLTargetIN : Msg::CMLTargetTO)},
		{DI_EDIT,      5,  3,  70, 3,  {reinterpret_cast<DWORD_PTR>(L"Copy")}, DIF_FOCUS | DIF_HISTORY | DIF_EDITEXPAND | DIF_USELASTHISTORY | DIF_EDITPATH, L""},
		{DI_TEXT,      3,  4,  0,  4,  {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      5,  5,  0,  5,  {}, 0, Msg::CopyIfFileExist},
		{DI_COMBOBOX,  29, 5,  70, 5,  {}, DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE, L""},
		{DI_CHECKBOX,  5,  6,  0,  6,  {}, 0, Msg::CopyMultiActions},
		{DI_CHECKBOX,  5,  7,  0,  7,  {}, 0, Msg::CopyAccessMode},
		{DI_CHECKBOX,  5,  8,  0,  8,  {}, 0, Msg::CopyXAttr},
		{DI_CHECKBOX,  5,  9,  0,  9,  {}, 0, Msg::CopyWriteThrough},
		{DI_CHECKBOX,  5,  10, 0,  10, {}, 0, Msg::CopySparseFiles},
		{DI_CHECKBOX,  5,  11, 0,  11, {}, 0, Msg::CopyUseCOW},
		{DI_TEXT,      5,  12, 0,  12, {}, 0, Msg::CopySymLinkText},
		{DI_COMBOBOX,  29, 12, 70, 12, {}, DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE, L""},
		{DI_TEXT,      5,  13, 0,  13, {}, DIF_HIDDEN, Msg::LinkCopyMoveExplainText},
		{DI_TEXT,      3,  (SHORT)(13 + msh), 0,  (SHORT)(13 + msh), {}, DIF_SEPARATOR, L""},
		{DI_CHECKBOX,  5,  (SHORT)(14 + msh), 0,  (SHORT)(14 + msh), {UseFilter ? BSTATE_CHECKED : BSTATE_UNCHECKED}, DIF_AUTOMATION, Msg::CopyUseFilter},
		{DI_TEXT,      3,  (SHORT)(15 + msh), 0,  (SHORT)(15 + msh), {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0,  (SHORT)(16 + msh), 0,  (SHORT)(16 + msh), {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::CopyDlgCopy},
		{DI_BUTTON,    0,  (SHORT)(16 + msh), 0,  (SHORT)(16 + msh), {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::CopyDlgTree},
		{DI_BUTTON,    0,  (SHORT)(16 + msh), 0,  (SHORT)(16 + msh), {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE | DIF_AUTOMATION | (UseFilter ? 0 : DIF_DISABLE), Msg::CopySetFilter},
		{DI_BUTTON,    0,  (SHORT)(16 + msh), 0,  (SHORT)(16 + msh), {}, DIF_CENTERGROUP, Msg::CopyDlgCancel},
		{DI_TEXT,      5,  2,  0,  2,  {}, DIF_SHOWAMPERSAND, L""}
	};
	MakeDialogItemsEx(CopyDlgData, CopyDlg);
	CopyDlg[ID_SC_MULTITARGET].Selected = Opt.CMOpt.MultiCopy;
	CopyDlg[ID_SC_WRITETHROUGH].Selected = Opt.CMOpt.WriteThrough;
	CopyDlg[ID_SC_SPARSEFILES].Selected = Opt.CMOpt.SparseFiles;
#ifdef COW_SUPPORTED
	CopyDlg[ID_SC_USECOW].Selected = Opt.CMOpt.UseCOW && (Opt.CMOpt.SparseFiles == 0);
#else
	CopyDlg[ID_SC_USECOW].Flags|= DIF_DISABLE | DIF_HIDDEN;
#endif
	CopyDlg[ID_SC_COPYACCESSMODE].Selected = Opt.CMOpt.CopyAccessMode;
	CopyDlg[ID_SC_COPYXATTR].Selected = Opt.CMOpt.CopyXAttr;

	FarList SymLinkHowComboList;
	FarListItem SymLinkHowTypeItems[3]{};

	if (Link) {
		CopyDlg[ID_SC_COMBOTEXT].strData = Msg::LinkType;
		//		CopyDlg[ID_SC_SEPARATOR2].Flags|= DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_COPYSYMLINK_TEXT].Flags|= DIF_DISABLE | DIF_HIDDEN;
		CopyDlg[ID_SC_COPYSYMLINK_COMBO].Flags|= DIF_DISABLE | DIF_HIDDEN;
		CopyDlg[ID_SC_SPARSEFILES].Flags|= DIF_DISABLE | DIF_HIDDEN;
		CopyDlg[ID_SC_USECOW].Flags|= DIF_DISABLE | DIF_HIDDEN;

	} else {
		SymLinkHowTypeItems[0].Text = Msg::LinkCopyAsIs;
		SymLinkHowTypeItems[1].Text = Msg::LinkCopySmart;
		SymLinkHowTypeItems[2].Text = Msg::LinkCopyContent;
		SymLinkHowComboList.ItemsNumber = ARRAYSIZE(SymLinkHowTypeItems);
		SymLinkHowComboList.Items = SymLinkHowTypeItems;
		SymLinkHowTypeItems[std::min(Opt.CMOpt.HowCopySymlink, (int)ARRAYSIZE(SymLinkHowTypeItems) - 1)]
				.Flags|= LIF_SELECTED;

		CopyDlg[ID_SC_COPYSYMLINK_COMBO].ListItems = &SymLinkHowComboList;

		if (Move)		// секция про перенос
		{
			CopyDlg[ID_SC_MULTITARGET].Selected = 0;
			CopyDlg[ID_SC_MULTITARGET].Flags|= DIF_DISABLE;
			CopyDlg[ID_SC_COPYSYMLINK_EXPLAIN_TEXT].Flags = DIF_DISABLE;
		}
		//		else // секция про копирование
		//		{
		//			CopyDlg[ID_SC_COPYSYMLINKOUTER].Selected=1;
		//		}
	}

	FARString strCopyStr;

	if (CDP.SelCount == 1) {
		if (SrcPanel->GetType() == TREE_PANEL) {
			FARString strNewDir(strSelName);
			size_t pos;

			if (FindLastSlash(pos, strNewDir)) {
				strNewDir.Truncate(pos);

				if (!pos || strNewDir.At(pos - 1) == L':')
					strNewDir+= WGOOD_SLASH;

				FarChDir(strNewDir);
			}
		}

		FARString strSelNameShort = strSelName;
		strCopyStr = (Move ? Msg::MoveFile : (Link ? Msg::LinkFile : Msg::CopyFile));
		TruncPathStr(strSelNameShort,
				static_cast<int>(
						CopyDlg[ID_SC_TITLE].X2 - CopyDlg[ID_SC_TITLE].X1 - strCopyStr.GetLength() - 7));
		strCopyStr+= L" " + strSelNameShort;

		// Если копируем одиночный файл, то запрещаем использовать фильтр
		if (!(CDP.FileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
			CopyDlg[ID_SC_USEFILTER].Selected = 0;
			CopyDlg[ID_SC_USEFILTER].Flags|= DIF_DISABLE;
		}
	} else		// Объектов несколько!
	{
		auto NOper = Msg::CopyFiles;

		if (Move)
			NOper = Msg::MoveFiles;
		else if (Link)
			NOper = Msg::LinkFiles;

		// коррекция языка - про окончания
		FormatString StrItems;
		StrItems << CDP.SelCount;
		size_t LenItems = StrItems.strValue().GetLength();
		auto NItems = Msg::CMLItemsA;

		if (LenItems > 0) {
			if ((LenItems >= 2 && StrItems[LenItems - 2] == '1') || StrItems[LenItems - 1] >= '5'
					|| StrItems[LenItems - 1] == '0')
				NItems = Msg::CMLItemsS;
			else if (StrItems[LenItems - 1] == '1')
				NItems = Msg::CMLItems0;
		}

		strCopyStr.Format(NOper, CDP.SelCount, NItems.CPtr());
	}

	CopyDlg[ID_SC_SOURCEFILENAME].strData = strCopyStr;
	CopyDlg[ID_SC_TITLE].strData =
			(Move ? Msg::MoveDlgTitle : (Link ? Msg::LinkDlgTitle : Msg::CopyDlgTitle));
	CopyDlg[ID_SC_BTNCOPY].strData =
			(Move ? Msg::CopyDlgRename : (Link ? Msg::CopyDlgLink : Msg::CopyDlgCopy));

	if (DestPanelMode == PLUGIN_PANEL) {
		// Если противоположная панель - плагин, то дисаблим OnlyNewer //?????
		/*
				CDP.CopySecurity=2;
				CopyDlg[ID_SC_ACCOPY].Selected=0;
				CopyDlg[ID_SC_ACINHERIT].Selected=0;
				CopyDlg[ID_SC_ACLEAVE].Selected=1;
				CopyDlg[ID_SC_ACCOPY].Flags|=DIF_DISABLE;
				CopyDlg[ID_SC_ACINHERIT].Flags|=DIF_DISABLE;
				CopyDlg[ID_SC_ACLEAVE].Flags|=DIF_DISABLE;
		*/
	}

	FARString strDestDir;
	DestPanel->GetCurDir(strDestDir);
	if (ToSubdir) {
		AddEndSlash(strDestDir);
		FARString strSubdir;
		DestPanel->GetCurName(strSubdir);
		strDestDir+= strSubdir;
	}
	FARString strSrcDir;
	SrcPanel->GetCurDir(strSrcDir);

	if (CurrentOnly) {
		// При копировании только элемента под курсором берем его имя в кавычки, если оно содержит разделители.
		CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;

		if (!Move && CopyDlg[ID_SC_TARGETEDIT].strData.ContainsAnyOf(",;")) {
			Unquote(CopyDlg[ID_SC_TARGETEDIT].strData);			// уберем все лишние кавычки
			InsertQuote(CopyDlg[ID_SC_TARGETEDIT].strData);		// возьмем в кавычки, т.к. могут быть разделители
		}
	} else {
		switch (DestPanelMode) {
			case NORMAL_PANEL: {
				if ((strDestDir.IsEmpty() || !DestPanel->IsVisible() || !StrCmp(strSrcDir, strDestDir))
						&& CDP.SelCount == 1)
					CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;
				else {
					CopyDlg[ID_SC_TARGETEDIT].strData = strDestDir;
					AddEndSlash(CopyDlg[ID_SC_TARGETEDIT].strData);
				}

				/*
					$ 19.07.2003 IS
					Если цель содержит разделители, то возьмем ее в кавычки, дабы не получить
					ерунду при F5, Enter в панелях, когда пользователь включит MultiCopy
				*/
				if (!Move && CopyDlg[ID_SC_TARGETEDIT].strData.ContainsAnyOf(",;")) {
					Unquote(CopyDlg[ID_SC_TARGETEDIT].strData);			// уберем все лишние кавычки
					InsertQuote(CopyDlg[ID_SC_TARGETEDIT].strData);		// возьмем в кавычки, т.к. могут быть разделители
				}

				break;
			}

			case PLUGIN_PANEL: {
				OpenPluginInfo Info;
				DestPanel->GetOpenPluginInfo(&Info);
				FARString strFormat = Info.Format;
				CopyDlg[ID_SC_TARGETEDIT].strData = strFormat + L":";

				while (CopyDlg[ID_SC_TARGETEDIT].strData.GetLength() < 2)
					CopyDlg[ID_SC_TARGETEDIT].strData+= L":";

				CDP.strPluginFormat = CopyDlg[ID_SC_TARGETEDIT].strData;
				CDP.strPluginFormat.Upper();
				break;
			}
		}
	}

	FARString strInitDestDir = CopyDlg[ID_SC_TARGETEDIT].strData;
	// Для фильтра
	FAR_FIND_DATA_EX fd;
	SrcPanel->GetSelNameCompat(nullptr, CDP.FileAttr);

	bool AddSlash = false;

	while (SrcPanel->GetSelNameCompat(&strSelName, CDP.FileAttr, &fd)) {
		if (UseFilter) {
			if (!Filter->FileInFilter(fd))
				continue;
		}

		if (CDP.FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
			CDP.FolderPresent = true;
			AddSlash = true;
			//			break;
		} else {
			CDP.FilesPresent = true;
		}
	}

	if (Link)		// рулесы по поводу линков (предварительные!)
	{
		for (int i = ID_SC_SEPARATOR3; i <= ID_SC_BTNCANCEL; i++) {
			CopyDlg[i].Y1-= 3;
			CopyDlg[i].Y2-= 3;
		}
		CopyDlg[ID_SC_TITLE].Y2-= 3;
		DLG_HEIGHT-= 3;
	}

	// корректирем позицию " to"
	CopyDlg[ID_SC_TARGETTITLE].X1 = CopyDlg[ID_SC_TARGETTITLE].X2 =
			CopyDlg[ID_SC_SOURCEFILENAME].X1 + (int)CopyDlg[ID_SC_SOURCEFILENAME].strData.GetLength();

	/*
		$ 15.06.2002 IS
		Обработка копирования мышкой - в этом случае диалог не показывается,
		но переменные все равно инициализируются. Если произойдет неудачная
		компиляция списка целей, то покажем диалог.
	*/
	FARString strCopyDlgValue;
	if (!Ask) {
		strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
		Unquote(strCopyDlgValue);
		InsertQuote(strCopyDlgValue);

		if (!DestList.Set(strCopyDlgValue))
			Ask = TRUE;
	}

	/*
	 ***********************************************************************
	 *** Вывод и обработка диалога
	 ***********************************************************************
	 */
	if (Ask) {
		FarList ComboList;
		FarListItem LinkTypeItems[2] = {}, CopyModeItems[8] = {};

		if (Link) {
			ComboList.ItemsNumber = ARRAYSIZE(LinkTypeItems);
			ComboList.Items = LinkTypeItems;
			ComboList.Items[0].Text = Msg::LinkTypeHardlink;
			ComboList.Items[1].Text = Msg::LinkTypeSymlink;

			//ComboList.Items[CDP.FilesPresent ? 0 : 1].Flags|= LIF_SELECTED;
			ComboList.Items[(Opt.MakeLinkSuggestSymlinkAlways || CDP.FolderPresent) ? 1 : 0].Flags|= LIF_SELECTED;
		} else {
			ComboList.ItemsNumber = ARRAYSIZE(CopyModeItems);
			ComboList.Items = CopyModeItems;
			ComboList.Items[CM_ASK].Text = Msg::CopyAsk;
			ComboList.Items[CM_OVERWRITE].Text = Msg::CopyOverwrite;
			ComboList.Items[CM_SKIP].Text = Msg::CopySkipOvr;
			ComboList.Items[CM_RENAME].Text = Msg::CopyRename;
			ComboList.Items[CM_APPEND].Text = Msg::CopyAppend;
			ComboList.Items[CM_ONLYNEWER].Text = Msg::CopyOnlyNewerFiles;
			ComboList.Items[CM_ASKRO].Text = Msg::CopyAskRO;
			// if uncehcked in Options->Confirmations then disable variants & set only Overwrite
			if ( (Move && !Opt.Confirm.Move) || (!Move && !Opt.Confirm.Copy) ) {
				ComboList.Items[CM_OVERWRITE].Flags= LIF_SELECTED;
				CopyDlg[ID_SC_COMBO].Flags|= DIF_DISABLE;
				CopyDlg[ID_SC_COMBOTEXT].Flags|= DIF_DISABLE;
			}
			else
				ComboList.Items[CM_ASK].Flags = LIF_SELECTED;
			ComboList.Items[CM_SEPARATOR].Flags = LIF_SEPARATOR;

			if (Opt.Confirm.RO) {
				ComboList.Items[CM_ASKRO].Flags = LIF_CHECKED;
			}
		}

		CopyDlg[ID_SC_COMBO].ListItems = &ComboList;

		Dialog Dlg(CopyDlg, ARRAYSIZE(CopyDlg), CopyDlgProc, (LONG_PTR)&CDP);
		Dlg.SetHelp(Link ? L"HardSymLink" : L"CopyFiles");
		Dlg.SetId(Link ? HardSymLinkId : (Move ? MoveFilesId : CopyFilesId));
		Dlg.SetPosition(-1, -1, DLG_WIDTH, DLG_HEIGHT);
		Dlg.SetAutomation(ID_SC_USEFILTER, ID_SC_BTNFILTER, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
		// Dlg.Show();
		//  $ 02.06.2001 IS + Проверим список целей и поднимем тревогу, если он содержит ошибки
		int DlgExitCode;

		for (;;) {
			Dlg.ClearDone();
			Dlg.Process();
			DlgExitCode = Dlg.GetExitCode();
			// Рефреш текущему времени для фильтра сразу после выхода из диалога
			Filter->UpdateCurrentTime();

			if (DlgExitCode == ID_SC_BTNCOPY) {
				/*
					$ 03.08.2001 IS
					Запомним строчку из диалога и начинаем ее мучить в зависимости от
					состояния опции мультикопирования
				*/
				strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
				if (!Move) {
					Opt.CMOpt.MultiCopy = CopyDlg[ID_SC_MULTITARGET].Selected;
				}
				Opt.CMOpt.WriteThrough = CopyDlg[ID_SC_WRITETHROUGH].Selected;
				Opt.CMOpt.CopyAccessMode = CopyDlg[ID_SC_COPYACCESSMODE].Selected;
				Opt.CMOpt.CopyXAttr = CopyDlg[ID_SC_COPYXATTR].Selected;
				Opt.CMOpt.SparseFiles = CopyDlg[ID_SC_SPARSEFILES].Selected;
				Opt.CMOpt.UseCOW = CopyDlg[ID_SC_USECOW].Selected;

				if (!CopyDlg[ID_SC_MULTITARGET].Selected || !strCopyDlgValue.ContainsAnyOf(",;"))		// отключено multi*
				{
					// уберем лишние кавычки
					Unquote(strCopyDlgValue);
					// добавим кавычки, чтобы "список" удачно скомпилировался вне
					// зависимости от наличия разделителей в оном
					InsertQuote(strCopyDlgValue);
				}

				if (DestList.Set(strCopyDlgValue)) {
					// Запомнить признак использования фильтра. KM
					UseFilter = CopyDlg[ID_SC_USEFILTER].Selected;
					break;
				} else {
					Message(MSG_WARNING, 1, Msg::Warning, Msg::CopyIncorrectTargetList, Msg::Ok);
				}
			} else
				break;
		}

		if (DlgExitCode == ID_SC_BTNCANCEL || DlgExitCode < 0
				|| (CopyDlg[ID_SC_BTNCOPY].Flags & DIF_DISABLE)) {
			if (DestPlugin)
				ToPlugin = -1;

			return;
		}
	}

	/*
	 ***********************************************************************
	 *** Стадия подготовки данных после диалога
	 ***********************************************************************
	 */
	Flags.WRITETHROUGH = Flags.COPYACCESSMODE = Flags.COPYXATTR = Flags.SPARSEFILES = Flags.USECOW = false;
	Flags.SYMLINK = COPY_SYMLINK_ASIS;
	ReadOnlyDelMode = ReadOnlyOvrMode = OvrMode = SkipMode = SkipDeleteMode = -1;

	if (Link) {
		switch (CopyDlg[ID_SC_COMBO].ListPos) {
			case 0:
				RPT = RP_HARDLINK;
				break;
			case 1:
				RPT = RP_JUNCTION;
				break;
			case 2:
				RPT = RP_SYMLINK;
				break;
			case 3:
				RPT = RP_SYMLINKFILE;
				break;
			case 4:
				RPT = RP_SYMLINKDIR;
				break;
		}
	} else {
		ReadOnlyOvrMode = CDP.AskRO ? -1 : 1;

		switch (CopyDlg[ID_SC_COMBO].ListPos) {
			case CM_ASK:
				OvrMode = -1;
				break;
			case CM_OVERWRITE:
				OvrMode = 1;
				break;
			case CM_SKIP:
				OvrMode = 3;
				ReadOnlyOvrMode = CDP.AskRO ? -1 : 3;
				break;
			case CM_RENAME:
				OvrMode = 5;
				break;
			case CM_APPEND:
				OvrMode = 7;
				break;
			case CM_ONLYNEWER:
				Flags.ONLYNEWERFILES = true;
				break;
		}

		Opt.CMOpt.HowCopySymlink = CopyDlg[ID_SC_COPYSYMLINK_COMBO].ListPos;
		switch (Opt.CMOpt.HowCopySymlink) {
			case 1:
				Flags.SYMLINK = COPY_SYMLINK_SMART;
				break;
			case 2:
				Flags.SYMLINK = COPY_SYMLINK_ASFILE;
				break;
		}
	}

	if (DestPlugin && !StrCmp(CopyDlg[ID_SC_TARGETEDIT].strData, strInitDestDir)) {
		ToPlugin = 1;
		return;
	}

	if (Opt.CMOpt.WriteThrough)
		Flags.WRITETHROUGH = true;
	if (Opt.CMOpt.CopyAccessMode)
		Flags.COPYACCESSMODE = true;
	if (Opt.CMOpt.CopyXAttr)
		Flags.COPYXATTR = true;
	if (Opt.CMOpt.SparseFiles)
		Flags.SPARSEFILES = true;
	if (Opt.CMOpt.UseCOW && Opt.CMOpt.SparseFiles == 0)
		Flags.USECOW = true;

	if (CDP.SelCount == 1)
		AddSlash = false;	//???

	if (DestPlugin == 2) {
		if (PluginDestPath)
			strCopyDlgValue = PluginDestPath;

		return;
	}

	if ((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && SrcPanel->IsDizDisplayed())
			|| Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS) {
		CtrlObject->Cp()->LeftPanel->ReadDiz();
		CtrlObject->Cp()->RightPanel->ReadDiz();
	}

	if (!DestPlugin) {
		const auto &fs = MountInfo().GetFileSystem(strSrcDir.GetMB());
		CaseInsensitiveFS = (fs == "vfat" || fs == "exfat" || fs == "msdos");
		fprintf(stderr, "Copy source fs='%s' dir='%s'\n", fs.c_str(), strSrcDir.GetMB().c_str());
	}

	DestPanel->CloseFile();
	strDestDizPath.Clear();
	SrcPanel->SaveSelection();
	// нужно ли показывать время копирования?
	bool ShowCopyTime = (Opt.CMOpt.CopyTimeRule & COPY_RULE_FILES) != 0;
	/*
	 ***********************************************************************
	 **** Здесь все подготовительные операции закончены, можно приступать
	 **** к процессу Copy/Move/Link
	 ***********************************************************************
	 */
	int NeedDizUpdate = FALSE;
	int NeedUpdateAPanel = FALSE;
	Flags.UPDATEPPANEL = true;
	/*
		ЕСЛИ ПРИНЯТЬ В КАЧЕСТВЕ РАЗДЕЛИТЕЛЯ ПУТЕЙ, НАПРИМЕР ';',
		то нужно парсить CopyDlgValue на предмет MultiCopy и
		вызывать CopyFileTree нужное количество раз.
	*/
	{
		Flags.MOVE = false;

		if (DestList.Set(strCopyDlgValue))		// если список успешно "скомпилировался"
		{
			const wchar_t *NamePtr;
			FARString strNameTmp;
			// посчитаем количество целей.
			CountTarget = DestList.GetTotal();
			TotalFiles = 0;
			TotalCopySize = TotalCopiedSize = TotalSkippedSize = 0;
			ProgressUpdateTime = 0;

			// Запомним время начала
			if (ShowCopyTime) {
				CopyStartTime = GetProcessUptimeMSec();
				WaitUserTime = OldCalcTime = 0;
			}

			if (CountTarget > 1)
				Move = 0;

			for (size_t DLI = 0; nullptr != (NamePtr = DestList.Get(DLI)); ++DLI) {
				CurCopiedSize = 0;
				strNameTmp = NamePtr;

				if (!StrCmp(strNameTmp, L"..") && IsLocalRootPath(strSrcDir)) {
					if (!Message(MSG_WARNING, 2, Msg::Error,
								((!Move ? Msg::CannotCopyToTwoDot : Msg::CannotMoveToTwoDot)),
								Msg::CannotCopyMoveToTwoDot, Msg::CopySkip, Msg::CopyCancel))
						continue;

					break;
				}

				if (DestList.IsLastElement(DLI)) {	// для последней операции нужно учесть моменты связанные с операцией Move.
					Flags.COPYLASTTIME = true;
					if (Move)
						Flags.MOVE = true;
				}

				// Если выделенных элементов больше 1 и среди них есть каталог, то всегда
				// делаем так, чтобы на конце был '/'
				// делаем так не всегда, а только когда NameTmp не является маской.
				if (AddSlash && !strNameTmp.ContainsAnyOf("*?"))
					AddEndSlash(strNameTmp);

				if (CDP.SelCount == 1 && !CDP.FolderPresent) {
					ShowTotalCopySize = false;
					TotalFilesToProcess = 1;
				}

				if (Move) {
					if (CDP.SelCount == 1 && CDP.FolderPresent
							&& CheckUpdateAnotherPanel(SrcPanel, strSelName)) {
						NeedUpdateAPanel = TRUE;
					}
				}

				CP = new CopyProgress(Move != 0, ShowTotalCopySize, ShowCopyTime);
				// Обнулим инфу про дизы
				strDestDizPath.Clear();
				Flags.DIZREAD = false;
				// сохраним выделение
				SrcPanel->SaveSelection();
				const auto OldFlagsSYMLINK = Flags.SYMLINK;
				// собственно - один проход копирования
				// Mantis#45: Необходимо привести копирование ссылок на папки с NTFS на FAT к более логичному виду
				{
					// todo: If dst does not support symlinks
					// Flags.SYMLINK = COPY_SYMLINK_ASFILE;
				}
				PreRedraw.Push(PR_ShellCopyMsg);
				PreRedrawItem preRedrawItem = PreRedraw.Peek();
				preRedrawItem.Param.Param1 = CP;
				PreRedraw.SetParam(preRedrawItem.Param);
				int I = CopyFileTree(strNameTmp);
				PreRedraw.Pop();
				Flags.SYMLINK = OldFlagsSYMLINK;

				if (I == COPY_CANCEL) {
					NeedDizUpdate = TRUE;
					break;
				}

				// если "есть порох в пороховницах" - восстановим выделение
				if (!DestList.IsLastElement(DLI))
					SrcPanel->RestoreSelection();

				// Позаботимся о дизах.
				if (!strDestDizPath.IsEmpty()) {
					FARString strDestDizName;
					DestDiz.GetDizName(strDestDizName);
					DWORD Attr = apiGetFileAttributes(strDestDizName);
					int DestReadOnly = (Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

					if (DestList.IsLastElement(DLI))	// Скидываем только во время последней Op.
						if (Move && !DestReadOnly)
							SrcPanel->FlushDiz();

					DestDiz.Flush(strDestDizPath);
				}
			}
		}
		_LOGCOPYR(else SysLog(L"Error: DestList.Set(CopyDlgValue) return FALSE"));
	}
	/*
	 ***********************************************************************
	 *** заключительеая стадия процесса
	 *** восстанавливаем/дизим/редравим
	 ***********************************************************************
	 */

	if (NeedDizUpdate)		// при мультикопировании может быть обрыв, но нам все
	{						// равно нужно апдейтить дизы!
		if (!strDestDizPath.IsEmpty()) {
			FARString strDestDizName;
			DestDiz.GetDizName(strDestDizName);
			DWORD Attr = apiGetFileAttributes(strDestDizName);
			int DestReadOnly = (Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

			if (Move && !DestReadOnly)
				SrcPanel->FlushDiz();

			DestDiz.Flush(strDestDizPath);
		}
	}

	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (CDP.SelCount == 1 && !strRenamedName.IsEmpty())
		SrcPanel->GoToFile(strRenamedName);

	if (NeedUpdateAPanel && CDP.FileAttr != INVALID_FILE_ATTRIBUTES
			&& (CDP.FileAttr & FILE_ATTRIBUTE_DIRECTORY) && DestPanelMode != PLUGIN_PANEL) {
		FARString strTmpSrcDir;
		SrcPanel->GetCurDir(strTmpSrcDir);
		DestPanel->SetCurDir(strTmpSrcDir, FALSE);
	}

	// проверим "нужность" апдейта пассивной панели
	if (Flags.UPDATEPPANEL) {
		DestPanel->SortFileList(TRUE);
		DestPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	}

	if (SrcPanelMode == PLUGIN_PANEL)
		SrcPanel->SetPluginModified();

	CtrlObject->Cp()->Redraw();

	if (Opt.NotifOpt.OnFileOperation) {
		DisplayNotification(Msg::FileOperationComplete, strSelName);	// looks like strSelName is best choice
	}
}

LONG_PTR WINAPI CopyDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
#define DM_CALLTREE (DM_USER + 1)
#define DM_SWITCHRO (DM_USER + 2)
	CopyDlgParam *DlgParam = (CopyDlgParam *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);

	switch (Msg) {
		case DN_INITDIALOG:
			SendDlgMessage(hDlg, DM_SETCOMBOBOXEVENT, ID_SC_COMBO, CBET_KEY | CBET_MOUSE);
			SendDlgMessage(hDlg, DM_SETMOUSEEVENTNOTIFY, TRUE, 0);
			break;
		case DM_SWITCHRO: {
			FarListGetItem LGI = {CM_ASKRO};
			SendDlgMessage(hDlg, DM_LISTGETITEM, ID_SC_COMBO, (LONG_PTR)&LGI);

			if (LGI.Item.Flags & LIF_CHECKED)
				LGI.Item.Flags&= ~LIF_CHECKED;
			else
				LGI.Item.Flags|= LIF_CHECKED;

			SendDlgMessage(hDlg, DM_LISTUPDATE, ID_SC_COMBO, (LONG_PTR)&LGI);
			SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return TRUE;
		}
		case DN_BTNCLICK: {

			if (Param1 == ID_SC_USEFILTER)		// "Use filter"
			{
				UseFilter = (int)Param2;
				return TRUE;
			}

			if (Param1 == ID_SC_BTNTREE)	// Tree
			{
				SendDlgMessage(hDlg, DM_CALLTREE, 0, 0);
				return FALSE;
			} else if (Param1 == ID_SC_BTNCOPY) {
				SendDlgMessage(hDlg, DM_CLOSE, ID_SC_BTNCOPY, 0);
			} else if (Param1 == ID_SC_SPARSEFILES) {
				SendDlgMessage(hDlg, DM_SETCHECK, ID_SC_USECOW, BSTATE_UNCHECKED);
			} else if (Param1 == ID_SC_USECOW) {
				SendDlgMessage(hDlg, DM_SETCHECK, ID_SC_SPARSEFILES, BSTATE_UNCHECKED);
			}
			/*
			else if(Param1 == ID_SC_ONLYNEWER && (DlgParam->thisClass->Flags.LINK))
			{
				// подсократим код путем эмуляции телодвижений в строке ввода :-))
				SendDlgMessage(hDlg,DN_EDITCHANGE,ID_SC_TARGETEDIT,0);
			}
			*/
			else if (Param1 == ID_SC_BTNFILTER)		// Filter
			{
				Filter->FilterEdit();
				return TRUE;
			}

			break;
		}
		case DM_KEY:	// по поводу дерева!
		{
			if (Param2 == KEY_ALTF10 || Param2 == KEY_F10 || Param2 == KEY_SHIFTF10) {
				DlgParam->AltF10 = Param2 == KEY_ALTF10 ? 1 : (Param2 == KEY_SHIFTF10 ? 2 : 0);
				SendDlgMessage(hDlg, DM_CALLTREE, DlgParam->AltF10, 0);
				return TRUE;
			}

			if (Param1 == ID_SC_COMBO) {
				if (Param2 == KEY_ENTER || Param2 == KEY_NUMENTER || Param2 == KEY_INS
						|| Param2 == KEY_NUMPAD0 || Param2 == KEY_SPACE) {
					if (SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SC_COMBO, 0) == CM_ASKRO)
						return SendDlgMessage(hDlg, DM_SWITCHRO, 0, 0);
				}
			}
		} break;

		case DN_LISTHOTKEY:
			if (Param1 == ID_SC_COMBO) {
				if (SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SC_COMBO, 0) == CM_ASKRO) {
					SendDlgMessage(hDlg, DM_SWITCHRO, 0, 0);
					return TRUE;
				}
			}
			break;
		case DN_MOUSEEVENT:

			if (SendDlgMessage(hDlg, DM_GETDROPDOWNOPENED, ID_SC_COMBO, 0)) {
				MOUSE_EVENT_RECORD *mer = (MOUSE_EVENT_RECORD *)Param2;

				if (SendDlgMessage(hDlg, DM_LISTGETCURPOS, ID_SC_COMBO, 0) == CM_ASKRO && mer->dwButtonState
						&& !(mer->dwEventFlags & MOUSE_MOVED)) {
					SendDlgMessage(hDlg, DM_SWITCHRO, 0, 0);
					return FALSE;
				}
			}

			break;

		case DM_CALLTREE: {
			/*
				$ 13.10.2001 IS
				+ При мультикопировании добавляем выбранный в "дереве" каталог к уже
				существующему списку через точку с запятой.
				- Баг: при мультикопировании выбранный в "дереве" каталог не
				заключался в кавычки, если он содержал в своем
				имени символы-разделители.
				- Баг: неправильно работало Shift-F10, если строка ввода содержала
				слеш на конце.
				- Баг: неправильно работало Shift-F10 при мультикопировании -
				показывался корневой каталог, теперь показывается самый первый каталог
				в списке.
			*/
			BOOL MultiCopy = SendDlgMessage(hDlg, DM_GETCHECK, ID_SC_MULTITARGET, 0) == BSTATE_CHECKED;
			FARString strOldFolder;
			int nLength;
			FarDialogItemData Data;
			nLength = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, ID_SC_TARGETEDIT, 0);
			Data.PtrData = strOldFolder.GetBuffer(nLength + 1);
			Data.PtrLength = nLength;
			SendDlgMessage(hDlg, DM_GETTEXT, ID_SC_TARGETEDIT, (LONG_PTR)&Data);
			strOldFolder.ReleaseBuffer();
			FARString strNewFolder;

			if (DlgParam->AltF10 == 2) {
				strNewFolder = strOldFolder;

				if (MultiCopy) {
					UserDefinedList DestList(0, 0, ULF_UNIQUE);

					if (DestList.Set(strOldFolder)) {
						const wchar_t *NamePtr = DestList.Get(0);

						if (NamePtr)
							strNewFolder = NamePtr;
					}
				}

				if (strNewFolder.IsEmpty())
					DlgParam->AltF10 = -1;
				else	// убираем лишний слеш
					DeleteEndSlash(strNewFolder);
			}

			if (DlgParam->AltF10 != -1) {
				{
					FARString strNewFolder2;
					FolderTree::Present(strNewFolder2,
							(DlgParam->AltF10 == 1
											? MODALTREE_PASSIVE
											: (DlgParam->AltF10 == 2 ? MODALTREE_FREE : MODALTREE_ACTIVE)),
							FALSE, FALSE);
					strNewFolder = strNewFolder2;
				}

				if (!strNewFolder.IsEmpty()) {
					AddEndSlash(strNewFolder);

					if (MultiCopy)		// мультикопирование
					{
						// Добавим кавычки, если имя каталога содержит символы-разделители
						if (strNewFolder.ContainsAnyOf(";,"))
							InsertQuote(strNewFolder);

						if (strOldFolder.GetLength())
							strOldFolder+= L";";	// добавим разделитель к непустому списку

						strOldFolder+= strNewFolder;
						strNewFolder = strOldFolder;
					}

					SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_SC_TARGETEDIT, (LONG_PTR)strNewFolder.CPtr());
					SendDlgMessage(hDlg, DM_SETFOCUS, ID_SC_TARGETEDIT, 0);
				}
			}

			DlgParam->AltF10 = 0;
			return TRUE;
		}
		case DN_CLOSE: {
			if (Param1 == ID_SC_BTNCOPY) {
				FarListGetItem LGI = {CM_ASKRO};
				SendDlgMessage(hDlg, DM_LISTGETITEM, ID_SC_COMBO, (LONG_PTR)&LGI);

				if (LGI.Item.Flags & LIF_CHECKED)
					DlgParam->AskRO = TRUE;
			}
		} break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

ShellCopy::~ShellCopy()
{
	_tran(SysLog(L"[%p] ShellCopy::~ShellCopy(), CopyBuffer=%p", this, CopyBuffer));

	// $ 26.05.2001 OT Разрешить перерисовку панелей
	_tran(SysLog(L"call (*FrameManager)[0]->UnlockRefresh()"));
	(*FrameManager)[0]->Unlock();
	(*FrameManager)[0]->Refresh();

	if (Filter)		// Уничтожим объект фильтра
		delete Filter;

	if (CP) {
		delete CP;
		CP = nullptr;
	}
}

COPY_CODES ShellCopy::CopyFileTree(const wchar_t *Dest)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	// SaveScreen SaveScr;
	DWORD DestAttr = INVALID_FILE_ATTRIBUTES;
	FARString strSelName;
	int Length;
	DWORD FileAttr;

	if (!(Length = StrLength(Dest)) || !StrCmp(Dest, L"."))
		return COPY_FAILURE;	//????

	SetCursorType(FALSE, 0);

	if (!TotalCopySize) {
		strTotalCopySizeText.Clear();

		// ! Не сканируем каталоги при создании линков
		if (ShowTotalCopySize && !Flags.LINK && !CalcTotalSize())
			return COPY_FAILURE;
	} else {
		CurCopiedSize = 0;
	}

	// Создание структуры каталогов в месте назначения
	FARString strNewPath = Dest;

	if (!IsSlash(strNewPath.At(strNewPath.GetLength() - 1)) && SrcPanel->GetSelCount() > 1
			&& !strNewPath.ContainsAnyOf("*?")
			&& apiGetFileAttributes(strNewPath) == INVALID_FILE_ATTRIBUTES) {
		switch (Message(FMSG_WARNING, 3, Msg::Warning, strNewPath, Msg::CopyDirectoryOrFile,
				Msg::CopyDirectoryOrFileDirectory, Msg::CopyDirectoryOrFileFile, Msg::Cancel)) {
			case 0:
				AddEndSlash(strNewPath);
				break;
			case -2:
			case -1:
			case 2:
				return COPY_CANCEL;
		}
	}

	size_t pos;

	if (FindLastSlash(pos, strNewPath)) {
		strNewPath.Truncate(pos + 1);

		DWORD Attr = apiGetFileAttributes(strNewPath);

		if (Attr == INVALID_FILE_ATTRIBUTES) {
			if (apiCreateDirectory(strNewPath, nullptr))
				TreeList::AddTreeName(strNewPath);
			else
				CreatePath(strNewPath);
		} else if (!(Attr & FILE_ATTRIBUTE_DIRECTORY)) {
			Message(MSG_WARNING, 1, Msg::Error, Msg::CopyCannotCreateFolder, strNewPath, Msg::Ok);
			return COPY_FAILURE;
		}
	}

	DestAttr = apiGetFileAttributes(Dest);

	// Выставим признак "Тот же диск"
	bool AllowMoveByOS = false;

	if (Flags.MOVE) {
		FARString strTmpSrcDir;
		SrcPanel->GetCurDir(strTmpSrcDir);
		AllowMoveByOS = (CheckDisksProps(strTmpSrcDir, Dest, CHECKEDPROPS_ISSAMEDISK)) != 0;
	}

	// Основной цикл копирования одной порции.
	SrcPanel->GetSelNameCompat(nullptr, FileAttr);
	while (SrcPanel->GetSelNameCompat(&strSelName, FileAttr)) {
		SelectedPanelItems.emplace_back();
		ConvertNameToFull(strSelName, SelectedPanelItems.back());
	}

	SrcPanel->GetSelNameCompat(nullptr, FileAttr);
	{
		while (SrcPanel->GetSelNameCompat(&strSelName, FileAttr)) {
			FARString strDest = Dest;

			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
				SelectedFolderNameLength = (int)strSelName.GetLength();
			else
				SelectedFolderNameLength = 0;

			if (strDest.ContainsAnyOf("*?"))
				ConvertWildcards(strSelName, strDest, SelectedFolderNameLength);

			DestAttr = apiGetFileAttributes(strDest);

			FARString strDestPath = strDest;
			FAR_FIND_DATA_EX SrcData;
			SrcData.Clear();
			int CopyCode = COPY_SUCCESS, KeepPathPos;
			Flags.OVERWRITENEXT = false;

			KeepPathPos = (int)(PointToName(strSelName) - strSelName.CPtr());

			if (RPT == RP_JUNCTION || RPT == RP_SYMLINK || RPT == RP_SYMLINKFILE || RPT == RP_SYMLINKDIR) {
				switch (MkSymLink(strSelName, strDest, RPT, true)) {
					case 2:
						break;
					case 1:

						// Отметим (Ins) несколько каталогов, ALT-F6 Enter - выделение с папок не снялось.
						if (!Flags.CURRENTONLY && Flags.COPYLASTTIME)
							SrcPanel->ClearLastGetSelection();

						continue;
					case 0:
						return COPY_FAILURE;
				}
			} else {
				// проверка на вшивость ;-)

				if (!apiGetFindDataForExactPathName(strSelName, SrcData)) {
					strDestPath = strSelName;
					CP->SetNames(strSelName, strDestPath);

					if (Message(MSG_WARNING, 2, Msg::Error, Msg::CopyCannotFind, strSelName, Msg::Skip,
								Msg::Cancel)
							== 1) {
						return COPY_FAILURE;
					}

					continue;
				}
				/// fprintf(stderr, "!!!!! RPT=%x for '%ls'\n", (SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT), strSelName.CPtr());
			}

			// KeepPathPos=PointToName(SelName)-SelName;

			// Мувим?
			if (Flags.MOVE) {
				// Тыкс, а как на счет "тот же диск"?
				if (KeepPathPos && PointToName(strDest) == strDest) {
					strDestPath = strSelName;
					strDestPath.Truncate(KeepPathPos);
					strDestPath+= strDest;
					AllowMoveByOS = true;
				}

				if (UseFilter || !AllowMoveByOS ||		// can't move across different devices
						// if any symlinks copy may occur - parse whole tree
						((SrcData.dwFileAttributes
									& (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
										!= 0
								&& Flags.SYMLINK != COPY_SYMLINK_ASIS)) {
					CopyCode = COPY_FAILURE;
				} else {
					CopyCode = ShellCopyOneFile(strSelName, SrcData, strDestPath, KeepPathPos, 1);

					if (CopyCode == COPY_SUCCESS_MOVE) {
						if (!strDestDizPath.IsEmpty()) {
							if (!strRenamedName.IsEmpty()) {
								DestDiz.DeleteDiz(strSelName);
								SrcPanel->CopyDiz(strSelName, strRenamedName, &DestDiz);
							} else {
								if (strCopiedName.IsEmpty())
									strCopiedName = strSelName;

								SrcPanel->CopyDiz(strSelName, strCopiedName, &DestDiz);
								SrcPanel->DeleteDiz(strSelName);
							}
						}

						continue;
					}

					if (CopyCode == COPY_CANCEL)
						return COPY_CANCEL;

					if (CopyCode == COPY_NEXT) {
						uint64_t CurSize = SrcData.nFileSize;
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
						continue;
					}

					if (!Flags.MOVE || CopyCode == COPY_FAILURE)
						Flags.OVERWRITENEXT = true;
				}
			}

			if (!Flags.MOVE || CopyCode == COPY_FAILURE) {
				FARString strCopyDest = strDest;

				CopyCode = ShellCopyOneFile(strSelName, SrcData, strCopyDest, KeepPathPos, 0);

				Flags.OVERWRITENEXT = false;

				if (CopyCode == COPY_CANCEL)
					return COPY_CANCEL;

				if (CopyCode != COPY_SUCCESS) {
					uint64_t CurSize = SrcData.nFileSize;

					if (CopyCode != COPY_NOFILTER)	//????
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;

					if (CopyCode == COPY_NEXT)
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;

					continue;
				}
			}

			if (CopyCode == COPY_SUCCESS && !strDestDizPath.IsEmpty()) {
				if (strCopiedName.IsEmpty())
					strCopiedName = strSelName;

				SrcPanel->CopyDiz(strSelName, strCopiedName, &DestDiz);
			}

			/*
				Mantis#44 - Потеря данных при копировании ссылок на папки
				если каталог (или нужно копировать симлинк) - придется рекурсивно спускаться...
			*/
			if (RPT != RP_SYMLINKFILE && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
					&& ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0
							|| ((SrcData.dwFileAttributes
										& (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN))
											== FILE_ATTRIBUTE_REPARSE_POINT
									&& Flags.SYMLINK == COPY_SYMLINK_ASFILE))) {
				int SubCopyCode;
				FARString strSubName;
				FARString strFullName;
				ScanTree ScTree(TRUE, TRUE, Flags.SYMLINK == COPY_SYMLINK_ASFILE);
				strSubName = strSelName;
				strSubName+= WGOOD_SLASH;

				if (DestAttr == INVALID_FILE_ATTRIBUTES)
					KeepPathPos = (int)strSubName.GetLength();

				int NeedRename = !((SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
						&& Flags.SYMLINK == COPY_SYMLINK_ASFILE && Flags.MOVE);
				ScTree.SetFindPath(strSubName, L"*", FSCANTREE_FILESFIRST);
				while (ScTree.GetNextName(&SrcData, strFullName)) {
					if (UseFilter && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						/*
							Просто пропустить каталог недостаточно - если каталог помечен в
							фильтре как некопируемый, то следует пропускать и его и всё его
							содержимое.
						*/
						if (!Filter->FileInFilter(SrcData)) {
							ScTree.SkipDir();
							continue;
						}
					}
					{
						int AttemptToMove = FALSE;

						if (Flags.MOVE && !UseFilter && AllowMoveByOS
								&& (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
								&& ((SrcData.dwFileAttributes
											& (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN))
												== 0
										|| Flags.SYMLINK == COPY_SYMLINK_ASIS)) {
							AttemptToMove = TRUE;
							int Ret = COPY_SUCCESS;
							FARString strCopyDest = strDest;

							Ret = ShellCopyOneFile(strFullName, SrcData, strCopyDest, KeepPathPos,
									NeedRename);

							switch (Ret)	// 1
							{
								case COPY_CANCEL:
									return COPY_CANCEL;
								case COPY_NEXT: {
									uint64_t CurSize = SrcData.nFileSize;
									TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
									TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
									continue;
								}
								case COPY_SUCCESS_MOVE: {
									continue;
								}
								case COPY_SUCCESS:

									if (!NeedRename)	// вариант при перемещении содержимого симлинка с опцией "копировать содержимое сим..."
									{
										uint64_t CurSize = SrcData.nFileSize;
										TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
										TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
										continue;	// ... т.к. мы ЭТО не мувили, а скопировали, то все, на этом закончим бодаться с этим файлов
									}
							}
						}

						int SaveOvrMode = OvrMode;

						if (AttemptToMove)
							OvrMode = 1;

						FARString strCopyDest = strDest;

						SubCopyCode = ShellCopyOneFile(strFullName, SrcData, strCopyDest, KeepPathPos, 0);

						if (AttemptToMove)
							OvrMode = SaveOvrMode;
					}

					if (SubCopyCode == COPY_CANCEL)
						return COPY_CANCEL;

					if (SubCopyCode == COPY_NEXT) {
						uint64_t CurSize = SrcData.nFileSize;
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
					}

					if (SubCopyCode == COPY_SUCCESS) {
						if (Flags.MOVE) {
							if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
								if (ScTree.IsDirSearchDone()
										|| ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
												&& (Flags.SYMLINK != COPY_SYMLINK_ASFILE))) {
									TemporaryMakeWritable tmw(strFullName);
									// if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
									//	apiMakeWritable(strFullName); //apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

									if (apiRemoveDirectory(strFullName))
										TreeList::DelTreeName(strFullName);
								}
							}
							/*
								здесь нужны проверка на FSCANTREE_INSIDEJUNCTION, иначе
								при мовинге будет удаление файла, что крайне неправильно!
							*/
							else if (!ScTree.IsInsideSymlink()) {
								if (DeleteAfterMove(strFullName, SrcData.dwFileAttributes) == COPY_CANCEL)
									return COPY_CANCEL;
							}
						}
					}
				}

				if (Flags.MOVE && CopyCode == COPY_SUCCESS) {
					TemporaryMakeWritable tmw(strSelName);
					// if (FileAttr & FILE_ATTRIBUTE_READONLY)
					//	apiMakeWritable(strSelName); //apiSetFileAttributes(strSelName,FILE_ATTRIBUTE_NORMAL);

					if (apiRemoveDirectory(strSelName)) {
						TreeList::DelTreeName(strSelName);

						if (!strDestDizPath.IsEmpty())
							SrcPanel->DeleteDiz(strSelName);
					}
				}
			} else if (Flags.MOVE && CopyCode == COPY_SUCCESS) {
				int DeleteCode;

				if ((DeleteCode = DeleteAfterMove(strSelName, FileAttr)) == COPY_CANCEL)
					return COPY_CANCEL;

				if (DeleteCode == COPY_SUCCESS && !strDestDizPath.IsEmpty())
					SrcPanel->DeleteDiz(strSelName);
			}

			if (!Flags.CURRENTONLY && Flags.COPYLASTTIME) {
				SrcPanel->ClearLastGetSelection();
			}
		}
	}

	SetEnqueuedDirectoriesAttributes();

	return COPY_SUCCESS;	// COPY_SUCCESS_MOVE???
}

void ShellCopy::EnqueueDirectoryAttributes(const FAR_FIND_DATA_EX &SrcData, FARString &strDest)
{
	DirectoriesAttributes.emplace_back();
	auto &cdb = DirectoriesAttributes.back();
	cdb.Path = strDest.GetMB();
	cdb.ftUnixAccessTime = SrcData.ftUnixAccessTime;
	cdb.ftUnixModificationTime = SrcData.ftUnixModificationTime;
	cdb.dwUnixMode = SrcData.dwUnixMode;
}

void ShellCopy::SetEnqueuedDirectoriesAttributes()
{
	std::sort(DirectoriesAttributes.begin(), DirectoriesAttributes.end(),
			[&](const CopiedDirectory &a, const CopiedDirectory &b) -> bool {
				return b.Path < a.Path;
			});
	for (const auto &cd : DirectoriesAttributes) {
		//		fprintf(stderr, "!!! '%s'\n", cd.Path.c_str());
		struct timespec ts[2] = {};
		WINPORT(FileTime_Win32ToUnix)(&cd.ftUnixAccessTime, &ts[0]);
		WINPORT(FileTime_Win32ToUnix)(&cd.ftUnixModificationTime, &ts[1]);
		if (sdc_utimens(cd.Path.c_str(), ts) == -1) {
			fprintf(stderr, "sdc_utimens error %d for '%s'\n", errno, cd.Path.c_str());
		}
		if (Flags.COPYACCESSMODE) {
			if (sdc_chmod(cd.Path.c_str(), cd.dwUnixMode) == -1) {
				fprintf(stderr, "sdc_chmod mode=0%o error %d for '%s'\n", cd.dwUnixMode, errno,
						cd.Path.c_str());
			}
		}
	}
	DirectoriesAttributes.clear();
}

bool ShellCopy::IsSymlinkTargetAlsoCopied(const wchar_t *SymLink)
{
	if (!SymLink || !*SymLink)
		return false;

	FARString strTarget;
	ConvertNameToReal(SymLink, strTarget);
	for (const auto &strItem : SelectedPanelItems) {
		if (strTarget == strItem) {
			//			fprintf(stderr, "%s('%ls'): TRUE, '%ls' matches '%ls'\n", __FUNCTION__, SymLink, strTarget.CPtr(), strItem.CPtr());
			return true;
		}
		FARString strItemReal;
		ConvertNameToReal(strItem, strItemReal);

		if (strTarget.GetLength() > strItemReal.GetLength()
				&& strTarget[strItemReal.GetLength()] == GOOD_SLASH && strTarget.Begins(strItemReal)) {
			//			fprintf(stderr, "%s('%ls'): TRUE, '%ls' under '%ls'\n", __FUNCTION__, SymLink, strTarget.CPtr(), strItemReal.CPtr());
			return true;
		}
	}

	//	fprintf(stderr, "IsSymlinkTargetAlsoCopied('%ls'): FALSE, '%ls'\n", SymLink, strTarget.CPtr());
	return false;
}

COPY_CODES
ShellCopy::CreateSymLink(const char *Target, const wchar_t *NewName, const FAR_FIND_DATA_EX &SrcData)
{
	if (apiIsDevNull(NewName))
		return COPY_SUCCESS;

	int r = sdc_symlink(Target, Wide2MB(NewName).c_str());
	if (r == 0)
		return COPY_SUCCESS;

	if (errno == EEXIST) {
		int RetCode = 0;
		bool Append = false;
		FARString strNewName = NewName, strTarget = Target;
		if (AskOverwrite(SrcData, strTarget, NewName, 0, 0, 0, 0, Append, strNewName, RetCode)) {
			if (strNewName == NewName) {
				fprintf(stderr, "CreateSymLink('%s', '%ls') - overwriting and strNewName='%ls'\n", Target,
						NewName, strNewName.CPtr());
				sdc_remove(strNewName.GetMB().c_str());

			} else {
				fprintf(stderr, "CreateSymLink('%s', '%ls') - renaming and strNewName='%ls'\n", Target,
						NewName, strNewName.CPtr());
			}
			return CreateSymLink(Target, strNewName.CPtr(), SrcData);
		}

		return (COPY_CODES)RetCode;
	}

	switch (Message(MSG_WARNING, 3, Msg::Error, Msg::CopyCannotCreateSymlinkAskCopyContents, NewName,
			Msg::Yes, Msg::Skip, Msg::Cancel)) {
		case 0:
			Flags.SYMLINK = COPY_SYMLINK_ASFILE;
			return COPY_RETRY;

		case 1:
			return COPY_FAILURE;

		case 2:
		default:
			return COPY_CANCEL;
	}
}

static bool InSameDirectory(const wchar_t *ExistingName, const wchar_t *NewName)	// #1334
{
	FARString strExistingDir, strNewDir;
	ConvertNameToFull(ExistingName, strExistingDir);
	ConvertNameToFull(NewName, strNewDir);
	CutToSlash(strExistingDir);
	CutToSlash(strNewDir);
	return strExistingDir == strNewDir;
}

COPY_CODES
ShellCopy::CopySymLink(const wchar_t *ExistingName, const wchar_t *NewName, const FAR_FIND_DATA_EX &SrcData)
{
	FARString strExistingName;
	ConvertNameToFull(ExistingName, strExistingName);

	// fprintf(stderr, "CopySymLink('%ls', '%ls', '%ls') '%ls'\n", Root, ExistingName, NewName, strRealName.CPtr());
	const std::string &mbExistingName = strExistingName.GetMB();
	char LinkTarget[PATH_MAX + 1];
	ssize_t r = sdc_readlink(mbExistingName.c_str(), LinkTarget, sizeof(LinkTarget) - 1);
	if (r <= 0 || r >= (ssize_t)sizeof(LinkTarget) || LinkTarget[0] == 0) {
		fprintf(stderr, "CopySymLink: r=%ld errno=%u from sdc_readlink('%ls')\n", (long)r, errno,
				strExistingName.CPtr());
		return COPY_FAILURE;
	}

	LinkTarget[r] = 0;

	/*
	create exactly same symlink as existing one in following cases:
	- if settings specifies to not be smart
	- if existing symlink is relative
	- if existing symlink points to unexisting destination that is also out or set of files being copied
	note that in case of being smart and if symlink is relative then caller
	guarantees that its target is within copied tree, so link will be valid
	*/
	if (Flags.SYMLINK != COPY_SYMLINK_SMART
			|| (LinkTarget[0] != GOOD_SLASH && !InSameDirectory(ExistingName, NewName))
			|| ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_BROKEN) != 0
					&& !IsSymlinkTargetAlsoCopied(ExistingName))) {
		FARString strNewName;
		ConvertNameToFull(NewName, strNewName);
		return CreateSymLink(LinkTarget, strNewName.CPtr(), SrcData);
	}

	/*
		this is a case of smart linking - create symlink that relatively points to _new_ target
		by applying to new link relative path from existing symlink to its existing target
	*/
	FARString strRealName;
	ConvertNameToReal(ExistingName, strRealName);
	std::vector<std::string> partsRealName, partsExistingName;
	StrExplode(partsRealName, strRealName.GetMB(), "/");
	StrExplode(partsExistingName, strExistingName.GetMB(), "/");

	size_t common_anchestors_count = 0;
	while (common_anchestors_count < partsRealName.size()
			&& common_anchestors_count < partsExistingName.size()
			&& partsRealName[common_anchestors_count] == partsExistingName[common_anchestors_count]) {
		++common_anchestors_count;
	}

	std::string relative_target;
	for (size_t i = common_anchestors_count; i + 1 < partsExistingName.size(); ++i) {
		relative_target+= "../";
	}
	for (size_t i = common_anchestors_count; i < partsRealName.size(); ++i) {
		relative_target+= partsRealName[i];
		if (i + 1 < partsRealName.size()) {
			relative_target+= GOOD_SLASH;
		}
	}
	if (relative_target.empty()) {
		fprintf(stderr, "CopySymLink: empty relative_target strRealName='%ls' strExistingName='%ls'\n",
				strRealName.CPtr(), strExistingName.CPtr());
	}

	return CreateSymLink(relative_target.c_str(), NewName, SrcData);
}

COPY_CODES ShellCopy::ShellCopyOneFile(const wchar_t *Src, const FAR_FIND_DATA_EX &SrcData,
		FARString &strDest, int KeepPathPos, int Rename)
{
	for (;;) {
		COPY_CODES out = ShellCopyOneFileNoRetry(Src, SrcData, strDest, KeepPathPos, Rename);
		if (out != COPY_RETRY)
			return out;
	}
}

COPY_CODES ShellCopy::ShellCopyOneFileNoRetry(const wchar_t *Src, const FAR_FIND_DATA_EX &SrcData,
		FARString &strDest, int KeepPathPos, int Rename)
{
	CurCopiedSize = 0;	// Сбросить текущий прогресс

	if (CP->Cancelled()) {
		return (COPY_CANCEL);
	}

	if (UseFilter) {
		if (!Filter->FileInFilter(SrcData))
			return COPY_NOFILTER;
	}

	FARString strDestPath = strDest;
	const wchar_t *NamePtr = PointToName(strDestPath);
	DWORD DestAttr = (strDestPath == WGOOD_SLASH || !*NamePtr || TestParentFolderName(NamePtr))
			? FILE_ATTRIBUTE_DIRECTORY
			: INVALID_FILE_ATTRIBUTES;

	FAR_FIND_DATA_EX DestData;
	DestData.Clear();
	if (DestAttr == INVALID_FILE_ATTRIBUTES) {
		if (apiGetFindDataForExactPathName(strDestPath, DestData))
			DestAttr = DestData.dwFileAttributes;
	}

	bool SameName = false, Append = false;

	if (DestAttr != INVALID_FILE_ATTRIBUTES && (DestAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		bool CmpCode = CmpFullNames(Src, strDestPath);
		if (CmpCode && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0
				&& RPT == RP_EXACTCOPY && Flags.SYMLINK != COPY_SYMLINK_ASFILE) {
			CmpCode = false;
		}

		if (CmpCode) {	// TODO: error check
			SameName = true;
			if (Rename) {
				CmpCode = CmpNames(PointToName(Src), PointToName(strDestPath));
			}

			if (CmpCode) {
				SetMessageHelp(L"ErrCopyItSelf");
				Message(MSG_WARNING, 1, Msg::Error, Msg::CannotCopyFolderToItself1, Src,
						Msg::CannotCopyFolderToItself2, Msg::Ok);
				return (COPY_CANCEL);
			}
		}

		if (!SameName) {
			int Length = (int)strDestPath.GetLength();

			if (!IsSlash(strDestPath.At(Length - 1)) && strDestPath.At(Length - 1) != L':')
				strDestPath+= WGOOD_SLASH;

			const wchar_t *PathPtr = Src + KeepPathPos;

			if (*PathPtr && !KeepPathPos && PathPtr[1] == L':')
				PathPtr+= 2;

			if (IsSlash(*PathPtr))
				PathPtr++;

			strDestPath+= PathPtr;

			if (!apiGetFindDataForExactPathName(strDestPath, DestData))
				DestAttr = INVALID_FILE_ATTRIBUTES;
			else
				DestAttr = DestData.dwFileAttributes;
		}
	}

	SetDestDizPath(strDestPath);

	CP->SetProgressValue(0, 0);
	CP->SetNames(Src, strDestPath);

	const bool copy_sym_link = (RPT == RP_EXACTCOPY
			&& (SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0
			&& ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_BROKEN) != 0
					|| (Flags.SYMLINK != COPY_SYMLINK_ASFILE
							&& (Flags.SYMLINK != COPY_SYMLINK_SMART || IsSymlinkTargetAlsoCopied(Src)
									|| InSameDirectory(Src, strDestPath)))));

	if ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || copy_sym_link) {
		if (!Rename)
			strCopiedName = PointToName(strDestPath);

		if (DestAttr != INVALID_FILE_ATTRIBUTES) {
			if ((DestAttr & FILE_ATTRIBUTE_DIRECTORY) && !SameName) {
				FARString strSrcFullName;
				ConvertNameToFull(Src, strSrcFullName);
				return (!StrCmp(strDestPath, strSrcFullName) ? COPY_NEXT : COPY_SUCCESS);
			}

			int Type = apiGetFileTypeByName(strDestPath);

			if (Type == FILE_TYPE_CHAR || Type == FILE_TYPE_PIPE)
				return (Rename ? COPY_NEXT : COPY_SUCCESS);
		}

		if ((SrcData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
				== FILE_ATTRIBUTE_DIRECTORY) {
			/*
				Enqueue attributes before creating directory, so even if will fail (like directory exists)
				but ignored then still will still try apply them on whole copy process finish successfully
			*/
			EnqueueDirectoryAttributes(SrcData, strDestPath);
		}

		if (Rename) {
			FARString strSrcFullName, strDestFullName;
			ConvertNameToFull(Src, strSrcFullName);

			// Пытаемся переименовать, пока не отменят
			for (;;) {
				BOOL SuccessMove = apiMoveFile(Src, strDestPath);

				if (SuccessMove) {
					if (PointToName(strDestPath) == strDestPath.CPtr())
						strRenamedName = strDestPath;
					else
						strCopiedName = PointToName(strDestPath);

					ConvertNameToFull(strDest, strDestFullName);
					TreeList::RenTreeName(strSrcFullName, strDestFullName);
					return (SameName ? COPY_NEXT : COPY_SUCCESS_MOVE);
				} else {
					int MsgCode = Message(Flags.ErrorMessageFlags, 3, Msg::Error,
							Msg::CopyCannotRenameFolder, Src, Msg::CopyRetry, Msg::CopyIgnore,
							Msg::CopyCancel);

					switch (MsgCode) {
						case 0:
							continue;
						case 1: {
							if (apiCreateDirectory(strDestPath, nullptr)) {
								if (PointToName(strDestPath) == strDestPath.CPtr())
									strRenamedName = strDestPath;
								else
									strCopiedName = PointToName(strDestPath);

								TreeList::AddTreeName(strDestPath);
								return (COPY_SUCCESS);
							}
						}
						default:
							return (COPY_CANCEL);
					}	/* switch */
				}		/* else */
			}			/* while */
		}				// if (Rename)
		if (RPT != RP_SYMLINKFILE
				&& (SrcData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
						== FILE_ATTRIBUTE_DIRECTORY) {
			while (!apiCreateDirectory(strDestPath, nullptr)) {
				int MsgCode = Message(Flags.ErrorMessageFlags, 3, Msg::Error, Msg::CopyCannotCreateFolder,
						strDestPath, Msg::CopyRetry, Msg::CopySkip, Msg::CopyCancel);

				if (MsgCode)
					return ((MsgCode == -2 || MsgCode == 2) ? COPY_CANCEL : COPY_NEXT);
			}
		}
		// [ ] Copy contents of symbolic links
		if (copy_sym_link) {
			COPY_CODES CopyRetCode = CopySymLink(Src, strDestPath, SrcData);
			if (CopyRetCode != COPY_SUCCESS && CopyRetCode != COPY_SUCCESS_MOVE)
				return CopyRetCode;
		}
		if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			TreeList::AddTreeName(strDestPath);
		return COPY_SUCCESS;
	}

	if (DestAttr != INVALID_FILE_ATTRIBUTES && !(DestAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (SrcData.nFileSize == DestData.nFileSize) {
			bool CmpCode = CmpFullNames(Src, strDestPath);

			if (CmpCode && SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT && RPT == RP_EXACTCOPY
					&& Flags.SYMLINK != COPY_SYMLINK_ASFILE) {
				CmpCode = false;
			}

			if (CmpCode) { // TODO: error check
				SameName = true;

				if (Rename) {
					CmpCode = CmpNames(PointToName(Src), PointToName(strDestPath));
				}

				if (CmpCode && !Rename) {
					Message(MSG_WARNING, 1, Msg::Error, Msg::CannotCopyFileToItself1, Src,
							Msg::CannotCopyFileToItself2, Msg::Ok);
					return (COPY_CANCEL);
				}
			}
		}
		int RetCode = 0;
		FARString strNewName;

		if (!AskOverwrite(SrcData, Src, strDestPath, DestAttr, SameName, Rename, (Flags.LINK ? 0 : 1), Append,
					strNewName, RetCode)) {
			return ((COPY_CODES)RetCode);
		}

		if (RetCode == COPY_RETRY) {
			strDest = strNewName;

			if (CutToSlash(strNewName) && apiGetFileAttributes(strNewName) == INVALID_FILE_ATTRIBUTES) {
				CreatePath(strNewName);
			}

			return COPY_RETRY;
		}
	}

	for (;;) {
		int CopyCode = 0;
		uint64_t SaveTotalSize = TotalCopiedSize;

		if (Rename) {
			int MoveCode = FALSE, AskDelete;

			if (!Append) {
				FARString strSrcFullName;
				ConvertNameToFull(Src, strSrcFullName);

				MoveCode = apiMoveFileEx(strSrcFullName, strDestPath,
						SameName ? MOVEFILE_COPY_ALLOWED : MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

				if (!MoveCode && WINPORT(GetLastError)() == ERROR_NOT_SAME_DEVICE) {
					return COPY_FAILURE;
				}

				if (ShowTotalCopySize && MoveCode) {
					TotalCopiedSize+= SrcData.nFileSize;
					CP->SetTotalProgressValue(TotalCopiedSize, TotalCopySize);
				}

				AskDelete = 0;
			} else {
				do {
					CopyCode = ShellCopyFile(Src, SrcData, strDestPath, Append);
				} while (CopyCode == COPY_RETRY);

				switch (CopyCode) {
					case COPY_SUCCESS:
						MoveCode = TRUE;
						break;
					case COPY_FAILUREREAD:
					case COPY_FAILURE:
						MoveCode = FALSE;
						break;
					case COPY_CANCEL:
						return COPY_CANCEL;
					case COPY_NEXT:
						return COPY_NEXT;
				}

				AskDelete = 1;
			}

			if (MoveCode) {
				if (DestAttr == INVALID_FILE_ATTRIBUTES || !(DestAttr & FILE_ATTRIBUTE_DIRECTORY)) {
					if (PointToName(strDestPath) == strDestPath.CPtr())
						strRenamedName = strDestPath;
					else
						strCopiedName = PointToName(strDestPath);
				}

				TotalFiles++;

				if (AskDelete && DeleteAfterMove(Src, SrcData.dwFileAttributes) == COPY_CANCEL)
					return COPY_CANCEL;

				return (COPY_SUCCESS_MOVE);
			}
		} else {
			do {
				CopyCode = ShellCopyFile(Src, SrcData, strDestPath, Append);
			} while (CopyCode == COPY_RETRY);

			if (Append && DestData.dwUnixMode != 0
					&& (CopyCode != COPY_SUCCESS || DestData.dwUnixMode != SrcData.dwUnixMode)) {
				const std::string &mbDestPath = strDestPath.GetMB();
				sdc_chmod(mbDestPath.c_str(), DestData.dwUnixMode);
			}
			if (CopyCode == COPY_SUCCESS) {
				strCopiedName = PointToName(strDestPath);
				TotalFiles++;
				return COPY_SUCCESS;
			} else if (CopyCode == COPY_CANCEL || CopyCode == COPY_NEXT) {
				return ((COPY_CODES)CopyCode);
			}
		}

		//????
		if (CopyCode == COPY_FAILUREREAD)
			return COPY_FAILURE;

		//????
		FARString strMsg1, strMsg2;
		FarLangMsg MsgMCannot = Flags.LINK ? Msg::CannotLink : Flags.MOVE ? Msg::CannotMove : Msg::CannotCopy;
		strMsg1 = Src;
		strMsg2 = strDestPath;
		InsertQuote(strMsg1);
		InsertQuote(strMsg2);

		int MsgCode;

		if (SkipMode != -1)
			MsgCode = SkipMode;
		else {
			MsgCode = Message(Flags.ErrorMessageFlags, 4, Msg::Error, MsgMCannot, strMsg1,
					Msg::CannotCopyTo, strMsg2, Msg::CopyRetry, Msg::CopySkip, Msg::CopySkipAll,
					Msg::CopyCancel);
		}

		switch (MsgCode) {
			case -1:
			case 1:
				return COPY_NEXT;
			case 2:
				SkipMode = 1;
				return COPY_NEXT;
			case -2:
			case 3:
				return COPY_CANCEL;
		}

		TotalCopiedSize = SaveTotalSize;
		int RetCode;
		FARString strNewName;

		if (!AskOverwrite(SrcData, Src, strDestPath, DestAttr, SameName, Rename, (Flags.LINK ? 0 : 1), Append,
					strNewName, RetCode))
			return ((COPY_CODES)RetCode);

		if (RetCode == COPY_RETRY) {
			strDest = strNewName;

			if (CutToSlash(strNewName) && apiGetFileAttributes(strNewName) == INVALID_FILE_ATTRIBUTES) {
				CreatePath(strNewName);
			}

			return COPY_RETRY;
		}
	}
}

int ShellCopy::DeleteAfterMove(const wchar_t *Name, DWORD Attr)
{
	if (Attr & FILE_ATTRIBUTE_READONLY) {
		int MsgCode;

		if (!Opt.Confirm.RO)
			ReadOnlyDelMode = 1;

		if (ReadOnlyDelMode != -1)
			MsgCode = ReadOnlyDelMode;
		else
			MsgCode = Message(MSG_WARNING, 5, Msg::Warning, Msg::CopyFileRO, Name, Msg::CopyAskDelete,
					Msg::CopyDeleteRO, Msg::CopyDeleteAllRO, Msg::CopySkipRO, Msg::CopySkipAllRO,
					Msg::CopyCancelRO);

		switch (MsgCode) {
			case 1:
				ReadOnlyDelMode = 1;
				break;
			case 2:
				return (COPY_NEXT);
			case 3:
				ReadOnlyDelMode = 3;
				return (COPY_NEXT);
			case -1:
			case -2:
			case 4:
				return (COPY_CANCEL);
		}
	}

	TemporaryMakeWritable tmw(Name);

	while ((Attr & FILE_ATTRIBUTE_DIRECTORY) ? !apiRemoveDirectory(Name) : !apiDeleteFile(Name)) {
		int MsgCode;

		if (SkipDeleteMode != -1)
			MsgCode = SkipDeleteMode;
		else
			MsgCode = Message(Flags.ErrorMessageFlags, 4, Msg::Error, Msg::CannotDeleteFile, Name,
					Msg::DeleteRetry, Msg::DeleteSkip, Msg::DeleteSkipAll, Msg::DeleteCancel);

		switch (MsgCode) {
			case 1:
				return COPY_NEXT;
			case 2:
				SkipDeleteMode = 1;
				return COPY_NEXT;
			case -1:
			case -2:
			case 3:
				return (COPY_CANCEL);
		}
	}

	return (COPY_SUCCESS);
}

static void ProgressUpdate(bool force, const FAR_FIND_DATA_EX &SrcData, const wchar_t *DestName)
{
	if (force || GetProcessUptimeMSec() - ProgressUpdateTime >= PROGRESS_REFRESH_THRESHOLD) {
		CP->SetProgressValue(CurCopiedSize, SrcData.nFileSize);

		if (ShowTotalCopySize) {
			CP->SetTotalProgressValue(TotalCopiedSize, TotalCopySize);
		}

		CP->SetNames(SrcData.strFileName, DestName);

		ProgressUpdateTime = GetProcessUptimeMSec();
	}
}

/////////////////////////////////////////////////////////// BEGIN OF ShellFileTransfer

ShellFileTransfer::ShellFileTransfer(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData,
		const FARString &strDestName, bool Append, ShellCopyBuffer &CopyBuffer, COPY_FLAGS &Flags)
	:
	_SrcName(SrcName), _strDestName(strDestName), _CopyBuffer(CopyBuffer), _Flags(Flags), _SrcData(SrcData)
{
	if (!_SrcFile.Open(SrcName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
				OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN))
		throw ErrnoSaver();

	if (_Flags.COPYXATTR) {
		_XAttrCopyPtr.reset(new ShellCopyFileExtendedAttributes(_SrcFile));
	}

	if (_Flags.COPYACCESSMODE) {	// force S_IWUSR for a while file being copied, it will be removed afterwards if not needed
		_ModeToCreateWith = _SrcData.dwUnixMode | S_IWUSR;
	}

	_DstFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	if (Flags.WRITETHROUGH) {
		_DstFlags|= FILE_FLAG_WRITE_THROUGH;

#ifdef __linux__										// anyway OSX doesn't have O_DIRECT
		if (SrcData.nFileSize > 32 * USE_PAGE_SIZE)		// just empiric
			_DstFlags|= FILE_FLAG_NO_BUFFERING;
#endif
	}

	bool DstOpened = _DestFile.Open(_strDestName, GENERIC_WRITE, FILE_SHARE_READ,
			_Flags.COPYACCESSMODE ? &_ModeToCreateWith : nullptr, (Append ? OPEN_EXISTING : CREATE_ALWAYS),
			_DstFlags);

	if ((_DstFlags & (FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING)) != 0) {
		if (!DstOpened) {
			_DstFlags&= ~(FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING);
			DstOpened = _DestFile.Open(_strDestName, GENERIC_WRITE, FILE_SHARE_READ,
					_Flags.COPYACCESSMODE ? &_ModeToCreateWith : nullptr,
					(Append ? OPEN_EXISTING : CREATE_ALWAYS), _DstFlags);
			if (DstOpened) {
				Flags.WRITETHROUGH = false;
				fprintf(stderr, "COPY: unbuffered FAILED: 0x%x\n",
						_DstFlags & (FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING));
			}
		}	/* else
			   fprintf(stderr, "COPY: unbuffered OK: 0x%x\n", DstFlags & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING));*/
	}

	if (!DstOpened) {
		ErrnoSaver ErSr;
		_SrcFile.Close();
		_LOGCOPYR(SysLog(L"return COPY_FAILURE -> %d CreateFile=-1, LastError=%d (0x%08X)", __LINE__,
				_localLastError, _localLastError));
		throw ErSr;
	}

	if (Append) {
		_AppendPos = 0;
		if (!_DestFile.SetPointer(0, &_AppendPos, FILE_END)) {
			ErrnoSaver ErSr;
			_SrcFile.Close();
			_DestFile.SetEnd();
			_DestFile.Close();
			throw ErSr;
		}
	} else if (SrcData.nFileSize > (uint64_t)_CopyBuffer.Size && !_Flags.SPARSEFILES && !_Flags.USECOW) {
		_DestFile.AllocationHint(SrcData.nFileSize);
	}
}

ShellFileTransfer::~ShellFileTransfer()
{
	if (!_Done)
		try {
			fprintf(stderr, "~ShellFileTransfer: discarding '%ls'\n", _strDestName.CPtr());
			_SrcFile.Close();
			CP->SetProgressValue(0, 0);
			CurCopiedSize = 0;	// Сбросить текущий прогресс

			if (_AppendPos != -1) {
				_DestFile.SetPointer(_AppendPos, nullptr, FILE_BEGIN);
			}

			_DestFile.SetEnd();
			_DestFile.Close();

			if (_AppendPos == -1) {
				TemporaryMakeWritable tmw(_strDestName);
				apiDeleteFile(_strDestName);
			}

			ProgressUpdate(true, _SrcData, _strDestName);
		} catch (std::exception &ex) {
			fprintf(stderr, "~ShellFileTransfer: %s\n", ex.what());
		} catch (...) {
			fprintf(stderr, "~ShellFileTransfer: ...\n");
		}
}

void ShellFileTransfer::Do()
{
	CP->SetProgressValue(0, 0);

	for (;;) {
		ProgressUpdate(false, _SrcData, _strDestName);

		if (OrigScrX != ScrX || OrigScrY != ScrY) {
			OrigScrX = ScrX;
			OrigScrY = ScrY;
			PR_ShellCopyMsg();
		}

		if (CP->Cancelled())
			return;

		_Stopwatch = (_SrcData.nFileSize - CurCopiedSize > (uint64_t)_CopyBuffer.Size)
				? GetProcessUptimeMSec()
				: 0;

		DWORD BytesWritten = PieceCopy();
		if (BytesWritten == 0)
			break;

		CurCopiedSize+= BytesWritten;

		if (_Stopwatch != 0 && BytesWritten == _CopyBuffer.Size) {
			_Stopwatch = GetProcessUptimeMSec() - _Stopwatch;
			if (_Stopwatch < 100) {
				if (_CopyBuffer.Size < _CopyBuffer.Capacity) {
					_CopyBuffer.Size = std::min(_CopyBuffer.Size * 2, _CopyBuffer.Capacity);
					fprintf(stderr, "CopyPieceSize increased to %d\n", _CopyBuffer.Size);
				}
			} else if (_Stopwatch >= 1000 && _CopyBuffer.Size > (int)COPY_PIECE_MINIMAL) {
				_CopyBuffer.Size = std::max(_CopyBuffer.Size / 2, (DWORD)COPY_PIECE_MINIMAL);
				fprintf(stderr, "CopyPieceSize decreased to %d\n", _CopyBuffer.Size);
			}
		}

		if (ShowTotalCopySize)
			TotalCopiedSize+= BytesWritten;
	}

	_SrcFile.Close();

	if (!apiIsDevNull(_strDestName))	// avoid sudo prompt when copying to /dev/null
	{
		if (_LastWriteWasHole) {
			while (!_DestFile.SetEnd()) {
				RetryCancel(Msg::CopyWriteError, _strDestName);
			}
		}

		if (_XAttrCopyPtr)
			_XAttrCopyPtr->ApplyToCopied(_DestFile);

		if (_Flags.COPYACCESSMODE
				&& (_ModeToCreateWith != _SrcData.dwUnixMode || (g_umask & _SrcData.dwUnixMode) != 0))
			_DestFile.Chmod(_SrcData.dwUnixMode);

		_DestFile.SetTime(nullptr, nullptr, &_SrcData.ftLastWriteTime, nullptr);
	}

	if (!_DestFile.Close()) {
		/*
			#1387
			if file located on old samba share then in out of space condition
			write()-s succeed but close() reports error
		*/
		throw ErrnoSaver();
	}

	_Done = true;

	ProgressUpdate(false, _SrcData, _strDestName);
}

void ShellFileTransfer::RetryCancel(const wchar_t *Text, const wchar_t *Object)
{
	ErrnoSaver ErSr;
	_Stopwatch = 0;		// UI messes timings
	const int MsgCode =
			Message(_Flags.ErrorMessageFlags, 2, Msg::Error, Text, Object, Msg::Retry, Msg::Cancel);

	PR_ShellCopyMsg();

	if (MsgCode != 0)
		throw ErSr;
}

// returns std:::pair<OffsetOfNextHole, SizeOfNextHole> (SizeOfNextHole==0 means no holes found)
static std::pair<DWORD, DWORD> LookupNextHole(const unsigned char *Data, DWORD Size, uint64_t Offset)
{
	const DWORD Alignment = 0x1000;		// must be power of 2
	const DWORD OffsetMisalignment = DWORD(Offset) & (Alignment - 1);

	DWORD i = 0;
	if (OffsetMisalignment) {
		i+= Alignment - OffsetMisalignment;
	}

	for (; i < Size; i+= Alignment) {
		DWORD ZeroesCount = 0;
		while (i + ZeroesCount < Size && Data[i + ZeroesCount] == 0) {
			++ZeroesCount;
		}
		if (ZeroesCount >= Alignment) {
			return std::make_pair(i, (ZeroesCount / Alignment) * Alignment);
		}
	}

	return std::make_pair(Size, (DWORD)0);
}

DWORD ShellFileTransfer::PieceCopy()
{
#if defined(COW_SUPPORTED) && defined(__linux__)
	if (_Flags.USECOW)
		for (;;) {
			ssize_t sz = copy_file_range(_SrcFile.Descriptor(), nullptr, _DestFile.Descriptor(), nullptr,
					_CopyBuffer.Size, 0);

			if (sz >= 0)
				return (DWORD)sz;

			if (errno == EXDEV) {
				fprintf(stderr, "copy_file_range returned EXDEV, fallback to usual copy\n");
				break;
			}

			RetryCancel(Msg::CopyWriteError, _strDestName);
		}
#endif

	DWORD BytesRead, BytesWritten;

	while (!_SrcFile.Read(_CopyBuffer.Ptr, _CopyBuffer.Size, &BytesRead)) {
		RetryCancel(Msg::CopyReadError, _SrcName);
	}

	if (BytesRead == 0)
		return BytesRead;

	DWORD WriteSize = BytesRead;
	if ((_DstFlags & FILE_FLAG_NO_BUFFERING) != 0)
		WriteSize = AlignPageUp(WriteSize);

	BytesWritten = 0;
	if (_Flags.SPARSEFILES) {
		while (BytesWritten < WriteSize) {
			const unsigned char *Data = (const unsigned char *)_CopyBuffer.Ptr + BytesWritten;
			const std::pair<DWORD, DWORD> &NH =
					LookupNextHole(Data, WriteSize - BytesWritten, CurCopiedSize + BytesWritten);
			DWORD LeadingNonzeroesWritten = NH.first ? PieceWrite(Data, NH.first) : 0;
			BytesWritten+= LeadingNonzeroesWritten;
			if (NH.second && LeadingNonzeroesWritten == NH.first) {
				// fprintf(stderr, "!!! HOLE of size %x\n", SR.second);
				BytesWritten+= PieceWriteHole(NH.second);
			}
		}
	} else
		BytesWritten = PieceWrite(_CopyBuffer.Ptr, WriteSize);

	if (BytesWritten > BytesRead) {
		/*
			likely we written bit more due to no_buffering requires aligned io
			move backward and correct file size
		*/
		if (!_DestFile.SetPointer((INT64)BytesRead - (INT64)WriteSize, nullptr, FILE_CURRENT))
			throw ErrnoSaver();
		if (!_DestFile.SetEnd())
			throw ErrnoSaver();
		return BytesRead;
	}

	if (BytesWritten < BytesRead) {		// if written less than read then need to rewind source file by difference
		if (!_SrcFile.SetPointer((INT64)BytesWritten - (INT64)BytesRead, nullptr, FILE_CURRENT))
			throw ErrnoSaver();
	}

	return BytesWritten;
}

DWORD ShellFileTransfer::PieceWriteHole(DWORD Size)
{
	while (!_DestFile.SetPointer(Size, nullptr, FILE_CURRENT)) {
		RetryCancel(Msg::CopyWriteError, _strDestName);
	}
	_LastWriteWasHole = true;
	return Size;
}

DWORD ShellFileTransfer::PieceWrite(const void *Data, DWORD Size)
{
	DWORD BytesWritten = 0;
	while (!_DestFile.Write(Data, Size, &BytesWritten)) {
		RetryCancel(Msg::CopyWriteError, _strDestName);
	}
	_LastWriteWasHole = false;
	return BytesWritten;
}

/////////////////////////////////////////////////////////// END OF ShellFileTransfer

static dev_t GetRDev(FARString SrcName)
{
    struct stat st{};
    if (sdc_stat(SrcName.GetMB().c_str(), &st) == 0) {
        return st.st_rdev;
    }
    return 0;
}

int ShellCopy::ShellCopyFile(const wchar_t *SrcName, const FAR_FIND_DATA_EX &SrcData, FARString &strDestName,
		int Append)
{
	OrigScrX = ScrX;
	OrigScrY = ScrY;

	if (Flags.LINK) {
		if (RPT == RP_HARDLINK) {
			apiDeleteFile(strDestName);		// BUGBUG
			return (MkHardLink(SrcName, strDestName) ? COPY_SUCCESS : COPY_FAILURE);
		} else {
			return (MkSymLink(SrcName, strDestName, RPT, true) ? COPY_SUCCESS : COPY_FAILURE);
		}
	}
    if (SrcData.dwFileAttributes & (FILE_ATTRIBUTE_DEVICE_FIFO | FILE_ATTRIBUTE_DEVICE_BLOCK | FILE_ATTRIBUTE_DEVICE_CHAR)) {
        int r = (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE_FIFO)
                    ? sdc_mkfifo(strDestName.GetMB().c_str(), SrcData.dwUnixMode)
                    : sdc_mknod(strDestName.GetMB().c_str(), SrcData.dwUnixMode, GetRDev(SrcName));
        if (r == -1) {
            _localLastError = errno;
            return COPY_FAILURE;
        }
        return COPY_SUCCESS;
    }

	try {
#if defined(COW_SUPPORTED) && defined(__APPLE__)
		if (Flags.USECOW) {
			const std::string mbSrc = Wide2MB(SrcName);
			const std::string &mbDest = strDestName.GetMB();
			int r = clonefile(mbSrc.c_str(), mbDest.c_str(), 0);
			if (r == 0) {
				// fprintf(stderr, "CoW succeeded for '%s' -> '%s'\n", mbSrc.c_str(), mbDest.c_str());
				CurCopiedSize = SrcData.nFileSize;
				if (ShowTotalCopySize)
					TotalCopiedSize+= SrcData.nFileSize;

				ProgressUpdate(false, SrcData, strDestName);
				return CP->Cancelled() ? COPY_CANCEL : COPY_SUCCESS;
			}

			ErrnoSaver ErSr;
			if (ErSr.Get() != EXDEV && ErSr.Get() != ENOTSUP)
				throw ErSr;

			fprintf(stderr, "Skip CoW errno=%d for '%s' -> '%s'\n", ErSr.Get(), mbSrc.c_str(),
					mbDest.c_str());
		}
#endif

		ShellFileTransfer(SrcName, SrcData, strDestName, Append != 0, CopyBuffer, Flags).Do();
		return CP->Cancelled() ? COPY_CANCEL : COPY_SUCCESS;
	} catch (ErrnoSaver &ErSr) {
		_localLastError = ErSr.Get();
	}

	return CP->Cancelled() ? COPY_CANCEL : COPY_FAILURE;
}

void ShellCopy::SetDestDizPath(const wchar_t *DestPath)
{
	if (!Flags.DIZREAD) {
		ConvertNameToFull(DestPath, strDestDizPath);
		CutToSlash(strDestDizPath);

		if (strDestDizPath.IsEmpty())
			strDestDizPath = L".";

		if ((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && !SrcPanel->IsDizDisplayed())
				|| Opt.Diz.UpdateMode == DIZ_NOT_UPDATE)
			strDestDizPath.Clear();

		if (!strDestDizPath.IsEmpty())
			DestDiz.Read(strDestDizPath);

		Flags.DIZREAD = true;
	}
}

enum WarnDlgItems
{
	WDLG_BORDER,
	WDLG_TEXT,
	WDLG_FILENAME,
	WDLG_SEPARATOR,
	WDLG_SRCFILEBTN,
	WDLG_DSTFILEBTN,
	WDLG_SEPARATOR2,
	WDLG_CHECKBOX,
	WDLG_SEPARATOR3,
	WDLG_OVERWRITE,
	WDLG_SKIP,
	WDLG_RENAME,
	WDLG_APPEND,
	WDLG_CANCEL,
};

#define DM_OPENVIEWER DM_USER + 33

LONG_PTR WINAPI WarnDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DM_OPENVIEWER: {
			LPCWSTR ViewName = nullptr;
			FARString **WFN = reinterpret_cast<FARString **>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));

			if (WFN) {
				switch (Param1) {
					case WDLG_SRCFILEBTN:
						ViewName = *WFN[0];
						break;
					case WDLG_DSTFILEBTN:
						ViewName = *WFN[1];
						break;
				}

				FileViewer Viewer(
					// а этот трюк не даст пользователю сменить текущий каталог по CtrlF10 и этим ввести в заблуждение копир: TODODODO
					std::make_shared<FileHolder>(ViewName, true),
					FALSE, FALSE, TRUE, -1, nullptr, nullptr, FALSE);
				Viewer.SetDynamicallyBorn(FALSE);
				FrameManager->ExecuteModalEV();
				FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
			}
		} break;
		case DN_CTLCOLORDLGITEM: {
			if (Param1 == WDLG_FILENAME) {
				uint64_t *ItemColor = reinterpret_cast<uint64_t *>(Param2);
				uint64_t color = FarColorToReal(COL_WARNDIALOGTEXT);
				ItemColor[0] = color;
				ItemColor[2] = color;
				return 1;
			}
		} break;
		case DN_BTNCLICK: {
			switch (Param1) {
				case WDLG_SRCFILEBTN:
				case WDLG_DSTFILEBTN:
					SendDlgMessage(hDlg, DM_OPENVIEWER, Param1, 0);
					break;
				case WDLG_RENAME: {
					FARString **WFN =
							reinterpret_cast<FARString **>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
					FARString strDestName = *WFN[1];
					GenerateName(strDestName, *WFN[2]);

					if (SendDlgMessage(hDlg, DM_GETCHECK, WDLG_CHECKBOX, 0) == BSTATE_UNCHECKED) {
						int All = BSTATE_UNCHECKED;

						if (GetString(Msg::CopyRenameTitle, Msg::CopyRenameText, nullptr, strDestName,
									*WFN[1], L"CopyAskOverwrite",
									FIB_BUTTONS | FIB_NOAMPERSAND | FIB_EXPANDENV | FIB_CHECKBOX, &All,
									Msg::CopyRememberChoice)) {
							if (All != BSTATE_UNCHECKED) {
								*WFN[2] = *WFN[1];
								CutToSlash(*WFN[2]);
							}

							SendDlgMessage(hDlg, DM_SETCHECK, WDLG_CHECKBOX, All);
						} else {
							return TRUE;
						}
					} else {
						*WFN[1] = strDestName;
					}
				} break;
			}
		} break;
		case DN_KEY: {
			if ((Param1 == WDLG_SRCFILEBTN || Param1 == WDLG_DSTFILEBTN) && Param2 == KEY_F3) {
				SendDlgMessage(hDlg, DM_OPENVIEWER, Param1, 0);
			}
		} break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

int ShellCopy::AskOverwrite(const FAR_FIND_DATA_EX &SrcData, const wchar_t *SrcName, const wchar_t *DestName,
		DWORD DestAttr, bool SameName, bool Rename, bool AskAppend, bool &Append, FARString &strNewName,
		int &RetCode)
{
	enum
	{
		WARN_DLG_HEIGHT = 13,
		WARN_DLG_WIDTH  = 72,
	};
	DialogDataEx WarnCopyDlgData[] = {
		{DI_DOUBLEBOX, 3, 1,  WARN_DLG_WIDTH - 4, WARN_DLG_HEIGHT - 2, {}, 0, Msg::Warning},
		{DI_TEXT,      5, 2,  WARN_DLG_WIDTH - 6, 2,  {}, DIF_CENTERTEXT, Msg::CopyFileExist},
		{DI_EDIT,      5, 3,  WARN_DLG_WIDTH - 6, 3,  {}, DIF_READONLY, (wchar_t *)DestName},
		{DI_TEXT,      3, 4,  0,                  4,  {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    5, 5,  WARN_DLG_WIDTH - 6, 5,  {}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, L""},
		{DI_BUTTON,    5, 6,  WARN_DLG_WIDTH - 6, 6,  {}, DIF_BTNNOCLOSE | DIF_NOBRACKETS, L""},
		{DI_TEXT,      3, 7,  0,                  7,  {}, DIF_SEPARATOR, L""},
		{DI_CHECKBOX,  5, 8,  0,                  8,  {}, DIF_FOCUS, Msg::CopyRememberChoice},
		{DI_TEXT,      3, 9,  0,                  9,  {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0, 10, 0,                  10, {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::CopyOverwrite},
		{DI_BUTTON,    0, 10, 0,                  10, {}, DIF_CENTERGROUP, Msg::CopySkipOvr},
		{DI_BUTTON,    0, 10, 0,                  10, {}, DIF_CENTERGROUP, Msg::CopyRename},
		{DI_BUTTON,    0, 10, 0,                  10, {}, DIF_CENTERGROUP | (AskAppend ? 0 : (DIF_DISABLE | DIF_HIDDEN)), Msg::CopyAppend},
		{DI_BUTTON,    0, 10, 0,                  10, {}, DIF_CENTERGROUP,Msg::CopyCancelOvr}
	};
	FAR_FIND_DATA_EX DestData;
	DestData.Clear();
	int DestDataFilled = FALSE;
	Append = false;

	if (DestAttr == INVALID_FILE_ATTRIBUTES)
		if ((DestAttr = apiGetFileAttributes(DestName)) == INVALID_FILE_ATTRIBUTES)
			return TRUE;

	if (DestAttr & FILE_ATTRIBUTE_DIRECTORY)
		return TRUE;

	int MsgCode = OvrMode;
	FARString strDestName = DestName;

	if (OvrMode == -1) {
		int Type;

		if ((!Opt.Confirm.Copy && !Rename) || (!Opt.Confirm.Move && Rename) || SameName
				|| (Type = apiGetFileTypeByName(DestName)) == FILE_TYPE_CHAR || Type == FILE_TYPE_PIPE
				|| Flags.OVERWRITENEXT)
			MsgCode = 1;
		else {
			DestData.Clear();
			apiGetFindDataForExactPathName(DestName, DestData);
			DestDataFilled = TRUE;

			if (Flags.ONLYNEWERFILES) {
				// сравним время
				int64_t RetCompare = FileTimeDifference(&DestData.ftLastWriteTime, &SrcData.ftLastWriteTime);

				if (RetCompare < 0)
					MsgCode = 0;
				else
					MsgCode = 2;
			} else {
				FormatString strSrcFileStr, strDestFileStr;
				uint64_t SrcSize = SrcData.nFileSize;
				FILETIME SrcLastWriteTime = SrcData.ftLastWriteTime;
				if (Flags.SYMLINK == COPY_SYMLINK_ASFILE
						&& (SrcData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_BROKEN))
								== FILE_ATTRIBUTE_REPARSE_POINT) {
					FARString RealName = SrcName;
					FAR_FIND_DATA_EX FindData;
					apiGetFindDataForExactPathName(RealName, FindData);
					SrcSize = FindData.nFileSize;
					SrcLastWriteTime = FindData.ftLastWriteTime;
				}
				FormatString strSrcSizeText;
				strSrcSizeText << SrcSize;
				uint64_t DestSize = DestData.nFileSize;
				FormatString strDestSizeText;
				strDestSizeText << DestSize;
				FARString strDateText, strTimeText;
				ConvertDate(SrcLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE, TRUE);
				strSrcFileStr
						<< fmt::Cells() << fmt::LeftAlign() << fmt::Expand(17) << Msg::CopySource << L" "
						<< fmt::Size(25) << strSrcSizeText << L" " << strDateText << L" " << strTimeText;
				ConvertDate(DestData.ftLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE, TRUE);
				strDestFileStr << fmt::Cells() << fmt::LeftAlign() << fmt::Expand(17) << Msg::CopyDest << L" "
							<< fmt::Size(25) << strDestSizeText << L" " << strDateText << L" " << strTimeText;

				WarnCopyDlgData[WDLG_SRCFILEBTN].Data = strSrcFileStr;
				WarnCopyDlgData[WDLG_DSTFILEBTN].Data = strDestFileStr;
				MakeDialogItemsEx(WarnCopyDlgData, WarnCopyDlg);
				FARString strFullSrcName;
				ConvertNameToFull(SrcName, strFullSrcName);
				FARString *WFN[] = {&strFullSrcName, &strDestName, &strRenamedFilesPath};
				Dialog WarnDlg(WarnCopyDlg, ARRAYSIZE(WarnCopyDlg), WarnDlgProc, (LONG_PTR)&WFN);
				WarnDlg.SetDialogMode(DMODE_WARNINGSTYLE);
				WarnDlg.SetPosition(-1, -1, WARN_DLG_WIDTH, WARN_DLG_HEIGHT);
				WarnDlg.SetHelp(L"CopyAskOverwrite");
				WarnDlg.SetId(CopyOverwriteId);
				WarnDlg.Process();

				switch (WarnDlg.GetExitCode()) {
					case WDLG_OVERWRITE:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 1 : 0;
						break;
					case WDLG_SKIP:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 3 : 2;
						break;
					case WDLG_RENAME:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 5 : 4;
						break;
					case WDLG_APPEND:
						MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 7 : 6;
						break;
					case -1:
					case -2:
					case WDLG_CANCEL:
						MsgCode = 8;
						break;
				}
			}
		}
	}

	switch (MsgCode) {
		case 1:
			OvrMode = 1;
		case 0:
			break;
		case 3:
			OvrMode = 2;
		case 2:
			RetCode = COPY_NEXT;
			return FALSE;
		case 5:
			OvrMode = 5;
			GenerateName(strDestName, strRenamedFilesPath);
		case 4:
			RetCode = COPY_RETRY;
			strNewName = strDestName;
			break;
		case 7:
			OvrMode = 6;
		case 6:
			Append = true;
			break;
		case -1:
		case -2:
		case 8:
			RetCode = COPY_CANCEL;
			return FALSE;
	}

	if (RetCode != COPY_RETRY) {
		if ((DestAttr & FILE_ATTRIBUTE_READONLY) && !Flags.OVERWRITENEXT) {
			int MsgCode = 0;

			if (!SameName) {
				if (ReadOnlyOvrMode != -1) {
					MsgCode = ReadOnlyOvrMode;
				} else {
					if (!DestDataFilled) {
						DestData.Clear();
						apiGetFindDataForExactPathName(DestName, DestData);
					}

					FARString strDateText, strTimeText;
					FormatString strSrcFileStr, strDestFileStr;
					uint64_t SrcSize = SrcData.nFileSize;
					FormatString strSrcSizeText;
					strSrcSizeText << SrcSize;
					uint64_t DestSize = DestData.nFileSize;
					FormatString strDestSizeText;
					strDestSizeText << DestSize;
					ConvertDate(SrcData.ftLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE,
							TRUE);
					strSrcFileStr
							<< fmt::Cells() << fmt::LeftAlign() << fmt::Expand(17) << Msg::CopySource << L" "
							<< fmt::Size(25) << strSrcSizeText << L" " << strDateText << L" " << strTimeText;
					ConvertDate(DestData.ftLastWriteTime, strDateText, strTimeText, 8, FALSE, FALSE, TRUE,
							TRUE);
					strDestFileStr
							<< fmt::Cells() << fmt::LeftAlign() << fmt::Expand(17) << Msg::CopyDest << L" "
							<< fmt::Size(25) << strDestSizeText << L" " << strDateText << L" " << strTimeText;
					WarnCopyDlgData[WDLG_SRCFILEBTN].Data = strSrcFileStr;
					WarnCopyDlgData[WDLG_DSTFILEBTN].Data = strDestFileStr;
					WarnCopyDlgData[WDLG_TEXT].Data = Msg::CopyFileRO;
					WarnCopyDlgData[WDLG_OVERWRITE].Data = (Append ? Msg::CopyAppend : Msg::CopyOverwrite);
					WarnCopyDlgData[WDLG_RENAME].Type = DI_TEXT;
					WarnCopyDlgData[WDLG_RENAME].Data = L"";
					WarnCopyDlgData[WDLG_APPEND].Type = DI_TEXT;
					WarnCopyDlgData[WDLG_APPEND].Data = L"";
					MakeDialogItemsEx(WarnCopyDlgData, WarnCopyDlg);
					FARString strSrcName;
					ConvertNameToFull(SrcData.strFileName, strSrcName);
					LPCWSTR WFN[2] = {strSrcName, DestName};
					Dialog WarnDlg(WarnCopyDlg, ARRAYSIZE(WarnCopyDlg), WarnDlgProc, (LONG_PTR)&WFN);
					WarnDlg.SetDialogMode(DMODE_WARNINGSTYLE);
					WarnDlg.SetPosition(-1, -1, WARN_DLG_WIDTH, WARN_DLG_HEIGHT);
					WarnDlg.SetHelp(L"CopyFiles");
					WarnDlg.SetId(CopyReadOnlyId);
					WarnDlg.Process();

					switch (WarnDlg.GetExitCode()) {
						case WDLG_OVERWRITE:
							MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 1 : 0;
							break;
						case WDLG_SKIP:
							MsgCode = WarnCopyDlg[WDLG_CHECKBOX].Selected ? 3 : 2;
							break;
						case -1:
						case -2:
						case WDLG_CANCEL:
							MsgCode = 8;
							break;
					}
				}
			}

			switch (MsgCode) {
				case 1:
					ReadOnlyOvrMode = 1;
				case 0:
					break;
				case 3:
					ReadOnlyOvrMode = 2;
				case 2:
					RetCode = COPY_NEXT;
					return FALSE;
				case -1:
				case -2:
				case 8:
					RetCode = COPY_CANCEL;
					return FALSE;
			}
		}

		if (!SameName
				&& (DestAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
			apiMakeWritable(DestName);
	}

	return TRUE;
}

BOOL ShellCopySecuryMsg(const wchar_t *Name)
{
	static clock_t PrepareSecuryStartTime;

	if (!Name || !*Name
			|| (static_cast<DWORD>(GetProcessUptimeMSec() - PrepareSecuryStartTime)
					> Opt.ShowTimeoutDACLFiles)) {
		static int Width = 30;
		int WidthTemp;
		if (Name && *Name) {
			PrepareSecuryStartTime = GetProcessUptimeMSec();	// Первый файл рисуется всегда
			WidthTemp = Max(StrLength(Name), 30);
		} else
			Width = WidthTemp = 30;

		// ширина месага - 38%
		WidthTemp = Min(WidthTemp, WidthNameForMessage);
		Width = Max(Width, WidthTemp);

		FARString strOutFileName = Name;	//??? nullptr ???
		TruncPathStr(strOutFileName, Width);
		CenterStr(strOutFileName, strOutFileName, Width + 4);
		Message(0, 0, Msg::MoveDlgTitle, Msg::CopyPrepareSecury, strOutFileName);

		if (CP->Cancelled()) {
			return FALSE;
		}
	}

	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = Name;
	PreRedraw.SetParam(preRedrawItem.Param);
	return TRUE;
}

bool ShellCopy::CalcTotalSize()
{
	FARString strSelName;
	DWORD FileAttr;
	uint64_t FileSize;
	// Для фильтра
	FAR_FIND_DATA_EX fd;
	PreRedraw.Push(PR_ShellCopyMsg);
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = CP;
	PreRedraw.SetParam(preRedrawItem.Param);
	TotalCopySize = CurCopiedSize = 0;
	TotalFilesToProcess = 0;
	SrcPanel->GetSelNameCompat(nullptr, FileAttr);

	while (SrcPanel->GetSelNameCompat(&strSelName, FileAttr, &fd)) {
		if ((FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) && Flags.SYMLINK != COPY_SYMLINK_ASFILE)
			continue;

		if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
			{
				uint32_t DirCount, FileCount, ClusterSize;
				uint64_t PhysicalSize;
				CP->SetScanName(strSelName);
				int __Ret = GetDirInfo(L"", strSelName, DirCount, FileCount, FileSize, PhysicalSize,
						ClusterSize, -1, Filter,
						((Flags.SYMLINK == COPY_SYMLINK_ASFILE) ? GETDIRINFO_SCANSYMLINK : 0)
								| (UseFilter ? GETDIRINFO_USEFILTER : 0));

				if (__Ret <= 0) {
					ShowTotalCopySize = false;
					PreRedraw.Pop();
					return FALSE;
				}

				if (FileCount > 0) {
					TotalCopySize+= FileSize;
					TotalFilesToProcess+= FileCount;
				}
			}
		} else {
			// Подсчитаем количество файлов
			if (UseFilter) {
				if (!Filter->FileInFilter(fd))
					continue;
			}

			FileSize = SrcPanel->GetLastSelectedSize();

			if (FileSize != (uint64_t)-1) {
				TotalCopySize+= FileSize;
				TotalFilesToProcess++;
			}
		}
	}

	// INFO: Это для варианта, когда "ВСЕГО = общий размер * количество целей"
	TotalCopySize = TotalCopySize * CountTarget;
	InsertCommas(TotalCopySize, strTotalCopySizeText);
	PreRedraw.Pop();
	return true;
}
