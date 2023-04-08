#include <farplug-wide.h>
#include <fardlgbuilder.hpp>
#include "WrapLng.hpp"
#include "AutoWrap.hpp"

#include "WrapMix.icpp"

#include <utils.h>
#include <KeyFileHelper.h>

#define INI_LOCATION InMyConfig("plugins/autowrap/config.ini")
#define INI_SECTION  "Settings"

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	FSF = *Info->FSF;
	::Info.FSF = &FSF;
	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	Opt.Wrap = kfh.GetInt(("Wrap"), 0);
	Opt.RightMargin = kfh.GetInt(("RightMargin"), 75);
	kfh.GetChars(Opt.FileMasks, ARRAYSIZE(Opt.FileMasks), "FileMasks", L"*.*");
	kfh.GetChars(Opt.ExcludeFileMasks, ARRAYSIZE(Opt.ExcludeFileMasks), "ExcludeFileMasks", L"");
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	PluginDialogBuilder Builder(Info, MAutoWrap, NULL);
	Builder.AddCheckbox(MEnableWrap, &Opt.Wrap);
	FarDialogItem *RightMargin = Builder.AddIntEditField(&Opt.RightMargin, 3);
	Builder.AddTextAfter(RightMargin, MRightMargin);
	Builder.AddSeparator();
	Builder.AddText(MFileMasks);
	Builder.AddEditField(Opt.FileMasks, ARRAYSIZE(Opt.FileMasks), 65);
	Builder.AddText(MExcludeFileMasks);
	Builder.AddEditField(Opt.ExcludeFileMasks, ARRAYSIZE(Opt.ExcludeFileMasks), 65);
	Builder.AddOKCancel(MOk, MCancel);
	if (Builder.ShowDialog()) {
		KeyFileHelper kfh(INI_LOCATION);
		kfh.SetInt(INI_SECTION, ("Wrap"), Opt.Wrap);
		kfh.SetInt(INI_SECTION, ("RightMargin"), Opt.RightMargin);
		kfh.SetString(INI_SECTION, ("FileMasks"), Opt.FileMasks);
		kfh.SetString(INI_SECTION, ("ExcludeFileMasks"), Opt.ExcludeFileMasks);
	}
	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI EXP_NAME(ProcessEditorInput)(const INPUT_RECORD *Rec)
{
	if (!Opt.Wrap)
		return (FALSE);

	static int Reenter = FALSE;

	if (Reenter || Rec->EventType != KEY_EVENT || !Rec->Event.KeyEvent.bKeyDown
			|| Rec->Event.KeyEvent.wVirtualKeyCode == VK_F1)
		return (FALSE);

	struct EditorInfo startei;
	Info.EditorControl(ECTL_GETINFO, &startei);

	struct EditorGetString prevegs;
	prevegs.StringNumber = -1;
	Info.EditorControl(ECTL_GETSTRING, &prevegs);

	Reenter = TRUE;
	Info.EditorControl(ECTL_PROCESSINPUT, (void *)Rec);
	Reenter = FALSE;

	for (int Pass = 1;; Pass++) {
		EditorInfo ei;
		Info.EditorControl(ECTL_GETINFO, &ei);
#ifdef UNICODE
		LPWSTR FileName = NULL;
		size_t FileNameSize = Info.EditorControl(ECTL_GETFILENAME, NULL);
		if (FileNameSize) {
			FileName = new wchar_t[FileNameSize];
			if (FileName) {
				Info.EditorControl(ECTL_GETFILENAME, FileName);
			}
		}
#endif
		if (Pass == 1 && *Opt.FileMasks) {
			if (ei.CurLine != startei.CurLine) {
#ifdef UNICODE
				if (FileName) {
					delete[] FileName;
				}
#endif
				return TRUE;
			}
			int Found = FALSE;
			TCHAR FileMask[MAX_PATH], *MaskPtr = Opt.FileMasks;
			while ((MaskPtr = GetCommaWord(MaskPtr, FileMask)) != NULL)
				if (Info.CmpName(FileMask,
#ifndef UNICODE
							ei.
#endif
							FileName,
							TRUE)) {
					Found = TRUE;
					break;
				}
			if (!Found) {
#ifdef UNICODE
				if (FileName) {
					delete[] FileName;
				}
#endif
				return TRUE;
			}
			MaskPtr = Opt.ExcludeFileMasks;
			while ((MaskPtr = GetCommaWord(MaskPtr, FileMask)) != NULL)
				if (Info.CmpName(FileMask,
#ifndef UNICODE
							ei.
#endif
							FileName,
							TRUE)) {
					Found = FALSE;
					break;
				}
			if (!Found) {
#ifdef UNICODE
				if (FileName) {
					delete[] FileName;
				}
#endif
				return TRUE;
			}
		}
#ifdef UNICODE
		if (FileName) {
			delete[] FileName;
		}
#endif

		struct EditorGetString egs;
		egs.StringNumber = ei.CurLine;
		Info.EditorControl(ECTL_GETSTRING, &egs);

		int TabPresent = _tmemchr(egs.StringText, _T('\t'), egs.StringLength) != NULL;
		int TabLength = egs.StringLength;
		if (TabPresent) {
			struct EditorConvertPos ecp;
			ecp.StringNumber = -1;
			ecp.SrcPos = egs.StringLength;
			Info.EditorControl(ECTL_REALTOTAB, &ecp);
			TabLength = ecp.DestPos;
		}

		if ((Pass != 1 || prevegs.StringLength != egs.StringLength) && TabLength >= Opt.RightMargin
				&& ei.CurPos >= egs.StringLength) {
			int SpacePos = -1;
			int I;
			for (I = egs.StringLength - 1; I > 0; I--)
				if (egs.StringText[I] == _T(' ') || egs.StringText[I] == _T('\t')) {
					SpacePos = I;
					int TabPos = I;
					if (TabPresent) {
						struct EditorConvertPos ecp;
						ecp.StringNumber = -1;
						ecp.SrcPos = I;
						Info.EditorControl(ECTL_REALTOTAB, &ecp);
						TabPos = ecp.DestPos;
					}
					if (TabPos < Opt.RightMargin)
						break;
				}

			if (SpacePos <= 0)
				break;

			int SpaceOnly = TRUE;
			for (I = 0; I < SpacePos; I++)
				if (egs.StringText[I] != _T(' ') && egs.StringText[I] != _T('\t')) {
					SpaceOnly = FALSE;
					break;
				}

			if (SpaceOnly)
				break;

			struct EditorSetPosition esp;
			memset(&esp, -1, sizeof(esp));
			esp.CurPos = SpacePos + 1;
			Info.EditorControl(ECTL_SETPOSITION, &esp);
			int Indent = TRUE;
			if (!Info.EditorControl(ECTL_INSERTSTRING, &Indent))
				break;
			if (ei.CurPos < SpacePos) {
				esp.CurLine = ei.CurLine;
				esp.CurPos = ei.CurPos;
				Info.EditorControl(ECTL_SETPOSITION, &esp);
			} else {
				egs.StringNumber = ei.CurLine + 1;
				Info.EditorControl(ECTL_GETSTRING, &egs);
				esp.CurLine = ei.CurLine + 1;
				esp.CurPos = egs.StringLength;
				Info.EditorControl(ECTL_SETPOSITION, &esp);
			}
			Info.EditorControl(ECTL_REDRAW, NULL);
		} else
			break;
	}
	return (TRUE);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_EDITOR | PF_DISABLEPANELS;
	Info->DiskMenuStringsNumber = 0;
	static const TCHAR *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MAutoWrap);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
	Info->PluginConfigStringsNumber = 0;
}
