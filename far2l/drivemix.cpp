/*
drivemix.cpp

Misc functions for drive/disk info
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
#pragma hdrstop

#include "drivemix.hpp"
#include "config.hpp"
#include "cddrv.hpp"
#include "pathmix.hpp"


int CheckDisksProps(const wchar_t *SrcPath,const wchar_t *DestPath,int CheckedType)
{
	string strSrcRoot, strDestRoot;
	int SrcDriveType, DestDriveType;
	DWORD SrcVolumeNumber=0, DestVolumeNumber=0;
	string strSrcVolumeName, strDestVolumeName;
	string strSrcFileSystemName, strDestFileSystemName;
	DWORD SrcFileSystemFlags, DestFileSystemFlags;
	DWORD SrcMaximumComponentLength, DestMaximumComponentLength;
	strSrcRoot=SrcPath;
	strDestRoot=DestPath;
	//ConvertNameToUNC(strSrcRoot);
	//ConvertNameToUNC(strDestRoot);
	GetPathRoot(strSrcRoot,strSrcRoot);
	GetPathRoot(strDestRoot,strDestRoot);
	SrcDriveType=FAR_GetDriveType(strSrcRoot,nullptr,TRUE);
	DestDriveType=FAR_GetDriveType(strDestRoot,nullptr,TRUE);

	if (!apiGetVolumeInformation(strSrcRoot,&strSrcVolumeName,&SrcVolumeNumber,&SrcMaximumComponentLength,&SrcFileSystemFlags,&strSrcFileSystemName))
		return FALSE;

	if (!apiGetVolumeInformation(strDestRoot,&strDestVolumeName,&DestVolumeNumber,&DestMaximumComponentLength,&DestFileSystemFlags,&strDestFileSystemName))
		return FALSE;

	if (CheckedType == CHECKEDPROPS_ISSAMEDISK)
	{
		if (!wcspbrk(DestPath,L"/:"))
			return TRUE;

		if (((strSrcRoot.At(0)==GOOD_SLASH && strSrcRoot.At(1)==GOOD_SLASH) || (strDestRoot.At(0)==GOOD_SLASH && strDestRoot.At(1)==GOOD_SLASH)) &&
		        StrCmpI(strSrcRoot,strDestRoot))
			return FALSE;

		if (!*SrcPath || !*DestPath || (SrcPath[1]!=L':' && DestPath[1]!=L':'))  //????
			return TRUE;

		if (Upper(strDestRoot.At(0))==Upper(strSrcRoot.At(0)))
			return TRUE;

		uint64_t SrcTotalSize,SrcTotalFree,SrcUserFree;
		uint64_t DestTotalSize,DestTotalFree,DestUserFree;

		if (!apiGetDiskSize(SrcPath,&SrcTotalSize,&SrcTotalFree,&SrcUserFree))
			return FALSE;

		if (!apiGetDiskSize(DestPath,&DestTotalSize,&DestTotalFree,&DestUserFree))
			return FALSE;

		if (!(SrcVolumeNumber &&
		        SrcVolumeNumber==DestVolumeNumber &&
		        !StrCmpI(strSrcVolumeName, strDestVolumeName) &&
		        SrcTotalSize==DestTotalSize))
			return FALSE;
	}
	else if (CheckedType == CHECKEDPROPS_ISDST_ENCRYPTION)
	{
		return FALSE;
	}

	return TRUE;
}

