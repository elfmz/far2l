/* ptp.h
 *
 * Copyright (C) 2001 Mariusz Woloszyn <emsi@ipartners.pl>
 * Copyright (C) 2003-2020 Marcus Meissner <marcus@jet.franken.de>
 * Copyright (C) 2006-2008 Linus Walleij <triad@df.lth.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __PTP_H__
#define __PTP_H__

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
#include <iconv.h>
#endif
#include "gphoto2-endian.h"
#include "device-flags.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* PTP datalayer byteorder */

#define PTP_DL_BE			0xF0
#define	PTP_DL_LE			0x0F

/* USB interface class */
#ifndef USB_CLASS_PTP
#define USB_CLASS_PTP			6
#endif

/* PTP request/response/event general PTP container (transport independent) */

struct _PTPContainer {
	uint16_t Code;
	uint32_t SessionID;
	uint32_t Transaction_ID;
	/* params  may be of any type of size less or equal to uint32_t */
	uint32_t Param1;
	uint32_t Param2;
	uint32_t Param3;
	/* events can only have three parameters */
	uint32_t Param4;
	uint32_t Param5;
	/* the number of meaningfull parameters */
	uint8_t	 Nparam;
};
typedef struct _PTPContainer PTPContainer;

/* PTP USB Bulk-Pipe container */
/* USB bulk max packet length for high speed endpoints */
/* The max packet is set to 512 bytes. The spec says
 * "end of data transfers are signaled by short packets or NULL
 * packets". It never says anything about 512, but current
 * implementations seem to have chosen this value, which also
 * happens to be the size of an USB 2.0 HS endpoint, even though
 * this is not necessary.
 *
 * Previously we had this as 4096 for MTP devices. We have found
 * and fixed the bugs that made this necessary and it can be 512 again.
 *
 * USB 3.0 has now 1024 byte EPs.
 */
#define PTP_USB_BULK_HS_MAX_PACKET_LEN_WRITE	512
#define PTP_USB_BULK_HS_MAX_PACKET_LEN_READ   512
#define PTP_USB_BULK_SS_MAX_PACKET_LEN_WRITE	1024
#define PTP_USB_BULK_SS_MAX_PACKET_LEN_READ   1024
#define PTP_USB_BULK_HDR_LEN		(2*sizeof(uint32_t)+2*sizeof(uint16_t))
#define PTP_USB_BULK_PAYLOAD_LEN_WRITE	(PTP_USB_BULK_SS_MAX_PACKET_LEN_WRITE-PTP_USB_BULK_HDR_LEN)
#define PTP_USB_BULK_PAYLOAD_LEN_READ	(PTP_USB_BULK_SS_MAX_PACKET_LEN_READ-PTP_USB_BULK_HDR_LEN)
#define PTP_USB_BULK_REQ_LEN	(PTP_USB_BULK_HDR_LEN+5*sizeof(uint32_t))

struct _PTPUSBBulkContainer {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t trans_id;
	union {
		struct {
			uint32_t param1;
			uint32_t param2;
			uint32_t param3;
			uint32_t param4;
			uint32_t param5;
		} params;
       /* this must be set to the maximum of PTP_USB_BULK_PAYLOAD_LEN_WRITE
        * and PTP_USB_BULK_PAYLOAD_LEN_READ */
		unsigned char data[PTP_USB_BULK_PAYLOAD_LEN_READ];
	} payload;
};
typedef struct _PTPUSBBulkContainer PTPUSBBulkContainer;

/* PTP USB Asynchronous Event Interrupt Data Format */
struct _PTPUSBEventContainer {
	uint32_t length;
	uint16_t type;
	uint16_t code;
	uint32_t trans_id;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
};
typedef struct _PTPUSBEventContainer PTPUSBEventContainer;

struct _PTPCanon_directtransfer_entry {
	uint32_t	oid;
	char		*str;
};
typedef struct _PTPCanon_directtransfer_entry PTPCanon_directtransfer_entry;

/* USB container types */

#define PTP_USB_CONTAINER_UNDEFINED		0x0000
#define PTP_USB_CONTAINER_COMMAND		0x0001
#define PTP_USB_CONTAINER_DATA			0x0002
#define PTP_USB_CONTAINER_RESPONSE		0x0003
#define PTP_USB_CONTAINER_EVENT			0x0004

/* PTP/IP definitions */
#define PTPIP_INIT_COMMAND_REQUEST	1
#define PTPIP_INIT_COMMAND_ACK		2
#define PTPIP_INIT_EVENT_REQUEST	3
#define PTPIP_INIT_EVENT_ACK		4
#define PTPIP_INIT_FAIL			5
#define PTPIP_CMD_REQUEST		6
#define PTPIP_CMD_RESPONSE		7
#define PTPIP_EVENT			8
#define PTPIP_START_DATA_PACKET		9
#define PTPIP_DATA_PACKET		10
#define PTPIP_CANCEL_TRANSACTION	11
#define PTPIP_END_DATA_PACKET		12
#define PTPIP_PING			13
#define PTPIP_PONG			14

struct _PTPIPHeader {
	uint32_t	length;
	uint32_t	type;
};
typedef struct _PTPIPHeader PTPIPHeader;

/* Vendor IDs */
/* List is linked from here: http://www.imaging.org/site/IST/Standards/PTP_Standards.aspx */
#define PTP_VENDOR_EASTMAN_KODAK		0x00000001
#define PTP_VENDOR_SEIKO_EPSON			0x00000002
#define PTP_VENDOR_AGILENT			0x00000003
#define PTP_VENDOR_POLAROID			0x00000004
#define PTP_VENDOR_AGFA_GEVAERT			0x00000005
#define PTP_VENDOR_MICROSOFT			0x00000006
#define PTP_VENDOR_EQUINOX			0x00000007
#define PTP_VENDOR_VIEWQUEST			0x00000008
#define PTP_VENDOR_STMICROELECTRONICS		0x00000009
#define PTP_VENDOR_NIKON			0x0000000A
#define PTP_VENDOR_CANON			0x0000000B
#define PTP_VENDOR_FOTONATION			0x0000000C
#define PTP_VENDOR_PENTAX			0x0000000D
#define PTP_VENDOR_FUJI				0x0000000E
#define PTP_VENDOR_NDD_MEDICAL_TECHNOLOGIES	0x00000012
#define PTP_VENDOR_SAMSUNG			0x0000001a
#define PTP_VENDOR_PARROT			0x0000001b
#define PTP_VENDOR_PANASONIC			0x0000001c
/* not from standards papers, but from traces: */
#define PTP_VENDOR_SONY				0x00000011 /* observed in the A900 */

/* Vendor extension ID used for MTP (occasionally, usually 6 is used) */
#define PTP_VENDOR_MTP			0xffffffff

/* gphoto overrides */
#define PTP_VENDOR_GP_OLYMPUS          0x0000fffe
#define PTP_VENDOR_GP_OLYMPUS_OMD      0x0000fffd
#define PTP_VENDOR_GP_LEICA            0x0000fffc


/* Operation Codes */

/* PTP v1.0 operation codes */
#define PTP_OC_Undefined                0x1000
#define PTP_OC_GetDeviceInfo            0x1001
#define PTP_OC_OpenSession              0x1002
#define PTP_OC_CloseSession             0x1003
#define PTP_OC_GetStorageIDs            0x1004
#define PTP_OC_GetStorageInfo           0x1005
#define PTP_OC_GetNumObjects            0x1006
#define PTP_OC_GetObjectHandles         0x1007
#define PTP_OC_GetObjectInfo            0x1008
#define PTP_OC_GetObject                0x1009
#define PTP_OC_GetThumb                 0x100A
#define PTP_OC_DeleteObject             0x100B
#define PTP_OC_SendObjectInfo           0x100C
#define PTP_OC_SendObject               0x100D
#define PTP_OC_InitiateCapture          0x100E
#define PTP_OC_FormatStore              0x100F
#define PTP_OC_ResetDevice              0x1010
#define PTP_OC_SelfTest                 0x1011
#define PTP_OC_SetObjectProtection      0x1012
#define PTP_OC_PowerDown                0x1013
#define PTP_OC_GetDevicePropDesc        0x1014
#define PTP_OC_GetDevicePropValue       0x1015
#define PTP_OC_SetDevicePropValue       0x1016
#define PTP_OC_ResetDevicePropValue     0x1017
#define PTP_OC_TerminateOpenCapture     0x1018
#define PTP_OC_MoveObject               0x1019
#define PTP_OC_CopyObject               0x101A
#define PTP_OC_GetPartialObject         0x101B
#define PTP_OC_InitiateOpenCapture      0x101C
/* PTP v1.1 operation codes */
#define PTP_OC_StartEnumHandles		0x101D
#define PTP_OC_EnumHandles		0x101E
#define PTP_OC_StopEnumHandles		0x101F
#define PTP_OC_GetVendorExtensionMaps	0x1020
#define PTP_OC_GetVendorDeviceInfo	0x1021
#define PTP_OC_GetResizedImageObject	0x1022
#define PTP_OC_GetFilesystemManifest	0x1023
#define PTP_OC_GetStreamInfo		0x1024
#define PTP_OC_GetStream		0x1025

/* Eastman Kodak extension Operation Codes */
#define PTP_OC_EK_GetSerial		0x9003
#define PTP_OC_EK_SetSerial		0x9004
#define PTP_OC_EK_SendFileObjectInfo	0x9005
#define PTP_OC_EK_SendFileObject	0x9006
#define PTP_OC_EK_SetText		0x9008

/* Canon extension Operation Codes */
#define PTP_OC_CANON_GetPartialObjectInfo	0x9001
/* 9002 - sends 2 uint32, nothing back  */
#define PTP_OC_CANON_SetObjectArchive		0x9002
#define PTP_OC_CANON_KeepDeviceOn		0x9003
#define PTP_OC_CANON_LockDeviceUI		0x9004
#define PTP_OC_CANON_UnlockDeviceUI		0x9005
#define PTP_OC_CANON_GetObjectHandleByName	0x9006
/* no 9007 observed yet */
#define PTP_OC_CANON_InitiateReleaseControl	0x9008
#define PTP_OC_CANON_TerminateReleaseControl	0x9009
#define PTP_OC_CANON_TerminatePlaybackMode	0x900A
#define PTP_OC_CANON_ViewfinderOn		0x900B
#define PTP_OC_CANON_ViewfinderOff		0x900C
#define PTP_OC_CANON_DoAeAfAwb			0x900D

/* 900e - send nothing, gets 5 uint16t in 32bit entities back in 20byte datablob */
#define PTP_OC_CANON_GetCustomizeSpec		0x900E
#define PTP_OC_CANON_GetCustomizeItemInfo	0x900F
#define PTP_OC_CANON_GetCustomizeData		0x9010
#define PTP_OC_CANON_SetCustomizeData		0x9011
#define PTP_OC_CANON_GetCaptureStatus		0x9012
#define PTP_OC_CANON_CheckEvent			0x9013
#define PTP_OC_CANON_FocusLock			0x9014
#define PTP_OC_CANON_FocusUnlock		0x9015
#define PTP_OC_CANON_GetLocalReleaseParam	0x9016
#define PTP_OC_CANON_SetLocalReleaseParam	0x9017
#define PTP_OC_CANON_AskAboutPcEvf		0x9018
#define PTP_OC_CANON_SendPartialObject		0x9019
#define PTP_OC_CANON_InitiateCaptureInMemory	0x901A
#define PTP_OC_CANON_GetPartialObjectEx		0x901B
#define PTP_OC_CANON_SetObjectTime		0x901C
#define PTP_OC_CANON_GetViewfinderImage		0x901D
#define PTP_OC_CANON_GetObjectAttributes	0x901E
#define PTP_OC_CANON_ChangeUSBProtocol		0x901F
#define PTP_OC_CANON_GetChanges			0x9020
#define PTP_OC_CANON_GetObjectInfoEx		0x9021
#define PTP_OC_CANON_InitiateDirectTransfer	0x9022
#define PTP_OC_CANON_TerminateDirectTransfer 	0x9023
#define PTP_OC_CANON_SendObjectInfoByPath 	0x9024
#define PTP_OC_CANON_SendObjectByPath 		0x9025
#define PTP_OC_CANON_InitiateDirectTansferEx	0x9026
#define PTP_OC_CANON_GetAncillaryObjectHandles	0x9027
#define PTP_OC_CANON_GetTreeInfo 		0x9028
#define PTP_OC_CANON_GetTreeSize 		0x9029
#define PTP_OC_CANON_NotifyProgress 		0x902A
#define PTP_OC_CANON_NotifyCancelAccepted	0x902B
/* 902c: no parms, read 3 uint32 in data, no response parms */
#define PTP_OC_CANON_902C			0x902C
#define PTP_OC_CANON_GetDirectory		0x902D
#define PTP_OC_CANON_902E			0x902E
#define PTP_OC_CANON_902F			0x902F	/* used during camera init */

#define PTP_OC_CANON_SetPairingInfo		0x9030
#define PTP_OC_CANON_GetPairingInfo		0x9031
#define PTP_OC_CANON_DeletePairingInfo		0x9032
#define PTP_OC_CANON_GetMACAddress		0x9033 /* no args */
/*
0000  12 00 00 00 02 00 33 90-1a 00 00 00 2c 9e fc c8  ......3.....,...
0010  33 e3                  -                         3.
 */

/* 9034: 1 param, no parms returned */
#define PTP_OC_CANON_SetDisplayMonitor		0x9034
#define PTP_OC_CANON_PairingComplete		0x9035
#define PTP_OC_CANON_GetWirelessMAXChannel	0x9036

#define PTP_OC_CANON_GetWebServiceSpec		0x9068 /* no args */
/* data returned:
0000  1e 00 00 00 02 00 68 90-1a 00 00 00 00 01 08 00  ......h.........
0010  14 00 bc ce 00 00 78 00-78 00 00 14 00 00        ......x.x.....
*/

#define PTP_OC_CANON_GetWebServiceData		0x9069 /* no args */
#define PTP_OC_CANON_SetWebServiceData		0x906A
#define PTP_OC_CANON_DeleteWebServiceData	0x906B
#define PTP_OC_CANON_GetRootCertificateSpec	0x906C /* no args */
/*
0000  12 00 00 00 02 00 6c 90-1a 00 00 00 00 01 6c 30  ......l.......l0
0010  00 00                  -                         ..
*/
#define PTP_OC_CANON_GetRootCertificateData	0x906D /* no args */
#define PTP_OC_CANON_SetRootCertificateData	0x906E
#define PTP_OC_CANON_DeleteRootCertificateData	0x906F
#define PTP_OC_CANON_GetGpsMobilelinkObjectInfo	0x9075 /* 2 args: utcstart, utcend */
#define PTP_OC_CANON_SendGpsTagInfo		0x9076 /* 1 arg: oid */
#define PTP_OC_CANON_GetTranscodeApproxSize	0x9077 /* 1 arg: oid? */
#define PTP_OC_CANON_RequestTranscodeStart	0x9078 /* 1 arg: oid? */
#define PTP_OC_CANON_RequestTranscodeCancel	0x9079 /* 1 arg: oid? */

#define PTP_OC_CANON_SetRemoteShootingMode	0x9086

/* 9101: no args, 8 byte data (01 00 00 00 00 00 00 00), no resp data. */
#define PTP_OC_CANON_EOS_GetStorageIDs		0x9101
/* 9102: 1 arg (0)
 * 0x28 bytes of data:
    00000000: 34 00 00 00 02 00 02 91 0a 00 00 00 04 00 03 00
    00000010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00000020: 00 00 ff ff ff ff 03 43 00 46 00 00 00 03 41 00
    00000030: 3a 00 00 00
 * no resp args
 */
#define PTP_OC_CANON_EOS_GetStorageInfo		0x9102
#define PTP_OC_CANON_EOS_GetObjectInfo		0x9103
#define PTP_OC_CANON_EOS_GetObject		0x9104
#define PTP_OC_CANON_EOS_DeleteObject		0x9105
#define PTP_OC_CANON_EOS_FormatStore		0x9106
#define PTP_OC_CANON_EOS_GetPartialObject	0x9107
#define PTP_OC_CANON_EOS_GetDeviceInfoEx	0x9108

/* sample1:
 * 3 cmdargs: 1,0xffffffff,00 00 10 00;
 * data:
    00000000: 48 00 00 00 02 00 09 91 12 00 00 00 01 00 00 00
    00000010: 38 00 00 00 00 00 00 30 01 00 00 00 01 30 00 00
    00000020: 01 00 00 00 10 00 00 00 00 00 00 00 00 00 00 20
    00000030: 00 00 00 30 44 43 49 4d 00 00 00 00 00 00 00 00	DCIM
    00000040: 00 00 00 00 cc c3 01 46
 * 2 respargs: 0x0, 0x3c
 *
 * sample2:
 *
    00000000: 18 00 00 00 01 00 09 91 15 00 00 00 01 00 00 00
    00000010: 00 00 00 30 00 00 10 00

    00000000: 48 00 00 00 02 00 09 91 15 00 00 00 01 00 00 00
    00000010: 38 00 00 00 00 00 9c 33 01 00 00 00 01 30 00 00
    00000020: 01 00 00 00 10 00 00 00 00 00 00 00 00 00 00 30
    00000030: 00 00 9c 33 32 33 31 43 41 4e 4f 4e 00 00 00 00	 231CANON
    00000040: 00 00 00 00 cc c3 01 46

 */
#define PTP_OC_CANON_EOS_GetObjectInfoEx	0x9109
#define PTP_OC_CANON_EOS_GetThumbEx		0x910A
#define PTP_OC_CANON_EOS_SendPartialObject	0x910B
#define PTP_OC_CANON_EOS_SetObjectAttributes	0x910C
#define PTP_OC_CANON_EOS_GetObjectTime		0x910D
#define PTP_OC_CANON_EOS_SetObjectTime		0x910E

/* 910f: no args, no data, 1 response arg (0). */
#define PTP_OC_CANON_EOS_RemoteRelease		0x910F
/* Marcus: looks more like "Set DeviceProperty" in the trace.
 *
 * no cmd args
 * data phase (0xc, 0xd11c, 0x1)
 * no resp args
 */
#define PTP_OC_CANON_EOS_SetDevicePropValueEx	0x9110
#define PTP_OC_CANON_EOS_GetRemoteMode		0x9113
/* 9114: 1 arg (0x1), no data, no resp data. */
#define PTP_OC_CANON_EOS_SetRemoteMode		0x9114
/* 9115: 1 arg (0x1), no data, no resp data. */
#define PTP_OC_CANON_EOS_SetEventMode		0x9115
/* 9116: no args, data phase, no resp data. */
#define PTP_OC_CANON_EOS_GetEvent		0x9116
#define PTP_OC_CANON_EOS_TransferComplete	0x9117
#define PTP_OC_CANON_EOS_CancelTransfer		0x9118
#define PTP_OC_CANON_EOS_ResetTransfer		0x9119

/* 911a: 3 args (0xfffffff7, 0x00001000, 0x00000001), no data, no resp data. */
/* 911a: 3 args (0x001dfc60, 0x00001000, 0x00000001), no data, no resp data. */
#define PTP_OC_CANON_EOS_PCHDDCapacity		0x911A

/* 911b: no cmd args, no data, no resp args */
#define PTP_OC_CANON_EOS_SetUILock		0x911B
/* 911c: no cmd args, no data, no resp args */
#define PTP_OC_CANON_EOS_ResetUILock		0x911C
#define PTP_OC_CANON_EOS_KeepDeviceOn		0x911D /* no arg */
#define PTP_OC_CANON_EOS_SetNullPacketMode	0x911E /* 1 param */
#define PTP_OC_CANON_EOS_UpdateFirmware		0x911F
#define PTP_OC_CANON_EOS_TransferCompleteDT	0x9120
#define PTP_OC_CANON_EOS_CancelTransferDT	0x9121
#define PTP_OC_CANON_EOS_SetWftProfile		0x9122
#define PTP_OC_CANON_EOS_GetWftProfile		0x9123 /* 2 args: setnum, configid */
#define PTP_OC_CANON_EOS_SetProfileToWft	0x9124
#define PTP_OC_CANON_EOS_BulbStart		0x9125
#define PTP_OC_CANON_EOS_BulbEnd		0x9126
#define PTP_OC_CANON_EOS_RequestDevicePropValue	0x9127

/* 0x9128 args (0x1/0x2, 0x0), no data, no resp args */
#define PTP_OC_CANON_EOS_RemoteReleaseOn	0x9128
/* 0x9129 args (0x1/0x2), no data, no resp args */
#define PTP_OC_CANON_EOS_RemoteReleaseOff	0x9129

#define PTP_OC_CANON_EOS_RegistBackgroundImage	0x912A
#define PTP_OC_CANON_EOS_ChangePhotoStudioMode	0x912B
#define PTP_OC_CANON_EOS_GetPartialObjectEx	0x912C
#define PTP_OC_CANON_EOS_ResetMirrorLockupState	0x9130 /* no args */
#define PTP_OC_CANON_EOS_PopupBuiltinFlash	0x9131
#define PTP_OC_CANON_EOS_EndGetPartialObjectEx	0x9132
#define PTP_OC_CANON_EOS_MovieSelectSWOn	0x9133 /* no args */
#define PTP_OC_CANON_EOS_MovieSelectSWOff	0x9134 /* no args */
#define PTP_OC_CANON_EOS_GetCTGInfo		0x9135
#define PTP_OC_CANON_EOS_GetLensAdjust		0x9136
#define PTP_OC_CANON_EOS_SetLensAdjust		0x9137
#define PTP_OC_CANON_EOS_ReadyToSendMusic	0x9138
/* 3 paramaeters, no data, OFC, size, unknown */
#define PTP_OC_CANON_EOS_CreateHandle		0x9139
#define PTP_OC_CANON_EOS_SendPartialObjectEx	0x913A
#define PTP_OC_CANON_EOS_EndSendPartialObjectEx	0x913B
#define PTP_OC_CANON_EOS_SetCTGInfo		0x913C
#define PTP_OC_CANON_EOS_SetRequestOLCInfoGroup	0x913D
#define PTP_OC_CANON_EOS_SetRequestRollingPitchingLevel	0x913E /* 1 arg: onoff? */
/* 3 args, 0x21201020, 0x110, 0x1000000 (potentially reverse order) */

/* EOS M6 Mark2:	opargs: 0x01000000, 0x000001020, 0 (supportkind, modelid ?),
			response args: 0x00000811, 0x00000001 */

#define PTP_OC_CANON_EOS_GetCameraSupport	0x913F
#define PTP_OC_CANON_EOS_SetRating		0x9140 /* 2 args, objectid, rating? */
#define PTP_OC_CANON_EOS_RequestInnerDevelopStart	0x9141 /* 2 args: 1 type, 1 object? */
#define PTP_OC_CANON_EOS_RequestInnerDevelopParamChange	0x9142
#define PTP_OC_CANON_EOS_RequestInnerDevelopEnd		0x9143
#define PTP_OC_CANON_EOS_GpsLoggingDataMode		0x9144 /* 1 arg */
#define PTP_OC_CANON_EOS_GetGpsLogCurrentHandle		0x9145
#define PTP_OC_CANON_EOS_SetImageRecoveryData		0x9146 /* sends data? */
#define PTP_OC_CANON_EOS_GetImageRecoveryList		0x9147
#define PTP_OC_CANON_EOS_FormatImageRecoveryData	0x9148
#define PTP_OC_CANON_EOS_GetPresetLensAdjustParam	0x9149 /* no arg */
#define PTP_OC_CANON_EOS_GetRawDispImage		0x914A /* ? 2 args ? */
#define PTP_OC_CANON_EOS_SaveImageRecoveryData		0x914B
#define PTP_OC_CANON_EOS_RequestBLE			0x914C /* 2? args? */
#define PTP_OC_CANON_EOS_DrivePowerZoom			0x914D /* 1 arg */

#define PTP_OC_CANON_EOS_GetIptcData		0x914F
#define PTP_OC_CANON_EOS_SetIptcData		0x9150 /* sends data? */
#define PTP_OC_CANON_EOS_InitiateViewfinder	0x9151	/* no arg */
#define PTP_OC_CANON_EOS_TerminateViewfinder	0x9152
/* EOS M2 wlan: 2 params, 0x00200000 0x01000000 */
#define PTP_OC_CANON_EOS_GetViewFinderData	0x9153
#define PTP_OC_CANON_EOS_DoAf			0x9154
#define PTP_OC_CANON_EOS_DriveLens		0x9155
#define PTP_OC_CANON_EOS_DepthOfFieldPreview	0x9156 /* 1 arg */
#define PTP_OC_CANON_EOS_ClickWB		0x9157 /* 2 args: x,y */
#define PTP_OC_CANON_EOS_Zoom			0x9158 /* 1 arg: zoom */
#define PTP_OC_CANON_EOS_ZoomPosition		0x9159 /* 2 args: x,y */
#define PTP_OC_CANON_EOS_SetLiveAfFrame		0x915A /* sends data? */
#define PTP_OC_CANON_EOS_TouchAfPosition	0x915B /* 3 args: type,x,y */
#define PTP_OC_CANON_EOS_SetLvPcFlavoreditMode	0x915C /* 1 arg */
#define PTP_OC_CANON_EOS_SetLvPcFlavoreditParam	0x915D /* 1 arg */
#define PTP_OC_CANON_EOS_RequestSensorCleaning	0x915E /* 1 arg? */
#define PTP_OC_CANON_EOS_AfCancel		0x9160
#define PTP_OC_CANON_EOS_SetImageRecoveryDataEx	0x916B
#define PTP_OC_CANON_EOS_GetImageRecoveryListEx	0x916C
#define PTP_OC_CANON_EOS_CompleteAutoSendImages	0x916D
#define PTP_OC_CANON_EOS_NotifyAutoTransferStatus	0x916E
#define PTP_OC_CANON_EOS_GetReducedObject	0x916F
#define PTP_OC_CANON_EOS_GetObjectInfo64	0x9170	/* 1 arg: oid */
#define PTP_OC_CANON_EOS_GetObject64		0x9171	/* 1 arg: oid */
#define PTP_OC_CANON_EOS_GetPartialObject64	0x9172	/* args: oid, offset, maxbyte */
#define PTP_OC_CANON_EOS_GetObjectInfoEx64	0x9173	/* 2 args: storageid, oid  ? */
#define PTP_OC_CANON_EOS_GetPartialObjectEX64	0x9174	/* args: oid, offset 64bit, maxbyte */
#define PTP_OC_CANON_EOS_CreateHandle64		0x9175
#define PTP_OC_CANON_EOS_NotifySaveComplete	0x9177
#define PTP_OC_CANON_EOS_GetTranscodedBlock	0x9178
#define PTP_OC_CANON_EOS_TransferCompleteTranscodedBlock	0x9179
#define PTP_OC_CANON_EOS_NotifyEstimateNumberofImport		0x9182 /* 1 arg: importnumber */
#define PTP_OC_CANON_EOS_NotifyNumberofImported	0x9183 /* 1 arg: importnumber */
#define PTP_OC_CANON_EOS_NotifySizeOfPartialDataTransfer	0x9184 /* 4 args: filesizelow, filesizehigh, downloadsizelow, downloadsizehigh */
#define PTP_OC_CANON_EOS_NotifyFinish		0x9185 /* 1 arg: reason */
#define PTP_OC_CANON_EOS_GetWFTData		0x9186
#define PTP_OC_CANON_EOS_SetWFTData		0x9187
#define PTP_OC_CANON_EOS_ChangeWFTSettingNumber	0x9188
#define PTP_OC_CANON_EOS_GetPictureStylePCFlavorParam	0x9189
#define PTP_OC_CANON_EOS_SetPictureStylePCFlavorParam	0x918A
#define PTP_OC_CANON_EOS_GetObjectURL		0x91AB
#define PTP_OC_CANON_EOS_SetCAssistMode		0x91AC
#define PTP_OC_CANON_EOS_GetCAssistPresetThumb	0x91AD
#define PTP_OC_CANON_EOS_SetFELock		0x91B9
#define PTP_OC_CANON_EOS_DeleteWFTSettingNumber	0x91BA
#define PTP_OC_CANON_EOS_SetDefaultCameraSetting		0x91BE
#define PTP_OC_CANON_EOS_GetAEData		0x91BF
#define PTP_OC_CANON_EOS_SendHostInfo		0x91E4 /* https://research.checkpoint.com/say-cheese-ransomware-ing-a-dslr-camera/ */
#define PTP_OC_CANON_EOS_NotifyNetworkError	0x91E8 /* 1 arg: errorcode */
#define PTP_OC_CANON_EOS_AdapterTransferProgress		0x91E9
#define PTP_OC_CANON_EOS_TransferCompleteFTP	0x91F0
#define PTP_OC_CANON_EOS_CancelTransferFTP	0x91F1
#define PTP_OC_CANON_EOS_NotifyBtStatus		0x91F9 /* https://research.checkpoint.com/say-cheese-ransomware-ing-a-dslr-camera/ */
#define PTP_OC_CANON_EOS_SetAdapterBatteryReport		0x91FD /* https://research.checkpoint.com/say-cheese-ransomware-ing-a-dslr-camera/ */
#define PTP_OC_CANON_EOS_FAPIMessageTX		0x91FE
#define PTP_OC_CANON_EOS_FAPIMessageRX		0x91FF

/* A1E8 ... also seen? is an error code? */

/* Nikon extension Operation Codes */
#define PTP_OC_NIKON_GetProfileAllData	0x9006
#define PTP_OC_NIKON_SendProfileData	0x9007
#define PTP_OC_NIKON_DeleteProfile	0x9008
#define PTP_OC_NIKON_SetProfileData	0x9009
#define PTP_OC_NIKON_AdvancedTransfer	0x9010
#define PTP_OC_NIKON_GetFileInfoInBlock	0x9011
#define PTP_OC_NIKON_InitiateCaptureRecInSdram		0x90C0	/* 1 param,   no data */
#define PTP_OC_NIKON_AfDrive		0x90C1	/* no params, no data */
#define PTP_OC_NIKON_ChangeCameraMode	0x90C2	/* 1 param,  no data */
#define PTP_OC_NIKON_DelImageSDRAM	0x90C3	/* 1 param (0x0: all, others: cancel this image) ,  no data */
#define PTP_OC_NIKON_GetLargeThumb	0x90C4
#define PTP_OC_NIKON_CurveDownload	0x90C5	/* 1 param,   data in */
#define PTP_OC_NIKON_CurveUpload	0x90C6	/* 1 param,   data out */
#define PTP_OC_NIKON_GetEvent		0x90C7	/* no params, data in */
#define PTP_OC_NIKON_DeviceReady	0x90C8	/* no params, no data */
#define PTP_OC_NIKON_SetPreWBData	0x90C9	/* 3 params,  data out */
#define PTP_OC_NIKON_GetVendorPropCodes	0x90CA	/* 0 params, data in */
#define PTP_OC_NIKON_AfCaptureSDRAM	0x90CB	/* no params, no data */
#define PTP_OC_NIKON_GetPictCtrlData	0x90CC	/* 2 params, data in */
#define PTP_OC_NIKON_SetPictCtrlData	0x90CD	/* 2 params, data out */
#define PTP_OC_NIKON_DelCstPicCtrl	0x90CE	/* 1 param, no data */
#define PTP_OC_NIKON_GetPicCtrlCapability	0x90CF	/* 1 param, data in */

/* Nikon Liveview stuff */
#define PTP_OC_NIKON_GetPreviewImg	0x9200
#define PTP_OC_NIKON_StartLiveView	0x9201	/* no params */
#define PTP_OC_NIKON_EndLiveView	0x9202	/* no params */
#define PTP_OC_NIKON_GetLiveViewImg	0x9203	/* no params, data in */
#define PTP_OC_NIKON_MfDrive		0x9204	/* 2 params */
#define PTP_OC_NIKON_ChangeAfArea	0x9205	/* 2 params */
#define PTP_OC_NIKON_AfDriveCancel	0x9206	/* no params */
/* 2 params:
 * 0xffffffff == No AF before,  0xfffffffe == AF before capture
 * sdram=1, card=0
 */
#define PTP_OC_NIKON_InitiateCaptureRecInMedia	0x9207	/* 1 params */

#define PTP_OC_NIKON_GetVendorStorageIDs	0x9209	/* no params, data in */

#define PTP_OC_NIKON_StartMovieRecInCard	0x920a	/* no params, no data */
#define PTP_OC_NIKON_EndMovieRec		0x920b	/* no params, no data */

#define PTP_OC_NIKON_TerminateCapture		0x920c	/* 2 params */
#define PTP_OC_NIKON_GetFhdPicture		0x920f	/* param: objecthandle. returns (at most) 1920x1028 picture */

#define PTP_OC_NIKON_GetDevicePTPIPInfo	0x90E0

