/*
setattr.cpp

Установка атрибутов файлов
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

#include <set>			// for std::set
#include <pwd.h>		// for getpwent()
#include <grp.h>		// for getgrent()

#include "lang.hpp"
#include "dialog.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "datetime.hpp"
#include "fileattr.hpp"
#include "setattr.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "fileowner.hpp"
#include "wakeful.hpp"
#include "DlgGuid.hpp"
#include "execute.hpp"
#include "FSFileFlags.h"

#include "interf.hpp" // for BoxSymbols

struct FSFileFlagsSafe : FSFileFlags
{
	FSFileFlagsSafe(const FARString &path, DWORD attrs)
		: FSFileFlags((attrs & FILE_ATTRIBUTE_DEVICE) ? DEVNULL : path.GetMB())
	{ }
};

enum SETATTRDLG
{
	SA_DOUBLEBOX,
	SA_TEXT_LABEL,
	SA_TEXT_NAME,
	SA_SEPARATOR1,
	SA_TXTBTN_INFO,
	SA_TEXT_INFO_CHMARK,
	SA_EDIT_INFO,
	SA_SEPARATOR_OWNERSHIP,
	SA_TEXT_OWNER,
	SA_TEXT_OWNER_CHMARK,
	SA_COMBO_OWNER,
	SA_TEXT_GROUP,
	SA_TEXT_GROUP_CHMARK,
	SA_COMBO_GROUP,

	SA_SEPARATOR_MODE,
	SA_SEPARATOR_MODE_V1,
	SA_SEPARATOR_MODE_V2,
	SA_TEXT_MODE_USER,
	SA_TEXT_USER_READ_CHMARK,
	SA_CHECKBOX_USER_READ,
	SA_TEXT_USER_WRITE_CHMARK,
	SA_CHECKBOX_USER_WRITE,
	SA_TEXT_USER_EXECUTE_CHMARK,
	SA_CHECKBOX_USER_EXECUTE,
	SA_TEXT_MODE_GROUP,
	SA_TEXT_GROUP_READ_CHMARK,
	SA_CHECKBOX_GROUP_READ,
	SA_TEXT_GROUP_WRITE_CHMARK,
	SA_CHECKBOX_GROUP_WRITE,
	SA_TEXT_GROUP_EXECUTE_CHMARK,
	SA_CHECKBOX_GROUP_EXECUTE,
	SA_TEXT_MODE_OTHER,
	SA_TEXT_OTHER_READ_CHMARK,
	SA_CHECKBOX_OTHER_READ,
	SA_TEXT_OTHER_WRITE_CHMARK,
	SA_CHECKBOX_OTHER_WRITE,
	SA_TEXT_OTHER_EXECUTE_CHMARK,
	SA_CHECKBOX_OTHER_EXECUTE,
	SA_TEXT_SUID_CHMARK,
	SA_CHECKBOX_SUID,
	SA_TEXT_SGID_CHMARK,
	SA_CHECKBOX_SGID,
	SA_TEXT_STICKY_CHMARK,
	SA_CHECKBOX_STICKY,

	SA_TEXT_MODE_OCTAL,
	SA_TEXT_MODE_OCTAL_ORIGINAL,
	SA_TEXT_MODE_OCTAL_CHMARK,
	SA_FIXEDIT_MODE_OCTAL,
	SA_BUTTON_MODE_ORIGINAL,

	SA_SEPARATOR_ATTRIBUTES,
	SA_TEXT_IMMUTABLE_CHMARK,
	SA_CHECKBOX_IMMUTABLE,
	SA_TEXT_APPEND_CHMARK,
	SA_CHECKBOX_APPEND,
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	SA_TEXT_HIDDEN_CHMARK,
	SA_CHECKBOX_HIDDEN,
#endif

	SA_SEPARATOR3,
	SA_TEXT_TITLEDATE,
	SA_TEXT_LAST_ACCESS,
	SA_TEXT_LAST_ACCESS_DATE_CHMARK,
	SA_FIXEDIT_LAST_ACCESS_DATE,
	SA_TEXT_LAST_ACCESS_TIME_CHMARK,
	SA_FIXEDIT_LAST_ACCESS_TIME,
	SA_TEXT_LAST_MODIFICATION,
	SA_TEXT_LAST_MODIFICATION_DATE_CHMARK,
	SA_FIXEDIT_LAST_MODIFICATION_DATE,
	SA_TEXT_LAST_MODIFICATION_TIME_CHMARK,
	SA_FIXEDIT_LAST_MODIFICATION_TIME,
	SA_TEXT_LAST_CHANGE,
	SA_TEXT_LAST_CHANGE_DATE_CHMARK,
	SA_FIXEDIT_LAST_CHANGE_DATE,
	SA_TEXT_LAST_CHANGE_TIME_CHMARK,
	SA_FIXEDIT_LAST_CHANGE_TIME,
	SA_BUTTON_ORIGINAL,
	SA_BUTTON_CURRENT,
	SA_BUTTON_BLANK,
	SA_SEPARATOR4,
	SA_CHECKBOX_SUBFOLDERS,
	SA_SEPARATOR5,
	SC_SYMLINK_PROPERTIES_EXPLAIN_TEXT_1,
	SC_SYMLINK_PROPERTIES_EXPLAIN_TEXT_2,
	SA_BUTTON_SET,
	SA_BUTTON_CANCEL
};

static const struct MODEPAIR
{
	SETATTRDLG Item;
	mode_t Mode;
} AP[] = {
		{SA_CHECKBOX_USER_READ,     S_IRUSR},
		{SA_CHECKBOX_USER_WRITE,    S_IWUSR},
		{SA_CHECKBOX_USER_EXECUTE,  S_IXUSR},
		{SA_CHECKBOX_GROUP_READ,    S_IRGRP},
		{SA_CHECKBOX_GROUP_WRITE,   S_IWGRP},
		{SA_CHECKBOX_GROUP_EXECUTE, S_IXGRP},
		{SA_CHECKBOX_OTHER_READ,    S_IROTH},
		{SA_CHECKBOX_OTHER_WRITE,   S_IWOTH},
		{SA_CHECKBOX_OTHER_EXECUTE, S_IXOTH},
		{SA_CHECKBOX_SUID,          S_ISUID},
		{SA_CHECKBOX_SGID,          S_ISGID},
		{SA_CHECKBOX_STICKY,        S_ISVTX}
};

#define EDITABLE_MODES                                                                                         \
	(S_IXOTH | S_IWOTH | S_IROTH | S_IXGRP | S_IWGRP | S_IRGRP | S_IXUSR | S_IWUSR | S_IRUSR | S_ISUID         \
			| S_ISGID | S_ISVTX)

static const int PreserveOriginalIDs[] = {
		SA_CHECKBOX_USER_READ, SA_CHECKBOX_USER_WRITE, SA_CHECKBOX_USER_EXECUTE,
		SA_CHECKBOX_GROUP_READ, SA_CHECKBOX_GROUP_WRITE, SA_CHECKBOX_GROUP_EXECUTE,
		SA_CHECKBOX_OTHER_READ, SA_CHECKBOX_OTHER_WRITE, SA_CHECKBOX_OTHER_EXECUTE,
		SA_CHECKBOX_SUID, SA_CHECKBOX_SGID, SA_CHECKBOX_STICKY,
		SA_CHECKBOX_IMMUTABLE, SA_CHECKBOX_APPEND,
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		SA_CHECKBOX_HIDDEN,
#endif
};

enum DIALOGMODE
{
	MODE_FILE,
	MODE_FOLDER,
	MODE_MULTIPLE,
};

struct SetAttrDlgParam
{
	bool Plugin = false;
	DWORD FileSystemFlags = 0;
	DIALOGMODE DialogMode;
	FARString strSelName;
	FARString strOwner;
	FARString strGroup;
	bool OwnerChanged = false, GroupChanged = false;
	// значения CheckBox`ов на момент старта диалога
	int OriginalCBAttr[ARRAYSIZE(PreserveOriginalIDs)]; // values at dialog start and any user change for restore by subfolders checkbox
	int OriginalCBAttr0[ARRAYSIZE(PreserveOriginalIDs)]; // values at dialog start
	int OriginalCBAttr2[ARRAYSIZE(PreserveOriginalIDs)]; // -1 = original, other = was change
	DWORD OriginalCBFlag[ARRAYSIZE(PreserveOriginalIDs)]; // checkbox flags at dialog start
	bool _b_mode_check_or_edit_process = false;
	FARCHECKEDSTATE OSubfoldersState;
	bool OAccessTime, OModifyTime, OStatusChangeTime;
	unsigned char SymLinkInfoCycle = 0;
	FARString SymLink;
	FARString SymlinkButtonTitles[3];
	// initial values at dialog start:
	FARString strInitInfoSymLink,
		strInitOwner, strInitGroup,
		strInitOctal,
		strInitAccessDate, strInitModifyDate, strInitStatusChangeDate,
		strInitAccessTime, strInitModifyTime, strInitStatusChangeTime;
};

#define DM_SETATTR (DM_USER + 1)

static int DialogID2PreservedOriginalIndex(int id)
{
	for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
		if (PreserveOriginalIDs[i] == id)
			return i;
	}
	return -1;
}

inline static bool IsEditChanged(HANDLE hDlg, int EditControl, FARString &Original)
{
	return 0 != StrCmp(
		reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EditControl, 0)),
		Original);
}

static void BlankEditIfChanged(HANDLE hDlg, int EditControl, FARString &Remembered, bool &Changed)
{
	LPCWSTR Actual = reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, EditControl, 0));
	if (!Changed)
		Changed = StrCmp(Actual, Remembered) != 0;

	Remembered = Actual;

	if (!Changed)
		SendDlgMessage(hDlg, DM_SETTEXTPTR, EditControl, reinterpret_cast<LONG_PTR>(L""));
}

static std::wstring BriefInfo(const FARString &strSelName)
{
	std::vector<std::wstring> lines;

	std::string cmd = "file -- \"";
	cmd+= EscapeCmdStr(Wide2MB(strSelName.CPtr()));
	cmd+= '\"';

	std::wstring out;

	if (POpen(lines, cmd.c_str())) {
		for (const auto &line : lines) {
			out = line;
			size_t p = out.find(':');
			if (p != std::string::npos) {
				out.erase(0, p + 1);
			}
			StrTrim(out);
			if (p != std::string::npos && !out.empty()) {
				break;
			}
		}
	}
	return out;
}

inline void SetAttrDefaultMark(HANDLE hDlg, int id, bool nodefault)
{
	// id - id of DI_TEXT for non-default marker
	SendDlgMessage(hDlg, DM_SETTEXTPTR, id, // mark (un)changed
		nodefault ? reinterpret_cast<LONG_PTR>(L"*") : reinterpret_cast<LONG_PTR>(L"") );
}

static void SetAttrSetCheckAndDefaultMark(HANDLE hDlg, SetAttrDlgParam *DlgParam, int idx, int id, int bstate)
{
	// idx - index in arrays PreserveOriginalIDs and OriginalCBAttr0, if < 0 will be detected here
	// id - id of DI_CHECKBOX
	// id-1 - must be DI_TEXT for non-default marker
	SendDlgMessage(hDlg, DM_SETCHECK, id, bstate);

	if( idx < 0 )
		idx = DialogID2PreservedOriginalIndex(id);
	if (idx != -1) {
		if( DlgParam == nullptr )
			DlgParam = reinterpret_cast<SetAttrDlgParam *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
		SetAttrDefaultMark(hDlg, id-1, bstate != DlgParam->OriginalCBAttr0[idx]);
	}
}

static char SetAttrGetBitCharFromModeCheckBoxes(HANDLE hDlg, int _i1, int _i2, int _i3)
{
	int i1, i2, i3;

	i1 = (int)SendDlgMessage(hDlg, DM_GETCHECK, _i1, 0);
	i2 = (int)SendDlgMessage(hDlg, DM_GETCHECK, _i2, 0);
	i3 = (int)SendDlgMessage(hDlg, DM_GETCHECK, _i3, 0);
	if (i1 == BSTATE_3STATE || i2 == BSTATE_3STATE || i3 == BSTATE_3STATE)
		return '-';
	else {
		int i = (i1 == BSTATE_CHECKED ? 1 : 0) + (i2 == BSTATE_CHECKED ? 2 : 0) + (i3 == BSTATE_CHECKED ? 4 : 0);
		return '0' + i;
	}
}

static void SetAttrCalcBitsCharFromModeCheckBoxes(HANDLE hDlg)
{
	wchar_t str_octal[5] = {0};
	str_octal[0] = SetAttrGetBitCharFromModeCheckBoxes(hDlg, SA_CHECKBOX_STICKY, SA_CHECKBOX_SGID, SA_CHECKBOX_SUID);
	str_octal[1] = SetAttrGetBitCharFromModeCheckBoxes(hDlg, SA_CHECKBOX_USER_EXECUTE,  SA_CHECKBOX_USER_WRITE,  SA_CHECKBOX_USER_READ);
	str_octal[2] = SetAttrGetBitCharFromModeCheckBoxes(hDlg, SA_CHECKBOX_GROUP_EXECUTE, SA_CHECKBOX_GROUP_WRITE, SA_CHECKBOX_GROUP_READ);
	str_octal[3] = SetAttrGetBitCharFromModeCheckBoxes(hDlg, SA_CHECKBOX_OTHER_EXECUTE, SA_CHECKBOX_OTHER_WRITE, SA_CHECKBOX_OTHER_READ);

	SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_FIXEDIT_MODE_OCTAL, reinterpret_cast<LONG_PTR>(str_octal));
}

static void SetAttrGetModeCheckBoxesFromChar(HANDLE hDlg, SetAttrDlgParam *DlgParam, wchar_t c, int _i1, int _i2, int _i3)
{
	switch(c) {
		case L'0':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_UNCHECKED);
			break;
		case L'1':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_UNCHECKED);
			break;
		case L'2':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_UNCHECKED);
			break;
		case L'3':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_UNCHECKED);
			break;
		case L'4':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_CHECKED);
			break;
		case L'5':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_CHECKED);
			break;
		case L'6':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_UNCHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_CHECKED);
			break;
		case L'7':
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_CHECKED);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_CHECKED);
			break;
		default:
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i1, BSTATE_3STATE);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i2, BSTATE_3STATE);
			SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, -1, _i3, BSTATE_3STATE);
			break;
	}
}

LONG_PTR WINAPI SetAttrDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	SetAttrDlgParam *DlgParam =
			reinterpret_cast<SetAttrDlgParam *>(SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0));
	int OrigIdx;

	switch (Msg) {
			case DN_CLOSE:
			if (DlgParam->SymLinkInfoCycle == 0) {
				DlgParam->SymLink = reinterpret_cast<LPCWSTR>
					(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, SA_EDIT_INFO, 0));
			}
			break;

			case DN_BTNCLICK:
			OrigIdx = DialogID2PreservedOriginalIndex(Param1);
			if (OrigIdx != -1 || Param1 == SA_CHECKBOX_SUBFOLDERS) {
				if (OrigIdx != -1) {
					DlgParam->OriginalCBAttr[OrigIdx] = static_cast<int>(Param2);
					DlgParam->OriginalCBAttr2[OrigIdx] = 0;
					if( !DlgParam->_b_mode_check_or_edit_process) {
						DlgParam->_b_mode_check_or_edit_process = true;
						SetAttrCalcBitsCharFromModeCheckBoxes(hDlg);
						DlgParam->_b_mode_check_or_edit_process = false;
					}
					SetAttrDefaultMark(hDlg, Param1-1, Param2 != DlgParam->OriginalCBAttr0[OrigIdx]); // mark (un)changed
				}
				int FocusPos = static_cast<int>(SendDlgMessage(hDlg, DM_GETFOCUS, 0, 0));
				FARCHECKEDSTATE SubfoldersState = static_cast<FARCHECKEDSTATE>(
						SendDlgMessage(hDlg, DM_GETCHECK, SA_CHECKBOX_SUBFOLDERS, 0));

				{
					DlgParam->_b_mode_check_or_edit_process = true;
					// если снимаем атрибуты для SubFolders
					// этот кусок всегда работает если есть хотя бы одна папка
					// иначе SA_CHECKBOX_SUBFOLDERS недоступен и всегда снят.
					if (FocusPos == SA_CHECKBOX_SUBFOLDERS) {
						if (DlgParam->DialogMode == MODE_FOLDER)					// каталог однозначно!
						{
							if (DlgParam->OSubfoldersState != SubfoldersState)		// Состояние изменилось?
							{
								// установили?
								if (SubfoldersState != BSTATE_UNCHECKED) {
									for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
										SendDlgMessage(hDlg, DM_SET3STATE, PreserveOriginalIDs[i], TRUE);
										if (DlgParam->OriginalCBAttr2[i] == -1) {
											SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, i, PreserveOriginalIDs[i],
													BSTATE_3STATE);
										}
									}

									BlankEditIfChanged(hDlg, SA_COMBO_OWNER, DlgParam->strOwner,
											DlgParam->OwnerChanged);
									BlankEditIfChanged(hDlg, SA_COMBO_GROUP, DlgParam->strGroup,
											DlgParam->GroupChanged);
								}
								// сняли?
								else {
									for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
										SendDlgMessage(hDlg, DM_SET3STATE, PreserveOriginalIDs[i], FALSE);
										SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, i, PreserveOriginalIDs[i],
												DlgParam->OriginalCBAttr[i]);
									}
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_OWNER,
											reinterpret_cast<LONG_PTR>(DlgParam->strOwner.CPtr()));
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_GROUP,
											reinterpret_cast<LONG_PTR>(DlgParam->strGroup.CPtr()));
								}

								if (Opt.SetAttrFolderRules) {
									FAR_FIND_DATA_EX FindData;

									if (apiGetFindDataEx(DlgParam->strSelName, FindData)) {
										const SETATTRDLG Items[] = {SA_TEXT_LAST_ACCESS,
												SA_TEXT_LAST_MODIFICATION, SA_TEXT_LAST_CHANGE};
										bool *ParamTimes[] = {&DlgParam->OAccessTime, &DlgParam->OModifyTime,
												&DlgParam->OStatusChangeTime};
										const PFILETIME FDTimes[] = {&FindData.ftUnixAccessTime,
												&FindData.ftUnixModificationTime,
												&FindData.ftUnixStatusChangeTime};

										for (size_t i = 0; i < ARRAYSIZE(Items); i++) {
											if (!*ParamTimes[i]) {
												SendDlgMessage(hDlg, DM_SETATTR, Items[i],
														(SubfoldersState != BSTATE_UNCHECKED)
																? 0
																: (LONG_PTR)FDTimes[i]);
												*ParamTimes[i] = false;
											}
										}
									}
								}
							}
						}
						// много объектов
						else {
							// Состояние изменилось?
							if (DlgParam->OSubfoldersState != SubfoldersState) {
								// установили?
								if (SubfoldersState != BSTATE_UNCHECKED) {
									for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
										if (DlgParam->OriginalCBAttr2[i] == -1) {
											SendDlgMessage(hDlg, DM_SET3STATE, PreserveOriginalIDs[i], TRUE);
											SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, i, PreserveOriginalIDs[i],
													BSTATE_3STATE);
										}
									}
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_OWNER,
											reinterpret_cast<LONG_PTR>(L""));
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_GROUP,
											reinterpret_cast<LONG_PTR>(L""));
								}
								// сняли?
								else {
									for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
										SendDlgMessage(hDlg, DM_SET3STATE, PreserveOriginalIDs[i],
												((DlgParam->OriginalCBFlag[i] & DIF_3STATE) ? TRUE : FALSE));
										SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, i, PreserveOriginalIDs[i],
												DlgParam->OriginalCBAttr[i]);
									}
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_OWNER,
											reinterpret_cast<LONG_PTR>(DlgParam->strOwner.CPtr()));
									SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_COMBO_GROUP,
											reinterpret_cast<LONG_PTR>(DlgParam->strGroup.CPtr()));
								}
							}
						}

						DlgParam->OSubfoldersState = SubfoldersState;
					}
					SetAttrCalcBitsCharFromModeCheckBoxes(hDlg);
					DlgParam->_b_mode_check_or_edit_process = false;
				}

				return TRUE;
			}
			else if (Param1 == SA_TXTBTN_INFO) {
				FARString strText;
				SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_TXTBTN_INFO,
					reinterpret_cast<LONG_PTR>(DlgParam->SymlinkButtonTitles[DlgParam->SymLinkInfoCycle].CPtr()));

				switch (DlgParam->SymLinkInfoCycle++) {
					case 0: {
							DlgParam->SymLink = reinterpret_cast<LPCWSTR>
								(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, SA_EDIT_INFO, 0));
							ConvertNameToReal(DlgParam->SymLink, strText);
							SendDlgMessage(hDlg, DM_SETREADONLY, SA_EDIT_INFO, 1);
						} break;

					case 1:
						ConvertNameToReal(DlgParam->SymLink, strText);
						strText = BriefInfo(strText);
						SendDlgMessage(hDlg, DM_SETREADONLY, SA_EDIT_INFO, 1);
						break;

					default:
						strText = DlgParam->SymLink;
						SendDlgMessage(hDlg, DM_SETREADONLY, SA_EDIT_INFO, 0);
						DlgParam->SymLinkInfoCycle = 0;
				}
				SendDlgMessage(hDlg, DM_SETTEXTPTR, SA_EDIT_INFO, reinterpret_cast<LONG_PTR>(strText.CPtr()));
				return TRUE;

			// Set Original for Modes and Attributes
			} else if (Param1 == SA_BUTTON_MODE_ORIGINAL) {
				DlgParam->_b_mode_check_or_edit_process = true;
				for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
					DlgParam->OriginalCBAttr2[i] = -1;
					SendDlgMessage(hDlg, DM_SET3STATE, PreserveOriginalIDs[i],
							((DlgParam->OriginalCBFlag[i] & DIF_3STATE) ? TRUE : FALSE));
					SetAttrSetCheckAndDefaultMark(hDlg, DlgParam, i, PreserveOriginalIDs[i],
							DlgParam->OriginalCBAttr0[i]);
				}
				SetAttrCalcBitsCharFromModeCheckBoxes(hDlg);
				DlgParam->_b_mode_check_or_edit_process = false;
				SendDlgMessage(hDlg, DM_SETFOCUS, SA_FIXEDIT_MODE_OCTAL, 0);

			// Set Original? / Set All? / Clear All?
			} else if (Param1 == SA_BUTTON_ORIGINAL) {
				FAR_FIND_DATA_EX FindData;

				if (apiGetFindDataEx(DlgParam->strSelName, FindData)) {
					SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_ACCESS,
							(LONG_PTR)&FindData.ftUnixAccessTime);
					SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_MODIFICATION,
							(LONG_PTR)&FindData.ftUnixModificationTime);
					SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_CHANGE,
							(LONG_PTR)&FindData.ftUnixStatusChangeTime);
					DlgParam->OAccessTime = DlgParam->OModifyTime = DlgParam->OStatusChangeTime = false;
				}

				SendDlgMessage(hDlg, DM_SETFOCUS, SA_FIXEDIT_LAST_ACCESS_DATE, 0);
				return TRUE;
			} else if (Param1 == SA_BUTTON_CURRENT || Param1 == SA_BUTTON_BLANK) {
				LONG_PTR Value = 0;
				FILETIME CurrentTime;
				if (Param1 == SA_BUTTON_CURRENT) {
					WINPORT(GetSystemTimeAsFileTime)(&CurrentTime);
					Value = reinterpret_cast<LONG_PTR>(&CurrentTime);
				}
				SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_ACCESS, Value);
				SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_MODIFICATION, Value);
				// SendDlgMessage(hDlg, DM_SETATTR, SA_TEXT_LAST_CHANGE, Value);
				DlgParam->OAccessTime = DlgParam->OModifyTime = DlgParam->OStatusChangeTime = true;
				SendDlgMessage(hDlg, DM_SETFOCUS, SA_FIXEDIT_LAST_ACCESS_DATE, 0);
				return TRUE;
			}

			break;

		case DN_MOUSECLICK: {
			//_SVS(SysLog(L"Msg=DN_MOUSECLICK Param1=%d Param2=%d",Param1,Param2));
			if (Param1 >= SA_TEXT_LAST_ACCESS && Param1 <= SA_FIXEDIT_LAST_MODIFICATION_TIME/*SA_FIXEDIT_LAST_CHANGE_TIME*/) {
				if (reinterpret_cast<MOUSE_EVENT_RECORD *>(Param2)->dwEventFlags == DOUBLE_CLICK) {
					// Дадим Менеджеру диалогов "попотеть"
					DefDlgProc(hDlg, Msg, Param1, Param2);
					SendDlgMessage(hDlg, DM_SETATTR, Param1, -1);
				}

				if (Param1 == SA_TEXT_LAST_ACCESS || Param1 == SA_TEXT_LAST_MODIFICATION
						/*|| Param1 == SA_TEXT_LAST_CHANGE*/) {
					Param1++;
				}

				SendDlgMessage(hDlg, DM_SETFOCUS, Param1, 0);
			}
		} break;

		case DN_EDITCHANGE: {
			switch (Param1) {
				case SA_FIXEDIT_MODE_OCTAL:
					if (!DlgParam->_b_mode_check_or_edit_process) {
						DlgParam->_b_mode_check_or_edit_process = true;
						int length = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, SA_FIXEDIT_MODE_OCTAL, 0);
						LPCWSTR str_octal = reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, SA_FIXEDIT_MODE_OCTAL, 0));
						if (length>0 && str_octal!=nullptr) {
							SetAttrGetModeCheckBoxesFromChar(hDlg, DlgParam, length<1 ? ' ' : str_octal[0], SA_CHECKBOX_STICKY, SA_CHECKBOX_SGID, SA_CHECKBOX_SUID);
							SetAttrGetModeCheckBoxesFromChar(hDlg, DlgParam, length<2 ? ' ' : str_octal[1], SA_CHECKBOX_USER_EXECUTE,  SA_CHECKBOX_USER_WRITE,  SA_CHECKBOX_USER_READ);
							SetAttrGetModeCheckBoxesFromChar(hDlg, DlgParam, length<3 ? ' ' : str_octal[2], SA_CHECKBOX_GROUP_EXECUTE, SA_CHECKBOX_GROUP_WRITE, SA_CHECKBOX_GROUP_READ);
							SetAttrGetModeCheckBoxesFromChar(hDlg, DlgParam, length<4 ? ' ' : str_octal[3], SA_CHECKBOX_OTHER_EXECUTE, SA_CHECKBOX_OTHER_WRITE, SA_CHECKBOX_OTHER_READ);
						}
						DlgParam->_b_mode_check_or_edit_process = false;
					}
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_MODE_OCTAL-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_MODE_OCTAL, DlgParam->strInitOctal) );
					break;
				case SA_EDIT_INFO:
					SetAttrDefaultMark(hDlg, SA_EDIT_INFO-1, // mark (un)changed
						DlgParam->SymLinkInfoCycle
							? StrCmp(DlgParam->SymLink, DlgParam->strInitInfoSymLink)
							: IsEditChanged(hDlg, SA_EDIT_INFO, DlgParam->strInitInfoSymLink) );
				case SA_COMBO_OWNER:
					SetAttrDefaultMark(hDlg, SA_COMBO_OWNER-1, // mark (un)changed
						IsEditChanged(hDlg, SA_COMBO_OWNER, DlgParam->strInitOwner) );
					break;
				case SA_COMBO_GROUP:
					SetAttrDefaultMark(hDlg, SA_COMBO_GROUP-1, // mark (un)changed
						IsEditChanged(hDlg, SA_COMBO_GROUP, DlgParam->strInitOwner) );
					break;
				case SA_FIXEDIT_LAST_ACCESS_DATE:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_ACCESS_DATE-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_ACCESS_DATE, DlgParam->strInitAccessDate) );
					DlgParam->OAccessTime = true;
					break;
				case SA_FIXEDIT_LAST_ACCESS_TIME:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_ACCESS_TIME-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_ACCESS_TIME, DlgParam->strInitAccessTime) );
					DlgParam->OAccessTime = true;
					break;
				case SA_FIXEDIT_LAST_MODIFICATION_DATE:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_MODIFICATION_DATE-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_MODIFICATION_DATE, DlgParam->strInitModifyDate) );
					DlgParam->OModifyTime = true;
					break;
				case SA_FIXEDIT_LAST_MODIFICATION_TIME:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_MODIFICATION_TIME-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_MODIFICATION_TIME, DlgParam->strInitModifyTime) );
					DlgParam->OModifyTime = true;
					break;
				case SA_FIXEDIT_LAST_CHANGE_DATE:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_CHANGE_DATE-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_CHANGE_DATE, DlgParam->strInitStatusChangeDate) );
					DlgParam->OStatusChangeTime = true;
					break;
				case SA_FIXEDIT_LAST_CHANGE_TIME:
					SetAttrDefaultMark(hDlg, SA_FIXEDIT_LAST_CHANGE_TIME-1, // mark (un)changed
						IsEditChanged(hDlg, SA_FIXEDIT_LAST_CHANGE_TIME, DlgParam->strInitStatusChangeTime) );
					DlgParam->OStatusChangeTime = true;
					break;
			}

			break;
		}

		case DN_GOTFOCUS: {
			if (Param1 == SA_FIXEDIT_LAST_ACCESS_DATE || Param1 == SA_FIXEDIT_LAST_MODIFICATION_DATE
					|| Param1 == SA_FIXEDIT_LAST_CHANGE_DATE) {
				if (GetDateFormat() == 2) {
					if (reinterpret_cast<LPCWSTR>(SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, Param1, 0))[0]
							== L' ') {
						COORD Pos;
						SendDlgMessage(hDlg, DM_GETCURSORPOS, Param1, (LONG_PTR)&Pos);
						if (Pos.X == 0) {
							Pos.X = 1;
							SendDlgMessage(hDlg, DM_SETCURSORPOS, Param1, (LONG_PTR)&Pos);
						}
					}
				}
			}
		} break;

		case DM_SETATTR: {
			FARString strDate, strTime;

			if (Param2)		// Set?
			{
				FILETIME ft;

				if (Param2 == -1) {
					WINPORT(GetSystemTimeAsFileTime)(&ft);
				} else {
					ft = *reinterpret_cast<PFILETIME>(Param2);
				}

				ConvertDate(ft, strDate, strTime, 12, FALSE, FALSE, 2, TRUE);
			}

			// Глянем на место, где был клик
			int Set1 = -1;
			int Set2 = Param1;

			switch (Param1) {
				case SA_TEXT_LAST_ACCESS:
					Set1 = SA_FIXEDIT_LAST_ACCESS_DATE;
					Set2 = SA_FIXEDIT_LAST_ACCESS_TIME;
					DlgParam->OAccessTime = true;
					break;
				case SA_TEXT_LAST_MODIFICATION:
					Set1 = SA_FIXEDIT_LAST_MODIFICATION_DATE;
					Set2 = SA_FIXEDIT_LAST_MODIFICATION_TIME;
					DlgParam->OModifyTime = true;
					break;
				case SA_TEXT_LAST_CHANGE:
					Set1 = SA_FIXEDIT_LAST_CHANGE_DATE;
					Set2 = SA_FIXEDIT_LAST_CHANGE_TIME;
					DlgParam->OStatusChangeTime = true;
					break;

				case SA_FIXEDIT_LAST_ACCESS_DATE:
				case SA_FIXEDIT_LAST_MODIFICATION_DATE:
				case SA_FIXEDIT_LAST_CHANGE_DATE:
					Set1 = Param1;
					Set2 = -1;
					break;
			}

			if (Set1 != -1) {
				SendDlgMessage(hDlg, DM_SETTEXTPTR, Set1, (LONG_PTR)strDate.CPtr());
			}

			if (Set2 != -1) {
				SendDlgMessage(hDlg, DM_SETTEXTPTR, Set2, (LONG_PTR)strTime.CPtr());
			}

			return TRUE;
		}
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

