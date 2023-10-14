#pragma once
#include <WinCompat.h>
#include <memory>
#include "FARString.hpp"

typedef std::shared_ptr<class FileHolder> FileHolderPtr;

class FileHolder
{
protected:
	FARString _file_path_name;
	bool _temporary;

public:
	FileHolder(const FARString &file_path_name, bool temporary = false);
	virtual ~FileHolder();

	bool IsTemporary() const { return _temporary; }
	const FARString &GetPathName() const { return _file_path_name; }

	virtual void OnFileEdited(const wchar_t *FileName);
	virtual void CheckForChanges();
};

class TempFileHolder : public FileHolder
{
	enum
	{
		DONT_DELETE = 0,
		DELETE_FILE,
		DELETE_FILE_AND_PARENT_DIR
	} _delete = DONT_DELETE;

	TempFileHolder(const TempFileHolder &)            = delete;
	TempFileHolder &operator=(const TempFileHolder &) = delete;

protected:
	virtual void OnFileEdited(const wchar_t *FileName);

public:
	TempFileHolder(const FARString &temp_file_name, bool delete_parent_dir = true);
	virtual ~TempFileHolder();
};

class TempFileUploadHolder : public TempFileHolder
{
	struct timespec _mtim{};

	void GetCurrentTimestamp();
	virtual void OnFileEdited(const wchar_t *FileName);

protected:
	virtual bool UploadTempFile() = 0;

public:
	TempFileUploadHolder(const FARString &temp_file_name, bool delete_parent_dir = true);
	virtual ~TempFileUploadHolder();
	void CheckForChanges();
};
