#define _FAR_NO_NAMELESS_UNIONS
#define _UNICODE
#include <farplug-wide.h>
#include <algorithm>
#include <ctype.h>
#include <utils.h>
#include <KeyFileHelper.h>

#ifndef UNICODE
#define _cFileName     cFileName
#define SelItems(n, m) SelectedItems[n].m
#else
#define _cFileName     lpwszFileName
#define SelItems(n, m) SelectedItems[n]->m
#endif

#ifndef UNICODE
#define GetCheck(i)      DialogItems[i].Param.Selected
#define GetDataPtr(i)    DialogItems[i].Data.Data
#define CheckDisabled(i) (DialogItems[i].Flags & DIF_DISABLE)
#else
#define GetCheck(i)      (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, i, 0)
#define GetDataPtr(i)    ((const TCHAR *)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, i, 0))
#define CheckDisabled(i) (!((int)Info.SendDlgMessage(hDlg, DM_ENABLE, i, -1)))
#endif

#define INI_LOCATION InMyConfig("plugins/compare/config.ini")
#define INI_SECTION  "Settings"

/****************************************************************************
 *
 ****************************************************************************/
enum CompareLng
{
	MNoLngStringDefined = -1,

	MOK,
	MCancel,

	MCompare,

	MCmpTitle,
	MProcessBox,
	MProcessSubfolders,
	MUseMaxScanDepth,
	MProcessSelected,
	MCompareBox,
	MCompareCaseFileNames,
	MCompareTime,
	MCompareLowPrecision,
	MCompareIgnoreTimeZone,
	MCompareSize,
	MCompareContents,
	MCompareContentsIgnore,
	MCompareIgnoreNewLines,
	MCompareIgnoreWhitespace,
	MMessageWhenNoDiff,

	MFilePanelsRequired,

	MComparing,
	MComparingWith,

	MComparingFiles,

	MNoDiffTitle,
	MNoDiffBody,

	MNoMemTitle,
	MNoMemBody,

	MEscTitle,
	MEscBody,

	MOpenErrorTitle,
	MOpenErrorBody,
};

/****************************************************************************
 *
 ****************************************************************************/
struct Options
{
	int ProcessSubfolders, UseMaxScanDepth, MaxScanDepth, ProcessSelected, ProcessHidden,
			CompareCaseFileNames, CompareTime,
			LowPrecisionTime, IgnorePossibleTimeZoneDifferences, CompareSize, CompareContents,
			CompareContentsIgnore, IgnoreWhitespace, IgnoreNewLines, MessageWhenNoDiff;
} Opt;

/****************************************************************************
 *
 ****************************************************************************/
static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;
static TCHAR *PluginRootKey = NULL;

static void WFD2FFD(WIN32_FIND_DATA &wfd, FAR_FIND_DATA &ffd)
{
	ffd.dwFileAttributes = wfd.dwFileAttributes;
	ffd.ftCreationTime = wfd.ftCreationTime;
	ffd.ftLastAccessTime = wfd.ftLastAccessTime;
	ffd.ftLastWriteTime = wfd.ftLastWriteTime;
	ffd.nFileSize = wfd.nFileSize;
	ffd.nPhysicalSize = wfd.nPhysicalSize;
#ifndef UNICODE
	lstrcpy(ffd.cFileName, wfd.cFileName);
#else
	ffd.lpwszFileName = wcsdup(wfd.cFileName);
#endif
}

#ifndef UNICODE
typedef PanelInfo OwnPanelInfo;
#else
struct OwnPanelInfo
{
	int PanelType;
	int Plugin;
	PluginPanelItem *PanelItems;
	int ItemsNumber;
	PluginPanelItem *SelectedItems;
	int SelectedItemsNumber;
	wchar_t *lpwszCurDir;
};
#endif

/****************************************************************************
 *
 ****************************************************************************/
static const TCHAR *GetMsg(int CompareLng)
{
	return Info.GetMsg(Info.ModuleNumber, CompareLng);
}

static int iTruncLen;

/****************************************************************************
 *
 *
 ****************************************************************************/
static void TrunCopy(TCHAR *cpDest, const TCHAR *cpSrc)
{
	int iLen = (int)lstrlen(FSF.TruncStr(lstrcpy(cpDest, cpSrc), iTruncLen));

	if (iLen < iTruncLen) {
		_tmemset(&cpDest[iLen], _T(' '), iTruncLen - iLen);
		cpDest[iTruncLen] = _T('\0');
	}
}

static bool bStart;
static bool bOpenFail;

/****************************************************************************
 *
 ****************************************************************************/
static void ShowMessage(const TCHAR *Name1, const TCHAR *Name2)
{
	static DWORD dwTicks;
	DWORD dwNewTicks = GetTickCount();
	if (dwNewTicks - dwTicks < 500)
		return;
	dwTicks = dwNewTicks;

	TCHAR TruncName1[MAX_PATH], TruncName2[MAX_PATH];
	TrunCopy(TruncName1, Name1);
	TrunCopy(TruncName2, Name2);

	const TCHAR *MsgItems[] = {GetMsg(MCmpTitle), GetMsg(MComparing), TruncName1, GetMsg(MComparingWith),
			TruncName2};
	Info.Message(Info.ModuleNumber, bStart ? FMSG_LEFTALIGN : FMSG_LEFTALIGN | FMSG_KEEPBACKGROUND, NULL,
			MsgItems, ARRAYSIZE(MsgItems), 0);
	bStart = false;
}

/****************************************************************************
 *
 ****************************************************************************/
LONG_PTR WINAPI ShowDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	static int CompareContents, CompareContentsIgnore, ProcessSubfolders, CompareTime;

	switch (Msg) {
		case DN_INITDIALOG:
			CompareContents = ((DWORD)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0)) & 0x000000FF;
			CompareContentsIgnore = CompareContents + 1;
			ProcessSubfolders = (((DWORD)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0)) >> 8) & 0x000000FF;
			CompareTime = ((DWORD)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0)) >> 16;
			break;

		case DN_BTNCLICK:
			if (Param1 == CompareTime || Param1 == ProcessSubfolders || Param1 == CompareContents
					|| Param1 == CompareContentsIgnore) {
				if (Param2) {
					Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 1, TRUE);
					if (!(Param1 == CompareContents
								&& !Info.SendDlgMessage(hDlg, DM_GETCHECK, CompareContentsIgnore, 0))) {
						Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 2, TRUE);
						if (Param1 == CompareContents)
							Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 3, TRUE);
					}
				} else {
					Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 1, FALSE);
					Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 2, FALSE);
					if (Param1 == CompareContents)
						Info.SendDlgMessage(hDlg, DM_ENABLE, Param1 + 3, FALSE);
				}
			}
			break;
	}

	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