#define PTP_OC_NIKON_GetPartialObjectHiSpeed	0x9400	/* 3 params, p1: object handle, p2: 32bit transfer size, p3: terminate after transfer. DATA in, Reuslt: r1: 32bit number sent, r2: before offset low 32bit , r3: before offset high 32bit */
#define PTP_OC_NIKON_StartSpotWb		0x9402
#define PTP_OC_NIKON_EndSpotWb			0x9403
#define PTP_OC_NIKON_ChangeSpotWbArea		0x9404
#define PTP_OC_NIKON_MeasureSpotWb		0x9405
#define PTP_OC_NIKON_EndSpotWbResultDisp	0x9406
#define PTP_OC_NIKON_CancelImagesInSDRAM	0x940c
#define PTP_OC_NIKON_GetSBHandles		0x9414
#define PTP_OC_NIKON_GetSBAttrDesc		0x9415
#define PTP_OC_NIKON_GetSBAttrValue		0x9416
#define PTP_OC_NIKON_SetSBAttrValue		0x9417
#define PTP_OC_NIKON_GetSBGroupAttrDesc		0x9418
#define PTP_OC_NIKON_GetSBGroupAttrValue	0x9419
#define PTP_OC_NIKON_SetSBGroupAttrValue	0x941a
#define PTP_OC_NIKON_TestFlash			0x941b
#define PTP_OC_NIKON_GetEventEx			0x941c	/* can do multiparameter events, compared to GetEvent */
#define PTP_OC_NIKON_MirrorUpCancel		0x941d
#define PTP_OC_NIKON_PowerZoomByFocalLength	0x941e
#define PTP_OC_NIKON_ActiveSelectionControl	0x941f
#define PTP_OC_NIKON_SaveCameraSetting		0x9420
#define PTP_OC_NIKON_GetObjectSize		0x9421	/* param: objecthandle, returns 64bit objectsize as DATA */
#define PTP_OC_NIKON_ChangeMonitorOff		0x9422
#define PTP_OC_NIKON_GetLiveViewCompressedSize	0x9423
#define PTP_OC_NIKON_StartTracking		0x9424
#define PTP_OC_NIKON_EndTracking		0x9425
#define PTP_OC_NIKON_ChangeAELock		0x9426
#define PTP_OC_NIKON_GetLiveViewImageEx		0x9428
#define PTP_OC_NIKON_GetPartialObjectEx		0x9431	/* p1: objecthandle, p2: offset lower 32bit, p3: offset higher 32bit, p4: maxsize lower 32bit, p5: maxsize upper 32bit, response is r1: lower 32bit, r2: higher 32bit */
#define PTP_OC_NIKON_GetManualSettingLensData	0x9432
#define PTP_OC_NIKON_InitiatePixelMapping	0x9433
#define PTP_OC_NIKON_GetObjectsMetaData		0x9434
#define PTP_OC_NIKON_ChangeApplicationMode	0x9435
#define PTP_OC_NIKON_ResetMenu			0x9436


/* From Nikon V1 Trace */
#define PTP_OC_NIKON_GetDevicePropEx		0x9504	/* gets device prop data */


/* Casio EX-F1 (from http://code.google.com/p/exf1ctrl/ ) */
#define PTP_OC_CASIO_STILL_START	0x9001
#define PTP_OC_CASIO_STILL_STOP		0x9002

#define PTP_OC_CASIO_FOCUS		0x9007
#define PTP_OC_CASIO_CF_PRESS		0x9009
#define PTP_OC_CASIO_CF_RELEASE		0x900A
#define PTP_OC_CASIO_GET_OBJECT_INFO	0x900C

#define PTP_OC_CASIO_SHUTTER		0x9024
#define PTP_OC_CASIO_GET_STILL_HANDLES	0x9027
#define PTP_OC_CASIO_STILL_RESET	0x9028
#define PTP_OC_CASIO_HALF_PRESS		0x9029
#define PTP_OC_CASIO_HALF_RELEASE	0x902A
#define PTP_OC_CASIO_CS_PRESS		0x902B
#define PTP_OC_CASIO_CS_RELEASE		0x902C

#define PTP_OC_CASIO_ZOOM		0x902D
#define PTP_OC_CASIO_CZ_PRESS		0x902E
#define PTP_OC_CASIO_CZ_RELEASE		0x902F

#define PTP_OC_CASIO_MOVIE_START	0x9041
#define PTP_OC_CASIO_MOVIE_STOP		0x9042
#define PTP_OC_CASIO_MOVIE_PRESS	0x9043
#define PTP_OC_CASIO_MOVIE_RELEASE	0x9044
#define PTP_OC_CASIO_GET_MOVIE_HANDLES	0x9045
#define PTP_OC_CASIO_MOVIE_RESET	0x9046

#define PTP_OC_CASIO_GET_OBJECT		0x9025
#define PTP_OC_CASIO_GET_THUMBNAIL	0x9026

/* Sony stuff */
/* 9201:
 *  3 params: 1,0,0 ; IN: data 8 bytes all 0
 * or:
 *  3 params: 2,0,0 ; IN: data 8 bytes all 0
 * or
 *  3 params: 3,0,0,: IN: data 8 bytes all 0
 */
#define PTP_OC_SONY_SDIOConnect			0x9201
/* 9202: 1 param, 0xc8; IN data:
 * 16 bit: 0xc8
 * ptp array 32 bit: index, 16 bit values of propcodes  */
#define PTP_OC_SONY_GetSDIOGetExtDeviceInfo	0x9202

#define PTP_OC_SONY_GetDevicePropdesc		0x9203
#define PTP_OC_SONY_GetDevicePropertyValue	0x9204
/* 1 param, 16bit propcode, SEND DATA: propvalue */
#define PTP_OC_SONY_SetControlDeviceA		0x9205
#define PTP_OC_SONY_GetControlDeviceDesc	0x9206
/* 1 param, 16bit propcode, SEND DATA: propvalue */
#define PTP_OC_SONY_SetControlDeviceB		0x9207
/* get all device property data at once */
#define PTP_OC_SONY_GetAllDevicePropData	0x9209	/* gets a 4126 byte blob of device props ?*/

#define PTP_OC_SONY_QX_SetExtPictureProfile	0x96F2
#define PTP_OC_SONY_QX_GetExtPictureProfile	0x96F3
#define PTP_OC_SONY_QX_GetExtLensInfo		0x96F4
#define PTP_OC_SONY_QX_SendUpdateFile		0x96F5
#define PTP_OC_SONY_QX_GetAllDevicePropData	0x96F6
#define PTP_OC_SONY_QX_SetControlDeviceB	0x96F8 /* ControlDevice */
#define PTP_OC_SONY_QX_SetControlDeviceA	0x96FA /* SetExtDevicePropValue */
#define PTP_OC_SONY_QX_GetSDIOGetExtDeviceInfo	0x96FD
#define PTP_OC_SONY_QX_Connect			0x96FE

/* Microsoft / MTP extension codes */

#define PTP_OC_MTP_GetObjectPropsSupported	0x9801
#define PTP_OC_MTP_GetObjectPropDesc		0x9802
#define PTP_OC_MTP_GetObjectPropValue		0x9803
#define PTP_OC_MTP_SetObjectPropValue		0x9804
#define PTP_OC_MTP_GetObjPropList		0x9805
#define PTP_OC_MTP_SetObjPropList		0x9806
#define PTP_OC_MTP_GetInterdependendPropdesc	0x9807
#define PTP_OC_MTP_SendObjectPropList		0x9808
#define PTP_OC_MTP_GetObjectReferences		0x9810
#define PTP_OC_MTP_SetObjectReferences		0x9811
#define PTP_OC_MTP_UpdateDeviceFirmware		0x9812
#define PTP_OC_MTP_Skip				0x9820

/*
 * Windows Media Digital Rights Management for Portable Devices
 * Extension Codes (microsoft.com/WMDRMPD: 10.1)
 */
#define PTP_OC_MTP_WMDRMPD_GetSecureTimeChallenge	0x9101
#define PTP_OC_MTP_WMDRMPD_GetSecureTimeResponse	0x9102
#define PTP_OC_MTP_WMDRMPD_SetLicenseResponse	0x9103
#define PTP_OC_MTP_WMDRMPD_GetSyncList		0x9104
#define PTP_OC_MTP_WMDRMPD_SendMeterChallengeQuery	0x9105
#define PTP_OC_MTP_WMDRMPD_GetMeterChallenge	0x9106
#define PTP_OC_MTP_WMDRMPD_SetMeterResponse		0x9107
#define PTP_OC_MTP_WMDRMPD_CleanDataStore		0x9108
#define PTP_OC_MTP_WMDRMPD_GetLicenseState		0x9109
#define PTP_OC_MTP_WMDRMPD_SendWMDRMPDCommand	0x910A
#define PTP_OC_MTP_WMDRMPD_SendWMDRMPDRequest	0x910B

/*
 * Windows Media Digital Rights Management for Portable Devices
 * Extension Codes (microsoft.com/WMDRMPD: 10.1)
 * Below are operations that have no public documented identifier
 * associated with them "Vendor-defined Command Code"
 */
#define PTP_OC_MTP_WMDRMPD_SendWMDRMPDAppRequest	0x9212
#define PTP_OC_MTP_WMDRMPD_GetWMDRMPDAppResponse	0x9213
#define PTP_OC_MTP_WMDRMPD_EnableTrustedFilesOperations	0x9214
#define PTP_OC_MTP_WMDRMPD_DisableTrustedFilesOperations 0x9215
#define PTP_OC_MTP_WMDRMPD_EndTrustedAppSession		0x9216
/* ^^^ guess ^^^ */

/*
 * Microsoft Advanced Audio/Video Transfer
 * Extensions (microsoft.com/AAVT: 1.0)
 */
#define PTP_OC_MTP_AAVT_OpenMediaSession		0x9170
#define PTP_OC_MTP_AAVT_CloseMediaSession		0x9171
#define PTP_OC_MTP_AAVT_GetNextDataBlock		0x9172
#define PTP_OC_MTP_AAVT_SetCurrentTimePosition		0x9173

/*
 * Windows Media Digital Rights Management for Network Devices
 * Extensions (microsoft.com/WMDRMND: 1.0) MTP/IP?
 */
#define PTP_OC_MTP_WMDRMND_SendRegistrationRequest	0x9180
#define PTP_OC_MTP_WMDRMND_GetRegistrationResponse	0x9181
#define PTP_OC_MTP_WMDRMND_GetProximityChallenge	0x9182
#define PTP_OC_MTP_WMDRMND_SendProximityResponse	0x9183
#define PTP_OC_MTP_WMDRMND_SendWMDRMNDLicenseRequest	0x9184
#define PTP_OC_MTP_WMDRMND_GetWMDRMNDLicenseResponse	0x9185

/*
 * Windows Media Player Portiable Devices
 * Extension Codes (microsoft.com/WMPPD: 11.1)
 */
#define PTP_OC_MTP_WMPPD_ReportAddedDeletedItems	0x9201
#define PTP_OC_MTP_WMPPD_ReportAcquiredItems 	        0x9202
#define PTP_OC_MTP_WMPPD_PlaylistObjectPref		0x9203

/*
 * Undocumented Zune Operation Codes
 * maybe related to WMPPD extension set?
 */
#define PTP_OC_MTP_ZUNE_GETUNDEFINED001		        0x9204

/* WiFi Provisioning MTP Extension Codes (microsoft.com/WPDWCN: 1.0) */
#define PTP_OC_MTP_WPDWCN_ProcessWFCObject		0x9122

/* Olympus OMD series commands */
#define PTP_OC_OLYMPUS_OMD_Capture			0x9481
#define PTP_OC_OLYMPUS_GetLiveViewImage			0x9484	/* liveview */
#define PTP_OC_OLYMPUS_OMD_GetImage			0x9485	/* gets an JPEG image (from the capture? SDRAM style?) */
#define PTP_OC_OLYMPUS_OMD_ChangedProperties		0x9486
#define PTP_OC_OLYMPUS_OMD_MFDrive			0x9487
#define PTP_OC_OLYMPUS_OMD_SetProperties		0x9489 /* Sends to the device a PTP list of all 16 bit device properties , count 32bit, then 16bit vals */
/* 948C: Record Video? */
/* 9482: Set One Touch WB Gain */
/* 9483: Set / Start Magnifying Live View Point */
/* 9488: Change Magnifying Live View Area */
/* 9493: Start Driving Zoom Lens For Direction / Focal Length  / Stop Driving zoom
 * start direction: 		x1=1,x2=0,x3= STEPS?, x4=1 or 2 (near / far ? )
 * start to focallength:	x1=1,x2=3,x3= VALUE? ,x4=4 (potentially more)
 * stop:  			x1=2,x2=0,x3=0,x4=0
 * unclear:			x1=4,x2=0,x3=0,x4=0
 */
/* 9495: Set / Clear Auto Focus Point? */
/* 94a0: Set / Clear Auto Exposure Point? */
/* 94b7 or 94bf: Set Focus Adjust Pulse */
/* 94A1: Detect One Touch WB Gain */
/* 94A2: AdjustLevelGauge? */
/* 94A4: Get Direct Item Buffer */
/* 94A5: Get Direct Item Info */
/* 94B7: Get Recording Folder List? */
/* 94BA / 94a1: Pixel Mapping? */
/* 94ba: TransferModeStartStop */
/* 94bb: Get Un Transfer List */
/* 94bc: GetLocalObject info? */
/* 94bd: GetLocalObject? */
/* 94be: delete local object? */
/* 94c0 / 94b9 : Set Comment String */
/* 94bf: Set Connect Pc Info? */
/* 94c0: Get Connect Pc Info? */
/* 94c1: Clear Connect Pc Info? */
/* 94c4: Get Camera Af Target Frames? */
/* 94c3: Start Station Mode */
/* 94c3: End Station Mode */
/* 911c: Get Firmware Update Mode? */
/* 9121: Firmware Check? */
/* 9122: Get Firmware Status? */
/* 9123: Firmware Update Initiate? */
/* 9124: Get Firmware Language? */
/* 9125: Get Firmware Status? */
/* 9126: Trans Firmware? */

/* Olympus E series commands */

#define PTP_OC_OLYMPUS_Capture				0x9101
#define PTP_OC_OLYMPUS_SelfCleaning			0x9103
#define PTP_OC_OLYMPUS_SetRGBGain			0x9106
#define PTP_OC_OLYMPUS_SetPresetMode			0x9107
#define PTP_OC_OLYMPUS_SetWBBiasAll			0x9108
#define PTP_OC_OLYMPUS_GetCameraControlMode		0x910a
#define PTP_OC_OLYMPUS_SetCameraControlMode		0x910b
#define PTP_OC_OLYMPUS_SetWBRGBGain			0x910c
#define PTP_OC_OLYMPUS_GetDeviceInfo			0x9301
#define PTP_OC_OLYMPUS_OpenSession			0x9302
#define PTP_OC_OLYMPUS_SetDateTime			0x9402
#define PTP_OC_OLYMPUS_GetDateTime			0x9482
#define PTP_OC_OLYMPUS_SetCameraID			0x9501
#define PTP_OC_OLYMPUS_GetCameraID			0x9581


/* Android Random I/O Extensions Codes */
#define PTP_OC_ANDROID_GetPartialObject64		0x95C1
#define PTP_OC_ANDROID_SendPartialObject		0x95C2
#define PTP_OC_ANDROID_TruncateObject			0x95C3
#define PTP_OC_ANDROID_BeginEditObject			0x95C4
#define PTP_OC_ANDROID_EndEditObject			0x95C5

/* Leica opcodes, from Lightroom tether plugin */
/* also from:
 * https://alexhude.github.io/2019/01/24/hacking-leica-m240.html */
#define PTP_OC_LEICA_SetCameraSettings			0x9001 	/* image shuttle */
#define PTP_OC_LEICA_GetCameraSettings			0x9002
#define PTP_OC_LEICA_GetLensParameter			0x9003	/* lrplugin */
/* probably 2 arguments.
 * generic: releaseStage, stepSize
 * Release(releasestage) = (releasestage,0)
 * Release() = (0,0)
 * AEStart() = (1,0)
 * Autofocusrelease() = (2,0)
 * AutofocusPush() = (1,0) ... same as AEStart?
 * KeepCameraActive() = (0xe,0)
 */
#define PTP_OC_LEICA_LEReleaseStages			0x9004	/* lrplugin seeing 1 (push af control), 2 (release af control), 0x0c (continuous start), 0x0d (continuous end) as potential arguments */
#define PTP_OC_LEICA_LEOpenSession			0x9005	/* lrplugin one argument, possible 0 is ok? */
#define PTP_OC_LEICA_LECloseSession			0x9006	/* lrplugin */
#define PTP_OC_LEICA_RequestObjectTransferReady		0x9007
#define PTP_OC_LEICA_GetGeoTrackingData			0x9008
#define PTP_OC_LEICA_OpenDebugSession			0x900a
#define PTP_OC_LEICA_CloseDebugSession			0x900b
#define PTP_OC_LEICA_GetDebugBuffer			0x900c
#define PTP_OC_LEICA_DebugCommandString			0x900d
#define PTP_OC_LEICA_GetDebugRoute			0x900e
#define PTP_OC_LEICA_SetIPTCData			0x900f
#define PTP_OC_LEICA_GetIPTCData			0x9010
#define PTP_OC_LEICA_LEControlAutoFocus			0x9016	/* lr plugin */
#define PTP_OC_LEICA_LEControlBulbExposure		0x9019	/* seen in lr plugin ... similar to 9004 and 901a release/press shutter */
#define PTP_OC_LEICA_LEControlContinuousExposure	0x901a	/* seen in lr plugin ... similar to 9004 and 9019 release/press shutter */
#define PTP_OC_LEICA_901b				0x901b	/* seen in lr plugin ... related to release not listed in debugprint */
#define PTP_OC_LEICA_LEControlPhotoLiveView		0x901c	/* seen in lr plugin ... */
#define PTP_OC_LEICA_LEKeepSessionActive		0x901d	/* seen in lr plugin ... */
#define PTP_OC_LEICA_LEMoveLens				0x901e	/* seen in image shuttle ... 1? arg */
#define PTP_OC_LEICA_Get3DAxisData			0x9020
#define PTP_OC_LEICA_LESetZoomMode			0x9021	/* lr plugin */
#define PTP_OC_LEICA_LESetFocusCrossPosition		0x9022	/* lr plugin */
#define PTP_OC_LEICA_LESetDisplayWindowPosition		0x9024	/* lr plugin */
#define PTP_OC_LEICA_LEGetStreamData			0x9025	/* lr plugin */
#define PTP_OC_LEICA_OpenLiveViewSession		0x9030
#define PTP_OC_LEICA_CloseLiveViewSession		0x9031
#define PTP_OC_LEICA_LESetDateTime			0x9036	/* lr plugin */
#define PTP_OC_LEICA_GetObjectPropListPaginated		0x9037
#define PTP_OC_LEICA_OpenProductionSession		0x9100
#define PTP_OC_LEICA_CloseProductionSession		0x9101
#define PTP_OC_LEICA_UpdateFirmware			0x9102
#define PTP_OC_LEICA_OpenOSDSession			0x9103
#define PTP_OC_LEICA_CloseOSDSession			0x9104
#define PTP_OC_LEICA_GetOSDData				0x9105
#define PTP_OC_LEICA_GetFirmwareStruct			0x9106
#define PTP_OC_LEICA_GetDebugMenu			0x910b
#define PTP_OC_LEICA_SetDebugMenu			0x910c
#define PTP_OC_LEICA_OdinMessage			0x910d
#define PTP_OC_LEICA_GetDebugObjectHandles		0x910e
#define PTP_OC_LEICA_GetDebugObject			0x910f
#define PTP_OC_LEICA_DeleteDebugObject			0x9110
#define PTP_OC_LEICA_GetDebugObjectInfo			0x9111
#define PTP_OC_LEICA_WriteDebugObject			0x9112
#define PTP_OC_LEICA_CreateDebugObject			0x9113
#define PTP_OC_LEICA_Calibrate3DAxis			0x9114
#define PTP_OC_LEICA_MagneticCalibration		0x9115
#define PTP_OC_LEICA_GetViewFinderData			0x9116

#define PTP_OC_PARROT_GetSunshineValues		0x9201
#define PTP_OC_PARROT_GetTemperatureValues	0x9202
#define PTP_OC_PARROT_GetAngleValues		0x9203
#define PTP_OC_PARROT_GetGpsValues		0x9204
#define PTP_OC_PARROT_GetGyroscopeValues	0x9205
#define PTP_OC_PARROT_GetAccelerometerValues	0x9206
#define PTP_OC_PARROT_GetMagnetometerValues	0x9207
#define PTP_OC_PARROT_GetImuValues		0x9208
#define PTP_OC_PARROT_GetStatusMask		0x9209
#define PTP_OC_PARROT_EjectStorage		0x920A
#define PTP_OC_PARROT_StartMagnetoCalib		0x9210
#define PTP_OC_PARROT_StopMagnetoCalib		0x9211
#define PTP_OC_PARROT_MagnetoCalibStatus	0x9212
#define PTP_OC_PARROT_SendFirmwareUpdate	0x9213

#define PTP_OC_PANASONIC_9101			0x9101
#define PTP_OC_PANASONIC_OpenSession		0x9102	/* opensession (1 arg, seems to be storage id 0x00010001)*/
#define PTP_OC_PANASONIC_CloseSession		0x9103	/* closesession (no arg) */
#define PTP_OC_PANASONIC_9104			0x9104	/* get ext device id (1 arg?) */
/* 9104 gets this data:
0000  24 00 00 00 02 00 04 91-04 00 00 00 01 00 01 00  $...............
0010  01 00 e1 07 10 00 00 00-00 00 00 00 00 00 00 00  ................
0020  00 00 00 00            -                         ....
*/

#define PTP_OC_PANASONIC_9107			0x9107	/* getsize? */
#define PTP_OC_PANASONIC_ListProperty		0x9108
#define PTP_OC_PANASONIC_9110			0x9110 	/* Get_Object infos */
#define PTP_OC_PANASONIC_9112			0x9112 	/* Get Partial Object , 4 args */
#define PTP_OC_PANASONIC_9113			0x9113 	/* Skip Objects Transfer , 1 arg */

#define PTP_OC_PANASONIC_9401			0x9401
#define PTP_OC_PANASONIC_GetProperty		0x9402
#define PTP_OC_PANASONIC_SetProperty		0x9403
#define PTP_OC_PANASONIC_InitiateCapture	0x9404	/* Rec Ctrl Release */
#define PTP_OC_PANASONIC_9405			0x9405	/* Rec Ctrl AF AE */
#define PTP_OC_PANASONIC_9406			0x9406	/* Setup Ctrl various functions: Format, Sensor Cleaning, Menu Save, firmware update? */
#define PTP_OC_PANASONIC_9408			0x9408
#define PTP_OC_PANASONIC_9409			0x9409	/* 1 arg */
#define PTP_OC_PANASONIC_940A			0x940A	/* 1 arg, e.g. 0x08000010 */
#define PTP_OC_PANASONIC_SetCaptureTarget	0x940B	/* 1 arg, e.g. 0x08000010 */
#define PTP_OC_PANASONIC_MoveRecControl		0x940C	/* 07000011 start, 07000012 stop, 0700013 still capture */
#define PTP_OC_PANASONIC_PowerControl		0x940D	/* 1 arg: 0x0A000011 power off, 0x0a00012 device reset, 0x0a00013 device restart */
#define PTP_OC_PANASONIC_PlayControl		0x940E	/* 2 arg? 0x05000011 current=0, next=1, prev=0xffffffff */
#define PTP_OC_PANASONIC_PlayControlPlay	0x940F	/* 0x05000020 */
#define PTP_OC_PANASONIC_9410			0x9410	/* Rec Ctrl Other */
#define PTP_OC_PANASONIC_SetGPSDataInfo		0x9411
#define PTP_OC_PANASONIC_Liveview		0x9412	/* 0d000010 start, 0d000011 stop */
#define PTP_OC_PANASONIC_PollEvents		0x9414	/* ? 1 arg e.g 12000020 */
#define PTP_OC_PANASONIC_ManualFocusDrive	0x9416	/* Rec Ctrl Mf Assist, Rec Ctrl Backup Req ... 1 arg */

#define PTP_OC_PANASONIC_ChangeEvent		0x9603	/* 2 args ... e.g. 0x4002, new (change object added event) */
#define PTP_OC_PANASONIC_GetFromEventInfo	0x9605	/* 1 arg, e.g. 0x41000013 , 15c00021: setup exec menu save comp, 15c00022: setup exec pixel refresh comp */
#define PTP_OC_PANASONIC_SendDataInfo		0x9606	/* no args? used during firmware update */
#define PTP_OC_PANASONIC_StartSendData		0x9607	/* no args? used during firmware update */

#define PTP_OC_PANASONIC_9703			0x9703	/* Mnt_GetInfo_GetVersion  ... 1 arg? */
#define PTP_OC_PANASONIC_9704			0x9704	/* Set USB Mode ... 80040001 */
#define PTP_OC_PANASONIC_9705			0x9705	/* Ctrl Liveview */
#define PTP_OC_PANASONIC_LiveviewImage		0x9706	/* Get Liveview Data */
#define PTP_OC_PANASONIC_9707			0x9707	/* 4k6k cutting get stream */

/* Samsung NX:
 * 9002 send check event
 * 9003 send get event
 * 9004 require capture exec
 * 9005 capture exec
 * 9006 liveview info
 * 9007 liveview exec / check get file
 * 9008 set focus position
 * 9009 get focus position
 * 900a reset device
 * 900b format device
 * 900d get tick
 * 900e set tick
 * 900f set hidden command
 * 9010 get record status
 * 9011 file transfer
 * 9012 set enlarge
 * 9013 movie complete exec / movie transfer
 * 9014 set record pause
 * 9015 set record resume
 * 9017 set live view
 * 9018 image transfer
 * 9019 set imge receive complete
 * 901a set dev prop value
 * 901b get dev unique id / get dev mode index / get dev prop desc
 * 901d get dev unique id
 * 9020 set live receive complete
 * 9021 sensor cleaning device
 * 9022 interval capture stop
 * 9023 display save mode wakeup
 * 9024 movie cancel
 * 9025 get capture count
 * 9026 control touch af
 * 9027 get image path
 * 9028 tracking af stop
 * 902a connect try ptp/ip
 * 902b connect confirm ptp/ip
 * 90fe firmware update
 */

/* These opcodes are probably FUJI Wifi only (but not USB) */
#define PTP_OC_FUJI_InitiateMovieCapture		0x9020
#define PTP_OC_FUJI_TerminateMovieCapture		0x9021
#define PTP_OC_FUJI_GetCapturePreview			0x9022
#define PTP_OC_FUJI_SetFocusPoint			0x9026	/* LockS1Lock */
#define PTP_OC_FUJI_ResetFocusPoint			0x9027	/* UnlockS1Lock */
#define PTP_OC_FUJI_GetDeviceInfo			0x902B
#define PTP_OC_FUJI_SetShutterSpeed			0x902C	/* StepShutterSpeed */
#define PTP_OC_FUJI_SetAperture				0x902D	/* StepFNumber */
#define PTP_OC_FUJI_SetExposureCompensation		0x902E	/* StepExposureBias */
#define PTP_OC_FUJI_CancelInitiateCapture		0x9030
#define PTP_OC_FUJI_FmSendObjectInfo			0x9040
#define PTP_OC_FUJI_FmSendObject			0x9041
#define PTP_OC_FUJI_FmSendPartialObject			0x9042

/* Proprietary vendor extension operations mask */
#define PTP_OC_EXTENSION_MASK           0xF000
#define PTP_OC_EXTENSION                0x9000


/* Response Codes */

/* PTP v1.0 response codes */
#define PTP_RC_Undefined                0x2000
#define PTP_RC_OK                       0x2001
#define PTP_RC_GeneralError             0x2002
#define PTP_RC_SessionNotOpen           0x2003
#define PTP_RC_InvalidTransactionID     0x2004
#define PTP_RC_OperationNotSupported    0x2005
#define PTP_RC_ParameterNotSupported    0x2006
#define PTP_RC_IncompleteTransfer       0x2007
#define PTP_RC_InvalidStorageId         0x2008
#define PTP_RC_InvalidObjectHandle      0x2009
#define PTP_RC_DevicePropNotSupported   0x200A
#define PTP_RC_InvalidObjectFormatCode  0x200B
#define PTP_RC_StoreFull                0x200C
#define PTP_RC_ObjectWriteProtected     0x200D
#define PTP_RC_StoreReadOnly            0x200E
#define PTP_RC_AccessDenied             0x200F
#define PTP_RC_NoThumbnailPresent       0x2010
#define PTP_RC_SelfTestFailed           0x2011
#define PTP_RC_PartialDeletion          0x2012
#define PTP_RC_StoreNotAvailable        0x2013
#define PTP_RC_SpecificationByFormatUnsupported         0x2014
#define PTP_RC_NoValidObjectInfo        0x2015
#define PTP_RC_InvalidCodeFormat        0x2016
#define PTP_RC_UnknownVendorCode        0x2017
#define PTP_RC_CaptureAlreadyTerminated 0x2018
#define PTP_RC_DeviceBusy               0x2019
#define PTP_RC_InvalidParentObject      0x201A
#define PTP_RC_InvalidDevicePropFormat  0x201B
#define PTP_RC_InvalidDevicePropValue   0x201C
#define PTP_RC_InvalidParameter         0x201D
#define PTP_RC_SessionAlreadyOpened     0x201E
#define PTP_RC_TransactionCanceled      0x201F
#define PTP_RC_SpecificationOfDestinationUnsupported            0x2020
/* PTP v1.1 response codes */
#define PTP_RC_InvalidEnumHandle	0x2021
#define PTP_RC_NoStreamEnabled		0x2022
#define PTP_RC_InvalidDataSet		0x2023

/* Eastman Kodak extension Response Codes */
#define PTP_RC_EK_FilenameRequired	0xA001
#define PTP_RC_EK_FilenameConflicts	0xA002
#define PTP_RC_EK_FilenameInvalid	0xA003

/* Nikon specific response codes */
#define PTP_RC_NIKON_HardwareError		0xA001
#define PTP_RC_NIKON_OutOfFocus			0xA002
#define PTP_RC_NIKON_ChangeCameraModeFailed	0xA003
#define PTP_RC_NIKON_InvalidStatus		0xA004
#define PTP_RC_NIKON_SetPropertyNotSupported	0xA005
#define PTP_RC_NIKON_WbResetError		0xA006
#define PTP_RC_NIKON_DustReferenceError		0xA007
#define PTP_RC_NIKON_ShutterSpeedBulb		0xA008
#define PTP_RC_NIKON_MirrorUpSequence		0xA009
#define PTP_RC_NIKON_CameraModeNotAdjustFNumber	0xA00A
#define PTP_RC_NIKON_NotLiveView		0xA00B
#define PTP_RC_NIKON_MfDriveStepEnd		0xA00C
#define PTP_RC_NIKON_MfDriveStepInsufficiency	0xA00E
#define PTP_RC_NIKON_NoFullHDPresent		0xA00F
#define PTP_RC_NIKON_StoreError			0xA021
#define PTP_RC_NIKON_StoreUnformatted		0xA022	/* from z6 sdk */
#define PTP_RC_NIKON_AdvancedTransferCancel	0xA022	/* dup from me*/
#define PTP_RC_NIKON_Bulb_Release_Busy		0xA200
#define PTP_RC_NIKON_Silent_Release_Busy	0xA201
#define PTP_RC_NIKON_MovieFrame_Release_Busy	0xA202
#define PTP_RC_NIKON_Shutter_Speed_Time		0xA204
#define PTP_RC_NIKON_Waiting_2ndRelease		0xA207
#define PTP_RC_NIKON_MirrorUpCapture_Already_Start		0xA208
#define PTP_RC_NIKON_Invalid_SBAttribute_Value	0xA209

/* Canon specific response codes */
#define PTP_RC_CANON_UNKNOWN_COMMAND		0xA001
#define PTP_RC_CANON_OPERATION_REFUSED		0xA005
#define PTP_RC_CANON_LENS_COVER			0xA006
#define PTP_RC_CANON_BATTERY_LOW		0xA101
#define PTP_RC_CANON_NOT_READY			0xA102

#define PTP_RC_CANON_A009			0xA009

#define PTP_RC_CANON_EOS_UnknownCommand		0xA001
#define PTP_RC_CANON_EOS_OperationRefused	0xA005
#define PTP_RC_CANON_EOS_LensCoverClosed	0xA006
#define PTP_RC_CANON_EOS_LowBattery		0xA101
#define PTP_RC_CANON_EOS_ObjectNotReady		0xA102
#define PTP_RC_CANON_EOS_CannotMakeObject	0xA104
#define PTP_RC_CANON_EOS_MemoryStatusNotReady	0xA106


/* Microsoft/MTP specific codes */
#define PTP_RC_MTP_Undefined			0xA800
#define PTP_RC_MTP_Invalid_ObjectPropCode	0xA801
#define PTP_RC_MTP_Invalid_ObjectProp_Format	0xA802
#define PTP_RC_MTP_Invalid_ObjectProp_Value	0xA803
#define PTP_RC_MTP_Invalid_ObjectReference	0xA804
#define PTP_RC_MTP_Invalid_Dataset		0xA806
#define PTP_RC_MTP_Specification_By_Group_Unsupported		0xA807
#define PTP_RC_MTP_Specification_By_Depth_Unsupported		0xA808
#define PTP_RC_MTP_Object_Too_Large		0xA809
#define PTP_RC_MTP_ObjectProp_Not_Supported	0xA80A

/* Microsoft Advanced Audio/Video Transfer response codes
(microsoft.com/AAVT 1.0) */
#define PTP_RC_MTP_Invalid_Media_Session_ID	0xA170
#define PTP_RC_MTP_Media_Session_Limit_Reached	0xA171
#define PTP_RC_MTP_No_More_Data			0xA172

/* WiFi Provisioning MTP Extension Error Codes (microsoft.com/WPDWCN: 1.0) */
#define PTP_RC_MTP_Invalid_WFC_Syntax		0xA121
#define PTP_RC_MTP_WFC_Version_Not_Supported	0xA122

/* libptp2 extended ERROR codes */
#define PTP_ERROR_NODEVICE		0x02F9
#define PTP_ERROR_TIMEOUT		0x02FA
#define PTP_ERROR_CANCEL		0x02FB
#define PTP_ERROR_BADPARAM		0x02FC
#define PTP_ERROR_RESP_EXPECTED		0x02FD
#define PTP_ERROR_DATA_EXPECTED		0x02FE
#define PTP_ERROR_IO			0x02FF

/* PTP Event Codes */

