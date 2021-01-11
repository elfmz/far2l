#pragma once

namespace Locations
{
	struct Entry
	{
		FARString path;
		FARString text;
	};

	typedef std::vector<Entry> Entries;

	void Enum(Entries &out, const FARString &curdir, const FARString &another_curdir);
	bool Unmount(FARString &path, bool force);
}

//BOOL RemoveUSBDrive(char Letter,DWORD Flags);
//BOOL IsDriveUsb(wchar_t DriveName,void *pDevInst);
