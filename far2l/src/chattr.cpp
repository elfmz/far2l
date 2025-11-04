/*
chattr.cpp

Просмотр и установка атрибутов/флагов файлов аналогично lsattr/ls -lo и chattr/chflags
*/
/*
Copyright (c) 2025- Far2l Group
*/

#include "headers.hpp"

#include "panel.hpp"
#include "ctrlobj.hpp"
#include "message.hpp"
#include "chattr.hpp"
#include "pathmix.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "vmenu.hpp"

#ifdef __linux__
#include <linux/fs.h>
#endif

#include "FSFileFlags.h"

static struct {
	unsigned long flag;
	char name_short;
	const char *name_long;
} flags_list[] = {
#ifdef __linux__
// for linux systems see actual constants in <linux/fs.h>
#ifdef FS_SECRM_FL			// 0x00000001
	{ FS_SECRM_FL,			's', "(s) Secure deletion" },
#endif
#ifdef FS_UNRM_FL			// 0x00000002
	{ FS_UNRM_FL,			'u', "(u) Undelete" },
#endif
#ifdef FS_COMPR_FL			// 0x00000004
	{ FS_COMPR_FL,			'c', "(c) Compress" },
#endif
#ifdef FS_SYNC_FL			// 0x00000008
	{ FS_SYNC_FL,			'S', "(S) Synchronous updates" },
#endif
#ifdef FS_IMMUTABLE_FL		// 0x00000010
	{ FS_IMMUTABLE_FL,		'i', "(i) Immutable" },
#endif
#ifdef FS_APPEND_FL			// 0x00000020
	{ FS_APPEND_FL,			'a', "(a) Append only" },
#endif
#ifdef FS_NODUMP_FL			// 0x00000040
	{ FS_NODUMP_FL,			'd', "(d) No dump" },
#endif
#ifdef FS_NOATIME_FL		// 0x00000080
	{ FS_NOATIME_FL,		'A', "(A) No update atime" },
#endif
#ifdef FS_DIRTY_FL			// 0x00000100
	{ FS_DIRTY_FL,			'Z', "(Z) Compressed dirty file" },
#endif
#ifdef FS_COMPRBLK_FL		// 0x00000200
	{ FS_COMPRBLK_FL,		'B', "(B) Compressed clusters" },
#endif
#ifdef FS_NOCOMP_FL			// 0x00000400
	{ FS_NOCOMP_FL,			'X', "(X) Don't compress" },
#endif
#ifdef FS_ENCRYPT_FL		// 0x00000800
	{ FS_ENCRYPT_FL,		'E', "(E) Encrypted file" },
#endif
#ifdef FS_INDEX_FL			// 0x00001000
	{ FS_INDEX_FL,			'I', "(I) Indexed directory" },
#endif
#ifdef FS_IMAGIC_FL			// 0x00002000
	{ FS_IMAGIC_FL,			'+', "    AFS directory" },
#endif
#ifdef FS_JOURNAL_DATA_FL	// 0x00004000
	{ FS_JOURNAL_DATA_FL,	'j', "(j) Journaled data" },
#endif
#ifdef FS_NOTAIL_FL			// 0x00008000
	{ FS_NOTAIL_FL,			't', "(t) No tail merging" },
#endif
#ifdef FS_DIRSYNC_FL		// 0x00010000
	{ FS_DIRSYNC_FL,		'D', "(D) Synchronous directory updates" },
#endif
#ifdef FS_TOPDIR_FL			// 0x00020000
	{ FS_TOPDIR_FL,			'T', "(T) Top of directory hierarchies" },
#endif
#ifdef FS_HUGE_FILE_FL		// 0x00040000
	{ FS_HUGE_FILE_FL,		'h', "(h) Huge_file" },
#endif
#ifdef FS_EXTENT_FL			// 0x00080000
	{ FS_EXTENT_FL,			'e', "(e) Inode uses extents" },
#endif
#ifdef FS_VERITY_FL			// 0x00100000
	{ FS_VERITY_FL,			'V', "(V) Verity protected inode" },
#endif
#ifdef FS_EA_INODE_FL		// 0x00200000
	{ FS_EA_INODE_FL,		'+', "    Inode used for large EA" },
#endif
#ifdef FS_EOFBLOCKS_FL		// 0x00400000
	{ FS_EOFBLOCKS_FL,		'+', "    EOFBLOCKS" },
#endif
#ifdef FS_NOCOW_FL			// 0x00800000
	{ FS_NOCOW_FL,			'C', "(C) No COW" },
#endif
#ifdef FS_DAX_FL			// 0x02000000
	{ FS_DAX_FL,			'x', "(x) Direct access for files" },
#endif
#ifdef FS_INLINE_DATA_FL	// 0x10000000
	{ FS_INLINE_DATA_FL,	'N', "(N) Inode has inline data" },
#endif
#ifdef FS_PROJINHERIT_FL	// 0x20000000
	{ FS_PROJINHERIT_FL,	'P', "(P) Project hierarchy" },
#endif
#ifdef FS_CASEFOLD_FL		// 0x40000000
	{ FS_CASEFOLD_FL,		'F', "(F) Casefolded file" },
#endif
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
// see actual constants in <sys/stat.h>
// Super-user and owner changeable flags
#ifdef UF_NODUMP		// 0x00000001
	{ UF_NODUMP,		'+', "nodump:      do not dump file (owner or super-user only)" },
#endif
#ifdef UF_IMMUTABLE		// 0x00000002
	{ UF_IMMUTABLE,		'+', "uimmutable:  file may not be changed (owner or super-user only)" },
#endif
#ifdef UF_APPEND		// 0x00000004
	{ UF_APPEND,		'+', "uappend:     writes to file may only append (owner or super-user only)" },
#endif
#ifdef UF_OPAQUE		// 0x00000008
	{ UF_OPAQUE,		'+', "opaque:      directory is opaque wrt. union (owner or super-user only)" },
#endif
#ifdef UF_NOUNLINK		// 0x00000010
	{ UF_NOUNLINK,		'+', "uunlink:     file may not be removed or renamed (owner or super-user only)" },
#endif
#ifdef UF_COMPRESSED	// 0x00000020 (in macOS)
	{ UF_COMPRESSED,	'+', "ucompressed: file is compressed (macOS only) (read-only some file-systems)" },
#endif
#ifdef UF_TRACKED		// 0x00000040 (in macOS)
	{ UF_TRACKED,		'+', "utracked:    renames and deletes are tracked (macOS only) (owner or super-user only)" },
#endif
#ifdef UF_SYSTEM		// 0x00000080
	{ UF_SYSTEM,		'+', "usystem:     Windows system file bit (owner or super-user only)" },
#elif defined(UF_DATAVAULT)	// 0x00000080 (in macOS)
	{ UF_DATAVAULT,		'+', "udatavault:  entitlement required for reading and writing (read-only)" },
#endif
#ifdef UF_SPARSE		// 0x00000100
	{ UF_SPARSE,		'+', "usparse:     sparse file (owner or super-user only)" },
#endif
#ifdef UF_OFFLINE		// 0x00000200
	{ UF_OFFLINE,		'+', "uoffline:    file is offline (owner or super-user only)" },
#endif
#ifdef UF_REPARSE		// 0x00000400
	{ UF_REPARSE,		'+', "ureparse:    Windows reparse point file bit (owner or super-user only)" },
#endif
#ifdef UF_ARCHIVE		// 0x00000800
	{ UF_ARCHIVE,		'+', "uarchive:    file needs to be archived (owner or super-user only)" },
#endif
#ifdef UF_READONLY		// 0x00001000
	{ UF_READONLY,		'+', "ureadonly:   Windows readonly file bit (owner or super-user only)" },
#endif
#ifdef UF_HIDDEN		// 0x00008000
	{ UF_HIDDEN,		'+', "uhidden:     file is hidden (owner or super-user only)" },
#endif
// Super-user changeable flags
#ifdef SF_ARCHIVED		// 0x00010000
	{ SF_ARCHIVED,		'+', "sarchive:    file is archived (super-user only)" },
#endif
#ifdef SF_IMMUTABLE		// 0x00020000
	{ SF_IMMUTABLE,		'+', "simmutable:  file may not be changed (super-user only)" },
#endif
#ifdef SF_APPEND		// 0x00040000
	{ SF_APPEND,		'+', "sappend:     writes to file may only append (super-user only)" },
#endif
#ifdef SF_RESTRICTED	// 0x00080000 (in macOS)
	{ SF_RESTRICTED,	'+', "srestricted: entitlement required for writing (super-user only)" },
#endif
#ifdef SF_NOUNLINK		// 0x00100000
	{ SF_NOUNLINK,		'+', "sunlink:     file may not be removed or renamed (super-user only)" },
#endif
#ifdef SF_SNAPSHOT		// 0x00200000
	{ SF_SNAPSHOT,		'+', "snapshot:    snapshot inode (filesystems do not allow changing this flag)" },
#endif
#ifdef SF_FIRMLINK		// 0x00800000 (in macOS)
	{ SF_FIRMLINK,		'+', "sfirmlink:   file is a firmlink (super-user only)" },
#endif
#ifdef SF_DATALESS		// 0x40000000 (in macOS)
	{ SF_DATALESS,		'+', "sdataless:   file is dataless object (read-only)" },
#endif
#endif

#if defined(__HAIKU__)
// TODO ?
#endif
};

