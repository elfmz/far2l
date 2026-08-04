#include <windows.h>
#include <sudo.h>
#include <stdio.h>
#include <mutex>
#include <set>
#include "headers.hpp"
#include "ctrlobj.hpp"
#include "delete.hpp"
#include "dirmix.hpp"
#include "farwinapi.hpp"
#include "filepanels.hpp"
#include "fileholder.hpp"
#include "filelist.hpp"
#include "message.hpp"
#include "panelmix.hpp"
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

///////////////////////////////////////////////////////////////////////

PluginTempFileHolder::PluginTempFileHolder(const FARString &temp_file_name, HANDLE hPlugin_)
	:
	TempFileUploadHolder(temp_file_name), hPlugin(hPlugin_)
{
	CtrlObject->Plugins.RetainPlugin(hPlugin);
}

PluginTempFileHolder::~PluginTempFileHolder()
{
	CtrlObject->Plugins.ClosePlugin(hPlugin);
}

bool PluginTempFileHolder::UploadTempFile()
{
	PutCode = 0;
	FARString strSaveDir;
	apiGetCurrentDirectory(strSaveDir);

	FARString strPath = _file_path_name;

	if (apiGetFileAttributes(strPath) == INVALID_FILE_ATTRIBUTES) {
		FARString strFindName;
		CutToSlash(strPath, false);
		strFindName = strPath + L"*";
		FAR_FIND_DATA_EX FindData;
		::FindFile Find(strFindName);
		while (Find.Get(FindData)) {
			if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				strPath+= FindData.strFileName;
				break;
			}
		}
	}

	bool out = false;

	PluginPanelItem PanelItem;
	if (FileList::FileNameToPluginItem(strPath, &PanelItem)) {
		PutCode = CtrlObject->Plugins.PutFiles(hPlugin, &PanelItem, 1, FALSE, OPM_EDIT);

		if (PutCode != 0)
			out = true;

		FileList::FreePluginPanelItem(&PanelItem);
	}

	if (!out) {
		Message(MSG_WARNING, 1, Msg::Error, Msg::CannotSaveFile, Msg::TextSavedToTemp, strPath.CPtr(),
				Msg::Ok);
	}

	FarChDir(strSaveDir);

	if (out) {
		CheckPanelUpdate(CtrlObject->Cp()->LeftPanel);
		CheckPanelUpdate(CtrlObject->Cp()->RightPanel);
	}

	return out;
}

void PluginTempFileHolder::CheckPanelUpdate(Panel *panel)
{
	if (panel && panel->GetPluginHandle() == hPlugin) {
		ShellUpdatePanels(panel, FALSE);
	}
}