/****************************************************************************
 *
 *
 *
 ****************************************************************************/
static bool ShowDialog(bool bPluginPanels, bool bSelectionPresent)
{
	static constexpr struct InitDialogItem
	{
		unsigned char Type;
		unsigned char X1, Y1, X2, Y2;
		int Data;
		int DefaultRegValue;
		const char *SelectedRegValue;
		unsigned int Flags;
		int *StoreTo;
	} InitItems[] = {
			/* 0*/ {DI_DOUBLEBOX, 3, 1, 62, 21, MCmpTitle, 0, NULL, 0, NULL},
			/* 1*/ {DI_TEXT, 5, 2, 0, 0, MProcessBox, 0, NULL, 0, NULL},
			/* 2*/
			{DI_CHECKBOX, 5, 3, 0, 0, MProcessSubfolders, 0, ("ProcessSubfolders"), 0,
					&Opt.ProcessSubfolders},
			/* 3*/
			{DI_CHECKBOX, 9, 4, 0, 0, MUseMaxScanDepth, 0, ("UseMaxScanDepth"), 0, &Opt.UseMaxScanDepth},
			/* 4*/
			{DI_FIXEDIT, 0, 4, 4, 0, MNoLngStringDefined, 99, ("MaxScanDepth"), DIF_MASKEDIT,
					&Opt.MaxScanDepth},
			/* 5*/
			{DI_CHECKBOX, 5, 5, 0, 0, MProcessSelected, 0, ("ProcessSelected"), 0, &Opt.ProcessSelected},
			/* 6*/ {DI_TEXT, 0, 6, 0, 0, MNoLngStringDefined, 0, NULL, DIF_SEPARATOR, NULL},
			/* 7*/ {DI_TEXT, 5, 7, 0, 0, MCompareBox, 0, NULL, 0, NULL},
			/* 8*/ {DI_CHECKBOX, 5, 8, 0, 0, MCompareCaseFileNames, 1, ("CompareCaseFileNames"), 0, &Opt.CompareCaseFileNames},
			/* 9*/ {DI_CHECKBOX, 5, 9, 0, 0, MCompareTime, 1, ("CompareTime"), 0, &Opt.CompareTime},
			/*10*/
			{DI_CHECKBOX, 9, 10, 0, 0, MCompareLowPrecision, 1, ("LowPrecisionTime"), 0,
					&Opt.LowPrecisionTime},
			/*11*/
			{DI_CHECKBOX, 9, 11, 0, 0, MCompareIgnoreTimeZone, 1, ("IgnorePossibleTimeZoneDifferences"), 0,
					&Opt.IgnorePossibleTimeZoneDifferences},
			/*12*/ {DI_CHECKBOX, 5, 12, 0, 0, MCompareSize, 1, ("CompareSize"), 0, &Opt.CompareSize},
			/*13*/
			{DI_CHECKBOX, 5, 13, 0, 0, MCompareContents, 0, ("CompareContents"), 0, &Opt.CompareContents},
			/*14*/
			{DI_CHECKBOX, 9, 14, 0, 0, MCompareContentsIgnore, 0, ("CompareContentsIgnore"), 0,
					&Opt.CompareContentsIgnore},
			/*15*/
			{DI_RADIOBUTTON, 13, 15, 0, 0, MCompareIgnoreNewLines, 1, ("IgnoreNewLines"), DIF_GROUP,
					&Opt.IgnoreNewLines},
			/*16*/
			{DI_RADIOBUTTON, 13, 16, 0, 0, MCompareIgnoreWhitespace, 0, ("IgnoreWhitespace"), 0,
					&Opt.IgnoreWhitespace},
			/*17*/ {DI_TEXT, 0, 17, 0, 0, MNoLngStringDefined, 0, NULL, DIF_SEPARATOR, NULL},
			/*18*/
			{DI_CHECKBOX, 5, 18, 0, 0, MMessageWhenNoDiff, 0, ("MessageWhenNoDiff"), 0,
					&Opt.MessageWhenNoDiff},
			/*19*/ {DI_TEXT, 0, 19, 0, 0, MNoLngStringDefined, 0, NULL, DIF_SEPARATOR, NULL},
			/*20*/ {DI_BUTTON, 0, 20, 0, 0, MOK, 0, NULL, DIF_CENTERGROUP, NULL},
			/*21*/ {DI_BUTTON, 0, 20, 0, 0, MCancel, 0, NULL, DIF_CENTERGROUP, NULL}};
	struct FarDialogItem DialogItems[ARRAYSIZE(InitItems)];
	TCHAR Mask[] = _T("99999");
#ifdef UNICODE
	wchar_t tmpnum[ARRAYSIZE(InitItems)][32];
#endif

	memset(DialogItems, 0, sizeof(DialogItems));
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);

	size_t DlgData = 0;

	for (size_t i = 0; i < ARRAYSIZE(InitItems); i++) {
		DialogItems[i].Type = InitItems[i].Type;
		DialogItems[i].X1 = InitItems[i].X1;
		DialogItems[i].Y1 = InitItems[i].Y1;
		DialogItems[i].X2 = InitItems[i].X2;
		DialogItems[i].Y2 = InitItems[i].Y2;
		DialogItems[i].Focus = FALSE;
		DialogItems[i].Flags = InitItems[i].Flags;
#ifndef UNICODE
		lstrcpy(DialogItems[i].Data.Data,
				(InitItems[i].Data == MNoLngStringDefined) ? "" : GetMsg(InitItems[i].Data));
#else
		DialogItems[i].PtrData = (InitItems[i].Data == MNoLngStringDefined) ? L"" : GetMsg(InitItems[i].Data);
#endif

		const DWORD dwRegValue = InitItems[i].SelectedRegValue
				? kfh.GetInt(InitItems[i].SelectedRegValue, InitItems[i].DefaultRegValue)
				: InitItems[i].DefaultRegValue;

		if (DialogItems[i].Type == DI_CHECKBOX || DialogItems[i].Type == DI_RADIOBUTTON) {
			DialogItems[i].Param.Selected = dwRegValue;
		} else if (DialogItems[i].Type == DI_FIXEDIT) {
#ifndef UNICODE
#define PtrData Data.Data
#else
			DialogItems[i].PtrData = tmpnum[i];
#endif
			FSF.itoa(dwRegValue, (TCHAR *)DialogItems[i].PtrData, 10);
			DialogItems[i].Param.Mask = Mask;
			DialogItems[i].X1 = DialogItems[i - 1].X1 + lstrlen(DialogItems[i - 1].PtrData)
					- (_tcschr(DialogItems[i - 1].PtrData, _T('&')) ? 1 : 0) + 5;
			DialogItems[i].X2+= DialogItems[i].X1;
		}
	}

	bool bNoFocus = true;
	for (size_t i = 0; i < ARRAYSIZE(InitItems); i++) {
		switch (InitItems[i].Data) {
			case MCompareContents:
				ASSERT(i + 3 < ARRAYSIZE(InitItems));
				DlgData+= i;
				if (bPluginPanels) {
					DialogItems[i].Flags|= DIF_DISABLE;
					DialogItems[i].Param.Selected = 0;
				}
				if (!DialogItems[i].Param.Selected) {
					DialogItems[i + 1].Flags|= DIF_DISABLE;
					DialogItems[i + 2].Flags|= DIF_DISABLE;
					DialogItems[i + 3].Flags|= DIF_DISABLE;
				} else {
					DialogItems[i + 1].Flags&= ~DIF_DISABLE;
					DialogItems[i + 2].Flags&= ~DIF_DISABLE;
					DialogItems[i + 3].Flags&= ~DIF_DISABLE;
				}
				break;
			case MCompareContentsIgnore:
				ASSERT(i + 2 < ARRAYSIZE(InitItems));
				if (!DialogItems[i].Param.Selected || DialogItems[i].Flags & DIF_DISABLE) {
					DialogItems[i + 1].Flags|= DIF_DISABLE;
					DialogItems[i + 2].Flags|= DIF_DISABLE;
				} else {
					DialogItems[i + 1].Flags&= ~DIF_DISABLE;
					DialogItems[i + 2].Flags&= ~DIF_DISABLE;
				}
				break;
			case MCompareIgnoreWhitespace:
				ASSERT(i > 0);
				if (DialogItems[i].Param.Selected == DialogItems[i - 1].Param.Selected) {
					DialogItems[i - 1].Param.Selected = 1;
					DialogItems[i].Param.Selected = 0;
				}
				break;
			case MProcessSubfolders:
				ASSERT(i + 2 < ARRAYSIZE(InitItems));
				DlgData+= i << 8;
				if (bPluginPanels) {
					DialogItems[i].Flags|= DIF_DISABLE;
					DialogItems[i].Param.Selected = 0;
				}
				if (!DialogItems[i].Param.Selected) {
					DialogItems[i + 1].Flags|= DIF_DISABLE;
					DialogItems[i + 2].Flags|= DIF_DISABLE;
				} else {
					DialogItems[i + 1].Flags&= ~DIF_DISABLE;
					DialogItems[i + 2].Flags&= ~DIF_DISABLE;
				}
				break;
			case MProcessSelected:
				if (!bSelectionPresent) {
					DialogItems[i].Flags|= DIF_DISABLE;
					DialogItems[i].Param.Selected = 0;
				}
				break;
			case MCompareTime:
				ASSERT(i + 2 < ARRAYSIZE(InitItems));
				DlgData+= i << 16;
				if (!DialogItems[i].Param.Selected) {
					DialogItems[i + 1].Flags|= DIF_DISABLE;
					DialogItems[i + 2].Flags|= DIF_DISABLE;
				} else {
					DialogItems[i + 1].Flags&= ~DIF_DISABLE;
					DialogItems[i + 2].Flags&= ~DIF_DISABLE;
				}
				break;
			case MOK:
				DialogItems[i].DefaultButton = 1;
				break;
		}

		if (bNoFocus && DialogItems[i].Type == DI_CHECKBOX && !(DialogItems[i].Flags & DIF_DISABLE)) {
			DialogItems[i].Focus = TRUE;
			bNoFocus = false;
		}
	}

