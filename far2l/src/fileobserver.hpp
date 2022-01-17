#pragma once
#include <WinCompat.h>
#include "FARString.hpp"

struct IFileObserver
{
	virtual ~IFileObserver() {}
	virtual void OnFileEdited(const wchar_t *FileName) = 0;
};

struct TempFileHolder : IFileObserver
{
	struct timespec mtim{};

	void GetCurrentTimestamp();
	virtual void OnFileEdited(const wchar_t *FileName);

protected:
	FARString strTempFileName;

	virtual void UploadTempFile() = 0;

public:
	TempFileHolder(const FARString &strTempFileName_);
	virtual ~TempFileHolder();
	void UploadIfTimestampChanged();
};