inline void GetFlagStr2BarList(unsigned i, char &ch, FARString &str, bool on, bool changed)
{
	ch = on ? flags_list[i].name_short : '-';
	str.Format(L"%c[%c] %s", changed ? '*' : ' ', on ? 'x' : ' ', flags_list[i].name_long);
}

enum CHATTRDLG
{
	CA_DOUBLEBOX,
	CA_TEXT_FILENAME,
	CA_TEXT_FLAGSBAR,
	CA_SEPARATOR1,
	CA_LIST_FLAGS,
	CA_SEPARATOR2,
	CA_BUTTON_SET,
	CA_BUTTON_CANCEL,
	CA_BUTTON_RESET
};

static void flags_show(HANDLE hDlg, bool toggle)
{
	char ch;
	FARString strFlagsBar, strListBoxLine;

	VMenu *ListBox = reinterpret_cast<Dialog *>(hDlg)->GetAllItem()[CA_LIST_FLAGS]->ListPtr;
	FSFileFlags *FFFlags =
			reinterpret_cast<FSFileFlags *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));

	FarListPos ListPos;
	ListBox->GetSelectPos(&ListPos);
	bool sel_pos = ListPos.SelectPos >= 0 && ListPos.SelectPos < (int)ARRAYSIZE(flags_list);
	if (toggle && sel_pos)
		FFFlags->FlagInverse(flags_list[ListPos.SelectPos].flag);

	SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
	ListBox->DeleteItems();
	for (unsigned i=0; i<ARRAYSIZE(flags_list); i++) {
		GetFlagStr2BarList(i, ch, strListBoxLine,
			FFFlags->FlagIsOn(flags_list[i].flag), !FFFlags->FlagEqActual(flags_list[i].flag) );
		strFlagsBar += ch;
		ListBox->AddItem(strListBoxLine);
		SendDlgMessage(hDlg, DM_SETTEXTPTR, CA_TEXT_FLAGSBAR, (LONG_PTR)strFlagsBar.CPtr());
	}

	if (sel_pos) {
		ListBox->SetSelectPos(&ListPos);
		ListBox->FastShow();
	}
	SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
}

