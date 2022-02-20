#include <windows.h>
#include <sudo.h>
#include <stdio.h>
#include <mutex>
#include <set>
#include "headers.hpp"
#include "delete.hpp"
#include "farwinapi.hpp"
#include "fileholder.hpp"


static struct TempFileHolders : std::set<TempFileHolder *>, std::mutex
{
} s_temp_file_holders;

TempFileHolder::TempFileHolder(const FARString &temp_file_name, bool delete_parent_dir)
	: _temp_file_name(temp_file_name),
	_delete(delete_parent_dir ? DELETE_FILE_AND_PARENT_DIR : DELETE_FILE)
{
	std::lock_guard<std::mutex> lock(s_temp_file_holders);
	s_temp_file_holders.insert(this);
}

TempFileHolder::~TempFileHolder()
{
	{
		std::lock_guard<std::mutex> lock(s_temp_file_holders);
		s_temp_file_holders.erase(this);
		// if somehow there'is some another holder for same file then
		// don't delete it now, let that another delete it on release
		for (const auto &another : s_temp_file_holders) {
			if (another->_temp_file_name == _temp_file_name) {
				_delete = DONT_DELETE;
			}
		}
	}

	if (_delete == DELETE_FILE) {
		apiMakeWritable(_temp_file_name);
		apiDeleteFile(_temp_file_name);

	} else if (_delete == DELETE_FILE_AND_PARENT_DIR) {
		DeleteFileWithFolder(_temp_file_name);
	}
}

void TempFileHolder::OnFileEdited(const wchar_t *FileName)
{
	if (TempFileName() == FileName) {
		/* $ 11.10.2001 IS
		   Если было произведено сохранение с любым результатом, то не удалять файл
		*/
		_delete = DONT_DELETE;
	}
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
	if (TempFileName() != FileName) {
		fprintf(stderr, "TempFileUploadHolder::OnFileEdited: '%ls' != '%ls'\n", TempFileName().CPtr(), FileName);
		return;
	}

	if (UploadTempFile()) {
		GetCurrentTimestamp();
	}

	// dont invoke TempFileHolder::OnFileEdited as uploadedable files deleted even if edited
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
