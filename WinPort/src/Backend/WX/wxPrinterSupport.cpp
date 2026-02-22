#include <wx/wx.h>
#include <wx/display.h>

#if defined(__APPLE__)
#define MAC_NATIVE_PRINTING // comment this define if you want wxWidgets-aware dialogs
#endif

#ifndef MAC_NATIVE_PRINTING
#include <wx/html/htmprint.h>
#include <wx/richtext/richtextprint.h>
#include <wx/event.h>
#endif

#include <string.h>

#include "wxPrinterSupport.h"
#include "CallInMain.h"

#ifdef MAC_NATIVE_PRINTING
#include "Mac/printing.h"
#endif

class PreviewWatcher : public wxEvtHandler
{
public:
    PreviewWatcher(wxWindow* parent) : m_parent(parent) 
    {
        wxTheApp->Bind(wxEVT_CREATE, &PreviewWatcher::OnWindowCreate, this);
    }

    ~PreviewWatcher()
    {
        wxTheApp->Unbind(wxEVT_CREATE, &PreviewWatcher::OnWindowCreate, this);
    }

private:
    void OnWindowCreate(wxWindowCreateEvent& evt)
    {
        wxWindow* win = evt.GetWindow();

       	wxClassInfo* ci =  win->GetClassInfo();
        if (ci) {
	        fprintf(stderr, "New window created: %p, class info => %p, class name = %ls\n", win, ci, ci->GetClassName());
        }

        if (win->IsTopLevel() && ci && wxString(ci->GetClassName()).Contains("wxPreviewFrame"))
        {
            // Make it behave like a floating tool window
            long style = win->GetWindowStyle();
            style |= wxSTAY_ON_TOP;
            style |= wxFRAME_TOOL_WINDOW;
            win->SetWindowStyle(style);

            win->Bind(wxEVT_SHOW, &PreviewWatcher::OnPreviewShown, this);
        }

        evt.Skip();
    }

    void OnPreviewShown(wxShowEvent& evt)
    {
        wxWindow* win = static_cast<wxWindow*>(evt.GetEventObject());

        if (evt.IsShown())
        {
	        fprintf(stderr, "New window is shown: %p\n", win);

            win->Raise();          // Now safe
            win->SetFocus();       // Now safe
            win->CentreOnParent(); // Now safe

            // m_parent->Enable(true);
            wxWindow* top = wxTheApp->GetTopWindow();
	        fprintf(stderr, "Top: %p\n", win);
            top->Enable(true);
	        fprintf(stderr, "Top: %p => enabled\n", win);

            // We're done — remove the watcher
            win->Unbind(wxEVT_SHOW, &PreviewWatcher::OnPreviewShown, this);
            delete this;
        }

        evt.Skip();
    }

    wxWindow* m_parent;
};

wxPrinterSupportBackend::wxPrinterSupportBackend() {
#ifndef MAC_NATIVE_PRINTING
	wxWindow* top = wxTheApp->GetTopWindow();
	html_printer = new wxHtmlEasyPrinting("Printing", top);
	html_printer->SetStandardFonts(10 /*, "Arial", "Lucida Console" */);
	new PreviewWatcher(wxTheApp->GetTopWindow());
#endif
}

wxPrinterSupportBackend::~wxPrinterSupportBackend() {
#ifndef MAC_NATIVE_PRINTING
	delete html_printer;
#endif
}

void wxPrinterSupportBackend::PrintText(const wchar_t* jobName, const wchar_t* text)
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

void wxPrinterSupportBackend::PrintReducedHTML(const wchar_t* jobName, const wchar_t* text)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxPrinterSupportBackend::PrintReducedHTML, this, jobName, text);
		CallInMainNoRet(fn);
		return;
	}

#ifndef MAC_NATIVE_PRINTING
	html_printer->PrintText(text);
#else
	wxString wxText(text); 
	MacNativePrintHtml(wxText);
#endif
}

void wxPrinterSupportBackend::PrintTextFile(const wchar_t* fileName)
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

void wxPrinterSupportBackend::PrintHtmlFile(const wchar_t* fileName)
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

void wxPrinterSupportBackend::ShowPreviewForText(const wchar_t* jobName, const wchar_t* text)
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

void wxPrinterSupportBackend::ShowPreviewForReducedHTML(const wchar_t* jobName, const wchar_t* text)
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

void wxPrinterSupportBackend::ShowPreviewForTextFile(const wchar_t* fileName)
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

void wxPrinterSupportBackend::ShowPreviewForHtmlFile(const wchar_t* fileName)
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
