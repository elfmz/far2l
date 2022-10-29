#pragma once

#include "FARString.hpp"

namespace Mounts
{
	struct Entry
	{
		Entry() = default;
		Entry(const Entry&) = default;

		inline Entry(FARString path_, const wchar_t *col3_, bool unmountable_ = false, INT_PTR id_ = -1) : path(path_), col3(col3_)
		{
			unmountable = unmountable_;
			id = id_;
		}

		FARString path;
		FARString col2;
		FARString col3;
		bool unmountable = false;
		WCHAR hotkey = 0;
		int id = -1;
	};

	class Enum : public std::vector<Entry>
	{
		void AddMounts(bool &has_rootfs);
		void AddFavorites(bool &has_rootfs);

	public:
		Enum(FARString &another_curdir);

		size_t max_path = 4;
		size_t max_col2 = 0;
		size_t max_col3 = 4;
	};

	bool Unmount(const FARString &path, bool force);
	void EditHotkey(const FARString &path, int id);
}

//BOOL RemoveUSBDrive(char Letter,DWORD Flags);
//BOOL IsDriveUsb(wchar_t DriveName,void *pDevInst);
