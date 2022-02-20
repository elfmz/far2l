#pragma once
#include <WinCompat.h>
#include "FARString.hpp"

struct IFileHolder
{
	virtual ~IFileHolder() {}
	virtual void OnFileEdited(const wchar_t *FileName) = 0;
};

struct DummyFileHolder : IFileHolder
{
	virtual void OnFileEdited(const wchar_t *FileName) {}
};

class TempFileHolder : public DummyFileHolder
{
	FARString _temp_file_name;
	enum {
		DONT_DELETE = 0,
		DELETE_FILE,
		DELETE_FILE_AND_PARENT_DIR
	} _delete = DONT_DELETE;

	TempFileHolder(const TempFileHolder &) = delete;
	TempFileHolder &operator =(const TempFileHolder &) = delete;

protected:
	virtual void OnFileEdited(const wchar_t *FileName);

public:
	const FARString &TempFileName() const { return _temp_file_name; }

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

	void UploadIfTimestampChanged();
};
