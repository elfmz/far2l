/**
 * \file libusb-glue.h
 * Low-level USB interface glue towards libusb.
 *
 * Copyright (C) 2005-2007 Richard A. Low <richard@wentnet.com>
 * Copyright (C) 2005-2012 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2006-2011 Marcus Meissner
 * Copyright (C) 2007 Ted Bullock
 * Copyright (C) 2008 Chris Bagwell <chris@cnpbagwell.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Created by Richard Low on 24/12/2005.
 * Modified by Linus Walleij
 *
 */
#ifndef LIBUSB_GLUE_H
#define LIBUSB_GLUE_H

#include "ptp.h"
#ifdef HAVE_LIBUSB1
#include <libusb.h>
#endif
#ifdef HAVE_LIBUSB0
#include <usb.h>
#endif
#ifdef HAVE_LIBOPENUSB
#include <openusb.h>
#endif
#include "libmtp.h"
#include "device-flags.h"

/* Make functions available for C++ */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Debug macro
 */
#define LIBMTP_USB_DEBUG(format, args...) \
  do { \
    if ((LIBMTP_debug & LIBMTP_DEBUG_USB) != 0) \
      fprintf(stdout, "LIBMTP %s[%d]: " format, __FUNCTION__, __LINE__, ##args); \
  } while (0)

#define LIBMTP_USB_DATA(buffer, length, base) \
  do { \
    if ((LIBMTP_debug & LIBMTP_DEBUG_DATA) != 0) \
      data_dump_ascii (stdout, buffer, length, base); \
  } while (0)

#ifdef HAVE_LIBUSB1
#define USB_BULK_READ libusb_bulk_transfer
#define USB_BULK_WRITE libusb_bulk_transfer
#endif
#ifdef HAVE_LIBUSB0
#define USB_BULK_READ usb_bulk_read
#define USB_BULK_WRITE usb_bulk_write
#endif
#ifdef HAVE_LIBOPENUSB
#define USB_BULK_READ openusb_bulk_xfer
#define USB_BULK_WRITE openusb_bulk_xfer
#endif

/**
 * Internal USB struct.
 */
typedef struct _PTP_USB PTP_USB;
struct _PTP_USB {
  PTPParams *params;
#ifdef HAVE_LIBUSB1
  libusb_device_handle* handle;
#endif
#ifdef HAVE_LIBUSB0
  usb_dev_handle* handle;
#endif
#ifdef HAVE_LIBOPENUSB
  openusb_dev_handle_t* handle;
#endif
  uint8_t config;
  uint8_t interface;
  uint8_t altsetting;
  int inep;
  int inep_maxpacket;
  int outep;
  int outep_maxpacket;
  int intep;
  /** File transfer callbacks and counters */
  int callback_active;
  int timeout;
  uint16_t bcdusb;
  uint64_t current_transfer_total;
  uint64_t current_transfer_complete;
  LIBMTP_progressfunc_t current_transfer_callback;
  void const * current_transfer_callback_data;
  /** Any special device flags, only used internally */
  LIBMTP_raw_device_t rawdevice;
};

void dump_usbinfo(PTP_USB *ptp_usb);
const char *get_playlist_extension(PTP_USB *ptp_usb);
void close_device(PTP_USB *ptp_usb, PTPParams *params);
LIBMTP_error_number_t configure_usb_device(LIBMTP_raw_device_t *device,
					   PTPParams *params,
					   void **usbinfo);
void set_usb_device_timeout(PTP_USB *ptp_usb, int timeout);
void get_usb_device_timeout(PTP_USB *ptp_usb, int *timeout);
int guess_usb_speed(PTP_USB *ptp_usb);

/* Flag check macros */
#define FLAG_BROKEN_MTPGETOBJPROPLIST_ALL(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST_ALL)
#define FLAG_UNLOAD_DRIVER(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_UNLOAD_DRIVER)
#define FLAG_BROKEN_MTPGETOBJPROPLIST(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST)
#define FLAG_NO_ZERO_READS(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_NO_ZERO_READS)
#define FLAG_IRIVER_OGG_ALZHEIMER(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_IRIVER_OGG_ALZHEIMER)
#define FLAG_ONLY_7BIT_FILENAMES(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_ONLY_7BIT_FILENAMES)
#define FLAG_NO_RELEASE_INTERFACE(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_NO_RELEASE_INTERFACE)
#define FLAG_IGNORE_HEADER_ERRORS(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_IGNORE_HEADER_ERRORS)
#define FLAG_BROKEN_SET_OBJECT_PROPLIST(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_SET_OBJECT_PROPLIST)
#define FLAG_OGG_IS_UNKNOWN(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_OGG_IS_UNKNOWN)
#define FLAG_BROKEN_SET_SAMPLE_DIMENSIONS(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_SET_SAMPLE_DIMENSIONS)
#define FLAG_ALWAYS_PROBE_DESCRIPTOR(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_ALWAYS_PROBE_DESCRIPTOR)
#define FLAG_PLAYLIST_SPL_V1(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_PLAYLIST_SPL_V1)
#define FLAG_PLAYLIST_SPL_V2(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_PLAYLIST_SPL_V2)
#define FLAG_PLAYLIST_SPL(a) \
  ((a)->rawdevice.device_entry.device_flags & (DEVICE_FLAG_PLAYLIST_SPL_V1 | DEVICE_FLAG_PLAYLIST_SPL_V2))
#define FLAG_CANNOT_HANDLE_DATEMODIFIED(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_CANNOT_HANDLE_DATEMODIFIED)
#define FLAG_BROKEN_SEND_OBJECT_PROPLIST(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_SEND_OBJECT_PROPLIST)
#define FLAG_BROKEN_BATTERY_LEVEL(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_BROKEN_BATTERY_LEVEL)
#define FLAG_FLAC_IS_UNKNOWN(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_FLAC_IS_UNKNOWN)
#define FLAG_UNIQUE_FILENAMES(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_UNIQUE_FILENAMES)
#define FLAG_SWITCH_MODE_BLACKBERRY(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_SWITCH_MODE_BLACKBERRY)
#define FLAG_LONG_TIMEOUT(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_LONG_TIMEOUT)
#define FLAG_FORCE_RESET_ON_CLOSE(a) \
  ((a)->rawdevice.device_entry.device_flags & DEVICE_FLAG_FORCE_RESET_ON_CLOSE)

/* connect_first_device return codes */
#define PTP_CD_RC_CONNECTED	0
#define PTP_CD_RC_NO_DEVICES	1
#define PTP_CD_RC_ERROR_CONNECTING	2

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //  LIBUSB-GLUE_H
