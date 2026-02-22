#include "TTYPrinterSupport.h"
#include <utils.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

ttyPrinterSupportBackend::ttyPrinterSupportBackend() {
	fprintf(stderr, "print:: initialized\n");
}

ttyPrinterSupportBackend::~ttyPrinterSupportBackend() {}

void ttyPrinterSupportBackend::PrintText(const wchar_t* jobName, const wchar_t* text)
{
	char tmpl[] = "/tmp/far2l-editor-print-fragmentXXXXXX";
	int fd = mkstemp(tmpl);

	FILE* fp = fdopen(fd, "a+");
	fprintf(fp, "%ls\n", text);
	fclose(fp);

	char buf[MAX_PATH];
	sprintf(buf, "lp -s -t \"%ls\" %s", jobName, tmpl);
	fprintf(stderr, "print:: `%s`\n", buf);
	system(buf);
}

void ttyPrinterSupportBackend::PrintTextFile(const wchar_t* fileName)
{
	char buf[MAX_PATH];
	sprintf(buf, "lp -s %ls", fileName);
	fprintf(stderr, "print:: `%s`\n", buf);
	system(buf);
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