#define PTP_EC_Undefined		0x4000
#define PTP_EC_CancelTransaction	0x4001
#define PTP_EC_ObjectAdded		0x4002
#define PTP_EC_ObjectRemoved		0x4003
#define PTP_EC_StoreAdded		0x4004
#define PTP_EC_StoreRemoved		0x4005
#define PTP_EC_DevicePropChanged	0x4006
#define PTP_EC_ObjectInfoChanged	0x4007
#define PTP_EC_DeviceInfoChanged	0x4008
#define PTP_EC_RequestObjectTransfer	0x4009
#define PTP_EC_StoreFull		0x400A
#define PTP_EC_DeviceReset		0x400B
#define PTP_EC_StorageInfoChanged	0x400C
#define PTP_EC_CaptureComplete		0x400D
#define PTP_EC_UnreportedStatus		0x400E

/* Canon extension Event Codes */
#define PTP_EC_CANON_ExtendedErrorcode		0xC005	/* ? */
#define PTP_EC_CANON_ObjectInfoChanged		0xC008
#define PTP_EC_CANON_RequestObjectTransfer	0xC009
#define PTP_EC_CANON_ShutterButtonPressed0	0xC00B
#define PTP_EC_CANON_CameraModeChanged		0xC00C
#define PTP_EC_CANON_ShutterButtonPressed1	0xC00E

#define PTP_EC_CANON_StartDirectTransfer	0xC011
#define PTP_EC_CANON_StopDirectTransfer		0xC013

#define PTP_EC_CANON_TranscodeProgress		0xC01B /* EOS ? */

/* Canon EOS events */
#define PTP_EC_CANON_EOS_RequestGetEvent		0xc101
#define PTP_EC_CANON_EOS_RequestCancelTransferMA	0xc180
#define PTP_EC_CANON_EOS_ObjectAddedEx			0xc181
#define PTP_EC_CANON_EOS_ObjectRemoved			0xc182
#define PTP_EC_CANON_EOS_RequestGetObjectInfoEx		0xc183
#define PTP_EC_CANON_EOS_StorageStatusChanged		0xc184
#define PTP_EC_CANON_EOS_StorageInfoChanged		0xc185
#define PTP_EC_CANON_EOS_RequestObjectTransfer		0xc186
#define PTP_EC_CANON_EOS_ObjectInfoChangedEx		0xc187
#define PTP_EC_CANON_EOS_ObjectContentChanged		0xc188
#define PTP_EC_CANON_EOS_PropValueChanged		0xc189
#define PTP_EC_CANON_EOS_AvailListChanged		0xc18a
#define PTP_EC_CANON_EOS_CameraStatusChanged		0xc18b
#define PTP_EC_CANON_EOS_WillSoonShutdown		0xc18d
#define PTP_EC_CANON_EOS_ShutdownTimerUpdated		0xc18e
#define PTP_EC_CANON_EOS_RequestCancelTransfer		0xc18f
#define PTP_EC_CANON_EOS_RequestObjectTransferDT	0xc190
#define PTP_EC_CANON_EOS_RequestCancelTransferDT	0xc191
#define PTP_EC_CANON_EOS_StoreAdded			0xc192
#define PTP_EC_CANON_EOS_StoreRemoved			0xc193
#define PTP_EC_CANON_EOS_BulbExposureTime		0xc194
#define PTP_EC_CANON_EOS_RecordingTime			0xc195
#define PTP_EC_CANON_EOS_InnerDevelopParam		0xc196
#define PTP_EC_CANON_EOS_RequestObjectTransferDevelop	0xc197
#define PTP_EC_CANON_EOS_GPSLogOutputProgress		0xc198
#define PTP_EC_CANON_EOS_GPSLogOutputComplete		0xc199
#define PTP_EC_CANON_EOS_TouchTrans			0xc19a
#define PTP_EC_CANON_EOS_RequestObjectTransferExInfo	0xc19b
#define PTP_EC_CANON_EOS_PowerZoomInfoChanged		0xc19d
#define PTP_EC_CANON_EOS_RequestPushMode		0xc19f
#define PTP_EC_CANON_EOS_RequestObjectTransferTS	0xc1a2
#define PTP_EC_CANON_EOS_AfResult			0xc1a3
#define PTP_EC_CANON_EOS_CTGInfoCheckComplete		0xc1a4
#define PTP_EC_CANON_EOS_OLCInfoChanged			0xc1a5
#define PTP_EC_CANON_EOS_ObjectAddedEx64		0xc1a7
#define PTP_EC_CANON_EOS_ObjectInfoChangedEx64		0xc1a8
#define PTP_EC_CANON_EOS_RequestObjectTransfer64	0xc1a9
#define PTP_EC_CANON_EOS_RequestObjectTransferDT64	0xc1aa
#define PTP_EC_CANON_EOS_RequestObjectTransferFTP64	0xc1ab
#define PTP_EC_CANON_EOS_RequestObjectTransferInfoEx64	0xc1ac
#define PTP_EC_CANON_EOS_RequestObjectTransferMA64	0xc1ad
#define PTP_EC_CANON_EOS_ImportError			0xc1af
#define PTP_EC_CANON_EOS_BlePairing			0xc1b0
#define PTP_EC_CANON_EOS_RequestAutoSendImages		0xc1b1
#define PTP_EC_CANON_EOS_RequestTranscodedBlockTransfer	0xc1b2
#define PTP_EC_CANON_EOS_RequestCAssistImage		0xc1b4
#define PTP_EC_CANON_EOS_RequestObjectTransferFTP	0xc1f1

/* Nikon extension Event Codes */

/* Nikon extension Event Codes */
#define PTP_EC_Nikon_ObjectAddedInSDRAM		0xC101	/* e1: objecthandle */
#define PTP_EC_Nikon_CaptureCompleteRecInSdram	0xC102	/* no args */
/* Gets 1 parameter, objectid pointing to DPOF object */
#define PTP_EC_Nikon_AdvancedTransfer		0xC103
#define PTP_EC_Nikon_PreviewImageAdded		0xC104
#define PTP_EC_Nikon_MovieRecordInterrupted	0xC105	/* e1: errocode, e2: recordkind */
#define PTP_EC_Nikon_1stCaptureComplete		0xC106	/* 1st phase of mirror up is complete */
#define PTP_EC_Nikon_MirrorUpCancelComplete	0xC107	/* mirror up canceling is complete */
#define PTP_EC_Nikon_MovieRecordComplete	0xC108	/* e1: recordkind */
#define PTP_EC_Nikon_MovieRecordStarted		0xC10A	/* e1: recordkind */
#define PTP_EC_Nikon_PictureControlAdjustChanged	0xC10B	/* e1: picctrlitem e2: shootingmode */
#define PTP_EC_Nikon_LiveViewStateChanged	0xC10C	/* e1: liveview state */
#define PTP_EC_Nikon_ManualSettingsLensDataChanged	0xC10E	/* e1: lensnr */
#define PTP_EC_Nikon_ActiveSelectionInterrupted	0xC112	/* e1: errorcode */
#define PTP_EC_Nikon_SBAdded			0xC120	/* e1: sbhandle */
#define PTP_EC_Nikon_SBRemoved			0xC121	/* e1: sbhandle */
#define PTP_EC_Nikon_SBAttrChanged		0xC122	/* e1: sbhandle, e2: attrid */
#define PTP_EC_Nikon_SBGroupAttrChanged		0xC123	/* e1: sbgroupid, e2: groupattrid */

/* Sony */
#define PTP_EC_Sony_ObjectAdded			0xC201
#define PTP_EC_Sony_ObjectRemoved		0xC202
#define PTP_EC_Sony_PropertyChanged		0xC203

/* MTP Event codes */
#define PTP_EC_MTP_ObjectPropChanged		0xC801
#define PTP_EC_MTP_ObjectPropDescChanged	0xC802
#define PTP_EC_MTP_ObjectReferencesChanged	0xC803

#define PTP_EC_PARROT_Status			0xC201
#define PTP_EC_PARROT_MagnetoCalibrationStatus	0xC202

#define PTP_EC_PANASONIC_ObjectAdded		0xC108
#define PTP_EC_PANASONIC_ObjectAddedSDRAM	0xC109

/* Olympus E series, PTP style in the 2018+ range (e1mark2 etc.) */
/* From olympus capture tool */
#define PTP_EC_Olympus_CreateRecView		0xC001
#define PTP_EC_Olympus_CreateRecView_New	0xC101
#define PTP_EC_Olympus_ObjectAdded		0xC002
#define PTP_EC_Olympus_ObjectAdded_New		0xC102
#define PTP_EC_Olympus_AF_Frame			0xC003
#define PTP_EC_Olympus_AF_Frame_New		0xC103
#define PTP_EC_Olympus_DirectStoreImage		0xC004
#define PTP_EC_Olympus_DirectStoreImage_New	0xC104
#define PTP_EC_Olympus_ComplateCameraControlOff		0xC005
#define PTP_EC_Olympus_ComplateCameraControlOff_New	0xC105
#define PTP_EC_Olympus_AF_Frame_Over_Info	0xC006
#define PTP_EC_Olympus_AF_Frame_Over_Info_New	0xC106
#define PTP_EC_Olympus_DevicePropChanged	0xC008
#define PTP_EC_Olympus_DevicePropChanged_New	0xC108
#define PTP_EC_Olympus_ImageTransferModeFinish	0xC00C
#define PTP_EC_Olympus_ImageTransferModeFinish_New	0xC10C
#define PTP_EC_Olympus_ImageRecordFinish	0xC00D
#define PTP_EC_Olympus_ImageRecordFinish_New	0xC10D
#define PTP_EC_Olympus_SlotStatusChange		0xC00E
#define PTP_EC_Olympus_SlotStatusChange_New	0xC10E
#define PTP_EC_Olympus_PrioritizeRecord		0xC00F
#define PTP_EC_Olympus_PrioritizeRecord_New	0xC10F
#define PTP_EC_Olympus_FailCombiningAfterShooting	0xC010
#define PTP_EC_Olympus_FailCombiningAfterShooting_New	0xC110
#define PTP_EC_Olympus_NotifyAFTargetFrame	0xC011
#define PTP_EC_Olympus_NotifyAFTargetFrame_New	0xC111
#define PTP_EC_Olympus_RawEditParamChanged	0xC112
#define PTP_EC_Olympus_OlyNotifyCreateDrawEdit	0xC113

/* Used by the XML based E series driver */
#define PTP_EC_Olympus_PropertyChanged		0xC102
#define PTP_EC_Olympus_CaptureComplete		0xC103

#define PTP_EC_FUJI_PreviewAvailable		0xC001
#define PTP_EC_FUJI_ObjectAdded			0xC004

/* constants for GetObjectHandles */
#define PTP_GOH_ALL_STORAGE 0xffffffff
#define PTP_GOH_ALL_FORMATS 0x00000000
#define PTP_GOH_ALL_ASSOCS  0x00000000
#define PTP_GOH_ROOT_PARENT 0xffffffff

/* PTP device info structure (returned by GetDevInfo) */

struct _PTPDeviceInfo {
	uint16_t StandardVersion;
	uint32_t VendorExtensionID;
	uint16_t VendorExtensionVersion;
	char	*VendorExtensionDesc;
	uint16_t FunctionalMode;
	uint32_t OperationsSupported_len;
	uint16_t *OperationsSupported;
	uint32_t EventsSupported_len;
	uint16_t *EventsSupported;
	uint32_t DevicePropertiesSupported_len;
	uint16_t *DevicePropertiesSupported;
	uint32_t CaptureFormats_len;
	uint16_t *CaptureFormats;
	uint32_t ImageFormats_len;
	uint16_t *ImageFormats;
	char	*Manufacturer;
	char	*Model;
	char	*DeviceVersion;
	char	*SerialNumber;
};
typedef struct _PTPDeviceInfo PTPDeviceInfo;

/* PTP storageIDs structute (returned by GetStorageIDs) */

struct _PTPStorageIDs {
	uint32_t n;
	uint32_t *Storage;
};
typedef struct _PTPStorageIDs PTPStorageIDs;

/* PTP StorageInfo structure (returned by GetStorageInfo) */
struct _PTPStorageInfo {
	uint16_t StorageType;
	uint16_t FilesystemType;
	uint16_t AccessCapability;
	uint64_t MaxCapability;
	uint64_t FreeSpaceInBytes;
	uint32_t FreeSpaceInImages;
	char 	*StorageDescription;
	char	*VolumeLabel;
};
typedef struct _PTPStorageInfo PTPStorageInfo;

/* PTP Stream Info */
struct _PTPStreamInfo {
	uint64_t	DatasetSize;
	uint64_t	TimeResolution;
	uint32_t	FrameHeaderSize;
	uint32_t	FrameMaxSize;
	uint32_t	PacketHeaderSize;
	uint32_t	PacketMaxSize;
	uint32_t	PacketAlignment;
};
typedef struct _PTPStreamInfo PTPStreamInfo;

/* PTP objecthandles structure (returned by GetObjectHandles) */

struct _PTPObjectHandles {
	uint32_t n;
	uint32_t *Handler;
};
typedef struct _PTPObjectHandles PTPObjectHandles;

#define PTP_HANDLER_SPECIAL	0xffffffff
#define PTP_HANDLER_ROOT	0x00000000


/* PTP objectinfo structure (returned by GetObjectInfo) */

struct _PTPObjectInfo {
	uint32_t StorageID;
	uint16_t ObjectFormat;
	uint16_t ProtectionStatus;
	/* In the regular objectinfo this is 32bit,
	 * but we keep the general object size here
	 * that also arrives via other methods and so
	 * use 64bit */
	uint64_t ObjectCompressedSize;
	uint16_t ThumbFormat;
	uint32_t ThumbCompressedSize;
	uint32_t ThumbPixWidth;
	uint32_t ThumbPixHeight;
	uint32_t ImagePixWidth;
	uint32_t ImagePixHeight;
	uint32_t ImageBitDepth;
	uint32_t ParentObject;
	uint16_t AssociationType;
	uint32_t AssociationDesc;
	uint32_t SequenceNumber;
	char 	*Filename;
	time_t	CaptureDate;
	time_t	ModificationDate;
	char	*Keywords;
};
typedef struct _PTPObjectInfo PTPObjectInfo;

struct _PTPObjectFilesystemInfo {
	uint32_t ObjectHandle;
	uint32_t StorageID;
	uint16_t ObjectFormat;
	uint16_t ProtectionStatus;
	uint64_t ObjectCompressedSize64;
	uint32_t ParentObject;
	uint16_t AssociationType;
	uint32_t AssociationDesc;
	uint32_t SequenceNumber;
	char 	*Filename;
	time_t	ModificationDate;
};
typedef struct _PTPObjectFilesystemInfo PTPObjectFilesystemInfo;

/* max ptp string length INCLUDING terminating null character */

#define PTP_MAXSTRLEN				255

/* PTP Object Format Codes */

/* ancillary formats */
#define PTP_OFC_Undefined			0x3000
#define PTP_OFC_Defined				0x3800
#define PTP_OFC_Association			0x3001
#define PTP_OFC_Script				0x3002
#define PTP_OFC_Executable			0x3003
#define PTP_OFC_Text				0x3004
#define PTP_OFC_HTML				0x3005
#define PTP_OFC_DPOF				0x3006
#define PTP_OFC_AIFF	 			0x3007
#define PTP_OFC_WAV				0x3008
#define PTP_OFC_MP3				0x3009
#define PTP_OFC_AVI				0x300A
#define PTP_OFC_MPEG				0x300B
#define PTP_OFC_ASF				0x300C
#define PTP_OFC_QT				0x300D /* guessing */
/* image formats */
#define PTP_OFC_EXIF_JPEG			0x3801
#define PTP_OFC_TIFF_EP				0x3802
#define PTP_OFC_FlashPix			0x3803
#define PTP_OFC_BMP				0x3804
#define PTP_OFC_CIFF				0x3805
#define PTP_OFC_Undefined_0x3806		0x3806
#define PTP_OFC_GIF				0x3807
#define PTP_OFC_JFIF				0x3808
#define PTP_OFC_PCD				0x3809
#define PTP_OFC_PICT				0x380A
#define PTP_OFC_PNG				0x380B
#define PTP_OFC_Undefined_0x380C		0x380C
#define PTP_OFC_TIFF				0x380D
#define PTP_OFC_TIFF_IT				0x380E
#define PTP_OFC_JP2				0x380F
#define PTP_OFC_JPX				0x3810
/* ptp v1.1 has only DNG new */
#define PTP_OFC_DNG				0x3811
/* Eastman Kodak extension ancillary format */
#define PTP_OFC_EK_M3U				0xb002
/* Canon extension */
#define PTP_OFC_CANON_CRW			0xb101
#define PTP_OFC_CANON_CRW3			0xb103
#define PTP_OFC_CANON_MOV			0xb104
#define PTP_OFC_CANON_MOV2			0xb105
#define PTP_OFC_CANON_CR3			0xb108
/* CHDK specific raw mode */
#define PTP_OFC_CANON_CHDK_CRW			0xb1ff
/* Sony */
#define PTP_OFC_SONY_RAW			0xb101
/* MTP extensions */
#define PTP_OFC_MTP_MediaCard			0xb211
#define PTP_OFC_MTP_MediaCardGroup		0xb212
#define PTP_OFC_MTP_Encounter			0xb213
#define PTP_OFC_MTP_EncounterBox		0xb214
#define PTP_OFC_MTP_M4A				0xb215
#define PTP_OFC_MTP_ZUNEUNDEFINED		0xb217 /* Unknown file type */
#define PTP_OFC_MTP_Firmware			0xb802
#define PTP_OFC_MTP_WindowsImageFormat		0xb881
#define PTP_OFC_MTP_UndefinedAudio		0xb900
#define PTP_OFC_MTP_WMA				0xb901
#define PTP_OFC_MTP_OGG				0xb902
#define PTP_OFC_MTP_AAC				0xb903
#define PTP_OFC_MTP_AudibleCodec		0xb904
#define PTP_OFC_MTP_FLAC			0xb906
#define PTP_OFC_MTP_SamsungPlaylist		0xb909
#define PTP_OFC_MTP_UndefinedVideo		0xb980
#define PTP_OFC_MTP_WMV				0xb981
#define PTP_OFC_MTP_MP4				0xb982
#define PTP_OFC_MTP_MP2				0xb983
#define PTP_OFC_MTP_3GP				0xb984
#define PTP_OFC_MTP_UndefinedCollection		0xba00
#define PTP_OFC_MTP_AbstractMultimediaAlbum	0xba01
#define PTP_OFC_MTP_AbstractImageAlbum		0xba02
#define PTP_OFC_MTP_AbstractAudioAlbum		0xba03
#define PTP_OFC_MTP_AbstractVideoAlbum		0xba04
#define PTP_OFC_MTP_AbstractAudioVideoPlaylist	0xba05
#define PTP_OFC_MTP_AbstractContactGroup	0xba06
#define PTP_OFC_MTP_AbstractMessageFolder	0xba07
#define PTP_OFC_MTP_AbstractChapteredProduction	0xba08
#define PTP_OFC_MTP_AbstractAudioPlaylist	0xba09
#define PTP_OFC_MTP_AbstractVideoPlaylist	0xba0a
#define PTP_OFC_MTP_AbstractMediacast		0xba0b
#define PTP_OFC_MTP_WPLPlaylist			0xba10
#define PTP_OFC_MTP_M3UPlaylist			0xba11
#define PTP_OFC_MTP_MPLPlaylist			0xba12
#define PTP_OFC_MTP_ASXPlaylist			0xba13
#define PTP_OFC_MTP_PLSPlaylist			0xba14
#define PTP_OFC_MTP_UndefinedDocument		0xba80
#define PTP_OFC_MTP_AbstractDocument		0xba81
#define PTP_OFC_MTP_XMLDocument			0xba82
#define PTP_OFC_MTP_MSWordDocument		0xba83
#define PTP_OFC_MTP_MHTCompiledHTMLDocument	0xba84
#define PTP_OFC_MTP_MSExcelSpreadsheetXLS	0xba85
#define PTP_OFC_MTP_MSPowerpointPresentationPPT	0xba86
#define PTP_OFC_MTP_UndefinedMessage		0xbb00
#define PTP_OFC_MTP_AbstractMessage		0xbb01
#define PTP_OFC_MTP_UndefinedContact		0xbb80
#define PTP_OFC_MTP_AbstractContact		0xbb81
#define PTP_OFC_MTP_vCard2			0xbb82
#define PTP_OFC_MTP_vCard3			0xbb83
#define PTP_OFC_MTP_UndefinedCalendarItem	0xbe00
#define PTP_OFC_MTP_AbstractCalendarItem	0xbe01
#define PTP_OFC_MTP_vCalendar1			0xbe02
#define PTP_OFC_MTP_vCalendar2			0xbe03
#define PTP_OFC_MTP_UndefinedWindowsExecutable	0xbe80
#define PTP_OFC_MTP_MediaCast			0xbe81
#define PTP_OFC_MTP_Section			0xbe82

/* PTP Association Types */
#define PTP_AT_Undefined			0x0000
#define PTP_AT_GenericFolder			0x0001
#define PTP_AT_Album				0x0002
#define PTP_AT_TimeSequence			0x0003
#define PTP_AT_HorizontalPanoramic		0x0004
#define PTP_AT_VerticalPanoramic		0x0005
#define PTP_AT_2DPanoramic			0x0006
#define PTP_AT_AncillaryData			0x0007

/* PTP Protection Status */

#define PTP_PS_NoProtection			0x0000
#define PTP_PS_ReadOnly				0x0001
#define PTP_PS_MTP_ReadOnlyData			0x8002
#define PTP_PS_MTP_NonTransferableData		0x8003

/* PTP Storage Types */

#define PTP_ST_Undefined			0x0000
#define PTP_ST_FixedROM				0x0001
#define PTP_ST_RemovableROM			0x0002
#define PTP_ST_FixedRAM				0x0003
#define PTP_ST_RemovableRAM			0x0004

/* PTP FilesystemType Values */

#define PTP_FST_Undefined			0x0000
#define PTP_FST_GenericFlat			0x0001
#define PTP_FST_GenericHierarchical		0x0002
#define PTP_FST_DCF				0x0003

/* PTP StorageInfo AccessCapability Values */

#define PTP_AC_ReadWrite			0x0000
#define PTP_AC_ReadOnly				0x0001
#define PTP_AC_ReadOnly_with_Object_Deletion	0x0002

/* Property Describing Dataset, Range Form */

union _PTPPropertyValue {
	char		*str;	/* common string, malloced */
	uint8_t		u8;
	int8_t		i8;
	uint16_t	u16;
	int16_t		i16;
	uint32_t	u32;
	int32_t		i32;
	uint64_t	u64;
	int64_t		i64;
	/* XXXX: 128 bit signed and unsigned missing */
	struct array {
		uint32_t	count;
		union _PTPPropertyValue	*v;	/* malloced, count elements */
	} a;
};

typedef union _PTPPropertyValue PTPPropertyValue;

/* Metadata lists for MTP operations */
struct _MTPProperties {
	uint16_t 	 	property;
	uint16_t 	 	datatype;
	uint32_t 	 	ObjectHandle;
	PTPPropertyValue 	propval;
};
typedef struct _MTPProperties MTPProperties;

struct _PTPPropDescRangeForm {
	PTPPropertyValue 	MinimumValue;
	PTPPropertyValue 	MaximumValue;
	PTPPropertyValue 	StepSize;
};
typedef struct _PTPPropDescRangeForm PTPPropDescRangeForm;

/* Property Describing Dataset, Enum Form */

struct _PTPPropDescEnumForm {
	uint16_t		NumberOfValues;
	PTPPropertyValue	*SupportedValue;	/* malloced */
};
typedef struct _PTPPropDescEnumForm PTPPropDescEnumForm;

struct _PTPPropDescArrayLengthForm {
	uint16_t		NumberOfValues;
};
typedef struct _PTPPropDescArrayLengthForm PTPPropDescArrayLengthForm;

struct _PTPPropDescStringForm {
	char			*String;
};
typedef struct _PTPPropDescStringForm PTPPropDescStringForm;

/* Device Property Describing Dataset (DevicePropDesc) */

struct _PTPDevicePropDesc {
	uint16_t		DevicePropertyCode;
	uint16_t		DataType;
	uint8_t			GetSet;
	PTPPropertyValue	FactoryDefaultValue;
	PTPPropertyValue	CurrentValue;
	uint8_t			FormFlag;
	union	{
		PTPPropDescEnumForm	Enum;
		PTPPropDescRangeForm	Range;
	} FORM;
};
typedef struct _PTPDevicePropDesc PTPDevicePropDesc;

/* Object Property Describing Dataset (DevicePropDesc) */

struct _PTPObjectPropDesc {
	uint16_t		ObjectPropertyCode;
	uint16_t		DataType;
	uint8_t			GetSet;
	PTPPropertyValue	FactoryDefaultValue;
	uint32_t		GroupCode;
	uint8_t			FormFlag;
	union	{
		PTPPropDescEnumForm		Enum;
		PTPPropDescRangeForm		Range;
		PTPPropDescStringForm		DateTime;
		PTPPropDescArrayLengthForm	FixedLengthArray;
		PTPPropDescStringForm		RegularExpression;
		PTPPropDescArrayLengthForm	ByteArray;
		PTPPropDescStringForm		LongString;
	} FORM;
};
typedef struct _PTPObjectPropDesc PTPObjectPropDesc;

/* Canon filesystem's folder entry Dataset */

#define PTP_CANON_FilenameBufferLen	13
#define PTP_CANON_FolderEntryLen	28

struct _PTPCANONFolderEntry {
	uint32_t	ObjectHandle;
	uint16_t	ObjectFormatCode;
	uint8_t		Flags;
	uint32_t	ObjectSize;
	time_t		Time;
	char		Filename[PTP_CANON_FilenameBufferLen];

	uint32_t	StorageID;
};
typedef struct _PTPCANONFolderEntry PTPCANONFolderEntry;

/* Nikon Tone Curve Data */

#define PTP_NIKON_MaxCurvePoints 19

struct _PTPNIKONCoordinatePair {
	uint8_t		X;
	uint8_t		Y;
};

typedef struct _PTPNIKONCoordinatePair PTPNIKONCoordinatePair;

struct _PTPNTCCoordinatePair {
	uint8_t		X;
	uint8_t		Y;
};

typedef struct _PTPNTCCoordinatePair PTPNTCCoordinatePair;

struct _PTPNIKONCurveData {
	char 			static_preamble[6];
	uint8_t			XAxisStartPoint;
	uint8_t			XAxisEndPoint;
	uint8_t			YAxisStartPoint;
	uint8_t			YAxisEndPoint;
	uint8_t			MidPointIntegerPart;
	uint8_t			MidPointDecimalPart;
	uint8_t			NCoordinates;
	PTPNIKONCoordinatePair	CurveCoordinates[PTP_NIKON_MaxCurvePoints];
};

typedef struct _PTPNIKONCurveData PTPNIKONCurveData;

struct _PTPEKTextParams {
	char	*title;
	char	*line[5];
};
typedef struct _PTPEKTextParams PTPEKTextParams;

/* Nikon Wifi profiles */

struct _PTPNIKONWifiProfile {
	/* Values valid both when reading and writing profiles */
	char      profile_name[17];
	uint8_t   device_type;
	uint8_t   icon_type;
	char      essid[33];

	/* Values only valid when reading. Some of these are in the write packet,
	 * but are set automatically, like id, display_order and creation_date. */
	uint8_t   id;
	uint8_t   valid;
	uint8_t   display_order;
	char      creation_date[16];
	char      lastusage_date[16];

	/* Values only valid when writing */
	uint32_t  ip_address;
	uint8_t   subnet_mask; /* first zero bit position, e.g. 24 for 255.255.255.0 */
	uint32_t  gateway_address;
	uint8_t   address_mode; /* 0 - Manual, 2-3 -  DHCP ad-hoc/managed*/
	uint8_t   access_mode; /* 0 - Managed, 1 - Adhoc */
	uint8_t   wifi_channel; /* 1-11 */
	uint8_t   authentification; /* 0 - Open, 1 - Shared, 2 - WPA-PSK */
	uint8_t   encryption; /* 0 - None, 1 - WEP 64bit, 2 - WEP 128bit (not supported: 3 - TKIP) */
	uint8_t   key[64];
	uint8_t   key_nr;
/*	char      guid[16]; */
};

typedef struct _PTPNIKONWifiProfile PTPNIKONWifiProfile;

enum _PTPCanon_changes_types {
	PTP_CANON_EOS_CHANGES_TYPE_UNKNOWN,
	PTP_CANON_EOS_CHANGES_TYPE_OBJECTINFO,
	PTP_CANON_EOS_CHANGES_TYPE_OBJECTTRANSFER,
	PTP_CANON_EOS_CHANGES_TYPE_PROPERTY,
	PTP_CANON_EOS_CHANGES_TYPE_CAMERASTATUS,
	PTP_CANON_EOS_CHANGES_TYPE_FOCUSINFO,
	PTP_CANON_EOS_CHANGES_TYPE_FOCUSMASK,
	PTP_CANON_EOS_CHANGES_TYPE_OBJECTREMOVED,
	PTP_CANON_EOS_CHANGES_TYPE_OBJECTINFO_CHANGE,
	PTP_CANON_EOS_CHANGES_TYPE_OBJECTCONTENT_CHANGE
};

struct _PTPCanon_New_Object {
	uint32_t	oid;
	PTPObjectInfo	oi;
};

struct _PTPCanon_changes_entry {
	enum _PTPCanon_changes_types	type;
	union {
		struct _PTPCanon_New_Object	object;	/* TYPE_OBJECTINFO */
		char				*info;
		uint16_t			propid;
		int				status;
	} u;
};
typedef struct _PTPCanon_changes_entry PTPCanon_changes_entry;

typedef struct _PTPCanon_Property {
	uint32_t		size;
	uint32_t		proptype;
	unsigned char		*data;

	/* fill out for queries */
	PTPDevicePropDesc	dpd;
} PTPCanon_Property;

typedef struct _PTPCanonEOSDeviceInfo {
	/* length */
	uint32_t EventsSupported_len;
	uint32_t *EventsSupported;

	uint32_t DevicePropertiesSupported_len;
	uint32_t *DevicePropertiesSupported;

	uint32_t unk_len;
	uint32_t *unk;
} PTPCanonEOSDeviceInfo;

/* DataType Codes */

#define PTP_DTC_UNDEF		0x0000
#define PTP_DTC_INT8		0x0001
#define PTP_DTC_UINT8		0x0002
#define PTP_DTC_INT16		0x0003
#define PTP_DTC_UINT16		0x0004
#define PTP_DTC_INT32		0x0005
#define PTP_DTC_UINT32		0x0006
#define PTP_DTC_INT64		0x0007
#define PTP_DTC_UINT64		0x0008
#define PTP_DTC_INT128		0x0009
#define PTP_DTC_UINT128		0x000A

#define PTP_DTC_ARRAY_MASK	0x4000

#define PTP_DTC_AINT8		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT8)
#define PTP_DTC_AUINT8		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT8)
#define PTP_DTC_AINT16		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT16)
#define PTP_DTC_AUINT16		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT16)
#define PTP_DTC_AINT32		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT32)
#define PTP_DTC_AUINT32		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT32)
#define PTP_DTC_AINT64		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT64)
#define PTP_DTC_AUINT64		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT64)
#define PTP_DTC_AINT128		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT128)
#define PTP_DTC_AUINT128	(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT128)

#define PTP_DTC_STR		0xFFFF

/* Device Properties Codes */

/* PTP v1.0 property codes */
#define PTP_DPC_Undefined		0x5000
#define PTP_DPC_BatteryLevel		0x5001
#define PTP_DPC_FunctionalMode		0x5002
#define PTP_DPC_ImageSize		0x5003
#define PTP_DPC_CompressionSetting	0x5004
#define PTP_DPC_WhiteBalance		0x5005
#define PTP_DPC_RGBGain			0x5006
#define PTP_DPC_FNumber			0x5007
#define PTP_DPC_FocalLength		0x5008
#define PTP_DPC_FocusDistance		0x5009
#define PTP_DPC_FocusMode		0x500A
#define PTP_DPC_ExposureMeteringMode	0x500B
#define PTP_DPC_FlashMode		0x500C
#define PTP_DPC_ExposureTime		0x500D
#define PTP_DPC_ExposureProgramMode	0x500E
#define PTP_DPC_ExposureIndex		0x500F
#define PTP_DPC_ExposureBiasCompensation	0x5010
#define PTP_DPC_DateTime		0x5011
#define PTP_DPC_CaptureDelay		0x5012
#define PTP_DPC_StillCaptureMode	0x5013
#define PTP_DPC_Contrast		0x5014
#define PTP_DPC_Sharpness		0x5015
#define PTP_DPC_DigitalZoom		0x5016
#define PTP_DPC_EffectMode		0x5017
#define PTP_DPC_BurstNumber		0x5018
#define PTP_DPC_BurstInterval		0x5019
#define PTP_DPC_TimelapseNumber		0x501A
#define PTP_DPC_TimelapseInterval	0x501B
#define PTP_DPC_FocusMeteringMode	0x501C
#define PTP_DPC_UploadURL		0x501D
#define PTP_DPC_Artist			0x501E
#define PTP_DPC_CopyrightInfo		0x501F
/* PTP v1.1 property codes */
#define PTP_DPC_SupportedStreams	0x5020
#define PTP_DPC_EnabledStreams		0x5021
#define PTP_DPC_VideoFormat		0x5022
#define PTP_DPC_VideoResolution		0x5023
#define PTP_DPC_VideoQuality		0x5024
#define PTP_DPC_VideoFrameRate		0x5025
#define PTP_DPC_VideoContrast		0x5026
#define PTP_DPC_VideoBrightness		0x5027
#define PTP_DPC_AudioFormat		0x5028
#define PTP_DPC_AudioBitrate		0x5029
#define PTP_DPC_AudioSamplingRate	0x502A
#define PTP_DPC_AudioBitPerSample	0x502B
#define PTP_DPC_AudioVolume		0x502C

/* Proprietary vendor extension device property mask */
#define PTP_DPC_EXTENSION_MASK		0xF000
#define PTP_DPC_EXTENSION		0xD000