#ifndef UNICODE
	int ExitCode = Info.DialogEx(Info.ModuleNumber, -1, -1, 66, 23, _T("Contents"), DialogItems,
			ARRAYSIZE(DialogItems), 0, 0, ShowDialogProc, DlgData);
#else
	HANDLE hDlg = Info.DialogInit(Info.ModuleNumber, -1, -1, 66, 23, _T("Contents"), DialogItems,
			ARRAYSIZE(DialogItems), 0, 0, ShowDialogProc, DlgData);
	if (hDlg == INVALID_HANDLE_VALUE)
		return false;

	int ExitCode = Info.DialogRun(hDlg);
#endif

	if (ExitCode == (ARRAYSIZE(InitItems) - 2)) {
		for (size_t i = 0; i < ARRAYSIZE(InitItems); i++)
			if (InitItems[i].StoreTo) {
				if (InitItems[i].Type == DI_CHECKBOX || InitItems[i].Type == DI_RADIOBUTTON)
					*InitItems[i].StoreTo = (DWORD)GetCheck((int)i);
				else if (InitItems[i].Type == DI_FIXEDIT)
					*InitItems[i].StoreTo = FSF.atoi(GetDataPtr((int)i));
			}
#ifndef UNICODE
#undef PtrData
#endif

		KeyFileHelper kfh(INI_LOCATION);
		for (size_t i = 0; i < ARRAYSIZE(InitItems); i++)
			if (!(CheckDisabled((int)i)) && InitItems[i].SelectedRegValue) {
				kfh.SetInt(INI_SECTION, InitItems[i].SelectedRegValue, *InitItems[i].StoreTo);
			}
		kfh.Save();

		if (bPluginPanels) {
			Opt.ProcessSubfolders = FALSE;
			Opt.CompareContents = FALSE;
		}
		Opt.ProcessHidden =
				(Info.AdvControl(Info.ModuleNumber, ACTL_GETPANELSETTINGS, NULL, NULL)
						& FPS_SHOWHIDDENANDSYSTEMFILES)
				!= 0;

#ifdef UNICODE
		Info.DialogFree(hDlg);
#endif

		return true;
	}

#ifdef UNICODE
	Info.DialogFree(hDlg);
#endif

	return false;
}

static bool bBrokenByEsc;
static HANDLE hConInp = INVALID_HANDLE_VALUE;