static LONG_PTR WINAPI ChattrDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_INITDIALOG: {
			VMenu *ListBox = reinterpret_cast<Dialog *>(hDlg)->GetAllItem()[CA_LIST_FLAGS]->ListPtr;
			ListBox->ClearFlags(VMENU_MOUSEREACTION);
			flags_show(hDlg, false);
			return TRUE;
		}
		case DN_KEY:
			if (Param1 == CA_LIST_FLAGS && (Param2 == VK_SPACE || Param2 == KEY_ENTER || Param2 == KEY_NUMENTER)) {
				flags_show(hDlg, true);
				return TRUE;
			}
			break;
		case DN_CLOSE:
			if (Param1 == CA_LIST_FLAGS) { // without DIF_LISTNOCLOSE mouse click in ListBox try close dialog
				flags_show(hDlg, true);
				return FALSE; // no close if mouse click in ListBox
			}
			break;
		case DN_BTNCLICK:
			if (Param1 == CA_BUTTON_RESET) {
				FSFileFlags *FFFlags =
						reinterpret_cast<FSFileFlags *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
				FFFlags->Reset();
				flags_show(hDlg, false);
				return TRUE;
			}
			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void error_append(FARString &str, int err)
{
	const char *str_err = strerror(err);
	if (str_err)
		str.AppendFormat(L"\"%s\" (%d)", str_err, err);
	else
		str.AppendFormat(L"Error %d", err);
}

