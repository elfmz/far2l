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
#include "registry.hpp"
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
#include "mix.hpp"
#include "DlgGuid.hpp"
#include "console.hpp"
#include "wakeful.hpp"

/* Общее время ожидания пользователя */
extern long WaitUserTime;
/* Длф того, что бы время при одижании пользователя тикало, а remaining/speed нет */
static long OldCalcTime;

#define SDDATA_SIZE   64*1024

#define PROGRESS_REFRESH_THRESHOLD    1000 // msec

enum {COPY_BUFFER_SIZE  = 0x10000};

enum
{
	COPY_RULE_NUL    = 0x0001,
	COPY_RULE_FILES  = 0x0002,
};

static int TotalFiles,TotalFilesToProcess;

static clock_t CopyStartTime;

static int OrigScrX,OrigScrY;

static uint64_t TotalCopySize, TotalCopiedSize; // Общий индикатор копирования
static uint64_t CurCopiedSize;                  // Текущий индикатор копирования
static uint64_t TotalSkippedSize;               // Общий размер пропущенных файлов
static size_t   CountTarget;                    // всего целей.
static bool ShowTotalCopySize;
static FARString strTotalCopySizeText;

static FileFilter *Filter;
static int UseFilter=FALSE;

static BOOL ZoomedState,IconicState;

class FileExtendedAttributesCopy
{
	FileExtendedAttributes _xattr;
	bool _apply;

	public:
	FileExtendedAttributesCopy(File &f)
	{
		_apply = (f.QueryFileExtendedAttributes(_xattr) != FB_NO && !_xattr.empty());
	}

	void ApplyToCopied(File &f)
	{
		if (_apply) {
			f.SetFileExtendedAttributes(_xattr);
		}
	}
};

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
	ID_SC_COPYACCESSMODE,
	ID_SC_COPYXATTR,
	ID_SC_MULTITARGET,
	ID_SC_WRITETHROUGH,
	ID_SC_SEPARATOR2,	
	ID_SC_COPYSYMLINK,
	ID_SC_COPYSYMLINKOUTER,
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
		SMALL_RECT Rect;
		wchar_t Bar[100];
		size_t BarSize;
		bool Move,Total,Time;
		bool BgInit,ScanBgInit;
		bool IsCancelled;
		int Color;
		int Percents;
		DWORD LastWriteTime;
		FARString strSrc,strDst,strFiles,strTime;
		bool Timer();
		void Flush();
		void DrawNames();
		void CreateScanBackground();
		void SetProgress(bool TotalProgress,UINT64 CompletedSize,UINT64 TotalSize);
	public:
		CopyProgress(bool Move,bool Total,bool Time);
		void CreateBackground();
		bool Cancelled() {return IsCancelled;};
		void SetScanName(const wchar_t *Name);
		void SetNames(const wchar_t *Src,const wchar_t *Dst);
		void SetProgressValue(UINT64 CompletedSize,UINT64 TotalSize) {return SetProgress(false,CompletedSize,TotalSize);}
		void SetTotalProgressValue(UINT64 CompletedSize,UINT64 TotalSize) {return SetProgress(true,CompletedSize,TotalSize);}
};

static void GetTimeText(DWORD Time,FARString &strTimeText)
{
	DWORD Sec=Time;
	DWORD Min=Sec/60;
	Sec-=(Min*60);
	DWORD Hour=Min/60;
	Min-=(Hour*60);
	strTimeText.Format(L"%02u:%02u:%02u",Hour,Min,Sec);
}

bool CopyProgress::Timer()
{
	bool Result=false;
	DWORD Time=WINPORT(GetTickCount)();

	if (!LastWriteTime||(Time-LastWriteTime>=RedrawTimeout))
	{
		LastWriteTime=Time;
		Result=true;
	}

	return Result;
}

void CopyProgress::Flush()
{
	if (Timer())
	{
		if (!IsCancelled)
		{
			if (CheckForEscSilent())
			{
				(*FrameManager)[0]->Lock();
				IsCancelled=ConfirmAbortOp()!=0;
				(*FrameManager)[0]->Unlock();
			}
		}

		if (Total || (TotalFilesToProcess==1))
		{
			CopyTitle.Set(L"{%d%%} %ls",Total?ToPercent64(TotalCopiedSize>>8,TotalCopySize>>8):Percents,Move?MSG(MCopyMovingTitle):MSG(MCopyCopyingTitle));
		}
	}
}

CopyProgress::CopyProgress(bool Move,bool Total,bool Time):
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
{
}

void CopyProgress::SetScanName(const wchar_t *Name)
{
	if (!ScanBgInit)
	{
		CreateScanBackground();
	}

	GotoXY(Rect.Left+5,Rect.Top+3);
	FS<<fmt::LeftAlign()<<fmt::Width(Rect.Right-Rect.Left-9)<<fmt::Precision(Rect.Right-Rect.Left-9)<<Name;
	Flush();
}

void CopyProgress::CreateScanBackground()
{
	for (size_t i=0; i<BarSize; i++)
	{
		Bar[i]=L' ';
	}

	Bar[BarSize]=0;
	Message(MSG_LEFTALIGN,0,MSG(Move?MMoveDlgTitle:MCopyDlgTitle),MSG(MCopyScanning),Bar);
	int MX1,MY1,MX2,MY2;
	GetMessagePosition(MX1,MY1,MX2,MY2);
	Rect.Left=MX1;
	Rect.Right=MX2;
	Rect.Top=MY1;
	Rect.Bottom=MY2;
	ScanBgInit=true;
}

void CopyProgress::CreateBackground()
{
	for (size_t i=0; i<BarSize; i++)
	{
		Bar[i]=L' ';
	}

	Bar[BarSize]=0;

	if (!Total)
	{
		if (!Time)
		{
			Message(MSG_LEFTALIGN,0,MSG(Move?MMoveDlgTitle:MCopyDlgTitle),MSG(Move?MCopyMoving:MCopyCopying),L"",MSG(MCopyTo),L"",Bar,L"\x1",L"");
		}
		else
		{
			Message(MSG_LEFTALIGN,0,MSG(Move?MMoveDlgTitle:MCopyDlgTitle),MSG(Move?MCopyMoving:MCopyCopying),L"",MSG(MCopyTo),L"",Bar,L"\x1",L"",L"\x1",L"");
		}
	}
	else
	{
		FARString strTotalSeparator(L"\x1 ");
		strTotalSeparator+=MSG(MCopyDlgTotal);
		strTotalSeparator+=L": ";
		strTotalSeparator+=strTotalCopySizeText;
		strTotalSeparator+=L" ";

		if (!Time)
		{
			Message(MSG_LEFTALIGN,0,MSG(Move?MMoveDlgTitle:MCopyDlgTitle),MSG(Move?MCopyMoving:MCopyCopying),L"",MSG(MCopyTo),L"",Bar,strTotalSeparator,Bar,L"\x1",L"");
		}
		else
		{
			Message(MSG_LEFTALIGN,0,MSG(Move?MMoveDlgTitle:MCopyDlgTitle),MSG(Move?MCopyMoving:MCopyCopying),L"",MSG(MCopyTo),L"",Bar,strTotalSeparator,Bar,L"\x1",L"",L"\x1",L"");
		}
	}

	int MX1,MY1,MX2,MY2;
	GetMessagePosition(MX1,MY1,MX2,MY2);
	Rect.Left=MX1;
	Rect.Right=MX2;
	Rect.Top=MY1;
	Rect.Bottom=MY2;
	BgInit=true;
	DrawNames();
}

void CopyProgress::DrawNames()
{
	Text(Rect.Left+5,Rect.Top+3,Color,strSrc);
	Text(Rect.Left+5,Rect.Top+5,Color,strDst);
	Text(Rect.Left+5,Rect.Top+(Total?10:8),Color,strFiles);
}

void CopyProgress::SetNames(const wchar_t *Src,const wchar_t *Dst)
{
	if (!BgInit)
	{
		CreateBackground();
	}

	FormatString FString;
	FString<<fmt::LeftAlign()<<fmt::Width(Rect.Right-Rect.Left-9)<<fmt::Precision(Rect.Right-Rect.Left-9)<<Src;
	strSrc=FString.strValue();
	FString.Clear();
	FString<<fmt::LeftAlign()<<fmt::Width(Rect.Right-Rect.Left-9)<<fmt::Precision(Rect.Right-Rect.Left-9)<<Dst;
	strDst=FString.strValue();

	if (Total)
	{
		strFiles.Format(MSG(MCopyProcessedTotal),TotalFiles,TotalFilesToProcess);
	}
	else
	{
		strFiles.Format(MSG(MCopyProcessed),TotalFiles);
	}

	DrawNames();
	Flush();
}

void CopyProgress::SetProgress(bool TotalProgress,UINT64 CompletedSize,UINT64 TotalSize)
{
	if (!BgInit)
	{
		CreateBackground();
	}

	if (Total==TotalProgress)
	{
	}

	UINT64 OldCompletedSize = CompletedSize;
	UINT64 OldTotalSize = TotalSize;
	CompletedSize>>=8;
	TotalSize>>=8;
	CompletedSize=Min(CompletedSize,TotalSize);
	COORD BarCoord={(SHORT)(Rect.Left+5),(SHORT)(Rect.Top+(TotalProgress?8:6))};
	size_t BarLength=Rect.Right-Rect.Left-9-5; //-5 для процентов
	size_t Length=TotalSize?static_cast<size_t>((TotalSize<1000000?CompletedSize:CompletedSize/100)*BarLength/(TotalSize<1000000?TotalSize:TotalSize/100)):BarLength;

	for (size_t i=0; i<BarLength; i++)
	{
		Bar[i]=BoxSymbols[BS_X_B0];
	}

	if (TotalSize)
	{
		for (size_t i=0; i<Length; i++)
		{
			Bar[i]=BoxSymbols[BS_X_DB];
		}
	}

	Bar[BarLength]=0;
	Percents=ToPercent64(CompletedSize,TotalSize);
	FormatString strPercents;
	strPercents<<fmt::Width(4)<<Percents<<L"%";
	Text(BarCoord.X,BarCoord.Y,Color,Bar);
	Text(static_cast<int>(BarCoord.X+BarLength),BarCoord.Y,Color,strPercents);

	if (Time&&(!Total||TotalProgress))
	{
		DWORD WorkTime=GetProcessUptimeMSec()-CopyStartTime;
		UINT64 SizeLeft=(OldTotalSize>OldCompletedSize)?(OldTotalSize-OldCompletedSize):0;
		long CalcTime=OldCalcTime;

		if (WaitUserTime!=-1) // -1 => находимся в процессе ожидания ответа юзера
		{
			OldCalcTime=CalcTime=WorkTime-WaitUserTime;
		}

		WorkTime/=1000;
		CalcTime/=1000;

		if (!WorkTime)
		{
			strTime.Format(MSG(MCopyTimeInfo),L" ",L" ",L" ");
		}
		else
		{
			if (TotalProgress)
			{
				OldCompletedSize=OldCompletedSize-TotalSkippedSize;
			}

			UINT64 CPS=CalcTime?OldCompletedSize/CalcTime:0;
			DWORD TimeLeft=static_cast<DWORD>(CPS?SizeLeft/CPS:0);
			FARString strSpeed;
			FileSizeToStr(strSpeed,CPS,8,COLUMN_FLOATSIZE|COLUMN_COMMAS);
			FARString strWorkTimeStr,strTimeLeftStr;
			GetTimeText(WorkTime,strWorkTimeStr);
			GetTimeText(TimeLeft,strTimeLeftStr);
			if(strSpeed.At(0)==L' ' && strSpeed.At(strSpeed.GetLength()-1)>=L'0' && strSpeed.At(strSpeed.GetLength()-1)<=L'9')
			{
				strSpeed.LShift(1);
				strSpeed+=L" ";
			}
			strTime.Format(MSG(MCopyTimeInfo),strWorkTimeStr.CPtr(),strTimeLeftStr.CPtr(),strSpeed.CPtr());
		}

		Text(Rect.Left+5,Rect.Top+(Total?12:10),Color,strTime);
	}

	Flush();
}
// CopyProgress end

CopyProgress *CP;


