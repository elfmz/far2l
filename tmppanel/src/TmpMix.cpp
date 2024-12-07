/*
TMPMIX.CPP

Temporary panel miscellaneous utility functions

*/

#include "TmpPanel.hpp"
#include <string>
#include <utils.h>

const TCHAR *GetMsg(int MsgId)
{
	return (Info.GetMsg(Info.ModuleNumber, MsgId));
}

void InitDialogItems(const MyInitDialogItem *Init, struct FarDialogItem *Item, int ItemsNumber)
{
	int i;
	struct FarDialogItem *PItem = Item;
	const MyInitDialogItem *PInit = Init;
	for (i = 0; i < ItemsNumber; i++, PItem++, PInit++) {
		PItem->Type = PInit->Type;
		PItem->X1 = PInit->X1;
		PItem->Y1 = PInit->Y1;
		PItem->X2 = PInit->X2;
		PItem->Y2 = PInit->Y2;
		PItem->Flags = PInit->Flags;
		PItem->Focus = 0;
		PItem->History = 0;
		PItem->DefaultButton = 0;
		PItem->MaxLen = 0;
		PItem->PtrData = PInit->Data != -1 ? GetMsg(PInit->Data) : L"";
	}
}

void FreePanelItems(PluginPanelItem *Items, DWORD Total)
{
	if (Items) {
		for (DWORD I = 0; I < Total; I++) {
			if (Items[I].Owner)
				free((void *)Items[I].Owner);
			if (Items[I].FindData.lpwszFileName)
				free((void *)Items[I].FindData.lpwszFileName);
		}
		free(Items);
	}
}

TCHAR *ParseParam(TCHAR *&str)
{
	TCHAR *p = str;
	TCHAR *parm = NULL;
	if (*p == _T('|')) {
		parm = ++p;
		p = _tcschr(p, _T('|'));
		if (p) {
			*p = _T('\0');
			str = p + 1;
			FSF.LTrim(str);
			return parm;
		}
	}
	return NULL;
}

void GoToFile(const TCHAR *Target, BOOL AnotherPanel)
{
	HANDLE _PANEL_HANDLE = AnotherPanel ? PANEL_PASSIVE : PANEL_ACTIVE;

	PanelRedrawInfo PRI;
	PanelInfo PInfo;
	int pathlen;

	const TCHAR *p = FSF.PointToName(const_cast<TCHAR *>(Target));
	StrBuf Name(lstrlen(p) + 1);
	lstrcpy(Name, p);
	pathlen = (int)(p - Target);
	StrBuf Dir(pathlen + 1);
	if (pathlen)
		memcpy(Dir.Ptr(), Target, pathlen * sizeof(TCHAR));
	Dir[pathlen] = _T('\0');

	FSF.Trim(Name);
	FSF.Trim(Dir);
	FSF.Unquote(Name);
	FSF.Unquote(Dir);

	if (*Dir.Ptr()) {
		Info.Control(_PANEL_HANDLE, FCTL_SETPANELDIR, 0, (LONG_PTR)Dir.Ptr());
	}

	Info.Control(_PANEL_HANDLE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PInfo);

	PRI.CurrentItem = PInfo.CurrentItem;
	PRI.TopPanelItem = PInfo.TopPanelItem;

	for (int J = 0; J < PInfo.ItemsNumber; J++) {

		PluginPanelItem *PPI =
				(PluginPanelItem *)malloc(Info.Control(_PANEL_HANDLE, FCTL_GETPANELITEM, J, 0));
		if (PPI) {
			Info.Control(_PANEL_HANDLE, FCTL_GETPANELITEM, J, (LONG_PTR)PPI);
		}

		if (!FSF.LStricmp(Name, FSF.PointToName((PPI ? PPI->FindData.lpwszFileName : NULL))))
		{
			PRI.CurrentItem = J;
			PRI.TopPanelItem = J;
			free(PPI);
			break;
		}
		free(PPI);
	}
	Info.Control(_PANEL_HANDLE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&PRI);
}