/****************************************************************************
 *
 ****************************************************************************/
static bool CheckForEsc(void)
{
	if (hConInp == INVALID_HANDLE_VALUE)
		return false;

	static DWORD dwTicks;
	DWORD dwNewTicks = GetTickCount();
	if (dwNewTicks - dwTicks < 500)
		return false;
	dwTicks = dwNewTicks;

	WORD EscCode = VK_ESCAPE;
	if (!WINPORT(CheckForKeyPress)(NULL, &EscCode, 1, CFKP_KEEP_OTHER_EVENTS))
		return false;

	if (Info.AdvControl(Info.ModuleNumber, ACTL_GETCONFIRMATIONS, NULL, NULL) & FCS_INTERRUPTOPERATION) {
		const TCHAR *MsgItems[] = {GetMsg(MEscTitle), GetMsg(MEscBody), GetMsg(MOK), GetMsg(MCancel)};
		if (Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 2) != 0)
			return false;
	}

	bBrokenByEsc = true;
	return true;
}

/****************************************************************************
 *
 ****************************************************************************/
static TCHAR *BuildFullFilename(const TCHAR *cpDir, const TCHAR *cpFileName)
{
	static TCHAR cName[MAX_PATH];
	FSF.AddEndSlash(lstrcpy(cName, cpDir));

	return lstrcat(cName, cpFileName);
}

struct FileIndex
{
	PluginPanelItem **ppi{nullptr};
	int iCount{0};
};

/****************************************************************************
 *
 *
 ****************************************************************************/