void ShellSetFileAttributesMsg(const wchar_t *Name)
{
	static int Width = 54;
	int WidthTemp;

	if (Name && *Name)
		WidthTemp = Max(StrLength(Name), 54);
	else
		Width = WidthTemp = 54;

	WidthTemp = Min(WidthTemp, WidthNameForMessage);
	Width = Max(Width, WidthTemp);
	FARString strOutFileName = Name;
	TruncPathStr(strOutFileName, Width);
	CenterStr(strOutFileName, strOutFileName, Width + 4);
	Message(0, 0, Msg::SetAttrTitle, Msg::SetAttrSetting, strOutFileName);
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = Name;
	PreRedraw.SetParam(preRedrawItem.Param);
}

bool ReadFileTime(int Type, const wchar_t *Name, FILETIME &FileTime, const wchar_t *OSrcDate,
		const wchar_t *OSrcTime)
{
	bool Result = false;
	FAR_FIND_DATA_EX ffd = {};

	if (apiGetFindDataEx(Name, ffd)) {
		LPFILETIME Times[] = {&ffd.ftLastWriteTime, &ffd.ftCreationTime, &ffd.ftLastAccessTime,
				&ffd.ftChangeTime};
		LPFILETIME OriginalFileTime = Times[Type];
		FILETIME oft = {};
		if (WINPORT(FileTimeToLocalFileTime)(OriginalFileTime, &oft)) {
			SYSTEMTIME ost = {};
			if (WINPORT(FileTimeToSystemTime)(&oft, &ost)) {
				WORD DateN[3] = {};
				GetFileDateAndTime(OSrcDate, DateN, ARRAYSIZE(DateN), GetDateSeparator());
				WORD TimeN[4] = {};
				GetFileDateAndTime(OSrcTime, TimeN, ARRAYSIZE(TimeN), GetTimeSeparator());
				SYSTEMTIME st = {};

				switch (GetDateFormat()) {
					case 0:
						st.wMonth = DateN[0] != (WORD)-1 ? DateN[0] : ost.wMonth;
						st.wDay = DateN[1] != (WORD)-1 ? DateN[1] : ost.wDay;
						st.wYear = DateN[2] != (WORD)-1 ? DateN[2] : ost.wYear;
						break;
					case 1:
						st.wDay = DateN[0] != (WORD)-1 ? DateN[0] : ost.wDay;
						st.wMonth = DateN[1] != (WORD)-1 ? DateN[1] : ost.wMonth;
						st.wYear = DateN[2] != (WORD)-1 ? DateN[2] : ost.wYear;
						break;
					default:
						st.wYear = DateN[0] != (WORD)-1 ? DateN[0] : ost.wYear;
						st.wMonth = DateN[1] != (WORD)-1 ? DateN[1] : ost.wMonth;
						st.wDay = DateN[2] != (WORD)-1 ? DateN[2] : ost.wDay;
						break;
				}

				st.wHour = TimeN[0] != (WORD)-1 ? (TimeN[0]) : ost.wHour;
				st.wMinute = TimeN[1] != (WORD)-1 ? (TimeN[1]) : ost.wMinute;
				st.wSecond = TimeN[2] != (WORD)-1 ? (TimeN[2]) : ost.wSecond;
				st.wMilliseconds = TimeN[3] != (WORD)-1 ? (TimeN[3]) : ost.wMilliseconds;

				if (st.wYear < 100) {
					st.wYear = static_cast<WORD>(ConvertYearToFull(st.wYear));
				}

				FILETIME lft = {};
				if (WINPORT(SystemTimeToFileTime)(&st, &lft)) {
					if (WINPORT(LocalFileTimeToFileTime)(&lft, &FileTime)) {
						Result = WINPORT(CompareFileTime)(&FileTime, OriginalFileTime) != 0;
					}
				}
			}
		}
	}
	return Result;
}