/* Zune extension device property codes */
#define PTP_DPC_MTP_ZUNE_UNKNOWN1	0xD181
#define PTP_DPC_MTP_ZUNE_UNKNOWN2	0xD132
#define PTP_DPC_MTP_ZUNE_UNKNOWN3	0xD215
#define PTP_DPC_MTP_ZUNE_UNKNOWN4	0xD216

/* Eastman Kodak extension device property codes */
#define PTP_DPC_EK_ColorTemperature	0xD001
#define PTP_DPC_EK_DateTimeStampFormat	0xD002
#define PTP_DPC_EK_BeepMode		0xD003
#define PTP_DPC_EK_VideoOut		0xD004
#define PTP_DPC_EK_PowerSaving		0xD005
#define PTP_DPC_EK_UI_Language		0xD006

/* Canon extension device property codes */
#define PTP_DPC_CANON_BeepMode		0xD001
#define PTP_DPC_CANON_BatteryKind	0xD002
#define PTP_DPC_CANON_BatteryStatus	0xD003
#define PTP_DPC_CANON_UILockType	0xD004
#define PTP_DPC_CANON_CameraMode	0xD005
#define PTP_DPC_CANON_ImageQuality	0xD006
#define PTP_DPC_CANON_FullViewFileFormat 0xD007
#define PTP_DPC_CANON_ImageSize		0xD008
#define PTP_DPC_CANON_SelfTime		0xD009
#define PTP_DPC_CANON_FlashMode		0xD00A
#define PTP_DPC_CANON_Beep		0xD00B
#define PTP_DPC_CANON_ShootingMode	0xD00C
#define PTP_DPC_CANON_ImageMode		0xD00D
#define PTP_DPC_CANON_DriveMode		0xD00E
#define PTP_DPC_CANON_EZoom		0xD00F
#define PTP_DPC_CANON_MeteringMode	0xD010
#define PTP_DPC_CANON_AFDistance	0xD011
#define PTP_DPC_CANON_FocusingPoint	0xD012
#define PTP_DPC_CANON_WhiteBalance	0xD013
#define PTP_DPC_CANON_SlowShutterSetting	0xD014
#define PTP_DPC_CANON_AFMode		0xD015
#define PTP_DPC_CANON_ImageStabilization	0xD016
#define PTP_DPC_CANON_Contrast		0xD017
#define PTP_DPC_CANON_ColorGain		0xD018
#define PTP_DPC_CANON_Sharpness		0xD019
#define PTP_DPC_CANON_Sensitivity	0xD01A
#define PTP_DPC_CANON_ParameterSet	0xD01B
#define PTP_DPC_CANON_ISOSpeed		0xD01C
#define PTP_DPC_CANON_Aperture		0xD01D
#define PTP_DPC_CANON_ShutterSpeed	0xD01E
#define PTP_DPC_CANON_ExpCompensation	0xD01F
#define PTP_DPC_CANON_FlashCompensation	0xD020
#define PTP_DPC_CANON_AEBExposureCompensation	0xD021
#define PTP_DPC_CANON_AvOpen		0xD023
#define PTP_DPC_CANON_AvMax		0xD024
#define PTP_DPC_CANON_FocalLength	0xD025
#define PTP_DPC_CANON_FocalLengthTele	0xD026
#define PTP_DPC_CANON_FocalLengthWide	0xD027
#define PTP_DPC_CANON_FocalLengthDenominator	0xD028
#define PTP_DPC_CANON_CaptureTransferMode	0xD029
#define CANON_TRANSFER_ENTIRE_IMAGE_TO_PC	0x0002
#define CANON_TRANSFER_SAVE_THUMBNAIL_TO_DEVICE	0x0004
#define CANON_TRANSFER_SAVE_IMAGE_TO_DEVICE	0x0008
/* we use those values: */
#define CANON_TRANSFER_MEMORY		(2|1)
#define CANON_TRANSFER_CARD		(8|4|1)

#define PTP_DPC_CANON_Zoom		0xD02A
#define PTP_DPC_CANON_NamePrefix	0xD02B
#define PTP_DPC_CANON_SizeQualityMode	0xD02C
#define PTP_DPC_CANON_SupportedThumbSize	0xD02D
#define PTP_DPC_CANON_SizeOfOutputDataFromCamera	0xD02E
#define PTP_DPC_CANON_SizeOfInputDataToCamera		0xD02F
#define PTP_DPC_CANON_RemoteAPIVersion	0xD030
#define PTP_DPC_CANON_FirmwareVersion	0xD031
#define PTP_DPC_CANON_CameraModel	0xD032
#define PTP_DPC_CANON_CameraOwner	0xD033
#define PTP_DPC_CANON_UnixTime		0xD034
#define PTP_DPC_CANON_CameraBodyID	0xD035
#define PTP_DPC_CANON_CameraOutput	0xD036
#define PTP_DPC_CANON_DispAv		0xD037
#define PTP_DPC_CANON_AvOpenApex	0xD038
#define PTP_DPC_CANON_DZoomMagnification	0xD039
#define PTP_DPC_CANON_MlSpotPos		0xD03A
#define PTP_DPC_CANON_DispAvMax		0xD03B
#define PTP_DPC_CANON_AvMaxApex		0xD03C
#define PTP_DPC_CANON_EZoomStartPosition		0xD03D
#define PTP_DPC_CANON_FocalLengthOfTele	0xD03E
#define PTP_DPC_CANON_EZoomSizeOfTele	0xD03F
#define PTP_DPC_CANON_PhotoEffect	0xD040
#define PTP_DPC_CANON_AssistLight	0xD041
#define PTP_DPC_CANON_FlashQuantityCount	0xD042
#define PTP_DPC_CANON_RotationAngle	0xD043
#define PTP_DPC_CANON_RotationScene	0xD044
#define PTP_DPC_CANON_EventEmulateMode	0xD045
#define PTP_DPC_CANON_DPOFVersion	0xD046
#define PTP_DPC_CANON_TypeOfSupportedSlideShow	0xD047
#define PTP_DPC_CANON_AverageFilesizes	0xD048
#define PTP_DPC_CANON_ModelID		0xD049

#define PTP_DPC_CANON_EOS_PowerZoomPosition	0xD055
#define PTP_DPC_CANON_EOS_StrobeSettingSimple	0xD056
#define PTP_DPC_CANON_EOS_ConnectTrigger	0xD058
#define PTP_DPC_CANON_EOS_ChangeCameraMode	0xD059

/* From EOS 400D trace. */
#define PTP_DPC_CANON_EOS_Aperture		0xD101
#define PTP_DPC_CANON_EOS_ShutterSpeed		0xD102
#define PTP_DPC_CANON_EOS_ISOSpeed		0xD103
#define PTP_DPC_CANON_EOS_ExpCompensation	0xD104
#define PTP_DPC_CANON_EOS_AutoExposureMode	0xD105
#define PTP_DPC_CANON_EOS_DriveMode		0xD106
#define PTP_DPC_CANON_EOS_MeteringMode		0xD107
#define PTP_DPC_CANON_EOS_FocusMode		0xD108
#define PTP_DPC_CANON_EOS_WhiteBalance		0xD109
#define PTP_DPC_CANON_EOS_ColorTemperature	0xD10A
#define PTP_DPC_CANON_EOS_WhiteBalanceAdjustA	0xD10B
#define PTP_DPC_CANON_EOS_WhiteBalanceAdjustB	0xD10C
#define PTP_DPC_CANON_EOS_WhiteBalanceXA	0xD10D
#define PTP_DPC_CANON_EOS_WhiteBalanceXB	0xD10E
#define PTP_DPC_CANON_EOS_ColorSpace		0xD10F
#define PTP_DPC_CANON_EOS_PictureStyle		0xD110
#define PTP_DPC_CANON_EOS_BatteryPower		0xD111
#define PTP_DPC_CANON_EOS_BatterySelect		0xD112
#define PTP_DPC_CANON_EOS_CameraTime		0xD113
#define PTP_DPC_CANON_EOS_AutoPowerOff		0xD114
#define PTP_DPC_CANON_EOS_Owner			0xD115
#define PTP_DPC_CANON_EOS_ModelID		0xD116
#define PTP_DPC_CANON_EOS_PTPExtensionVersion	0xD119
#define PTP_DPC_CANON_EOS_DPOFVersion		0xD11A
#define PTP_DPC_CANON_EOS_AvailableShots	0xD11B
#define PTP_CANON_EOS_CAPTUREDEST_HD		4
#define PTP_DPC_CANON_EOS_CaptureDestination	0xD11C
#define PTP_DPC_CANON_EOS_BracketMode		0xD11D
#define PTP_DPC_CANON_EOS_CurrentStorage	0xD11E
#define PTP_DPC_CANON_EOS_CurrentFolder		0xD11F
#define PTP_DPC_CANON_EOS_ImageFormat		0xD120	/* file setting */
#define PTP_DPC_CANON_EOS_ImageFormatCF		0xD121	/* file setting CF */
#define PTP_DPC_CANON_EOS_ImageFormatSD		0xD122	/* file setting SD */
#define PTP_DPC_CANON_EOS_ImageFormatExtHD	0xD123	/* file setting exthd */
#define PTP_DPC_CANON_EOS_RefocusState		0xD124
#define PTP_DPC_CANON_EOS_CameraNickname	0xD125
#define PTP_DPC_CANON_EOS_StroboSettingExpCompositionControl	0xD126
#define PTP_DPC_CANON_EOS_ConnectStatus		0xD127
#define PTP_DPC_CANON_EOS_LensBarrelStatus	0xD128
#define PTP_DPC_CANON_EOS_SilentShutterSetting	0xD129
#define PTP_DPC_CANON_EOS_LV_AF_EyeDetect	0xD12C
#define PTP_DPC_CANON_EOS_AutoTransMobile	0xD12D
#define PTP_DPC_CANON_EOS_URLSupportFormat	0xD12E
#define PTP_DPC_CANON_EOS_SpecialAcc		0xD12F
#define PTP_DPC_CANON_EOS_CompressionS		0xD130
#define PTP_DPC_CANON_EOS_CompressionM1		0xD131
#define PTP_DPC_CANON_EOS_CompressionM2		0xD132
#define PTP_DPC_CANON_EOS_CompressionL		0xD133
#define PTP_DPC_CANON_EOS_IntervalShootSetting	0xD134
#define PTP_DPC_CANON_EOS_IntervalShootState	0xD135
#define PTP_DPC_CANON_EOS_PushMode		0xD136
#define PTP_DPC_CANON_EOS_LvCFilterKind		0xD137
#define PTP_DPC_CANON_EOS_AEModeDial		0xD138
#define PTP_DPC_CANON_EOS_AEModeCustom		0xD139
#define PTP_DPC_CANON_EOS_MirrorUpSetting	0xD13A
#define PTP_DPC_CANON_EOS_HighlightTonePriority	0xD13B
#define PTP_DPC_CANON_EOS_AFSelectFocusArea	0xD13C
#define PTP_DPC_CANON_EOS_HDRSetting		0xD13D
#define PTP_DPC_CANON_EOS_TimeShootSetting	0xD13E
#define PTP_DPC_CANON_EOS_NFCApplicationInfo	0xD13F
#define PTP_DPC_CANON_EOS_PCWhiteBalance1	0xD140
#define PTP_DPC_CANON_EOS_PCWhiteBalance2	0xD141
#define PTP_DPC_CANON_EOS_PCWhiteBalance3	0xD142
#define PTP_DPC_CANON_EOS_PCWhiteBalance4	0xD143
#define PTP_DPC_CANON_EOS_PCWhiteBalance5	0xD144
#define PTP_DPC_CANON_EOS_MWhiteBalance		0xD145
#define PTP_DPC_CANON_EOS_MWhiteBalanceEx	0xD146
#define PTP_DPC_CANON_EOS_PowerZoomSpeed	0xD149
#define PTP_DPC_CANON_EOS_NetworkServerRegion	0xD14A
#define PTP_DPC_CANON_EOS_GPSLogCtrl		0xD14B
#define PTP_DPC_CANON_EOS_GPSLogListNum		0xD14C
#define PTP_DPC_CANON_EOS_UnknownPropD14D	0xD14D  /*found in Canon EOS 5D M3*/
#define PTP_DPC_CANON_EOS_PictureStyleStandard	0xD150
#define PTP_DPC_CANON_EOS_PictureStylePortrait	0xD151
#define PTP_DPC_CANON_EOS_PictureStyleLandscape	0xD152
#define PTP_DPC_CANON_EOS_PictureStyleNeutral	0xD153
#define PTP_DPC_CANON_EOS_PictureStyleFaithful	0xD154
#define PTP_DPC_CANON_EOS_PictureStyleBlackWhite	0xD155
#define PTP_DPC_CANON_EOS_PictureStyleAuto	0xD156
#define PTP_DPC_CANON_EOS_PictureStyleExStandard	0xD157
#define PTP_DPC_CANON_EOS_PictureStyleExPortrait	0xD158
#define PTP_DPC_CANON_EOS_PictureStyleExLandscape	0xD159
#define PTP_DPC_CANON_EOS_PictureStyleExNeutral		0xD15A
#define PTP_DPC_CANON_EOS_PictureStyleExFaithful	0xD15B
#define PTP_DPC_CANON_EOS_PictureStyleExBlackWhite	0xD15C
#define PTP_DPC_CANON_EOS_PictureStyleExAuto		0xD15D
#define PTP_DPC_CANON_EOS_PictureStyleExFineDetail	0xD15E
#define PTP_DPC_CANON_EOS_PictureStyleUserSet1	0xD160
#define PTP_DPC_CANON_EOS_PictureStyleUserSet2	0xD161
#define PTP_DPC_CANON_EOS_PictureStyleUserSet3	0xD162
#define PTP_DPC_CANON_EOS_PictureStyleExUserSet1	0xD163
#define PTP_DPC_CANON_EOS_PictureStyleExUserSet2	0xD164
#define PTP_DPC_CANON_EOS_PictureStyleExUserSet3	0xD165
#define PTP_DPC_CANON_EOS_MovieAVModeFine	0xD166
#define PTP_DPC_CANON_EOS_ShutterReleaseCounter	0xD167	/* perhaps send a requestdeviceprop ex ? */
#define PTP_DPC_CANON_EOS_AvailableImageSize	0xD168
#define PTP_DPC_CANON_EOS_ErrorHistory		0xD169
#define PTP_DPC_CANON_EOS_LensExchangeHistory	0xD16A
#define PTP_DPC_CANON_EOS_StroboExchangeHistory	0xD16B
#define PTP_DPC_CANON_EOS_PictureStyleParam1	0xD170
#define PTP_DPC_CANON_EOS_PictureStyleParam2	0xD171
#define PTP_DPC_CANON_EOS_PictureStyleParam3	0xD172
#define PTP_DPC_CANON_EOS_MovieRecordVolumeLine	0xD174
#define PTP_DPC_CANON_EOS_NetworkCommunicationMode	0xD175
#define PTP_DPC_CANON_EOS_CanonLogGamma			0xD176
#define PTP_DPC_CANON_EOS_SmartphoneShowImageConfig	0xD177
#define PTP_DPC_CANON_EOS_HighISOSettingNoiseReduction	0xD178
#define PTP_DPC_CANON_EOS_MovieServoAF		0xD179
#define PTP_DPC_CANON_EOS_ContinuousAFValid	0xD17A
#define PTP_DPC_CANON_EOS_Attenuator		0xD17B
#define PTP_DPC_CANON_EOS_UTCTime		0xD17C
#define PTP_DPC_CANON_EOS_Timezone		0xD17D
#define PTP_DPC_CANON_EOS_Summertime		0xD17E
#define PTP_DPC_CANON_EOS_FlavorLUTParams	0xD17F
#define PTP_DPC_CANON_EOS_CustomFunc1		0xD180
#define PTP_DPC_CANON_EOS_CustomFunc2		0xD181
#define PTP_DPC_CANON_EOS_CustomFunc3		0xD182
#define PTP_DPC_CANON_EOS_CustomFunc4		0xD183
#define PTP_DPC_CANON_EOS_CustomFunc5		0xD184
#define PTP_DPC_CANON_EOS_CustomFunc6		0xD185
#define PTP_DPC_CANON_EOS_CustomFunc7		0xD186
#define PTP_DPC_CANON_EOS_CustomFunc8		0xD187
#define PTP_DPC_CANON_EOS_CustomFunc9		0xD188
#define PTP_DPC_CANON_EOS_CustomFunc10		0xD189
#define PTP_DPC_CANON_EOS_CustomFunc11		0xD18a
#define PTP_DPC_CANON_EOS_CustomFunc12		0xD18b
#define PTP_DPC_CANON_EOS_CustomFunc13		0xD18c
#define PTP_DPC_CANON_EOS_CustomFunc14		0xD18d
#define PTP_DPC_CANON_EOS_CustomFunc15		0xD18e
#define PTP_DPC_CANON_EOS_CustomFunc16		0xD18f
#define PTP_DPC_CANON_EOS_CustomFunc17		0xD190
#define PTP_DPC_CANON_EOS_CustomFunc18		0xD191
#define PTP_DPC_CANON_EOS_CustomFunc19		0xD192
#define PTP_DPC_CANON_EOS_CustomFunc19		0xD192
#define PTP_DPC_CANON_EOS_InnerDevelop		0xD193
#define PTP_DPC_CANON_EOS_MultiAspect		0xD194
#define PTP_DPC_CANON_EOS_MovieSoundRecord	0xD195
#define PTP_DPC_CANON_EOS_MovieRecordVolume	0xD196
#define PTP_DPC_CANON_EOS_WindCut		0xD197
#define PTP_DPC_CANON_EOS_ExtenderType		0xD198
#define PTP_DPC_CANON_EOS_OLCInfoVersion	0xD199
#define PTP_DPC_CANON_EOS_UnknownPropD19A	0xD19A /*found in Canon EOS 5D M3*/
#define PTP_DPC_CANON_EOS_UnknownPropD19C	0xD19C /*found in Canon EOS 5D M3*/
#define PTP_DPC_CANON_EOS_UnknownPropD19D	0xD19D /*found in Canon EOS 5D M3*/
#define PTP_DPC_CANON_EOS_GPSDeviceActive	0xD19F
#define PTP_DPC_CANON_EOS_CustomFuncEx		0xD1a0
#define PTP_DPC_CANON_EOS_MyMenu		0xD1a1
#define PTP_DPC_CANON_EOS_MyMenuList		0xD1a2
#define PTP_DPC_CANON_EOS_WftStatus		0xD1a3
#define PTP_DPC_CANON_EOS_WftInputTransmission	0xD1a4
#define PTP_DPC_CANON_EOS_HDDirectoryStructure	0xD1a5
#define PTP_DPC_CANON_EOS_BatteryInfo		0xD1a6
#define PTP_DPC_CANON_EOS_AdapterInfo		0xD1a7
#define PTP_DPC_CANON_EOS_LensStatus		0xD1a8
#define PTP_DPC_CANON_EOS_QuickReviewTime	0xD1a9
#define PTP_DPC_CANON_EOS_CardExtension		0xD1aa
#define PTP_DPC_CANON_EOS_TempStatus		0xD1ab
#define PTP_DPC_CANON_EOS_ShutterCounter	0xD1ac
#define PTP_DPC_CANON_EOS_SpecialOption		0xD1ad
#define PTP_DPC_CANON_EOS_PhotoStudioMode	0xD1ae
#define PTP_DPC_CANON_EOS_SerialNumber		0xD1af
#define PTP_DPC_CANON_EOS_EVFOutputDevice	0xD1b0
#define PTP_DPC_CANON_EOS_EVFMode		0xD1b1
#define PTP_DPC_CANON_EOS_DepthOfFieldPreview	0xD1b2
#define PTP_DPC_CANON_EOS_EVFSharpness		0xD1b3
#define PTP_DPC_CANON_EOS_EVFWBMode		0xD1b4
#define PTP_DPC_CANON_EOS_EVFClickWBCoeffs	0xD1b5
#define PTP_DPC_CANON_EOS_EVFColorTemp		0xD1b6
#define PTP_DPC_CANON_EOS_ExposureSimMode	0xD1b7
#define PTP_DPC_CANON_EOS_EVFRecordStatus	0xD1b8
#define PTP_DPC_CANON_EOS_LvAfSystem		0xD1ba
#define PTP_DPC_CANON_EOS_MovSize		0xD1bb
#define PTP_DPC_CANON_EOS_LvViewTypeSelect	0xD1bc
#define PTP_DPC_CANON_EOS_MirrorDownStatus	0xD1bd
#define PTP_DPC_CANON_EOS_MovieParam		0xD1be
#define PTP_DPC_CANON_EOS_MirrorLockupState	0xD1bf
#define PTP_DPC_CANON_EOS_FlashChargingState	0xD1C0
#define PTP_DPC_CANON_EOS_AloMode		0xD1C1
#define PTP_DPC_CANON_EOS_FixedMovie		0xD1C2
#define PTP_DPC_CANON_EOS_OneShotRawOn		0xD1C3
#define PTP_DPC_CANON_EOS_ErrorForDisplay	0xD1C4
#define PTP_DPC_CANON_EOS_AEModeMovie		0xD1C5
#define PTP_DPC_CANON_EOS_BuiltinStroboMode	0xD1C6
#define PTP_DPC_CANON_EOS_StroboDispState	0xD1C7
#define PTP_DPC_CANON_EOS_StroboETTL2Metering	0xD1C8
#define PTP_DPC_CANON_EOS_ContinousAFMode	0xD1C9
#define PTP_DPC_CANON_EOS_MovieParam2		0xD1CA
#define PTP_DPC_CANON_EOS_StroboSettingExpComposition		0xD1CB
#define PTP_DPC_CANON_EOS_MovieParam3		0xD1CC
#define PTP_DPC_CANON_EOS_MovieParam4		0xD1CD
#define PTP_DPC_CANON_EOS_LVMedicalRotate	0xD1CF
#define PTP_DPC_CANON_EOS_Artist		0xD1d0
#define PTP_DPC_CANON_EOS_Copyright		0xD1d1
#define PTP_DPC_CANON_EOS_BracketValue		0xD1d2
#define PTP_DPC_CANON_EOS_FocusInfoEx		0xD1d3
#define PTP_DPC_CANON_EOS_DepthOfField		0xD1d4
#define PTP_DPC_CANON_EOS_Brightness		0xD1d5
#define PTP_DPC_CANON_EOS_LensAdjustParams	0xD1d6
#define PTP_DPC_CANON_EOS_EFComp		0xD1d7
#define PTP_DPC_CANON_EOS_LensName		0xD1d8
#define PTP_DPC_CANON_EOS_AEB			0xD1d9
#define PTP_DPC_CANON_EOS_StroboSetting		0xD1da
#define PTP_DPC_CANON_EOS_StroboWirelessSetting	0xD1db
#define PTP_DPC_CANON_EOS_StroboFiring		0xD1dc
#define PTP_DPC_CANON_EOS_LensID		0xD1dd
#define PTP_DPC_CANON_EOS_LCDBrightness		0xD1de
#define PTP_DPC_CANON_EOS_CADarkBright		0xD1df

#define PTP_DPC_CANON_EOS_CAssistPreset		0xD201
#define PTP_DPC_CANON_EOS_CAssistBrightness	0xD202
#define PTP_DPC_CANON_EOS_CAssistContrast	0xD203
#define PTP_DPC_CANON_EOS_CAssistSaturation	0xD204
#define PTP_DPC_CANON_EOS_CAssistColorBA	0xD205
#define PTP_DPC_CANON_EOS_CAssistColorMG	0xD206
#define PTP_DPC_CANON_EOS_CAssistMonochrome	0xD207
#define PTP_DPC_CANON_EOS_FocusShiftSetting	0xD208
#define PTP_DPC_CANON_EOS_MovieSelfTimer	0xD209
#define PTP_DPC_CANON_EOS_Clarity		0xD20B
#define PTP_DPC_CANON_EOS_2GHDRSetting		0xD20C
#define PTP_DPC_CANON_EOS_MovieParam5		0xD20D
#define PTP_DPC_CANON_EOS_HDRViewAssistModeRec	0xD20E
#define PTP_DPC_CANON_EOS_PropFinderAFFrame	0xD214
#define PTP_DPC_CANON_EOS_VariableMovieRecSetting	0xD215
#define PTP_DPC_CANON_EOS_PropAutoRotate	0xD216
#define PTP_DPC_CANON_EOS_MFPeakingSetting	0xD217
#define PTP_DPC_CANON_EOS_MovieSpatialOversampling	0xD218
#define PTP_DPC_CANON_EOS_MovieCropMode		0xD219
#define PTP_DPC_CANON_EOS_ShutterType		0xD21A
#define PTP_DPC_CANON_EOS_WFTBatteryPower	0xD21B
#define PTP_DPC_CANON_EOS_BatteryInfoEx		0xD21C