static int __cdecl PICompare(const void *el1, const void *el2)
{
	const PluginPanelItem *ppi1 = *(const PluginPanelItem **)el1, *ppi2 = *(const PluginPanelItem **)el2;

	if (ppi1->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (!(ppi2->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return -1;
	} else {
		if (ppi2->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			return 1;
	}

	if (Opt.CompareCaseFileNames)
		return -FSF.LStrcmp(ppi1->FindData._cFileName, ppi2->FindData._cFileName);
	else
		return -FSF.LStricmp(ppi1->FindData._cFileName, ppi2->FindData._cFileName);
}

/****************************************************************************
 *
 ****************************************************************************/
static bool BuildPanelIndex(const OwnPanelInfo *pInfo, struct FileIndex *pIndex
#ifdef UNICODE
		,
		HANDLE Filter
#endif
)
{
	bool bProcessSelected;
	pIndex->ppi = NULL;
	pIndex->iCount = (bProcessSelected = (Opt.ProcessSelected && pInfo->SelectedItemsNumber
								&& (pInfo->SelectedItems[0].Flags & PPIF_SELECTED)))
			? pInfo->SelectedItemsNumber
			: pInfo->ItemsNumber;
	if (!pIndex->iCount)
		return true;
	if (!(pIndex->ppi = (PluginPanelItem **)malloc(pIndex->iCount * sizeof(pIndex->ppi[0]))))
		return false;
	int j = 0;
	for (int i = pInfo->ItemsNumber - 1; i >= 0 && j < pIndex->iCount; i--)
		if ((Opt.ProcessSubfolders
					|| !(pInfo->PanelItems[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				&& (!bProcessSelected || (pInfo->PanelItems[i].Flags & PPIF_SELECTED))
				&& lstrcmp(pInfo->PanelItems[i].FindData._cFileName, _T(".."))
				&& lstrcmp(pInfo->PanelItems[i].FindData._cFileName, _T("."))) {
#ifdef UNICODE
			if (!Info.FileFilterControl(Filter, FFCTL_ISFILEINFILTER, 0,
						(LONG_PTR)&pInfo->PanelItems[i].FindData))
				continue;
#endif
			pIndex->ppi[j++] = &pInfo->PanelItems[i];
		}
	if ((pIndex->iCount = j) != 0)
		FSF.qsort(pIndex->ppi, j, sizeof(pIndex->ppi[0]), PICompare);
	else {
		free(pIndex->ppi);
		pIndex->ppi = NULL;
		pIndex->iCount = 0;
	}

	return true;
}

/****************************************************************************
 *
 ****************************************************************************/
static void FreePanelIndex(struct FileIndex *pIndex)
{
	if (pIndex->ppi)
		free(pIndex->ppi);
	pIndex->ppi = NULL;
	pIndex->iCount = 0;
}

/****************************************************************************
 *
 *
 ****************************************************************************/
static int GetDirList(OwnPanelInfo *PInfo, const TCHAR *Dir)
{
	TCHAR cPathMask[MAX_PATH];
	WIN32_FIND_DATA wfdFindData;
	HANDLE hFind;
	struct PluginPanelItem **pPanelItem = &PInfo->PanelItems;
	int *pItemsNumber = &PInfo->ItemsNumber;
	{
		size_t dirLen = lstrlen(Dir);
		if (dirLen > ARRAYSIZE(cPathMask) - sizeof("/*")
#ifndef UNICODE
				|| dirLen >= MAX_PATH
#endif
		)
			return FALSE;
	}
#ifndef UNICODE
	lstrcpy(PInfo->CurDir, Dir);
#else
	PInfo->lpwszCurDir = wcsdup(Dir);
#endif
	*pPanelItem = NULL;
	*pItemsNumber = 0;

	if ((hFind = FindFirstFile(lstrcat(lstrcpy(cPathMask, Dir), _T("/*")), &wfdFindData))
			== INVALID_HANDLE_VALUE)
		return TRUE;

	int iRet = TRUE;
	do {
		if (!lstrcmp(wfdFindData.cFileName, _T(".")) || !lstrcmp(wfdFindData.cFileName, _T("..")))
			continue;
		if ((wfdFindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !Opt.ProcessHidden)
			continue;
		struct PluginPanelItem *pPPI;
		if (!(pPPI = (struct PluginPanelItem *)realloc(*pPanelItem, (*pItemsNumber + 1) * sizeof(*pPPI)))) {
			iRet = FALSE;
			break;
		}
		*pPanelItem = pPPI;
		WFD2FFD(wfdFindData, (*pPanelItem)[(*pItemsNumber)++].FindData);
	} while (FindNextFile(hFind, &wfdFindData));
	FindClose(hFind);
	return iRet;
}

/****************************************************************************
 *
 ****************************************************************************/
static void FreeDirList(OwnPanelInfo *AInfo)
{
	if (AInfo->PanelItems) {
#ifdef UNICODE
		for (int i = 0; i < AInfo->ItemsNumber; i++) {
			free((void *)AInfo->PanelItems[i].FindData.lpwszFileName);
		}
		free(AInfo->lpwszCurDir);
#endif
		delete[] AInfo->PanelItems;
	}
}

static bool
CompareDirs(const OwnPanelInfo *AInfo, const OwnPanelInfo *PInfo, bool bCompareAll, int ScanDepth);
static DWORD bufSize;

#ifdef UNICODE
static HANDLE AFilter, PFilter;
#endif

// TODO:
//
static char *ABuf, *PBuf;

bool isnewline(int c)
{
	return (c == '\r' || c == '\n');
}

/****************************************************************************
 *
 *
 *
 ****************************************************************************/
static bool CompareFiles(const FAR_FIND_DATA *AData, const FAR_FIND_DATA *PData, const TCHAR *ACurDir,
		const TCHAR *PCurDir, int ScanDepth)
{
	if (AData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		//
		if (Opt.ProcessSubfolders) {
			if (Opt.UseMaxScanDepth && Opt.MaxScanDepth < ScanDepth + 1)
				return true;
			//
			OwnPanelInfo AInfo, PInfo;
			memset(&AInfo, 0, sizeof(AInfo));
			memset(&PInfo, 0, sizeof(PInfo));
			bool bEqual;
			if (!GetDirList(&AInfo, BuildFullFilename(ACurDir, AData->_cFileName))
					|| !GetDirList(&PInfo, BuildFullFilename(PCurDir, PData->_cFileName))) {
				bBrokenByEsc = true;	//
				bEqual = false;			//
			} else
				bEqual = CompareDirs(&AInfo, &PInfo, false, ScanDepth + 1);
			FreeDirList(&AInfo);
			FreeDirList(&PInfo);
			return bEqual;
		}
	} else {
		//
		if (Opt.CompareSize && AData->nFileSize != PData->nFileSize)
			return false;

		if (Opt.CompareTime) {
			if (Opt.LowPrecisionTime || Opt.IgnorePossibleTimeZoneDifferences) {
				union
				{
					int64_t num;
					struct
					{
						DWORD lo;
						DWORD hi;
					} hilo;
				} Precision, Difference, TimeDelta, temp;

				Precision.hilo.hi = 0;
				Precision.hilo.lo = Opt.LowPrecisionTime ? 20000000 : 0;	// 2s or 0s
				Difference.num = int64_t(9000000000);						// 15m

				if (AData->ftLastWriteTime.dwHighDateTime > PData->ftLastWriteTime.dwHighDateTime) {
					TimeDelta.hilo.hi =
							AData->ftLastWriteTime.dwHighDateTime - PData->ftLastWriteTime.dwHighDateTime;
					TimeDelta.hilo.lo =
							AData->ftLastWriteTime.dwLowDateTime - PData->ftLastWriteTime.dwLowDateTime;
					if (TimeDelta.hilo.lo > AData->ftLastWriteTime.dwLowDateTime)
						--TimeDelta.hilo.hi;
				} else {
					if (AData->ftLastWriteTime.dwHighDateTime == PData->ftLastWriteTime.dwHighDateTime) {
						TimeDelta.hilo.hi = 0;
						TimeDelta.hilo.lo =
								std::max(PData->ftLastWriteTime.dwLowDateTime,
										AData->ftLastWriteTime.dwLowDateTime)
								- std::min(PData->ftLastWriteTime.dwLowDateTime,
										AData->ftLastWriteTime.dwLowDateTime);
					} else {
						TimeDelta.hilo.hi =
								PData->ftLastWriteTime.dwHighDateTime - AData->ftLastWriteTime.dwHighDateTime;
						TimeDelta.hilo.lo =
								PData->ftLastWriteTime.dwLowDateTime - AData->ftLastWriteTime.dwLowDateTime;
						if (TimeDelta.hilo.lo > PData->ftLastWriteTime.dwLowDateTime)
							--TimeDelta.hilo.hi;
					}
				}

				//
				if (Opt.IgnorePossibleTimeZoneDifferences) {
					int counter = 0;
					while (TimeDelta.hilo.hi > Difference.hilo.hi && counter <= 26 * 4) {
						temp.hilo.lo = TimeDelta.hilo.lo - Difference.hilo.lo;
						temp.hilo.hi = TimeDelta.hilo.hi - Difference.hilo.hi;
						if (temp.hilo.lo > TimeDelta.hilo.lo)
							--temp.hilo.hi;
						TimeDelta.hilo.lo = temp.hilo.lo;
						TimeDelta.hilo.hi = temp.hilo.hi;
						++counter;
					}
					if (counter <= 26 * 4 && TimeDelta.hilo.hi == Difference.hilo.hi) {
						TimeDelta.hilo.hi = 0;
						TimeDelta.hilo.lo = std::max(TimeDelta.hilo.lo, Difference.hilo.lo)
								- std::min(TimeDelta.hilo.lo, Difference.hilo.lo);
					}
				}

				if (Precision.hilo.hi < TimeDelta.hilo.hi
						|| (Precision.hilo.hi == TimeDelta.hilo.hi && Precision.hilo.lo < TimeDelta.hilo.lo))
					return false;
			} else if (AData->ftLastWriteTime.dwLowDateTime != PData->ftLastWriteTime.dwLowDateTime
					|| AData->ftLastWriteTime.dwHighDateTime != PData->ftLastWriteTime.dwHighDateTime)
				return false;
		}
		if (Opt.CompareContents) {
			HANDLE hFileA, hFileP;
			TCHAR cpFileA[MAX_PATH], cpFileP[MAX_PATH];
			ShowMessage(lstrcpy(cpFileA, BuildFullFilename(ACurDir, AData->_cFileName)),
					lstrcpy(cpFileP, BuildFullFilename(PCurDir, PData->_cFileName)));
			if ((hFileA = CreateFile(cpFileA, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							FILE_FLAG_SEQUENTIAL_SCAN, NULL))
					== INVALID_HANDLE_VALUE) {
				bOpenFail = true;
				return false;
			}
			if ((hFileP = CreateFile(cpFileP, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							FILE_FLAG_SEQUENTIAL_SCAN, NULL))
					== INVALID_HANDLE_VALUE) {
				CloseHandle(hFileA);
				bOpenFail = true;
				return false;
			}

			bool bEqual = true;
			DWORD ReadSizeA, ReadSizeP;
			if (!Opt.CompareContentsIgnore) {
				do {
					if (CheckForEsc() || !ReadFile(hFileA, ABuf, bufSize, &ReadSizeA, NULL)
							|| !ReadFile(hFileP, PBuf, bufSize, &ReadSizeP, NULL) || ReadSizeA != ReadSizeP
							|| memcmp(ABuf, PBuf, ReadSizeA)) {
						bEqual = false;
						break;
					}
				} while (ReadSizeA == bufSize);
			} else {
				ReadSizeA = 1;
				ReadSizeP = 1;
				char *PtrA = ABuf + ReadSizeA, *PtrP = PBuf + ReadSizeP;
				bool bExpectNewLineA = false;
				bool bExpectNewLineP = false;
				while (true) {
					while (PtrA >= ABuf + ReadSizeA && ReadSizeA) {
						if (CheckForEsc() || !ReadFile(hFileA, ABuf, bufSize, &ReadSizeA, NULL)) {
							bEqual = false;
							break;
						}
						PtrA = ABuf;
					}

					if (!bEqual)
						break;

					while (PtrP >= PBuf + ReadSizeP && ReadSizeP) {
						if (CheckForEsc() || !ReadFile(hFileP, PBuf, bufSize, &ReadSizeP, NULL)) {
							bEqual = false;
							break;
						}
						PtrP = PBuf;
					}

					if (!bEqual || (!ReadSizeP && !ReadSizeA))
						break;

					if (Opt.IgnoreWhitespace) {
						while (PtrA < ABuf + ReadSizeA && PtrP < PBuf + ReadSizeP && !isspace(*PtrA)
								&& !isspace(*PtrP)) {
							if (*PtrA != *PtrP) {
								bEqual = false;
								break;
							}
							++PtrA;
							++PtrP;
						}

						if (!bEqual)
							break;

						while (PtrA < ABuf + ReadSizeA && isspace(*PtrA))
							++PtrA;

						while (PtrP < PBuf + ReadSizeP && isspace(*PtrP))
							++PtrP;
					} else {
						if (bExpectNewLineA) {
							bExpectNewLineA = false;
							if (PtrA < ABuf + ReadSizeA && *PtrA == '\n')
								++PtrA;
						}

						if (bExpectNewLineP) {
							bExpectNewLineP = false;
							if (PtrP < PBuf + ReadSizeP && *PtrP == '\n')
								++PtrP;
						}

						while (PtrA < ABuf + ReadSizeA && PtrP < PBuf + ReadSizeP && !isnewline(*PtrA)
								&& !isnewline(*PtrP)) {
							if (*PtrA != *PtrP) {
								bEqual = false;
								break;
							}
							++PtrA;
							++PtrP;
						}

						if (!bEqual)
							break;

						if (PtrA < ABuf + ReadSizeA && PtrP < PBuf + ReadSizeP
								&& (!isnewline(*PtrA) || !isnewline(*PtrP))) {
							bEqual = false;
							break;
						}

						if (PtrA < ABuf + ReadSizeA && PtrP < PBuf + ReadSizeP) {
							if (*PtrA == '\r')
								bExpectNewLineA = true;

							if (*PtrP == '\r')
								bExpectNewLineP = true;

							++PtrA;
							++PtrP;
						}
					}

					if (PtrA < ABuf + ReadSizeA && !ReadSizeP) {
						bEqual = false;
						break;
					}

					if (PtrP < PBuf + ReadSizeP && !ReadSizeA) {
						bEqual = false;
						break;
					}
				}
			}

			CloseHandle(hFileA);
			CloseHandle(hFileP);
			return bEqual;
		}
	}
	return true;
}

/****************************************************************************
 *
 *
 *
 *
 *
 ****************************************************************************/
static bool CompareDirs(const OwnPanelInfo *AInfo, const OwnPanelInfo *PInfo, bool bCompareAll, int ScanDepth)
{
#ifndef _UNICODE
#define _CurDir CurDir
#else
#define _CurDir lpwszCurDir
#endif
	//
	struct FileIndex sfiA, sfiP;
	TCHAR DirA[MAX_PATH], DirP[MAX_PATH];
	ShowMessage(lstrcpy(DirA, BuildFullFilename(AInfo->_CurDir, _T("*"))),
			lstrcpy(DirP, BuildFullFilename(PInfo->_CurDir, _T("*"))));
#ifndef UNICODE
	if (!BuildPanelIndex(AInfo, &sfiA) || !BuildPanelIndex(PInfo, &sfiP))
#else
	if (!BuildPanelIndex(AInfo, &sfiA, AFilter) || !BuildPanelIndex(PInfo, &sfiP, PFilter))
#endif
	{
		const TCHAR *MsgItems[] = {GetMsg(MNoMemTitle), GetMsg(MNoMemBody), GetMsg(MOK)};
		Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
		bBrokenByEsc = true;
		FreePanelIndex(&sfiA);
		FreePanelIndex(&sfiP);
		return true;
	}
	bool bDifferenceNotFound = true;
	int i = sfiA.iCount - 1, j = sfiP.iCount - 1;
	while (i >= 0 && j >= 0 && (bDifferenceNotFound || bCompareAll) && !bBrokenByEsc) {
		const int iMaxCounter = 256;
		static int iCounter = iMaxCounter;
		if (!--iCounter) {
			iCounter = iMaxCounter;
			if (CheckForEsc())
				break;
		}
		switch (PICompare(&sfiA.ppi[i], &sfiP.ppi[j])) {
			case 0:							//
				if (CompareFiles(&sfiA.ppi[i]->FindData, &sfiP.ppi[j]->FindData, AInfo->_CurDir, PInfo->_CurDir,
							ScanDepth)) {	//
					sfiA.ppi[i--]->Flags&= ~PPIF_SELECTED;
					sfiP.ppi[j--]->Flags&= ~PPIF_SELECTED;
				} else {
					bDifferenceNotFound = false;
					sfiA.ppi[i--]->Flags|= PPIF_SELECTED;
					sfiP.ppi[j--]->Flags|= PPIF_SELECTED;
				}
				break;
			case 1:		//
				bDifferenceNotFound = false;
				sfiA.ppi[i--]->Flags|= PPIF_SELECTED;
				break;
			case -1:	//
				bDifferenceNotFound = false;
				sfiP.ppi[j--]->Flags|= PPIF_SELECTED;
				break;
		}
	}
	if (!bBrokenByEsc) {
		//
		if (i >= 0) {
			bDifferenceNotFound = false;
			if (bCompareAll)
				for (; i >= 0; i--)
					sfiA.ppi[i]->Flags|= PPIF_SELECTED;
		}
		if (j >= 0) {
			bDifferenceNotFound = false;
			if (bCompareAll)
				for (; j >= 0; j--)
					sfiP.ppi[j]->Flags|= PPIF_SELECTED;
		}
	}
	FreePanelIndex(&sfiA);
	FreePanelIndex(&sfiP);
	return bDifferenceNotFound;
#undef _CurDir
}

/****************************************************************************
 ***************************** Exported functions ***************************
 ****************************************************************************/

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

/****************************************************************************
 *
 ****************************************************************************/
SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	const TCHAR *cpPlugRegKey = _T("/AdvCompare");
	::Info = *Info;

	FSF = *Info->FSF;

	if (PluginRootKey) {
		free(PluginRootKey);
		PluginRootKey = NULL;
	}

	if ((PluginRootKey = (TCHAR *)malloc(
					sizeof(TCHAR) * (lstrlen(Info->RootKey) + lstrlen(cpPlugRegKey) + 1)))
			!= NULL) {
		lstrcpy(PluginRootKey, Info->RootKey);
		lstrcat(PluginRootKey, cpPlugRegKey);
	} else {
		const TCHAR *MsgItems[] = {GetMsg(MNoMemTitle), GetMsg(MNoMemBody), GetMsg(MOK)};
		::Info.Message(::Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
	}
}

/****************************************************************************
 *
 ****************************************************************************/
SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	static const TCHAR *PluginMenuStrings[1];

	Info->StructSize = (int)sizeof(*Info);
	Info->Flags = 0;
	Info->DiskMenuStrings = NULL;
	PluginMenuStrings[0] = GetMsg(MCompare);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
	Info->PluginConfigStrings = NULL;
	Info->CommandPrefix = NULL;
}

#ifndef UNICODE
#define FreePanelItems(a, b)
#else
void GetPanelItem(HANDLE hPlugin, int Command, int Param1, PluginPanelItem *Param2)
{
	PluginPanelItem *item = (PluginPanelItem *)malloc(Info.Control(hPlugin, Command, Param1, 0));
	if (item) {
		Info.Control(hPlugin, Command, Param1, (LONG_PTR)item);
		*Param2 = *item;
		Param2->FindData.lpwszFileName = wcsdup(item->FindData.lpwszFileName);
		Param2->Description = NULL;
		Param2->Owner = NULL;
		Param2->CustomColumnData = NULL;
		Param2->UserData = 0;
		free(item);
	}
}

void FreePanelItems(OwnPanelInfo &AInfo, OwnPanelInfo &PInfo)
{
	for (int i = 0; i < AInfo.ItemsNumber; i++) {
		free((void *)AInfo.PanelItems[i].FindData.lpwszFileName);
	}
	for (int i = 0; i < AInfo.SelectedItemsNumber; i++) {
		free((void *)AInfo.SelectedItems[i].FindData.lpwszFileName);
	}
	delete[] AInfo.PanelItems;
	delete[] AInfo.SelectedItems;

	for (int i = 0; i < PInfo.ItemsNumber; i++) {
		free((void *)PInfo.PanelItems[i].FindData.lpwszFileName);
	}
	for (int i = 0; i < PInfo.SelectedItemsNumber; i++) {
		free((void *)PInfo.SelectedItems[i].FindData.lpwszFileName);
	}
	delete[] PInfo.PanelItems;
	delete[] PInfo.SelectedItems;
}
#endif

/****************************************************************************
 *
 ****************************************************************************/
SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	OwnPanelInfo AInfo, PInfo;

#ifdef UNICODE
	memset(&AInfo, 0, sizeof(OwnPanelInfo));
	memset(&PInfo, 0, sizeof(OwnPanelInfo));
#endif

#ifndef UNICODE
	//
	if (!Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &AInfo)
			|| !Info.Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELINFO, &PInfo)) {
		return INVALID_HANDLE_VALUE;
	}
#else
	PanelInfo AI, PI;
	Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&AI);
	Info.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PI);

	AInfo.PanelType = AI.PanelType;
	AInfo.Plugin = AI.Plugin;
	AInfo.ItemsNumber = AI.ItemsNumber;
	AInfo.SelectedItemsNumber = AI.SelectedItemsNumber;

	int Size = Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)0);
	AInfo.lpwszCurDir = new wchar_t[Size];
	Info.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, Size, (LONG_PTR)AInfo.lpwszCurDir);

	if (AInfo.ItemsNumber) {
		AInfo.PanelItems = new PluginPanelItem[AInfo.ItemsNumber];
		for (int i = 0; i < AInfo.ItemsNumber; i++)
			GetPanelItem(PANEL_ACTIVE, FCTL_GETPANELITEM, i, &AInfo.PanelItems[i]);
	} else
		AInfo.PanelItems = NULL;

	if (AInfo.SelectedItemsNumber) {
		AInfo.SelectedItems = new PluginPanelItem[AInfo.SelectedItemsNumber];
		for (int i = 0; i < AInfo.SelectedItemsNumber; i++)
			GetPanelItem(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, &AInfo.SelectedItems[i]);
	} else
		AInfo.SelectedItems = NULL;

	PInfo.PanelType = PI.PanelType;
	PInfo.Plugin = PI.Plugin;
	PInfo.ItemsNumber = PI.ItemsNumber;
	PInfo.SelectedItemsNumber = PI.SelectedItemsNumber;

	Size = Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)0);
	PInfo.lpwszCurDir = new wchar_t[Size];
	Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, Size, (LONG_PTR)PInfo.lpwszCurDir);

	if (PInfo.ItemsNumber) {
		PInfo.PanelItems = new PluginPanelItem[PInfo.ItemsNumber];
		for (int i = 0; i < PInfo.ItemsNumber; i++)
			GetPanelItem(PANEL_PASSIVE, FCTL_GETPANELITEM, i, &PInfo.PanelItems[i]);
	} else
		PInfo.PanelItems = NULL;

	if (PInfo.SelectedItemsNumber) {
		PInfo.SelectedItems = new PluginPanelItem[PInfo.SelectedItemsNumber];
		for (int i = 0; i < PInfo.SelectedItemsNumber; i++)
			GetPanelItem(PANEL_PASSIVE, FCTL_GETSELECTEDPANELITEM, i, &PInfo.SelectedItems[i]);
	} else
		PInfo.SelectedItems = NULL;
