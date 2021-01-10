#pragma once

namespace Locations
{
	enum Kind
	{
		UNSPECIFIED = 0,
		FAVORITE,
		MOUNTPOINT,
	};

	struct Entry
	{
		Kind kind = UNSPECIFIED;

		FARString path;
		FARString text;
	};

	typedef std::vector<Entry> Entries;

	void Enum(Entries &out, const FARString &curdir, const FARString &another_curdir);

	bool Unmount(FARString &path, bool force);

	bool AddFavorite();
	bool EditFavorite(FARString &path);
	bool RemoveFavorite(FARString &path);
	
	void Show();
}

//BOOL RemoveUSBDrive(char Letter,DWORD Flags);
//BOOL IsDriveUsb(wchar_t DriveName,void *pDevInst);