/* Nikon extension device property codes */
#define PTP_DPC_NIKON_ShootingBank			0xD010
#define PTP_DPC_NIKON_ShootingBankNameA 		0xD011
#define PTP_DPC_NIKON_ShootingBankNameB			0xD012
#define PTP_DPC_NIKON_ShootingBankNameC			0xD013
#define PTP_DPC_NIKON_ShootingBankNameD			0xD014
#define PTP_DPC_NIKON_ResetBank0			0xD015
#define PTP_DPC_NIKON_RawCompression			0xD016
#define PTP_DPC_NIKON_WhiteBalanceAutoBias		0xD017
#define PTP_DPC_NIKON_WhiteBalanceTungstenBias		0xD018
#define PTP_DPC_NIKON_WhiteBalanceFluorescentBias	0xD019
#define PTP_DPC_NIKON_WhiteBalanceDaylightBias		0xD01A
#define PTP_DPC_NIKON_WhiteBalanceFlashBias		0xD01B
#define PTP_DPC_NIKON_WhiteBalanceCloudyBias		0xD01C
#define PTP_DPC_NIKON_WhiteBalanceShadeBias		0xD01D
#define PTP_DPC_NIKON_WhiteBalanceColorTemperature	0xD01E
#define PTP_DPC_NIKON_WhiteBalancePresetNo		0xD01F
#define PTP_DPC_NIKON_WhiteBalancePresetName0		0xD020
#define PTP_DPC_NIKON_WhiteBalancePresetName1		0xD021
#define PTP_DPC_NIKON_WhiteBalancePresetName2		0xD022
#define PTP_DPC_NIKON_WhiteBalancePresetName3		0xD023
#define PTP_DPC_NIKON_WhiteBalancePresetName4		0xD024
#define PTP_DPC_NIKON_WhiteBalancePresetVal0		0xD025
#define PTP_DPC_NIKON_WhiteBalancePresetVal1		0xD026
#define PTP_DPC_NIKON_WhiteBalancePresetVal2		0xD027
#define PTP_DPC_NIKON_WhiteBalancePresetVal3		0xD028
#define PTP_DPC_NIKON_WhiteBalancePresetVal4		0xD029
#define PTP_DPC_NIKON_ImageSharpening			0xD02A
#define PTP_DPC_NIKON_ToneCompensation			0xD02B
#define PTP_DPC_NIKON_ColorModel			0xD02C
#define PTP_DPC_NIKON_HueAdjustment			0xD02D
#define PTP_DPC_NIKON_NonCPULensDataFocalLength		0xD02E
#define PTP_DPC_NIKON_FmmManualSetting			0xD02E	/* on z6, same as PTP_DPC_NIKON_NonCPULensDataFocalLength? */
#define PTP_DPC_NIKON_NonCPULensDataMaximumAperture	0xD02F
#define PTP_DPC_NIKON_F0ManualSetting			0xD02F	/* on z6, same as PTP_DPC_NIKON_NonCPULensDataMaximumAperture? */
#define PTP_DPC_NIKON_ShootingMode			0xD030	/* also Capture Area Crop on Z6 */
#define PTP_DPC_NIKON_CaptureAreaCrop			0xD030	/* on z6, (DX = 1/FX = 0 / 1:1=2, 16:9=3)*/
#define PTP_DPC_NIKON_JPEG_Compression_Policy		0xD031
#define PTP_DPC_NIKON_ColorSpace			0xD032
#define PTP_DPC_NIKON_AutoDXCrop			0xD033
#define PTP_DPC_NIKON_FlickerReduction			0xD034
#define PTP_DPC_NIKON_RemoteMode			0xD035
#define PTP_DPC_NIKON_VideoMode				0xD036
#define PTP_DPC_NIKON_EffectMode			0xD037
#define PTP_DPC_NIKON_1_Mode				0xD038
#define PTP_DPC_NIKON_WhiteBalancePresetName5		0xD038	/* z6 */
#define PTP_DPC_NIKON_WhiteBalancePresetName6		0xD039	/* z6 */
#define PTP_DPC_NIKON_WhiteBalanceTunePreset5		0xD03A	/* z6 */
#define PTP_DPC_NIKON_WhiteBalanceTunePreset6		0xD03B	/* z6 */
#define PTP_DPC_NIKON_WhiteBalancePresetProtect5	0xD03C	/* z6 */
#define PTP_DPC_NIKON_WhiteBalancePresetProtect6	0xD03D	/* z6 */
#define PTP_DPC_NIKON_WhiteBalancePresetValue5		0xD03E	/* z6 */
#define PTP_DPC_NIKON_WhiteBalancePresetValue6		0xD03F	/* z6 */
#define PTP_DPC_NIKON_CSMMenuBankSelect			0xD040
#define PTP_DPC_NIKON_MenuBankNameA			0xD041
#define PTP_DPC_NIKON_MenuBankNameB			0xD042
#define PTP_DPC_NIKON_MenuBankNameC			0xD043
#define PTP_DPC_NIKON_MenuBankNameD			0xD044
#define PTP_DPC_NIKON_ResetBank				0xD045
#define PTP_DPC_NIKON_AFStillLockOnAcross		0xD046
#define PTP_DPC_NIKON_AFStillLockOnMove			0xD047
#define PTP_DPC_NIKON_A1AFCModePriority			0xD048
#define PTP_DPC_NIKON_A2AFSModePriority			0xD049
#define PTP_DPC_NIKON_A3GroupDynamicAF			0xD04A
#define PTP_DPC_NIKON_A4AFActivation			0xD04B
#define PTP_DPC_NIKON_FocusAreaIllumManualFocus		0xD04C
#define PTP_DPC_NIKON_FocusAreaIllumContinuous		0xD04D
#define PTP_DPC_NIKON_FocusAreaIllumWhenSelected 	0xD04E
#define PTP_DPC_NIKON_FocusAreaWrap			0xD04F /* area sel */
#define PTP_DPC_NIKON_FocusAreaSelect			0xD04F /* z6 */
#define PTP_DPC_NIKON_VerticalAFON			0xD050
#define PTP_DPC_NIKON_AFLockOn				0xD051
#define PTP_DPC_NIKON_FocusAreaZone			0xD052
#define PTP_DPC_NIKON_EnableCopyright			0xD053
#define PTP_DPC_NIKON_ISOAuto				0xD054
#define PTP_DPC_NIKON_EVISOStep				0xD055
#define PTP_DPC_NIKON_EVStep				0xD056 /* EV Step SS FN */
#define PTP_DPC_NIKON_EVStepExposureComp		0xD057
#define PTP_DPC_NIKON_ExposureCompensation		0xD058
#define PTP_DPC_NIKON_CenterWeightArea			0xD059	/* CenterweightedExRange */
#define PTP_DPC_NIKON_ExposureBaseMatrix		0xD05A
#define PTP_DPC_NIKON_ExposureBaseCenter		0xD05B
#define PTP_DPC_NIKON_ExposureBaseSpot			0xD05C
#define PTP_DPC_NIKON_LiveViewAFArea			0xD05D /* FIXME: AfAtLiveview? */
#define PTP_DPC_NIKON_AELockMode			0xD05E
#define PTP_DPC_NIKON_AELAFLMode			0xD05F
#define PTP_DPC_NIKON_LiveViewAFFocus			0xD061	/* AfModeAtLiveView */
#define PTP_DPC_NIKON_MeterOff				0xD062
#define PTP_DPC_NIKON_SelfTimer				0xD063
#define PTP_DPC_NIKON_MonitorOff			0xD064
#define PTP_DPC_NIKON_ImgConfTime			0xD065
#define PTP_DPC_NIKON_AutoOffTimers			0xD066
#define PTP_DPC_NIKON_AngleLevel			0xD067
#define PTP_DPC_NIKON_D1ShootingSpeed			0xD068 /* continuous speed low */
#define PTP_DPC_NIKON_D2MaximumShots			0xD069	/* BurstMaxNumer */
#define PTP_DPC_NIKON_ExposureDelayMode			0xD06A
#define PTP_DPC_NIKON_LongExposureNoiseReduction	0xD06B
#define PTP_DPC_NIKON_FileNumberSequence		0xD06C
#define PTP_DPC_NIKON_ControlPanelFinderRearControl	0xD06D
#define PTP_DPC_NIKON_ControlPanelFinderViewfinder	0xD06E
#define PTP_DPC_NIKON_D7Illumination			0xD06F
#define PTP_DPC_NIKON_NrHighISO				0xD070
#define PTP_DPC_NIKON_SHSET_CH_GUID_DISP		0xD071
#define PTP_DPC_NIKON_ArtistName			0xD072
#define PTP_DPC_NIKON_CopyrightInfo			0xD073
#define PTP_DPC_NIKON_FlashSyncSpeed			0xD074
#define PTP_DPC_NIKON_FlashShutterSpeed			0xD075	/* SB Low Limit */
#define PTP_DPC_NIKON_E3AAFlashMode			0xD076
#define PTP_DPC_NIKON_E4ModelingFlash			0xD077
#define PTP_DPC_NIKON_BracketSet			0xD078	/* Bracket Type? */
#define PTP_DPC_NIKON_E6ManualModeBracketing		0xD079	/* Bracket Factor? */
#define PTP_DPC_NIKON_BracketOrder			0xD07A
#define PTP_DPC_NIKON_E8AutoBracketSelection		0xD07B	/* Bracket Method? */
#define PTP_DPC_NIKON_BracketingSet			0xD07C
#define PTP_DPC_NIKON_AngleLevelPitching		0xD07D
#define PTP_DPC_NIKON_AngleLevelYawing			0xD07E
#define PTP_DPC_NIKON_ExtendShootingMenu		0xD07F
#define PTP_DPC_NIKON_F1CenterButtonShootingMode	0xD080
#define PTP_DPC_NIKON_CenterButtonPlaybackMode		0xD081
#define PTP_DPC_NIKON_F2Multiselector			0xD082
#define PTP_DPC_NIKON_F3PhotoInfoPlayback		0xD083	/* MultiSelector Dir */
#define PTP_DPC_NIKON_F4AssignFuncButton		0xD084  /* CMD Dial Rotate */
#define PTP_DPC_NIKON_F5CustomizeCommDials		0xD085  /* CMD Dial Change */
#define PTP_DPC_NIKON_ReverseCommandDial		0xD086  /* CMD Dial FN Set */
#define PTP_DPC_NIKON_ApertureSetting			0xD087  /* CMD Dial Active */
#define PTP_DPC_NIKON_MenusAndPlayback			0xD088  /* CMD Dial Active */
#define PTP_DPC_NIKON_F6ButtonsAndDials			0xD089  /* Universal Mode? */
#define PTP_DPC_NIKON_NoCFCard				0xD08A	/* Enable Shutter? */
#define PTP_DPC_NIKON_CenterButtonZoomRatio		0xD08B
#define PTP_DPC_NIKON_FunctionButton2			0xD08C
#define PTP_DPC_NIKON_AFAreaPoint			0xD08D
#define PTP_DPC_NIKON_NormalAFOn			0xD08E
#define PTP_DPC_NIKON_CleanImageSensor			0xD08F
#define PTP_DPC_NIKON_ImageCommentString		0xD090
#define PTP_DPC_NIKON_ImageCommentEnable		0xD091
#define PTP_DPC_NIKON_ImageRotation			0xD092
#define PTP_DPC_NIKON_ManualSetLensNo			0xD093
#define PTP_DPC_NIKON_RetractableLensWarning		0xD09C
#define PTP_DPC_NIKON_FaceDetection			0xD09D
#define PTP_DPC_NIKON_3DTrackingCaptureArea		0xD09E
#define PTP_DPC_NIKON_MatrixMetering			0xD09F
#define PTP_DPC_NIKON_MovScreenSize			0xD0A0
#define PTP_DPC_NIKON_MovVoice				0xD0A1
#define PTP_DPC_NIKON_MovMicrophone			0xD0A2
#define PTP_DPC_NIKON_MovFileSlot			0xD0A3
#define PTP_DPC_NIKON_MovRecProhibitCondition		0xD0A4
#define PTP_DPC_NIKON_ManualMovieSetting		0xD0A6
#define PTP_DPC_NIKON_MovQuality			0xD0A7
#define PTP_DPC_NIKON_MovRecordMicrophoneLevelValue	0xD0A8
#define PTP_DPC_NIKON_MovWindNoiseReduction		0xD0AA
#define PTP_DPC_NIKON_MovRecordingZone			0xD0AC
#define PTP_DPC_NIKON_MovISOAutoControl			0xD0AD
#define PTP_DPC_NIKON_MovISOAutoHighLimit		0xD0AE
#define PTP_DPC_NIKON_MovFileType			0xD0AF /* 0: mov, 1: mp4 */
#define PTP_DPC_NIKON_LiveViewScreenDisplaySetting	0xD0B2
#define PTP_DPC_NIKON_MonitorOffDelay			0xD0B3
#define PTP_DPC_NIKON_ExposureIndexEx			0xD0B4
#define PTP_DPC_NIKON_ISOControlSensitivity		0xD0B5
#define PTP_DPC_NIKON_RawImageSize			0xD0B6
#define PTP_DPC_NIKON_MultiBatteryInfo			0xD0B9
#define PTP_DPC_NIKON_FlickerReductionSetting		0xD0B7
#define PTP_DPC_NIKON_DiffractionCompensatipn		0xD0BA
#define PTP_DPC_NIKON_MovieLogOutput			0xD0BB
#define PTP_DPC_NIKON_MovieAutoDistortion		0xD0BC
#define PTP_DPC_NIKON_RemainingExposureTime		0xD0BE
#define PTP_DPC_NIKON_MovieLogSetting			0xD0BF
#define PTP_DPC_NIKON_Bracketing			0xD0C0
#define PTP_DPC_NIKON_AutoExposureBracketStep		0xD0C1
#define PTP_DPC_NIKON_AutoExposureBracketProgram	0xD0C2
#define PTP_DPC_NIKON_AutoExposureBracketCount		0xD0C3
#define PTP_DPC_NIKON_WhiteBalanceBracketStep		0xD0C4
#define PTP_DPC_NIKON_WhiteBalanceBracketProgram	0xD0C5
#define PTP_DPC_NIKON_ADLBracketingPattern		0xD0C6
#define PTP_DPC_NIKON_ADLBracketingStep			0xD0C7
#define PTP_DPC_NIKON_HDMIOutputDataDepth		0xD0CC
#define PTP_DPC_NIKON_LensID				0xD0E0
#define PTP_DPC_NIKON_LensSort				0xD0E1
#define PTP_DPC_NIKON_LensType				0xD0E2
#define PTP_DPC_NIKON_FocalLengthMin			0xD0E3
#define PTP_DPC_NIKON_FocalLengthMax			0xD0E4
#define PTP_DPC_NIKON_MaxApAtMinFocalLength		0xD0E5
#define PTP_DPC_NIKON_MaxApAtMaxFocalLength		0xD0E6
#define PTP_DPC_NIKON_LensTypeML			0xD0E7
#define PTP_DPC_NIKON_FinderISODisp			0xD0F0
#define PTP_DPC_NIKON_AutoOffPhoto			0xD0F2
#define PTP_DPC_NIKON_AutoOffMenu			0xD0F3
#define PTP_DPC_NIKON_AutoOffInfo			0xD0F4
#define PTP_DPC_NIKON_SelfTimerShootNum			0xD0F5
#define PTP_DPC_NIKON_VignetteCtrl			0xD0F7
#define PTP_DPC_NIKON_AutoDistortionControl		0xD0F8
#define PTP_DPC_NIKON_SceneMode				0xD0F9
#define PTP_DPC_NIKON_UserMode				0xD0FC
#define PTP_DPC_NIKON_SceneMode2			0xD0FD
#define PTP_DPC_NIKON_SelfTimerInterval			0xD0FE
#define PTP_DPC_NIKON_ExposureTime			0xD100	/* Shutter Speed */
#define PTP_DPC_NIKON_ACPower				0xD101
#define PTP_DPC_NIKON_WarningStatus			0xD102
#define PTP_DPC_NIKON_MaximumShots			0xD103 /* remain shots (in RAM buffer?) */
#define PTP_DPC_NIKON_AFLockStatus			0xD104
#define PTP_DPC_NIKON_AELockStatus			0xD105
#define PTP_DPC_NIKON_FVLockStatus			0xD106
#define PTP_DPC_NIKON_AutofocusLCDTopMode2		0xD107
#define PTP_DPC_NIKON_AutofocusArea			0xD108
#define PTP_DPC_NIKON_FlexibleProgram			0xD109
#define PTP_DPC_NIKON_LightMeter			0xD10A	/* Exposure Status */
#define PTP_DPC_NIKON_RecordingMedia			0xD10B	/* Card or SDRAM */
#define PTP_DPC_NIKON_USBSpeed				0xD10C
#define PTP_DPC_NIKON_CCDNumber				0xD10D
#define PTP_DPC_NIKON_CameraOrientation			0xD10E
#define PTP_DPC_NIKON_GroupPtnType			0xD10F
#define PTP_DPC_NIKON_FNumberLock			0xD110
#define PTP_DPC_NIKON_ExposureApertureLock		0xD111	/* shutterspeed lock*/
#define PTP_DPC_NIKON_TVLockSetting			0xD112
#define PTP_DPC_NIKON_AVLockSetting			0xD113
#define PTP_DPC_NIKON_IllumSetting			0xD114
#define PTP_DPC_NIKON_FocusPointBright			0xD115
#define PTP_DPC_NIKON_ExposureCompFlashUsed		0xD118
#define PTP_DPC_NIKON_ExternalFlashAttached		0xD120
#define PTP_DPC_NIKON_ExternalFlashStatus		0xD121
#define PTP_DPC_NIKON_ExternalFlashSort			0xD122
#define PTP_DPC_NIKON_ExternalFlashMode			0xD123
#define PTP_DPC_NIKON_ExternalFlashCompensation		0xD124
#define PTP_DPC_NIKON_NewExternalFlashMode		0xD125
#define PTP_DPC_NIKON_FlashExposureCompensation		0xD126
#define PTP_DPC_NIKON_ExternalFlashMultiFlashMode	0xD12D
#define PTP_DPC_NIKON_ConnectionPath			0xD12E
#define PTP_DPC_NIKON_HDRMode				0xD130
#define PTP_DPC_NIKON_HDRHighDynamic			0xD131
#define PTP_DPC_NIKON_HDRSmoothing			0xD132
#define PTP_DPC_NIKON_HDRSaveIndividualImages		0xD133
#define PTP_DPC_NIKON_VibrationReduction		0xD138
#define PTP_DPC_NIKON_OptimizeImage			0xD140
#define PTP_DPC_NIKON_WBAutoType			0xD141
#define PTP_DPC_NIKON_Saturation			0xD142
#define PTP_DPC_NIKON_BW_FillerEffect			0xD143
#define PTP_DPC_NIKON_BW_Sharpness			0xD144
#define PTP_DPC_NIKON_BW_Contrast			0xD145
#define PTP_DPC_NIKON_BW_Setting_Type			0xD146
#define PTP_DPC_NIKON_Slot2SaveMode			0xD148
#define PTP_DPC_NIKON_RawBitMode			0xD149
#define PTP_DPC_NIKON_ActiveDLighting			0xD14E /* was PTP_DPC_NIKON_ISOAutoTime */
#define PTP_DPC_NIKON_FlourescentType			0xD14F
#define PTP_DPC_NIKON_TuneColourTemperature		0xD150
#define PTP_DPC_NIKON_TunePreset0			0xD151
#define PTP_DPC_NIKON_TunePreset1			0xD152
#define PTP_DPC_NIKON_TunePreset2			0xD153
#define PTP_DPC_NIKON_TunePreset3			0xD154
#define PTP_DPC_NIKON_TunePreset4			0xD155
#define PTP_DPC_NIKON_PrimarySlot			0xD156
#define PTP_DPC_NIKON_WBPresetProtect1			0xD158
#define PTP_DPC_NIKON_WBPresetProtect2			0xD159
#define PTP_DPC_NIKON_WBPresetProtect3			0xD15A
#define PTP_DPC_NIKON_ActiveFolder			0xD15B
#define PTP_DPC_NIKON_WBPresetProtect4			0xD15C
#define PTP_DPC_NIKON_WhiteBalanceReset			0xD15D
#define PTP_DPC_NIKON_WhiteBalanceNaturalLightAutoBias	0xD15E /* Only encountered in D850, z6? */
#define PTP_DPC_NIKON_BeepOff				0xD160
#define PTP_DPC_NIKON_AutofocusMode			0xD161
#define PTP_DPC_NIKON_AFAssist				0xD163
#define PTP_DPC_NIKON_PADVPMode				0xD164	/* iso auto time */
#define PTP_DPC_NIKON_ISOAutoShutterTime		0xD164	/* z6 */
#define PTP_DPC_NIKON_ImageReview			0xD165
#define PTP_DPC_NIKON_AFAreaIllumination		0xD166
#define PTP_DPC_NIKON_FlashMode				0xD167
#define PTP_DPC_NIKON_FlashCommanderMode		0xD168
#define PTP_DPC_NIKON_FlashSign				0xD169
#define PTP_DPC_NIKON_ISO_Auto				0xD16A
#define PTP_DPC_NIKON_RemoteTimeout			0xD16B
#define PTP_DPC_NIKON_GridDisplay			0xD16C
#define PTP_DPC_NIKON_FlashModeManualPower		0xD16D
#define PTP_DPC_NIKON_FlashModeCommanderPower		0xD16E
#define PTP_DPC_NIKON_AutoFP				0xD16F
#define PTP_DPC_NIKON_DateImprintSetting		0xD170
#define PTP_DPC_NIKON_DateCounterSelect			0xD171
#define PTP_DPC_NIKON_DateCountData			0xD172
#define PTP_DPC_NIKON_DateCountDisplaySetting		0xD173
#define PTP_DPC_NIKON_RangeFinderSetting		0xD174
#define PTP_DPC_NIKON_LimitedAFAreaMode			0xD176
#define PTP_DPC_NIKON_AFModeRestrictions		0xD177
#define PTP_DPC_NIKON_LowLightAF			0xD17A
#define PTP_DPC_NIKON_ApplyLiveViewSetting		0xD17B
#define PTP_DPC_NIKON_MovieAfSpeed			0xD17C
#define PTP_DPC_NIKON_MovieAfSpeedWhenToApply		0xD17D
#define PTP_DPC_NIKON_MovieAfTrackingSensitivity	0xD17E
#define PTP_DPC_NIKON_CSMMenu				0xD180
#define PTP_DPC_NIKON_WarningDisplay			0xD181
#define PTP_DPC_NIKON_BatteryCellKind			0xD182
#define PTP_DPC_NIKON_ISOAutoHiLimit			0xD183
#define PTP_DPC_NIKON_DynamicAFArea			0xD184
#define PTP_DPC_NIKON_ContinuousSpeedHigh		0xD186
#define PTP_DPC_NIKON_InfoDispSetting			0xD187
#define PTP_DPC_NIKON_PreviewButton			0xD189
#define PTP_DPC_NIKON_PreviewButton2			0xD18A
#define PTP_DPC_NIKON_AEAFLockButton2			0xD18B
#define PTP_DPC_NIKON_IndicatorDisp			0xD18D
#define PTP_DPC_NIKON_CellKindPriority			0xD18E
#define PTP_DPC_NIKON_BracketingFramesAndSteps		0xD190
#define PTP_DPC_NIKON_MovieReleaseButton		0xD197
#define PTP_DPC_NIKON_FlashISOAutoHighLimit		0xD199
#define PTP_DPC_NIKON_LiveViewMode			0xD1A0
#define PTP_DPC_NIKON_LiveViewDriveMode			0xD1A1
#define PTP_DPC_NIKON_LiveViewStatus			0xD1A2
#define PTP_DPC_NIKON_LiveViewImageZoomRatio		0xD1A3
#define PTP_DPC_NIKON_LiveViewProhibitCondition		0xD1A4
#define PTP_DPC_NIKON_LiveViewExposurePreview		0xD1A5
#define PTP_DPC_NIKON_LiveViewSelector			0xD1A6
#define PTP_DPC_NIKON_LiveViewWhiteBalance		0xD1A7
#define PTP_DPC_NIKON_MovieShutterSpeed			0xD1A8
#define PTP_DPC_NIKON_MovieFNumber			0xD1A9
#define PTP_DPC_NIKON_MovieISO				0xD1AA
#define PTP_DPC_NIKON_MovieExposureBiasCompensation	0xD1AB
#define PTP_DPC_NIKON_LiveViewMovieMode			0xD1AC /* ? */
#define PTP_DPC_NIKON_LiveViewImageSize			0xD1AC /* d850 */
#define PTP_DPC_NIKON_LiveViewPhotography		0xD1AD
#define PTP_DPC_NIKON_MovieExposureMeteringMode		0xD1AF
#define PTP_DPC_NIKON_ExposureDisplayStatus		0xD1B0
#define PTP_DPC_NIKON_ExposureIndicateStatus		0xD1B1
#define PTP_DPC_NIKON_InfoDispErrStatus			0xD1B2
#define PTP_DPC_NIKON_ExposureIndicateLightup		0xD1B3
#define PTP_DPC_NIKON_ContinousShootingCount		0xD1B4
#define PTP_DPC_NIKON_MovieRecFrameCount		0xD1B7
#define PTP_DPC_NIKON_CameraLiveViewStatus		0xD1B8
#define PTP_DPC_NIKON_DetectionPeaking			0xD1B9
#define PTP_DPC_NIKON_LiveViewTFTStatus			0xD1BA
#define PTP_DPC_NIKON_LiveViewImageStatus		0xD1BB
#define PTP_DPC_NIKON_LiveViewImageCompression		0xD1BC
#define PTP_DPC_NIKON_LiveViewZoomArea			0xD1BD
#define PTP_DPC_NIKON_FlashOpen				0xD1C0
#define PTP_DPC_NIKON_FlashCharged			0xD1C1
#define PTP_DPC_NIKON_FlashMRepeatValue			0xD1D0
#define PTP_DPC_NIKON_FlashMRepeatCount			0xD1D1
#define PTP_DPC_NIKON_FlashMRepeatInterval		0xD1D2
#define PTP_DPC_NIKON_FlashCommandChannel		0xD1D3
#define PTP_DPC_NIKON_FlashCommandSelfMode		0xD1D4
#define PTP_DPC_NIKON_FlashCommandSelfCompensation	0xD1D5
#define PTP_DPC_NIKON_FlashCommandSelfValue		0xD1D6
#define PTP_DPC_NIKON_FlashCommandAMode			0xD1D7
#define PTP_DPC_NIKON_FlashCommandACompensation		0xD1D8
#define PTP_DPC_NIKON_FlashCommandAValue		0xD1D9
#define PTP_DPC_NIKON_FlashCommandBMode			0xD1DA
#define PTP_DPC_NIKON_FlashCommandBCompensation		0xD1DB
#define PTP_DPC_NIKON_FlashCommandBValue		0xD1DC
#define PTP_DPC_NIKON_ExternalRecordingControl		0xD1DE
#define PTP_DPC_NIKON_HighlightBrightness		0xD1DF
#define PTP_DPC_NIKON_SBWirelessMode			0xD1E2
#define PTP_DPC_NIKON_SBWirelessMultipleFlashMode	0xD1E3
#define PTP_DPC_NIKON_SBUsableGroup			0xD1E4
#define PTP_DPC_NIKON_WirelessCLSEntryMode		0xD1E5
#define PTP_DPC_NIKON_SBPINCode				0xD1E6
#define PTP_DPC_NIKON_RadioMultipleFlashChannel		0xD1E7
#define PTP_DPC_NIKON_OpticalMultipleFlashChannel	0xD1E8
#define PTP_DPC_NIKON_FlashRangeDisplay			0xD1E9
#define PTP_DPC_NIKON_AllTestFiringDisable		0xD1EA
#define PTP_DPC_NIKON_SBSettingMemberLock		0xD1EC
#define PTP_DPC_NIKON_SBIntegrationFlashReady		0xD1ED
#define PTP_DPC_NIKON_ApplicationMode			0xD1F0
#define PTP_DPC_NIKON_ExposureRemaining			0xD1F1
#define PTP_DPC_NIKON_ActiveSlot			0xD1F2
#define PTP_DPC_NIKON_ISOAutoShutterCorrectionTime	0xD1F4
#define PTP_DPC_NIKON_MirrorUpStatus			0xD1F6
#define PTP_DPC_NIKON_MirrorUpReleaseShootingCount	0xD1F7
#define PTP_DPC_NIKON_MovieAfAreaMode			0xD1F8
#define PTP_DPC_NIKON_MovieVibrationReduction		0xD1F9
#define PTP_DPC_NIKON_MovieFocusMode			0xD1FA
#define PTP_DPC_NIKON_RecordTimeCodes			0xD1FB
#define PTP_DPC_NIKON_CountUpMethod			0xD1FC
#define PTP_DPC_NIKON_TimeCodeOrigin			0xD1FD
#define PTP_DPC_NIKON_DropFrame				0xD1FE
#define PTP_DPC_NIKON_ActivePicCtrlItem			0xD200
#define PTP_DPC_NIKON_ChangePicCtrlItem			0xD201
#define PTP_DPC_NIKON_ElectronicFrontCurtainShutter	0xD20D
#define PTP_DPC_NIKON_MovieResetShootingMenu		0xD20E
#define PTP_DPC_NIKON_MovieCaptureAreaCrop		0xD20F
#define PTP_DPC_NIKON_MovieAutoDxCrop			0xD210
#define PTP_DPC_NIKON_MovieWbAutoType			0xD211
#define PTP_DPC_NIKON_MovieWbTuneAuto			0xD212
#define PTP_DPC_NIKON_MovieWbTuneIncandescent		0xD213
#define PTP_DPC_NIKON_MovieWbFlourescentType		0xD214
#define PTP_DPC_NIKON_MovieWbTuneFlourescent		0xD215
#define PTP_DPC_NIKON_MovieWbTuneSunny			0xD216
#define PTP_DPC_NIKON_MovieWbTuneCloudy			0xD218
#define PTP_DPC_NIKON_MovieWbTuneShade			0xD219
#define PTP_DPC_NIKON_MovieWbColorTemp			0xD21A
#define PTP_DPC_NIKON_MovieWbTuneColorTemp		0xD21B
#define PTP_DPC_NIKON_MovieWbPresetData0		0xD21C
#define PTP_DPC_NIKON_MovieWbPresetDataComment1		0xD21D
#define PTP_DPC_NIKON_MovieWbPresetDataComment2		0xD21E
#define PTP_DPC_NIKON_MovieWbPresetDataComment3		0xD21F
#define PTP_DPC_NIKON_MovieWbPresetDataComment4		0xD220
#define PTP_DPC_NIKON_MovieWbPresetDataComment5		0xD221
#define PTP_DPC_NIKON_MovieWbPresetDataComment6		0xD222
#define PTP_DPC_NIKON_MovieWbPresetDataValue1		0xD223
#define PTP_DPC_NIKON_MovieWbPresetDataValue2		0xD224
#define PTP_DPC_NIKON_MovieWbPresetDataValue3		0xD225
#define PTP_DPC_NIKON_MovieWbPresetDataValue4		0xD226
#define PTP_DPC_NIKON_MovieWbPresetDataValue5		0xD227
#define PTP_DPC_NIKON_MovieWbPresetDataValue6		0xD228
#define PTP_DPC_NIKON_MovieWbTunePreset1		0xD229
#define PTP_DPC_NIKON_MovieWbTunePreset2		0xD22A
#define PTP_DPC_NIKON_MovieWbTunePreset3		0xD22B
#define PTP_DPC_NIKON_MovieWbTunePreset4		0xD22C
#define PTP_DPC_NIKON_MovieWbTunePreset5		0xD22D
#define PTP_DPC_NIKON_MovieWbTunePreset6		0xD22E
#define PTP_DPC_NIKON_MovieWbPresetProtect1		0xD22F
#define PTP_DPC_NIKON_MovieWbPresetProtect2		0xD230
#define PTP_DPC_NIKON_MovieWbPresetProtect3		0xD231
#define PTP_DPC_NIKON_MovieWbPresetProtect4		0xD232
#define PTP_DPC_NIKON_MovieWbPresetProtect5		0xD233
#define PTP_DPC_NIKON_MovieWbPresetProtect6		0xD234
#define PTP_DPC_NIKON_MovieWhiteBalanceReset		0xD235
#define PTP_DPC_NIKON_MovieNrHighISO			0xD236
#define PTP_DPC_NIKON_MovieActivePicCtrlItem		0xD237
#define PTP_DPC_NIKON_MovieChangePicCtrlItem		0xD238
#define PTP_DPC_NIKON_ExposureBaseCompHighlight		0xD239
#define PTP_DPC_NIKON_MovieWhiteBalance			0xD23A
#define PTP_DPC_NIKON_MovieActiveDLighting		0xD23B
#define PTP_DPC_NIKON_MovieWbTuneNatural		0xD23C
#define PTP_DPC_NIKON_MovieAttenuator			0xD23D
#define PTP_DPC_NIKON_MovieVignetteControl		0xD23E
#define PTP_DPC_NIKON_MovieDiffractionCompensation	0xD23F
#define PTP_DPC_NIKON_UseDeviceStageFlag		0xD303
#define PTP_DPC_NIKON_MovieCaptureMode			0xD304
#define PTP_DPC_NIKON_SlowMotionMovieRecordScreenSize	0xD305
#define PTP_DPC_NIKON_HighSpeedStillCaptureRate		0xD306
#define PTP_DPC_NIKON_BestMomentCaptureMode		0xD307
#define PTP_DPC_NIKON_ActiveSelectionFrameSavedDefault	0xD308
#define PTP_DPC_NIKON_ActiveSelectionCapture40frameOver	0xD309
#define PTP_DPC_NIKON_ActiveSelectionOnReleaseRecord	0xD310
#define PTP_DPC_NIKON_ActiveSelectionSelectedPictures	0xD311
#define PTP_DPC_NIKON_ExposureRemainingInMovie		0xD312
#define PTP_DPC_NIKON_OpticalVR				0xD313
#define PTP_DPC_NIKON_ElectronicVR			0xD314
#define PTP_DPC_NIKON_SilentPhotography			0xD315
#define PTP_DPC_NIKON_FacePriority			0xD316
#define PTP_DPC_NIKON_LensTypeNikon1			0xD317
#define PTP_DPC_NIKON_ISONoiseReduction			0xD318
#define PTP_DPC_NIKON_MovieLoopLength			0xD323


/* Nikon V1 (or WU adapter?) Trace */
/* d241 - gets string "Nikon_WU2_0090B5123C61" */
#define PTP_DPC_NIKON_D241				0xD241
/* d244 - gets a single byte 0x00 */
#define PTP_DPC_NIKON_D244				0xD244
/* d247 - gets 3 bytes 0x01 0x00 0x00 */
#define PTP_DPC_NIKON_D247				0xD247
/* S9700 */
#define PTP_DPC_NIKON_GUID				0xD24F
/* d250 - gets a string "0000123C61" */
#define PTP_DPC_NIKON_D250				0xD250
/* d251 - gets a 0x0100000d */
#define PTP_DPC_NIKON_D251				0xD251

/* this is irregular, as it should be -0x5000 or 0xD000 based */
#define PTP_DPC_NIKON_1_ISO				0xF002
#define PTP_DPC_NIKON_1_FNumber				0xF003
#define PTP_DPC_NIKON_1_ShutterSpeed			0xF004
#define PTP_DPC_NIKON_1_FNumber2			0xF006
#define PTP_DPC_NIKON_1_ShutterSpeed2			0xF007
#define PTP_DPC_NIKON_1_ImageCompression		0xF009
#define PTP_DPC_NIKON_1_ImageSize			0xF00A
#define PTP_DPC_NIKON_1_WhiteBalance			0xF00C
#define PTP_DPC_NIKON_1_LongExposureNoiseReduction	0xF00D
#define PTP_DPC_NIKON_1_HiISONoiseReduction		0xF00E
#define PTP_DPC_NIKON_1_ActiveDLighting			0xF00F
#define PTP_DPC_NIKON_1_Language			0xF018
#define PTP_DPC_NIKON_1_ReleaseWithoutCard		0xF019 /* no sd card */
#define PTP_DPC_NIKON_1_MovQuality			0xF01C

/* Fuji specific */

#define PTP_DPC_FUJI_FilmSimulation			0xD001
#define PTP_DPC_FUJI_FilmSimulationTune			0xD002
#define PTP_DPC_FUJI_DRangeMode				0xD007
#define PTP_DPC_FUJI_ColorMode				0xD008
#define PTP_DPC_FUJI_ColorSpace				0xD00A
#define PTP_DPC_FUJI_WhitebalanceTune1			0xD00B
#define PTP_DPC_FUJI_WhitebalanceTune2			0xD00C
#define PTP_DPC_FUJI_ColorTemperature			0xD017
#define PTP_DPC_FUJI_Quality				0xD018
#define PTP_DPC_FUJI_RecMode				0xD019 /* LiveViewColorMode? */
#define PTP_DPC_FUJI_LiveViewBrightness			0xD01A
#define PTP_DPC_FUJI_ThroughImageZoom			0xD01B
#define PTP_DPC_FUJI_NoiseReduction			0xD01C
#define PTP_DPC_FUJI_MacroMode				0xD01D
#define PTP_DPC_FUJI_LiveViewStyle			0xD01E
#define PTP_DPC_FUJI_FaceDetectionMode			0xD020
#define PTP_DPC_FUJI_RedEyeCorrectionMode		0xD021
#define PTP_DPC_FUJI_RawCompression			0xD022
#define PTP_DPC_FUJI_GrainEffect			0xD023
#define PTP_DPC_FUJI_SetEyeAFMode			0xD024
#define PTP_DPC_FUJI_FocusPoints			0xD025
#define PTP_DPC_FUJI_MFAssistMode			0xD026
#define PTP_DPC_FUJI_InterlockAEAFArea			0xD027
#define PTP_DPC_FUJI_CommandDialMode			0xD028
#define PTP_DPC_FUJI_Shadowing				0xD029
/* d02a - d02c also appear in setafmode */
#define PTP_DPC_FUJI_ExposureIndex			0xD02A
#define PTP_DPC_FUJI_MovieISO				0xD02B
#define PTP_DPC_FUJI_WideDynamicRange			0xD02E
#define PTP_DPC_FUJI_TNumber				0xD02F
#define PTP_DPC_FUJI_Comment				0xD100
#define PTP_DPC_FUJI_SerialMode				0xD101
#define PTP_DPC_FUJI_ExposureDelay			0xD102
#define PTP_DPC_FUJI_PreviewTime			0xD103
#define PTP_DPC_FUJI_BlackImageTone			0xD104
#define PTP_DPC_FUJI_Illumination			0xD105
#define PTP_DPC_FUJI_FrameGuideMode			0xD106
#define PTP_DPC_FUJI_ViewfinderWarning			0xD107
#define PTP_DPC_FUJI_AutoImageRotation			0xD108
#define PTP_DPC_FUJI_DetectImageRotation		0xD109
#define PTP_DPC_FUJI_ShutterPriorityMode1		0xD10A
#define PTP_DPC_FUJI_ShutterPriorityMode2		0xD10B
#define PTP_DPC_FUJI_AFIlluminator			0xD112
#define PTP_DPC_FUJI_Beep				0xD113
#define PTP_DPC_FUJI_AELock				0xD114
#define PTP_DPC_FUJI_ISOAutoSetting1			0xD115
#define PTP_DPC_FUJI_ISOAutoSetting2			0xD116
#define PTP_DPC_FUJI_ISOAutoSetting3			0xD117
#define PTP_DPC_FUJI_ExposureStep			0xD118
#define PTP_DPC_FUJI_CompensationStep			0xD119
#define PTP_DPC_FUJI_ExposureSimpleSet			0xD11A
#define PTP_DPC_FUJI_CenterPhotometryRange		0xD11B
#define PTP_DPC_FUJI_PhotometryLevel1			0xD11C
#define PTP_DPC_FUJI_PhotometryLevel2			0xD11D
#define PTP_DPC_FUJI_PhotometryLevel3			0xD11E
#define PTP_DPC_FUJI_FlashTuneSpeed			0xD11F
#define PTP_DPC_FUJI_FlashShutterLimit			0xD120
#define PTP_DPC_FUJI_BuiltinFlashMode			0xD121
#define PTP_DPC_FUJI_FlashManualMode			0xD122
#define PTP_DPC_FUJI_FlashRepeatingMode1		0xD123
#define PTP_DPC_FUJI_FlashRepeatingMode2		0xD124
#define PTP_DPC_FUJI_FlashRepeatingMode3		0xD125
#define PTP_DPC_FUJI_FlashCommanderMode1		0xD126
#define PTP_DPC_FUJI_FlashCommanderMode2		0xD127
#define PTP_DPC_FUJI_FlashCommanderMode3		0xD128
#define PTP_DPC_FUJI_FlashCommanderMode4		0xD129
#define PTP_DPC_FUJI_FlashCommanderMode5		0xD12A
#define PTP_DPC_FUJI_FlashCommanderMode6		0xD12B
#define PTP_DPC_FUJI_FlashCommanderMode7		0xD12C
#define PTP_DPC_FUJI_ModelingFlash			0xD12D
#define PTP_DPC_FUJI_BKT				0xD12E
#define PTP_DPC_FUJI_BKTChange				0xD12F
#define PTP_DPC_FUJI_BKTOrder				0xD130
#define PTP_DPC_FUJI_BKTSelection			0xD131
#define PTP_DPC_FUJI_AEAFLockButton			0xD132
#define PTP_DPC_FUJI_CenterButton			0xD133
#define PTP_DPC_FUJI_MultiSelectorButton		0xD134
#define PTP_DPC_FUJI_FunctionLock			0xD136
#define PTP_DPC_FUJI_Password				0xD145
#define PTP_DPC_FUJI_ChangePassword			0xD146	/* ? */
#define PTP_DPC_FUJI_CommandDialSetting1		0xD147
#define PTP_DPC_FUJI_CommandDialSetting2		0xD148
#define PTP_DPC_FUJI_CommandDialSetting3		0xD149
#define PTP_DPC_FUJI_CommandDialSetting4		0xD14A
#define PTP_DPC_FUJI_ButtonsAndDials			0xD14B
#define PTP_DPC_FUJI_NonCPULensData			0xD14C
#define PTP_DPC_FUJI_MBD200Batteries			0xD14E
#define PTP_DPC_FUJI_AFOnForMBD200Batteries		0xD14F
#define PTP_DPC_FUJI_FirmwareVersion			0xD153
#define PTP_DPC_FUJI_ShotCount				0xD154
#define PTP_DPC_FUJI_ShutterExchangeCount		0xD155
#define PTP_DPC_FUJI_WorldClock				0xD157
#define PTP_DPC_FUJI_TimeDifference1			0xD158
#define PTP_DPC_FUJI_TimeDifference2			0xD159
#define PTP_DPC_FUJI_Language				0xD15A
#define PTP_DPC_FUJI_FrameNumberSequence		0xD15B
#define PTP_DPC_FUJI_VideoMode				0xD15C
#define PTP_DPC_FUJI_SetUSBMode				0xD15D
#define PTP_DPC_FUJI_CommentWriteSetting		0xD161
#define PTP_DPC_FUJI_BCRAppendDelimiter			0xD162
#define PTP_DPC_FUJI_CommentEx				0xD167
#define PTP_DPC_FUJI_VideoOutOnOff			0xD168
#define PTP_DPC_FUJI_CropMode				0xD16F
#define PTP_DPC_FUJI_LensZoomPos			0xD170
#define PTP_DPC_FUJI_FocusPosition			0xD171
#define PTP_DPC_FUJI_LiveViewImageQuality		0xD173
#define PTP_DPC_FUJI_LiveViewImageSize			0xD174
#define PTP_DPC_FUJI_LiveViewCondition			0xD175
#define PTP_DPC_FUJI_StandbyMode			0xD176
#define PTP_DPC_FUJI_LiveViewExposure			0xD177
#define PTP_DPC_FUJI_LiveViewWhiteBalance		0xD178 /* same values as 0x5005 */
#define PTP_DPC_FUJI_LiveViewWhiteBalanceGain		0xD179
#define PTP_DPC_FUJI_LiveViewTuning			0xD17A
#define PTP_DPC_FUJI_FocusMeteringMode			0xD17C
#define PTP_DPC_FUJI_FocusLength			0xD17D
#define PTP_DPC_FUJI_CropAreaFrameInfo			0xD17E
#define PTP_DPC_FUJI_ResetSetting			0xD17F /* also clean sensor? */
#define PTP_DPC_FUJI_IOPCode				0xD184
#define PTP_DPC_FUJI_TetherRawConditionCode		0xD186
#define PTP_DPC_FUJI_TetherRawCompatibilityCode		0xD187
#define PTP_DPC_FUJI_LightTune				0xD200
#define PTP_DPC_FUJI_ReleaseMode			0xD201
#define PTP_DPC_FUJI_BKTFrame1				0xD202
#define PTP_DPC_FUJI_BKTFrame2				0xD203
#define PTP_DPC_FUJI_BKTStep				0xD204
#define PTP_DPC_FUJI_ProgramShift			0xD205
#define PTP_DPC_FUJI_FocusAreas				0xD206
#define PTP_DPC_FUJI_PriorityMode			0xD207 /* from setprioritymode */
/* D208 is some kind of control, likely bitmasked. reported like an enum.
 * 0x200 seems to mean focusing?
 * 0x208 capture?
 * camera starts with 0x304
 * xt2:    0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1,0x9300,5
 * xt3:    0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1,0x9200,0x40,0x9300,5,0x804,0x80
 * xt30:   0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1,0x9200,0x40,0x9300,5
 * xt4:    0x104,0x200,0x4,0x304,0x500,0xc,0x8000,0xa000,6,0x9000,2,0x9100,1,0x9300,5,0xe,0x9200,0x40,0x804,0x80
 * xh1:    0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1,0x9300,5
 * gfx100: 0x104,0x200,0x4,0x304,0x500,0xc,0x8000,0xa000,6,0x9000,2,0x9100,1,0x9300,5,0xe,0x9200
 * gfx50r: 0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1,0x9300,5,0xe
 * xpro2:  0x104,0x200,0x4,0x304,0x500,0xc,0xa000,6,0x9000,2,0x9100,1
 *
 * 0x304 is for regular capture 	SDK_ShootS2toS0	(default) (SDK_Shoot)
 * 0x200 seems for autofocus (s1?)	SDK_ShootS1
 * 0x500 start bulb? 0xc end bulb?	SDK_StartBulb
 * 0x400 might also be start bulb?	SDK_StartBulb
 * 0xc					SDK_EndBulb
 * 0x600 				SDK_1PushAF
 * 0x4 					SDK_CancelS1
 * 0x300 				SDK_ShootS2
 * 0x8000 migh be autowhitebalance
 */