#endif

	//
	if (AInfo.PanelType != PTYPE_FILEPANEL || PInfo.PanelType != PTYPE_FILEPANEL) {
		const TCHAR *MsgItems[] = {GetMsg(MCmpTitle), GetMsg(MFilePanelsRequired), GetMsg(MOK)};
		Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
		FreePanelItems(AInfo, PInfo);
		return INVALID_HANDLE_VALUE;
	}

	//
	if (!ShowDialog(AInfo.Plugin || PInfo.Plugin,
				(AInfo.SelectedItemsNumber && (AInfo.SelectedItems[0].Flags & PPIF_SELECTED))
						|| (PInfo.SelectedItemsNumber && (PInfo.SelectedItems[0].Flags & PPIF_SELECTED)))) {
		FreePanelItems(AInfo, PInfo);
		return INVALID_HANDLE_VALUE;
	}

	//
	HANDLE hScreen = Info.SaveScreen(0, 0, -1, -1);
	hConInp = NULL;

	//
	HANDLE hConOut = NULL;
	CONSOLE_SCREEN_BUFFER_INFO csbiNfo;
	if (GetConsoleScreenBufferInfo(hConOut, &csbiNfo)) {
		if ((iTruncLen = csbiNfo.dwSize.X - 20) > MAX_PATH - 2)
			iTruncLen = MAX_PATH - 2;
		else if (iTruncLen < 0)
			iTruncLen = csbiNfo.dwSize.X - csbiNfo.dwSize.X / 4;
	} else
		iTruncLen = 60;

	//
	TCHAR cConsoleTitle[MAX_PATH], cBuffer[MAX_PATH];
	DWORD dwTitleSaved = GetConsoleTitle(NULL, cConsoleTitle, ARRAYSIZE(cConsoleTitle));
