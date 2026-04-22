#pragma once
#include "Backend.h"

class SDLPrinterSupportBackend : public IPrinterSupport
{
public:
	SDLPrinterSupportBackend();
	virtual ~SDLPrinterSupportBackend();

	virtual void PrintText(const wchar_t* jobName, const wchar_t* text);
	virtual void PrintReducedHTML(const wchar_t* jobName, const wchar_t* text);
	virtual void PrintTextFile(const wchar_t* fileName);
	virtual void PrintHtmlFile(const wchar_t* fileName);

	virtual void ShowPreviewForText(const wchar_t* jobName, const wchar_t* text);
	virtual void ShowPreviewForReducedHTML(const wchar_t* jobName, const wchar_t* text);
	virtual void ShowPreviewForTextFile(const wchar_t* fileName);
	virtual void ShowPreviewForHtmlFile(const wchar_t* fileName);

	virtual void ShowPrinterSetupDialog();

	virtual bool IsPrintPreviewSupported();
	virtual bool IsReducedHTMLSupported();
	virtual bool IsPrinterSetupDialogSupported();
};