#define PTP_DPC_FUJI_AFStatus				0xD209
#define PTP_DPC_FUJI_DeviceName				0xD20B
#define PTP_DPC_FUJI_MediaRecord			0xD20C /* from capmediarecord */
#define PTP_DPC_FUJI_MediaCapacity			0xD20D
#define PTP_DPC_FUJI_FreeSDRAMImages			0xD20E /* free images in SDRAM */
#define PTP_DPC_FUJI_MediaStatus			0xD211
#define PTP_DPC_FUJI_CurrentState			0xD212
#define PTP_DPC_FUJI_AELock2				0xD213
#define PTP_DPC_FUJI_Copyright				0xD215
#define PTP_DPC_FUJI_Copyright2				0xD216
#define PTP_DPC_FUJI_Aperture				0xD218
#define PTP_DPC_FUJI_ShutterSpeed			0xD219
#define PTP_DPC_FUJI_DeviceError			0xD21B
#define PTP_DPC_FUJI_SensitivityFineTune1		0xD222
#define PTP_DPC_FUJI_SensitivityFineTune2		0xD223
#define PTP_DPC_FUJI_CaptureRemaining			0xD229	/* Movie AF Mode? */
#define PTP_DPC_FUJI_MovieRemainingTime			0xD22A	/* Movie Focus Area? */
#define PTP_DPC_FUJI_ForceMode				0xD230
#define PTP_DPC_FUJI_ShutterSpeed2			0xD240 /* Movie Aperture */
#define PTP_DPC_FUJI_ImageAspectRatio			0xD241
#define PTP_DPC_FUJI_BatteryLevel			0xD242 /* Movie Sensitivity???? */
#define PTP_DPC_FUJI_TotalShotCount			0xD310
#define PTP_DPC_FUJI_HighLightTone			0xD320
#define PTP_DPC_FUJI_ShadowTone				0xD321
#define PTP_DPC_FUJI_LongExposureNR			0xD322
#define PTP_DPC_FUJI_FullTimeManualFocus		0xD323
#define PTP_DPC_FUJI_ISODialHn1				0xD332
#define PTP_DPC_FUJI_ISODialHn2				0xD333
#define PTP_DPC_FUJI_ViewMode1				0xD33F
#define PTP_DPC_FUJI_ViewMode2				0xD340
#define PTP_DPC_FUJI_DispInfoMode			0xD343
#define PTP_DPC_FUJI_LensISSwitch			0xD346
#define PTP_DPC_FUJI_FocusPoint				0xD347
#define PTP_DPC_FUJI_InstantAFMode			0xD34A
#define PTP_DPC_FUJI_PreAFMode				0xD34B
#define PTP_DPC_FUJI_CustomSetting			0xD34C
#define PTP_DPC_FUJI_LMOMode				0xD34D
#define PTP_DPC_FUJI_LockButtonMode			0xD34E
#define PTP_DPC_FUJI_AFLockMode				0xD34F
#define PTP_DPC_FUJI_MicJackMode			0xD350
#define PTP_DPC_FUJI_ISMode				0xD351
#define PTP_DPC_FUJI_DateTimeDispFormat			0xD352
#define PTP_DPC_FUJI_AeAfLockKeyAssign			0xD353
#define PTP_DPC_FUJI_CrossKeyAssign			0xD354
#define PTP_DPC_FUJI_SilentMode				0xD355
#define PTP_DPC_FUJI_PBSound				0xD356
#define PTP_DPC_FUJI_EVFDispAutoRotate			0xD358
#define PTP_DPC_FUJI_ExposurePreview			0xD359
#define PTP_DPC_FUJI_DispBrightness1			0xD35A
#define PTP_DPC_FUJI_DispBrightness2			0xD35B
#define PTP_DPC_FUJI_DispChroma1			0xD35C
#define PTP_DPC_FUJI_DispChroma2			0xD35D
#define PTP_DPC_FUJI_FocusCheckMode			0xD35E
#define PTP_DPC_FUJI_FocusScaleUnit			0xD35F
#define PTP_DPC_FUJI_SetFunctionButton			0xD361
#define PTP_DPC_FUJI_SensorCleanTiming			0xD363
#define PTP_DPC_FUJI_CustomAutoPowerOff			0xD364
#define PTP_DPC_FUJI_FileNamePrefix1			0xD365
#define PTP_DPC_FUJI_FileNamePrefix2			0xD366
#define PTP_DPC_FUJI_BatteryInfo1			0xD36A
#define PTP_DPC_FUJI_BatteryInfo2			0xD36B
#define PTP_DPC_FUJI_LensNameAndSerial			0xD36D
#define PTP_DPC_FUJI_CustomDispInfo			0xD36E
#define PTP_DPC_FUJI_FunctionLockCategory1		0xD36F
#define PTP_DPC_FUJI_FunctionLockCategory2		0xD370
#define PTP_DPC_FUJI_CustomPreviewTime			0xD371
#define PTP_DPC_FUJI_FocusArea1				0xD372
#define PTP_DPC_FUJI_FocusArea2				0xD373
#define PTP_DPC_FUJI_FocusArea3				0xD374
#define PTP_DPC_FUJI_FrameGuideGridInfo1		0xD375
#define PTP_DPC_FUJI_FrameGuideGridInfo2		0xD376
#define PTP_DPC_FUJI_FrameGuideGridInfo3		0xD377
#define PTP_DPC_FUJI_FrameGuideGridInfo4		0xD378
#define PTP_DPC_FUJI_LensUnknownData			0xD38A
#define PTP_DPC_FUJI_LensZoomPosCaps			0xD38C
#define PTP_DPC_FUJI_LensFNumberList			0xD38D
#define PTP_DPC_FUJI_LensFocalLengthList		0xD38E
#define PTP_DPC_FUJI_FocusLimiter			0xD390
#define PTP_DPC_FUJI_FocusArea4				0xD395
#define PTP_DPC_FUJI_InitSequence			0xDF01
#define PTP_DPC_FUJI_AppVersion				0xDF24

/* Microsoft/MTP specific */
#define PTP_DPC_MTP_SecureTime				0xD101
#define PTP_DPC_MTP_DeviceCertificate			0xD102
#define PTP_DPC_MTP_RevocationInfo			0xD103
#define PTP_DPC_MTP_SynchronizationPartner		0xD401
#define PTP_DPC_MTP_DeviceFriendlyName			0xD402
#define PTP_DPC_MTP_VolumeLevel				0xD403
#define PTP_DPC_MTP_DeviceIcon				0xD405
#define PTP_DPC_MTP_SessionInitiatorInfo		0xD406
#define PTP_DPC_MTP_PerceivedDeviceType			0xD407
#define PTP_DPC_MTP_PlaybackRate                        0xD410
#define PTP_DPC_MTP_PlaybackObject                      0xD411
#define PTP_DPC_MTP_PlaybackContainerIndex              0xD412
#define PTP_DPC_MTP_PlaybackPosition                    0xD413
#define PTP_DPC_MTP_PlaysForSureID                      0xD131

/* Zune specific property codes */
#define PTP_DPC_MTP_Zune_UnknownVersion			0xD181

/* Olympus */
/* these are from OMD E-M1 Mark 2 */
#define PTP_DPC_OLYMPUS_Aperture			0xD002
#define PTP_DPC_OLYMPUS_FocusMode			0xD003
#define PTP_DPC_OLYMPUS_ExposureMeteringMode		0xD004
#define PTP_DPC_OLYMPUS_ISO				0xD007
#define PTP_DPC_OLYMPUS_ExposureCompensation		0xD008
#define PTP_DPC_OLYMPUS_OMD_DriveMode			0xD009
#define PTP_DPC_OLYMPUS_ImageFormat			0xD00D
#define PTP_DPC_OLYMPUS_FaceDetection			0xD01A
#define PTP_DPC_OLYMPUS_AspectRatio			0xD01B
#define PTP_DPC_OLYMPUS_Shutterspeed			0xD01C
#define PTP_DPC_OLYMPUS_WhiteBalance			0xD01E
#define PTP_DPC_OLYMPUS_LiveViewModeOM			0xD06D
#define PTP_DPC_OLYMPUS_CaptureTarget			0xD0DC

/* unsure where these were from */
#define PTP_DPC_OLYMPUS_ResolutionMode			0xD102
#define PTP_DPC_OLYMPUS_FocusPriority			0xD103
#define PTP_DPC_OLYMPUS_DriveMode			0xD104
#define PTP_DPC_OLYMPUS_DateTimeFormat			0xD105
#define PTP_DPC_OLYMPUS_ExposureBiasStep		0xD106
#define PTP_DPC_OLYMPUS_WBMode				0xD107
#define PTP_DPC_OLYMPUS_OneTouchWB			0xD108
#define PTP_DPC_OLYMPUS_ManualWB			0xD109
#define PTP_DPC_OLYMPUS_ManualWBRBBias			0xD10A
#define PTP_DPC_OLYMPUS_CustomWB			0xD10B
#define PTP_DPC_OLYMPUS_CustomWBValue			0xD10C
#define PTP_DPC_OLYMPUS_ExposureTimeEx			0xD10D
#define PTP_DPC_OLYMPUS_BulbMode			0xD10E
#define PTP_DPC_OLYMPUS_AntiMirrorMode			0xD10F
#define PTP_DPC_OLYMPUS_AEBracketingFrame		0xD110
#define PTP_DPC_OLYMPUS_AEBracketingStep		0xD111
#define PTP_DPC_OLYMPUS_WBBracketingFrame		0xD112
#define PTP_DPC_OLYMPUS_WBBracketingRBFrame		0xD112 /* dup ? */
#define PTP_DPC_OLYMPUS_WBBracketingRBRange		0xD113
#define PTP_DPC_OLYMPUS_WBBracketingGMFrame		0xD114
#define PTP_DPC_OLYMPUS_WBBracketingGMRange		0xD115
#define PTP_DPC_OLYMPUS_FLBracketingFrame		0xD118
#define PTP_DPC_OLYMPUS_FLBracketingStep		0xD119
#define PTP_DPC_OLYMPUS_FlashBiasCompensation		0xD11A
#define PTP_DPC_OLYMPUS_ManualFocusMode			0xD11B
#define PTP_DPC_OLYMPUS_RawSaveMode			0xD11D
#define PTP_DPC_OLYMPUS_AUXLightMode			0xD11E
#define PTP_DPC_OLYMPUS_LensSinkMode			0xD11F
#define PTP_DPC_OLYMPUS_BeepStatus			0xD120
#define PTP_DPC_OLYMPUS_ColorSpace			0xD122
#define PTP_DPC_OLYMPUS_ColorMatching			0xD123
#define PTP_DPC_OLYMPUS_Saturation			0xD124
#define PTP_DPC_OLYMPUS_NoiseReductionPattern		0xD126
#define PTP_DPC_OLYMPUS_NoiseReductionRandom		0xD127
#define PTP_DPC_OLYMPUS_ShadingMode			0xD129
#define PTP_DPC_OLYMPUS_ISOBoostMode			0xD12A
#define PTP_DPC_OLYMPUS_ExposureIndexBiasStep		0xD12B
#define PTP_DPC_OLYMPUS_FilterEffect			0xD12C
#define PTP_DPC_OLYMPUS_ColorTune			0xD12D
#define PTP_DPC_OLYMPUS_Language			0xD12E
#define PTP_DPC_OLYMPUS_LanguageCode			0xD12F
#define PTP_DPC_OLYMPUS_RecviewMode			0xD130
#define PTP_DPC_OLYMPUS_SleepTime			0xD131
#define PTP_DPC_OLYMPUS_ManualWBGMBias			0xD132
#define PTP_DPC_OLYMPUS_AELAFLMode			0xD135
#define PTP_DPC_OLYMPUS_AELButtonStatus			0xD136
#define PTP_DPC_OLYMPUS_CompressionSettingEx		0xD137
#define PTP_DPC_OLYMPUS_ToneMode			0xD139
#define PTP_DPC_OLYMPUS_GradationMode			0xD13A
#define PTP_DPC_OLYMPUS_DevelopMode			0xD13B
#define PTP_DPC_OLYMPUS_ExtendInnerFlashMode		0xD13C
#define PTP_DPC_OLYMPUS_OutputDeviceMode		0xD13D
#define PTP_DPC_OLYMPUS_LiveViewMode			0xD13E
#define PTP_DPC_OLYMPUS_LCDBacklight			0xD140
#define PTP_DPC_OLYMPUS_CustomDevelop			0xD141
#define PTP_DPC_OLYMPUS_GradationAutoBias		0xD142
#define PTP_DPC_OLYMPUS_FlashRCMode			0xD143
#define PTP_DPC_OLYMPUS_FlashRCGroupValue		0xD144
#define PTP_DPC_OLYMPUS_FlashRCChannelValue		0xD145
#define PTP_DPC_OLYMPUS_FlashRCFPMode			0xD146
#define PTP_DPC_OLYMPUS_FlashRCPhotoChromicMode		0xD147
#define PTP_DPC_OLYMPUS_FlashRCPhotoChromicBias		0xD148
#define PTP_DPC_OLYMPUS_FlashRCPhotoChromicManualBias	0xD149
#define PTP_DPC_OLYMPUS_FlashRCQuantityLightLevel	0xD14A
#define PTP_DPC_OLYMPUS_FocusMeteringValue		0xD14B
#define PTP_DPC_OLYMPUS_ISOBracketingFrame		0xD14C
#define PTP_DPC_OLYMPUS_ISOBracketingStep		0xD14D
#define PTP_DPC_OLYMPUS_BulbMFMode			0xD14E
#define PTP_DPC_OLYMPUS_BurstFPSValue			0xD14F
#define PTP_DPC_OLYMPUS_ISOAutoBaseValue		0xD150
#define PTP_DPC_OLYMPUS_ISOAutoMaxValue			0xD151
#define PTP_DPC_OLYMPUS_BulbLimiterValue		0xD152
#define PTP_DPC_OLYMPUS_DPIMode				0xD153
#define PTP_DPC_OLYMPUS_DPICustomValue			0xD154
#define PTP_DPC_OLYMPUS_ResolutionValueSetting		0xD155
#define PTP_DPC_OLYMPUS_AFTargetSize			0xD157
#define PTP_DPC_OLYMPUS_LightSensorMode			0xD158
#define PTP_DPC_OLYMPUS_AEBracket			0xD159
#define PTP_DPC_OLYMPUS_WBRBBracket			0xD15A
#define PTP_DPC_OLYMPUS_WBGMBracket			0xD15B
#define PTP_DPC_OLYMPUS_FlashBracket			0xD15C
#define PTP_DPC_OLYMPUS_ISOBracket			0xD15D
#define PTP_DPC_OLYMPUS_MyModeStatus			0xD15E
#define PTP_DPC_OLYMPUS_DateTimeUTC			0xD176 /* check */

/* Sony A900 */
#define PTP_DPC_SONY_DPCCompensation			0xD200
#define PTP_DPC_SONY_DRangeOptimize			0xD201
#define PTP_DPC_SONY_ImageSize				0xD203
#define PTP_DPC_SONY_ShutterSpeed			0xD20D
#define PTP_DPC_SONY_ColorTemp				0xD20F
#define PTP_DPC_SONY_CCFilter				0xD210
#define PTP_DPC_SONY_AspectRatio			0xD211
#define PTP_DPC_SONY_FocusFound     			0xD213 /* seems to be signaled (1->2) when focus is achieved */
#define PTP_DPC_SONY_Zoom     				0xD214 /* might be focal length * 1.000.000 */
#define PTP_DPC_SONY_ObjectInMemory     		0xD215 /* used to signal when to retrieve new object */
#define PTP_DPC_SONY_ExposeIndex			0xD216
#define PTP_DPC_SONY_BatteryLevel			0xD218
#define PTP_DPC_SONY_SensorCrop				0xD219
#define PTP_DPC_SONY_PictureEffect			0xD21B
#define PTP_DPC_SONY_ABFilter				0xD21C
#define PTP_DPC_SONY_ISO				0xD21E	/* ? */
#define PTP_DPC_SONY_StillImageStoreDestination		0xD222  /* (type=0x4) Enumeration [1,17,16] value: 17 */
/* guessed DPC_SONY_DateTimeSettings 0xD223  error on query */
/* guessed DPC_SONY_FocusArea 0xD22C  (type=0x4) Enumeration [1,2,3,257,258,259,260,513,514,515,516,517,518,519,261,520] value: 1 */
/* guessed DPC_SONY_LiveDisplayEffect 0xD231 (type=0x2) Enumeration [1,2] value: 1 */
/* guessed DPC_SONY_FileType 0xD235  (enum: 0,1) */
/* guessed DPC_SONY_JpegQuality 0xD252 */
/* guessed DPC_SONY_PriorityKeySettings 0xD25A */
/* d255 reserved 5 */
/* d254 reserved 4 */
#define PTP_DPC_SONY_ExposureCompensation		0xD224
#define PTP_DPC_SONY_ISO2				0xD226
#define PTP_DPC_SONY_ShutterSpeed2			0xD229
#define PTP_DPC_SONY_AutoFocus				0xD2C1 /* ? half-press */
#define PTP_DPC_SONY_Capture				0xD2C2 /* ? full-press */
/* D2DB (2) , D2D3 (2) , D2C8 (2) also seen in Camera Remote related to D2C2 */
/* S1 ?
 * AEL - d2c3
 * FEL - d2c9
 * AFL - d2c4
 * AWBL - d2d9
 */
/* semi control opcodes */
#define PTP_DPC_SONY_Movie				0xD2C8 /* ? */
#define PTP_DPC_SONY_StillImage				0xD2C7 /* ? */

#define PTP_DPC_SONY_NearFar				0xD2D1
/*#define PTP_DPC_SONY_AutoFocus				0xD2D2 something related */

#define PTP_DPC_SONY_AF_Area_Position			0xD2DC

/* Sony QX properties */
/* all for 96f8 Control Device */
#define PTP_DPC_SONY_QX_Zoom_Absolute			0xD60E
#define PTP_DPC_SONY_QX_Movie_Rec			0xD60F
#define PTP_DPC_SONY_QX_Request_For_Update		0xD612
#define PTP_DPC_SONY_QX_Zoom_Wide_For_One_Shot		0xD613
#define PTP_DPC_SONY_QX_Zoom_Tele_For_One_Shot		0xD614
#define PTP_DPC_SONY_QX_S2_Button 			0xD617
#define PTP_DPC_SONY_QX_Media_Format 			0xD61C
#define PTP_DPC_SONY_QX_S1_Button 			0xD61D
#define PTP_DPC_SONY_QX_AE_Lock 			0xD61E
#define PTP_DPC_SONY_QX_Request_For_Update_For_Lens 	0xD625
#define PTP_DPC_SONY_QX_Power_Off			0xD637
#define PTP_DPC_SONY_QX_RequestOneShooting		0xD638
#define PTP_DPC_SONY_QX_AF_Lock				0xD63B
#define PTP_DPC_SONY_QX_Zoom_Tele			0xD63C
#define PTP_DPC_SONY_QX_Zoom_Wide			0xD63E
#define PTP_DPC_SONY_QX_Focus_Magnification		0xD641
#define PTP_DPC_SONY_QX_Focus_Near_For_One_Shot		0xD6A1
#define PTP_DPC_SONY_QX_Focus_Far_For_One_Shot		0xD6A2
#define PTP_DPC_SONY_QX_Focus_Near_For_Continuous	0xD6A3
#define PTP_DPC_SONY_QX_Focus_Far_For_Continuous	0xD6A4
#define PTP_DPC_SONY_QX_Camera_Setting_Reset		0xD6D9
#define PTP_DPC_SONY_QX_Camera_Initialize		0xD6DA

/* old */
#define PTP_DPC_SONY_QX_Capture				0xD617
#define PTP_DPC_SONY_QX_AutoFocus			0xD61D

/* set via 96fa */
#define PTP_DPC_SONY_QX_PictureProfileInitialize 	0xD620
#define PTP_DPC_SONY_QX_PictureProfile 			0xD621
#define PTP_DPC_SONY_QX_AFSPrioritySetting 		0xD622
#define PTP_DPC_SONY_QX_AFCPrioritySetting 		0xD623
#define PTP_DPC_SONY_QX_LensUpdateState 		0xD624
#define PTP_DPC_SONY_QX_SilentShooting 			0xD626
#define PTP_DPC_SONY_QX_HDMIInfoDisplay 		0xD627
#define PTP_DPC_SONY_QX_TCUBDisp 			0xD628
#define PTP_DPC_SONY_QX_TCPreset 			0xD629
#define PTP_DPC_SONY_QX_TCMake 				0xD62A
#define PTP_DPC_SONY_QX_TCRun 				0xD62B
#define PTP_DPC_SONY_QX_UBPreset 			0xD62C
#define PTP_DPC_SONY_QX_TCFormat 			0xD62D
#define PTP_DPC_SONY_QX_LongExposureNR 			0xD62E
#define PTP_DPC_SONY_QX_UBTimeRec 			0xD62F
#define PTP_DPC_SONY_QX_FocusMagnificationLevel 	0xD6A7
#define PTP_DPC_SONY_QX_FocusMagnificationPosition 	0xD6A8
#define PTP_DPC_SONY_QX_LensStatus 			0xD6A9
#define PTP_DPC_SONY_QX_LiveviewResolution 		0xD6AA
#define PTP_DPC_SONY_QX_NotifyFocusPosition 		0xD6AF
#define PTP_DPC_SONY_QX_DriveMode 			0xD6B0
#define PTP_DPC_SONY_QX_DateTime 			0xD6B1
#define PTP_DPC_SONY_QX_AspectRatio 			0xD6B3
#define PTP_DPC_SONY_QX_ImageSize 			0xD6B7
#define PTP_DPC_SONY_QX_WhiteBalance 			0xD6B8
#define PTP_DPC_SONY_QX_CompressionSetting 		0xD6B9
#define PTP_DPC_SONY_QX_CautionError 			0xD6BA
#define PTP_DPC_SONY_QX_StorageInformation		0xD6BB
#define PTP_DPC_SONY_QX_MovieQualitySetting 		0xD6BC
#define PTP_DPC_SONY_QX_MovieFormatSetting 		0xD6BD
#define PTP_DPC_SONY_QX_ZoomSetAbsolute 		0xD6BE
#define PTP_DPC_SONY_QX_ZoomInformation 		0xD6BF
#define PTP_DPC_SONY_QX_FocusSpeedForOneShot 		0xD6C1
#define PTP_DPC_SONY_QX_FlashCompensation 		0xD6C2
#define PTP_DPC_SONY_QX_ExposureCompensation 		0xD6C3
#define PTP_DPC_SONY_QX_Aperture 			0xD6C5
#define PTP_DPC_SONY_QX_ShootingFileInformation		0xD6C6
#define PTP_DPC_SONY_QX_MediaFormatState		0xD6C7
#define PTP_DPC_SONY_QX_ZoomMode 			0xD6C9
#define PTP_DPC_SONY_QX_FlashMode 			0xD6CA
#define PTP_DPC_SONY_QX_FocusMode 			0xD6CB
#define PTP_DPC_SONY_QX_ExposureMode 			0xD6CC
#define PTP_DPC_SONY_QX_MovieRecordingState 		0xD6CD
#define PTP_DPC_SONY_QX_SelectSaveMedia 		0xD6CF
#define PTP_DPC_SONY_QX_StillSteady 			0xD6D0
#define PTP_DPC_SONY_QX_MovieSteady 			0xD6D1
#define PTP_DPC_SONY_QX_Housing 			0xD6D2
#define PTP_DPC_SONY_QX_K4OutputSetting 		0xD6D3
#define PTP_DPC_SONY_QX_HDMIRECControl 			0xD6D4
#define PTP_DPC_SONY_QX_TimeCodeOutputToHDMI 		0xD6D5
#define PTP_DPC_SONY_QX_HDMIResolution 			0xD6D6
#define PTP_DPC_SONY_QX_NTSC_PAL_Selector		0xD6D7
#define PTP_DPC_SONY_QX_HDMIOutput 			0xD6D8
#define PTP_DPC_SONY_QX_ISOAutoMinimum 			0xD6DB
#define PTP_DPC_SONY_QX_ISOAutoMaximum 			0xD6DC
#define PTP_DPC_SONY_QX_APSCSuper35mm 			0xD6DD
#define PTP_DPC_SONY_QX_LiveviewStatus 			0xD6DE
#define PTP_DPC_SONY_QX_WhiteBalanceInitialize 		0xD6DF
#define PTP_DPC_SONY_QX_OperatingMode 			0xD6E2
#define PTP_DPC_SONY_QX_BiaxialFineTuningABDirection	0xD6E3
#define PTP_DPC_SONY_QX_HighISONr			0xD6E5
#define PTP_DPC_SONY_QX_AELockIndication 		0xD6E8
#define PTP_DPC_SONY_QX_ElectronicFrontCurtainShutter 	0xD6E9
#define PTP_DPC_SONY_QX_ShutterSpeed 			0xD6EA
#define PTP_DPC_SONY_QX_FocusIndication 		0xD6EC
#define PTP_DPC_SONY_QX_BiaxialFineTuningGMDirection	0xD6EF
#define PTP_DPC_SONY_QX_ColorTemperature		0xD6F0
#define PTP_DPC_SONY_QX_BatteryLevelIndication		0xD6F1
#define PTP_DPC_SONY_QX_ISO 				0xD6F2
#define PTP_DPC_SONY_QX_AutoSlowShutter 		0xD6F3
#define PTP_DPC_SONY_QX_DynamicRangeOptimizer 		0xD6FE


/* Casio EX-F1 */
#define PTP_DPC_CASIO_MONITOR		0xD001
#define PTP_DPC_CASIO_STORAGE		0xD002 //Not reported by DeviceInfo?
#define PTP_DPC_CASIO_UNKNOWN_1		0xD004
#define PTP_DPC_CASIO_UNKNOWN_2		0xD005
#define PTP_DPC_CASIO_UNKNOWN_3		0xD007
#define PTP_DPC_CASIO_RECORD_LIGHT	0xD008
#define PTP_DPC_CASIO_UNKNOWN_4		0xD009
#define PTP_DPC_CASIO_UNKNOWN_5		0xD00A
#define PTP_DPC_CASIO_MOVIE_MODE	0xD00B
#define PTP_DPC_CASIO_HD_SETTING	0xD00C
#define PTP_DPC_CASIO_HS_SETTING	0xD00D
#define PTP_DPC_CASIO_CS_HIGH_SPEED	0xD00F
#define PTP_DPC_CASIO_CS_UPPER_LIMIT	0xD010
#define PTP_DPC_CASIO_CS_SHOT		0xD011
#define PTP_DPC_CASIO_UNKNOWN_6		0xD012
#define PTP_DPC_CASIO_UNKNOWN_7		0xD013
#define PTP_DPC_CASIO_UNKNOWN_8		0xD015
#define PTP_DPC_CASIO_UNKNOWN_9		0xD017
#define PTP_DPC_CASIO_UNKNOWN_10	0xD018
#define PTP_DPC_CASIO_UNKNOWN_11	0xD019
#define PTP_DPC_CASIO_UNKNOWN_12	0xD01A
#define PTP_DPC_CASIO_UNKNOWN_13	0xD01B
#define PTP_DPC_CASIO_UNKNOWN_14	0xD01C
#define PTP_DPC_CASIO_UNKNOWN_15	0xD01D
#define PTP_DPC_CASIO_UNKNOWN_16	0xD020
#define PTP_DPC_CASIO_UNKNOWN_17	0xD030
#define PTP_DPC_CASIO_UNKNOWN_18	0xD080

#define PTP_DPC_RICOH_ShutterSpeed	0xD00F

/* https://github.com/Parrot-Developers/sequoia-ptpy */
#define PTP_DPC_PARROT_PhotoSensorEnableMask			0xD201
#define PTP_DPC_PARROT_PhotoSensorsKeepOn			0xD202
#define PTP_DPC_PARROT_MultispectralImageSize			0xD203
#define PTP_DPC_PARROT_MainBitDepth				0xD204
#define PTP_DPC_PARROT_MultispectralBitDepth			0xD205
#define PTP_DPC_PARROT_HeatingEnable				0xD206
#define PTP_DPC_PARROT_WifiStatus				0xD207
#define PTP_DPC_PARROT_WifiSSID					0xD208
#define PTP_DPC_PARROT_WifiEncryptionType			0xD209
#define PTP_DPC_PARROT_WifiPassphrase				0xD20A
#define PTP_DPC_PARROT_WifiChannel				0xD20B
#define PTP_DPC_PARROT_Localization				0xD20C
#define PTP_DPC_PARROT_WifiMode					0xD20D
#define PTP_DPC_PARROT_AntiFlickeringFrequency			0xD210
#define PTP_DPC_PARROT_DisplayOverlayMask			0xD211
#define PTP_DPC_PARROT_GPSInterval				0xD212
#define PTP_DPC_PARROT_MultisensorsExposureMeteringMode		0xD213
#define PTP_DPC_PARROT_MultisensorsExposureTime			0xD214
#define PTP_DPC_PARROT_MultisensorsExposureProgramMode		0xD215
#define PTP_DPC_PARROT_MultisensorsExposureIndex		0xD216
#define PTP_DPC_PARROT_MultisensorsIrradianceGain		0xD217
#define PTP_DPC_PARROT_MultisensorsIrradianceIntegrationTime	0xD218
#define PTP_DPC_PARROT_OverlapRate				0xD219

