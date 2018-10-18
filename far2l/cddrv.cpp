/*
cddrv.cpp

про сидюк
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "cddrv.hpp"
#include "drivemix.hpp"
#include "pathmix.hpp"

static CDROM_DeviceCapabilities getCapsUsingProductId(const char* prodID)
{
	char productID[1024];
	int idx = 0;

	for (int i = 0; prodID[i]; i++)
	{
		char c = prodID[i];

		if (c >= 'A' && c <= 'Z')
			productID[idx++] = c;
		else if (c >= 'a' && c <= 'z')
			productID[idx++] = c - 'a' + 'A';
	}

	productID[idx] = 0;

	int caps = CAPABILITIES_NONE;

	if ( strstr(productID, "CD") )
		caps |= CAPABILITIES_GENERIC_CDROM;

	if ( strstr(productID, "CDRW") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_CDRW);

	if ( strstr(productID, "DVD") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_DVDROM);

	if ( strstr(productID, "DVDRW") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_DVDROM | CAPABILITIES_GENERIC_DVDRW);

	if ( strstr(productID, "DVDRAM") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_DVDROM | CAPABILITIES_GENERIC_DVDRW | CAPABILITIES_GENERIC_DVDRAM);

	if ( strstr(productID, "BDROM") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_DVDROM | CAPABILITIES_GENERIC_BDROM);

	if ( strstr(productID, "HDDVD") )
		caps |= (CAPABILITIES_GENERIC_CDROM | CAPABILITIES_GENERIC_DVDROM | CAPABILITIES_GENERIC_HDDVD);

	return (CDROM_DeviceCapabilities)caps;
}


CDROM_DeviceCapabilities GetDeviceCapabilities(File& Device)
{
	CDROM_DeviceCapabilities caps = CAPABILITIES_NONE;


	return caps;
}

UINT GetDeviceTypeByCaps(CDROM_DeviceCapabilities caps)
{
	if ( caps & CAPABILITIES_GENERIC_BDRW )
		return DRIVE_BD_RW;

	if ( caps & CAPABILITIES_GENERIC_BDROM )
		return DRIVE_BD_ROM;

	if ( caps & CAPABILITIES_GENERIC_HDDVDRW )
		return DRIVE_HDDVD_RW;

	if ( caps & CAPABILITIES_GENERIC_HDDVD )
		return DRIVE_HDDVD_ROM;

	if (caps & CAPABILITIES_GENERIC_DVDRAM )
		return DRIVE_DVD_RAM;

	if (caps & CAPABILITIES_GENERIC_DVDRW)
		return DRIVE_DVD_RW;

	if ((caps & CAPABILITIES_GENERIC_CDRW) && (caps & CAPABILITIES_GENERIC_DVDROM))
		return DRIVE_CD_RWDVD;

	if (caps & CAPABILITIES_GENERIC_DVDROM)
		return DRIVE_DVD_ROM;

	if (caps & CAPABILITIES_GENERIC_CDRW)
		return DRIVE_CD_RW;

	if (caps & CAPABILITIES_GENERIC_CDROM)
		return DRIVE_CDROM;

	return DRIVE_UNKNOWN;
}

bool IsDriveTypeCDROM(UINT DriveType)
{
	return DriveType == DRIVE_CDROM || (DriveType >= DRIVE_CD_RW && DriveType <= DRIVE_HDDVD_RW);
}


UINT FAR_GetDriveType(const wchar_t *RootDir, CDROM_DeviceCapabilities *Caps, DWORD Detect)
{
	FARString strRootDir;

	if (!RootDir || !*RootDir)
	{
		FARString strCurDir;
		apiGetCurrentDirectory(strCurDir);
		GetPathRoot(strCurDir, strRootDir);
	}
	else
	{
		strRootDir = RootDir;
	}

	AddEndSlash(strRootDir);

	CDROM_DeviceCapabilities caps = CAPABILITIES_NONE;

	UINT DrvType = WINPORT(GetDriveType)(strRootDir);

	// анализ CD-привода
	if ((Detect&1) && DrvType == DRIVE_CDROM)
	{
		FARString VolumePath = strRootDir;
		DeleteEndSlash(VolumePath);

		/* if (VolumePath.Equal(0, L"//?/"))
			VolumePath.Replace(0, 4, L"//./");
		else
			VolumePath.Insert(0, L"//./"); */

		File Device;
		if(Device.Open(VolumePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING))
		{
			caps = GetDeviceCapabilities(Device);
			Device.Close();

			DrvType = GetDeviceTypeByCaps(caps);
		}

		if (DrvType == DRIVE_UNKNOWN) // фигня могла кака-нить произойти, посему...
			DrvType=DRIVE_CDROM;       // ...вертаем в зад сидюк.
	}

//  if((Detect&2) && IsDriveUsb(*LocalName,nullptr)) //DrvType == DRIVE_REMOVABLE
//    DrvType=DRIVE_USBDRIVE;
//  if((Detect&4) && GetSubstName(DrvType,LocalName,nullptr,0))
//    DrvType=DRIVE_SUBSTITUTE;

	if (Caps)
		*Caps=caps;

	return DrvType;
}