#ifndef UNICODE
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (GetVersionEx(&ovi) && ovi.dwPlatformId != VER_PLATFORM_WIN32_NT)
		OemToChar(GetMsg(MComparingFiles), cBuffer);
	else
#endif
		lstrcpy(cBuffer, GetMsg(MComparingFiles));
	SetConsoleTitle(NULL, cBuffer);

	//

	bufSize = KeyFileReadSection(INI_LOCATION, INI_SECTION).GetInt("CompareBufferSize", 32768);
	if (bufSize > 32768)
		bufSize = 32768;

	ABuf = (char *)malloc(bufSize);
	PBuf = (char *)malloc(bufSize);

	bBrokenByEsc = false;
	bStart = true;
	bOpenFail = false;
	bool bDifferenceNotFound = false;

#ifdef UNICODE
	AFilter = INVALID_HANDLE_VALUE;
	PFilter = INVALID_HANDLE_VALUE;

	Info.FileFilterControl(PANEL_ACTIVE, FFCTL_CREATEFILEFILTER, FFT_PANEL, (LONG_PTR)&AFilter);
	Info.FileFilterControl(PANEL_PASSIVE, FFCTL_CREATEFILEFILTER, FFT_PANEL, (LONG_PTR)&PFilter);

	Info.FileFilterControl(AFilter, FFCTL_STARTINGTOFILTER, 0, 0);
	Info.FileFilterControl(PFilter, FFCTL_STARTINGTOFILTER, 0, 0);