/* Panasonic does not have regular device properties, they use some 32bit values */
#define PTP_DPC_PANASONIC_PhotoStyle			0x02000010
#define PTP_DPC_PANASONIC_ISO				0x02000020
#define PTP_DPC_PANASONIC_ShutterSpeed			0x02000030
#define PTP_DPC_PANASONIC_Aperture			0x02000040
#define PTP_DPC_PANASONIC_WhiteBalance			0x02000050
#define PTP_DPC_PANASONIC_Exposure			0x02000060
#define PTP_DPC_PANASONIC_AFArea			0x02000070
#define PTP_DPC_PANASONIC_CameraMode			0x02000080
#define PTP_DPC_PANASONIC_ImageFormat			0x020000A2
#define PTP_DPC_PANASONIC_MeteringInfo			0x020000B0
#define PTP_DPC_PANASONIC_IntervalInfo			0x020000C0
#define PTP_DPC_PANASONIC_RecDispConfig			0x020000E0
#define PTP_DPC_PANASONIC_RecInfoFlash			0x02000110
#define PTP_DPC_PANASONIC_BurstBracket			0x02000140
#define PTP_DPC_PANASONIC_RecPreviewConfig		0x02000170
#define PTP_DPC_PANASONIC_RecInfoSelfTimer		0x020001A0
#define PTP_DPC_PANASONIC_RecInfoFlash2			0x020001B0
#define PTP_DPC_PANASONIC_MovConfig			0x06000010
#define PTP_DPC_PANASONIC_08000010			0x08000010
/* various modes of the camera, HDMI, GetDateTimeWorldTime Mode/Area, SetupCfgInfo, SetupConfig_DateTime, GetSystemFreq Mode, GetSetupConfig Info */
/*
0000  54 00 00 00 02 00 0a 94-04 00 00 00 11 00 00 08  T...............
0010  0a 00 00 00 e2 07 07 00-10 00 11 00 09 00 12 00  ................
0020  00 08 02 00 00 00 00 00-13 00 00 08 02 00 00 00  ................
0030  00 00 14 00 00 08 04 00-00 00 00 00 00 00 15 00  ................
0040  00 08 04 00 00 00 00 00-00 00 16 00 00 08 02 00  ................
0050  00 00 01 00            -                         ....

0000  d0 00 00 00 02 00 07 91-04 00 00 00 10 00 00 08  ................
0010  14 00 00 00 14 00 00 00-01 00 01 00 00 00 00 00  ................
0020  06 00 00 00 38 03 00 00-11 00 00 08 14 00 00 00  ....8...........
0030  14 00 00 00 01 00 01 00-00 00 00 00 05 00 00 00  ................
0040  c8 00 00 00 12 00 00 08-14 00 00 00 14 00 00 00  ................
0050  01 00 01 00 00 00 00 00-01 00 00 00 36 00 00 00  ............6...
0060  13 00 00 08 14 00 00 00-14 00 00 00 01 00 01 00  ................
0070  00 00 00 00 01 00 00 00-2a 00 00 00 14 00 00 08  ........*.......
0080  14 00 00 00 14 00 00 00-01 00 01 00 00 00 00 00  ................
0090  02 00 00 00 9e 00 00 00-15 00 00 08 14 00 00 00  ................
00a0  14 00 00 00 01 00 01 00-00 00 00 00 02 00 00 00  ................
00b0  9e 00 00 00 16 00 00 08-14 00 00 00 14 00 00 00  ................
00c0  01 00 01 00 00 00 00 00-01 00 00 00 2c 00 00 00  ............,...
 */
#define PTP_DPC_PANASONIC_08000091			0x08000091 /* SetupFilesConfig_Set_Target */
/*
0000  16 00 00 00 02 00 0a 94-04 00 00 00 91 00 00 08  ................
0010  02 00 00 00 00 00      -                         ......

0000  44 00 00 00 02 00 07 91-04 00 00 00 90 00 00 08  D...............
0010  14 00 00 00 14 00 00 00-01 00 01 00 00 00 00 00  ................
0020  01 00 00 00 48 00 00 00-91 00 00 08 14 00 00 00  ....H...........
0030  14 00 00 00 01 00 01 00-00 00 00 00 01 00 00 00  ................
0040  2c 00 00 00            -                         ,...
 */

#define PTP_DPC_PANASONIC_GetFreeSpaceInImages		0x12000010
/*
0000  98 00 00 00 02 00 14 94-04 00 00 00 11 00 00 12  ................
0010  04 00 00 00 4e 00 00 00-12 00 00 12 04 00 00 00  ....N...........
0020  00 00 00 00 13 00 00 12-02 00 00 00 00 00 14 00  ................
0030  00 12 04 00 00 00 00 00-00 00 15 00 00 12 06 00  ................
0040  00 00 02 00 01 00 00 00-16 00 00 12 3a 00 00 00  ............:...
0050  02 00 4e 00 00 00 00 00-00 00 4e 00 00 00 ff ff  ..N.......N.....
0060  ff ff 00 00 00 00 ff ff-ff ff 00 00 00 00 00 00  ................
0070  00 00 00 00 00 00 ff ff-ff ff 00 00 00 00 ff ff  ................
0080  ff ff 00 00 00 00 00 00-00 00 17 00 00 12 06 00  ................
0090  00 00 00 00 00 00 00 00-                         ........
 */
#define PTP_DPC_PANASONIC_GetBatteryInfo		0x16000010
/*
0000  1c 00 00 00 02 00 14 94-04 00 00 00 11 00 00 16  ................
0010  08 00 00 00 4b 00 00 00-4b 00 ff ff              ....K...K...
 */
#define PTP_DPC_PANASONIC_LensGetMFBar			0x12010040
/* 15c00010 GetSetupInfo Error */
/* 18000010 GetUSBSpeed */

/* Leica */
#define PTP_DPC_LEICA_ExternalShooting			0xD018
/* d040 */
/* d60c */
/* d60e */
/* d610 */


/* MTP specific Object Properties */
#define PTP_OPC_StorageID				0xDC01
#define PTP_OPC_ObjectFormat				0xDC02
#define PTP_OPC_ProtectionStatus			0xDC03
#define PTP_OPC_ObjectSize				0xDC04
#define PTP_OPC_AssociationType				0xDC05
#define PTP_OPC_AssociationDesc				0xDC06
#define PTP_OPC_ObjectFileName				0xDC07
#define PTP_OPC_DateCreated				0xDC08
#define PTP_OPC_DateModified				0xDC09
#define PTP_OPC_Keywords				0xDC0A
#define PTP_OPC_ParentObject				0xDC0B
#define PTP_OPC_AllowedFolderContents			0xDC0C
#define PTP_OPC_Hidden					0xDC0D
#define PTP_OPC_SystemObject				0xDC0E
#define PTP_OPC_PersistantUniqueObjectIdentifier	0xDC41
#define PTP_OPC_SyncID					0xDC42
#define PTP_OPC_PropertyBag				0xDC43
#define PTP_OPC_Name					0xDC44
#define PTP_OPC_CreatedBy				0xDC45
#define PTP_OPC_Artist					0xDC46
#define PTP_OPC_DateAuthored				0xDC47
#define PTP_OPC_Description				0xDC48
#define PTP_OPC_URLReference				0xDC49
#define PTP_OPC_LanguageLocale				0xDC4A
#define PTP_OPC_CopyrightInformation			0xDC4B
#define PTP_OPC_Source					0xDC4C
#define PTP_OPC_OriginLocation				0xDC4D
#define PTP_OPC_DateAdded				0xDC4E
#define PTP_OPC_NonConsumable				0xDC4F
#define PTP_OPC_CorruptOrUnplayable			0xDC50
#define PTP_OPC_ProducerSerialNumber			0xDC51
#define PTP_OPC_RepresentativeSampleFormat		0xDC81
#define PTP_OPC_RepresentativeSampleSize		0xDC82
#define PTP_OPC_RepresentativeSampleHeight		0xDC83
#define PTP_OPC_RepresentativeSampleWidth		0xDC84
#define PTP_OPC_RepresentativeSampleDuration		0xDC85
#define PTP_OPC_RepresentativeSampleData		0xDC86
#define PTP_OPC_Width					0xDC87
#define PTP_OPC_Height					0xDC88
#define PTP_OPC_Duration				0xDC89
#define PTP_OPC_Rating					0xDC8A
#define PTP_OPC_Track					0xDC8B
#define PTP_OPC_Genre					0xDC8C
#define PTP_OPC_Credits					0xDC8D
#define PTP_OPC_Lyrics					0xDC8E
#define PTP_OPC_SubscriptionContentID			0xDC8F
#define PTP_OPC_ProducedBy				0xDC90
#define PTP_OPC_UseCount				0xDC91
#define PTP_OPC_SkipCount				0xDC92
#define PTP_OPC_LastAccessed				0xDC93
#define PTP_OPC_ParentalRating				0xDC94
#define PTP_OPC_MetaGenre				0xDC95
#define PTP_OPC_Composer				0xDC96
#define PTP_OPC_EffectiveRating				0xDC97
#define PTP_OPC_Subtitle				0xDC98
#define PTP_OPC_OriginalReleaseDate			0xDC99
#define PTP_OPC_AlbumName				0xDC9A
#define PTP_OPC_AlbumArtist				0xDC9B
#define PTP_OPC_Mood					0xDC9C
#define PTP_OPC_DRMStatus				0xDC9D
#define PTP_OPC_SubDescription				0xDC9E
#define PTP_OPC_IsCropped				0xDCD1
#define PTP_OPC_IsColorCorrected			0xDCD2
#define PTP_OPC_ImageBitDepth				0xDCD3
#define PTP_OPC_Fnumber					0xDCD4
#define PTP_OPC_ExposureTime				0xDCD5
#define PTP_OPC_ExposureIndex				0xDCD6
#define PTP_OPC_DisplayName				0xDCE0
#define PTP_OPC_BodyText				0xDCE1
#define PTP_OPC_Subject					0xDCE2
#define PTP_OPC_Priority				0xDCE3
#define PTP_OPC_GivenName				0xDD00
#define PTP_OPC_MiddleNames				0xDD01
#define PTP_OPC_FamilyName				0xDD02
#define PTP_OPC_Prefix					0xDD03
#define PTP_OPC_Suffix					0xDD04
#define PTP_OPC_PhoneticGivenName			0xDD05
#define PTP_OPC_PhoneticFamilyName			0xDD06
#define PTP_OPC_EmailPrimary				0xDD07
#define PTP_OPC_EmailPersonal1				0xDD08
#define PTP_OPC_EmailPersonal2				0xDD09
#define PTP_OPC_EmailBusiness1				0xDD0A
#define PTP_OPC_EmailBusiness2				0xDD0B
#define PTP_OPC_EmailOthers				0xDD0C
#define PTP_OPC_PhoneNumberPrimary			0xDD0D
#define PTP_OPC_PhoneNumberPersonal			0xDD0E
#define PTP_OPC_PhoneNumberPersonal2			0xDD0F
#define PTP_OPC_PhoneNumberBusiness			0xDD10
#define PTP_OPC_PhoneNumberBusiness2			0xDD11
#define PTP_OPC_PhoneNumberMobile			0xDD12
#define PTP_OPC_PhoneNumberMobile2			0xDD13
#define PTP_OPC_FaxNumberPrimary			0xDD14
#define PTP_OPC_FaxNumberPersonal			0xDD15
#define PTP_OPC_FaxNumberBusiness			0xDD16
#define PTP_OPC_PagerNumber				0xDD17
#define PTP_OPC_PhoneNumberOthers			0xDD18
#define PTP_OPC_PrimaryWebAddress			0xDD19
#define PTP_OPC_PersonalWebAddress			0xDD1A
#define PTP_OPC_BusinessWebAddress			0xDD1B
#define PTP_OPC_InstantMessengerAddress			0xDD1C
#define PTP_OPC_InstantMessengerAddress2		0xDD1D
#define PTP_OPC_InstantMessengerAddress3		0xDD1E
#define PTP_OPC_PostalAddressPersonalFull		0xDD1F
#define PTP_OPC_PostalAddressPersonalFullLine1		0xDD20
#define PTP_OPC_PostalAddressPersonalFullLine2		0xDD21
#define PTP_OPC_PostalAddressPersonalFullCity		0xDD22
#define PTP_OPC_PostalAddressPersonalFullRegion		0xDD23
#define PTP_OPC_PostalAddressPersonalFullPostalCode	0xDD24
#define PTP_OPC_PostalAddressPersonalFullCountry	0xDD25
#define PTP_OPC_PostalAddressBusinessFull		0xDD26
#define PTP_OPC_PostalAddressBusinessLine1		0xDD27
#define PTP_OPC_PostalAddressBusinessLine2		0xDD28
#define PTP_OPC_PostalAddressBusinessCity		0xDD29
#define PTP_OPC_PostalAddressBusinessRegion		0xDD2A
#define PTP_OPC_PostalAddressBusinessPostalCode		0xDD2B
#define PTP_OPC_PostalAddressBusinessCountry		0xDD2C
#define PTP_OPC_PostalAddressOtherFull			0xDD2D
#define PTP_OPC_PostalAddressOtherLine1			0xDD2E
#define PTP_OPC_PostalAddressOtherLine2			0xDD2F
#define PTP_OPC_PostalAddressOtherCity			0xDD30
#define PTP_OPC_PostalAddressOtherRegion		0xDD31
#define PTP_OPC_PostalAddressOtherPostalCode		0xDD32
#define PTP_OPC_PostalAddressOtherCountry		0xDD33
#define PTP_OPC_OrganizationName			0xDD34
#define PTP_OPC_PhoneticOrganizationName		0xDD35
#define PTP_OPC_Role					0xDD36
#define PTP_OPC_Birthdate				0xDD37
#define PTP_OPC_MessageTo				0xDD40
#define PTP_OPC_MessageCC				0xDD41
#define PTP_OPC_MessageBCC				0xDD42
#define PTP_OPC_MessageRead				0xDD43
#define PTP_OPC_MessageReceivedTime			0xDD44
#define PTP_OPC_MessageSender				0xDD45
#define PTP_OPC_ActivityBeginTime			0xDD50
#define PTP_OPC_ActivityEndTime				0xDD51
#define PTP_OPC_ActivityLocation			0xDD52
#define PTP_OPC_ActivityRequiredAttendees		0xDD54
#define PTP_OPC_ActivityOptionalAttendees		0xDD55
#define PTP_OPC_ActivityResources			0xDD56
#define PTP_OPC_ActivityAccepted			0xDD57
#define PTP_OPC_Owner					0xDD5D
#define PTP_OPC_Editor					0xDD5E
#define PTP_OPC_Webmaster				0xDD5F
#define PTP_OPC_URLSource				0xDD60
#define PTP_OPC_URLDestination				0xDD61
#define PTP_OPC_TimeBookmark				0xDD62
#define PTP_OPC_ObjectBookmark				0xDD63
#define PTP_OPC_ByteBookmark				0xDD64
#define PTP_OPC_LastBuildDate				0xDD70
#define PTP_OPC_TimetoLive				0xDD71
#define PTP_OPC_MediaGUID				0xDD72
#define PTP_OPC_TotalBitRate				0xDE91
#define PTP_OPC_BitRateType				0xDE92
#define PTP_OPC_SampleRate				0xDE93
#define PTP_OPC_NumberOfChannels			0xDE94
#define PTP_OPC_AudioBitDepth				0xDE95
#define PTP_OPC_ScanDepth				0xDE97
#define PTP_OPC_AudioWAVECodec				0xDE99
#define PTP_OPC_AudioBitRate				0xDE9A
#define PTP_OPC_VideoFourCCCodec			0xDE9B
#define PTP_OPC_VideoBitRate				0xDE9C
#define PTP_OPC_FramesPerThousandSeconds		0xDE9D
#define PTP_OPC_KeyFrameDistance			0xDE9E
#define PTP_OPC_BufferSize				0xDE9F
#define PTP_OPC_EncodingQuality				0xDEA0
#define PTP_OPC_EncodingProfile				0xDEA1
#define PTP_OPC_BuyFlag					0xD901

/* WiFi Provisioning MTP Extension property codes */
#define PTP_OPC_WirelessConfigurationFile		0xB104

/* Device Property Form Flag */

#define PTP_DPFF_None			0x00
#define PTP_DPFF_Range			0x01
#define PTP_DPFF_Enumeration		0x02

/* Object Property Codes used by MTP (first 3 are same as DPFF codes) */
#define PTP_OPFF_None			0x00
#define PTP_OPFF_Range			0x01
#define PTP_OPFF_Enumeration		0x02
#define PTP_OPFF_DateTime		0x03
#define PTP_OPFF_FixedLengthArray	0x04
#define PTP_OPFF_RegularExpression	0x05
#define PTP_OPFF_ByteArray		0x06
#define PTP_OPFF_LongString		0xFF

/* Device Property GetSet type */
#define PTP_DPGS_Get			0x00
#define PTP_DPGS_GetSet			0x01

/* Glue stuff starts here */

typedef struct _PTPParams PTPParams;


typedef uint16_t (* PTPDataGetFunc)	(PTPParams* params, void*priv,
					unsigned long wantlen,
	                                unsigned char *data, unsigned long *gotlen);

typedef uint16_t (* PTPDataPutFunc)	(PTPParams* params, void*priv,
					unsigned long sendlen,
	                                unsigned char *data);
typedef struct _PTPDataHandler {
	PTPDataGetFunc		getfunc;
	PTPDataPutFunc		putfunc;
	void			*priv;
} PTPDataHandler;

/*
 * This functions take PTP oriented arguments and send them over an
 * appropriate data layer doing byteorder conversion accordingly.
 */
typedef uint16_t (* PTPIOSendReq)	(PTPParams* params, PTPContainer* req, int dataphase);
typedef uint16_t (* PTPIOSendData)	(PTPParams* params, PTPContainer* ptp,
					 uint64_t size, PTPDataHandler*getter);

typedef uint16_t (* PTPIOGetResp)	(PTPParams* params, PTPContainer* resp);
typedef uint16_t (* PTPIOGetData)	(PTPParams* params, PTPContainer* ptp,
	                                 PTPDataHandler *putter);
typedef uint16_t (* PTPIOCancelReq)	(PTPParams* params, uint32_t transaction_id);
typedef uint16_t (* PTPIODevStatReq) (PTPParams* params);

/* debug functions */
typedef void (* PTPErrorFunc) (void *data, const char *format, va_list args)
#if (__GNUC__ >= 3)
	__attribute__((__format__(printf,2,0)))
#endif
;
typedef void (* PTPDebugFunc) (void *data, const char *format, va_list args)
#if (__GNUC__ >= 3)
	__attribute__((__format__(printf,2,0)))
#endif
;

struct _PTPObject {
	uint32_t	oid;
	unsigned int	flags;
#define PTPOBJECT_OBJECTINFO_LOADED	(1<<0)
#define PTPOBJECT_CANONFLAGS_LOADED	(1<<1)
#define PTPOBJECT_MTPPROPLIST_LOADED	(1<<2)
#define PTPOBJECT_DIRECTORY_LOADED	(1<<3)
#define PTPOBJECT_PARENTOBJECT_LOADED	(1<<4)
#define PTPOBJECT_STORAGEID_LOADED	(1<<5)

	PTPObjectInfo	oi;
	uint32_t	canon_flags;
	MTPProperties	*mtpprops;
	unsigned int	nrofmtpprops;
};
typedef struct _PTPObject PTPObject;

/* The Device Property Cache */
struct _PTPDeviceProperty {
	time_t			timestamp;
	PTPDevicePropDesc	desc;
	PTPPropertyValue	value;
};
typedef struct _PTPDeviceProperty PTPDeviceProperty;

struct _MTPPropertyDesc {
	uint16_t	opc;
	PTPObjectPropDesc	opd;
};
typedef struct _MTPPropertyDesc MTPPropertyDesc;

struct _MTPObjectFormat {
	uint16_t	ofc;
	unsigned int	nrofpds;
	MTPPropertyDesc	*pds;
};
typedef struct _MTPObjectFormat MTPObjectFormat;

/* Transaction data phase description, internal flags to sendreq / transaction driver. */
#define PTP_DP_NODATA           0x0000  /* no data phase */
#define PTP_DP_SENDDATA         0x0001  /* sending data */
#define PTP_DP_GETDATA          0x0002  /* receiving data */
#define PTP_DP_DATA_MASK        0x00ff  /* data phase mask */

struct _PTPParams {
	/* device flags */
	uint32_t	device_flags;

	/* data layer byteorder */
	uint8_t		byteorder;
	uint16_t	maxpacketsize;

	/* PTP IO: Custom IO functions */
	PTPIOSendReq	sendreq_func;
	PTPIOSendData	senddata_func;
	PTPIOGetResp	getresp_func;
	PTPIOGetData	getdata_func;
	PTPIOGetResp	event_check;
	PTPIOGetResp	event_check_queue;
	PTPIOGetResp	event_wait;
	PTPIOCancelReq	cancelreq_func;
	PTPIODevStatReq	devstatreq_func;

	/* Custom error and debug function */
	PTPErrorFunc	error_func;
	PTPDebugFunc	debug_func;

	/* Data passed to above functions */
	void		*data;

	/* ptp transaction ID */
	uint32_t	transaction_id;
	/* ptp session ID */
	uint32_t	session_id;

	/* used for open capture */
	uint32_t	opencapture_transid;

	/* PTP IO: if we have MTP style split header/data transfers */
	int		split_header_data;
	int		ocs64; /* 64bit objectsize */

	int		nrofobjectformats;
	MTPObjectFormat	*objectformats;

	/* PTP: internal structures used by ptp driver */
	PTPObject	*objects;
	unsigned int	nrofobjects;

	PTPDeviceInfo	deviceinfo;

	/* PTP: the current event queue */
	PTPContainer	*events;
	unsigned int	nrofevents;

	/* Capture count for SDRAM capture style images */
	unsigned int		capcnt;

	/* live view enabled */
	int			inliveview;

	/* PTP: caching time for properties, default 2 */
	int			cachetime;

	/* PTP: Storage Caching */
	PTPStorageIDs		storageids;
	int			storagechanged;

	/* PTP: Device Property Caching */
	PTPDeviceProperty	*deviceproperties;
	unsigned int		nrofdeviceproperties;

	/* PTP: Canon specific flags list */
	PTPCanon_Property	*canon_props;
	unsigned int		nrofcanon_props;
	int			canon_viewfinder_on;
	int			canon_event_mode;

	/* PTP: Canon EOS event queue */
	PTPCanon_changes_entry	*backlogentries;
	unsigned int		nrofbacklogentries;
	int			eos_captureenabled;
	int			eos_camerastatus;

	/* PTP: Nikon specifics */
	int			controlmode;
	int			event90c7works;
	int			deletesdramfails;

	/* PTP: Sony specific */
	struct timeval		starttime;

	/* PTP: Wifi profiles */
	uint8_t 	wifi_profiles_version;
	uint8_t		wifi_profiles_number;
	PTPNIKONWifiProfile *wifi_profiles;

	/* IO: PTP/IP related data */
	int		cmdfd, evtfd, jpgfd;
	uint8_t		cameraguid[16];
	uint32_t	eventpipeid;
	char		*cameraname;

	/* Olympus UMS wrapping related data */
	PTPDeviceInfo	outer_deviceinfo;
	char		*olympus_cmd;
	char		*olympus_reply;
	struct _PTPParams *outer_params;

#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
	/* PTP: iconv converters */
	iconv_t	cd_locale_to_ucs2;
	iconv_t cd_ucs2_to_locale;
#endif

	/* IO: Sometimes the response packet get send in the dataphase
	 * too. This only happens for a Samsung player now.
	 */
	uint8_t		*response_packet;
	uint16_t	response_packet_size;
};

/* Asynchronous event callback */
typedef void (*PTPEventCbFn)(PTPParams *params, uint16_t code, PTPContainer *event, void *user_data);

/* last, but not least - ptp functions */
uint16_t ptp_usb_sendreq	(PTPParams* params, PTPContainer* req, int dataphase);
uint16_t ptp_usb_senddata	(PTPParams* params, PTPContainer* ptp,
				 uint64_t size, PTPDataHandler *handler);
uint16_t ptp_usb_getresp	(PTPParams* params, PTPContainer* resp);
uint16_t ptp_usb_getdata	(PTPParams* params, PTPContainer* ptp,
	                         PTPDataHandler *handler);
uint16_t ptp_usb_event_async	(PTPParams *params, PTPEventCbFn cb, void *user_data);
uint16_t ptp_usb_event_wait	(PTPParams* params, PTPContainer* event);
uint16_t ptp_usb_event_check	(PTPParams* params, PTPContainer* event);
uint16_t ptp_usb_event_check_queue	(PTPParams* params, PTPContainer* event);

uint16_t ptp_usb_control_get_extended_event_data (PTPParams *params, char *buffer, int *size);
uint16_t ptp_usb_control_device_reset_request (PTPParams *params);
uint16_t ptp_usb_control_get_device_status (PTPParams *params, char *buffer, int *size);
uint16_t ptp_usb_control_cancel_request (PTPParams *params, uint32_t transid);
uint16_t ptp_usb_control_device_status_request (PTPParams *params);


int      ptp_ptpip_connect	(PTPParams* params, const char *port);
uint16_t ptp_ptpip_sendreq	(PTPParams* params, PTPContainer* req, int dataphase);
uint16_t ptp_ptpip_senddata	(PTPParams* params, PTPContainer* ptp,
				uint64_t size, PTPDataHandler *handler);
uint16_t ptp_ptpip_getresp	(PTPParams* params, PTPContainer* resp);
uint16_t ptp_ptpip_getdata	(PTPParams* params, PTPContainer* ptp,
	                         PTPDataHandler *handler);
uint16_t ptp_ptpip_event_wait	(PTPParams* params, PTPContainer* event);
uint16_t ptp_ptpip_event_check	(PTPParams* params, PTPContainer* event);
uint16_t ptp_ptpip_event_check_queue	(PTPParams* params, PTPContainer* event);

int      ptp_fujiptpip_connect	(PTPParams* params, const char *port);
int      ptp_fujiptpip_init_event (PTPParams* params, const char *address);
uint16_t ptp_fujiptpip_sendreq	(PTPParams* params, PTPContainer* req, int dataphase);
uint16_t ptp_fujiptpip_senddata	(PTPParams* params, PTPContainer* ptp,
				uint64_t size, PTPDataHandler *handler);
uint16_t ptp_fujiptpip_getresp	(PTPParams* params, PTPContainer* resp);
uint16_t ptp_fujiptpip_getdata	(PTPParams* params, PTPContainer* ptp,
	                         PTPDataHandler *handler);
uint16_t ptp_fujiptpip_event_wait	(PTPParams* params, PTPContainer* event);
uint16_t ptp_fujiptpip_event_check	(PTPParams* params, PTPContainer* event);
uint16_t ptp_fujiptpip_event_check_queue(PTPParams* params, PTPContainer* event);

uint16_t ptp_fujiptpip_jpeg (PTPParams* params, unsigned char** xdata, unsigned int *xsize);

uint16_t ptp_getdeviceinfo	(PTPParams* params, PTPDeviceInfo* deviceinfo);

uint16_t ptp_generic_no_data	(PTPParams* params, uint16_t opcode, unsigned int cnt, ...);

uint16_t ptp_opensession	(PTPParams *params, uint32_t session);

uint16_t ptp_transaction_new (PTPParams* params, PTPContainer* ptp,
                uint16_t flags, uint64_t sendlen,
                PTPDataHandler *handler
);
uint16_t ptp_transaction (PTPParams* params, PTPContainer* ptp,
                uint16_t flags, uint64_t sendlen,
                unsigned char **data, unsigned int *recvlen
);

/**
 * ptp_closesession:
 * params:      PTPParams*
 *
 * Closes session.
 *
 * Return values: Some PTP_RC_* code.
 **/
#define ptp_closesession(params) ptp_generic_no_data(params,PTP_OC_CloseSession,0)

/**
 * ptp_powerdown:
 * params:      PTPParams*
 *
 * Powers down device.
 *
 * Return values: Some PTP_RC_* code.
 **/
#define ptp_powerdown(params) ptp_generic_no_data(params,PTP_OC_PowerDown,0)
/**
 * ptp_resetdevice:
 * params:      PTPParams*
 *
 * Uses the built-in function to reset the device
 *
 * Return values: Some PTP_RC_* code.
 *
 */
#define ptp_resetdevice(params) ptp_generic_no_data(params,PTP_OC_ResetDevice,0)

uint16_t ptp_getstorageids	(PTPParams* params, PTPStorageIDs* storageids);
uint16_t ptp_getstorageinfo 	(PTPParams* params, uint32_t storageid,
				PTPStorageInfo* storageinfo);
/**
 * ptp_formatstore:
 * params:      PTPParams*
 *              storageid               - StorageID
 *
 * Formats the storage on the device.
 *
 * Return values: Some PTP_RC_* code.
 **/
#define ptp_formatstore(params,storageid) ptp_generic_no_data(params,PTP_OC_FormatStore,1,storageid)

uint16_t ptp_getobjecthandles 	(PTPParams* params, uint32_t storage,
				uint32_t objectformatcode,
				uint32_t associationOH,
				PTPObjectHandles* objecthandles);


uint16_t ptp_getnumobjects 	(PTPParams* params, uint32_t storage,
				uint32_t objectformatcode,
				uint32_t associationOH,
				uint32_t* numobs);

uint16_t ptp_getobjectinfo	(PTPParams *params, uint32_t handle,
				PTPObjectInfo* objectinfo);

uint16_t ptp_getobject		(PTPParams *params, uint32_t handle,
				unsigned char** object);
uint16_t ptp_getobject_with_size	(PTPParams *params, uint32_t handle,
				unsigned char** object, unsigned int *size);
uint16_t ptp_getobject_tofd     (PTPParams* params, uint32_t handle, int fd);
uint16_t ptp_getobject_to_handler (PTPParams* params, uint32_t handle, PTPDataHandler*);
uint16_t ptp_getpartialobject	(PTPParams* params, uint32_t handle, uint32_t offset,
				uint32_t maxbytes, unsigned char** object,
				uint32_t *len);
uint16_t ptp_getpartialobject_to_handler (PTPParams* params, uint32_t handle, uint32_t offset,
                        	uint32_t maxbytes, PTPDataHandler *handler);

uint16_t ptp_getthumb		(PTPParams *params, uint32_t handle,
				unsigned char** object, unsigned int *len);

uint16_t ptp_deleteobject	(PTPParams* params, uint32_t handle,
				uint32_t ofc);

uint16_t ptp_moveobject		(PTPParams* params, uint32_t handle,
				uint32_t storage, uint32_t parent);

uint16_t ptp_copyobject		(PTPParams* params, uint32_t handle,
				uint32_t storage, uint32_t parent);

uint16_t ptp_sendobjectinfo	(PTPParams* params, uint32_t* store,
				uint32_t* parenthandle, uint32_t* handle,
				PTPObjectInfo* objectinfo);
/**
 * ptp_setobjectprotection:
 * params:      PTPParams*
 *              uint16_t newprot        - object protection flag
 *
 * Set protection of object.
 *
 * Return values: Some PTP_RC_* code.
 *
 */
#define ptp_setobjectprotection(params,oid,newprot) ptp_generic_no_data(params,PTP_OC_SetObjectProtection,2,oid,newprot)
uint16_t ptp_sendobject		(PTPParams* params, unsigned char* object,
				 uint64_t size);
uint16_t ptp_sendobject_fromfd  (PTPParams* params, int fd, uint64_t size);
uint16_t ptp_sendobject_from_handler  (PTPParams* params, PTPDataHandler*, uint64_t size);
/**
 * ptp_initiatecapture:
 * params:      PTPParams*
 *              storageid               - destination StorageID on Responder
 *              ofc                     - object format code
 *
 * Causes device to initiate the capture of one or more new data objects
 * according to its current device properties, storing the data into store
 * indicated by storageid. If storageid is 0x00000000, the object(s) will
 * be stored in a store that is determined by the capturing device.
 * The capturing of new data objects is an asynchronous operation.
 *
 * Return values: Some PTP_RC_* code.
 **/
#define ptp_initiatecapture(params,storageid,ofc) ptp_generic_no_data(params,PTP_OC_InitiateCapture,2,storageid,ofc)

#define ptp_initiateopencapture(params,storageid,ofc)	ptp_generic_no_data(params,PTP_OC_InitiateOpenCapture,2,storageid,ofc)
#define ptp_terminateopencapture(params,transid)	ptp_generic_no_data(params,PTP_OC_TerminateOpenCapture,1,transid)

uint16_t ptp_getdevicepropdesc	(PTPParams* params, uint16_t propcode,
				PTPDevicePropDesc *devicepropertydesc);
uint16_t ptp_generic_getdevicepropdesc (PTPParams *params, uint16_t propcode,
				PTPDevicePropDesc *dpd);
