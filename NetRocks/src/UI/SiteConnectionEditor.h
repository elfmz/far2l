#pragma once
#include <string>
#include <windows.h>

class SiteConnectionEditor
{
	std::string _site;

	static LONG_PTR WINAPI sDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	static LONG_PTR DlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

public:
	SiteConnectionEditor(const std::string &site);


	bool Edit();

	const std::string &SiteName() const { _site; }
};