/* $ 25.05.2002 IS
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

static int CmpFullNames(const wchar_t *Src,const wchar_t *Dest)
{
	FARString strSrcFullName, strDestFullName;

	// получим полные пути с учетом символических связей
    	ConvertNameToFull(Src, strSrcFullName);
    	ConvertNameToFull(Dest, strDestFullName);
	DeleteEndSlash(strSrcFullName);
	DeleteEndSlash(strDestFullName);

	return !StrCmp(strSrcFullName,strDestFullName);
}

static bool CheckNulOrCon(const wchar_t *Src)
{
	return IsPathIn(Src, DEVNULLW);
}

static FARString& GetParentFolder(const wchar_t *Src, FARString &strDest)
{
	strDest = Src;
	CutToSlash(strDest,true);
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

static void GenerateName(FARString &strName,const wchar_t *Path=nullptr)
{
	if (Path&&*Path)
	{
		FARString strTmp=Path;
		AddEndSlash(strTmp);
		strTmp+=PointToName(strName);
		strName=strTmp;
	}

	FARString strExt=PointToExt(strName);
	size_t NameLength=strName.GetLength()-strExt.GetLength();

	for (int i=1; apiGetFileAttributes(strName)!=INVALID_FILE_ATTRIBUTES; i++)
	{
		WCHAR Suffix[20]=L"_";
		_itow(i,Suffix+1,10);
		strName.SetLength(NameLength);
		strName+=Suffix;
		strName+=strExt;
	}
}

void PR_ShellCopyMsg()
{
	PreRedrawItem preRedrawItem=PreRedraw.Peek();

	if (preRedrawItem.Param.Param1)
	{
		((CopyProgress*)preRedrawItem.Param.Param1)->CreateBackground();
	}
}

BOOL CheckAndUpdateConsole(BOOL IsChangeConsole)
{
	BOOL curZoomedState = Console.IsZoomed();
	BOOL curIconicState = Console.IsIconic();

	if (ZoomedState!=curZoomedState && IconicState==curIconicState)
	{
		ZoomedState=curZoomedState;
		ChangeVideoMode(ZoomedState);
		Frame *frame=FrameManager->GetBottomFrame();
		int LockCount=-1;

		while (frame->Locked())
		{
			LockCount++;
			frame->Unlock();
		}

		FrameManager->ResizeAllFrame();
		FrameManager->PluginCommit();

		while (LockCount > 0)
		{
			frame->Lock();
			LockCount--;
		}

		IsChangeConsole=TRUE;
	}

	return IsChangeConsole;
}

#define USE_PAGE_SIZE 0x1000
template <class T> static T AlignPageUp(T v)
{
//todo: use actual system page size
	uintptr_t al = ((uintptr_t)v) & (USE_PAGE_SIZE - 1);
	if (al) {
		v = (T)(((uintptr_t)v) + (USE_PAGE_SIZE - al));
	}
	return v;
}

ShellCopy::ShellCopy(Panel *SrcPanel,        // исходная панель (активная)
                     int Move,               // =1 - операция Move
                     int Link,               // =1 - Sym/Hard Link
                     int CurrentOnly,        // =1 - только текущий файл, под курсором
                     int Ask,                // =1 - выводить диалог?
                     int &ToPlugin,          // =?
                     const wchar_t *PluginDestPath,
                     bool ToSubdir):
	sddata(nullptr),
	CopyBuffer(nullptr), CopyBufferBase(nullptr),
	RPT(RP_EXACTCOPY)
{
	Filter=nullptr;
	DestList.SetParameters(0,0,ULF_UNIQUE);
	CopyDlgParam CDP{};
	if (!(CDP.SelCount=SrcPanel->GetSelCount()))
		return;

	FARString strSelName;

	if (CDP.SelCount==1)
	{
		SrcPanel->GetSelNameCompat(nullptr,CDP.FileAttr); //????
		SrcPanel->GetSelNameCompat(&strSelName,CDP.FileAttr);

		if (TestParentFolderName(strSelName))
			return;
	}

	ZoomedState=Console.IsZoomed();
	IconicState=Console.IsIconic();
	// Создадим объект фильтра
	Filter=new FileFilter(SrcPanel, FFT_COPY);
	sddata=new char[SDDATA_SIZE];
	// $ 26.05.2001 OT Запретить перерисовку панелей во время копирования
	_tran(SysLog(L"call (*FrameManager)[0]->LockRefresh()"));
	(*FrameManager)[0]->Lock();
	// Размер буфера берется из реестра
	GetRegKey(L"System", L"CopyBufferSize", CopyBufferSize, 0);
	CopyBufferSize=AlignPageUp(Max(CopyBufferSize,(int)COPY_BUFFER_SIZE));
	// Progress bar update threshold
	GetRegKey(L"System", L"ProgressUpdateThreshold", ProgressUpdateThreshold, PROGRESS_REFRESH_THRESHOLD);
	CDP.thisClass=this;
	CDP.AltF10=0;
	CDP.FolderPresent=false;
	CDP.FilesPresent=false;
	Flags=(Move?FCOPY_MOVE:0)|(Link?FCOPY_LINK:0)|(CurrentOnly?FCOPY_CURRENTONLY:0);
	ShowTotalCopySize=Opt.CMOpt.CopyShowTotal!=0;
	strTotalCopySizeText.Clear();
	SelectedFolderNameLength=0;
	int DestPlugin=ToPlugin;
	ToPlugin=FALSE;
	this->SrcPanel=SrcPanel;
	DestPanel=CtrlObject->Cp()->GetAnotherPanel(SrcPanel);
	DestPanelMode=DestPlugin ? DestPanel->GetMode():NORMAL_PANEL;
	SrcPanelMode=SrcPanel->GetMode();

	// ***********************************************************************
	// *** Prepare Dialog Controls
	// ***********************************************************************
	int DLG_HEIGHT=19, DLG_WIDTH=76;

	DialogDataEx CopyDlgData[]=
	{
		{DI_DOUBLEBOX,   3, 1,(SHORT)(DLG_WIDTH-4),(SHORT)(DLG_HEIGHT-2),{},0,MSG(MCopyDlgTitle)},
		{DI_TEXT,        5, 2, 0, 2,{},0,MSG(Link?MCMLTargetIN:MCMLTargetTO)},
		{DI_EDIT,        5, 3,70, 3,{reinterpret_cast<DWORD_PTR>(L"Copy")},DIF_FOCUS|DIF_HISTORY|DIF_EDITEXPAND|DIF_USELASTHISTORY|DIF_EDITPATH,L""},
		{DI_TEXT,        3, 4, 0, 4,{},DIF_SEPARATOR,L""},
		{DI_TEXT,        5, 5, 0, 5,{},0,MSG(MCopyIfFileExist)},
		{DI_COMBOBOX,   29, 5,70, 5,{},DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND|DIF_LISTWRAPMODE,L""},
		{DI_CHECKBOX,    5, 6, 0, 6,{},0,MSG(MCopyAccessMode)},
		{DI_CHECKBOX,    5, 7, 0, 7,{},0,MSG(MCopyXAttr)},
		{DI_CHECKBOX,    5, 8, 0, 8,{},0,MSG(MCopyMultiActions)},
		{DI_CHECKBOX,    5, 9, 0, 9,{},0,MSG(MCopyWriteThrough)},
		{DI_TEXT,        3,10, 0,10,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,    5, 11, 0, 11,{}, Move ? DIF_DISABLE : 0, MSG(MCopySymLinkContents)},
		{DI_CHECKBOX,    5, 12, 0, 12,{},0,MSG(MCopySymLinkContentsOuter)},
		{DI_TEXT,        3,13, 0,13,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,    5,14, 0,14,{UseFilter?BSTATE_CHECKED:BSTATE_UNCHECKED},DIF_AUTOMATION,(wchar_t *)MCopyUseFilter},
		{DI_TEXT,        3,15, 0,15,{},DIF_SEPARATOR,L""},
		{DI_BUTTON,      0,16, 0,16,{},DIF_DEFAULT|DIF_CENTERGROUP,MSG(MCopyDlgCopy)},
		{DI_BUTTON,      0,16, 0,16,{},DIF_CENTERGROUP|DIF_BTNNOCLOSE,MSG(MCopyDlgTree)},
		{DI_BUTTON,      0,16, 0,16,{},DIF_CENTERGROUP|DIF_BTNNOCLOSE|DIF_AUTOMATION|(UseFilter?0:DIF_DISABLE),MSG(MCopySetFilter)},
		{DI_BUTTON,      0,16, 0,16,{},DIF_CENTERGROUP,MSG(MCopyDlgCancel)},
		{DI_TEXT,        5, 2, 0, 2,{},DIF_SHOWAMPERSAND,L""}
	};
	MakeDialogItemsEx(CopyDlgData,CopyDlg);
	CopyDlg[ID_SC_MULTITARGET].Selected=Opt.CMOpt.MultiCopy;
	CopyDlg[ID_SC_WRITETHROUGH].Selected=Opt.CMOpt.WriteThrough;
	CopyDlg[ID_SC_COPYACCESSMODE].Selected=Opt.CMOpt.CopyAccessMode;
	CopyDlg[ID_SC_COPYXATTR].Selected=Opt.CMOpt.CopyXAttr;

	if (Link)
	{
		CopyDlg[ID_SC_COMBOTEXT].strData=MSG(MLinkType);
		CopyDlg[ID_SC_SEPARATOR2].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_COPYSYMLINK].Selected=0;
		CopyDlg[ID_SC_COPYSYMLINK].Flags|=DIF_DISABLE|DIF_HIDDEN;
		CopyDlg[ID_SC_COPYSYMLINKOUTER].Selected=0;
		CopyDlg[ID_SC_COPYSYMLINKOUTER].Flags|=DIF_DISABLE|DIF_HIDDEN;
	}
	else if (Move) // секция про перенос
	{
		CopyDlg[ID_SC_MULTITARGET].Selected = 0;
		CopyDlg[ID_SC_MULTITARGET].Flags |= DIF_DISABLE;
	}
	else // секция про копирование
	{
		CopyDlg[ID_SC_COPYSYMLINKOUTER].Selected=1;
	}

	FARString strCopyStr;

	if (CDP.SelCount==1)
	{
		if (SrcPanel->GetType()==TREE_PANEL)
		{
			FARString strNewDir(strSelName);
			size_t pos;

			if (FindLastSlash(pos,strNewDir))
			{
				strNewDir.SetLength(pos);

				if (!pos || strNewDir.At(pos-1)==L':')
					strNewDir += L"/";

				FarChDir(strNewDir);
			}
		}

		FARString strSelNameShort = strSelName;
		strCopyStr=MSG(Move?MMoveFile:(Link?MLinkFile:MCopyFile));
		TruncPathStr(strSelNameShort,static_cast<int>(CopyDlg[ID_SC_TITLE].X2-CopyDlg[ID_SC_TITLE].X1-strCopyStr.GetLength()-7));
		strCopyStr+=L" "+strSelNameShort;

		// Если копируем одиночный файл, то запрещаем использовать фильтр
		if (!(CDP.FileAttr&FILE_ATTRIBUTE_DIRECTORY))
		{
			CopyDlg[ID_SC_USEFILTER].Selected=0;
			CopyDlg[ID_SC_USEFILTER].Flags|=DIF_DISABLE;
		}
	}
	else // Объектов несколько!
	{
		int NOper=MCopyFiles;

		if (Move) NOper=MMoveFiles;
		else if (Link) NOper=MLinkFiles;

		// коррекция языка - про окончания
		FormatString StrItems;
		StrItems<<CDP.SelCount;
		size_t LenItems=StrItems.strValue().GetLength();
		int NItems=MCMLItemsA;

		if (LenItems > 0)
		{
			if ((LenItems >= 2 && StrItems[LenItems-2] == '1') ||
			        StrItems[LenItems-1] >= '5' ||
			        StrItems[LenItems-1] == '0')
				NItems=MCMLItemsS;
			else if (StrItems[LenItems-1] == '1')
				NItems=MCMLItems0;
		}

		strCopyStr.Format(MSG(NOper),CDP.SelCount,MSG(NItems));
	}

	CopyDlg[ID_SC_SOURCEFILENAME].strData=strCopyStr;
	CopyDlg[ID_SC_TITLE].strData = MSG(Move?MMoveDlgTitle :(Link?MLinkDlgTitle:MCopyDlgTitle));
	CopyDlg[ID_SC_BTNCOPY].strData = MSG(Move?MCopyDlgRename:(Link?MCopyDlgLink:MCopyDlgCopy));

	if (DestPanelMode == PLUGIN_PANEL)
	{
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
	if(ToSubdir)
	{
		AddEndSlash(strDestDir);
		FARString strSubdir;
		DestPanel->GetCurName(strSubdir);
		strDestDir+=strSubdir;
	}
	FARString strSrcDir;
	SrcPanel->GetCurDir(strSrcDir);

	if (CurrentOnly)
	{
		//   При копировании только элемента под курсором берем его имя в кавычки, если оно содержит разделители.
		CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;

		if (!Move && wcspbrk(CopyDlg[ID_SC_TARGETEDIT].strData,L",;"))
		{
			Unquote(CopyDlg[ID_SC_TARGETEDIT].strData);     // уберем все лишние кавычки
			InsertQuote(CopyDlg[ID_SC_TARGETEDIT].strData); // возьмем в кавычки, т.к. могут быть разделители
		}
	}
	else
	{
		switch (DestPanelMode)
		{
			case NORMAL_PANEL:
			{
				if ((strDestDir.IsEmpty() || !DestPanel->IsVisible() || !StrCmp(strSrcDir,strDestDir)) && CDP.SelCount==1)
					CopyDlg[ID_SC_TARGETEDIT].strData = strSelName;
				else
				{
					CopyDlg[ID_SC_TARGETEDIT].strData = strDestDir;
					AddEndSlash(CopyDlg[ID_SC_TARGETEDIT].strData);
				}

				/* $ 19.07.2003 IS
				   Если цель содержит разделители, то возьмем ее в кавычки, дабы не получить
				   ерунду при F5, Enter в панелях, когда пользователь включит MultiCopy
				*/
				if (!Move && wcspbrk(CopyDlg[ID_SC_TARGETEDIT].strData,L",;"))
				{
					Unquote(CopyDlg[ID_SC_TARGETEDIT].strData);     // уберем все лишние кавычки
					InsertQuote(CopyDlg[ID_SC_TARGETEDIT].strData); // возьмем в кавычки, т.к. могут быть разделители
				}

				break;
			}

			case PLUGIN_PANEL:
			{
				OpenPluginInfo Info;
				DestPanel->GetOpenPluginInfo(&Info);
				FARString strFormat = Info.Format;
				CopyDlg[ID_SC_TARGETEDIT].strData = strFormat+L":";

				while (CopyDlg[ID_SC_TARGETEDIT].strData.GetLength()<2)
					CopyDlg[ID_SC_TARGETEDIT].strData += L":";

				CDP.strPluginFormat = CopyDlg[ID_SC_TARGETEDIT].strData;
				CDP.strPluginFormat.Upper();
				break;
			}
		}
	}

	FARString strInitDestDir = CopyDlg[ID_SC_TARGETEDIT].strData;
	// Для фильтра
	FAR_FIND_DATA_EX fd;
	SrcPanel->GetSelNameCompat(nullptr,CDP.FileAttr);

	bool AddSlash=false;

	while (SrcPanel->GetSelNameCompat(&strSelName,CDP.FileAttr,&fd))
	{
		if (UseFilter)
		{
			if (!Filter->FileInFilter(fd))
				continue;
		}

		if (CDP.FileAttr & FILE_ATTRIBUTE_DIRECTORY)
		{
			CDP.FolderPresent=true;
			AddSlash=true;
//      break;
		}
		else
		{
			CDP.FilesPresent=true;
		}
	}

	if (Link) // рулесы по поводу линков (предварительные!)
	{
		for(int i=ID_SC_SEPARATOR3;i<=ID_SC_BTNCANCEL;i++)
		{
			CopyDlg[i].Y1-=3;
			CopyDlg[i].Y2-=3;
		}
		CopyDlg[ID_SC_TITLE].Y2-=3;
		DLG_HEIGHT-=3;
	}

	// корректирем позицию " to"
	CopyDlg[ID_SC_TARGETTITLE].X1=CopyDlg[ID_SC_TARGETTITLE].X2=CopyDlg[ID_SC_SOURCEFILENAME].X1+(int)CopyDlg[ID_SC_SOURCEFILENAME].strData.GetLength();

	/* $ 15.06.2002 IS
	   Обработка копирования мышкой - в этом случае диалог не показывается,
	   но переменные все равно инициализируются. Если произойдет неудачная
	   компиляция списка целей, то покажем диалог.
	*/
	FARString strCopyDlgValue;
	if (!Ask)
	{
		strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
		Unquote(strCopyDlgValue);
		InsertQuote(strCopyDlgValue);

		if (!DestList.Set(strCopyDlgValue))
			Ask=TRUE;
	}

	// ***********************************************************************
	// *** Вывод и обработка диалога
	// ***********************************************************************
	if (Ask)
	{
		FarList ComboList;
		FarListItem LinkTypeItems[2]={},CopyModeItems[8]={};

		if (Link)
		{
			ComboList.ItemsNumber=ARRAYSIZE(LinkTypeItems);
			ComboList.Items=LinkTypeItems;
			ComboList.Items[0].Text=MSG(MLinkTypeHardlink);
			ComboList.Items[1].Text=MSG(MLinkTypeSymlink);
//			ComboList.Items[2].Text=MSG(MLinkTypeJunction);
//			ComboList.Items[3].Text=MSG(MLinkTypeSymlinkFile);
//			ComboList.Items[4].Text=MSG(MLinkTypeSymlinkDirectory);

			if (CDP.FilesPresent)
				ComboList.Items[0].Flags|=LIF_SELECTED;
			else
				ComboList.Items[1].Flags|=LIF_SELECTED;
		}
		else
		{
			ComboList.ItemsNumber=ARRAYSIZE(CopyModeItems);
			ComboList.Items=CopyModeItems;
			ComboList.Items[CM_ASK].Text=MSG(MCopyAsk);
			ComboList.Items[CM_OVERWRITE].Text=MSG(MCopyOverwrite);
			ComboList.Items[CM_SKIP].Text=MSG(MCopySkipOvr);
			ComboList.Items[CM_RENAME].Text=MSG(MCopyRename);
			ComboList.Items[CM_APPEND].Text=MSG(MCopyAppend);
			ComboList.Items[CM_ONLYNEWER].Text=MSG(MCopyOnlyNewerFiles);
			ComboList.Items[CM_ASKRO].Text=MSG(MCopyAskRO);
			ComboList.Items[CM_ASK].Flags=LIF_SELECTED;
			ComboList.Items[CM_SEPARATOR].Flags=LIF_SEPARATOR;

			if (Opt.Confirm.RO)
			{
				ComboList.Items[CM_ASKRO].Flags=LIF_CHECKED;
			}
		}

		CopyDlg[ID_SC_COMBO].ListItems=&ComboList;
		Dialog Dlg(CopyDlg,ARRAYSIZE(CopyDlg),CopyDlgProc,(LONG_PTR)&CDP);
		Dlg.SetHelp(Link?L"HardSymLink":L"CopyFiles");
		Dlg.SetId(Link?HardSymLinkId:(Move?MoveFilesId:CopyFilesId));
		Dlg.SetPosition(-1,-1,DLG_WIDTH,DLG_HEIGHT);
		Dlg.SetAutomation(ID_SC_USEFILTER,ID_SC_BTNFILTER,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
//    Dlg.Show();
		// $ 02.06.2001 IS + Проверим список целей и поднимем тревогу, если он содержит ошибки
		int DlgExitCode;

		for (;;)
		{
			Dlg.ClearDone();
			Dlg.Process();
			DlgExitCode=Dlg.GetExitCode();
			//Рефреш текущему времени для фильтра сразу после выхода из диалога
			Filter->UpdateCurrentTime();

			if (DlgExitCode == ID_SC_BTNCOPY)
			{
				/* $ 03.08.2001 IS
				   Запомним строчку из диалога и начинаем ее мучить в зависимости от
				   состояния опции мультикопирования
				*/
				strCopyDlgValue = CopyDlg[ID_SC_TARGETEDIT].strData;
				if(!Move)
				{
					Opt.CMOpt.MultiCopy=CopyDlg[ID_SC_MULTITARGET].Selected;
				}
				Opt.CMOpt.WriteThrough=CopyDlg[ID_SC_WRITETHROUGH].Selected;
				Opt.CMOpt.CopyAccessMode=CopyDlg[ID_SC_COPYACCESSMODE].Selected;
				Opt.CMOpt.CopyXAttr=CopyDlg[ID_SC_COPYXATTR].Selected;

				if (!CopyDlg[ID_SC_MULTITARGET].Selected || !wcspbrk(strCopyDlgValue,L",;")) // отключено multi*
				{
					// уберем лишние кавычки
					Unquote(strCopyDlgValue);
					// добавим кавычки, чтобы "список" удачно скомпилировался вне
					// зависимости от наличия разделителей в оном
					InsertQuote(strCopyDlgValue);
				}

				if (DestList.Set(strCopyDlgValue))
				{
					// Запомнить признак использования фильтра. KM
					UseFilter=CopyDlg[ID_SC_USEFILTER].Selected;
					break;
				}
				else
				{
					Message(MSG_WARNING,1,MSG(MWarning),MSG(MCopyIncorrectTargetList), MSG(MOk));
				}
			}
			else
				break;
		}

		if (DlgExitCode == ID_SC_BTNCANCEL || DlgExitCode < 0 || (CopyDlg[ID_SC_BTNCOPY].Flags&DIF_DISABLE))
		{
			if (DestPlugin)
				ToPlugin=-1;

			return;
		}
	}

	// ***********************************************************************
	// *** Стадия подготовки данных после диалога
	// ***********************************************************************
	if (Opt.CMOpt.WriteThrough)
		Flags|=FCOPY_WRITETHROUGH;
	else
		Flags&=~FCOPY_WRITETHROUGH;

	if (Opt.CMOpt.CopyAccessMode)
		Flags|=FCOPY_COPYACCESSMODE;
	else
		Flags&=~FCOPY_COPYACCESSMODE;

	if (Opt.CMOpt.CopyXAttr)
		Flags|=FCOPY_COPYXATTR;
	else
		Flags&=~FCOPY_COPYXATTR;


	ReadOnlyDelMode=ReadOnlyOvrMode=OvrMode=SkipEncMode=SkipMode=SkipDeleteMode=-1;

	if (Link)
	{
		switch (CopyDlg[ID_SC_COMBO].ListPos)
		{
			case 0:
				RPT=RP_HARDLINK;
				break;
			case 1:
				RPT=RP_JUNCTION;
				break;
			case 2:
				RPT=RP_SYMLINK;
				break;
			case 3:
				RPT=RP_SYMLINKFILE;
				break;
			case 4:
				RPT=RP_SYMLINKDIR;
				break;
		}
	}
	else
	{
		ReadOnlyOvrMode=CDP.AskRO?-1:1;

		switch (CopyDlg[ID_SC_COMBO].ListPos)
		{
			case CM_ASK:
				OvrMode=-1;
				break;
			case CM_OVERWRITE:
				OvrMode=1;
				break;
			case CM_SKIP:
				OvrMode=3;
				ReadOnlyOvrMode=CDP.AskRO?-1:3;
				break;
			case CM_RENAME:
				OvrMode=5;
				break;
			case CM_APPEND:
				OvrMode=7;
				break;
			case CM_ONLYNEWER:
				Flags|=FCOPY_ONLYNEWERFILES;
				break;
		}
	}

	Flags|=CopyDlg[ID_SC_COPYSYMLINK].Selected?FCOPY_COPYSYMLINKCONTENTS:0;
	Flags|=CopyDlg[ID_SC_COPYSYMLINKOUTER].Selected?FCOPY_COPYSYMLINKCONTENTSOUTER:0;

	if (DestPlugin && !StrCmp(CopyDlg[ID_SC_TARGETEDIT].strData,strInitDestDir))
	{
		ToPlugin=1;
		return;
	}

	if (CheckNulOrCon(strCopyDlgValue))
		Flags|=FCOPY_COPYTONUL;

	if (Flags&FCOPY_COPYTONUL)
	{
		Flags&=~FCOPY_MOVE;
		Move=0;
	}

	if (CDP.SelCount==1 || (Flags&FCOPY_COPYTONUL))
		AddSlash=false; //???

	if (DestPlugin==2)
	{
		if (PluginDestPath)
			strCopyDlgValue = PluginDestPath;

		return;
	}

	if ((Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && SrcPanel->IsDizDisplayed()) ||
	        Opt.Diz.UpdateMode==DIZ_UPDATE_ALWAYS)
	{
		CtrlObject->Cp()->LeftPanel->ReadDiz();
		CtrlObject->Cp()->RightPanel->ReadDiz();
	}

	//allocate page-aligned memory: IO works faster on that, also direct-io requires buffer to be aligned sometimes
	//OSX lacks aligned_malloc so do it manually
	CopyBufferBase = new char[CopyBufferSize + USE_PAGE_SIZE];
	CopyBuffer = AlignPageUp(CopyBufferBase);
	DestPanel->CloseFile();
	strDestDizPath.Clear();
	SrcPanel->SaveSelection();
	// нужно ли показывать время копирования?
	bool ShowCopyTime=(Opt.CMOpt.CopyTimeRule&((Flags&FCOPY_COPYTONUL)?COPY_RULE_NUL:COPY_RULE_FILES))!=0;
	// ***********************************************************************
	// **** Здесь все подготовительные операции закончены, можно приступать
	// **** к процессу Copy/Move/Link
	// ***********************************************************************
	int NeedDizUpdate=FALSE;
	int NeedUpdateAPanel=FALSE;
	// ПОКА! принудительно выставим обновление.
	// В последствии этот флаг будет выставляться в ShellCopy::CheckUpdatePanel()
	Flags|=FCOPY_UPDATEPPANEL;
	/*
	   ЕСЛИ ПРИНЯТЬ В КАЧЕСТВЕ РАЗДЕЛИТЕЛЯ ПУТЕЙ, НАПРИМЕР ';',
	   то нужно парсить CopyDlgValue на предмет MultiCopy и
	   вызывать CopyFileTree нужное количество раз.
	*/
	{
		Flags&=~FCOPY_MOVE;

		if (DestList.Set(strCopyDlgValue)) // если список успешно "скомпилировался"
		{
			const wchar_t *NamePtr;
			FARString strNameTmp;
			// посчитаем количество целей.
			CountTarget=DestList.GetTotal();
			DestList.Reset();
			TotalFiles=0;
			TotalCopySize=TotalCopiedSize=TotalSkippedSize=0;
			ProgressUpdateTime=0;

			// Запомним время начала
			if (ShowCopyTime)
			{
				CopyStartTime = GetProcessUptimeMSec();
				WaitUserTime = OldCalcTime = 0;
			}

			if (CountTarget > 1)
				Move=0;

			while (nullptr!=(NamePtr=DestList.GetNext()))
			{
				CurCopiedSize=0;
				strNameTmp = NamePtr;

				if ((strNameTmp.GetLength() == 2) && IsAlpha(strNameTmp.At(0)) && (strNameTmp.At(1) == L':'))
					PrepareDiskPath(strNameTmp);

				if (!StrCmp(strNameTmp,L"..") && IsLocalRootPath(strSrcDir))
				{
					if (!Message(MSG_WARNING,2,MSG(MError),MSG((!Move?MCannotCopyToTwoDot:MCannotMoveToTwoDot)),MSG(MCannotCopyMoveToTwoDot),MSG(MCopySkip),MSG(MCopyCancel)))
						continue;

					break;
				}

				if (CheckNulOrCon(strNameTmp))
					Flags|=FCOPY_COPYTONUL;
				else
					Flags&=~FCOPY_COPYTONUL;

				if (Flags&FCOPY_COPYTONUL)
				{
					Flags&=~FCOPY_MOVE;
					Move=0;
				}

				if (DestList.IsEmpty()) // нужно учесть моменты связанные с операцией Move.
				{
					Flags|=FCOPY_COPYLASTTIME|(Move?FCOPY_MOVE:0); // только для последней операции
				}

				// Если выделенных элементов больше 1 и среди них есть каталог, то всегда
				// делаем так, чтобы на конце был '/'
				// деламем так не всегда, а только когда NameTmp не является маской.
				if (AddSlash && !wcspbrk(strNameTmp,L"*?"))
					AddEndSlash(strNameTmp);

				if (CDP.SelCount==1 && !CDP.FolderPresent)
				{
					ShowTotalCopySize=false;
					TotalFilesToProcess=1;
				}

				if (Move)
				{
					if (CDP.SelCount==1 && CDP.FolderPresent && CheckUpdateAnotherPanel(SrcPanel,strSelName))
					{
						NeedUpdateAPanel=TRUE;
					}
				}

				CP=new CopyProgress(Move!=0,ShowTotalCopySize,ShowCopyTime);
				// Обнулим инфу про дизы
				strDestDizPath.Clear();
				Flags&=~FCOPY_DIZREAD;
				// сохраним выделение
				SrcPanel->SaveSelection();
				strDestFSName.Clear();
				int OldCopySymlinkContents=Flags&(FCOPY_COPYSYMLINKCONTENTS|FCOPY_COPYSYMLINKCONTENTSOUTER);
				// собственно - один проход копирования
				// Mantis#45: Необходимо привсти копирование ссылок на папки с NTFS на FAT к более логичному виду
				{
					//todo: If dst does not support symlinks
					//Flags|=FCOPY_COPYSYMLINKCONTENTS;
				}
				PreRedraw.Push(PR_ShellCopyMsg);
				PreRedrawItem preRedrawItem=PreRedraw.Peek();
				preRedrawItem.Param.Param1=CP;
				PreRedraw.SetParam(preRedrawItem.Param);
				int I=CopyFileTree(strNameTmp);
				PreRedraw.Pop();

				Flags&=~(FCOPY_COPYSYMLINKCONTENTS|FCOPY_COPYSYMLINKCONTENTSOUTER);
				Flags|=OldCopySymlinkContents;					

				if (I == COPY_CANCEL)
				{
					NeedDizUpdate=TRUE;
					break;
				}

				// если "есть порох в пороховницах" - восстановим выделение
				if (!DestList.IsEmpty())
					SrcPanel->RestoreSelection();

				// Позаботимся о дизах.
				if (!(Flags&FCOPY_COPYTONUL) && !strDestDizPath.IsEmpty())
				{
					FARString strDestDizName;
					DestDiz.GetDizName(strDestDizName);
					DWORD Attr=apiGetFileAttributes(strDestDizName);
					int DestReadOnly=(Attr!=INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

					if (DestList.IsEmpty()) // Скидываем только во время последней Op.
						if (Move && !DestReadOnly)
							SrcPanel->FlushDiz();

					DestDiz.Flush(strDestDizPath);
				}
			}
		}
		_LOGCOPYR(else SysLog(L"Error: DestList.Set(CopyDlgValue) return FALSE"));
	}
	// ***********************************************************************
	// *** заключительеая стадия процесса
	// *** восстанавливаем/дизим/редравим
	// ***********************************************************************

	if (NeedDizUpdate) // при мультикопировании может быть обрыв, но нам все
	{                 // равно нужно апдейтить дизы!
		if (!(Flags&FCOPY_COPYTONUL) && !strDestDizPath.IsEmpty())
		{
			FARString strDestDizName;
			DestDiz.GetDizName(strDestDizName);
			DWORD Attr=apiGetFileAttributes(strDestDizName);
			int DestReadOnly=(Attr!=INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_READONLY));

			if (Move && !DestReadOnly)
				SrcPanel->FlushDiz();

			DestDiz.Flush(strDestDizPath);
		}
	}

#if 1
	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (CDP.SelCount==1 && !strRenamedName.IsEmpty())
		SrcPanel->GoToFile(strRenamedName);

#if 1

	if (NeedUpdateAPanel && CDP.FileAttr != INVALID_FILE_ATTRIBUTES && (CDP.FileAttr&FILE_ATTRIBUTE_DIRECTORY) && DestPanelMode != PLUGIN_PANEL)
	{
		FARString strTmpSrcDir;
		SrcPanel->GetCurDir(strTmpSrcDir);
		DestPanel->SetCurDir(strTmpSrcDir,FALSE);
	}

#else

	if (CDP.FileAttr != INVALID_FILE_ATTRIBUTES && (CDP.FileAttr&FILE_ATTRIBUTE_DIRECTORY) && DestPanelMode != PLUGIN_PANEL)
	{
		// если SrcDir содержится в DestDir...
		FARString strTmpDestDir;
		FARString strTmpSrcDir;
		DestPanel->GetCurDir(strTmpDestDir);
		SrcPanel->GetCurDir(strTmpSrcDir);

		if (CheckUpdateAnotherPanel(SrcPanel,strTmpSrcDir))
			DestPanel->SetCurDir(strTmpDestDir,FALSE);
	}

#endif

	// проверим "нужность" апдейта пассивной панели
	if (Flags&FCOPY_UPDATEPPANEL)
	{
		DestPanel->SortFileList(TRUE);
		DestPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	}

	if (SrcPanelMode == PLUGIN_PANEL)
		SrcPanel->SetPluginModified();

	CtrlObject->Cp()->Redraw();
#else
	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (CDP.SelCount==1 && strRenamedName.IsEmpty())
		SrcPanel->GoToFile(strRenamedName);

	SrcPanel->Redraw();
	DestPanel->SortFileList(TRUE);
	DestPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	DestPanel->Redraw();
#endif

	if (Opt.NotifOpt.OnFileOperation) {
		DisplayNotification(MSG(MFileOperationComplete), strSelName); // looks like strSelName is best choice
	}

}