void PR_ShellSetFileAttributesMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	ShellSetFileAttributesMsg(reinterpret_cast<const wchar_t *>(preRedrawItem.Param.Param1));
}

static void CheckFileOwnerGroup(DialogItemEx &ComboItem,
		bool(WINAPI *GetFN)(const wchar_t *, const wchar_t *, FARString &),
		FARString strComputerName,
		FARString strSelName)
{
	FARString strCur;
	GetFN(strComputerName, strSelName, strCur);
	if (ComboItem.strData.IsEmpty()) {
		ComboItem.strData = strCur;
	}
	else if (ComboItem.strData != strCur) {
		ComboItem.strData = Msg::SetAttrOwnerMultiple;
	}
}

static bool ApplyFileOwnerGroupIfChanged(DialogItemEx &ComboItem,
		int (*ESetFN)(LPCWSTR Name, LPCWSTR Owner, int SkipMode),
		int &SkipMode, const FARString &strSelName, const FARString &strInit, bool force = false)
{
	if (!ComboItem.strData.IsEmpty() && (force || StrCmp(strInit, ComboItem.strData))) {
		int Result = ESetFN(strSelName, ComboItem.strData, SkipMode);
		if (Result == SETATTR_RET_SKIPALL) {
			SkipMode = SETATTR_RET_SKIP;
		}
		else if (Result == SETATTR_RET_ERROR) {
			return false;
		}
	}
	return true;
}

