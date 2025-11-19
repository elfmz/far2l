#ifndef __FARAPI_H__
#define __FARAPI_H__

#include <farplug-wide.h>
#include <memory>
#include "common.h"

class FarApi {
private:
	struct PluginStartupInfo & PSI;
	struct FarStandardFunctions & FSF;
public:
	// towstr can use only for ASCII symbols
	std::wstring towstr(const char * name) const;
	std::string tostr(const wchar_t * name) const;

	const wchar_t * DublicateCountString(int64_t value) const;
	const wchar_t * DublicateFileSizeString(uint64_t value) const;

	void GetPanelInfo(PanelInfo & pi) const;
	PluginPanelItem * GetPanelItem(intptr_t itemNum) const;
	void FreePanelItem(PluginPanelItem * ppi) const;
	PluginPanelItem * GetCurrentPanelItem(PanelInfo * piret = nullptr) const;
	PluginPanelItem * GetSelectedPanelItem(intptr_t selectedItemNum) const;
	const wchar_t * GetMsg(int msgId) const;
	int Select(HANDLE hDlg, const wchar_t ** elements, int count, uint32_t setIndex) const;
	int SelectNum(HANDLE hDlg, const wchar_t ** elements, int count, const wchar_t * subTitle, uint32_t setIndex) const;

	FarApi(struct PluginStartupInfo & psi, struct FarStandardFunctions & FSF);
	FarApi();
	virtual ~FarApi();
};

#endif /* __FARAPI_H__ */