LONG_PTR WINAPI CopyDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
#define DM_CALLTREE (DM_USER+1)
#define DM_SWITCHRO (DM_USER+2)
	CopyDlgParam *DlgParam=(CopyDlgParam *)SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);

	switch (Msg)
	{
		case DN_INITDIALOG:
			SendDlgMessage(hDlg,DM_SETCOMBOBOXEVENT,ID_SC_COMBO,CBET_KEY|CBET_MOUSE);
			SendDlgMessage(hDlg,DM_SETMOUSEEVENTNOTIFY,TRUE,0);
			break;
		case DM_SWITCHRO:
		{
			FarListGetItem LGI={CM_ASKRO};
			SendDlgMessage(hDlg,DM_LISTGETITEM,ID_SC_COMBO,(LONG_PTR)&LGI);

			if (LGI.Item.Flags&LIF_CHECKED)
				LGI.Item.Flags&=~LIF_CHECKED;
			else
				LGI.Item.Flags|=LIF_CHECKED;

			SendDlgMessage(hDlg,DM_LISTUPDATE,ID_SC_COMBO,(LONG_PTR)&LGI);
			SendDlgMessage(hDlg,DM_REDRAW,0,0);
			return TRUE;
		}
		case DN_BTNCLICK:
		{

			if (Param1==ID_SC_USEFILTER) // "Use filter"
			{
				UseFilter=(int)Param2;
				return TRUE;
			}

			if (Param1 == ID_SC_BTNTREE) // Tree
			{
				SendDlgMessage(hDlg,DM_CALLTREE,0,0);
				return FALSE;
			}
			else if (Param1 == ID_SC_BTNCOPY)
			{
				SendDlgMessage(hDlg,DM_CLOSE,ID_SC_BTNCOPY,0);
			}
			else if (Param1 == ID_SC_COPYSYMLINK)
			{
				SendDlgMessage(hDlg,DM_SETCHECK,ID_SC_COPYSYMLINKOUTER,BSTATE_UNCHECKED);
			}
			else if (Param1 == ID_SC_COPYSYMLINKOUTER)
			{
				SendDlgMessage(hDlg,DM_SETCHECK,ID_SC_COPYSYMLINK,BSTATE_UNCHECKED);				
			}
			/*
			else if(Param1 == ID_SC_ONLYNEWER && ((DlgParam->thisClass->Flags)&FCOPY_LINK))
			{
			  // подсократим код путем эмуляции телодвижений в строке ввода :-))
			  		SendDlgMessage(hDlg,DN_EDITCHANGE,ID_SC_TARGETEDIT,0);
			}
			*/
			else if (Param1==ID_SC_BTNFILTER) // Filter
			{
				Filter->FilterEdit();
				return TRUE;
			}

			break;
		}
		case DM_KEY: // по поводу дерева!
		{
			if (Param2 == KEY_ALTF10 || Param2 == KEY_F10 || Param2 == KEY_SHIFTF10)
			{
				DlgParam->AltF10=Param2 == KEY_ALTF10?1:(Param2 == KEY_SHIFTF10?2:0);
				SendDlgMessage(hDlg,DM_CALLTREE,DlgParam->AltF10,0);
				return TRUE;
			}

			if (Param1 == ID_SC_COMBO)
			{
				if (Param2==KEY_ENTER || Param2==KEY_NUMENTER || Param2==KEY_INS || Param2==KEY_NUMPAD0 || Param2==KEY_SPACE)
				{
					if (SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID_SC_COMBO,0)==CM_ASKRO)
						return SendDlgMessage(hDlg,DM_SWITCHRO,0,0);
				}
			}
		}
		break;

		case DN_LISTHOTKEY:
			if(Param1==ID_SC_COMBO)
			{
				if (SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID_SC_COMBO,0)==CM_ASKRO)
				{
					SendDlgMessage(hDlg,DM_SWITCHRO,0,0);
					return TRUE;
				}
			}
			break;
		case DN_MOUSEEVENT:

			if (SendDlgMessage(hDlg,DM_GETDROPDOWNOPENED,ID_SC_COMBO,0))
			{
				MOUSE_EVENT_RECORD *mer=(MOUSE_EVENT_RECORD *)Param2;

				if (SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID_SC_COMBO,0)==CM_ASKRO && mer->dwButtonState && !(mer->dwEventFlags&MOUSE_MOVED))
				{
					SendDlgMessage(hDlg,DM_SWITCHRO,0,0);
					return FALSE;
				}
			}

			break;
