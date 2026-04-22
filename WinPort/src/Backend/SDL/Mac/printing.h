#pragma once

void MacNativePrintTextW(const wchar_t* text);
void MacNativePrintHtmlW(const wchar_t* html);
void MacNativePrintTextFileW(const wchar_t* path);
void MacNativePrintHtmlFileW(const wchar_t* path);

void MacNativeShowPageSetupDialogW();

void MacNativePrintPreviewTextW(const wchar_t* text);
void MacNativePrintPreviewHtmlW(const wchar_t* html);
void MacNativePrintPreviewTextFileW(const wchar_t* path);
void MacNativePrintPreviewHtmlFileW(const wchar_t* path);
