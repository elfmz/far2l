#ifndef __EditCase_H
#define __EditCase_H

static struct PluginStartupInfo Info;
static FARSTANDARDFUNCTIONS FSF;

// Menu item numbers
#define CCLower     0
#define CCTitle     1
#define CCUpper     2
#define CCToggle    3
#define CCCyclic    4

// Plugin Registry root
static WCHAR PluginRootKey[80];
// This chars aren't letters
static WCHAR WordDiv[80];
static int WordDivLen;

const WCHAR *GetMsg(int MsgId);

BOOL FindBounds(WCHAR *Str, int Len, int Pos, int &Start, int &End);
int FindEnd(WCHAR *Str, int Len, int Pos);
int FindStart(WCHAR *Str, int Len, int Pos);
BOOL MyIsAlpha(int c);
int GetNextCCType(WCHAR *Str, int StrLen, int Start, int End);
int ChangeCase(WCHAR *NewString, int Start, int End, int CCType);

#endif
