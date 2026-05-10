#include "TTYPrinterSupport.h"
#include <utils.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

ttyPrinterSupportBackend::ttyPrinterSupportBackend()
{
	fprintf(stderr, "ttyPrinter: initialized\n");
}

ttyPrinterSupportBackend::~ttyPrinterSupportBackend() {}

static void ttyPrinterRun(const std::string &cmd)
{
	fprintf(stderr, "ttyPrinter: cmd='%s'\n", cmd.c_str());
	int r = system(cmd.c_str());
	fprintf(stderr, "ttyPrinter: r=%d\n", r);
}

void ttyPrinterSupportBackend::PrintText(const wchar_t* jobName, const wchar_t* text)
{
	char tmpl[] = "/tmp/far2l-editor-print-fragmentXXXXXX";
	int fd = mkstemp(tmpl);

	FILE* fp = fdopen(fd, "a+");
	fprintf(fp, "%ls\n", text);
	fclose(fp);

	ttyPrinterRun(StrPrintf("lp -s -t \"%ls\" %s", jobName, tmpl));
}

void ttyPrinterSupportBackend::PrintTextFile(const wchar_t* fileName)
{
	ttyPrinterRun(StrPrintf("lp -s %ls", fileName));
}

// The rest is not implemented

void ttyPrinterSupportBackend::PrintHtmlFile(const wchar_t* fileName)
{
}

void ttyPrinterSupportBackend::PrintReducedHTML(const wchar_t* jobName, const wchar_t*  text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForText(const wchar_t* jobName, const wchar_t* text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForReducedHTML(const wchar_t* jobName, const wchar_t* text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForTextFile(const wchar_t* fileName)
{
}

void ttyPrinterSupportBackend::ShowPreviewForHtmlFile(const wchar_t* fileName)
{
}

void ttyPrinterSupportBackend::ShowPrinterSetupDialog()
{
}