#endif

	//
	if (ABuf && PBuf
#ifdef UNICODE
			&& AFilter != INVALID_HANDLE_VALUE && PFilter != INVALID_HANDLE_VALUE
#endif
	) {
		bDifferenceNotFound = CompareDirs(&AInfo, &PInfo, true, 0);
	}

#ifdef UNICODE
	Info.FileFilterControl(AFilter, FFCTL_FREEFILEFILTER, 0, 0);
	Info.FileFilterControl(PFilter, FFCTL_FREEFILEFILTER, 0, 0);
#endif

	free(ABuf);
	free(PBuf);
	Info.RestoreScreen(hScreen);

	//
	if (!bBrokenByEsc) {
#ifndef UNICODE
		Info.Control(INVALID_HANDLE_VALUE, FCTL_SETSELECTION, &AInfo);
		Info.Control(INVALID_HANDLE_VALUE, FCTL_SETANOTHERSELECTION, &PInfo);
		Info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, (LONG_PTR)0);
		Info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWANOTHERPANEL, (LONG_PTR)0);
#else
		Info.Control(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, (LONG_PTR)0);
		for (int i = 0; i < AInfo.ItemsNumber; i++) {
			Info.Control(PANEL_ACTIVE, FCTL_SETSELECTION, i, AInfo.PanelItems[i].Flags & PPIF_SELECTED);
		}
		Info.Control(PANEL_ACTIVE, FCTL_ENDSELECTION, 0, (LONG_PTR)0);

		Info.Control(PANEL_PASSIVE, FCTL_BEGINSELECTION, 0, (LONG_PTR)0);
		for (int i = 0; i < PInfo.ItemsNumber; i++) {
			Info.Control(PANEL_PASSIVE, FCTL_SETSELECTION, i, PInfo.PanelItems[i].Flags & PPIF_SELECTED);
		}
		Info.Control(PANEL_PASSIVE, FCTL_ENDSELECTION, 0, (LONG_PTR)0);

		Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)0);
		Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)0);

#endif
		if (bOpenFail) {
			const TCHAR *MsgItems[] = {
					GetMsg(MOpenErrorTitle),
					GetMsg(MOpenErrorBody),
					GetMsg(MOK),
			};
			Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
		if (bDifferenceNotFound && Opt.MessageWhenNoDiff) {
			const TCHAR *MsgItems[] = {GetMsg(MNoDiffTitle), GetMsg(MNoDiffBody), GetMsg(MOK)};
			Info.Message(Info.ModuleNumber, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
	}
	//
	if (dwTitleSaved)
		SetConsoleTitle(NULL, cConsoleTitle);
	FreePanelItems(AInfo, PInfo);
	return INVALID_HANDLE_VALUE;
}

/****************************************************************************
 *
 ****************************************************************************/
SHAREDSYMBOL void WINAPI EXP_NAME(ExitFAR)(void)
{
	if (PluginRootKey) {
		free(PluginRootKey);
		PluginRootKey = NULL;
	}
}
