#pragma once
#ifndef __FAR2SDK_FARCOMMON_H__
#define __FAR2SDK_FARCOMMON_H__

typedef int FarLangMsgID;
typedef uint32_t FarKey;

#ifndef FARLANGMSGID_BAD
# define FARLANGMSGID_BAD ((FarLangMsgID)-1)
#endif

#ifndef CP_AUTODETECT
# define CP_AUTODETECT ((UINT)-1)
#endif

enum VTLogExportFlags
{
	VT_LOGEXPORT_COLORED          = 0x00000001,
	VT_LOGEXPORT_WITH_SCREENLINES = 0x00000002
};

// returns actual handles count, if returned value > count argument - then need to extend array buffer and retry
typedef SIZE_T (WINAPI *FARAPIVT_ENUM_BACKGROUND)(HANDLE *con_hnds, SIZE_T count);

#endif // __FAR2SDK_FARCOMMON_H__