#if 0
		case DN_EDITCHANGE:

			if (Param1 == ID_SC_TARGETEDIT)
			{
				FarDialogItem *DItemACCopy,*DItemACInherit,*DItemACLeave,/**DItemOnlyNewer,*/*DItemBtnCopy;
				FARString strTmpSrcDir;
				DlgParam->thisClass->SrcPanel->GetCurDir(strTmpSrcDir);
				DItemACCopy = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACCOPY,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACCOPY,(LONG_PTR)DItemACCopy);
				DItemACInherit = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACINHERIT,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACINHERIT,(LONG_PTR)DItemACInherit);
				DItemACLeave = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACLEAVE,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ACLEAVE,(LONG_PTR)DItemACLeave);
				//DItemOnlyNewer = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ONLYNEWER,0));
				//SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_ONLYNEWER,(LONG_PTR)DItemOnlyNewer);
				DItemBtnCopy = (FarDialogItem *)xf_malloc(SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_BTNCOPY,0));
				SendDlgMessage(hDlg,DM_GETDLGITEM,ID_SC_BTNCOPY,(LONG_PTR)DItemBtnCopy);

				// не создание линка, обычные Copy/Move
				if (!(DlgParam->thisClass->Flags&FCOPY_LINK))
				{
					FARString strBuf = ((FarDialogItem *)Param2)->PtrData;
					strBuf.Upper();

					if (!DlgParam->strPluginFormat.IsEmpty() && wcsstr(strBuf, DlgParam->strPluginFormat))
					{
						DItemACCopy->Flags|=DIF_DISABLE;
						DItemACInherit->Flags|=DIF_DISABLE;
						DItemACLeave->Flags|=DIF_DISABLE;
						//DItemOnlyNewer->Flags|=DIF_DISABLE;
						//DlgParam->OnlyNewerFiles=DItemOnlyNewer->Param.Selected;
						DlgParam->CopySecurity=0;

						if (DItemACCopy->Param.Selected)
							DlgParam->CopySecurity=1;
						else if (DItemACLeave->Param.Selected)
							DlgParam->CopySecurity=2;

						DItemACCopy->Param.Selected=0;
						DItemACInherit->Param.Selected=0;
						DItemACLeave->Param.Selected=1;
						//DItemOnlyNewer->Param.Selected=0;
					}
					else
					{
						DItemACCopy->Flags&=~DIF_DISABLE;
						DItemACInherit->Flags&=~DIF_DISABLE;
						DItemACLeave->Flags&=~DIF_DISABLE;
						//DItemOnlyNewer->Flags&=~DIF_DISABLE;
						//DItemOnlyNewer->Param.Selected=DlgParam->OnlyNewerFiles;
						DItemACCopy->Param.Selected=0;
						DItemACInherit->Param.Selected=0;
						DItemACLeave->Param.Selected=0;

						if (DlgParam->CopySecurity == 1)
						{
							DItemACCopy->Param.Selected=1;
						}
						else if (DlgParam->CopySecurity == 2)
						{
							DItemACLeave->Param.Selected=1;
						}
						else
							DItemACInherit->Param.Selected=1;
					}
				}

				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_SC_ACCOPY,(LONG_PTR)DItemACCopy);
				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_SC_ACINHERIT,(LONG_PTR)DItemACInherit);
				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_SC_ACLEAVE,(LONG_PTR)DItemACLeave);
				//SendDlgMessage(hDlg,DM_SETDLGITEM,ID_SC_ONLYNEWER,(LONG_PTR)DItemOnlyNewer);
				SendDlgMessage(hDlg,DM_SETDLGITEM,ID_SC_BTNCOPY,(LONG_PTR)DItemBtnCopy);
				xf_free(DItemACCopy);
				xf_free(DItemACInherit);
				xf_free(DItemACLeave);
				//xf_free(DItemOnlyNewer);
				xf_free(DItemBtnCopy);
			}

			break;