bool ChattrDialog(Panel *SrcPanel)
{
	if (SrcPanel == nullptr)
		return false;

	if (SrcPanel->GetSelCount() < 1)
		return false;

	if (ARRAYSIZE(flags_list) < 1) {
		ExMessager em(Msg::ChAttrTitle);
		em.AddMultiline(Msg::ChAttrWarnSystem);
		em.AddDup(Msg::Ok);
		em.Show(MSG_WARNING, 1);
		return false;
	}

	if (SrcPanel->GetSelCount() > 1) {
		ExMessager em(Msg::ChAttrTitle);
		em.AddMultiline(Msg::ChAttrWarnNoOne);
		em.AddDup(Msg::Ok);
		em.Show(MSG_WARNING, 1);
		return false;
	}

	if (SrcPanel->GetMode() == PLUGIN_PANEL) {
		OpenPluginInfo Info;
		HANDLE hPlugin = SrcPanel->GetPluginHandle();

		if (hPlugin == INVALID_HANDLE_VALUE) {
			return false;
		}

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		if (!(Info.Flags & OPIF_REALNAMES)) {
			ExMessager em(Msg::ChAttrTitle);
			em.AddMultiline(Msg::ChAttrWarnNoRealFile);
			em.AddDup(Msg::Ok);
			em.Show(MSG_WARNING, 1);
			return false;
		}
	}

	FARString strSelName;
	DWORD FileAttr, FileMode;
	SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
	SrcPanel->GetSelName(&strSelName, FileAttr, FileMode);
	//SrcPanel->GetCurName(strSelName);

	if (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
		ExMessager em(Msg::ChAttrTitle);
		em.AddMultiline(Msg::ChAttrWarnNoSymlinks);
		em.AddDup(Msg::Ok);
		em.Show(MSG_WARNING, 1);
		return false;
	}

	if (TestParentFolderName(strSelName))
		return false;

	FSFileFlags FFFlags(strSelName.GetMB());
	if (FFFlags.Errno() != 0) {
		FARString str;
		str.Format(Msg::ChAttrErrorGetFlags.CPtr(), strSelName.CPtr());
		error_append(str, FFFlags.Errno());

		ExMessager em(Msg::ChAttrTitle);
		em.AddMultiline(str.CPtr());
		em.AddDup(Msg::Ok);
		em.Show(MSG_WARNING, 1);
		return false;
	}

	const int flags_list_y = ARRAYSIZE(flags_list);
	int flags_list_x = strSelName.GetLength(); // width by file name
	if (flags_list_x < flags_list_y) // width of next line with chars ___
		flags_list_x = flags_list_y;
	for (unsigned i=0; i<ARRAYSIZE(flags_list); i++) { // width of flags in list + 7=(" [ ] " + "  ")
		int tmp = strlen(flags_list[i].name_long) + 7;
		if (flags_list_x < tmp)
			flags_list_x = tmp;
	}
	int DlgWidth = (ScrX - 14 < flags_list_x) ? ScrX + 1 - 2 : flags_list_x + 10;
	int DlgHeight = (ScrY - 11 < flags_list_y) ? ScrY + 1 - 2 : flags_list_y + 9;
	DialogDataEx ChattrDlgData[] = {
		{DI_DOUBLEBOX, 3, 1, (short)(DlgWidth - 4), (short)(DlgHeight - 2), {}, DIF_SHOWAMPERSAND, Msg::ChAttrTitle},
		{DI_TEXT,      4, 2, (short)(DlgWidth - 5), 2, {}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, strSelName},
		{DI_TEXT,      4, 3, (short)(DlgWidth - 5), 3, {}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, L""},
		{DI_TEXT,      0, 4, 0, 4, {}, DIF_SEPARATOR, L""},
		{DI_LISTBOX,   4, 5, (short)(DlgWidth - 5), (short)(DlgHeight - 5), {}, DIF_FOCUS | DIF_LISTNOBOX /*| DIF_LISTNOCLOSE*/, L""},
		{DI_TEXT,      0, (short)(DlgHeight - 4), 0, (short)(DlgHeight - 4), {}, DIF_SEPARATOR, L""},
		{DI_BUTTON,    0, (short)(DlgHeight - 3), 0, (short)(DlgHeight - 3), {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::SetAttrSet},
		{DI_BUTTON,    0, (short)(DlgHeight - 3), 0, (short)(DlgHeight - 3), {}, DIF_CENTERGROUP, Msg::Cancel},
		{DI_BUTTON,    0, (short)(DlgHeight - 3), 0, (short)(DlgHeight - 3), {}, DIF_CENTERGROUP, Msg::Reset},
	};
	MakeDialogItemsEx(ChattrDlgData, ChattrDlg);

	TruncStr(ChattrDlg[CA_TEXT_FILENAME].strData, DlgWidth - 10);

	Dialog Dlg(ChattrDlg, ARRAYSIZE(ChattrDlg), ChattrDlgProc, (LONG_PTR)&FFFlags);
	//Dlg.SetHelp(L"Chattr");
	Dlg.SetPosition(-1, -1, DlgWidth, DlgHeight);

	Dlg.Process();

	switch (Dlg.GetExitCode()) {
		case CA_BUTTON_SET: {
			FFFlags.Apply(strSelName.GetMB());
			if (FFFlags.Errno() != 0) {
				FARString str;
				str.Format(Msg::ChAttrErrorSetFlags.CPtr(), strSelName.CPtr());
				error_append(str, FFFlags.Errno());

				ExMessager em(Msg::ChAttrTitle);
				em.AddMultiline(str.CPtr());
				em.AddDup(Msg::Ok);
				em.Show(MSG_WARNING, 1);
				return false;
			}
			return true;
		} break;
		default:
			return false;
	}
}
