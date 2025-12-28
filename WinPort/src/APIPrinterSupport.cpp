#include <stdlib.h>
#include <string>
#include <locale>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "utils.h"
#include "Backend.h"


static IPrinterSupport *g_printer_backend = nullptr;
static std::mutex g_printer_backend_mutex;

__attribute__ ((visibility("default"))) IPrinterSupport *WinPortPrinterSupport_SetBackend(IPrinterSupport *printer_backend)
{
	std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
	auto out = g_printer_backend;
	g_printer_backend = printer_backend;
	return out;
}

extern "C" {

	WINPORT_DECL(PrintTextFragment, VOID, (LPCWSTR lpszJobName, LPCWSTR lpszTextFragment))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->PrintText(lpszJobName, lpszTextFragment);
	}

	WINPORT_DECL(PrintHtmlFragment, VOID, (LPCWSTR lpszJobName, LPCWSTR lpszTextFragment))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->PrintReducedHTML(lpszJobName, lpszTextFragment);
	}

	WINPORT_DECL(PrintTextFile, VOID, (LPCWSTR lpszFileName))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		fprintf(stderr, "print::text file:: backend=%p file=%ls\n", g_printer_backend, lpszFileName);
		if(g_printer_backend) g_printer_backend->PrintTextFile(lpszFileName);
	}

	WINPORT_DECL(PrintHtmlFile, VOID, (LPCWSTR lpszFileName))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->PrintHtmlFile(lpszFileName);
	}

	WINPORT_DECL(PrintPreviewTextFragment, VOID, (LPCWSTR lpszJobName, LPCWSTR lpszTextFragment))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->ShowPreviewForText(lpszJobName, lpszTextFragment);
	}

	WINPORT_DECL(PrintPreviewHtmlFragment, VOID, (LPCWSTR lpszJobName, LPCWSTR lpszTextFragment))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->ShowPreviewForReducedHTML(lpszJobName, lpszTextFragment);
	}

	WINPORT_DECL(PrintPreviewTextFile, VOID, (LPCWSTR lpszFileName))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->ShowPreviewForTextFile(lpszFileName);
	}

	WINPORT_DECL(PrintPreviewHtmlFile, VOID, (LPCWSTR lpszFileName))
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->ShowPreviewForHtmlFile(lpszFileName);
	}

	WINPORT_DECL(PrintSettingsDialog, VOID, ())
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		if(g_printer_backend) g_printer_backend->ShowPrinterSetupDialog();
	}

	WINPORT_DECL(PrintIsHTMLSupported, BOOL, ())
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		return g_printer_backend ? g_printer_backend->IsReducedHTMLSupported() : false;
	}

	WINPORT_DECL(PrintIsSettingsDialogSupported, BOOL, ())
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		return g_printer_backend ? g_printer_backend->IsPrinterSetupDialogSupported() : false;
	}

	WINPORT_DECL(PrintIsPreviewSupported, BOOL, ())
	{
		std::lock_guard<std::mutex> lock(g_printer_backend_mutex);
		return g_printer_backend ? g_printer_backend->IsPrintPreviewSupported() : false;
	}
}
