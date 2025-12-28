#include <wx/wx.h>
#include <wx/display.h>
#include <wx/html/htmprint.h>
#include <wx/richtext/richtextprint.h>

#include <string.h>

#include "wxPrinterSupport.h"

wxPrinterSupportBackend::wxPrinterSupportBackend() {}
wxPrinterSupportBackend::~wxPrinterSupportBackend() {}

static wxHtmlEasyPrinting html_printer;
static wxRichTextPrinting rtf_printer;

void wxPrinterSupportBackend::PrintText(const std::wstring& jobName, const std::wstring& text)
{
	wxRichTextBuffer buffer; 
	wxString wxText(text); 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	
	for (auto& line : lines) buffer.AddParagraph(line);
	rtf_printer.PrintBuffer(buffer);
}

void wxPrinterSupportBackend::PrintReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	html_printer.PrintText(text);
}

void wxPrinterSupportBackend::PrintTextFile(const std::wstring& fileName)
{
	rtf_printer.PrintFile(fileName);
}

void wxPrinterSupportBackend::PrintHtmlFile(const std::wstring& fileName)
{
	html_printer.PrintFile(fileName);
}

void wxPrinterSupportBackend::ShowPreviewForText(const std::wstring& jobName, const std::wstring& text)
{
	wxRichTextBuffer buffer; 
	wxString wxText(text); 
	wxArrayString lines = wxSplit(wxText, '\n'); 
	
	for (auto& line : lines) buffer.AddParagraph(line);
	rtf_printer.PreviewBuffer(buffer);
}

void wxPrinterSupportBackend::ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	html_printer.PreviewText(text);
}

void wxPrinterSupportBackend::ShowPreviewForTextFile(const std::wstring& fileName)
{
	rtf_printer.PreviewFile(fileName);
}

void wxPrinterSupportBackend::ShowPreviewForHtmlFile(const std::wstring& fileName)
{
	html_printer.PreviewFile(fileName);
}

void wxPrinterSupportBackend::ShowPrinterSetupDialog()
{
	html_printer.PageSetup();
}
