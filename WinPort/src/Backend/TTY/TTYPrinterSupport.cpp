#include "TTYPrinterSupport.h"
#include <utils.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

ttyPrinterSupportBackend::ttyPrinterSupportBackend() {}
ttyPrinterSupportBackend::~ttyPrinterSupportBackend() {}

void ttyPrinterSupportBackend::PrintText(const std::wstring& jobName, const std::wstring& text)
{
	char tmpl[] = "/tmp/far2l-editor-print-fragmentXXXXXX";
	int fd = mkstemp(tmpl);
	FILE* fp = fdopen(fd, "a+");

	fprintf(fp, "%ls\n", text.c_str());

	fclose(fp);

	char buf[MAX_PATH];
	sprintf(buf, "lp \'%s\'", tmpl);
	system(buf);
}

void ttyPrinterSupportBackend::PrintTextFile(const std::wstring& fileName)
{
	char buf[MAX_PATH];
	sprintf(buf, "lp \'%ls\'", fileName.c_str());
	system(buf);
}

// The rest is not implemented

void ttyPrinterSupportBackend::PrintHtmlFile(const std::wstring& fileName)
{

}

void ttyPrinterSupportBackend::PrintReducedHTML(const std::wstring& jobName, const std::wstring&  text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForText(const std::wstring& jobName, const std::wstring& text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
}

void ttyPrinterSupportBackend::ShowPreviewForTextFile(const std::wstring& fileName)
{
}

void ttyPrinterSupportBackend::ShowPreviewForHtmlFile(const std::wstring& fileName)
{
}

void ttyPrinterSupportBackend::ShowPrinterSetupDialog()
{
}
