#pragma once
#include "Backend.h"

class wxPrinterSupportBackend : public IPrinterSupport
{
public:
	wxPrinterSupportBackend();
	virtual ~wxPrinterSupportBackend();

	virtual void PrintText(const std::wstring& jobName, const std::wstring& text);
	virtual void PrintReducedHTML(const std::wstring& jobName, const std::wstring& text);
	virtual void PrintTextFile(const std::wstring& fileName);
	virtual void PrintHtmlFile(const std::wstring& fileName);

	virtual void ShowPreviewForText(const std::wstring&  jobName, const std::wstring& text);
	virtual void ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text);
	virtual void ShowPreviewForTextFile(const std::wstring& fileName);
	virtual void ShowPreviewForHtmlFile(const std::wstring& fileName);

	virtual void ShowPrinterSetupDialog();

	virtual bool IsPrintPreviewSupported();
	virtual bool IsReducedHTMLSupported();
	virtual bool IsPrinterSetupDialogSupported();
};
