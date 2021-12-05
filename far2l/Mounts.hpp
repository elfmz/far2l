#pragma once

#include "FARString.hpp"

namespace Mounts
{
	struct Entry
	{
		Entry() = default;
		Entry(const Entry&) = default;

		inline Entry(FARString path_, FARString info_, bool unmountable_ = false) : path(path_), info(info_)
		{
			unmountable = unmountable_;
		}

		FARString path;
		FARString usage;
		FARString info;
		bool unmountable = false;
		WCHAR hotkey = 0;
	};

	struct Enum : std::vector<Entry>
	{
		Enum(FARString &another_curdir);

		size_t max_path = 4;
		size_t max_usage = 0;
		size_t max_info = 4;
	};

	bool Unmount(const FARString &path, bool force);
	void EditHotkey(const FARString &path);
}

//BOOL RemoveUSBDrive(char Letter,DWORD Flags);
//BOOL IsDriveUsb(wchar_t DriveName,void *pDevInst);
