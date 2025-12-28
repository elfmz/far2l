#include <wx/wx.h>
#include <wx/display.h>

#if defined(__APPLE__)
#define MAC_NATIVE_PRINTING
#endif

#ifndef MAC_NATIVE_PRINTING
#include <wx/html/htmprint.h>
#include <wx/richtext/richtextprint.h>
#endif

#include <string.h>

#include "wxPrinterSupport.h"

wxPrinterSupportBackend::wxPrinterSupportBackend() {}
wxPrinterSupportBackend::~wxPrinterSupportBackend() {}

#ifndef MAC_NATIVE_PRINTING
static wxHtmlEasyPrinting html_printer;
static wxRichTextPrinting rtf_printer;
#endif

void MacNativePrintText(const wxString& text);
void MacNativePrintHtml(const wxString& html);
void MacNativePrintTextFile(const wxString& path);
void MacNativePrintHtmlFile(const wxString& path);

void MacNativeShowPageSetupDialog();

void MacNativePrintPreviewText(const wxString& text);
void MacNativePrintPreviewHtml(const wxString& html);
void MacNativePrintPreviewTextFile(const wxString& path);
void MacNativePrintPreviewHtmlFile(const wxString& path);

void wxPrinterSupportBackend::PrintText(const std::wstring& jobName, const std::wstring& text)
{
	wxString wxText(text); 

#ifndef MAC_NATIVE_PRINTING
	wxRichTextBuffer buffer; 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	
	for (auto& line : lines) buffer.AddParagraph(line);
	rtf_printer.PrintBuffer(buffer);
#else
	MacNativePrintText(wxText);
#endif
}

void wxPrinterSupportBackend::PrintReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
#ifndef MAC_NATIVE_PRINTING
	html_printer.PrintText(text);
#else
	wxString wxText(text); 
	MacNativePrintHtml(wxText);
#endif
}

void wxPrinterSupportBackend::PrintTextFile(const std::wstring& fileName)
{
#ifndef MAC_NATIVE_PRINTING
	rtf_printer.PrintFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintTextFile(wxText);
#endif
}

void wxPrinterSupportBackend::PrintHtmlFile(const std::wstring& fileName)
{
#ifndef MAC_NATIVE_PRINTING
	html_printer.PrintFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintHtmlFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForText(const std::wstring& jobName, const std::wstring& text)
{
#ifndef MAC_NATIVE_PRINTING
	wxRichTextBuffer buffer; 
	wxString wxText(text); 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	
	for (auto& line : lines) buffer.AddParagraph(line);
	rtf_printer.PreviewBuffer(buffer);
#else
	wxString wxText(text); 
	MacNativePrintPreviewText(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
#ifndef MAC_NATIVE_PRINTING
	html_printer.PreviewText(text);
#else
	wxString wxText(text); 
	MacNativePrintPreviewHtml(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForTextFile(const std::wstring& fileName)
{
#ifndef MAC_NATIVE_PRINTING
	rtf_printer.PreviewFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintPreviewTextFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPreviewForHtmlFile(const std::wstring& fileName)
{
#ifndef MAC_NATIVE_PRINTING
	html_printer.PreviewFile(fileName);
#else
	wxString wxText(fileName); 
	MacNativePrintPreviewHtmlFile(wxText);
#endif
}

void wxPrinterSupportBackend::ShowPrinterSetupDialog()
{
#ifndef MAC_NATIVE_PRINTING
	html_printer.PageSetup();
#else
	MacNativeShowPageSetupDialog();
#endif
}

bool wxPrinterSupportBackend::IsPrintPreviewSupported(){ return true; }
bool wxPrinterSupportBackend::IsReducedHTMLSupported(){ return true; }
bool wxPrinterSupportBackend::IsPrinterSetupDialogSupported(){ return true; }