#endif
		case DM_CALLTREE:
		{
			/* $ 13.10.2001 IS
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
			BOOL MultiCopy=SendDlgMessage(hDlg,DM_GETCHECK,ID_SC_MULTITARGET,0)==BSTATE_CHECKED;
			FARString strOldFolder;
			int nLength;
			FarDialogItemData Data;
			nLength = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, ID_SC_TARGETEDIT, 0);
			Data.PtrData = strOldFolder.GetBuffer(nLength+1);
			Data.PtrLength = nLength;
			SendDlgMessage(hDlg,DM_GETTEXT,ID_SC_TARGETEDIT,(LONG_PTR)&Data);
			strOldFolder.ReleaseBuffer();
			FARString strNewFolder;

			if (DlgParam->AltF10 == 2)
			{
				strNewFolder = strOldFolder;

				if (MultiCopy)
				{
					UserDefinedList DestList(0,0,ULF_UNIQUE);

					if (DestList.Set(strOldFolder))
					{
						DestList.Reset();
						const wchar_t *NamePtr=DestList.GetNext();

						if (NamePtr)
							strNewFolder = NamePtr;
					}
				}

				if (strNewFolder.IsEmpty())
					DlgParam->AltF10=-1;
				else // убираем лишний слеш
					DeleteEndSlash(strNewFolder);
			}

			if (DlgParam->AltF10 != -1)
			{
				{
					FARString strNewFolder2;
					FolderTree Tree(strNewFolder2,
					                (DlgParam->AltF10==1?MODALTREE_PASSIVE:
					                 (DlgParam->AltF10==2?MODALTREE_FREE:
					                  MODALTREE_ACTIVE)),
					                FALSE,FALSE);
					strNewFolder = strNewFolder2;
				}

				if (!strNewFolder.IsEmpty())
				{
					AddEndSlash(strNewFolder);

					if (MultiCopy) // мультикопирование
					{
						// Добавим кавычки, если имя каталога содержит символы-разделители
						if (wcspbrk(strNewFolder,L";,"))
							InsertQuote(strNewFolder);

						if (strOldFolder.GetLength())
							strOldFolder += L";"; // добавим разделитель к непустому списку

						strOldFolder += strNewFolder;
						strNewFolder = strOldFolder;
					}

					SendDlgMessage(hDlg,DM_SETTEXTPTR,ID_SC_TARGETEDIT,(LONG_PTR)strNewFolder.CPtr());
					SendDlgMessage(hDlg,DM_SETFOCUS,ID_SC_TARGETEDIT,0);
				}
			}

			DlgParam->AltF10=0;
			return TRUE;
		}
		case DN_CLOSE:
		{
			if (Param1==ID_SC_BTNCOPY)
			{
				FarListGetItem LGI={CM_ASKRO};
				SendDlgMessage(hDlg,DM_LISTGETITEM,ID_SC_COMBO,(LONG_PTR)&LGI);

				if (LGI.Item.Flags&LIF_CHECKED)
					DlgParam->AskRO=TRUE;
			}
		}
		break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

ShellCopy::~ShellCopy()
{
	_tran(SysLog(L"[%p] ShellCopy::~ShellCopy(), CopyBufer=%p",this,CopyBuffer));

	if (CopyBufferBase)
		delete[] CopyBufferBase;

	// $ 26.05.2001 OT Разрешить перерисовку панелей
	_tran(SysLog(L"call (*FrameManager)[0]->UnlockRefresh()"));
	(*FrameManager)[0]->Unlock();
	(*FrameManager)[0]->Refresh();

	if (sddata)
		delete[] sddata;

	if (Filter) // Уничтожим объект фильтра
		delete Filter;

	if (CP)
	{
		delete CP;
		CP=nullptr;
	}
}


COPY_CODES ShellCopy::CopyFileTree(const wchar_t *Dest)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	//SaveScreen SaveScr;
	DWORD DestAttr=INVALID_FILE_ATTRIBUTES;
	FARString strSelName;
	int Length;
	DWORD FileAttr;
	
	if (!(Length=StrLength(Dest)) || !StrCmp(Dest,L"."))
		return COPY_FAILURE; //????

	SetCursorType(FALSE,0);
	Flags&=~(FCOPY_STREAMSKIP|FCOPY_STREAMALL);

	if (!TotalCopySize)
	{
		strTotalCopySizeText.Clear();

		//  ! Не сканируем каталоги при создании линков
		if (ShowTotalCopySize && !(Flags&FCOPY_LINK) && !CalcTotalSize())
			return COPY_FAILURE;
	}
	else
	{
		CurCopiedSize=0;
	}

	// Создание структуры каталогов в месте назначения
	if (!(Flags&FCOPY_COPYTONUL))
	{
		//if (Length > 1 && Dest[Length-1]=='/' && Dest[Length-2]!=':') //??????????
		{
			FARString strNewPath = Dest;

			if (!IsSlash(strNewPath.At(strNewPath.GetLength()-1)) &&
			        SrcPanel->GetSelCount()>1 &&
			        !wcspbrk(strNewPath,L"*?") &&
			        apiGetFileAttributes(strNewPath)==INVALID_FILE_ATTRIBUTES)
			{
				switch (Message(FMSG_WARNING,3,MSG(MWarning),strNewPath,MSG(MCopyDirectoryOrFile),MSG(MCopyDirectoryOrFileDirectory),MSG(MCopyDirectoryOrFileFile),MSG(MCancel)))
				{
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

			if (FindLastSlash(pos,strNewPath))
			{
				strNewPath.SetLength(pos+1);

				DWORD Attr=apiGetFileAttributes(strNewPath);

				if (Attr==INVALID_FILE_ATTRIBUTES)
				{
					if (apiCreateDirectory(strNewPath,nullptr))
						TreeList::AddTreeName(strNewPath);
					else
						CreatePath(strNewPath);
				}
				else if (!(Attr & FILE_ATTRIBUTE_DIRECTORY))
				{
					Message(MSG_WARNING,1,MSG(MError),MSG(MCopyCannotCreateFolder),strNewPath,MSG(MOk));
					return COPY_FAILURE;
				}
			}
		}

		DestAttr=apiGetFileAttributes(Dest);
	}

	// Выставим признак "Тот же диск"
	bool AllowMoveByOS=false;

	if ( (Flags & (FCOPY_COPYSYMLINKCONTENTS|FCOPY_COPYSYMLINKCONTENTSOUTER|FCOPY_MOVE)) == FCOPY_MOVE)
	{
		FARString strTmpSrcDir;
		SrcPanel->GetCurDir(strTmpSrcDir);
		AllowMoveByOS=(CheckDisksProps(strTmpSrcDir,Dest,CHECKEDPROPS_ISSAMEDISK))!=0;
	}
	

	// Основной цикл копирования одной порции.
	SrcPanel->GetSelNameCompat(nullptr,FileAttr);
	{
		while (SrcPanel->GetSelNameCompat(&strSelName,FileAttr))
		{
			FARString strDest = Dest;

			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
				SelectedFolderNameLength=(int)strSelName.GetLength();
			else
				SelectedFolderNameLength=0;

			if (!(Flags&FCOPY_COPYTONUL))
			{
				if (wcspbrk(Dest,L"*?"))
					ConvertWildcards(strSelName, strDest, SelectedFolderNameLength);

				DestAttr=apiGetFileAttributes(strDest);
			}

			FARString strDestPath = strDest;
			FAR_FIND_DATA_EX SrcData;
			int CopyCode=COPY_SUCCESS,KeepPathPos;
			Flags&=~FCOPY_OVERWRITENEXT;


/*			if (!AreSymlinksSupported(Wide2MB(Dest).c_str()) && AreSymlinksSupported(Wide2MB(Src).c_str())) {
				// "замочим" к едрене фени симлинк - копируем полный контент, независимо от опции
				// (но не для случая переименования линка)
				Flags&= ~FCOPY_COPYSYMLINKCONTENTSOUTER;
				Flags|=FCOPY_COPYSYMLINKCONTENTS;
			}*/

			KeepPathPos=(int)(PointToName(strSelName)-strSelName.CPtr());

			if (RPT==RP_JUNCTION || RPT==RP_SYMLINK || RPT==RP_SYMLINKFILE || RPT==RP_SYMLINKDIR)
			{
				switch (MkSymLink(strSelName,strDest,RPT,Flags))
				{
					case 2:
						break;
					case 1:

						// Отметим (Ins) несколько каталогов, ALT-F6 Enter - выделение с папок не снялось.
						if ((!(Flags&FCOPY_CURRENTONLY)) && (Flags&FCOPY_COPYLASTTIME))
							SrcPanel->ClearLastGetSelection();

						continue;
					case 0:
						return COPY_FAILURE;
				}
			}
			else
			{
				// проверка на вшивость ;-)
				if (!apiGetFindDataEx(strSelName,SrcData))
				{
					strDestPath = strSelName;
					CP->SetNames(strSelName,strDestPath);

					if (Message(MSG_WARNING,2,MSG(MError),MSG(MCopyCannotFind),
					            strSelName,MSG(MSkip),MSG(MCancel))==1)
					{
						return COPY_FAILURE;
					}

					continue;
				}
			}


			//KeepPathPos=PointToName(SelName)-SelName;

			// Мувим?
			if ((Flags&FCOPY_MOVE))
			{
				// Тыкс, а как на счет "тот же диск"?
				if (KeepPathPos && PointToName(strDest)==strDest &&
					(Flags & (FCOPY_COPYSYMLINKCONTENTS|FCOPY_COPYSYMLINKCONTENTSOUTER)) == 0)
				{
					strDestPath = strSelName;
					strDestPath.SetLength(KeepPathPos);
					strDestPath += strDest;
					AllowMoveByOS=true;
				}

				if ((UseFilter || !AllowMoveByOS) || //can't move across different devices
				//if any symlinks copy may occur - parse whole tree
					( (SrcData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)) != 0 && 
						(Flags & (FCOPY_COPYSYMLINKCONTENTS|FCOPY_COPYSYMLINKCONTENTSOUTER)) != 0 ) ) 
				{
					CopyCode=COPY_FAILURE;
				}
				else
				{
					CopyCode=ShellCopyOneFile(strSelName,SrcData,strDestPath,KeepPathPos,1);

					if (CopyCode==COPY_SUCCESS_MOVE)
					{
						if (!strDestDizPath.IsEmpty())
						{
							if (!strRenamedName.IsEmpty())
							{
								DestDiz.DeleteDiz(strSelName);
								SrcPanel->CopyDiz(strSelName,strRenamedName,&DestDiz);
							}
							else
							{
								if (strCopiedName.IsEmpty())
									strCopiedName = strSelName;

								SrcPanel->CopyDiz(strSelName,strCopiedName,&DestDiz);
								SrcPanel->DeleteDiz(strSelName);
							}
						}

						continue;
					}

					if (CopyCode==COPY_CANCEL)
						return COPY_CANCEL;

					if (CopyCode==COPY_NEXT)
					{
						uint64_t CurSize = SrcData.nFileSize;
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
						continue;
					}

					if (!(Flags&FCOPY_MOVE) || CopyCode==COPY_FAILURE)
						Flags|=FCOPY_OVERWRITENEXT;
				}
			}

			if (!(Flags&FCOPY_MOVE) || CopyCode==COPY_FAILURE)
			{
				FARString strCopyDest=strDest;

				CopyCode=ShellCopyOneFile(strSelName,SrcData,strCopyDest,KeepPathPos,0);

				Flags&=~FCOPY_OVERWRITENEXT;

				if (CopyCode==COPY_CANCEL)
					return COPY_CANCEL;

				if (CopyCode!=COPY_SUCCESS)
				{
					uint64_t CurSize = SrcData.nFileSize;

					if (CopyCode != COPY_NOFILTER) //????
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;

					if (CopyCode == COPY_NEXT)
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;

					continue;
				}
			}

			if (CopyCode==COPY_SUCCESS && !(Flags&FCOPY_COPYTONUL) && !strDestDizPath.IsEmpty())
			{
				if (strCopiedName.IsEmpty())
					strCopiedName = strSelName;

				SrcPanel->CopyDiz(strSelName,strCopiedName,&DestDiz);
			}


			// Mantis#44 - Потеря данных при копировании ссылок на папки
			// если каталог (или нужно копировать симлинк) - придется рекурсивно спускаться...
			if (RPT!=RP_SYMLINKFILE && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			        (
			            !(SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) ||
			            ((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS))
			        )
			   )
			{
				int SubCopyCode;
				FARString strSubName;
				FARString strFullName;
				ScanTree ScTree(TRUE,TRUE,Flags&FCOPY_COPYSYMLINKCONTENTS);
				strSubName = strSelName;
				strSubName += L"/";

				if (DestAttr==INVALID_FILE_ATTRIBUTES)
					KeepPathPos=(int)strSubName.GetLength();

				int NeedRename=!((SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) && (Flags&FCOPY_COPYSYMLINKCONTENTS) && (Flags&FCOPY_MOVE));
				ScTree.SetFindPath(strSubName,L"*",FSCANTREE_FILESFIRST);

				while (ScTree.GetNextName(&SrcData,strFullName))
				{
					if (UseFilter && (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						// Просто пропустить каталог недостаточно - если каталог помечен в
						// фильтре как некопируемый, то следует пропускать и его и всё его
						// содержимое.
						if (!Filter->FileInFilter(SrcData))
						{
							ScTree.SkipDir();
							continue;
						}
					}
					{
						int AttemptToMove=FALSE;

						if ((Flags&FCOPY_MOVE) && (!UseFilter && AllowMoveByOS) && !(SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						{
							AttemptToMove=TRUE;
							int Ret=COPY_SUCCESS;
							FARString strCopyDest=strDest;

							Ret=ShellCopyOneFileWithRoot(strSubName.CPtr(),strFullName,SrcData,strCopyDest,KeepPathPos,NeedRename);

							switch (Ret) // 1
							{
								case COPY_CANCEL:
									return COPY_CANCEL;
								case COPY_NEXT:
								{
									uint64_t CurSize = SrcData.nFileSize;
									TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
									TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
									continue;
								}
								case COPY_SUCCESS_MOVE:
								{
									continue;
								}
								case COPY_SUCCESS:

									if (!NeedRename) // вариант при перемещении содержимого симлика с опцией "копировать содержимое сим..."
									{
										uint64_t CurSize = SrcData.nFileSize;
										TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
										TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
										continue;     // ...  т.к. мы ЭТО не мувили, а скопировали, то все, на этом закончим бадаться с этим файлов
									}
							}
						}

						int SaveOvrMode=OvrMode;

						if (AttemptToMove)
							OvrMode=1;

						FARString strCopyDest=strDest;

						SubCopyCode=ShellCopyOneFileWithRoot(strSubName.CPtr(),strFullName,SrcData,strCopyDest,KeepPathPos,0);

						if (AttemptToMove)
							OvrMode=SaveOvrMode;
					}

					if (SubCopyCode==COPY_CANCEL)
						return COPY_CANCEL;

					if (SubCopyCode==COPY_NEXT)
					{
						uint64_t CurSize = SrcData.nFileSize;
						TotalCopiedSize = TotalCopiedSize - CurCopiedSize + CurSize;
						TotalSkippedSize = TotalSkippedSize + CurSize - CurCopiedSize;
					}

					if (SubCopyCode==COPY_SUCCESS)
					{
						if (Flags&FCOPY_MOVE)
						{
							if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							{
								if (ScTree.IsDirSearchDone() ||
								        ((SrcData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && !(Flags&FCOPY_COPYSYMLINKCONTENTS)))
								{
									TemporaryMakeWritable tmw(strFullName);
									//if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
									//	apiMakeWritable(strFullName); //apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

									if (apiRemoveDirectory(strFullName))
										TreeList::DelTreeName(strFullName);
								}
							}
							// здесь нужны проверка на FSCANTREE_INSIDEJUNCTION, иначе
							// при мовинге будет удаление файла, что крайне неправильно!
							else if (!ScTree.InsideJunction())
							{
								if (DeleteAfterMove(strFullName,SrcData.dwFileAttributes)==COPY_CANCEL)
									return COPY_CANCEL;
							}
						}
					}
				}

				if ((Flags&FCOPY_MOVE) && CopyCode==COPY_SUCCESS)
				{
					TemporaryMakeWritable tmw(strSelName);
					//if (FileAttr & FILE_ATTRIBUTE_READONLY)
					//	apiMakeWritable(strSelName); //apiSetFileAttributes(strSelName,FILE_ATTRIBUTE_NORMAL);

					if (apiRemoveDirectory(strSelName))
					{
						TreeList::DelTreeName(strSelName);

						if (!strDestDizPath.IsEmpty())
							SrcPanel->DeleteDiz(strSelName);
					}
				}
			}
			else if ((Flags&FCOPY_MOVE) && CopyCode==COPY_SUCCESS)
			{
				int DeleteCode;

				if ((DeleteCode=DeleteAfterMove(strSelName,FileAttr))==COPY_CANCEL)
					return COPY_CANCEL;

				if (DeleteCode==COPY_SUCCESS && !strDestDizPath.IsEmpty())
					SrcPanel->DeleteDiz(strSelName);
			}

			if ((!(Flags&FCOPY_CURRENTONLY)) && (Flags&FCOPY_COPYLASTTIME))
			{
				SrcPanel->ClearLastGetSelection();
			}
		}
	}
	return COPY_SUCCESS; //COPY_SUCCESS_MOVE???
}



static bool IsOuterTarget(const wchar_t *Root, const wchar_t *SymLink)
{
	//fprintf(stderr, "IsOuterTarget('%ls', '%ls')\n", Root, SymLink);
	if (!Root || !*Root || !SymLink || !*SymLink)
		return true;

	FARString strRoot, strTarget;
	ConvertNameToReal(Root, strRoot);
	ConvertNameToReal(SymLink, strTarget);
	
	if (strTarget.GetLength() < strRoot.GetLength())
		return true;

	if (memcmp(strTarget.CPtr(), strRoot.CPtr(), strRoot.GetLength() * sizeof(wchar_t)) != 0)
		return true;

	if (Root[strRoot.GetLength() - 1] != GOOD_SLASH && 
		strTarget.GetLength() > strRoot.GetLength() && strTarget[strRoot.GetLength()] != GOOD_SLASH) {
		return true;
	}

	return false;
}

COPY_CODES ShellCopy::DumbCopySymLink(const wchar_t *Target, const wchar_t *NewName, const FAR_FIND_DATA_EX &SrcData)
{
	int r = sdc_symlink( Wide2MB(Target).c_str() , Wide2MB(NewName).c_str() );
	if (r == 0) 
		return COPY_SUCCESS;

	if (errno == EEXIST ) {
		int RetCode = 0, Append = 0;
		FARString strNewName = NewName;
		if (AskOverwrite(SrcData, Target, NewName, 0, 0, 0, 0, Append, strNewName, RetCode)) {
			if (strNewName == NewName) {
				fprintf(stderr, 
					"DumbCopySymLink('%ls', '%ls') - overwriting and strNewName='%ls'\n", 
					Target, NewName, strNewName.CPtr());
				sdc_remove(strNewName.GetMB().c_str() );
				sdc_rmdir(strNewName.GetMB().c_str() );
			} else {
				fprintf(stderr, 
					"DumbCopySymLink('%ls', '%ls') - renaming and strNewName='%ls'\n", 
					Target, NewName, strNewName.CPtr());
			}
			return DumbCopySymLink(Target, strNewName.CPtr(), SrcData);
		}
			
		return (COPY_CODES)RetCode;
	} 
	
	
	switch (Message(MSG_WARNING, 3 ,MSG(MError),
			MSG(MCopyCannotCreateSymlinkAskCopyContents),
			NewName, MSG(MYes), MSG(MSkip), MSG(MCancel)   ))
	{
		case 0: 
			Flags&= ~FCOPY_COPYSYMLINKCONTENTSOUTER;
			Flags|= FCOPY_COPYSYMLINKCONTENTS;
			return COPY_RETRY;

		case 1: 
			return COPY_FAILURE;

		case 2: default:
			return COPY_CANCEL;
	}
}

COPY_CODES ShellCopy::CopySymLink(const wchar_t *Root, const wchar_t *ExistingName, 
						const wchar_t *NewName, ReparsePointTypes LinkType, const FAR_FIND_DATA_EX &SrcData)
{
	FARString strRealName;
	ConvertNameToReal(ExistingName, strRealName);

	//fprintf(stderr, "CopySymLink('%ls', '%ls', '%ls') '%ls'\n", Root, ExistingName, NewName, strRealName.CPtr());

	if (IsOuterTarget(Root, ExistingName))  {
		FARString strNewName;
		ConvertNameToFull(NewName, strNewName);
		return DumbCopySymLink(strRealName.CPtr(), strNewName.CPtr(), SrcData);
	}

	FARString strExistingName;
	ConvertNameToFull(ExistingName, strExistingName);
	
	std::wstring relative_name;
	for (size_t i = 0;; ++i) {
		if (i == strRealName.GetLength() || i == strExistingName.GetLength() || strRealName[i] != strExistingName[i]) {
			assert( i > 0 );
			for (--i; (i > 0 && strRealName[i] != GOOD_SLASH); --i);
			assert( i > 0 );
			relative_name = strRealName.CPtr() + i + 1;
			for (++i; i < strExistingName.GetLength(); ++i) {
				if (strExistingName[i] == GOOD_SLASH) {
					relative_name.insert(0, L"../");
				}
			}

			break;
		}
	}

	if (relative_name.empty()) {
		fprintf(stderr, "CopySymLink: empty relative name for strRealName='%ls' strExistingName='%ls'\n",strRealName.CPtr(), strExistingName.CPtr());
	}
	assert(!relative_name.empty());

	return DumbCopySymLink(relative_name.c_str(), NewName, SrcData);
}


// абсолютно невменяемая функция. функция таких размеров вменяема быть не может. переписать ASAP

COPY_CODES ShellCopy::ShellCopyOneFile(const wchar_t *Src,
					const FAR_FIND_DATA_EX &SrcData,
					FARString &strDest,
					int KeepPathPos, int Rename)
{
	return ShellCopyOneFileWithRoot(nullptr, Src, SrcData, strDest, KeepPathPos, Rename);
}

COPY_CODES ShellCopy::ShellCopyOneFileWithRoot(
    const wchar_t *Root,
    const wchar_t *Src,
    const FAR_FIND_DATA_EX &SrcData,
    FARString &strDest,
    int KeepPathPos,
    int Rename
)
{
	for (;;) {
		COPY_CODES out = ShellCopyOneFileWithRootNoRetry(Root, Src, SrcData, strDest, KeepPathPos, Rename);
		if (out != COPY_RETRY)
			return out;
	}
}

COPY_CODES ShellCopy::ShellCopyOneFileWithRootNoRetry(
    const wchar_t *Root,
    const wchar_t *Src,
    const FAR_FIND_DATA_EX &SrcData,
    FARString &strDest,
    int KeepPathPos,
    int Rename
)
{
	CurCopiedSize = 0; // Сбросить текущий прогресс

	if (CP->Cancelled())
	{
		return(COPY_CANCEL);
	}

	if (UseFilter)
	{
		if (!Filter->FileInFilter(SrcData))
			return COPY_NOFILTER;
	}

	FARString strDestPath = strDest;
	const wchar_t *NamePtr=PointToName(strDestPath);
	DWORD DestAttr=INVALID_FILE_ATTRIBUTES;

	if (strDestPath.At(0)==GOOD_SLASH && strDestPath.At(1)==GOOD_SLASH)
	{
		FARString strRoot;
		GetPathRoot(strDestPath, strRoot);
		DeleteEndSlash(strRoot);

		if (!StrCmp(strDestPath,strRoot))
			DestAttr=FILE_ATTRIBUTE_DIRECTORY;
	}

	if (!*NamePtr || TestParentFolderName(NamePtr))
		DestAttr=FILE_ATTRIBUTE_DIRECTORY;

	FAR_FIND_DATA_EX DestData;
	if (DestAttr==INVALID_FILE_ATTRIBUTES)
	{
		if (apiGetFindDataEx(strDestPath,DestData))
			DestAttr=DestData.dwFileAttributes;
	}
	
	int SameName=0, Append=0;

	if (DestAttr!=INVALID_FILE_ATTRIBUTES && (DestAttr & FILE_ATTRIBUTE_DIRECTORY))
	{
		int CmpCode=CmpFullNames(Src,strDestPath);

		if(CmpCode && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && RPT==RP_EXACTCOPY && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
		{
			CmpCode = 0;
		}

		if (CmpCode==1) // TODO: error check
		{
			SameName=1;

			if (Rename)
			{
				CmpCode=!StrCmp(PointToName(Src),PointToName(strDestPath));
			}

			if (CmpCode==1)
			{
				SetMessageHelp(L"ErrCopyItSelf");
				Message(MSG_WARNING,1,MSG(MError),MSG(MCannotCopyFolderToItself1),
				        Src,MSG(MCannotCopyFolderToItself2),MSG(MOk));
				return(COPY_CANCEL);
			}
		}

		if (!SameName)
		{
			int Length=(int)strDestPath.GetLength();

			if (!IsSlash(strDestPath.At(Length-1)) && strDestPath.At(Length-1)!=L':')
				strDestPath += L"/";

			const wchar_t *PathPtr=Src+KeepPathPos;

			if (*PathPtr && !KeepPathPos && PathPtr[1]==L':')
				PathPtr+=2;

			if (IsSlash(*PathPtr))
				PathPtr++;

			strDestPath += PathPtr;

			if (!apiGetFindDataEx(strDestPath,DestData))
				DestAttr=INVALID_FILE_ATTRIBUTES;
			else
				DestAttr=DestData.dwFileAttributes;
		}
	}

	if (!(Flags&FCOPY_COPYTONUL))
		SetDestDizPath(strDestPath);

	CP->SetProgressValue(0,0);
	CP->SetNames(Src,strDestPath);

	if (!(Flags&FCOPY_COPYTONUL))
	{
		const bool copy_sym_link = (RPT == RP_EXACTCOPY && 
			SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && 
			!(Flags&FCOPY_COPYSYMLINKCONTENTS) &&
			(!(Flags&FCOPY_COPYSYMLINKCONTENTSOUTER) || !IsOuterTarget(Root, Src))  );
		
		if (SrcData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY || copy_sym_link)
		{
			if (!Rename)
				strCopiedName = PointToName(strDestPath);

			if (DestAttr!=INVALID_FILE_ATTRIBUTES)
			{
				if ((DestAttr & FILE_ATTRIBUTE_DIRECTORY) && !SameName)
				{
					DWORD SetAttr=SrcData.dwFileAttributes;

					if (SetAttr!=DestAttr)
						ShellSetAttr(strDestPath,SetAttr);

					FARString strSrcFullName;
					ConvertNameToFull(Src,strSrcFullName);
					return(!StrCmp(strDestPath,strSrcFullName) ? COPY_NEXT:COPY_SUCCESS);
				}

				int Type=apiGetFileTypeByName(strDestPath);

				if (Type==FILE_TYPE_CHAR || Type==FILE_TYPE_PIPE)
					return(Rename ? COPY_NEXT:COPY_SUCCESS);
			}

			if (Rename)
			{
				FARString strSrcFullName,strDestFullName;
				ConvertNameToFull(Src,strSrcFullName);

				// Пытаемся переименовать, пока не отменят
				for (;;)
				{
					BOOL SuccessMove=apiMoveFile(Src,strDestPath);

					if (SuccessMove)
					{
						if (PointToName(strDestPath)==strDestPath.CPtr())
							strRenamedName = strDestPath;
						else
							strCopiedName = PointToName(strDestPath);

						ConvertNameToFull(strDest, strDestFullName);
						TreeList::RenTreeName(strSrcFullName,strDestFullName);
						return(SameName ? COPY_NEXT:COPY_SUCCESS_MOVE);
					}
					else
					{
						int MsgCode = Message(MSG_WARNING|MSG_ERRORTYPE,3,MSG(MError),
						                      MSG(MCopyCannotRenameFolder),Src,MSG(MCopyRetry),
						                      MSG(MCopyIgnore),MSG(MCopyCancel));

						switch (MsgCode)
						{
							case 0:  continue;
							case 1:
							{

								if (apiCreateDirectory(strDestPath, nullptr))
								{
									if (PointToName(strDestPath)==strDestPath.CPtr())
										strRenamedName = strDestPath;
									else
										strCopiedName = PointToName(strDestPath);

									TreeList::AddTreeName(strDestPath);
									return(COPY_SUCCESS);
								}
							}
							default:
								return (COPY_CANCEL);
						} /* switch */
					} /* else */
				} /* while */
			} // if (Rename)
			if (RPT!=RP_SYMLINKFILE && 
				(SrcData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) == FILE_ATTRIBUTE_DIRECTORY)
			{
				while (!apiCreateDirectoryEx(Src, strDestPath, nullptr))
				{
					int MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,3,MSG(MError),
					                MSG(MCopyCannotCreateFolder),strDestPath,MSG(MCopyRetry),
					                MSG(MCopySkip),MSG(MCopyCancel));

					if (MsgCode)
						return((MsgCode==-2 || MsgCode==2) ? COPY_CANCEL:COPY_NEXT);
				}
			}
			// [ ] Copy contents of symbolic links
			if (copy_sym_link)
			{
				COPY_CODES CopyRetCode = CopySymLink(Root, Src, strDestPath, RPT, SrcData);
				if (CopyRetCode != COPY_SUCCESS && CopyRetCode != COPY_SUCCESS_MOVE)
					return CopyRetCode;
			}
			TreeList::AddTreeName(strDestPath);
			return COPY_SUCCESS;
		}

		if (DestAttr!=INVALID_FILE_ATTRIBUTES && !(DestAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (SrcData.nFileSize==DestData.nFileSize)
			{
				int CmpCode=CmpFullNames(Src,strDestPath);

				if(CmpCode && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT && RPT==RP_EXACTCOPY && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
				{
					CmpCode = 0;
				}

				if (CmpCode==1) // TODO: error check
				{
					SameName=1;

					if (Rename)
					{
						CmpCode=!StrCmp(PointToName(Src),PointToName(strDestPath));
					}

					if (CmpCode==1 && !Rename)
					{
						Message(MSG_WARNING,1,MSG(MError),MSG(MCannotCopyFileToItself1),
							    Src,MSG(MCannotCopyFileToItself2),MSG(MOk));
						return(COPY_CANCEL);
					}
				}
			}
			int RetCode=0;
			FARString strNewName;

			if (!AskOverwrite(SrcData,Src,strDestPath,DestAttr,SameName,Rename,((Flags&FCOPY_LINK)?0:1),Append,strNewName,RetCode))
			{
				return((COPY_CODES)RetCode);
			}

			if (RetCode==COPY_RETRY)
			{
				strDest=strNewName;

				if (CutToSlash(strNewName) && apiGetFileAttributes(strNewName)==INVALID_FILE_ATTRIBUTES)
				{
					CreatePath(strNewName);
				}

				return COPY_RETRY;
			}
		}
	}
	else
	{
		if (SrcData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return COPY_SUCCESS;
		}
	}

	int NWFS_Attr=(Opt.Nowell.MoveRO && !StrCmp(strDestFSName,L"NWFS"))?TRUE:FALSE;
	{
		for (;;)
		{
			int CopyCode=0;
			uint64_t SaveTotalSize=TotalCopiedSize;

			if (!(Flags&FCOPY_COPYTONUL) && Rename)
			{
				int MoveCode=FALSE,AskDelete;

				if ((!StrCmp(strDestFSName,L"NWFS")) && !Append &&
				        DestAttr!=INVALID_FILE_ATTRIBUTES && !SameName)
				{
					apiDeleteFile(strDestPath); //BUGBUG
				}

				if (!Append)
				{
					FARString strSrcFullName;
					ConvertNameToFull(Src,strSrcFullName);

					if (NWFS_Attr)
						apiSetFileAttributes(strSrcFullName,SrcData.dwFileAttributes&(~FILE_ATTRIBUTE_READONLY));

					if (!StrCmp(strDestFSName,L"NWFS"))
						MoveCode=apiMoveFile(strSrcFullName,strDestPath);
					else
						MoveCode=apiMoveFileEx(strSrcFullName,strDestPath,SameName ? MOVEFILE_COPY_ALLOWED:MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);

					if (!MoveCode)
					{
						int MoveLastError=WINPORT(GetLastError)();

						if (NWFS_Attr)
							apiSetFileAttributes(strSrcFullName,SrcData.dwFileAttributes);

						if (MoveLastError==ERROR_NOT_SAME_DEVICE)
							return COPY_FAILURE;

						WINPORT(SetLastError)(MoveLastError);
					}

					if (NWFS_Attr)
						apiSetFileAttributes(strDestPath,SrcData.dwFileAttributes);

					if (ShowTotalCopySize && MoveCode)
					{
						TotalCopiedSize+=SrcData.nFileSize;
						CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
					}

					AskDelete=0;
				}
				else
				{
					do
					{
						DWORD Attr=INVALID_FILE_ATTRIBUTES;
						CopyCode=ShellCopyFile(Src,SrcData,strDestPath,Attr,Append);
					}
					while (CopyCode==COPY_RETRY);

					switch (CopyCode)
					{
						case COPY_SUCCESS:
							MoveCode=TRUE;
							break;
						case COPY_FAILUREREAD:
						case COPY_FAILURE:
							MoveCode=FALSE;
							break;
						case COPY_CANCEL:
							return COPY_CANCEL;
						case COPY_NEXT:
							return COPY_NEXT;
					}

					AskDelete=1;
				}

				if (MoveCode)
				{
					if (DestAttr==INVALID_FILE_ATTRIBUTES || !(DestAttr & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (PointToName(strDestPath)==strDestPath.CPtr())
							strRenamedName = strDestPath;
						else
							strCopiedName = PointToName(strDestPath);
					}

					TotalFiles++;

					if (AskDelete && DeleteAfterMove(Src,SrcData.dwFileAttributes)==COPY_CANCEL)
						return COPY_CANCEL;

					return(COPY_SUCCESS_MOVE);
				}
			}
			else
			{
				do
				{
					CopyCode=ShellCopyFile(Src,SrcData,strDestPath,DestAttr,Append);
				}
				while (CopyCode==COPY_RETRY);

				if (CopyCode==COPY_SUCCESS)
				{
					strCopiedName = PointToName(strDestPath);

					if (!(Flags&FCOPY_COPYTONUL))
					{
						if (DestAttr!=INVALID_FILE_ATTRIBUTES && !StrCmp(strCopiedName,DestData.strFileName) &&
						        StrCmp(strCopiedName,DestData.strFileName))
							apiMoveFile(strDestPath,strDestPath); //???
					}

					TotalFiles++;

					if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
						apiSetFileAttributes(strDestPath,DestAttr);

					return COPY_SUCCESS;
				}
				else if (CopyCode==COPY_CANCEL || CopyCode==COPY_NEXT)
				{
					if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
						apiSetFileAttributes(strDestPath,DestAttr);

					return((COPY_CODES)CopyCode);
				}

				if (DestAttr!=INVALID_FILE_ATTRIBUTES && Append)
					apiSetFileAttributes(strDestPath,DestAttr);
			}

			//????
			if (CopyCode == COPY_FAILUREREAD)
				return COPY_FAILURE;

			//????
			FARString strMsg1, strMsg2;
			int MsgMCannot=(Flags&FCOPY_LINK) ? MCannotLink: (Flags&FCOPY_MOVE) ? MCannotMove: MCannotCopy;
			strMsg1 = Src;
			strMsg2 = strDestPath;
			InsertQuote(strMsg1);
			InsertQuote(strMsg2);

			int MsgCode;

			if (SkipMode!=-1)
				MsgCode=SkipMode;
			else
			{
				MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),
						        MSG(MsgMCannot),
						        strMsg1,
						        MSG(MCannotCopyTo),
						        strMsg2,
						        MSG(MCopyRetry),MSG(MCopySkip),
						        MSG(MCopySkipAll),MSG(MCopyCancel));
			}

			switch (MsgCode)
			{
				case -1:
				case  1:
					return COPY_NEXT;
				case  2:
					SkipMode=1;
					return COPY_NEXT;
				case -2:
				case  3:
					return COPY_CANCEL;
			}

			TotalCopiedSize=SaveTotalSize;
			int RetCode;
			FARString strNewName;

			if (!AskOverwrite(SrcData,Src,strDestPath,DestAttr,SameName,Rename,((Flags&FCOPY_LINK)?0:1),Append,strNewName,RetCode))
				return((COPY_CODES)RetCode);

			if (RetCode==COPY_RETRY)
			{
				strDest=strNewName;

				if (CutToSlash(strNewName) && apiGetFileAttributes(strNewName)==INVALID_FILE_ATTRIBUTES)
				{
					CreatePath(strNewName);
				}

				return COPY_RETRY;
			}
		}
	}
}

int ShellCopy::DeleteAfterMove(const wchar_t *Name,DWORD Attr)
{
	if (Attr & FILE_ATTRIBUTE_READONLY)
	{
		int MsgCode;

		if (!Opt.Confirm.RO)
			ReadOnlyDelMode=1;

		if (ReadOnlyDelMode!=-1)
			MsgCode=ReadOnlyDelMode;
		else
			MsgCode=Message(MSG_WARNING,5,MSG(MWarning),
			                MSG(MCopyFileRO),Name,MSG(MCopyAskDelete),
			                MSG(MCopyDeleteRO),MSG(MCopyDeleteAllRO),
			                MSG(MCopySkipRO),MSG(MCopySkipAllRO),MSG(MCopyCancelRO));

		switch (MsgCode)
		{
			case 1:
				ReadOnlyDelMode=1;
				break;
			case 2:
				return(COPY_NEXT);
			case 3:
				ReadOnlyDelMode=3;
				return(COPY_NEXT);
			case -1:
			case -2:
			case 4:
				return(COPY_CANCEL);
		}

		//apiSetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
	}
	
	TemporaryMakeWritable tmw(Name);

	while ((Attr&FILE_ATTRIBUTE_DIRECTORY)?!apiRemoveDirectory(Name):!apiDeleteFile(Name))
	{
		int MsgCode;

		if (SkipDeleteMode!=-1)
			MsgCode=SkipDeleteMode;
		else
			MsgCode=Message(MSG_WARNING|MSG_ERRORTYPE,4,MSG(MError),MSG(MCannotDeleteFile),Name,
			                MSG(MDeleteRetry),MSG(MDeleteSkip),MSG(MDeleteSkipAll),MSG(MDeleteCancel));

		switch (MsgCode)
		{
			case 1:
				return COPY_NEXT;
			case 2:
				SkipDeleteMode=1;
				return COPY_NEXT;
			case -1:
			case -2:
			case 3:
				return(COPY_CANCEL);
		}
	}

	return(COPY_SUCCESS);
}



int ShellCopy::ShellCopyFile(const wchar_t *SrcName,const FAR_FIND_DATA_EX &SrcData,
                             FARString &strDestName,DWORD &DestAttr,int Append)
{
	OrigScrX=ScrX;
	OrigScrY=ScrY;

	if ((Flags&FCOPY_LINK))
	{
		if (RPT==RP_HARDLINK)
		{
			apiDeleteFile(strDestName); //BUGBUG
			return(MkHardLink(SrcName,strDestName) ? COPY_SUCCESS:COPY_FAILURE);
		}
		else
		{
			return(MkSymLink(SrcName,strDestName,RPT,0) ? COPY_SUCCESS:COPY_FAILURE);
		}
	}

	File SrcFile;
	bool Opened = SrcFile.Open(SrcName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);

	if (!Opened)
	{
		_localLastError=WINPORT(GetLastError)();
		return COPY_FAILURE;
	}

	std::unique_ptr<FileExtendedAttributesCopy> XAttrCopyPtr;
	if (((Flags & FCOPY_COPYXATTR) != 0))
		XAttrCopyPtr.reset(new FileExtendedAttributesCopy(SrcFile));


	File DestFile;
	int64_t AppendPos=0;

	//bool CopySparse=false;
	DWORD DstFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	if (!(Flags&FCOPY_COPYTONUL))
	{
		if ((Flags & FCOPY_WRITETHROUGH) != 0) {
			DstFlags|= FILE_FLAG_WRITE_THROUGH;

#ifdef __linux__ //anyway OSX doesn't have O_DIRECT
			if (SrcData.nFileSize > 128 * USE_PAGE_SIZE)// just empiric
				DstFlags|= FILE_FLAG_NO_BUFFERING;
#endif
		}
		
		bool DstOpened = DestFile.Open(strDestName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, (Append ? OPEN_EXISTING:CREATE_ALWAYS), DstFlags);
		if ((DstFlags & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING)) != 0) {
			if (!DstOpened) {
				DstFlags&= ~(FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING);
				DstOpened = DestFile.Open(strDestName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, (Append ? OPEN_EXISTING:CREATE_ALWAYS), DstFlags);
				if (DstOpened) {
					Flags&= ~FCOPY_WRITETHROUGH; 
					fprintf(stderr, "COPY: unbuffered FAILED: 0x%x\n", DstFlags & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING));
				}
			}/* else
				fprintf(stderr, "COPY: unbuffered OK: 0x%x\n", DstFlags & (FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING));*/
		}

		if (!DstOpened)
		{
			_localLastError=WINPORT(GetLastError)();
			SrcFile.Close();
			_LOGCOPYR(SysLog(L"return COPY_FAILURE -> %d CreateFile=-1, LastError=%d (0x%08X)",__LINE__,_localLastError,_localLastError));
			return COPY_FAILURE;
		}
		if (XAttrCopyPtr)
			XAttrCopyPtr->ApplyToCopied(DestFile);

		if (((Flags & FCOPY_COPYACCESSMODE) != 0))
			DestFile.Chmod(SrcData.dwUnixMode);

		FARString strDriveRoot;
		GetPathRoot(strDestName,strDriveRoot);

		/*if (SrcData.dwFileAttributes&FILE_ATTRIBUTE_SPARSE_FILE)
		{
			DWORD VolFlags=0;
			if(apiGetVolumeInformation(strDriveRoot,nullptr,nullptr,nullptr,&VolFlags,nullptr))
			{
				if(VolFlags&FILE_SUPPORTS_SPARSE_FILES)
				{
					DWORD Temp;
					if (DestFile.IoControl(FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &Temp))
					{
						CopySparse=true;
					}
				}
			}
		}*/

		if (Append)
		{
			if (!DestFile.SetPointer(0,&AppendPos,FILE_END))
			{
				_localLastError=WINPORT(GetLastError)();
				SrcFile.Close();
				DestFile.SetEnd();
				DestFile.Close();
				return COPY_FAILURE;
			}
		}

		// если места в приёмнике хватает - займём сразу.
		UINT64 FreeBytes=0;
		if (apiGetDiskSize(strDriveRoot,nullptr,nullptr,&FreeBytes))
		{
			if (FreeBytes>SrcData.nFileSize)
			{
				INT64 CurPtr=0;

				if (DestFile.GetPointer(CurPtr) &&
				        DestFile.SetPointer(SrcData.nFileSize,nullptr,FILE_CURRENT) &&
				        DestFile.SetEnd())
					DestFile.SetPointer(CurPtr,nullptr,FILE_BEGIN);
			}
		}
	}

	int   AbortOp = FALSE;
	BOOL SparseQueryResult=TRUE;
	//FILE_ALLOCATED_RANGE_BUFFER queryrange;
	//FILE_ALLOCATED_RANGE_BUFFER ranges[1024];
	//queryrange.FileOffset.QuadPart = 0;
	//queryrange.Length.QuadPart = SrcData.nFileSize;
	CP->SetProgressValue(0,0);

	do
	{
		/*DWORD n=0,nbytes=0;
		
		if (CopySparse)
		{
			SparseQueryResult=SrcFile.IoControl(FSCTL_QUERY_ALLOCATED_RANGES, &queryrange, sizeof(queryrange), ranges, sizeof(ranges), &nbytes);

			if (!SparseQueryResult && WINPORT(GetLastError)()!=ERROR_MORE_DATA)
				break;

			n=nbytes/sizeof(FILE_ALLOCATED_RANGE_BUFFER);
		}*/

		//for (DWORD i=0; i<(CopySparse?n:i+1); i++)
		for (DWORD i=0; i<(i+1); i++)
		{
//			INT64 Size=0;

			/*if (CopySparse)
			{
				Size=ranges[i].Length.QuadPart;
				SrcFile.SetPointer(ranges[i].FileOffset.QuadPart,nullptr,FILE_BEGIN);
				INT64 DestPos=ranges[i].FileOffset.QuadPart;

				if (Append)
					DestPos+=AppendPos;

				DestFile.SetPointer(DestPos,nullptr,FILE_BEGIN);
			}*/

			DWORD BytesRead,BytesWritten;

			//while (CopySparse?(Size>0):true)
			while (true)
			{
				BOOL IsChangeConsole=OrigScrX != ScrX || OrigScrY != ScrY;

				if (CP->Cancelled())
				{
					AbortOp=true;
				}

				IsChangeConsole=CheckAndUpdateConsole(IsChangeConsole);

				if (IsChangeConsole)
				{
					OrigScrX=ScrX;
					OrigScrY=ScrY;
					PR_ShellCopyMsg();
				}

				if(GetProcessUptimeMSec()-ProgressUpdateTime >= ProgressUpdateThreshold) {

					CP->SetProgressValue(CurCopiedSize,SrcData.nFileSize);

					if (ShowTotalCopySize)
					{
						CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
					}
					ProgressUpdateTime=GetProcessUptimeMSec();
				}

				if (AbortOp)
				{
					SrcFile.Close();

					if (Append)
					{
						DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
					}

					DestFile.SetEnd();
					DestFile.Close();

					if (!Append)
					{
						TemporaryMakeWritable tmw(strDestName); //apiSetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
						apiDeleteFile(strDestName); //BUGBUG
					}

					return COPY_CANCEL;
				}

				//while (!SrcFile.Read(CopyBuffer,(CopySparse?(DWORD)Min((LONGLONG)CopyBufferSize,Size):CopyBufferSize),&BytesRead,nullptr))
				while (!SrcFile.Read(CopyBuffer, CopyBufferSize, &BytesRead,nullptr))
				{
					int MsgCode = Message(MSG_WARNING|MSG_ERRORTYPE,2,MSG(MError),
					                      MSG(MCopyReadError),SrcName,
					                      MSG(MRetry),MSG(MCancel));
					PR_ShellCopyMsg();

					if (!MsgCode)
						continue;

					DWORD LastError=WINPORT(GetLastError)();
					SrcFile.Close();

					if (!(Flags&FCOPY_COPYTONUL))
					{
						if (Append)
						{
							DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
						}

						DestFile.SetEnd();
						DestFile.Close();

						if (!Append)
						{
							TemporaryMakeWritable tmw(strDestName); //apiSetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
							apiDeleteFile(strDestName); //BUGBUG
						}
					}

					CP->SetProgressValue(0,0);
					WINPORT(SetLastError)(_localLastError=LastError);
					CurCopiedSize = 0; // Сбросить текущий прогресс
					return COPY_FAILURE;
				}

				if (!BytesRead)
				{
					SparseQueryResult=FALSE;
					break;
				}

				if (!(Flags&FCOPY_COPYTONUL))
				{
					DWORD WriteSize = BytesRead;
					if ((DstFlags & FILE_FLAG_NO_BUFFERING)!=0)
						WriteSize = AlignPageUp(WriteSize);
					BytesWritten = 0;
					while (!DestFile.Write(CopyBuffer, WriteSize, &BytesWritten, nullptr))
					{
						DWORD LastError=WINPORT(GetLastError)();
						int Split=FALSE,SplitCancelled=FALSE,SplitSkipped=FALSE;
/*TODO
						if ((LastError==ERROR_DISK_FULL || LastError==ERROR_HANDLE_DISK_FULL) &&
						        !strDestName.IsEmpty() && strDestName.At(0)==L'/')
						{
							FARString strDriveRoot;
							GetPathRoot(strDestName,strDriveRoot);
							UINT64 FreeSize=0;

							if (apiGetDiskSize(strDriveRoot,nullptr,nullptr,&FreeSize))
							{
								if (FreeSize<BytesRead &&
								        DestFile.Write(CopyBuffer,(DWORD)FreeSize,&BytesWritten,nullptr) &&
										SrcFile.SetPointer(FreeSize-BytesRead,nullptr,FILE_CURRENT))
								{
									DestFile.Close();
									SetMessageHelp(L"CopyFiles");
									int MsgCode=Message(MSG_WARNING,4,MSG(MError),
									                    MSG(MErrorInsufficientDiskSpace),strDestName,
									                    MSG(MSplit),MSG(MSkip),MSG(MRetry),MSG(MCancel));
									PR_ShellCopyMsg();

									if (MsgCode==2)
									{
										SrcFile.Close();

										if (!Append)
										{
											apiSetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
											apiDeleteFile(strDestName); //BUGBUG
										}

										return COPY_FAILURE;
									}

									if (!MsgCode)
									{
										Split=TRUE;

										for (;;)
										{
											if (apiGetDiskSize(strDriveRoot,nullptr,nullptr,&FreeSize))
												if (FreeSize<BytesRead)
												{
													int MsgCode2 = Message(MSG_WARNING,2,MSG(MWarning),
													                       MSG(MCopyErrorDiskFull),strDestName,
													                       MSG(MRetry),MSG(MCancel));
													PR_ShellCopyMsg();

													if (MsgCode2)
													{
														Split=FALSE;
														SplitCancelled=TRUE;
													}
													else
														continue;
												}

											break;
										}
									}

									if (MsgCode==1)
										SplitSkipped=TRUE;

									if (MsgCode==-1 || MsgCode==3)
										SplitCancelled=TRUE;
								}
							}
						}*/

						if (Split)
						{
							INT64 FilePtr;
							SrcFile.GetPointer(FilePtr);
							FAR_FIND_DATA_EX SplitData=SrcData;
							SplitData.nFileSize-=FilePtr;
							int RetCode;
							FARString strNewName;

							if (!AskOverwrite(SplitData,SrcName,strDestName,INVALID_FILE_ATTRIBUTES,FALSE,((Flags&FCOPY_MOVE)?TRUE:FALSE),((Flags&FCOPY_LINK)?0:1),Append,strNewName,RetCode))
							{
								SrcFile.Close();
								return(COPY_CANCEL);
							}

							if (RetCode==COPY_RETRY)
							{
								strDestName=strNewName;

								if (CutToSlash(strNewName) && apiGetFileAttributes(strNewName)==INVALID_FILE_ATTRIBUTES)
								{
									CreatePath(strNewName);
								}

								return COPY_RETRY;
							}

							FARString strDestDir = strDestName;

							if (CutToSlash(strDestDir,true))
								CreatePath(strDestDir);

							;

							if (!DestFile.Open(strDestName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, (Append ? OPEN_EXISTING:CREATE_ALWAYS), SrcData.dwFileAttributes|FILE_FLAG_SEQUENTIAL_SCAN) || (Append && !DestFile.SetPointer(0,nullptr,FILE_END)))
							{
								_localLastError=WINPORT(GetLastError)();
								SrcFile.Close();
								DestFile.Close();
								return COPY_FAILURE;
							}
							if (XAttrCopyPtr)
								XAttrCopyPtr->ApplyToCopied(DestFile);
							if (((Flags & FCOPY_COPYACCESSMODE) != 0))
								DestFile.Chmod(SrcData.dwUnixMode);
						}
						else
						{
							if (!SplitCancelled && !SplitSkipped &&
							        !Message(MSG_WARNING|MSG_ERRORTYPE,2,MSG(MError),
							                MSG(MCopyWriteError),strDestName,MSG(MRetry),MSG(MCancel)))
							{
								continue;
							}

							SrcFile.Close();

							if (Append)
							{
								DestFile.SetPointer(AppendPos,nullptr,FILE_BEGIN);
							}

							DestFile.SetEnd();
							DestFile.Close();

							if (!Append)
							{
								TemporaryMakeWritable tmw(strDestName); //apiSetFileAttributes(strDestName,FILE_ATTRIBUTE_NORMAL);
								apiDeleteFile(strDestName); //BUGBUG
							}

							CP->SetProgressValue(0,0);
							WINPORT(SetLastError)(_localLastError=LastError);

							if (SplitSkipped)
								return COPY_NEXT;

							return(SplitCancelled ? COPY_CANCEL:COPY_FAILURE);
						}

						break;
					}
					if (BytesWritten > BytesRead) { 
						//likely we written bit more due to no_buffering requires aligned io
						//move backward and correct file size
						DestFile.SetPointer((INT64)BytesRead - (INT64)WriteSize, nullptr, FILE_CURRENT);
						DestFile.SetEnd();
						BytesWritten = BytesRead;
					}
				}
				else
				{
					BytesWritten=BytesRead; // не забудем приравнять количество записанных байт
				}

				CurCopiedSize+=BytesWritten;

				if (ShowTotalCopySize)
					TotalCopiedSize+=BytesWritten;

				if(GetProcessUptimeMSec()-ProgressUpdateTime >= ProgressUpdateThreshold) {

					CP->SetProgressValue(CurCopiedSize,SrcData.nFileSize);

					if (ShowTotalCopySize)
					{
						CP->SetTotalProgressValue(TotalCopiedSize,TotalCopySize);
					}

					CP->SetNames(SrcData.strFileName,strDestName);

					ProgressUpdateTime=GetProcessUptimeMSec();
				}
				//if (CopySparse)
				//	Size -= BytesRead;
			}

			//if (!CopySparse || !SparseQueryResult)
				break;
		} /* for */

		if (!SparseQueryResult)
			break;

		/*if (CopySparse)
		{
			if (!SparseQueryResult && n>0)
			{
				queryrange.FileOffset.QuadPart=ranges[n-1].FileOffset.QuadPart+ranges[n-1].Length.QuadPart;
				queryrange.Length.QuadPart = SrcData.nFileSize-queryrange.FileOffset.QuadPart;
			}
		}*/
	}
	while (false);//!SparseQueryResult && CopySparse);

	if (!(Flags&FCOPY_COPYTONUL))
	{
		DestFile.SetTime(nullptr, nullptr, &SrcData.ftLastWriteTime, nullptr);
		SrcFile.Close();

		/*if (CopySparse)
		{
			INT64 Pos=SrcData.nFileSize;

			if (Append)
				Pos+=AppendPos;

			DestFile.SetPointer(Pos,nullptr,FILE_BEGIN);
			DestFile.SetEnd();
		}*/

		DestFile.Close();
	}
	else
		SrcFile.Close();

	return COPY_SUCCESS;
}

