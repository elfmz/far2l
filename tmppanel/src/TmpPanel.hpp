/*
TMPPANEL.HPP

Temporary panel header file

*/

#ifndef __TMPPANEL_HPP__
#define __TMPPANEL_HPP__
#include <windows.h>
#include "PlatformConstants.h"
#include <farplug-wide.h>
#include "expname.h"
#include "TmpLng.hpp"
#include "TmpClass.hpp"
#include "TmpCfg.hpp"

#define COMMONPANELSNUMBER 10

typedef struct _MyInitDialogItem
{
	unsigned char Type;
	unsigned char X1, Y1, X2, Y2;
	DWORD Flags;
	signed char Data;
} MyInitDialogItem;

typedef struct _PluginPanels
{
	PluginPanelItem *Items;
	unsigned int ItemsNumber;
	unsigned int OpenFrom;
} PluginPanels;

extern PluginPanels CommonPanels[COMMONPANELSNUMBER];

extern unsigned int CurrentCommonPanel;

extern struct PluginStartupInfo Info;
extern struct FarStandardFunctions FSF;

extern int StartupOptFullScreenPanel, StartupOptCommonPanel, StartupOpenFrom;
extern wchar_t *PluginRootKey;

const wchar_t *GetMsg(int MsgId);
void InitDialogItems(const MyInitDialogItem *Init, struct FarDialogItem *Item, int ItemsNumber);

int Config();
void GoToFile(const wchar_t *Target, BOOL AnotherPanel);
void FreePanelItems(PluginPanelItem *Items, DWORD Total);

wchar_t *ParseParam(wchar_t *&str);
void GetOptions(void);
void WFD2FFD(WIN32_FIND_DATA &wfd, FAR_FIND_DATA &ffd);
int PWZ_to_PZ(const wchar_t *src, char *dst, int lendst);

#define NT_MAX_PATH 32768

class StrBuf
{
	wchar_t *ptr;
	int len;

private:
	StrBuf(const StrBuf &);
	StrBuf &operator=(const StrBuf &);

public:
	StrBuf()
	{
		ptr = NULL;
		len = 0;
	}
	StrBuf(int len)
	{
		ptr = NULL;
		Reset(len);
	}
	void Reset(int len)
	{
		if (ptr)
			free(ptr);
		ptr = (wchar_t *)malloc(len * sizeof(wchar_t));
		*ptr = 0;
		this->len = len;
	}
	void Grow(int len)
	{
		if (len > this->len)
			Reset(len);
	}
	operator wchar_t *() { return ptr; }
	wchar_t *Ptr() { return ptr; }
	int Size() const { return len; }
	~StrBuf() { free(ptr); }
};

class PtrGuard
{
	wchar_t *ptr;

private:
	PtrGuard(const PtrGuard &);
	PtrGuard &operator=(const PtrGuard &);

public:
	PtrGuard() { ptr = NULL; }
	PtrGuard(wchar_t *ptr) { this->ptr = ptr; }
	PtrGuard &operator=(wchar_t *ptr)
	{
		free(this->ptr);
		this->ptr = ptr;
		return *this;
	}
	operator wchar_t *() { return ptr; }
	wchar_t *Ptr() { return ptr; }
	wchar_t **PtrPtr() { return &ptr; }
	~PtrGuard() { free(ptr); }
};

wchar_t *GetFullPath(const wchar_t *input, StrBuf &output);
wchar_t *ExpandEnvStrs(const wchar_t *input, StrBuf &output);
bool FindListFile(const wchar_t *FileName, StrBuf &output);
const wchar_t *GetTmpPanelModule();

#endif	/* __TMPPANEL_HPP__ */
