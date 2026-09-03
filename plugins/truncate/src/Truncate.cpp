#include <farplug-wide.h>
#include <utils.h>

enum
{
	MTruncate,
	MDeletedStats,
	MNothingToDelete,
	MEditorLocked
};

static struct PluginStartupInfo Info;
static FARSTANDARDFUNCTIONS FSF;

const wchar_t *GetMsg(int MsgId)
{
	return Info.GetMsg(Info.ModuleNumber,MsgId);
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	FSF = *Info->FSF;
	::Info.FSF = &FSF;
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom != OPEN_EDITOR && OpenFrom != (OPEN_FROMMACRO | MACROAREA_EDITOR)) {
		return INVALID_HANDLE_VALUE;
	}

	EditorInfo ei {};
	if (!Info.EditorControl(ECTL_GETINFO, &ei)) {
		return INVALID_HANDLE_VALUE;
	}

	if (ei.CurState & ECSTATE_LOCKED) {
		const wchar_t* items[] = {GetMsg(MTruncate), GetMsg(MEditorLocked)};
		Info.Message(Info.ModuleNumber, FMSG_MB_OK, nullptr, items, ARRAYSIZE(items), 0);
		return INVALID_HANDLE_VALUE;
	}

	int count = 0;
	int empty_lines = 0;

	EditorUndoRedo eur {};
	eur.Command = EUR_BEGIN;
	Info.EditorControl(ECTL_UNDOREDO, &eur);

	for (int i = 0; i < ei.TotalLines; i++) {
		EditorGetString gs {};
		gs.StringNumber = i;
		if (!Info.EditorControl(ECTL_GETSTRING, &gs)) continue;

		/* Count trailing spaces and tabs */
		int len = gs.StringLength;
		while (len > 0 && (gs.StringText[len - 1] == L' ' || gs.StringText[len - 1] == L'\t')) len--;
		if (len) {
			/* We are interested only in the empty lines at the end of file. So, reset the counter */
			empty_lines = 0;
		} else {
			/* Count empty lines */
			empty_lines++;
		}

		/* Strip trailing spaces and tabs */
		if (len != gs.StringLength) {
			wchar_t *s = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
			if (!s) continue;
			wmemcpy(s, gs.StringText, len);
			s[len] = 0;

			EditorSetString ss {};
			ss.StringNumber = i;
			ss.StringText = s;
			ss.StringEOL = gs.StringEOL;
			ss.StringLength = len;
			Info.EditorControl(ECTL_SETSTRING, &ss);

			free(s);

			count += gs.StringLength - len;
		}
	}

	if (empty_lines > 1) {
		EditorSetPosition pos {};

		/* Delete trailing empty lines */
		for (int i = 1; i <= empty_lines; i++) {
			pos.CurLine = ei.TotalLines - i;
			pos.CurPos = 0;
			pos.CurTabPos = ei.CurTabPos;
			pos.TopScreenLine = ei.TopScreenLine;
			pos.LeftPos = ei.LeftPos;
			pos.Overtype = ei.Overtype;
			Info.EditorControl(ECTL_SETPOSITION, &pos);
			Info.EditorControl(ECTL_DELETESTRING, nullptr);
		}

		/* Restore original position */
		if (ei.CurLine > ei.TotalLines - empty_lines) {
			pos.CurLine = ei.TotalLines - empty_lines;
			pos.CurPos = 0;
		} else {
			pos.CurLine = ei.CurLine;
			pos.CurPos = ei.CurPos;
		}
		pos.CurTabPos = ei.CurTabPos;
		pos.TopScreenLine = ei.TopScreenLine;
		pos.LeftPos = ei.LeftPos;
		pos.Overtype = ei.Overtype;
		Info.EditorControl(ECTL_SETPOSITION, &pos);
	}

	eur.Command = EUR_END;
	Info.EditorControl(ECTL_UNDOREDO, &eur);

	/* Don't count the final EOL */
	if (empty_lines) empty_lines--;

	if (count || empty_lines) {
		wchar_t message[255] = {};
		FSF.snprintf(message, ARRAYSIZE(message) - 1, GetMsg(MDeletedStats), count, empty_lines);
		const wchar_t* items[] = {GetMsg(MTruncate), message};
		Info.Message(Info.ModuleNumber, FMSG_MB_OK, nullptr, items, ARRAYSIZE(items), 0);
	} else {
		const wchar_t* items[] = {GetMsg(MTruncate), GetMsg(MNothingToDelete)};
		Info.Message(Info.ModuleNumber, FMSG_MB_OK, nullptr, items, ARRAYSIZE(items), 0);
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->SysID = 0x54524354; // 'TRCT'
	Info->Flags = PF_EDITOR | PF_DISABLEPANELS;
	Info->DiskMenuStringsNumber = 0;
	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MTruncate);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
	Info->PluginConfigStringsNumber = 0;
}
