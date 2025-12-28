#pragma once

#include <wx/string.h>

void MacNativePrintText(const wxString& text);
void MacNativePrintHtml(const wxString& html);
void MacNativePrintTextFile(const wxString& path);
void MacNativePrintHtmlFile(const wxString& path);

void MacNativeShowPageSetupDialog();

void MacNativePrintPreviewText(const wxString& text);
void MacNativePrintPreviewHtml(const wxString& html);
void MacNativePrintPreviewTextFile(const wxString& path);
void MacNativePrintPreviewHtmlFile(const wxString& path);
