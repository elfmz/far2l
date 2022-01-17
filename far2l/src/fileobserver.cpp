#include <windows.h>
#include <sudo.h>
#include <stdio.h>
#include "delete.hpp"
#include "fileobserver.hpp"

void TempFileHolder::GetCurrentTimestamp()
{
	struct stat s{};
	if (sdc_stat(strTempFileName.GetMB().c_str(), &s) == 0)
	{
		mtim = s.st_mtim;
	}
}

void TempFileHolder::OnFileEdited(const wchar_t *FileName)
{
	if (strTempFileName != FileName)
	{
		fprintf(stderr, "TempFileHolder::OnFileEdited: '%ls' != '%ls'\n", strTempFileName.CPtr(), FileName);
		return;
	}

	UploadTempFile();
	GetCurrentTimestamp();
}

TempFileHolder::TempFileHolder(const FARString &strTempFileName_)
	: strTempFileName(strTempFileName_)
{
	GetCurrentTimestamp();
}

TempFileHolder::~TempFileHolder()
{
	DeleteFileWithFolder(strTempFileName);
}

void TempFileHolder::UploadIfTimestampChanged()
{
	struct stat s{};
	if (sdc_stat(strTempFileName.GetMB().c_str(), &s) == 0
	 && (mtim.tv_sec != s.st_mtim.tv_sec || mtim.tv_nsec != s.st_mtim.tv_nsec))
	{
		UploadTempFile();
	}
}