void ShellCopy::SetDestDizPath(const wchar_t *DestPath)
{
	if (!(Flags&FCOPY_DIZREAD))
	{
		ConvertNameToFull(DestPath, strDestDizPath);
		CutToSlash(strDestDizPath);

		if (strDestDizPath.IsEmpty())
			strDestDizPath = L".";

		if ((Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && !SrcPanel->IsDizDisplayed()) ||
		        Opt.Diz.UpdateMode==DIZ_NOT_UPDATE)
			strDestDizPath.Clear();

		if (!strDestDizPath.IsEmpty())
			DestDiz.Read(strDestDizPath);

		Flags|=FCOPY_DIZREAD;
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

#define DM_OPENVIEWER DM_USER+33

LONG_PTR WINAPI WarnDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2)
{
	switch (Msg)
	{
		case DM_OPENVIEWER:
		{
			LPCWSTR ViewName=nullptr;
			FARString** WFN=reinterpret_cast<FARString**>(SendDlgMessage(hDlg,DM_GETDLGDATA,0,0));

			if (WFN)
			{
				switch (Param1)
				{
					case WDLG_SRCFILEBTN:
						ViewName=*WFN[0];
						break;
					case WDLG_DSTFILEBTN:
						ViewName=*WFN[1];
						break;
				}

				FileViewer Viewer(ViewName,FALSE,FALSE,TRUE,-1,nullptr,nullptr,FALSE);
				Viewer.SetDynamicallyBorn(FALSE);
				// а этот трюк не даст пользователю сменить текущий каталог по CtrlF10 и этим ввести в заблуждение копир:
				Viewer.SetTempViewName(L"nul",FALSE);
				FrameManager->EnterModalEV();
				FrameManager->ExecuteModal();
				FrameManager->ExitModalEV();
				FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
			}
		}
		break;
		case DN_CTLCOLORDLGITEM:
		{
			if (Param1==WDLG_FILENAME)
			{
				int Color=FarColorToReal(COL_WARNDIALOGTEXT)&0xFF;
				return ((Param2&0xFF00FF00)|(Color<<16)|Color);
			}
		}
		break;
		case DN_BTNCLICK:
		{
			switch (Param1)
			{
				case WDLG_SRCFILEBTN:
				case WDLG_DSTFILEBTN:
					SendDlgMessage(hDlg,DM_OPENVIEWER,Param1,0);
					break;
				case WDLG_RENAME:
				{
					FARString** WFN=reinterpret_cast<FARString**>(SendDlgMessage(hDlg,DM_GETDLGDATA,0,0));
					FARString strDestName=*WFN[1];
					GenerateName(strDestName,*WFN[2]);

					if (SendDlgMessage(hDlg,DM_GETCHECK,WDLG_CHECKBOX,0)==BSTATE_UNCHECKED)
					{
						int All=BSTATE_UNCHECKED;

						if (GetString(MSG(MCopyRenameTitle),MSG(MCopyRenameText),nullptr,strDestName,*WFN[1],L"CopyAskOverwrite",FIB_BUTTONS|FIB_NOAMPERSAND|FIB_EXPANDENV|FIB_CHECKBOX,&All,MSG(MCopyRememberChoice)))
						{
							if (All!=BSTATE_UNCHECKED)
							{
								*WFN[2]=*WFN[1];
								CutToSlash(*WFN[2]);
							}

							SendDlgMessage(hDlg,DM_SETCHECK,WDLG_CHECKBOX,All);
						}
						else
						{
							return TRUE;
						}
					}
					else
					{
						*WFN[1]=strDestName;
					}
				}
				break;
			}
		}
		break;
		case DN_KEY:
		{
			if ((Param1==WDLG_SRCFILEBTN || Param1==WDLG_DSTFILEBTN) && Param2==KEY_F3)
			{
				SendDlgMessage(hDlg,DM_OPENVIEWER,Param1,0);
			}
		}
		break;
	}

	return DefDlgProc(hDlg,Msg,Param1,Param2);
}

int ShellCopy::AskOverwrite(const FAR_FIND_DATA_EX &SrcData,
                            const wchar_t *SrcName,
                            const wchar_t *DestName, DWORD DestAttr,
                            int SameName,int Rename,int AskAppend,
                            int &Append,FARString &strNewName,int &RetCode)
{
	enum
	{
		WARN_DLG_HEIGHT=13,
		WARN_DLG_WIDTH=72,
	};
	DialogDataEx WarnCopyDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,WARN_DLG_WIDTH-4,WARN_DLG_HEIGHT-2,{},0,MSG(MWarning)},
		{DI_TEXT,5,2,WARN_DLG_WIDTH-6,2,{},DIF_CENTERTEXT,MSG(MCopyFileExist)},
		{DI_EDIT,5,3,WARN_DLG_WIDTH-6,3,{},DIF_READONLY,(wchar_t*)DestName},
		{DI_TEXT,3,4,0,4,{},DIF_SEPARATOR,L""},
		{DI_BUTTON,5,5,WARN_DLG_WIDTH-6,5,{},DIF_BTNNOCLOSE|DIF_NOBRACKETS,L""},
		{DI_BUTTON,5,6,WARN_DLG_WIDTH-6,6,{},DIF_BTNNOCLOSE|DIF_NOBRACKETS,L""},
		{DI_TEXT,3,7,0,7,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX,5,8,0,8,{},DIF_FOCUS,MSG(MCopyRememberChoice)},
		{DI_TEXT,3,9,0,9,{},DIF_SEPARATOR,L""},

		{DI_BUTTON,0,10,0,10,{},DIF_DEFAULT|DIF_CENTERGROUP,MSG(MCopyOverwrite)},
		{DI_BUTTON,0,10,0,10,{},DIF_CENTERGROUP,MSG(MCopySkipOvr)},
		{DI_BUTTON,0,10,0,10,{},DIF_CENTERGROUP,MSG(MCopyRename)},
		{DI_BUTTON,0,10,0,10,{},DIF_CENTERGROUP|(AskAppend?0:(DIF_DISABLE|DIF_HIDDEN)),MSG(MCopyAppend)},
		{DI_BUTTON,0,10,0,10,{},DIF_CENTERGROUP,MSG(MCopyCancelOvr)}
	};
	FAR_FIND_DATA_EX DestData;
	DestData.Clear();
	int DestDataFilled=FALSE;
	Append=FALSE;

	if ((Flags&FCOPY_COPYTONUL))
	{
		RetCode=COPY_NEXT;
		return TRUE;
	}

	if (DestAttr==INVALID_FILE_ATTRIBUTES)
		if ((DestAttr=apiGetFileAttributes(DestName))==INVALID_FILE_ATTRIBUTES)
			return TRUE;

	if (DestAttr & FILE_ATTRIBUTE_DIRECTORY)
		return TRUE;

	int MsgCode=OvrMode;
	FARString strDestName=DestName;

	if (OvrMode==-1)
	{
		int Type;

		if ((!Opt.Confirm.Copy && !Rename) || (!Opt.Confirm.Move && Rename) ||
		        SameName || (Type=apiGetFileTypeByName(DestName))==FILE_TYPE_CHAR ||
		        Type==FILE_TYPE_PIPE || (Flags&FCOPY_OVERWRITENEXT))
			MsgCode=1;
		else
		{
			DestData.Clear();
			apiGetFindDataEx(DestName,DestData);
			DestDataFilled=TRUE;

			if ((Flags&FCOPY_ONLYNEWERFILES))
			{
				// сравним время
				int64_t RetCompare=FileTimeDifference(&DestData.ftLastWriteTime,&SrcData.ftLastWriteTime);

				if (RetCompare < 0)
					MsgCode=0;
				else
					MsgCode=2;
			}
			else
			{
				FormatString strSrcFileStr, strDestFileStr;
				uint64_t SrcSize = SrcData.nFileSize;
				FILETIME SrcLastWriteTime = SrcData.ftLastWriteTime;
				if(Flags&FCOPY_COPYSYMLINKCONTENTS && SrcData.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)
				{
					FARString RealName = SrcName;
					FAR_FIND_DATA_EX FindData;
					apiGetFindDataEx(RealName,FindData);
					SrcSize=FindData.nFileSize;
					SrcLastWriteTime = FindData.ftLastWriteTime;

				}
				FormatString strSrcSizeText;
				strSrcSizeText<<SrcSize;
				uint64_t DestSize = DestData.nFileSize;
				FormatString strDestSizeText;
				strDestSizeText<<DestSize;
				FARString strDateText, strTimeText;
				ConvertDate(SrcLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE,TRUE);
				strSrcFileStr<<fmt::LeftAlign()<<fmt::Width(17)<<MSG(MCopySource)<<L" "<<fmt::Width(25)<<fmt::Precision(25)<<strSrcSizeText<<L" "<<strDateText<<L" "<<strTimeText;
				ConvertDate(DestData.ftLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE,TRUE);
				strDestFileStr<<fmt::LeftAlign()<<fmt::Width(17)<<MSG(MCopyDest)<<L" "<<fmt::Width(25)<<fmt::Precision(25)<<strDestSizeText<<L" "<<strDateText<<L" "<<strTimeText;

				WarnCopyDlgData[WDLG_SRCFILEBTN].Data=strSrcFileStr;
				WarnCopyDlgData[WDLG_DSTFILEBTN].Data=strDestFileStr;
				MakeDialogItemsEx(WarnCopyDlgData,WarnCopyDlg);
				FARString strFullSrcName;
				ConvertNameToFull(SrcName,strFullSrcName);
				FARString *WFN[]={&strFullSrcName,&strDestName,&strRenamedFilesPath};
				Dialog WarnDlg(WarnCopyDlg,ARRAYSIZE(WarnCopyDlg),WarnDlgProc,(LONG_PTR)&WFN);
				WarnDlg.SetDialogMode(DMODE_WARNINGSTYLE);
				WarnDlg.SetPosition(-1,-1,WARN_DLG_WIDTH,WARN_DLG_HEIGHT);
				WarnDlg.SetHelp(L"CopyAskOverwrite");
				WarnDlg.SetId(CopyOverwriteId);
				WarnDlg.Process();

				switch (WarnDlg.GetExitCode())
				{
					case WDLG_OVERWRITE:
						MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?1:0;
						break;
					case WDLG_SKIP:
						MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?3:2;
						break;
					case WDLG_RENAME:
						MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?5:4;
						break;
					case WDLG_APPEND:
						MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?7:6;
						break;
					case -1:
					case -2:
					case WDLG_CANCEL:
						MsgCode=8;
						break;
				}
			}
		}
	}

	switch (MsgCode)
	{
		case 1:
			OvrMode=1;
		case 0:
			break;
		case 3:
			OvrMode=2;
		case 2:
			RetCode=COPY_NEXT;
			return FALSE;
		case 5:
			OvrMode=5;
			GenerateName(strDestName,strRenamedFilesPath);
		case 4:
			RetCode=COPY_RETRY;
			strNewName=strDestName;
			break;
		case 7:
			OvrMode=6;
		case 6:
			Append=TRUE;
			break;
		case -1:
		case -2:
		case 8:
			RetCode=COPY_CANCEL;
			return FALSE;
	}

	if (RetCode!=COPY_RETRY)
	{
		if ((DestAttr & FILE_ATTRIBUTE_READONLY) && !(Flags&FCOPY_OVERWRITENEXT))
		{
			int MsgCode=0;

			if (!SameName)
			{
				if (ReadOnlyOvrMode!=-1)
				{
					MsgCode=ReadOnlyOvrMode;
				}
				else
				{
					if (!DestDataFilled)
					{
						DestData.Clear();
						apiGetFindDataEx(DestName,DestData);
					}

					FARString strDateText,strTimeText;
					FormatString strSrcFileStr, strDestFileStr;
					uint64_t SrcSize = SrcData.nFileSize;
					FormatString strSrcSizeText;
					strSrcSizeText<<SrcSize;
					uint64_t DestSize = DestData.nFileSize;
					FormatString strDestSizeText;
					strDestSizeText<<DestSize;
					ConvertDate(SrcData.ftLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE,TRUE);
					strSrcFileStr<<fmt::LeftAlign()<<fmt::Width(17)<<MSG(MCopySource)<<L" "<<fmt::Width(25)<<fmt::Precision(25)<<strSrcSizeText<<L" "<<strDateText<<L" "<<strTimeText;
					ConvertDate(DestData.ftLastWriteTime,strDateText,strTimeText,8,FALSE,FALSE,TRUE,TRUE);
					strDestFileStr<<fmt::LeftAlign()<<fmt::Width(17)<<MSG(MCopyDest)<<L" "<<fmt::Width(25)<<fmt::Precision(25)<<strDestSizeText<<L" "<<strDateText<<L" "<<strTimeText;
					WarnCopyDlgData[WDLG_SRCFILEBTN].Data=strSrcFileStr;
					WarnCopyDlgData[WDLG_DSTFILEBTN].Data=strDestFileStr;
					WarnCopyDlgData[WDLG_TEXT].Data=MSG(MCopyFileRO);
					WarnCopyDlgData[WDLG_OVERWRITE].Data=MSG(Append?MCopyAppend:MCopyOverwrite);
					WarnCopyDlgData[WDLG_RENAME].Type=DI_TEXT;
					WarnCopyDlgData[WDLG_RENAME].Data=L"";
					WarnCopyDlgData[WDLG_APPEND].Type=DI_TEXT;
					WarnCopyDlgData[WDLG_APPEND].Data=L"";
					MakeDialogItemsEx(WarnCopyDlgData,WarnCopyDlg);
					FARString strSrcName;
					ConvertNameToFull(SrcData.strFileName,strSrcName);
					LPCWSTR WFN[2]={strSrcName,DestName};
					Dialog WarnDlg(WarnCopyDlg,ARRAYSIZE(WarnCopyDlg),WarnDlgProc,(LONG_PTR)&WFN);
					WarnDlg.SetDialogMode(DMODE_WARNINGSTYLE);
					WarnDlg.SetPosition(-1,-1,WARN_DLG_WIDTH,WARN_DLG_HEIGHT);
					WarnDlg.SetHelp(L"CopyFiles");
					WarnDlg.SetId(CopyReadOnlyId);
					WarnDlg.Process();

					switch (WarnDlg.GetExitCode())
					{
						case WDLG_OVERWRITE:
							MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?1:0;
							break;
						case WDLG_SKIP:
							MsgCode=WarnCopyDlg[WDLG_CHECKBOX].Selected?3:2;
							break;
						case -1:
						case -2:
						case WDLG_CANCEL:
							MsgCode=8;
							break;
					}
				}
			}

			switch (MsgCode)
			{
				case 1:
					ReadOnlyOvrMode=1;
				case 0:
					break;
				case 3:
					ReadOnlyOvrMode=2;
				case 2:
					RetCode=COPY_NEXT;
					return FALSE;
				case -1:
				case -2:
				case 8:
					RetCode=COPY_CANCEL;
					return FALSE;
			}
		}

		if (!SameName && (DestAttr & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)))
			apiMakeWritable(DestName); //apiSetFileAttributes(DestName,FILE_ATTRIBUTE_NORMAL);
	}

	return TRUE;
}




