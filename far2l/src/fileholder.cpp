#include <windows.h>
#include <sudo.h>
#include <stdio.h>
#include <mutex>
#include <set>
#include "headers.hpp"
#include "delete.hpp"
#include "farwinapi.hpp"
#include "fileholder.hpp"
#include "pathmix.hpp"

static struct TempFileHolders : std::set<TempFileHolder *>, std::mutex
{
} s_temp_file_holders;

///////////

FileHolder::FileHolder(const FARString &file_path_name, bool temporary)
	: _file_path_name(file_path_name), _temporary(temporary)
{
	if (!_file_path_name.IsEmpty()
		&& StrCmp(_file_path_name, Msg::NewFileName)
		&& !IsPluginPrefixPath(_file_path_name.CPtr())) {
			ConvertHomePrefixInPath(_file_path_name);
			ConvertNameToFull(_file_path_name);
	}
}

FileHolder::~FileHolder()
{
}

void FileHolder::OnFileEdited(const wchar_t *FileName)
{
}

void FileHolder::CheckForChanges()
{
}

////////////

TempFileHolder::TempFileHolder(const FARString &temp_file_name, bool delete_parent_dir)
	:
	FileHolder(temp_file_name, true), _delete(delete_parent_dir ? DELETE_FILE_AND_PARENT_DIR : DELETE_FILE)
{
	std::lock_guard<std::mutex> lock(s_temp_file_holders);
	s_temp_file_holders.insert(this);
}

TempFileHolder::~TempFileHolder()
{
	{
		std::lock_guard<std::mutex> lock(s_temp_file_holders);
		s_temp_file_holders.erase(this);
		/*
			if somehow there'is some another holder for same file then
			don't delete it now, let that another delete it on release
		*/
		for (const auto &another : s_temp_file_holders) {
			if (another->_file_path_name == _file_path_name) {
				_delete = DONT_DELETE;
			}
		}
	}

	if (_delete == DELETE_FILE) {
		apiMakeWritable(_file_path_name);
		apiDeleteFile(_file_path_name);

	} else if (_delete == DELETE_FILE_AND_PARENT_DIR) {
		DeleteFileWithFolder(_file_path_name);
	}
}

void TempFileHolder::OnFileEdited(const wchar_t *FileName)
{
	if (_file_path_name == FileName) {
		/*
			$ 11.10.2001 IS
			Если было произведено сохранение с любым результатом, то не удалять файл
		*/
		_delete = DONT_DELETE;
	}
}

///////////////////////////////////////////////////////////////////////

TempFileUploadHolder::TempFileUploadHolder(const FARString &temp_file_name, bool delete_parent_dir)
	:
	TempFileHolder(temp_file_name, delete_parent_dir)
{
	GetCurrentTimestamp();
}

TempFileUploadHolder::~TempFileUploadHolder()
{
}

void TempFileUploadHolder::GetCurrentTimestamp()
{
	struct stat s{};
	if (sdc_stat(_file_path_name.GetMB().c_str(), &s) == 0) {
		_mtim = s.st_mtim;
	}
}

void TempFileUploadHolder::OnFileEdited(const wchar_t *FileName)
{
	if (_file_path_name != FileName) {
		fprintf(stderr, "TempFileUploadHolder::OnFileEdited: '%ls' != '%ls'\n", _file_path_name.CPtr(), FileName);
		return;
	}

	if (UploadTempFile()) {
		GetCurrentTimestamp();
	}

	// dont invoke TempFileHolder::OnFileEdited as uploadedable files deleted even if edited
}

void TempFileUploadHolder::CheckForChanges()
{
	struct stat s{};
	if (sdc_stat(_file_path_name.GetMB().c_str(), &s) == 0
			&& (_mtim.tv_sec != s.st_mtim.tv_sec || _mtim.tv_nsec != s.st_mtim.tv_nsec)) {
		if (UploadTempFile()) {
			_mtim = s.st_mtim;
		}
	}
}
