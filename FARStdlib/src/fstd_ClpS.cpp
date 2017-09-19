#include <all_far.h>


#include "fstdlib.h"

BOOL WINAPI FP_CopyToClipboard(LPVOID Data, SIZE_T DataSize)
{
	HGLOBAL  hData;
	void    *GData;
	BOOL     rc = FALSE;

	if(!Data || !DataSize ||
	        !WINPORT(OpenClipboard)(NULL))
		return FALSE;

	WINPORT(EmptyClipboard)();

	do
	{
		if((hData=WINPORT(GlobalAlloc)(GMEM_MOVEABLE|GMEM_DDESHARE,DataSize+1))!=NULL)
		{
			if((GData=WINPORT(GlobalLock)(hData))!=NULL)
			{
				memcpy(GData,Data,DataSize+1);
				WINPORT(GlobalUnlock)(hData);
				WINPORT(SetClipboardData)(CF_TEXT,(HANDLE)hData);
				rc = TRUE;
			}
			else
			{
				WINPORT(GlobalFree)(hData);
				break;
			}
		}
	}
	while(0);

	WINPORT(CloseClipboard)();
	return rc;
}

BOOL WINAPI FP_GetFromClipboard(LPVOID& Data, SIZE_T& DataSize)
{
	HANDLE   hData;
	void    *GData;
	BOOL     rc = FALSE;
	Data     = NULL;
	//DataSize = 0;

	if(!WINPORT(OpenClipboard)(NULL)) return FALSE;

	do
	{
		hData = WINPORT(GetClipboardData)(CF_TEXT);

		if(!hData)
			break;

		//DataSize = WINPORT(GlobalSize)(hData);

		//if(!DataSize)
		//	break;

		GData = WINPORT(GlobalLock)(hData);

		if(!GData)
			break;

		Data = strdup((char *)GData);//malloc(DataSize+1);
		//memcpy(Data,GData,DataSize);
		rc = TRUE;
	}
	while(0);

	WINPORT(CloseClipboard)();
	return rc;
}
