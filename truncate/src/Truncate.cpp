#include <farplug-wide.h>
#include <fardlgbuilder.hpp>

#include <utils.h>
#include <KeyFileHelper.h>

enum
{
	MTruncate,
	MMessage1,
	MMessage2,
};

static struct PluginStartupInfo Info;
static FARSTANDARDFUNCTIONS FSF;
TCHAR PluginRootKey[80];

const TCHAR *GetMsg(int MsgId)
{
	return(Info.GetMsg(Info.ModuleNumber,MsgId));
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
	int count = 0;
	int empty_lines = 0;

	struct EditorInfo ei;
	Info.EditorControl(ECTL_GETINFO, &ei);

	for (int i = 0; i < ei.TotalLines; i++) {
		struct EditorGetString gs;
		gs.StringNumber = i;
		if (!Info.EditorControl(ECTL_GETSTRING, &gs)) continue;

		/* Count trailing spaces and tabs */
		int len = gs.StringLength;
		while (len > 0 && (gs.StringText[len - 1] == _T(' ') || gs.StringText[len - 1] == _T('\t'))) len--;
		if (len) {
			/* We are interested only in the empty lines at the end of file. So, reset the counter */
			empty_lines = 0;
		} else {
			/* Count empty lines */
			empty_lines++;
		}

		/* Strip trailing spaces and tabs */
		if (len != gs.StringLength) {
			TCHAR *s = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
			if (!s) continue;
			_tmemcpy(s, gs.StringText, len);
			s[len] = 0;

			struct EditorSetString ss;
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
		struct EditorSetPosition pos;

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

	/* Don't count the final EOL */
	if (empty_lines) empty_lines--;

	if (count || empty_lines) {
		TCHAR message[255] = {};
		FSF.snprintf(message, ARRAYSIZE(message) - 1, GetMsg(MMessage1), count, empty_lines);
		const TCHAR* items[] = {GetMsg(MTruncate), message};
		Info.Message(Info.ModuleNumber, FMSG_MB_OK, nullptr, items, ARRAYSIZE(items), 2);
	} else {
		const TCHAR* items[] = {GetMsg(MTruncate), GetMsg(MMessage2)};
		Info.Message(Info.ModuleNumber, FMSG_MB_OK, nullptr, items, ARRAYSIZE(items), 2);
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_EDITOR | PF_DISABLEPANELS;
	Info->DiskMenuStringsNumber = 0;
	static const TCHAR *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MTruncate);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
	Info->PluginConfigStringsNumber = 0;
}