void WFD2FFD(WIN32_FIND_DATA &wfd, FAR_FIND_DATA &ffd)
{
	ffd.dwFileAttributes = wfd.dwFileAttributes;
	ffd.dwUnixMode = wfd.dwUnixMode;
	ffd.ftCreationTime = wfd.ftCreationTime;
	ffd.ftLastAccessTime = wfd.ftLastAccessTime;
	ffd.ftLastWriteTime = wfd.ftLastWriteTime;
	ffd.nFileSize = wfd.nFileSize;
	ffd.nPhysicalSize = wfd.nPhysicalSize;
	ffd.lpwszFileName = wcsdup(wfd.cFileName);
}

wchar_t *FormNtPath(const wchar_t *path, StrBuf &buf)
{
	int l = lstrlen(path);
	buf.Grow(l + 1);
	lstrcpy(buf, path);
	return buf;
}

TCHAR *ExpandEnvStrs(const TCHAR *input, StrBuf &output)
{

	std::string s;
	Wide2MB(input, s);
	Environment::ExpandString(s, false);
	std::wstring w;
	StrMB2Wide(s, w);
	output.Grow(w.size() + 1);
	lstrcpy(output, w.c_str());

	return output;
}

static DWORD LookAtPath(const TCHAR *dir, const TCHAR *name, TCHAR *buffer = NULL, DWORD buf_size = 0)
{
	std::wstring path(dir);
	if (path.empty())
		return 0;

	if (path[path.size() - 1] != GOOD_SLASH)
		path+= GOOD_SLASH;

	path+= name;
	if (GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
		return 0;

	if (buf_size >= (path.size() + 1))
		wcscpy(buffer, path.c_str());

	return path.size() + 1;
}

bool FindListFile(const TCHAR *FileName, StrBuf &output)
{
	StrBuf Path;
	DWORD dwSize;

	StrBuf FullPath;
	GetFullPath(FileName, FullPath);
	StrBuf NtPath;
	FormNtPath(FullPath, NtPath);

	const TCHAR *final = NULL;
	if (GetFileAttributes(NtPath) != INVALID_FILE_ATTRIBUTES) {
		output.Grow(FullPath.Size());
		lstrcpy(output, FullPath);
		return true;
	}
	{
		const TCHAR *tmp = FSF.PointToName(Info.ModuleName);
		Path.Grow((int)(tmp - Info.ModuleName + 1));
		lstrcpyn(Path, Info.ModuleName, (int)(tmp - Info.ModuleName + 1));
		dwSize = LookAtPath(Path, FileName);
		if (dwSize) {
			final = Path;
			goto success;
		}
	}
	ExpandEnvStrs(_T("$FARHOME:$PATH"), Path);
	for (TCHAR *str = Path, *p = _tcschr(Path, _T(':')); *str; p = _tcschr(str, _T(':'))) {
		if (p)
			*p = 0;

		FSF.Unquote(str);
		FSF.Trim(str);

		if (*str) {
			dwSize = LookAtPath(str, FileName);
			if (dwSize) {
				final = str;
				goto success;
			}
		}

		if (p)
			str = p + 1;
		else
			break;
	}

	return false;

success:

	output.Grow(dwSize);
	if (LookAtPath(final, FileName, output, dwSize) != dwSize)
		fprintf(stderr, "LookAtPath: unexpected size\n");

	return true;
}

wchar_t *GetFullPath(const wchar_t *input, StrBuf &output)
{
	output.Grow(MAX_PATH);
	int size = FSF.ConvertPath(CPM_FULL, input, output, output.Size());
	if (size > output.Size()) {
		output.Grow(size);
		FSF.ConvertPath(CPM_FULL, input, output, output.Size());
	}
	return output;
}

int PWZ_to_PZ(const wchar_t *src, char *dst, int lendst)
{
    ErrnoSaver ErSr;
    return WINPORT(WideCharToMultiByte)(CP_UTF8, 0, (src), -1, (dst), (int)(lendst), nullptr, nullptr);
}
