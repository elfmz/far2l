#pragma once
#include "Backend.h"

class ttyPrinterSupportBackend : public IPrinterSupport
{
public:
	ttyPrinterSupportBackend();
	virtual ~ttyPrinterSupportBackend();

	virtual void PrintText(const wchar_t* jobName, const wchar_t* text);
	virtual void PrintReducedHTML(const wchar_t* jobName, const wchar_t* text);
	virtual void PrintTextFile(const wchar_t* fileName);
	virtual void PrintHtmlFile(const wchar_t* fileName);

	virtual void ShowPreviewForText(const wchar_t*  jobName, const wchar_t* text);
	virtual void ShowPreviewForReducedHTML(const wchar_t* jobName, const wchar_t* text);
	virtual void ShowPreviewForTextFile(const wchar_t* fileName);
	virtual void ShowPreviewForHtmlFile(const wchar_t* fileName);

	virtual void ShowPrinterSetupDialog();

	virtual bool IsPrintPreviewSupported(){ return false; }
	virtual bool IsReducedHTMLSupported(){ return false; }
	virtual bool IsPrinterSetupDialogSupported(){ return false; }
};