static void ApplyFSFileFlags(DialogItemEx *AttrDlg, const FARString &strSelName, DWORD FileAttr)
{
	FSFileFlagsSafe FFFlags(strSelName.GetMB(), FileAttr);
	if (AttrDlg[SA_CHECKBOX_IMMUTABLE].Selected == BSTATE_CHECKED
			|| AttrDlg[SA_CHECKBOX_IMMUTABLE].Selected == BSTATE_UNCHECKED) {
		FFFlags.SetImmutable(AttrDlg[SA_CHECKBOX_IMMUTABLE].Selected != BSTATE_UNCHECKED);
	}
	if (AttrDlg[SA_CHECKBOX_APPEND].Selected == BSTATE_CHECKED
			|| AttrDlg[SA_CHECKBOX_APPEND].Selected == BSTATE_UNCHECKED) {
		FFFlags.SetAppend(AttrDlg[SA_CHECKBOX_APPEND].Selected != BSTATE_UNCHECKED);
	}
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	if (AttrDlg[SA_CHECKBOX_HIDDEN].Selected == BSTATE_CHECKED
			|| AttrDlg[SA_CHECKBOX_HIDDEN].Selected == BSTATE_UNCHECKED) {
		FFFlags.SetHidden(AttrDlg[SA_CHECKBOX_HIDDEN].Selected != BSTATE_UNCHECKED);
	}
#endif
	FFFlags.Apply(strSelName.GetMB());
}

class ListPwGrEnt {
	std::set<FARString> Set; // sorts and prevents duplicates
	std::vector<FarListItem> Items;
	FarList List;
	void Append(const wchar_t *s) { Set.emplace(s); }
	void Append(const char *s) { Set.emplace(s); }
public:
	ListPwGrEnt(bool bGroups, int SelCount);
	FarList *GetFarList() {
		return &List;
	}
};

ListPwGrEnt::ListPwGrEnt(bool bGroups, int SelCount)
{
	if (SelCount >= 2)
		Append(Msg::SetAttrOwnerMultiple);

	if (!bGroups) { // usernames
		struct passwd *pw;
		setpwent();
		while ((pw = getpwent()) != NULL) {
			Append(pw->pw_name);
		}
		endpwent();
	}
	else { // groups
		struct group *gr;
		setgrent();
		while ((gr = getgrent()) != NULL) {
			Append(gr->gr_name);
		}
		endgrent();
	}

	Items.reserve(Set.size());
	for (const auto &Str: Set) {
		Items.emplace_back();
		Items.back().Flags = 0;
		Items.back().Text = Str.CPtr();
	}

	List.ItemsNumber = Items.size();
	List.Items = Items.data();
}