uint16_t ptp_getdevicepropvalue	(PTPParams* params, uint16_t propcode,
				PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_setdevicepropvalue (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_generic_setdevicepropvalue (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_getfilesystemmanifest (PTPParams* params, uint32_t storage,
                        uint32_t objectformatcode, uint32_t associationOH,
        		uint64_t *numoifs, PTPObjectFilesystemInfo **oifs);
uint16_t ptp_getstreaminfo (PTPParams *params, uint32_t streamid, PTPStreamInfo *si);
uint16_t ptp_getstream (PTPParams* params, unsigned char **data, unsigned int *size);


uint16_t ptp_check_event (PTPParams *params);
uint16_t ptp_check_event_queue (PTPParams *params);
uint16_t ptp_wait_event (PTPParams *params);
uint16_t ptp_add_event (PTPParams *params, PTPContainer *evt);
int ptp_have_event(PTPParams *params, uint16_t code);
int ptp_get_one_event (PTPParams *params, PTPContainer *evt);
int ptp_get_one_event_by_type(PTPParams *params, uint16_t code, PTPContainer *event);
uint16_t ptp_check_eos_events (PTPParams *params);
int ptp_get_one_eos_event (PTPParams *params, PTPCanon_changes_entry *entry);


/* Microsoft MTP extensions */
uint16_t ptp_mtp_getobjectpropdesc (PTPParams* params, uint16_t opc, uint16_t ofc,
				PTPObjectPropDesc *objectpropertydesc);
uint16_t ptp_mtp_getobjectpropvalue (PTPParams* params, uint32_t oid, uint16_t opc,
				PTPPropertyValue *value, uint16_t datatype);
uint16_t ptp_mtp_setobjectpropvalue (PTPParams* params, uint32_t oid, uint16_t opc,
				PTPPropertyValue *value, uint16_t datatype);
uint16_t ptp_mtp_getobjectreferences (PTPParams* params, uint32_t handle, uint32_t** ohArray, uint32_t* arraylen);
uint16_t ptp_mtp_setobjectreferences (PTPParams* params, uint32_t handle, uint32_t* ohArray, uint32_t arraylen);
uint16_t ptp_mtp_getobjectproplist_generic (PTPParams* params, uint32_t handle, uint32_t formats, uint32_t properties, uint32_t propertygroups, uint32_t level, MTPProperties **props, int *nrofprops);
uint16_t ptp_mtp_getobjectproplist_level (PTPParams* params, uint32_t handle, uint32_t level, MTPProperties **props, int *nrofprops);
uint16_t ptp_mtp_getobjectproplist (PTPParams* params, uint32_t handle, MTPProperties **props, int *nrofprops);
uint16_t ptp_mtp_getobjectproplist_single (PTPParams* params, uint32_t handle, MTPProperties **props, int *nrofprops);
uint16_t ptp_mtp_sendobjectproplist (PTPParams* params, uint32_t* store, uint32_t* parenthandle, uint32_t* handle,
				     uint16_t objecttype, uint64_t objectsize, MTPProperties *props, int nrofprops);
uint16_t ptp_mtp_setobjectproplist (PTPParams* params, MTPProperties *props, int nrofprops);

/* Microsoft MTPZ (Zune) extensions */
uint16_t ptp_mtpz_sendwmdrmpdapprequest (PTPParams*, unsigned char *, uint32_t);
#define  ptp_mtpz_resethandshake(params) ptp_generic_no_data(params, PTP_OC_MTP_WMDRMPD_EndTrustedAppSession, 0)
uint16_t ptp_mtpz_getwmdrmpdappresponse (PTPParams*, unsigned char **, uint32_t*);
#define  ptp_mtpz_wmdrmpd_enabletrustedfilesoperations(params,hash1,hash2,hash3,hash4) \
	 ptp_generic_no_data(params, PTP_OC_MTP_WMDRMPD_EnableTrustedFilesOperations, 4,\
		hash1, hash2, hash3, hash4)

/* Eastman Kodak extensions */
uint16_t ptp_ek_9007 (PTPParams* params, unsigned char **serial, unsigned int *size);
uint16_t ptp_ek_9009 (PTPParams* params, uint32_t*, uint32_t*);
uint16_t ptp_ek_900c (PTPParams* params, unsigned char **serial, unsigned int *size);
uint16_t ptp_ek_getserial (PTPParams* params, unsigned char **serial, unsigned int *size);
uint16_t ptp_ek_setserial (PTPParams* params, unsigned char *serial, unsigned int size);
uint16_t ptp_ek_settext (PTPParams* params, PTPEKTextParams *text);
uint16_t ptp_ek_sendfileobjectinfo (PTPParams* params, uint32_t* store,
				uint32_t* parenthandle, uint32_t* handle,
				PTPObjectInfo* objectinfo);
uint16_t ptp_ek_sendfileobject	(PTPParams* params, unsigned char* object,
				uint32_t size);
uint16_t ptp_ek_sendfileobject_from_handler	(PTPParams* params, PTPDataHandler*,
				uint32_t size);

/* Canon PTP extensions */
#define ptp_canon_9012(params) ptp_generic_no_data(params,0x9012,0)
uint16_t ptp_canon_gettreeinfo (PTPParams* params, uint32_t* out);
uint16_t ptp_canon_gettreesize (PTPParams* params, PTPCanon_directtransfer_entry**, unsigned int*cnt);
uint16_t ptp_canon_getpartialobjectinfo (PTPParams* params, uint32_t handle,
				uint32_t p2, uint32_t* size, uint32_t* rp2);

uint16_t ptp_canon_get_mac_address (PTPParams* params, unsigned char **mac);
/**
 * ptp_canon_startshootingmode:
 * params:      PTPParams*
 *
 * Starts shooting session. It emits a StorageInfoChanged
 * event via the interrupt pipe and pushes the StorageInfoChanged
 * and CANON_CameraModeChange events onto the event stack
 * (see operation PTP_OC_CANON_CheckEvent).
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_startshootingmode(params) ptp_generic_no_data(params,PTP_OC_CANON_InitiateReleaseControl,0)
/**
 * ptp_canon_endshootingmode:
 * params:      PTPParams*
 *
 * This operation is observed after pressing the Disconnect
 * button on the Remote Capture app. It emits a StorageInfoChanged
 * event via the interrupt pipe and pushes the StorageInfoChanged
 * and CANON_CameraModeChange events onto the event stack
 * (see operation PTP_OC_CANON_CheckEvent).
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_endshootingmode(params) ptp_generic_no_data(params,PTP_OC_CANON_TerminateReleaseControl,0)
/**
 * ptp_canon_viewfinderon:
 * params:      PTPParams*
 *
 * Prior to start reading viewfinder images, one  must call this operation.
 * Supposedly, this operation affects the value of the CANON_ViewfinderMode
 * property.
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_viewfinderon(params) ptp_generic_no_data(params,PTP_OC_CANON_ViewfinderOn,0)
/**
 * ptp_canon_viewfinderoff:
 * params:      PTPParams*
 *
 * Before changing the shooting mode, or when one doesn't need to read
 * viewfinder images any more, one must call this operation.
 * Supposedly, this operation affects the value of the CANON_ViewfinderMode
 * property.
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_viewfinderoff(params) ptp_generic_no_data(params,PTP_OC_CANON_ViewfinderOff,0)
/**
 * ptp_canon_reset_aeafawb:
 * params:      PTPParams*
 *              uint32_t flags  - what shall be reset.
 *                      1 - autoexposure
 *                      2 - autofocus
 *                      4 - autowhitebalance
 *
 * Called "Reset AeAfAwb" (auto exposure, focus, white balance)
 *
 * Return values: Some PTP_RC_* code.
 **/
#define PTP_CANON_RESET_AE	0x1
#define PTP_CANON_RESET_AF	0x2
#define PTP_CANON_RESET_AWB	0x4
#define ptp_canon_reset_aeafawb(params,flags) ptp_generic_no_data(params,PTP_OC_CANON_DoAeAfAwb,1,flags)
uint16_t ptp_canon_checkevent (PTPParams* params,
				PTPContainer* event, int* isevent);
/**
 * ptp_canon_focuslock:
 *
 * This operation locks the focus. It is followed by the CANON_GetChanges(?)
 * operation in the log.
 * It affects the CANON_MacroMode property.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_focuslock(params) ptp_generic_no_data(params,PTP_OC_CANON_FocusLock,0)
/**
 * ptp_canon_focusunlock:
 *
 * This operation unlocks the focus. It is followed by the CANON_GetChanges(?)
 * operation in the log.
 * It sets the CANON_MacroMode property value to 1 (where it occurs in the log).
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_focusunlock(params) ptp_generic_no_data(params,PTP_OC_CANON_FocusUnlock,0)
/**
 * ptp_canon_keepdeviceon:
 *
 * This operation sends a "ping" style message to the camera.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_keepdeviceon(params) ptp_generic_no_data(params,PTP_OC_CANON_KeepDeviceOn,0)
/**
 * ptp_canon_eos_keepdeviceon:
 *
 * This operation sends a "ping" style message to the camera.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_keepdeviceon(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_KeepDeviceOn,0)

/**
 * ptp_canon_eos_popupflash:
 *
 * This operation pops up the builtin flash of the Canon EOS.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_popupflash(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_PopupBuiltinFlash,0)
/**
 * ptp_canon_initiatecaptureinmemory:
 *
 * This operation starts the image capture according to the current camera
 * settings. When the capture has happened, the camera emits a CaptureComplete
 * event via the interrupt pipe and pushes the CANON_RequestObjectTransfer,
 * CANON_DeviceInfoChanged and CaptureComplete events onto the event stack
 * (see operation CANON_CheckEvent). From the CANON_RequestObjectTransfer
 * event's parameter one can learn the just captured image's ObjectHandle.
 * The image is stored in the camera's own RAM.
 * On the next capture the image will be overwritten!
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_initiatecaptureinmemory(params) ptp_generic_no_data(params,PTP_OC_CANON_InitiateCaptureInMemory,0)
/**
 * ptp_canon_eos_requestdevicepropvalue:
 *
 * This operation sends a "ping" style message to the camera.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define CANON_EOS_OLC_BUTTON 		0x0001
#define CANON_EOS_OLC_SHUTTERSPEED 	0x0002
#define CANON_EOS_OLC_APERTURE 		0x0004
#define CANON_EOS_OLC_ISO 		0x0008

#define ptp_canon_eos_setrequestolcinfogroup(params,igmask)	ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetRequestOLCInfoGroup,1,igmask)
#define ptp_canon_eos_requestdevicepropvalue(params,prop)	ptp_generic_no_data(params,PTP_OC_CANON_EOS_RequestDevicePropValue,1,prop)
#define ptp_canon_eos_setrequestrollingpitchinglevel(params,onoff)	ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetRequestRollingPitchingLevel,1,onoff)
uint16_t ptp_canon_eos_getremotemode (PTPParams*, uint32_t *);
uint16_t ptp_canon_eos_capture (PTPParams* params, uint32_t *result);
uint16_t ptp_canon_eos_getevent (PTPParams* params, PTPCanon_changes_entry **entries, int *nrofentries);
uint16_t ptp_canon_getpartialobject (PTPParams* params, uint32_t handle,
				uint32_t offset, uint32_t size,
				uint32_t pos, unsigned char** block,
				uint32_t* readnum);
uint16_t ptp_canon_getviewfinderimage (PTPParams* params, unsigned char** image,
				uint32_t* size);
uint16_t ptp_canon_getchanges (PTPParams* params, uint16_t** props,
				uint32_t* propnum);
uint16_t ptp_canon_getobjectinfo (PTPParams* params, uint32_t store,
				uint32_t p2, uint32_t parenthandle,
				uint32_t handle,
				PTPCANONFolderEntry** entries,
				uint32_t* entnum);
uint16_t ptp_canon_eos_getdeviceinfo (PTPParams* params, PTPCanonEOSDeviceInfo*di);
/**
 * ptp_canon_eos_setuilock:
 *
 * This command sets UI lock
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_setuilock(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetUILock,0)
/**
 * ptp_canon_eos_resetuilock:
 *
 * This command sets UI lock
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_resetuilock(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_ResetUILock,0)
/**
 * ptp_canon_eos_start_viewfinder:
 *
 * This command starts Viewfinder mode of newer Canon DSLRs.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_start_viewfinder(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_InitiateViewfinder,0)
/**
 * ptp_canon_eos_end_viewfinder:
 *
 * This command ends Viewfinder mode of newer Canon DSLRs.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_end_viewfinder(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_TerminateViewfinder,0)
uint16_t ptp_canon_eos_get_viewfinder_image (PTPParams* params, unsigned char **data, unsigned int *size);
uint16_t ptp_canon_eos_get_viewfinder_image_handler (PTPParams* params, PTPDataHandler*);
uint16_t ptp_canon_get_objecthandle_by_name (PTPParams* params, char* name, uint32_t* objectid);
uint16_t ptp_canon_get_directory (PTPParams* params, PTPObjectHandles *handles, PTPObjectInfo **oinfos, uint32_t **flags);
/**
 * ptp_canon_setobjectarchive:
 *
 * params:      PTPParams*
 *              uint32_t        objectid
 *              uint32_t        flags
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_setobjectarchive(params,oid,flags) ptp_generic_no_data(params,PTP_OC_CANON_SetObjectArchive,2,oid,flags)
#define ptp_canon_eos_setobjectattributes(params,oid,flags) ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetObjectAttributes,2,oid,flags)
uint16_t ptp_canon_get_customize_data (PTPParams* params, uint32_t themenr,
				unsigned char **data, unsigned int *size);
uint16_t ptp_canon_getpairinginfo (PTPParams* params, uint32_t nr, unsigned char**, unsigned int*);

uint16_t ptp_canon_eos_getstorageids (PTPParams* params, PTPStorageIDs* storageids);
uint16_t ptp_canon_eos_getstorageinfo (PTPParams* params, uint32_t p1, unsigned char**, unsigned int*);
uint16_t ptp_canon_eos_getpartialobject (PTPParams* params, uint32_t oid, uint32_t off, uint32_t xsize, unsigned char**data);
uint16_t ptp_canon_eos_getpartialobjectex (PTPParams* params, uint32_t oid, uint32_t off, uint32_t xsize, unsigned char**data);
uint16_t ptp_canon_eos_getobjectinfoex (PTPParams* params, uint32_t storageid, uint32_t objectid, uint32_t unk,
        PTPCANONFolderEntry **entries, unsigned int *nrofentries);
uint16_t ptp_canon_eos_setdevicepropvalueex (PTPParams* params, unsigned char* data, unsigned int size);
#define ptp_canon_eos_setremotemode(params,p1) ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetRemoteMode,1,p1)
#define ptp_canon_eos_seteventmode(params,p1) ptp_generic_no_data(params,PTP_OC_CANON_EOS_SetEventMode,1,p1)
/**
 * ptp_canon_eos_transfercomplete:
 *
 * This ends a direct object transfer from an EOS camera.
 *
 * params:      PTPParams*
 *              oid             Object ID
 *
 * Return values: Some PTP_RC_* code.
 *
 */
#define ptp_canon_eos_transfercomplete(params,oid) ptp_generic_no_data(params,PTP_OC_CANON_EOS_TransferComplete,1,oid)
/* inHDD = %d, inLength =%d, inReset = %d */
#define ptp_canon_eos_pchddcapacity(params,p1,p2,p3) ptp_generic_no_data(params,PTP_OC_CANON_EOS_PCHDDCapacity,3,p1,p2,p3)
uint16_t ptp_canon_eos_bulbstart (PTPParams* params);
uint16_t ptp_canon_eos_bulbend (PTPParams* params);
uint16_t ptp_canon_eos_905f (PTPParams* params, uint32_t);
uint16_t ptp_canon_eos_getdevicepropdesc (PTPParams* params, uint16_t propcode,
				PTPDevicePropDesc *devicepropertydesc);
uint16_t ptp_canon_eos_setdevicepropvalue (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_nikon_get_vendorpropcodes (PTPParams* params, uint16_t **props, unsigned int *size);
uint16_t ptp_nikon_curve_download (PTPParams* params,
				unsigned char **data, unsigned int *size);
uint16_t ptp_nikon_getlargethumb (PTPParams *params, uint32_t handle,
				unsigned char** object, unsigned int *len);
uint16_t ptp_nikon_getobjectsize (PTPParams* params, uint32_t handle, uint64_t *objectsize);
uint16_t ptp_nikon_getpartialobjectex (PTPParams* params, uint32_t handle, uint64_t offset, uint64_t maxbytes, unsigned char** object, uint32_t *len);
uint16_t ptp_nikon_getptpipinfo (PTPParams* params, unsigned char **data, unsigned int *size);
uint16_t ptp_nikon_getwifiprofilelist (PTPParams* params);
uint16_t ptp_nikon_writewifiprofile (PTPParams* params, PTPNIKONWifiProfile* profile);

uint16_t ptp_sony_sdioconnect (PTPParams* params, uint32_t p1, uint32_t p2, uint32_t p3);
uint16_t ptp_sony_qx_connect (PTPParams* params, uint32_t p1, uint32_t p2, uint32_t p3);
uint16_t ptp_sony_get_vendorpropcodes (PTPParams* params, uint16_t **props, unsigned int *size);
uint16_t ptp_sony_qx_get_vendorpropcodes (PTPParams* params, uint16_t **props, unsigned int *size);
uint16_t ptp_sony_getdevicepropdesc (PTPParams* params, uint16_t propcode,
				PTPDevicePropDesc *devicepropertydesc);
uint16_t ptp_sony_getalldevicepropdesc (PTPParams* params);
uint16_t ptp_sony_qx_getalldevicepropdesc (PTPParams* params);
uint16_t ptp_sony_setdevicecontrolvaluea (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_sony_qx_setdevicecontrolvaluea (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_sony_setdevicecontrolvalueb (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_sony_qx_setdevicecontrolvalueb (PTPParams* params, uint16_t propcode,
                        	PTPPropertyValue* value, uint16_t datatype);
uint16_t ptp_sony_9280 (PTPParams* params, uint32_t additional, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint8_t x, uint8_t y);
uint16_t ptp_sony_9281 (PTPParams* params, uint32_t param1);
/**
 * ptp_nikon_deletewifiprofile:
 *
 * This command deletes a wifi profile.
 *
 * params:      PTPParams*
 *      unsigned int profilenr  - profile number
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_deletewifiprofile(params,profilenr) ptp_generic_no_data(params,PTP_OC_NIKON_DeleteProfile,1,profilenr)
/**
 * ptp_nikon_changecameramode:
 *
 * This command can switch the camera to full PC control mode.
 *
 * params:      PTPParams*
 *      uint32_t mode - mode
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_changecameramode(params,mode) ptp_generic_no_data(params,PTP_OC_NIKON_ChangeCameraMode,1,mode)
/**
 * ptp_nikon_changeapplicationmeramode:
 *
 * This command can switch the camera between PC control and remote mode.
 *
 * params:      PTPParams*
 *      uint32_t mode - mode
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_changeapplicationmode(params,mode) ptp_generic_no_data(params,PTP_OC_NIKON_ChangeApplicationMode,1,mode)
/**
 * ptp_nikon_terminatecapture:
 *
 * This command appears to terminate a longer capture
 *
 * params:      PTPParams*
 *      uint32_t a
 *      uint32_t b
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_terminatecapture(params,p1,p2) ptp_generic_no_data(params,PTP_OC_NIKON_TerminateCapture,2,p1,p2)
/**
 * ptp_nikon_afdrive:
 *
 * This command runs (drives) the lens autofocus.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_afdrive(params) ptp_generic_no_data(params,PTP_OC_NIKON_AfDrive,0)
/**
 * ptp_nikon_changeafarea:
 *
 * This command starts movie capture (to card)
 *
 * params:      PTPParams*
 * x: x coordinate
 * y: y coordinate
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_changeafarea(params,x,y) ptp_generic_no_data(params,PTP_OC_NIKON_ChangeAfArea,2,x,y)
/**
 * ptp_nikon_startmovie:
 *
 * This command starts movie capture (to card)
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_startmovie(params) ptp_generic_no_data(params,PTP_OC_NIKON_StartMovieRecInCard,0)
/**
 * ptp_nikon_stopmovie:
 *
 * This command stops movie capture (to card)
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_stopmovie(params) ptp_generic_no_data(params,PTP_OC_NIKON_EndMovieRec,0)
/**
 * ptp_canon_eos_afdrive:
 *
 * This command runs (drives) the lens autofocus.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_afdrive(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_DoAf,0)
/**
 * ptp_canon_eos_afcancel:
 *
 * This command cancels the lens autofocus.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_afcancel(params) ptp_generic_no_data(params,PTP_OC_CANON_EOS_AfCancel,0)
/**
 * ptp_canon_eos_zoom:
 *
 * This command runs (drives) the lens autofocus.
 *
 * params:      PTPParams*
 * params:      arg1 unknown
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_zoom(params,x) ptp_generic_no_data(params,PTP_OC_CANON_EOS_Zoom,1,x)
#define ptp_canon_eos_zoomposition(params,x,y) ptp_generic_no_data(params,PTP_OC_CANON_EOS_ZoomPosition,2,x,y)

#define ptp_canon_eos_remotereleaseon(params,x,y) ptp_generic_no_data(params,PTP_OC_CANON_EOS_RemoteReleaseOn,2,x,y)
#define ptp_canon_eos_remotereleaseoff(params,x) ptp_generic_no_data(params,PTP_OC_CANON_EOS_RemoteReleaseOff,1,x)
/**
 * ptp_nikon_mfdrive:
 *
 * This command runs (drives) the lens focus manually.
 *
 * params:      PTPParams*
 * flag:        0x1 for (no limit - closest), 0x2 for (closest - no limit)
 * amount:      amount of steps
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_mfdrive(params,flag,amount) ptp_generic_no_data(params,PTP_OC_NIKON_MfDrive,2,flag,amount)

/**
 * ptp_canon_eos_drivelens:
 *
 * This command runs (drives) the lens focus manually.
 *
 * params:      PTPParams*
 * amount:      0x1-0x3 for near range, 0x8001-0x8003 for far range.
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_canon_eos_drivelens(params,amount) ptp_generic_no_data(params,PTP_OC_CANON_EOS_DriveLens,1,amount)
/**
 * ptp_nikon_capture:
 *
 * This command captures a picture on the Nikon.
 *
 * params:      PTPParams*
 *      uint32_t x: unknown parameter. seen to be -1.
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_capture(params,x) ptp_generic_no_data(params,PTP_OC_NIKON_InitiateCaptureRecInSdram,1,x)

/**
 * ptp_nikon_capture2:
 *
 * This command captures a picture on the Nikon.
 *
 * params:      PTPParams*
 * af: 		autofocus before capture (1 yes , 0 no)
 * target:	sdram 1, card 0
 *
 * Return values: Some PTP_RC_* code.
 * 2 params:
 * 0xffffffff == No AF before,  0xfffffffe == AF before capture
 * sdram=1, card=0
 */
#define ptp_nikon_capture2(params,af,target) ptp_generic_no_data(params,PTP_OC_NIKON_InitiateCaptureRecInMedia,2,af?0xfffffffe:0xffffffff,target)
/**
 * ptp_nikon_capture_sdram:
 *
 * This command captures a picture on the Nikon.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_capture_sdram(params) ptp_generic_no_data(params,PTP_OC_NIKON_AfCaptureSDRAM,0)
/**
 * ptp_nikon_delete_sdram_image:
 *
 * This command deletes the current SDRAM image
 *
 * params:      PTPParams*
 * uint32_t	oid
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_delete_sdram_image(params,oid) ptp_generic_no_data(params,PTP_OC_NIKON_DelImageSDRAM,1,oid)
/**
 * ptp_nikon_start_liveview:
 *
 * This command starts LiveView mode of newer Nikons DSLRs.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_start_liveview(params) ptp_generic_no_data(params,PTP_OC_NIKON_StartLiveView,0)
uint16_t ptp_nikon_get_liveview_image (PTPParams* params, unsigned char**,unsigned int*);
uint16_t ptp_nikon_get_preview_image (PTPParams* params, unsigned char**, unsigned int*, uint32_t*);
/**
 * ptp_nikon_end_liveview:
 *
 * This command ends LiveView mode of newer Nikons DSLRs.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_end_liveview(params) ptp_generic_no_data(params,PTP_OC_NIKON_EndLiveView,0)
uint16_t ptp_nikon_check_event (PTPParams* params, PTPContainer **evt, unsigned int *evtcnt);
uint16_t ptp_nikon_check_eventex (PTPParams* params, PTPContainer **evt, unsigned int *evtcnt);
uint16_t ptp_nikon_getfileinfoinblock (PTPParams* params, uint32_t p1, uint32_t p2, uint32_t p3,
					unsigned char **data, unsigned int *size);
/**
 * ptp_nikon_device_ready:
 *
 * This command checks if the device is ready. Used after
 * a capture.
 *
 * params:      PTPParams*
 *
 * Return values: Some PTP_RC_* code.
 *
 **/
#define ptp_nikon_device_ready(params) ptp_generic_no_data (params, PTP_OC_NIKON_DeviceReady, 0)
uint16_t ptp_mtp_getobjectpropssupported (PTPParams* params, uint16_t ofc, uint32_t *propnum, uint16_t **props);


/* Android MTP Extensions */
uint16_t ptp_android_getpartialobject64	(PTPParams* params, uint32_t handle, uint64_t offset,
					uint32_t maxbytes, unsigned char** object,
					uint32_t *len);
#define ptp_android_begineditobject(params,handle) ptp_generic_no_data (params, PTP_OC_ANDROID_BeginEditObject, 1, handle)
#define ptp_android_truncate(params,handle,offset) ptp_generic_no_data (params, PTP_OC_ANDROID_TruncateObject, 3, handle, (offset & 0xFFFFFFFF), (offset >> 32))
uint16_t ptp_android_sendpartialobject (PTPParams *params, uint32_t handle,
					uint64_t offset, unsigned char *object, uint32_t len);
#define ptp_android_endeditobject(params,handle) ptp_generic_no_data (params, PTP_OC_ANDROID_EndEditObject, 1, handle)

uint16_t ptp_olympus_getdeviceinfo (PTPParams*, PTPDeviceInfo*);
#define ptp_olympus_setcameracontrolmode(params,p1) ptp_generic_no_data (params, PTP_OC_OLYMPUS_SetCameraControlMode, 1, p1)
uint16_t ptp_olympus_opensession (PTPParams*, unsigned char**, unsigned int *);
#define ptp_olympus_capture(params,p1) ptp_generic_no_data (params, PTP_OC_OLYMPUS_Capture, 1, p1)
uint16_t ptp_olympus_getcameraid (PTPParams*, unsigned char**, unsigned int *);

uint16_t ptp_olympus_omd_capture (PTPParams* params);
uint16_t ptp_olympus_omd_move_focus (PTPParams* params, uint32_t direction, uint32_t step_size);

/* Non PTP protocol functions */
static inline int
ptp_operation_issupported(PTPParams* params, uint16_t operation)
{
	unsigned int i=0;

	for (;i<params->deviceinfo.OperationsSupported_len;i++) {
		if (params->deviceinfo.OperationsSupported[i]==operation)
			return 1;
	}
	return 0;
}

int ptp_event_issupported	(PTPParams* params, uint16_t event);
int ptp_property_issupported	(PTPParams* params, uint16_t property);

void ptp_free_params		(PTPParams *params);
void ptp_free_objectpropdesc	(PTPObjectPropDesc*);
void ptp_free_devicepropdesc	(PTPDevicePropDesc*);
void ptp_free_devicepropvalue	(uint16_t, PTPPropertyValue*);
void ptp_free_objectinfo	(PTPObjectInfo *oi);
void ptp_free_object		(PTPObject *oi);

const char *ptp_strerror	(uint16_t ret, uint16_t vendor);
void ptp_debug			(PTPParams *params, const char *format, ...);
void ptp_error			(PTPParams *params, const char *format, ...);


const char* ptp_get_property_description(PTPParams* params, uint16_t dpc);

const char* ptp_get_opcode_name(PTPParams* params, uint16_t opcode);
const char* ptp_get_event_code_name(PTPParams* params, uint16_t event_code);

int
ptp_render_property_value(PTPParams* params, uint16_t dpc,
                          PTPDevicePropDesc *dpd, unsigned int length, char *out);
int ptp_render_ofc(PTPParams* params, uint16_t ofc, int spaceleft, char *txt);
int ptp_render_mtp_propname(uint16_t propid, int spaceleft, char *txt);
MTPProperties *ptp_get_new_object_prop_entry(MTPProperties **props, int *nrofprops);
void ptp_destroy_object_prop(MTPProperties *prop);
void ptp_destroy_object_prop_list(MTPProperties *props, int nrofprops);
MTPProperties *ptp_find_object_prop_in_cache(PTPParams *params, uint32_t const handle, uint32_t const attribute_id);
uint16_t ptp_remove_object_from_cache(PTPParams *params, uint32_t handle);
uint16_t ptp_add_object_to_cache(PTPParams *params, uint32_t handle);
uint16_t ptp_object_want (PTPParams *, uint32_t handle, unsigned int want, PTPObject**retob);
void ptp_objects_sort (PTPParams *);
uint16_t ptp_object_find (PTPParams *params, uint32_t handle, PTPObject **retob);
uint16_t ptp_object_find_or_insert (PTPParams *params, uint32_t handle, PTPObject **retob);
uint16_t ptp_list_folder (PTPParams *params, uint32_t storage, uint32_t handle);
/* ptpip.c */
void ptp_nikon_getptpipguid (unsigned char* guid);

/* CHDK specifics */
#define PTP_OC_CHDK	0x9999
typedef struct tagptp_chdk_videosettings {
	long live_image_buffer_width;
	long live_image_width;
	long live_image_height;
	long bitmap_buffer_width;
	long bitmap_width;
	long bitmap_height;
	unsigned palette[16];
} ptp_chdk_videosettings;

/* Nafraf: Test this!!!*/
#define ptp_chdk_switch_mode(params,mode) ptp_generic_no_data(params,PTP_OC_CHDK,2,PTP_CHDK_SwitchMode,mode)

/* include CHDK ptp protocol definitions from a CHDK source tree */
#include "chdk_ptp.h"
#if (PTP_CHDK_VERSION_MAJOR < 2 || (PTP_CHDK_VERSION_MAJOR == 2 && PTP_CHDK_VERSION_MINOR < 5))
#error your chdk headers are too old, unset CHDK_SRC_DIR in config.mk
#endif
#include "chdk_live_view.h"

/* the following happens to match what is used in CHDK, but is not part of the protocol */
typedef struct {
    unsigned size;
    unsigned script_id; /* id of script message is to/from  */
    unsigned type;
    unsigned subtype;
    char data[];
} ptp_chdk_script_msg;

/*
chunk for remote capture
*/
typedef struct {
	uint32_t size; /* length of data */
	int last; /* is it the last chunk? */
	uint32_t offset; /* offset within file, or -1 */
	unsigned char *data; /* data, must be free'd by caller when done */
} ptp_chdk_rc_chunk;


uint16_t ptp_chdk_get_memory(PTPParams* params, int start, int num, unsigned char **);
uint16_t ptp_chdk_set_memory_long(PTPParams* params, int addr, int val);
int ptp_chdk_upload(PTPParams* params, char *local_fn, char *remote_fn);
uint16_t ptp_chdk_download(PTPParams* params, char *remote_fn, PTPDataHandler *handler);

/* remote capture */
uint16_t ptp_chdk_rcisready(PTPParams* params, int *isready,int *imgnum);
uint16_t ptp_chdk_rcgetchunk(PTPParams* params,int fmt, ptp_chdk_rc_chunk *chunk);

uint16_t ptp_chdk_exec_lua(PTPParams* params, char *script, int flags, int *script_id,int *status);
uint16_t ptp_chdk_get_version(PTPParams* params, int *major, int *minor);
uint16_t ptp_chdk_get_script_support(PTPParams* params, unsigned *status);
uint16_t ptp_chdk_get_script_status(PTPParams* params, unsigned *status);
uint16_t ptp_chdk_write_script_msg(PTPParams* params, char *data, unsigned size, int target_script_id, int *status);
uint16_t ptp_chdk_read_script_msg(PTPParams* params, ptp_chdk_script_msg **msg);
uint16_t ptp_chdk_get_live_data(PTPParams* params, unsigned flags, unsigned char **data, unsigned int *data_size);
uint16_t ptp_chdk_parse_live_data (PTPParams* params, unsigned char *data, unsigned int data_size,
				   lv_data_header *header, lv_framebuffer_desc *vpd, lv_framebuffer_desc *bmd);
uint16_t ptp_chdk_call_function(PTPParams* params, int *args, int size, int *ret);

/*uint16_t ptp_chdk_get_script_output(PTPParams* params, char **output ); */
/*uint16_t ptp_chdk_get_video_settings(PTPParams* params, ptp_chdk_videosettings* vsettings);*/

uint16_t ptp_fuji_getevents (PTPParams* params, uint16_t** events, uint16_t* count);
uint16_t ptp_fuji_getdeviceinfo (PTPParams* params, uint16_t **props, unsigned int *numprops);

#define ptp_panasonic_liveview(params,enable) ptp_generic_no_data(params,PTP_OC_PANASONIC_Liveview,1,enable?0xD000010:0xD000011)
uint16_t ptp_panasonic_liveview_image (PTPParams* params, unsigned char **data, unsigned int *size);

uint16_t ptp_panasonic_setdeviceproperty (PTPParams* params, uint32_t propcode, unsigned char *value, uint16_t valuesize);
uint16_t ptp_panasonic_getdeviceproperty (PTPParams *params, uint32_t propcode, uint16_t *valuesize, uint32_t *currentValue);
uint16_t ptp_panasonic_getdevicepropertydesc (PTPParams *params, uint32_t propcode, uint16_t valuesize, uint32_t *currentValue, uint32_t **propertyValueList, uint32_t *propertyValueListLength);
uint16_t ptp_panasonic_getdevicepropertysize (PTPParams *params, uint32_t propcode);
uint16_t ptp_panasonic_setcapturetarget (PTPParams *params, uint16_t mode);
uint16_t ptp_panasonic_manualfocusdrive (PTPParams* params, uint16_t mode);
uint16_t ptp_panasonic_9401 (PTPParams* params, uint32_t x);

uint16_t ptp_olympus_liveview_image (PTPParams* params, unsigned char **data, unsigned int *size);
#define ptp_olympus_omd_move_focus(params,direction,step_size) ptp_generic_no_data(params,PTP_OC_OLYMPUS_OMD_MFDrive,2,direction,step_size)
uint16_t ptp_olympus_omd_capture (PTPParams* params);
uint16_t ptp_olympus_omd_bulbstart (PTPParams* params);
uint16_t ptp_olympus_omd_bulbend (PTPParams* params);
uint16_t ptp_olympus_init_pc_mode (PTPParams* params);
uint16_t ptp_olympus_sdram_image (PTPParams* params, unsigned char **data, unsigned int *size);



#define ptp_panasonic_capture(params) ptp_generic_no_data(params,PTP_OC_PANASONIC_InitiateCapture,1,0x3000011)

#define ptp_leica_leopensession(params,session) ptp_generic_no_data(params,PTP_OC_LEICA_LEOpenSession,1,session)
#define ptp_leica_leclosesession(params) ptp_generic_no_data(params,PTP_OC_LEICA_LECloseSession,0)
uint16_t ptp_leica_getstreamdata (PTPParams* params, unsigned char **data, unsigned int *size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PTP_H__ */
