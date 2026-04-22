#include "SDLPrinterSupport.h"

#include <utils.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <fstream>
#include "WideMB.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#include <pthread.h>
#include "Mac/printing.h"
#endif

SDLPrinterSupportBackend::SDLPrinterSupportBackend()
{
	fprintf(stderr, "print:: initialized (SDL)\n");
}

SDLPrinterSupportBackend::~SDLPrinterSupportBackend() {}

#if defined(__APPLE__)
struct MainThreadCall {
	std::function<void()> fn;
};

static void RunOnMainThread(std::function<void()> fn)
{
	if (pthread_main_np()) {
		fn();
	} else {
		MainThreadCall call{std::move(fn)};
		dispatch_sync_f(dispatch_get_main_queue(), &call, [](void *ctx) {
			auto *call_ptr = static_cast<MainThreadCall*>(ctx);
			call_ptr->fn();
		});
	}
}
#endif

void SDLPrinterSupportBackend::PrintText(const wchar_t* jobName, const wchar_t* text)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintTextW(text ? text : L"");
	});
#else
	char tmpl[] = "/tmp/far2l-editor-print-fragmentXXXXXX";
	int fd = mkstemp(tmpl);
	if (fd < 0) {
		return;
	}

	FILE* fp = fdopen(fd, "a+");
	if (!fp) {
		close(fd);
		return;
	}
	fprintf(fp, "%ls\n", text);
	fclose(fp);

	char buf[MAX_PATH];
	sprintf(buf, "lp -s -t \"%ls\" %s", jobName ? jobName : L"", tmpl);
	fprintf(stderr, "print:: `%s`\n", buf);
	system(buf);
#endif
}

void SDLPrinterSupportBackend::PrintTextFile(const wchar_t* fileName)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintTextFileW(fileName ? fileName : L"");
	});
#else
	if (!fileName || !*fileName) {
		return;
	}
	char buf[MAX_PATH];
	sprintf(buf, "lp -s %ls", fileName);
	fprintf(stderr, "print:: `%s`\n", buf);
	system(buf);
#endif
}

void SDLPrinterSupportBackend::PrintHtmlFile(const wchar_t* fileName)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintHtmlFileW(fileName ? fileName : L"");
	});
#else
	PrintTextFile(fileName);
#endif
}

void SDLPrinterSupportBackend::PrintReducedHTML(const wchar_t* jobName, const wchar_t* text)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintHtmlW(text ? text : L"");
	});
#else
	PrintText(jobName, text);
#endif
}

void SDLPrinterSupportBackend::ShowPreviewForText(const wchar_t* jobName, const wchar_t* text)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintPreviewTextW(text ? text : L"");
	});
#else
	(void)jobName;
	const std::string content = Wide2MB(text ? text : L"");
	char tmpl[] = "/tmp/far2l-print-previewXXXXXX.html";
	int fd = mkstemp(tmpl);
	if (fd < 0) {
		return;
	}
	close(fd);
	std::ofstream out(tmpl);
	if (!out.is_open()) {
		return;
	}
	out << "<!doctype html><meta charset=\"utf-8\"><title>Print Preview</title><pre>";
	for (char c : content) {
		switch (c) {
			case '&': out << "&amp;"; break;
			case '<': out << "&lt;"; break;
			case '>': out << "&gt;"; break;
			default: out << c; break;
		}
	}
	out << "</pre>\n";
	out.close();
	char cmd[MAX_PATH];
	snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", tmpl);
	system(cmd);
#endif
}

void SDLPrinterSupportBackend::ShowPreviewForReducedHTML(const wchar_t* jobName, const wchar_t* text)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintPreviewHtmlW(text ? text : L"");
	});
#else
	(void)jobName;
	const std::string content = Wide2MB(text ? text : L"");
	char tmpl[] = "/tmp/far2l-print-previewXXXXXX.html";
	int fd = mkstemp(tmpl);
	if (fd < 0) {
		return;
	}
	close(fd);
	std::ofstream out(tmpl);
	if (!out.is_open()) {
		return;
	}
	out << content;
	out.close();
	char cmd[MAX_PATH];
	snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", tmpl);
	system(cmd);
#endif
}

void SDLPrinterSupportBackend::ShowPreviewForTextFile(const wchar_t* fileName)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintPreviewTextFileW(fileName ? fileName : L"");
	});
#else
	if (!fileName || !*fileName) {
		return;
	}
	const std::string path = Wide2MB(fileName);
	std::ifstream in(path);
	if (!in.is_open()) {
		return;
	}
	std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();
	char tmpl[] = "/tmp/far2l-print-previewXXXXXX.html";
	int fd = mkstemp(tmpl);
	if (fd < 0) {
		return;
	}
	close(fd);
	std::ofstream out(tmpl);
	if (!out.is_open()) {
		return;
	}
	out << "<!doctype html><meta charset=\"utf-8\"><title>Print Preview</title><pre>";
	for (char c : content) {
		switch (c) {
			case '&': out << "&amp;"; break;
			case '<': out << "&lt;"; break;
			case '>': out << "&gt;"; break;
			default: out << c; break;
		}
	}
	out << "</pre>\n";
	out.close();
	char cmd[MAX_PATH];
	snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", tmpl);
	system(cmd);
#endif
}

void SDLPrinterSupportBackend::ShowPreviewForHtmlFile(const wchar_t* fileName)
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativePrintPreviewHtmlFileW(fileName ? fileName : L"");
	});
#else
	if (!fileName || !*fileName) {
		return;
	}
	const std::string path = Wide2MB(fileName);
	char cmd[MAX_PATH];
	snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", path.c_str());
	system(cmd);
#endif
}

void SDLPrinterSupportBackend::ShowPrinterSetupDialog()
{
#if defined(__APPLE__)
	RunOnMainThread([&] {
		MacNativeShowPageSetupDialogW();
	});
#endif
}

bool SDLPrinterSupportBackend::IsPrintPreviewSupported(){
#if defined(__APPLE__)
	return true;
#else
	return true;
#endif
}
bool SDLPrinterSupportBackend::IsReducedHTMLSupported(){
#if defined(__APPLE__)
	return true;
#else
	return true;
#endif
}
bool SDLPrinterSupportBackend::IsPrinterSetupDialogSupported(){
#if defined(__APPLE__)
	return true;
#else
	return false;
#endif
}