bool ShellSetFileAttributes(Panel *SrcPanel, LPCWSTR Object)
{
	std::vector<FARString> SelectedNames;

	SudoClientRegion scr;

	ChangePriority ChPriority(ChangePriority::NORMAL);
	short DlgX = 70, DlgY = 25;

	int SelCount = SrcPanel ? SrcPanel->GetSelCount() : 1;

	if (!SelCount) {
		return false;
	}

	const short ColX1Of3 = 5;
	const short ColX2Of3 = ColX1Of3 + (DlgX - 3) / 3;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	const short ColX3Of3 = ColX2Of3 + (DlgX - 3) / 3;
#endif
	static const wchar_t VerticalLine[] = {BoxSymbols[BS_V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1], 0};

	DialogDataEx AttrDlgData[] = {
		{DI_DOUBLEBOX, 3,                   1,               short(DlgX - 4),  short(DlgY - 2), {}, 0, Msg::SetAttrTitle},
		{DI_TEXT,      -1,                  2,               0,                2,               {}, 0, Msg::SetAttrFor},
		{DI_TEXT,      -1,                  3,               0,                3,               {}, DIF_SHOWAMPERSAND, L""},
		{DI_TEXT,      3,                   4,               0,                4,               {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      5,                   5,               17,               5,               {}, DIF_FOCUS, Msg::SetAttrBriefInfo}, // if symlink in will Button & need first focus here
		{DI_TEXT,      18,                  5,               18,               5,               {}, 0, L""},
		{DI_EDIT,      19,                  5,               short(DlgX - 6),  5,               {}, DIF_SELECTONENTRY | DIF_FOCUS | DIF_READONLY, L""}, // not readonly only if symlink
		{DI_TEXT,      3,                   6,               0,                6,               {}, DIF_SEPARATOR, Msg::SetAttrOwnerTitle},
		{DI_TEXT,      5,                   7,               17,               7,               {}, 0, Msg::SetAttrOwner},
		{DI_TEXT,      18,                  7,               18,               7,               {}, 0, L""},
		{DI_COMBOBOX,  19,                  7,               short(DlgX-6),    7,               {}, DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND|DIF_LISTWRAPMODE,L""},
		{DI_TEXT,      5,                   8,               17,               8,               {}, 0, Msg::SetAttrGroup},
		{DI_TEXT,      18,                  8,               18,               8,               {}, 0, L""},
		{DI_COMBOBOX,  19,                  8,               short(DlgX-6),    8,               {}, DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND|DIF_LISTWRAPMODE,L""},

		{DI_TEXT,      3,                   9,               0,                9,               {}, DIF_SEPARATOR, Msg::SetAttrModeTitle},
		{DI_VTEXT,     39,                  10,              39,               12,              {}, DIF_BOXCOLOR, VerticalLine},
		{DI_VTEXT,     52,                  10,              52,               12,              {}, DIF_BOXCOLOR, VerticalLine},

		{DI_TEXT,      5,                   10,              17,               10,              {}, 0, Msg::SetAttrAccessUser},
		{DI_TEXT,      18,                  10,              18,               10,              {}, 0, L""},
		{DI_CHECKBOX,  19,                  10,              23,               10,              {}, DIF_3STATE, Msg::SetAttrAccessUserRead},
		{DI_TEXT,      25,                  10,              25,               10,              {}, 0, L""},
		{DI_CHECKBOX,  26,                  10,              30,               10,              {}, DIF_3STATE, Msg::SetAttrAccessUserWrite},
		{DI_TEXT,      32,                  10,              32,               10,              {}, 0, L""},
		{DI_CHECKBOX,  33,                  10,              37,               10,              {}, DIF_3STATE, Msg::SetAttrAccessUserExecute},
		{DI_TEXT,      5,                   11,              18,               11,              {}, 0, Msg::SetAttrAccessGroup},
		{DI_TEXT,      18,                  11,              18,               11,              {}, 0, L""},
		{DI_CHECKBOX,  19,                  11,              23,               11,              {}, DIF_3STATE, Msg::SetAttrAccessGroupRead},
		{DI_TEXT,      25,                  11,              25,               11,              {}, 0, L""},
		{DI_CHECKBOX,  26,                  11,              30,               11,              {}, DIF_3STATE, Msg::SetAttrAccessGroupWrite},
		{DI_TEXT,      32,                  11,              32,               11,              {}, 0, L""},
		{DI_CHECKBOX,  33,                  11,              37,               11,              {}, DIF_3STATE, Msg::SetAttrAccessGroupExecute},
		{DI_TEXT,      5,                   12,              18,               12,              {}, 0, Msg::SetAttrAccessOther},
		{DI_TEXT,      18,                  12,              18,               12,              {}, 0, L""},
		{DI_CHECKBOX,  19,                  12,              23,               12,              {}, DIF_3STATE, Msg::SetAttrAccessOtherRead},
		{DI_TEXT,      25,                  12,              25,               12,              {}, 0, L""},
		{DI_CHECKBOX,  26,                  12,              30,               12,              {}, DIF_3STATE, Msg::SetAttrAccessOtherWrite},
		{DI_TEXT,      32,                  12,              32,               12,              {}, 0, L""},
		{DI_CHECKBOX,  33,                  12,              37,               12,              {}, DIF_3STATE, Msg::SetAttrAccessOtherExecute},

		{DI_TEXT,      40,                  10,              40,               10,              {}, 0, L""},
		{DI_CHECKBOX,  41,                  10,              53,               10,              {}, DIF_3STATE, Msg::SetAttrSUID},
		{DI_TEXT,      40,                  11,              40,               11,              {}, 0, L""},
		{DI_CHECKBOX,  41,                  11,              53,               11,              {}, DIF_3STATE, Msg::SetAttrSGID},
		{DI_TEXT,      40,                  12,              40,               12,              {}, 0, L""},
		{DI_CHECKBOX,  41,                  12,              53,               12,              {}, DIF_3STATE, Msg::SetAttrSticky},

		{DI_TEXT,      53,                  10,              63,               10,              {}, 0, L"O&ctal: SUGO"},
		{DI_TEXT,      54,                  11,              57,               11,              {}, 0, L""},
		{DI_TEXT,      59,                  11,              59,               11,              {}, 0, L""},
		{DI_FIXEDIT,   60,                  11,              63,               11,              {(DWORD_PTR)L"####"}, DIF_MASKEDIT, L""},
		{DI_BUTTON,    53,                  12,              63,               12,              {}, DIF_BTNNOCLOSE, Msg::SetAttrModeOriginal},

		{DI_TEXT,      3,                   13,              0,                13,              {}, DIF_SEPARATOR, Msg::SetAttrAttributesTitle},
		{DI_TEXT,      ColX1Of3,            14,              ColX1Of3,         14,              {}, 0, L""},
		{DI_CHECKBOX,  short(ColX1Of3+1),   14,              0,                14,              {}, DIF_FOCUS | DIF_3STATE, Msg::SetAttrImmutable},
		{DI_TEXT,      ColX2Of3,            14,              ColX2Of3,         14,              {}, 0, L""},
		{DI_CHECKBOX,  short(ColX2Of3+1),   14,              0,                14,              {}, DIF_3STATE, Msg::SetAttrAppend},
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		{DI_TEXT,      ColX3Of3,            14,              ColX3Of3,         14,              {}, 0, L""},
		{DI_CHECKBOX,  short(ColX3Of3+1),   14,              0,                14,              {}, DIF_3STATE, Msg::SetAttrHidden},
#endif

		{DI_TEXT,      3,                   15,              0,                15,              {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      short(DlgX - 29),    16,              0,                16,              {}, 0, L""},
		{DI_TEXT,      5,                   17,              0,                17,              {}, 0, Msg::SetAttrAccessTime},
		{DI_TEXT,      short(DlgX - 30),    17,              short(DlgX - 30), 17,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 29),    17,              short(DlgX - 19), 17,              {}, DIF_MASKEDIT, L""},
		{DI_TEXT,      short(DlgX - 18),    17,              short(DlgX - 18), 17,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 17),    17,              short(DlgX - 6),  17,              {}, DIF_MASKEDIT, L""},
		{DI_TEXT,      5,                   18,              0,                18,              {}, 0, Msg::SetAttrModificationTime},
		{DI_TEXT,      short(DlgX - 30),    18,              short(DlgX - 30), 18,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 29),    18,              short(DlgX - 19), 18,              {}, DIF_MASKEDIT, L""},
		{DI_TEXT,      short(DlgX - 18),    18,              short(DlgX - 18), 18,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 17),    18,              short(DlgX - 6),  18,              {}, DIF_MASKEDIT, L""},
		{DI_TEXT,      5,                   19,              0,                19,              {}, 0, Msg::SetAttrStatusChangeTime},
		{DI_TEXT,      short(DlgX - 30),    19,              short(DlgX - 30), 19,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 29),    19,              short(DlgX - 19), 19,              {}, DIF_MASKEDIT | DIF_READONLY, L""},
		{DI_TEXT,      short(DlgX - 18),    19,              short(DlgX - 18), 19,              {}, 0, L""},
		{DI_FIXEDIT,   short(DlgX - 17),    19,              short(DlgX - 6),  19,              {}, DIF_MASKEDIT | DIF_READONLY, L""},
		{DI_BUTTON,    0,                   20,              0,                20,              {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::SetAttrOriginal},
		{DI_BUTTON,    0,                   20,              0,                20,              {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::SetAttrCurrent},
		{DI_BUTTON,    0,                   20,              0,                20,              {}, DIF_CENTERGROUP | DIF_BTNNOCLOSE, Msg::SetAttrBlank},
		{DI_TEXT,      3,                   21,              0,                21,              {}, DIF_SEPARATOR | DIF_HIDDEN, L""},
		{DI_CHECKBOX,  5,                   22,              0,                22,              {}, DIF_DISABLE | DIF_HIDDEN, Msg::SetAttrSubfolders},
		{DI_TEXT,      3,                   short(DlgY - 4), 0,                short(DlgY - 4), {}, DIF_SEPARATOR, L""},
		{DI_TEXT,      5,                   short(DlgY - 3), 0,                short(DlgY - 3), {}, DIF_DISABLE | DIF_HIDDEN, Msg::SetAttrSymlinkExplain1},
		{DI_TEXT,      5,                   short(DlgY - 2), 0,                short(DlgY - 2), {}, DIF_DISABLE | DIF_HIDDEN, Msg::SetAttrSymlinkExplain2},
		{DI_BUTTON,    0,                   short(DlgY - 3), 0,                short(DlgY - 3), {}, DIF_DEFAULT | DIF_CENTERGROUP, Msg::SetAttrSet},
		{DI_BUTTON,    0,                   short(DlgY - 3), 0,                short(DlgY - 3), {}, DIF_CENTERGROUP,  Msg::Cancel}
	};
	MakeDialogItemsEx(AttrDlgData, AttrDlg);
	SetAttrDlgParam DlgParam{};

	ListPwGrEnt Owners(false, SelCount);
	ListPwGrEnt Groups(true, SelCount);
	AttrDlg[SA_COMBO_OWNER].ListItems = Owners.GetFarList();
	AttrDlg[SA_COMBO_GROUP].ListItems = Groups.GetFarList();

	if (SrcPanel && SrcPanel->GetMode() == PLUGIN_PANEL) {
		OpenPluginInfo Info;
		HANDLE hPlugin = SrcPanel->GetPluginHandle();

		if (hPlugin == INVALID_HANDLE_VALUE) {
			return false;
		}

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		if (!(Info.Flags & OPIF_REALNAMES)) {
			AttrDlg[SA_BUTTON_SET].Flags|= DIF_DISABLE;
			//AttrDlg[SA_BUTTON_BRIEFINFO].Flags|= DIF_DISABLE;
			DlgParam.Plugin = true;
		}
	}

	{
		DWORD FileAttr = INVALID_FILE_ATTRIBUTES, FileMode = 0;
		FARString strSelName;
		FAR_FIND_DATA_EX FindData;
		if (SrcPanel) {
			SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
			SrcPanel->GetSelName(&strSelName, FileAttr, FileMode, &FindData);
			/*			if (!FileMode) {
							FAR_FIND_DATA_EX FindData2;
							if (apiGetFindDataEx(strSelName, FindData2) && FindData2.dwUnixMode) {
								FindData = FindData2;
								FileAttr = FindData.dwFileAttributes;
								FileMode = FindData.dwUnixMode;
							}
						}*/
		} else {
			strSelName = Object;
			apiGetFindDataEx(Object, FindData);
			FileAttr = FindData.dwFileAttributes;
			FileMode = FindData.dwUnixMode;
		}

//		fprintf(stderr, "FileMode=%u\n", FileMode);

		if (SelCount == 1 && TestParentFolderName(strSelName))
			return false;

		wchar_t DateSeparator = GetDateSeparator();
		wchar_t TimeSeparator = GetTimeSeparator();
		wchar_t DecimalSeparator = GetDecimalSeparator();
		LPCWSTR FmtMask1 = L"99%c99%c99%c99N", FmtMask2 = L"99%c99%c9999N", FmtMask3 = L"N9999%c99%c99";
		FARString strDMask, strTMask;
		strTMask.Format(FmtMask1, TimeSeparator, TimeSeparator, DecimalSeparator);

		switch (GetDateFormat()) {
			case 0:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle1, DateSeparator,
						DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				strDMask.Format(FmtMask2, DateSeparator, DateSeparator);
				break;
			case 1:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle2, DateSeparator,
						DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				strDMask.Format(FmtMask2, DateSeparator, DateSeparator);
				break;
			default:
				AttrDlg[SA_TEXT_TITLEDATE].strData.Format(Msg::SetAttrTimeTitle3, DateSeparator,
						DateSeparator, TimeSeparator, TimeSeparator, DecimalSeparator);
				strDMask.Format(FmtMask3, DateSeparator, DateSeparator);
				break;
		}

		AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strMask = AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strMask =
				AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strMask = strDMask;

		AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strMask = AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strMask =
				AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strMask = strTMask;
		bool FolderPresent = false;//, LinkPresent = false;
		int  FolderCount = 0, Link2FileCount = 0, Link2DirCount = 0;
		FARString strLinkName;

		if (SelCount == 1) {
			FSFileFlagsSafe FFFlags(strSelName.GetMB(), FileAttr);
			if (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) {
				DlgParam.SymlinkButtonTitles[0] = Msg::SetAttrSymlinkObject;
				DlgParam.SymlinkButtonTitles[1] = Msg::SetAttrSymlinkObjectInfo;
				DlgParam.SymlinkButtonTitles[2] = Msg::SetAttrSymlinkContent;

				AttrDlg[SA_TXTBTN_INFO].Type = DI_BUTTON;
				AttrDlg[SA_TXTBTN_INFO].strData = DlgParam.SymlinkButtonTitles[2];
				ReadSymlink(strSelName, DlgParam.SymLink);
				AttrDlg[SA_EDIT_INFO].strData = DlgParam.SymLink;
				AttrDlg[SA_EDIT_INFO].Flags &= ~DIF_READONLY; // not readonly only if symlink

				Link2FileCount=1; // only for show symlink explain text
			}
			else if (FileAttr & FILE_ATTRIBUTE_DEVICE_CHAR)
				AttrDlg[SA_EDIT_INFO].strData = Msg::FileFilterAttrDevChar;
			else if (FileAttr & FILE_ATTRIBUTE_DEVICE_BLOCK)
				AttrDlg[SA_EDIT_INFO].strData = Msg::FileFilterAttrDevBlock;
			else if (FileAttr & FILE_ATTRIBUTE_DEVICE_FIFO)
				AttrDlg[SA_EDIT_INFO].strData = Msg::FileFilterAttrDevFIFO;
			else if (FileAttr & FILE_ATTRIBUTE_DEVICE_SOCK)
				AttrDlg[SA_EDIT_INFO].strData = Msg::FileFilterAttrDevSock;
			else {
				AttrDlg[SA_EDIT_INFO].strData = BriefInfo(strSelName);
			}


			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				if (!DlgParam.Plugin) {
					FileAttr = apiGetFileAttributes(strSelName);
				}

				//_SVS(SysLog(L"SelName=%ls  FileAttr=0x%08X",SelName,FileAttr));
				AttrDlg[SA_SEPARATOR4].Flags&= ~DIF_HIDDEN;
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&= ~(DIF_DISABLE | DIF_HIDDEN);
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected =
						Opt.SetAttrFolderRules ? BSTATE_UNCHECKED : BSTATE_CHECKED;
				AttrDlg[SA_DOUBLEBOX].Y2+= 2;
				for (int i = SA_SEPARATOR5; i <= SA_BUTTON_CANCEL; i++) {
					AttrDlg[i].Y1+= 2;
					AttrDlg[i].Y2+= 2;
				}
				DlgY+= 2;

				if (Opt.SetAttrFolderRules) {
					if (DlgParam.Plugin || apiGetFindDataEx(strSelName, FindData)) {
						ConvertDate(FindData.ftUnixAccessTime, AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,
								AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData, 12, FALSE, FALSE, 2, TRUE);
						ConvertDate(FindData.ftUnixModificationTime,
								AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,
								AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData, 12, FALSE, FALSE, 2,
								TRUE);
						ConvertDate(FindData.ftUnixStatusChangeTime,
								AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData,
								AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData, 12, FALSE, FALSE, 2, TRUE);
					}
				}

				FolderPresent = TRUE;
			}

			if ((FileAttr != INVALID_FILE_ATTRIBUTES)
					&& ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 || Opt.SetAttrFolderRules)) {
				for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
					AttrDlg[AP[i].Item].Selected =
							(FileMode & AP[i].Mode) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
				}
				AttrDlg[SA_CHECKBOX_IMMUTABLE].Selected = FFFlags.Immutable() ? BSTATE_CHECKED : BSTATE_UNCHECKED;
				AttrDlg[SA_CHECKBOX_APPEND].Selected = FFFlags.Append() ? BSTATE_CHECKED : BSTATE_UNCHECKED;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
				AttrDlg[SA_CHECKBOX_HIDDEN].Selected = FFFlags.Hidden() ? BSTATE_CHECKED : BSTATE_UNCHECKED;
#endif
			}

			for (auto i : PreserveOriginalIDs) {
				AttrDlg[i].Flags&= ~DIF_3STATE;
			}

			AttrDlg[SA_TEXT_NAME].strData = strSelName;
			TruncStr(AttrDlg[SA_TEXT_NAME].strData, DlgX - 10);

			const SETATTRDLG Dates[] = {SA_FIXEDIT_LAST_ACCESS_DATE, SA_FIXEDIT_LAST_MODIFICATION_DATE,
					SA_FIXEDIT_LAST_CHANGE_DATE};
			const SETATTRDLG Times[] = {SA_FIXEDIT_LAST_ACCESS_TIME, SA_FIXEDIT_LAST_MODIFICATION_TIME,
					SA_FIXEDIT_LAST_CHANGE_TIME};
			const PFILETIME TimeValues[] = {&FindData.ftUnixAccessTime, &FindData.ftUnixModificationTime,
					&FindData.ftUnixStatusChangeTime};

			if (DlgParam.Plugin || (!DlgParam.Plugin && apiGetFindDataEx(strSelName, FindData))) {
				for (size_t i = 0; i < ARRAYSIZE(Dates); i++) {
					ConvertDate(*TimeValues[i], AttrDlg[Dates[i]].strData, AttrDlg[Times[i]].strData, 12,
							FALSE, FALSE, 2, TRUE);
				}
			}

			FARString strComputerName;
			if (SrcPanel) {
				FARString strCurDir;
				SrcPanel->GetCurDir(strCurDir);
			}

			GetFileOwner(strComputerName, strSelName, AttrDlg[SA_COMBO_OWNER].strData);
			GetFileGroup(strComputerName, strSelName, AttrDlg[SA_COMBO_GROUP].strData);
		}		// end of if (SelCount==1)
		else {
			for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
				AttrDlg[AP[i].Item].Selected = BSTATE_3STATE;
			}

			AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData.Clear();
			AttrDlg[SA_BUTTON_ORIGINAL].Flags|= DIF_DISABLE;
			AttrDlg[SA_TEXT_NAME].strData = Msg::SetAttrSelectedObjects;

			for (auto i : PreserveOriginalIDs) {
				AttrDlg[i].Selected = BSTATE_UNCHECKED;
			}

			// проверка - есть ли среди выделенных - каталоги?
			// так же проверка на атрибуты
			// так же подсчет числа каталогов и symlinks
			if (SrcPanel) {
				SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
			}
			FolderPresent = false;
			FolderCount = 0; Link2FileCount = 0; Link2DirCount = 0;

			if (SrcPanel) {
				FARString strComputerName;
				FARString strCurDir;
				SrcPanel->GetCurDir(strCurDir);

				while (SrcPanel->GetSelName(&strSelName, FileAttr, FileMode, &FindData)) {
					if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
						if (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT)
							Link2DirCount++;
						else
							FolderCount++;
						if (!FolderPresent) {
							FolderPresent = true;
							AttrDlg[SA_SEPARATOR4].Flags&= ~DIF_HIDDEN;
							AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&= ~(DIF_DISABLE | DIF_HIDDEN);
							AttrDlg[SA_DOUBLEBOX].Y2+= 2;
							for (int i = SA_SEPARATOR5; i <= SA_BUTTON_CANCEL; i++) {
								AttrDlg[i].Y1+= 2;
								AttrDlg[i].Y2+= 2;
							}
							DlgY+= 2;
						}
					}
					else if (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT)
						Link2FileCount++;

					for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
						if (FileMode & AP[i].Mode) {
							AttrDlg[AP[i].Item].Selected++;
						}
					}
					CheckFileOwnerGroup(AttrDlg[SA_COMBO_OWNER], GetFileOwner, strComputerName, strSelName);
					CheckFileOwnerGroup(AttrDlg[SA_COMBO_GROUP], GetFileGroup, strComputerName, strSelName);

					FSFileFlagsSafe FFFlags(strSelName.GetMB(), FileAttr);
					if (FFFlags.Immutable())
						AttrDlg[SA_CHECKBOX_IMMUTABLE].Selected++;
					if (FFFlags.Append())
						AttrDlg[SA_CHECKBOX_APPEND].Selected++;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
					if (FFFlags.Hidden())
						AttrDlg[SA_CHECKBOX_HIDDEN].Selected++;
