#include <windows.h>
#include <sudo.h>
#include <stdio.h>
#include "headers.hpp"
#include "delete.hpp"
#include "farwinapi.hpp"
#include "fileholder.hpp"

TempFileHolder::TempFileHolder(const FARString &temp_file_name, bool delete_parent_dir)
	: _temp_file_name(temp_file_name), _delete_parent_dir(delete_parent_dir)
{
}

TempFileHolder::~TempFileHolder()
{
	if (!_delete_parent_dir) {
		apiMakeWritable(_temp_file_name);
		apiDeleteFile(_temp_file_name);
	}
	else
		DeleteFileWithFolder(_temp_file_name);
}

///////////////////////////////////////////////////////////////////////

TempFileUploadHolder::TempFileUploadHolder(const FARString &temp_file_name, bool delete_parent_dir)
	: TempFileHolder(temp_file_name, delete_parent_dir)
{
	GetCurrentTimestamp();
}

void TempFileUploadHolder::GetCurrentTimestamp()
{
	struct stat s{};
	if (sdc_stat(TempFileName().GetMB().c_str(), &s) == 0) {
		_mtim = s.st_mtim;
	}
}

void TempFileUploadHolder::OnFileEdited(const wchar_t *FileName)
{
	if (TempFileName() != FileName)
	{
		fprintf(stderr, "TempFileUploadHolder::OnFileEdited: '%ls' != '%ls'\n", TempFileName().CPtr(), FileName);
		return;
	}

	if (UploadTempFile()) {
		GetCurrentTimestamp();
	}
}

void TempFileUploadHolder::UploadIfTimestampChanged()
{
	struct stat s{};
	if (sdc_stat(TempFileName().GetMB().c_str(), &s) == 0
	 && (_mtim.tv_sec != s.st_mtim.tv_sec || _mtim.tv_nsec != s.st_mtim.tv_nsec))
	{
		if (UploadTempFile()) {
			_mtim = s.st_mtim;
		}
	}
}
