#ifndef __FTP_PROGRESS_INTERNAL
#define __FTP_PROGRESS_INTERNAL
#include <utils.h>
#include "fstdlib.h"	//FAR plugin stdlib
#include "../Plugin.h"

#define MAX_TRAF_LINES 20
#define MAX_TRAF_WIDTH 200
#define MAX_TRAF_ITEMS 50

enum tAlignment
{
	tNone,
	tLeft,
	tRight,
	tCenter,
	tRightFill
};

// One element drawed inside dialog lines array
struct InfoItem
{
	int Type;	// Type of element (
	int Line;	// Line number (Y)
	int Pos;	// Starting position in line (X)
	int Size;	// Width of element (not for all alignment)
	int Align;	// Element alignment (tAlignment)
	char Fill;	// Filler character
};

struct TrafficInformation : public ProgressInterface
{
	HANDLE hConnect;
	char ConsoleTitle[FAR_MAX_TITLE];
	char Lines[MAX_TRAF_LINES][MAX_TRAF_WIDTH + 1];
	int LineCount;

	char SrcFileName[MAX_PATH];
	char DestFileName[MAX_PATH];
	int64_t FileSize;
	int64_t StartFileSize;
	int64_t FullFileSize;
	time_t FileStartTime;
	time_t FileWaitTime;
	double Cps;
	double AvCps[3];

	int64_t TotalFiles;
	int64_t TotalComplete;
	int64_t TotalSkipped;

	int64_t TotalSize;
	int64_t TotalStartSize;
	int64_t TotalFullSize;
	time_t TotalStartTime;
	time_t TotalWaitTime;
	double TotalCps;

	int TitleMsg;
	BOOL ShowStatus;
	DWORD LastTime;
	int64_t LastSize;

	InfoItem Items[MAX_TRAF_ITEMS];
	int Count;

	// ------------- INTERNAL
	// Format and draw lines
	void FormatLine(int num, LPCSTR line, time_t tm);
	void DrawInfo(InfoItem *it, time_t tm);
	void DrawInfos(time_t tm);

	// Current infos
	int64_t CurrentSz(void) { return FileSize + StartFileSize; }
	int64_t CurrentRemain(void) { return FullFileSize - CurrentSz(); }
	int64_t CurrentDoRemain(void) { return FullFileSize - StartFileSize; }
	int64_t TotalSz(void) { return TotalSize + TotalStartSize + CurrentSz(); }
	int64_t TotalRemain(void) { return TotalFullSize - TotalSz(); }
	int64_t TotalDoRemain(void) { return TotalFullSize - TotalStartSize; }

	// ------------- PUBLICS
	// Resume
	void Resume(LPCSTR LocalFileName);
	void Resume(int64_t size);

	// Called for every copied portion
	BOOL Callback(int Size);

	// Start using traffic (start of whole operation)
	void Init(HANDLE h, int tMsg, int OpMode, FP_SizeItemList *il);

	// Start of every file
	void InitFile(PluginPanelItem *pi, LPCSTR SrcName, LPCSTR DestName);
	void InitFile(int64_t sz, LPCSTR SrcName, LPCSTR DestName);

	// Skip last part of current file
	void Skip(void);

	// Inserts pause to values (f.e. while plugin wait Y/N dialog input)
	void Waiting(time_t paused);

	// Attach to specified connection
	void SetConnection(HANDLE Connection) { hConnect = Connection; }
};

LPCSTR FCps4(char *buff, double val);
void PPercent(char *str, int x, int x1, int percent);	// Draws a percent gouge.
double ToPercent(int64_t Value, int64_t ValueLimit);	// Calculate float percent.
void StrYTime(char *str, struct tm *tm);
void StrTTime(char *str, struct tm *tm);

#endif