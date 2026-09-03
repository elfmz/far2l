#include <all_far.h>

#include "fstdlib.h"

BOOL WINAPI FP_CopyToClipboard(LPVOID Data, SIZE_T DataSize)
{
	void *CData;
	BOOL rc = FALSE;

	if (!Data || !DataSize || !WINPORT(OpenClipboard)(NULL))
		return FALSE;

	WINPORT(EmptyClipboard)();

	if ((CData = WINPORT(ClipboardAlloc)(DataSize + 1)) != NULL) {
		memcpy(CData, Data, DataSize + 1);
		if (WINPORT(SetClipboardData)(CF_TEXT, CData)) {
			rc = TRUE;
		} else {
			WINPORT(ClipboardFree)(CData);
		}
	}

	WINPORT(CloseClipboard)();

	return rc;
}

BOOL WINAPI FP_GetFromClipboard(LPVOID &Data, SIZE_T &DataSize)
{
	void *CData;
	BOOL rc = FALSE;
	Data = NULL;

	if (!WINPORT(OpenClipboard)(NULL))
		return FALSE;

	CData = WINPORT(GetClipboardData)(CF_TEXT);

	if (CData) {
		Data = strdup((char *)CData);
		rc = TRUE;
	}

	WINPORT(CloseClipboard)();
	return rc;
}
