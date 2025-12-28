#pragma once
#include "Backend.h"

class ttyPrinterSupportBackend : public IPrinterSupport
{
public:
	ttyPrinterSupportBackend();
	virtual ~ttyPrinterSupportBackend();

	virtual void PrintText(const std::wstring& jobName, const std::wstring& text);
	virtual void PrintReducedHTML(const std::wstring& jobName, const std::wstring& text);
	virtual void PrintTextFile(const std::wstring& fileName);
	virtual void PrintHtmlFile(const std::wstring& fileName);

	virtual void ShowPreviewForText(const std::wstring&  jobName, const std::wstring& text);
	virtual void ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text);
	virtual void ShowPreviewForTextFile(const std::wstring& fileName);
	virtual void ShowPreviewForHtmlFile(const std::wstring& fileName);

	virtual void ShowPrinterSetupDialog();

	virtual bool IsPrintPreviewSupported(){ return false; }
	virtual bool IsReducedHTMLSupported(){ return false; }
	virtual bool IsPrinterSetupDialogSupported(){ return false; }
};
