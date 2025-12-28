#include <wx/wx.h>
#include <wx/display.h>

#if defined(__APPLE__)
// For now, we're playing with wxWidgets-based code for all platforms; later we'll switch to native path for macos
// #define MAC_NATIVE_PRINTING
#endif

#ifndef MAC_NATIVE_PRINTING
#include <wx/html/htmprint.h>
#include <wx/richtext/richtextprint.h>
#endif

#include <string.h>

#include "wxPrinterSupport.h"
#include "CallInMain.h"

#ifdef MAC_NATIVE_PRINTING
#include "Mac/printing.h"
#endif


wxPrinterSupportBackend::wxPrinterSupportBackend() {
#ifndef MAC_NATIVE_PRINTING
	wxWindow* top = wxTheApp->GetTopWindow();
	html_printer = new wxHtmlEasyPrinting("Printing", top);
#endif
}
wxPrinterSupportBackend::~wxPrinterSupportBackend() {
#ifndef MAC_NATIVE_PRINTING
	delete html_printer;
#endif
}

void wxPrinterSupportBackend::PrintText(const std::wstring& jobName, const std::wstring& text)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::PrintText, this, jobName, text);
		CallInMainNoRet(fn);
		return;
	}

	wxString wxText(text); 

#ifndef MAC_NATIVE_PRINTING
	wxRichTextBuffer buffer; 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	
	for (auto& line : lines) buffer.AddParagraph(line);

	wxRichTextPrinting rtf_printer(jobName, wxTheApp->GetTopWindow());
	rtf_printer.PrintBuffer(buffer);
#else
	MacNativePrintText(wxText);
#endif
}

void wxPrinterSupportBackend::PrintReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::PrintReducedHTML, this, jobName, text);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	//wxHtmlEasyPrinting html_printer(jobName);
	//html_printer.PrintText(text);
	html_printer->PrintText(text);
#else
	wxString wxText(text); 
	MacNativePrintHtml(wxText);
#endif
}

void wxPrinterSupportBackend::PrintTextFile(const std::wstring& fileName)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::PrintTextFile, this, fileName);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	wxRichTextPrinting rtf_printer(fileName, wxTheApp->GetTopWindow());
	rtf_printer.PrintFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintTextFile(wxText);
#endif
}

void wxPrinterSupportBackend::PrintHtmlFile(const std::wstring& fileName)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::PrintHtmlFile, this, fileName);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	// wxHtmlEasyPrinting html_printer(fileName);
	// html_printer.PrintFile(fileName);
	html_printer->PrintFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintHtmlFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForText(const std::wstring& jobName, const std::wstring& text)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::ShowPreviewForText, this, jobName, text);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	wxRichTextBuffer buffer; 
	wxString wxText(text); 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	for (auto& line : lines) buffer.AddParagraph(line);

	wxRichTextPrinting rtf_printer(jobName, wxTheApp->GetTopWindow());
	rtf_printer.PreviewBuffer(buffer);
#else
	wxString wxText(text); 
	MacNativePrintPreviewText(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::ShowPreviewForReducedHTML, this, jobName, text);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	// wxHtmlEasyPrinting html_printer(jobName);
	// html_printer.PreviewText(text);
	html_printer->PreviewText(text);
#else
	wxString wxText(text); 
	MacNativePrintPreviewHtml(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForTextFile(const std::wstring& fileName)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::ShowPreviewForTextFile, this, fileName);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	wxRichTextPrinting rtf_printer(fileName, wxTheApp->GetTopWindow());
	rtf_printer.PreviewFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintPreviewTextFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForHtmlFile(const std::wstring& fileName)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::ShowPreviewForHtmlFile, this, fileName);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	// wxHtmlEasyPrinting html_printer(fileName);
	// html_printer.PreviewFile(fileName);
	html_printer->PreviewFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintPreviewHtmlFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPrinterSetupDialog()
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::ShowPrinterSetupDialog, this);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	// wxHtmlEasyPrinting html_printer("Printer Setup");
	// html_printer.PageSetup();
	html_printer->PageSetup();
#else
	MacNativeShowPageSetupDialog();
#endif
}

bool wxPrinterSupportBackend::IsPrintPreviewSupported(){ return true; }
bool wxPrinterSupportBackend::IsReducedHTMLSupported(){ return true; }
bool wxPrinterSupportBackend::IsPrinterSetupDialogSupported(){ return true; }