BOOL ShellCopySecuryMsg(const wchar_t *Name)
{
	static clock_t PrepareSecuryStartTime;

	if (!Name || !*Name || (static_cast<DWORD>(GetProcessUptimeMSec() - PrepareSecuryStartTime) > Opt.ShowTimeoutDACLFiles))
	{
		static int Width=30;
		int WidthTemp;
		if (Name && *Name)
		{
			PrepareSecuryStartTime = GetProcessUptimeMSec();     // Первый файл рисуется всегда
			WidthTemp=Max(StrLength(Name),30);
		}
		else
			Width=WidthTemp=30;

		// ширина месага - 38%
		WidthTemp=Min(WidthTemp,WidthNameForMessage);
		Width=Max(Width,WidthTemp);

		FARString strOutFileName = Name; //??? nullptr ???
		TruncPathStr(strOutFileName,Width);
		CenterStr(strOutFileName, strOutFileName,Width+4);
		Message(0,0,MSG(MMoveDlgTitle),MSG(MCopyPrepareSecury),strOutFileName);

		if (CP->Cancelled())
		{
			return FALSE;
		}
	}

	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	preRedrawItem.Param.Param1=Name;
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
	PreRedrawItem preRedrawItem=PreRedraw.Peek();
	preRedrawItem.Param.Param1=CP;
	PreRedraw.SetParam(preRedrawItem.Param);
	TotalCopySize=CurCopiedSize=0;
	TotalFilesToProcess = 0;
	SrcPanel->GetSelNameCompat(nullptr,FileAttr);

	while (SrcPanel->GetSelNameCompat(&strSelName,FileAttr,&fd))
	{
		if ((FileAttr&FILE_ATTRIBUTE_REPARSE_POINT) && !(Flags&FCOPY_COPYSYMLINKCONTENTS))
			continue;

		if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
		{
			{
				uint32_t DirCount,FileCount,ClusterSize;
				uint64_t CompressedSize,RealFileSize;
				CP->SetScanName(strSelName);
				int __Ret=GetDirInfo(L"",strSelName,DirCount,FileCount,FileSize,CompressedSize,
				                     RealFileSize,ClusterSize,-1,
				                     Filter,
				                     (Flags&FCOPY_COPYSYMLINKCONTENTS?GETDIRINFO_SCANSYMLINK:0)|
				                     (UseFilter?GETDIRINFO_USEFILTER:0));

				if (__Ret <= 0)
				{
					ShowTotalCopySize=false;
					PreRedraw.Pop();
					return FALSE;
				}

				if (FileCount > 0)
				{
					TotalCopySize+=FileSize;
					TotalFilesToProcess += FileCount;
				}
			}
		}
		else
		{
			//  Подсчитаем количество файлов
			if (UseFilter)
			{
				if (!Filter->FileInFilter(fd))
					continue;
			}

			FileSize = SrcPanel->GetLastSelectedSize();

			if (FileSize != (uint64_t)-1)
			{
				TotalCopySize+=FileSize;
				TotalFilesToProcess++;
			}
		}
	}

	// INFO: Это для варианта, когда "ВСЕГО = общий размер * количество целей"
	TotalCopySize=TotalCopySize*CountTarget;
	InsertCommas(TotalCopySize,strTotalCopySizeText);
	PreRedraw.Pop();
	return true;
}

/*
  Оболочка вокруг SetFileAttributes() для
  корректного выставления атрибутов
*/
bool ShellCopy::ShellSetAttr(const wchar_t *Dest,DWORD Attr)
{
	FARString strRoot;
	ConvertNameToFull(Dest,strRoot);
	GetPathRoot(strRoot,strRoot);

	if (apiGetFileAttributes(strRoot)==INVALID_FILE_ATTRIBUTES) // Неудача, когда сетевой путь, да еще и симлинк
	{
		// ... в этом случае проверим AS IS
		ConvertNameToFull(Dest,strRoot);
		GetPathRoot(strRoot,strRoot);

		if (apiGetFileAttributes(strRoot)==INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
	}


	if (!apiSetFileAttributes(Dest,Attr))
	{
		return FALSE;
	}

	return true;
}

void ShellCopy::CheckUpdatePanel() // выставляет флаг FCOPY_UPDATEPPANEL
{
}