#endif
				}
			} else {
				// BUGBUG, copy-paste
				if (!FolderPresent && (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					FolderPresent = true;
					AttrDlg[SA_SEPARATOR4].Flags&= ~DIF_HIDDEN;
					AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&= ~(DIF_DISABLE | DIF_HIDDEN);
					AttrDlg[SA_DOUBLEBOX].Y2+= 2;
					for (int i = SA_SEPARATOR5; i <= SA_BUTTON_CANCEL; i++) {
						AttrDlg[i].Y1+= 2;
						AttrDlg[i].Y2+= 2;
					}
					DlgY+= 2;
				}
				for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
					if (FindData.dwUnixMode & AP[i].Mode) {
						AttrDlg[AP[i].Item].Selected++;
					}
				}
			}
			if (SrcPanel) {
				SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
				SrcPanel->GetSelName(&strSelName, FileAttr, FileMode, &FindData);
			}

			// выставим "неопределенку" или то, что нужно
			for (auto i : PreserveOriginalIDs) {
				// снимаем 3-state, если "есть все или нет ничего"
				// за исключением случая, если есть Фолдер среди объектов
				if ((!AttrDlg[i].Selected || AttrDlg[i].Selected >= SelCount) && !FolderPresent) {
					AttrDlg[i].Flags&= ~DIF_3STATE;
				}

				AttrDlg[i].Selected = (AttrDlg[i].Selected >= SelCount)
						? BST_CHECKED
						: (!AttrDlg[i].Selected ? BSTATE_UNCHECKED : BSTATE_3STATE);
			}

			{
				FARString strTmp, strSep=L" (";
				int FilesCount = SelCount-FolderCount-Link2FileCount-Link2DirCount;
				strTmp.Format(Msg::SetAttrInfoSelAll, SelCount);
				if (FolderCount>0)
				{ strTmp.AppendFormat(Msg::SetAttrInfoSelDirs, strSep.CPtr(), FolderCount); strSep=L", "; }
				if (FilesCount>0)
				{ strTmp.AppendFormat(Msg::SetAttrInfoSelFiles, strSep.CPtr(), FilesCount); strSep=L", "; }
				if (Link2DirCount>0)
				{ strTmp.AppendFormat(Msg::SetAttrInfoSelSymDirs, strSep.CPtr(), Link2DirCount); strSep=L", "; }
				if (Link2FileCount>0)
					strTmp.AppendFormat(Msg::SetAttrInfoSelSymFiles, strSep.CPtr(), Link2FileCount);
				strTmp.Append(L')');
				AttrDlg[SA_EDIT_INFO].strData = strTmp;
			}

		}

		// поведение для каталогов как у 1.65?
		if (FolderPresent && !Opt.SetAttrFolderRules) {
			AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected = BSTATE_CHECKED;
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData.Clear();
			AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData.Clear();

			for (auto i : PreserveOriginalIDs) {
				AttrDlg[i].Selected = BSTATE_3STATE;
				AttrDlg[i].Flags|= DIF_3STATE;
			}
		}

		// запомним состояние переключателей.
		for (size_t i = 0; i < ARRAYSIZE(PreserveOriginalIDs); ++i) {
			DlgParam.OriginalCBAttr[i] = DlgParam.OriginalCBAttr0[i] =AttrDlg[PreserveOriginalIDs[i]].Selected;
			DlgParam.OriginalCBAttr2[i] = -1;
			DlgParam.OriginalCBFlag[i] = AttrDlg[PreserveOriginalIDs[i]].Flags;
		}

		// explain text about symlinks properties
		if (FolderPresent || Link2FileCount>0 || Link2DirCount>0) {
			AttrDlg[SC_SYMLINK_PROPERTIES_EXPLAIN_TEXT_1].Flags&= ~DIF_HIDDEN;
			AttrDlg[SC_SYMLINK_PROPERTIES_EXPLAIN_TEXT_2].Flags&= ~DIF_HIDDEN;
			AttrDlg[SA_DOUBLEBOX].Y2+= 2;
			for (int i = SC_SYMLINK_PROPERTIES_EXPLAIN_TEXT_2+1; i <= SA_BUTTON_CANCEL; i++) {
				AttrDlg[i].Y1+= 2;
				AttrDlg[i].Y2+= 2;
			}
			DlgY+= 2;
		}

		DlgParam.strOwner = AttrDlg[SA_COMBO_OWNER].strData;
		DlgParam.strGroup = AttrDlg[SA_COMBO_GROUP].strData;
		DlgParam.strInitOwner = DlgParam.strOwner; DlgParam.strInitGroup = DlgParam.strGroup;

		DlgParam.DialogMode = ((SelCount == 1 && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY))
						? MODE_FILE
						: (SelCount == 1 ? MODE_FOLDER : MODE_MULTIPLE));
		DlgParam.strSelName = strSelName;
		DlgParam.OSubfoldersState = static_cast<FARCHECKEDSTATE>(AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected);

		Dialog Dlg(AttrDlg, ARRAYSIZE(AttrDlgData), SetAttrDlgProc, (LONG_PTR)&DlgParam);
		Dlg.SetHelp(L"FileAttrDlg");	//  ^ - это одиночный диалог!
		Dlg.SetId(FileAttrDlgId);

		/*if (LinkPresent) {
			DlgY++;
		}*/

		Dlg.SetPosition(-1, -1, DlgX, DlgY);
		SetAttrCalcBitsCharFromModeCheckBoxes(&Dlg);	// set octal
		// initial values at dialog start for control change marker
		DlgParam.strInitInfoSymLink = reinterpret_cast<LPCWSTR>(SendDlgMessage(&Dlg, DM_GETCONSTTEXTPTR, SA_EDIT_INFO, 0));
		DlgParam.strInitOctal = reinterpret_cast<LPCWSTR>(SendDlgMessage(&Dlg, DM_GETCONSTTEXTPTR, SA_FIXEDIT_MODE_OCTAL, 0));
		SendDlgMessage(&Dlg, DM_SETTEXTPTR, SA_TEXT_MODE_OCTAL_ORIGINAL, reinterpret_cast<LONG_PTR>(DlgParam.strInitOctal.CPtr()));
		DlgParam.strInitAccessDate = AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData;
		DlgParam.strInitAccessTime = AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData;
		DlgParam.strInitModifyDate = AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData;
		DlgParam.strInitModifyTime = AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData;
		DlgParam.strInitStatusChangeDate = AttrDlg[SA_FIXEDIT_LAST_CHANGE_DATE].strData;
		DlgParam.strInitStatusChangeTime = AttrDlg[SA_FIXEDIT_LAST_CHANGE_TIME].strData;
		// workaround to compare current & initial date:
		//  - in case of mask "9999N" each edit adds a space to the last position after 4-digit year
		switch (GetDateFormat()) {
			case 0:
			case 1:
				if (DlgParam.strInitAccessDate.GetLength() < 11)
					DlgParam.strInitAccessDate += L' ';
				if (DlgParam.strInitModifyDate.GetLength() < 11)
					DlgParam.strInitModifyDate += L' ';
				if (DlgParam.strInitStatusChangeDate.GetLength() < 11)
					DlgParam.strInitStatusChangeDate += L' ';
			default:
				break;
		}

		Dlg.Process();

		switch (Dlg.GetExitCode()) {
			case SA_BUTTON_SET: {
				const size_t Times[] = {SA_FIXEDIT_LAST_ACCESS_TIME, SA_FIXEDIT_LAST_MODIFICATION_TIME,
						SA_FIXEDIT_LAST_CHANGE_TIME};

				for (size_t i = 0; i < ARRAYSIZE(Times); i++) {
					LPWSTR TimePtr = AttrDlg[Times[i]].strData.GetBuffer();
					TimePtr[8] = GetTimeSeparator();
					AttrDlg[Times[i]].strData.ReleaseBuffer(AttrDlg[Times[i]].strData.GetLength());
				}

				TPreRedrawFuncGuard preRedrawFuncGuard(PR_ShellSetFileAttributesMsg);
				ShellSetFileAttributesMsg(SelCount == 1 ? strSelName.CPtr() : nullptr);
				int SkipMode = SETATTR_RET_UNKNOWN;

				if (SelCount == 1 && (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
					FARString OldSymLink;
					ReadSymlink(strSelName, OldSymLink);
					if (DlgParam.SymLink != OldSymLink) {
						int r = 1;
						if ( !apiPathExists(DlgParam.SymLink) ) {
							FARString strTmp1, strTmp2, strTmp3;
							strTmp1.Format(Msg::SetAttrSymlinkWarn1, strSelName.CPtr());
							strTmp2.Format(Msg::SetAttrSymlinkWarn2, DlgParam.SymLink.CPtr());
							strTmp3.Format(Msg::SetAttrSymlinkWarn3, OldSymLink.CPtr());
							r = Message(MSG_WARNING, 2,
								Msg::Error, strTmp1, strTmp2, strTmp3, L" ", Msg::SetAttrSymlinkWarn4,
								Msg::HSkip, Msg::HChange);
						}
						if( r == 1 ) {
							fprintf(stderr, "Symlink change: '%ls' -> '%ls'\n",
								OldSymLink.CPtr(), DlgParam.SymLink.CPtr());
							sdc_unlink(strSelName.GetMB().c_str());
							r = sdc_symlink(DlgParam.SymLink.GetMB().c_str(), strSelName.GetMB().c_str());
							if (r != 0) {
								Message(MSG_WARNING | MSG_ERRORTYPE, 1,
									Msg::Error, Msg::SetAttrSymlinkFailed, strSelName, Msg::Ok);
							}
						}
					}
				}
				if (SelCount == 1 && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
					DWORD NewMode = 0;

					for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
						if (AttrDlg[AP[i].Item].Selected) {
							NewMode|= AP[i].Mode;
						}
					}

					if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_OWNER], ESetFileOwner, SkipMode,
								strSelName, DlgParam.strInitOwner))
						break;
					if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_GROUP], ESetFileGroup, SkipMode,
								strSelName, DlgParam.strInitGroup))
						break;

					FILETIME UnixAccessTime = {}, UnixModificationTime = {};
					int SetAccessTime = DlgParam.OAccessTime
							&& ReadFileTime(0, strSelName, UnixAccessTime,
									AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,
									AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
					int SetModifyTime = DlgParam.OModifyTime
							&& ReadFileTime(1, strSelName, UnixModificationTime,
									AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,
									AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);

					//_SVS(SysLog(L"\n\tSetWriteTime=%d\n\tSetCreationTime=%d\n\tSetLastAccessTime=%d",SetWriteTime,SetCreationTime,SetLastAccessTime));

					if (SetAccessTime || SetModifyTime) {
						if (ESetFileTime(strSelName, SetAccessTime ? &UnixAccessTime : nullptr,
									SetModifyTime ? &UnixModificationTime : nullptr, FileAttr, SkipMode)
								== SETATTR_RET_SKIPALL) {
							SkipMode = SETATTR_RET_SKIP;
						}
					}

					if ((FileMode & EDITABLE_MODES) != (NewMode & EDITABLE_MODES)) {
						if (ESetFileMode(strSelName, NewMode, SkipMode) == SETATTR_RET_SKIPALL) {
							SkipMode = SETATTR_RET_SKIP;
						}
					}

					ApplyFSFileFlags(AttrDlg, strSelName, FileAttr);
				}
				/* Multi *********************************************************** */
				else {
					int RetCode = 1;
					ConsoleTitle SetAttrTitle(Msg::SetAttrTitle);
					if (SrcPanel) {
						CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->CloseFile();
					}
					DWORD SetMode = 0, ClearMode = 0;

					for (size_t i = 0; i < ARRAYSIZE(AP); i++) {
						switch (AttrDlg[AP[i].Item].Selected) {
							case BSTATE_CHECKED:
								SetMode|= AP[i].Mode;
								break;
							case BSTATE_UNCHECKED:
								ClearMode|= AP[i].Mode;
								break;
						}
					}

					if (SrcPanel) {
						SrcPanel->GetSelName(nullptr, FileAttr, FileMode);
					}
					wakeful W;
					bool Cancel = false;
					DWORD LastTime = 0;

					bool SingleFileDone = false;
					while ((SrcPanel ? SrcPanel->GetSelName(&strSelName, FileAttr, FileMode, &FindData)
										: !SingleFileDone)
							&& !Cancel) {
						if (!SrcPanel) {
							SingleFileDone = true;
						}
						//_SVS(SysLog(L"SelName='%ls'\n\tFileAttr =0x%08X\n\tSetAttr  =0x%08X\n\tClearAttr=0x%08X\n\tResult   =0x%08X",
						//	SelName,FileAttr,SetAttr,ClearAttr,((FileAttr|SetAttr)&(~ClearAttr))));
						DWORD CurTime = WINPORT(GetTickCount)();

						if (CurTime - LastTime > RedrawTimeout) {
							LastTime = CurTime;
							ShellSetFileAttributesMsg(strSelName);

							if (CheckForEsc())
								break;
						}

						if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_OWNER], ESetFileOwner, SkipMode,
									strSelName, DlgParam.strInitOwner))
							break;
						if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_GROUP], ESetFileGroup, SkipMode,
									strSelName, DlgParam.strInitGroup))
							break;

						FILETIME UnixAccessTime = {}, UnixModificationTime = {};
						int SetAccessTime = DlgParam.OAccessTime
								&& ReadFileTime(0, strSelName, UnixAccessTime,
										AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,
										AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
						int SetModifyTime = DlgParam.OModifyTime
								&& ReadFileTime(1, strSelName, UnixModificationTime,
										AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,
										AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);

						RetCode = ESetFileTime(strSelName, SetAccessTime ? &UnixAccessTime : nullptr,
								SetModifyTime ? &UnixModificationTime : nullptr, FileAttr, SkipMode);

						if (RetCode == SETATTR_RET_ERROR)
							break;
						else if (RetCode == SETATTR_RET_SKIP)
							continue;
						else if (RetCode == SETATTR_RET_SKIPALL) {
							SkipMode = SETATTR_RET_SKIP;
							continue;
						}

						if (FileAttr != INVALID_FILE_ATTRIBUTES) {
							if (((FileMode | SetMode) & ~ClearMode) != FileMode) {

								RetCode = ESetFileMode(strSelName, ((FileMode | SetMode) & (~ClearMode)),
										SkipMode);

								if (RetCode == SETATTR_RET_ERROR)
									break;
								else if (RetCode == SETATTR_RET_SKIP)
									continue;
								else if (RetCode == SETATTR_RET_SKIPALL) {
									SkipMode = SETATTR_RET_SKIP;
									continue;
								}
							}

							if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY)
									&& AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected) {
								ScanTree ScTree(FALSE);
								ScTree.SetFindPath(strSelName, L"*");
								DWORD LastTime = WINPORT(GetTickCount)();
								FARString strFullName;

								while (ScTree.GetNextName(&FindData, strFullName)) {
									DWORD CurTime = WINPORT(GetTickCount)();

									if (CurTime - LastTime > RedrawTimeout) {
										LastTime = CurTime;
										ShellSetFileAttributesMsg(strFullName);

										if (CheckForEsc()) {
											Cancel = true;
											break;
										}
									}

									if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_GROUP], ESetFileOwner,
												SkipMode, strFullName, DlgParam.strInitOwner,
												DlgParam.OSubfoldersState))
										break;
									if (!ApplyFileOwnerGroupIfChanged(AttrDlg[SA_COMBO_GROUP], ESetFileGroup,
												SkipMode, strFullName, DlgParam.strInitGroup,
												DlgParam.OSubfoldersState))
										break;

									SetAccessTime = DlgParam.OAccessTime
											&& ReadFileTime(0, strFullName, UnixAccessTime,
													AttrDlg[SA_FIXEDIT_LAST_ACCESS_DATE].strData,
													AttrDlg[SA_FIXEDIT_LAST_ACCESS_TIME].strData);
									SetModifyTime = DlgParam.OModifyTime
											&& ReadFileTime(1, strFullName, UnixModificationTime,
													AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_DATE].strData,
													AttrDlg[SA_FIXEDIT_LAST_MODIFICATION_TIME].strData);

									if (SetAccessTime || SetModifyTime) {
										RetCode = ESetFileTime(strFullName,
												SetAccessTime ? &UnixAccessTime : nullptr,
												SetModifyTime ? &UnixModificationTime : nullptr,
												FindData.dwFileAttributes, SkipMode);

										if (RetCode == SETATTR_RET_ERROR) {
											Cancel = true;
											break;
										} else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL) {
											SkipMode = SETATTR_RET_SKIP;
											continue;
										}
									}

									if (((FindData.dwUnixMode | SetMode) & (~ClearMode))
											!= FindData.dwUnixMode) {
										RetCode = ESetFileMode(strFullName,
												(FindData.dwUnixMode | SetMode) & (~ClearMode), SkipMode);

										if (RetCode == SETATTR_RET_ERROR) {
											Cancel = true;
											break;
										} else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL) {
											SkipMode = SETATTR_RET_SKIP;
											continue;
										}
										ApplyFSFileFlags(AttrDlg, strFullName, FileAttr);
									}
								}
							}
						}
						ApplyFSFileFlags(AttrDlg, strSelName, FileAttr);

					}	// END: while (SrcPanel->GetSelNameCompat(...))
				}
			} break;
			default:
				return false;
		}
	}

	if (SrcPanel) {
		SrcPanel->SaveSelection();
		SrcPanel->Update(UPDATE_KEEP_SELECTION);
		SrcPanel->ClearSelection();
		CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	}
	CtrlObject->Cp()->Redraw();
	return true;
}
