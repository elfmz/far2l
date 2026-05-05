/**
 * \file libmtp.c
 *
 * Copyright (C) 2005-2011 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2005-2008 Richard A. Low <richard@wentnet.com>
 * Copyright (C) 2007 Ted Bullock <tbullock@canada.com>
 * Copyright (C) 2007 Tero Saarni <tero.saarni@gmail.com>
 * Copyright (C) 2008 Florent Mertens <flomertens@gmail.com>
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
 * This file provides an interface "glue" to the underlying
 * PTP implementation from libgphoto2. It uses some local
 * code to convert from/to UTF-8 (stored in unicode.c/.h)
 * and some small utility functions, mainly for debugging
 * (stored in util.c/.h).
 *
 * The three PTP files (ptp.c, ptp.h and ptp-pack.c) are
 * plain copied from the libhphoto2 codebase.
 *
 * The files libusb-glue.c/.h are just what they say: an
 * interface to libusb for the actual, physical USB traffic.
 */
#include "config.h"
#include "libmtp.h"
#include "unicode.h"
#include "ptp.h"
#include "libusb-glue.h"
#include "device-flags.h"
#include "playlist-spl.h"
#include "util.h"

#include "mtpz.h"
int use_mtpz;

#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifdef _MSC_VER // For MSVC++
#define USE_WINDOWS_IO_H
#include <io.h>
#endif


/**
 * Global debug level
 * We use a flag system to enable a part of logs.
 *
 * The LIBMTP_DEBUG environment variable sets the debug flags for any binary
 * that uses libmtp and calls LIBMTP_Init. The value can be given in decimal
 * (must not start with "0" or it will be interpreted in octal), or in
 * hexadecimal (must start with "0x").
 *
 * The value "-1" enables all debug flags.
 *
 * Some of the utilities in examples/ also take a command-line flag "-d" that
 * enables LIBMTP_DEBUG_PTP and LIBMTP_DEBUG_DATA (same as setting
 * LIBMTP_DEBUG=9).
 *
 * Flags (combine by adding the hex values):
 *  0x00 [0000 0000] : LIBMTP_DEBUG_NONE  : no debug (default)
 *  0x01 [0000 0001] : LIBMTP_DEBUG_PTP   : PTP debug
 *  0x02 [0000 0010] : LIBMTP_DEBUG_PLST  : Playlist debug
 *  0x04 [0000 0100] : LIBMTP_DEBUG_USB   : USB debug
 *  0x08 [0000 1000] : LIBMTP_DEBUG_DATA  : USB data debug
 *
 * (Please keep this list in sync with libmtp.h.)
 */
int LIBMTP_debug = LIBMTP_DEBUG_NONE;


/*
 * This is a mapping between libmtp internal MTP filetypes and
 * the libgphoto2/PTP equivalent defines. We need this because
 * otherwise the libmtp.h device has to be dependent on ptp.h
 * to be installed too, and we don't want that.
 */
//typedef struct filemap_struct filemap_t;
typedef struct filemap_struct {
  char *description; /**< Text description for the file type */
  LIBMTP_filetype_t id; /**< LIBMTP internal type for the file type */
  uint16_t ptp_id; /**< PTP ID for the filetype */
  struct filemap_struct *next;
} filemap_t;

/*
 * This is a mapping between libmtp internal MTP properties and
 * the libgphoto2/PTP equivalent defines. We need this because
 * otherwise the libmtp.h device has to be dependent on ptp.h
 * to be installed too, and we don't want that.
 */
typedef struct propertymap_struct {
  char *description; /**< Text description for the property */
  LIBMTP_property_t id; /**< LIBMTP internal type for the property */
  uint16_t ptp_id; /**< PTP ID for the property */
  struct propertymap_struct *next;
} propertymap_t;

/*
 * This is a simple container for holding our callback and user_data
 * for parsing onwards to the usb_event_async function.
 */
typedef struct event_cb_data_struct {
  LIBMTP_event_cb_fn cb;
  void *user_data;
} event_cb_data_t;

// Global variables
// This holds the global filetype mapping table
static filemap_t *g_filemap = NULL;
// This holds the global property mapping table
static propertymap_t *g_propertymap = NULL;

/*
 * Forward declarations of local (static) functions.
 */
static int register_filetype(char const * const description, LIBMTP_filetype_t const id,
			     uint16_t const ptp_id);
static void init_filemap();
static int register_property(char const * const description, LIBMTP_property_t const id,
			     uint16_t const ptp_id);
static void init_propertymap();
static void add_error_to_errorstack(LIBMTP_mtpdevice_t *device,
				    LIBMTP_error_number_t errornumber,
				    char const * const error_text);
static void add_ptp_error_to_errorstack(LIBMTP_mtpdevice_t *device,
					uint16_t ptp_error,
					char const * const error_text);
static void flush_handles(LIBMTP_mtpdevice_t *device);
static uint16_t get_handles_recursively(LIBMTP_mtpdevice_t *device,
				    PTPParams *params,
				    uint32_t storageid,
				    uint32_t parent);
static void free_storage_list(LIBMTP_mtpdevice_t *device);
static int sort_storage_by(LIBMTP_mtpdevice_t *device, int const sortby);
static uint32_t get_writeable_storageid(LIBMTP_mtpdevice_t *device,
					uint64_t fitsize);
static int get_storage_freespace(LIBMTP_mtpdevice_t *device,
				 LIBMTP_devicestorage_t *storage,
				 uint64_t *freespace);
static int check_if_file_fits(LIBMTP_mtpdevice_t *device,
			      LIBMTP_devicestorage_t *storage,
			      uint64_t const filesize);
static uint16_t map_libmtp_type_to_ptp_type(LIBMTP_filetype_t intype);
static LIBMTP_filetype_t map_ptp_type_to_libmtp_type(uint16_t intype);
static uint16_t map_libmtp_property_to_ptp_property(LIBMTP_property_t inproperty);
static LIBMTP_property_t map_ptp_property_to_libmtp_property(uint16_t intype);
static int get_device_unicode_property(LIBMTP_mtpdevice_t *device,
				       char **unicstring, uint16_t property);
static uint16_t adjust_u16(uint16_t val, PTPObjectPropDesc *opd);
static uint32_t adjust_u32(uint32_t val, PTPObjectPropDesc *opd);
static char *get_iso8601_stamp(void);
static char *get_string_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    uint16_t const attribute_id);
static uint64_t get_u64_from_object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
                                    uint16_t const attribute_id, uint64_t const value_default);
static uint32_t get_u32_from_object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
				    uint16_t const attribute_id, uint32_t const value_default);
static uint16_t get_u16_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    uint16_t const attribute_id, uint16_t const value_default);
static uint8_t get_u8_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				  uint16_t const attribute_id, uint8_t const value_default);
static int set_object_string(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			     uint16_t const attribute_id, char const * const string);
static int set_object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  uint16_t const attribute_id, uint32_t const value);
static int set_object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  uint16_t const attribute_id, uint16_t const value);
static int set_object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			 uint16_t const attribute_id, uint8_t const value);
static void get_track_metadata(LIBMTP_mtpdevice_t *device, uint16_t objectformat,
			       LIBMTP_track_t *track);
static LIBMTP_folder_t *get_subfolders_for_folder(LIBMTP_folder_t *list, uint32_t parent);
static int create_new_abstract_list(LIBMTP_mtpdevice_t *device,
				    char const * const name,
				    char const * const artist,
				    char const * const composer,
				    char const * const genre,
				    uint32_t const parenthandle,
				    uint32_t const storageid,
				    uint16_t const objectformat,
				    char const * const suffix,
				    uint32_t * const newid,
				    uint32_t const * const tracks,
				    uint32_t const no_tracks);
static int update_abstract_list(LIBMTP_mtpdevice_t *device,
				char const * const name,
				char const * const artist,
				char const * const composer,
				char const * const genre,
				uint32_t const objecthandle,
				uint16_t const objectformat,
				uint32_t const * const tracks,
				uint32_t const no_tracks);
static int send_file_object_info(LIBMTP_mtpdevice_t *device, LIBMTP_file_t *filedata);
static void add_object_to_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id);
static void update_metadata_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id);
static int set_object_filename(LIBMTP_mtpdevice_t *device,
		uint32_t object_id,
		uint16_t ptp_type,
                const char **newname);
static char *generate_unique_filename(PTPParams* params, char const * const filename);
static int check_filename_exists(PTPParams* params, char const * const filename);
static void LIBMTP_Handle_Event(PTPContainer *ptp_event,
                                LIBMTP_event_t *event, uint32_t *out1);

/**
 * These are to wrap the get/put handlers to convert from the MTP types to PTP types
 * in a reliable way
 */
typedef struct _MTPDataHandler {
	MTPDataGetFunc		getfunc;
	MTPDataPutFunc		putfunc;
	void			*priv;
} MTPDataHandler;

static uint16_t get_func_wrapper(PTPParams* params, void* priv, unsigned long wantlen, unsigned char *data, unsigned long *gotlen);
static uint16_t put_func_wrapper(PTPParams* params, void* priv, unsigned long sendlen, unsigned char *data);

/**
 * Checks if a filename ends with ".ogg". Used in various
 * situations when the device has no idea that it support
 * OGG but still does.
 *
 * @param name string to be checked.
 * @return 0 if this does not end with ogg, any other
 *           value means it does.
 */
static int has_ogg_extension(char *name) {
  char *ptype;

  if (name == NULL)
    return 0;
  ptype = strrchr(name,'.');
  if (ptype == NULL)
    return 0;
  if (!strcasecmp (ptype, ".ogg"))
    return 1;
  return 0;
}

/**
 * Checks if a filename ends with ".flac". Used in various
 * situations when the device has no idea that it support
 * FLAC but still does.
 *
 * @param name string to be checked.
 * @return 0 if this does not end with flac, any other
 *           value means it does.
 */
static int has_flac_extension(char *name) {
  char *ptype;

  if (name == NULL)
    return 0;
  ptype = strrchr(name,'.');
  if (ptype == NULL)
    return 0;
  if (!strcasecmp (ptype, ".flac"))
    return 1;
  return 0;
}



/**
 * Create a new file mapping entry
 * @return a newly allocated filemapping entry.
 */
static filemap_t *new_filemap_entry()
{
  filemap_t *filemap;

  filemap = (filemap_t *)malloc(sizeof(filemap_t));

  if( filemap != NULL ) {
    filemap->description = NULL;
    filemap->id = LIBMTP_FILETYPE_UNKNOWN;
    filemap->ptp_id = PTP_OFC_Undefined;
    filemap->next = NULL;
  }

  return filemap;
}

/**
 * Register an MTP or PTP filetype for data retrieval
 *
 * @param description Text description of filetype
 * @param id libmtp internal filetype id
 * @param ptp_id PTP filetype id
 * @return 0 for success any other value means error.
*/
static int register_filetype(char const * const description, LIBMTP_filetype_t const id,
			     uint16_t const ptp_id)
{
  filemap_t *new = NULL, *current;

  // Has this LIBMTP filetype been registered before ?
  current = g_filemap;
  while (current != NULL) {
    if(current->id == id) {
      break;
    }
    current = current->next;
  }

  // Create the entry
  if(current == NULL) {
    new = new_filemap_entry();
    if(new == NULL) {
      return 1;
    }

    new->id = id;
    if(description != NULL) {
      new->description = strdup(description);
    }
    new->ptp_id = ptp_id;

    // Add the entry to the list
    if(g_filemap == NULL) {
      g_filemap = new;
    } else {
      current = g_filemap;
      while (current->next != NULL ) current=current->next;
      current->next = new;
    }
    // Update the existing entry
  } else {
    if (current->description != NULL) {
      free(current->description);
    }
    current->description = NULL;
    if(description != NULL) {
      current->description = strdup(description);
    }
    current->ptp_id = ptp_id;
  }

  return 0;
}

static void init_filemap()
{
  register_filetype("Folder", LIBMTP_FILETYPE_FOLDER, PTP_OFC_Association);
  register_filetype("MediaCard", LIBMTP_FILETYPE_MEDIACARD, PTP_OFC_MTP_MediaCard);
  register_filetype("RIFF WAVE file", LIBMTP_FILETYPE_WAV, PTP_OFC_WAV);
  register_filetype("ISO MPEG-1 Audio Layer 3", LIBMTP_FILETYPE_MP3, PTP_OFC_MP3);
  register_filetype("ISO MPEG-1 Audio Layer 2", LIBMTP_FILETYPE_MP2, PTP_OFC_MTP_MP2);
  register_filetype("Microsoft Windows Media Audio", LIBMTP_FILETYPE_WMA, PTP_OFC_MTP_WMA);
  register_filetype("Ogg container format", LIBMTP_FILETYPE_OGG, PTP_OFC_MTP_OGG);
  register_filetype("Free Lossless Audio Codec (FLAC)", LIBMTP_FILETYPE_FLAC, PTP_OFC_MTP_FLAC);
  register_filetype("Advanced Audio Coding (AAC)/MPEG-2 Part 7/MPEG-4 Part 3", LIBMTP_FILETYPE_AAC, PTP_OFC_MTP_AAC);
  register_filetype("MPEG-4 Part 14 Container Format (Audio Emphasis)", LIBMTP_FILETYPE_M4A, PTP_OFC_MTP_M4A);
  register_filetype("MPEG-4 Part 14 Container Format (Audio+Video Emphasis)", LIBMTP_FILETYPE_MP4, PTP_OFC_MTP_MP4);
  register_filetype("Audible.com Audio Codec", LIBMTP_FILETYPE_AUDIBLE, PTP_OFC_MTP_AudibleCodec);
  register_filetype("Undefined audio file", LIBMTP_FILETYPE_UNDEF_AUDIO, PTP_OFC_MTP_UndefinedAudio);
  register_filetype("Microsoft Windows Media Video", LIBMTP_FILETYPE_WMV, PTP_OFC_MTP_WMV);
  register_filetype("Audio Video Interleave", LIBMTP_FILETYPE_AVI, PTP_OFC_AVI);
  register_filetype("MPEG video stream", LIBMTP_FILETYPE_MPEG, PTP_OFC_MPEG);
  register_filetype("Microsoft Advanced Systems Format", LIBMTP_FILETYPE_ASF, PTP_OFC_ASF);
  register_filetype("Apple Quicktime container format", LIBMTP_FILETYPE_QT, PTP_OFC_QT);
  register_filetype("Undefined video file", LIBMTP_FILETYPE_UNDEF_VIDEO, PTP_OFC_MTP_UndefinedVideo);
  register_filetype("JPEG file", LIBMTP_FILETYPE_JPEG, PTP_OFC_EXIF_JPEG);
  register_filetype("JP2 file", LIBMTP_FILETYPE_JP2, PTP_OFC_JP2);
  register_filetype("JPX file", LIBMTP_FILETYPE_JPX, PTP_OFC_JPX);
  register_filetype("JFIF file", LIBMTP_FILETYPE_JFIF, PTP_OFC_JFIF);
  register_filetype("TIFF bitmap file", LIBMTP_FILETYPE_TIFF, PTP_OFC_TIFF);
  register_filetype("BMP bitmap file", LIBMTP_FILETYPE_BMP, PTP_OFC_BMP);
  register_filetype("GIF bitmap file", LIBMTP_FILETYPE_GIF, PTP_OFC_GIF);
  register_filetype("PICT bitmap file", LIBMTP_FILETYPE_PICT, PTP_OFC_PICT);
  register_filetype("Portable Network Graphics", LIBMTP_FILETYPE_PNG, PTP_OFC_PNG);
  register_filetype("Microsoft Windows Image Format", LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT, PTP_OFC_MTP_WindowsImageFormat);
  register_filetype("VCalendar version 1", LIBMTP_FILETYPE_VCALENDAR1, PTP_OFC_MTP_vCalendar1);
  register_filetype("VCalendar version 2", LIBMTP_FILETYPE_VCALENDAR2, PTP_OFC_MTP_vCalendar2);
  register_filetype("VCard version 2", LIBMTP_FILETYPE_VCARD2, PTP_OFC_MTP_vCard2);
  register_filetype("VCard version 3", LIBMTP_FILETYPE_VCARD3, PTP_OFC_MTP_vCard3);
  register_filetype("Undefined Windows executable file", LIBMTP_FILETYPE_WINEXEC, PTP_OFC_MTP_UndefinedWindowsExecutable);
  register_filetype("Text file", LIBMTP_FILETYPE_TEXT, PTP_OFC_Text);
  register_filetype("HTML file", LIBMTP_FILETYPE_HTML, PTP_OFC_HTML);
  register_filetype("XML file", LIBMTP_FILETYPE_XML, PTP_OFC_MTP_XMLDocument);
  register_filetype("DOC file", LIBMTP_FILETYPE_DOC, PTP_OFC_MTP_MSWordDocument);
  register_filetype("XLS file", LIBMTP_FILETYPE_XLS, PTP_OFC_MTP_MSExcelSpreadsheetXLS);
  register_filetype("PPT file", LIBMTP_FILETYPE_PPT, PTP_OFC_MTP_MSPowerpointPresentationPPT);
  register_filetype("MHT file", LIBMTP_FILETYPE_MHT, PTP_OFC_MTP_MHTCompiledHTMLDocument);
  register_filetype("Firmware file", LIBMTP_FILETYPE_FIRMWARE, PTP_OFC_MTP_Firmware);
  register_filetype("Abstract Album file", LIBMTP_FILETYPE_ALBUM, PTP_OFC_MTP_AbstractAudioAlbum);
  register_filetype("Abstract Playlist file", LIBMTP_FILETYPE_PLAYLIST, PTP_OFC_MTP_AbstractAudioVideoPlaylist);
  register_filetype("Undefined filetype", LIBMTP_FILETYPE_UNKNOWN, PTP_OFC_Undefined);
}

/**
 * Returns the PTP filetype that maps to a certain libmtp internal file type.
 * @param intype the MTP library interface type
 * @return the PTP (libgphoto2) interface type
 */
static uint16_t map_libmtp_type_to_ptp_type(LIBMTP_filetype_t intype)
{
  filemap_t *current;

  current = g_filemap;

  while (current != NULL) {
    if(current->id == intype) {
      return current->ptp_id;
    }
    current = current->next;
  }
  // printf("map_libmtp_type_to_ptp_type: unknown filetype.\n");
  return PTP_OFC_Undefined;
}


/**
 * Returns the MTP internal interface type that maps to a certain ptp
 * interface type.
 * @param intype the PTP (libgphoto2) interface type
 * @return the MTP library interface type
 */
static LIBMTP_filetype_t map_ptp_type_to_libmtp_type(uint16_t intype)
{
  filemap_t *current;

  current = g_filemap;

  while (current != NULL) {
    if(current->ptp_id == intype) {
      return current->id;
    }
    current = current->next;
  }
  // printf("map_ptp_type_to_libmtp_type: unknown filetype.\n");
  return LIBMTP_FILETYPE_UNKNOWN;
}

/**
 * Create a new property mapping entry
 * @return a newly allocated propertymapping entry.
 */
static propertymap_t *new_propertymap_entry()
{
  propertymap_t *propertymap;

  propertymap = (propertymap_t *)malloc(sizeof(propertymap_t));

  if( propertymap != NULL ) {
    propertymap->description = NULL;
    propertymap->id = LIBMTP_PROPERTY_UNKNOWN;
    propertymap->ptp_id = 0;
    propertymap->next = NULL;
  }

  return propertymap;
}

/**
 * Register an MTP or PTP property for data retrieval
 *
 * @param description Text description of property
 * @param id libmtp internal property id
 * @param ptp_id PTP property id
 * @return 0 for success any other value means error.
*/
static int register_property(char const * const description, LIBMTP_property_t const id,
			     uint16_t const ptp_id)
{
  propertymap_t *new = NULL, *current;

  // Has this LIBMTP propety been registered before ?
  current = g_propertymap;
  while (current != NULL) {
    if(current->id == id) {
      break;
    }
    current = current->next;
  }

  // Create the entry
  if(current == NULL) {
    new = new_propertymap_entry();
    if(new == NULL) {
      return 1;
    }

    new->id = id;
    if(description != NULL) {
      new->description = strdup(description);
    }
    new->ptp_id = ptp_id;

    // Add the entry to the list
    if(g_propertymap == NULL) {
      g_propertymap = new;
    } else {
      current = g_propertymap;
      while (current->next != NULL ) current=current->next;
      current->next = new;
    }
    // Update the existing entry
  } else {
    if (current->description != NULL) {
      free(current->description);
    }
    current->description = NULL;
    if(description != NULL) {
      current->description = strdup(description);
    }
    current->ptp_id = ptp_id;
  }

  return 0;
}

static void init_propertymap()
{
  register_property("Storage ID", LIBMTP_PROPERTY_StorageID, PTP_OPC_StorageID);
  register_property("Object Format", LIBMTP_PROPERTY_ObjectFormat, PTP_OPC_ObjectFormat);
  register_property("Protection Status", LIBMTP_PROPERTY_ProtectionStatus, PTP_OPC_ProtectionStatus);
  register_property("Object Size", LIBMTP_PROPERTY_ObjectSize, PTP_OPC_ObjectSize);
  register_property("Association Type", LIBMTP_PROPERTY_AssociationType, PTP_OPC_AssociationType);
  register_property("Association Desc", LIBMTP_PROPERTY_AssociationDesc, PTP_OPC_AssociationDesc);
  register_property("Object File Name", LIBMTP_PROPERTY_ObjectFileName, PTP_OPC_ObjectFileName);
  register_property("Date Created", LIBMTP_PROPERTY_DateCreated, PTP_OPC_DateCreated);
  register_property("Date Modified", LIBMTP_PROPERTY_DateModified, PTP_OPC_DateModified);
  register_property("Keywords", LIBMTP_PROPERTY_Keywords, PTP_OPC_Keywords);
  register_property("Parent Object", LIBMTP_PROPERTY_ParentObject, PTP_OPC_ParentObject);
  register_property("Allowed Folder Contents", LIBMTP_PROPERTY_AllowedFolderContents, PTP_OPC_AllowedFolderContents);
  register_property("Hidden", LIBMTP_PROPERTY_Hidden, PTP_OPC_Hidden);
  register_property("System Object", LIBMTP_PROPERTY_SystemObject, PTP_OPC_SystemObject);
  register_property("Persistant Unique Object Identifier", LIBMTP_PROPERTY_PersistantUniqueObjectIdentifier, PTP_OPC_PersistantUniqueObjectIdentifier);
  register_property("Sync ID", LIBMTP_PROPERTY_SyncID, PTP_OPC_SyncID);
  register_property("Property Bag", LIBMTP_PROPERTY_PropertyBag, PTP_OPC_PropertyBag);
  register_property("Name", LIBMTP_PROPERTY_Name, PTP_OPC_Name);
  register_property("Created By", LIBMTP_PROPERTY_CreatedBy, PTP_OPC_CreatedBy);
  register_property("Artist", LIBMTP_PROPERTY_Artist, PTP_OPC_Artist);
  register_property("Date Authored", LIBMTP_PROPERTY_DateAuthored, PTP_OPC_DateAuthored);
  register_property("Description", LIBMTP_PROPERTY_Description, PTP_OPC_Description);
  register_property("URL Reference", LIBMTP_PROPERTY_URLReference, PTP_OPC_URLReference);
  register_property("Language Locale", LIBMTP_PROPERTY_LanguageLocale, PTP_OPC_LanguageLocale);
  register_property("Copyright Information", LIBMTP_PROPERTY_CopyrightInformation, PTP_OPC_CopyrightInformation);
  register_property("Source", LIBMTP_PROPERTY_Source, PTP_OPC_Source);
  register_property("Origin Location", LIBMTP_PROPERTY_OriginLocation, PTP_OPC_OriginLocation);
  register_property("Date Added", LIBMTP_PROPERTY_DateAdded, PTP_OPC_DateAdded);
  register_property("Non Consumable", LIBMTP_PROPERTY_NonConsumable, PTP_OPC_NonConsumable);
  register_property("Corrupt Or Unplayable", LIBMTP_PROPERTY_CorruptOrUnplayable, PTP_OPC_CorruptOrUnplayable);
  register_property("Producer Serial Number", LIBMTP_PROPERTY_ProducerSerialNumber, PTP_OPC_ProducerSerialNumber);
  register_property("Representative Sample Format", LIBMTP_PROPERTY_RepresentativeSampleFormat, PTP_OPC_RepresentativeSampleFormat);
  register_property("Representative Sample Sise", LIBMTP_PROPERTY_RepresentativeSampleSize, PTP_OPC_RepresentativeSampleSize);
  register_property("Representative Sample Height", LIBMTP_PROPERTY_RepresentativeSampleHeight, PTP_OPC_RepresentativeSampleHeight);
  register_property("Representative Sample Width", LIBMTP_PROPERTY_RepresentativeSampleWidth, PTP_OPC_RepresentativeSampleWidth);
  register_property("Representative Sample Duration", LIBMTP_PROPERTY_RepresentativeSampleDuration, PTP_OPC_RepresentativeSampleDuration);
  register_property("Representative Sample Data", LIBMTP_PROPERTY_RepresentativeSampleData, PTP_OPC_RepresentativeSampleData);
  register_property("Width", LIBMTP_PROPERTY_Width, PTP_OPC_Width);
  register_property("Height", LIBMTP_PROPERTY_Height, PTP_OPC_Height);
  register_property("Duration", LIBMTP_PROPERTY_Duration, PTP_OPC_Duration);
  register_property("Rating", LIBMTP_PROPERTY_Rating, PTP_OPC_Rating);
  register_property("Track", LIBMTP_PROPERTY_Track, PTP_OPC_Track);
  register_property("Genre", LIBMTP_PROPERTY_Genre, PTP_OPC_Genre);
  register_property("Credits", LIBMTP_PROPERTY_Credits, PTP_OPC_Credits);
  register_property("Lyrics", LIBMTP_PROPERTY_Lyrics, PTP_OPC_Lyrics);
  register_property("Subscription Content ID", LIBMTP_PROPERTY_SubscriptionContentID, PTP_OPC_SubscriptionContentID);
  register_property("Produced By", LIBMTP_PROPERTY_ProducedBy, PTP_OPC_ProducedBy);
  register_property("Use Count", LIBMTP_PROPERTY_UseCount, PTP_OPC_UseCount);
  register_property("Skip Count", LIBMTP_PROPERTY_SkipCount, PTP_OPC_SkipCount);
  register_property("Last Accessed", LIBMTP_PROPERTY_LastAccessed, PTP_OPC_LastAccessed);
  register_property("Parental Rating", LIBMTP_PROPERTY_ParentalRating, PTP_OPC_ParentalRating);
  register_property("Meta Genre", LIBMTP_PROPERTY_MetaGenre, PTP_OPC_MetaGenre);
  register_property("Composer", LIBMTP_PROPERTY_Composer, PTP_OPC_Composer);
  register_property("Effective Rating", LIBMTP_PROPERTY_EffectiveRating, PTP_OPC_EffectiveRating);
  register_property("Subtitle", LIBMTP_PROPERTY_Subtitle, PTP_OPC_Subtitle);
  register_property("Original Release Date", LIBMTP_PROPERTY_OriginalReleaseDate, PTP_OPC_OriginalReleaseDate);
  register_property("Album Name", LIBMTP_PROPERTY_AlbumName, PTP_OPC_AlbumName);
  register_property("Album Artist", LIBMTP_PROPERTY_AlbumArtist, PTP_OPC_AlbumArtist);
  register_property("Mood", LIBMTP_PROPERTY_Mood, PTP_OPC_Mood);
  register_property("DRM Status", LIBMTP_PROPERTY_DRMStatus, PTP_OPC_DRMStatus);
  register_property("Sub Description", LIBMTP_PROPERTY_SubDescription, PTP_OPC_SubDescription);
  register_property("Is Cropped", LIBMTP_PROPERTY_IsCropped, PTP_OPC_IsCropped);
  register_property("Is Color Corrected", LIBMTP_PROPERTY_IsColorCorrected, PTP_OPC_IsColorCorrected);
  register_property("Image Bit Depth", LIBMTP_PROPERTY_ImageBitDepth, PTP_OPC_ImageBitDepth);
  register_property("f Number", LIBMTP_PROPERTY_Fnumber, PTP_OPC_Fnumber);
  register_property("Exposure Time", LIBMTP_PROPERTY_ExposureTime, PTP_OPC_ExposureTime);
  register_property("Exposure Index", LIBMTP_PROPERTY_ExposureIndex, PTP_OPC_ExposureIndex);
  register_property("Display Name", LIBMTP_PROPERTY_DisplayName, PTP_OPC_DisplayName);
  register_property("Body Text", LIBMTP_PROPERTY_BodyText, PTP_OPC_BodyText);
  register_property("Subject", LIBMTP_PROPERTY_Subject, PTP_OPC_Subject);
  register_property("Priority", LIBMTP_PROPERTY_Priority, PTP_OPC_Priority);
  register_property("Given Name", LIBMTP_PROPERTY_GivenName, PTP_OPC_GivenName);
  register_property("Middle Names", LIBMTP_PROPERTY_MiddleNames, PTP_OPC_MiddleNames);
  register_property("Family Name", LIBMTP_PROPERTY_FamilyName, PTP_OPC_FamilyName);
  register_property("Prefix", LIBMTP_PROPERTY_Prefix, PTP_OPC_Prefix);
  register_property("Suffix", LIBMTP_PROPERTY_Suffix, PTP_OPC_Suffix);
  register_property("Phonetic Given Name", LIBMTP_PROPERTY_PhoneticGivenName, PTP_OPC_PhoneticGivenName);
  register_property("Phonetic Family Name", LIBMTP_PROPERTY_PhoneticFamilyName, PTP_OPC_PhoneticFamilyName);
  register_property("Email: Primary", LIBMTP_PROPERTY_EmailPrimary, PTP_OPC_EmailPrimary);
  register_property("Email: Personal 1", LIBMTP_PROPERTY_EmailPersonal1, PTP_OPC_EmailPersonal1);
  register_property("Email: Personal 2", LIBMTP_PROPERTY_EmailPersonal2, PTP_OPC_EmailPersonal2);
  register_property("Email: Business 1", LIBMTP_PROPERTY_EmailBusiness1, PTP_OPC_EmailBusiness1);
  register_property("Email: Business 2", LIBMTP_PROPERTY_EmailBusiness2, PTP_OPC_EmailBusiness2);
  register_property("Email: Others", LIBMTP_PROPERTY_EmailOthers, PTP_OPC_EmailOthers);
  register_property("Phone Number: Primary", LIBMTP_PROPERTY_PhoneNumberPrimary, PTP_OPC_PhoneNumberPrimary);
  register_property("Phone Number: Personal", LIBMTP_PROPERTY_PhoneNumberPersonal, PTP_OPC_PhoneNumberPersonal);
  register_property("Phone Number: Personal 2", LIBMTP_PROPERTY_PhoneNumberPersonal2, PTP_OPC_PhoneNumberPersonal2);
  register_property("Phone Number: Business", LIBMTP_PROPERTY_PhoneNumberBusiness, PTP_OPC_PhoneNumberBusiness);
  register_property("Phone Number: Business 2", LIBMTP_PROPERTY_PhoneNumberBusiness2, PTP_OPC_PhoneNumberBusiness2);
  register_property("Phone Number: Mobile", LIBMTP_PROPERTY_PhoneNumberMobile, PTP_OPC_PhoneNumberMobile);
  register_property("Phone Number: Mobile 2", LIBMTP_PROPERTY_PhoneNumberMobile2, PTP_OPC_PhoneNumberMobile2);
  register_property("Fax Number: Primary", LIBMTP_PROPERTY_FaxNumberPrimary, PTP_OPC_FaxNumberPrimary);
  register_property("Fax Number: Personal", LIBMTP_PROPERTY_FaxNumberPersonal, PTP_OPC_FaxNumberPersonal);
  register_property("Fax Number: Business", LIBMTP_PROPERTY_FaxNumberBusiness, PTP_OPC_FaxNumberBusiness);
  register_property("Pager Number", LIBMTP_PROPERTY_PagerNumber, PTP_OPC_PagerNumber);
  register_property("Phone Number: Others", LIBMTP_PROPERTY_PhoneNumberOthers, PTP_OPC_PhoneNumberOthers);
  register_property("Primary Web Address", LIBMTP_PROPERTY_PrimaryWebAddress, PTP_OPC_PrimaryWebAddress);
  register_property("Personal Web Address", LIBMTP_PROPERTY_PersonalWebAddress, PTP_OPC_PersonalWebAddress);
  register_property("Business Web Address", LIBMTP_PROPERTY_BusinessWebAddress, PTP_OPC_BusinessWebAddress);
  register_property("Instant Messenger Address 1", LIBMTP_PROPERTY_InstantMessengerAddress, PTP_OPC_InstantMessengerAddress);
  register_property("Instant Messenger Address 2", LIBMTP_PROPERTY_InstantMessengerAddress2, PTP_OPC_InstantMessengerAddress2);
  register_property("Instant Messenger Address 3", LIBMTP_PROPERTY_InstantMessengerAddress3, PTP_OPC_InstantMessengerAddress3);
  register_property("Postal Address: Personal: Full", LIBMTP_PROPERTY_PostalAddressPersonalFull, PTP_OPC_PostalAddressPersonalFull);
  register_property("Postal Address: Personal: Line 1", LIBMTP_PROPERTY_PostalAddressPersonalFullLine1, PTP_OPC_PostalAddressPersonalFullLine1);
  register_property("Postal Address: Personal: Line 2", LIBMTP_PROPERTY_PostalAddressPersonalFullLine2, PTP_OPC_PostalAddressPersonalFullLine2);
  register_property("Postal Address: Personal: City", LIBMTP_PROPERTY_PostalAddressPersonalFullCity, PTP_OPC_PostalAddressPersonalFullCity);
  register_property("Postal Address: Personal: Region", LIBMTP_PROPERTY_PostalAddressPersonalFullRegion, PTP_OPC_PostalAddressPersonalFullRegion);
  register_property("Postal Address: Personal: Postal Code", LIBMTP_PROPERTY_PostalAddressPersonalFullPostalCode, PTP_OPC_PostalAddressPersonalFullPostalCode);
  register_property("Postal Address: Personal: Country", LIBMTP_PROPERTY_PostalAddressPersonalFullCountry, PTP_OPC_PostalAddressPersonalFullCountry);
  register_property("Postal Address: Business: Full", LIBMTP_PROPERTY_PostalAddressBusinessFull, PTP_OPC_PostalAddressBusinessFull);
  register_property("Postal Address: Business: Line 1", LIBMTP_PROPERTY_PostalAddressBusinessLine1, PTP_OPC_PostalAddressBusinessLine1);
  register_property("Postal Address: Business: Line 2", LIBMTP_PROPERTY_PostalAddressBusinessLine2, PTP_OPC_PostalAddressBusinessLine2);
  register_property("Postal Address: Business: City", LIBMTP_PROPERTY_PostalAddressBusinessCity, PTP_OPC_PostalAddressBusinessCity);
  register_property("Postal Address: Business: Region", LIBMTP_PROPERTY_PostalAddressBusinessRegion, PTP_OPC_PostalAddressBusinessRegion);
  register_property("Postal Address: Business: Postal Code", LIBMTP_PROPERTY_PostalAddressBusinessPostalCode, PTP_OPC_PostalAddressBusinessPostalCode);
  register_property("Postal Address: Business: Country", LIBMTP_PROPERTY_PostalAddressBusinessCountry, PTP_OPC_PostalAddressBusinessCountry);
  register_property("Postal Address: Other: Full", LIBMTP_PROPERTY_PostalAddressOtherFull, PTP_OPC_PostalAddressOtherFull);
  register_property("Postal Address: Other: Line 1", LIBMTP_PROPERTY_PostalAddressOtherLine1, PTP_OPC_PostalAddressOtherLine1);
  register_property("Postal Address: Other: Line 2", LIBMTP_PROPERTY_PostalAddressOtherLine2, PTP_OPC_PostalAddressOtherLine2);
  register_property("Postal Address: Other: City", LIBMTP_PROPERTY_PostalAddressOtherCity, PTP_OPC_PostalAddressOtherCity);
  register_property("Postal Address: Other: Region", LIBMTP_PROPERTY_PostalAddressOtherRegion, PTP_OPC_PostalAddressOtherRegion);
  register_property("Postal Address: Other: Postal Code", LIBMTP_PROPERTY_PostalAddressOtherPostalCode, PTP_OPC_PostalAddressOtherPostalCode);
  register_property("Postal Address: Other: Counrtry", LIBMTP_PROPERTY_PostalAddressOtherCountry, PTP_OPC_PostalAddressOtherCountry);
  register_property("Organization Name", LIBMTP_PROPERTY_OrganizationName, PTP_OPC_OrganizationName);
  register_property("Phonetic Organization Name", LIBMTP_PROPERTY_PhoneticOrganizationName, PTP_OPC_PhoneticOrganizationName);
  register_property("Role", LIBMTP_PROPERTY_Role, PTP_OPC_Role);
  register_property("Birthdate", LIBMTP_PROPERTY_Birthdate, PTP_OPC_Birthdate);
  register_property("Message To", LIBMTP_PROPERTY_MessageTo, PTP_OPC_MessageTo);
  register_property("Message CC", LIBMTP_PROPERTY_MessageCC, PTP_OPC_MessageCC);
  register_property("Message BCC", LIBMTP_PROPERTY_MessageBCC, PTP_OPC_MessageBCC);
  register_property("Message Read", LIBMTP_PROPERTY_MessageRead, PTP_OPC_MessageRead);
  register_property("Message Received Time", LIBMTP_PROPERTY_MessageReceivedTime, PTP_OPC_MessageReceivedTime);
  register_property("Message Sender", LIBMTP_PROPERTY_MessageSender, PTP_OPC_MessageSender);
  register_property("Activity Begin Time", LIBMTP_PROPERTY_ActivityBeginTime, PTP_OPC_ActivityBeginTime);
  register_property("Activity End Time", LIBMTP_PROPERTY_ActivityEndTime, PTP_OPC_ActivityEndTime);
  register_property("Activity Location", LIBMTP_PROPERTY_ActivityLocation, PTP_OPC_ActivityLocation);
  register_property("Activity Required Attendees", LIBMTP_PROPERTY_ActivityRequiredAttendees, PTP_OPC_ActivityRequiredAttendees);
  register_property("Optional Attendees", LIBMTP_PROPERTY_ActivityOptionalAttendees, PTP_OPC_ActivityOptionalAttendees);
  register_property("Activity Resources", LIBMTP_PROPERTY_ActivityResources, PTP_OPC_ActivityResources);
  register_property("Activity Accepted", LIBMTP_PROPERTY_ActivityAccepted, PTP_OPC_ActivityAccepted);
  register_property("Owner", LIBMTP_PROPERTY_Owner, PTP_OPC_Owner);
  register_property("Editor", LIBMTP_PROPERTY_Editor, PTP_OPC_Editor);
  register_property("Webmaster", LIBMTP_PROPERTY_Webmaster, PTP_OPC_Webmaster);
  register_property("URL Source", LIBMTP_PROPERTY_URLSource, PTP_OPC_URLSource);
  register_property("URL Destination", LIBMTP_PROPERTY_URLDestination, PTP_OPC_URLDestination);
  register_property("Time Bookmark", LIBMTP_PROPERTY_TimeBookmark, PTP_OPC_TimeBookmark);
  register_property("Object Bookmark", LIBMTP_PROPERTY_ObjectBookmark, PTP_OPC_ObjectBookmark);
  register_property("Byte Bookmark", LIBMTP_PROPERTY_ByteBookmark, PTP_OPC_ByteBookmark);
  register_property("Last Build Date", LIBMTP_PROPERTY_LastBuildDate, PTP_OPC_LastBuildDate);
  register_property("Time To Live", LIBMTP_PROPERTY_TimetoLive, PTP_OPC_TimetoLive);
  register_property("Media GUID", LIBMTP_PROPERTY_MediaGUID, PTP_OPC_MediaGUID);
  register_property("Total Bit Rate", LIBMTP_PROPERTY_TotalBitRate, PTP_OPC_TotalBitRate);
  register_property("Bit Rate Type", LIBMTP_PROPERTY_BitRateType, PTP_OPC_BitRateType);
  register_property("Sample Rate", LIBMTP_PROPERTY_SampleRate, PTP_OPC_SampleRate);
  register_property("Number Of Channels", LIBMTP_PROPERTY_NumberOfChannels, PTP_OPC_NumberOfChannels);
  register_property("Audio Bit Depth", LIBMTP_PROPERTY_AudioBitDepth, PTP_OPC_AudioBitDepth);
  register_property("Scan Depth", LIBMTP_PROPERTY_ScanDepth, PTP_OPC_ScanDepth);
  register_property("Audio WAVE Codec", LIBMTP_PROPERTY_AudioWAVECodec, PTP_OPC_AudioWAVECodec);
  register_property("Audio Bit Rate", LIBMTP_PROPERTY_AudioBitRate, PTP_OPC_AudioBitRate);
  register_property("Video Four CC Codec", LIBMTP_PROPERTY_VideoFourCCCodec, PTP_OPC_VideoFourCCCodec);
  register_property("Video Bit Rate", LIBMTP_PROPERTY_VideoBitRate, PTP_OPC_VideoBitRate);
  register_property("Frames Per Thousand Seconds", LIBMTP_PROPERTY_FramesPerThousandSeconds, PTP_OPC_FramesPerThousandSeconds);
  register_property("Key Frame Distance", LIBMTP_PROPERTY_KeyFrameDistance, PTP_OPC_KeyFrameDistance);
  register_property("Buffer Size", LIBMTP_PROPERTY_BufferSize, PTP_OPC_BufferSize);
  register_property("Encoding Quality", LIBMTP_PROPERTY_EncodingQuality, PTP_OPC_EncodingQuality);
  register_property("Encoding Profile", LIBMTP_PROPERTY_EncodingProfile, PTP_OPC_EncodingProfile);
  register_property("Buy flag", LIBMTP_PROPERTY_BuyFlag, PTP_OPC_BuyFlag);
  register_property("Unknown property", LIBMTP_PROPERTY_UNKNOWN, 0);
}

/**
 * Returns the PTP property that maps to a certain libmtp internal property type.
 * @param inproperty the MTP library interface property
 * @return the PTP (libgphoto2) property type
 */
static uint16_t map_libmtp_property_to_ptp_property(LIBMTP_property_t inproperty)
{
  propertymap_t *current;

  current = g_propertymap;

  while (current != NULL) {
    if(current->id == inproperty) {
      return current->ptp_id;
    }
    current = current->next;
  }
  return 0;
}


/**
 * Returns the MTP internal interface property that maps to a certain ptp
 * interface property.
 * @param inproperty the PTP (libgphoto2) interface property
 * @return the MTP library interface property
 */
static LIBMTP_property_t map_ptp_property_to_libmtp_property(uint16_t inproperty)
{
  propertymap_t *current;

  current = g_propertymap;

  while (current != NULL) {
    if(current->ptp_id == inproperty) {
      return current->id;
    }
    current = current->next;
  }
  // printf("map_ptp_type_to_libmtp_type: unknown filetype.\n");
  return LIBMTP_PROPERTY_UNKNOWN;
}


/**
 * Set the debug level.
 *
 * By default, the debug level is set to '0' (disable).
 */
void LIBMTP_Set_Debug(int level)
{
  if (LIBMTP_debug || level)
    LIBMTP_ERROR("LIBMTP_Set_Debug: Setting debugging level to %d (0x%02x) "
                 "(%s)\n", level, level, level ? "on" : "off");

  LIBMTP_debug = level;
}


/**
 * Initialize the library. You are only supposed to call this
 * one, before using the library for the first time in a program.
 * Never re-initialize libmtp!
 *
 * The only thing this does at the moment is to initialise the
 * filetype mapping table, as well as load MTPZ data if necessary.
 */
void LIBMTP_Init(void)
{
  const char *env_debug = getenv("LIBMTP_DEBUG");
  if (env_debug) {
    const long debug_flags = strtol(env_debug, NULL, 0);
    if (debug_flags != LONG_MIN && debug_flags != LONG_MAX &&
        INT_MIN <= debug_flags && debug_flags <= INT_MAX) {
      LIBMTP_Set_Debug(debug_flags);
    } else {
      fprintf(stderr, "LIBMTP_Init: error setting debug flags from environment "
                      "value \"%s\"\n", env_debug);
    }
  }

  init_filemap();
  init_propertymap();

  if (mtpz_loaddata() == -1)
    use_mtpz = 0;
  else
    use_mtpz = 1;

  return;
}


/**
 * This helper function returns a textual description for a libmtp
 * file type to be used in dialog boxes etc.
 * @param intype the libmtp internal filetype to get a description for.
 * @return a string representing the filetype, this must <b>NOT</b>
 *         be free():ed by the caller!
 */
char const * LIBMTP_Get_Filetype_Description(LIBMTP_filetype_t intype)
{
  filemap_t *current;

  current = g_filemap;

  while (current != NULL) {
    if(current->id == intype) {
      return current->description;
    }
    current = current->next;
  }

  return "Unknown filetype";
}

/**
 * This helper function returns a textual description for a libmtp
 * property to be used in dialog boxes etc.
 * @param inproperty the libmtp internal property to get a description for.
 * @return a string representing the filetype, this must <b>NOT</b>
 *         be free():ed by the caller!
 */
char const * LIBMTP_Get_Property_Description(LIBMTP_property_t inproperty)
{
  propertymap_t *current;

  current = g_propertymap;

  while (current != NULL) {
    if(current->id == inproperty) {
      return current->description;
    }
    current = current->next;
  }

  return "Unknown property";
}

/**
 * This function will do its best to fit a 16bit
 * value into a PTP object property if the property
 * is limited in range or step sizes.
 */
static uint16_t adjust_u16(uint16_t val, PTPObjectPropDesc *opd)
{
  switch (opd->FormFlag) {
  case PTP_DPFF_Range:
    if (val < opd->FORM.Range.MinimumValue.u16) {
      return opd->FORM.Range.MinimumValue.u16;
    }
    if (val > opd->FORM.Range.MaximumValue.u16) {
      return opd->FORM.Range.MaximumValue.u16;
    }
    // Round down to last step.
    if (val % opd->FORM.Range.StepSize.u16 != 0) {
      return val - (val % opd->FORM.Range.StepSize.u16);
    }
    return val;
    break;
  case PTP_DPFF_Enumeration:
    {
      int i;
      uint16_t bestfit = opd->FORM.Enum.SupportedValue[0].u16;

      for (i=0; i<opd->FORM.Enum.NumberOfValues; i++) {
	if (val == opd->FORM.Enum.SupportedValue[i].u16) {
	  return val;
	}
	// Rough guess of best fit
	if (opd->FORM.Enum.SupportedValue[i].u16 < val) {
	  bestfit = opd->FORM.Enum.SupportedValue[i].u16;
	}
      }
      // Just some default that'll work.
      return bestfit;
    }
  default:
    // Will accept any value
    break;
  }
  return val;
}

/**
 * This function will do its best to fit a 32bit
 * value into a PTP object property if the property
 * is limited in range or step sizes.
 */
static uint32_t adjust_u32(uint32_t val, PTPObjectPropDesc *opd)
{
  switch (opd->FormFlag) {
  case PTP_DPFF_Range:
    if (val < opd->FORM.Range.MinimumValue.u32) {
      return opd->FORM.Range.MinimumValue.u32;
    }
    if (val > opd->FORM.Range.MaximumValue.u32) {
      return opd->FORM.Range.MaximumValue.u32;
    }
    // Round down to last step.
    if (val % opd->FORM.Range.StepSize.u32 != 0) {
      return val - (val % opd->FORM.Range.StepSize.u32);
    }
    return val;
    break;
  case PTP_DPFF_Enumeration:
    {
      int i;
      uint32_t bestfit = opd->FORM.Enum.SupportedValue[0].u32;

      for (i=0; i<opd->FORM.Enum.NumberOfValues; i++) {
	if (val == opd->FORM.Enum.SupportedValue[i].u32) {
	  return val;
	}
	// Rough guess of best fit
	if (opd->FORM.Enum.SupportedValue[i].u32 < val) {
	  bestfit = opd->FORM.Enum.SupportedValue[i].u32;
	}
      }
      // Just some default that'll work.
      return bestfit;
    }
  default:
    // Will accept any value
    break;
  }
  return val;
}

/**
 * This function returns a newly created ISO 8601 timestamp with the
 * current time in as high precision as possible. It even adds
 * the time zone if it can.
 */
static char *get_iso8601_stamp(void)
{
  time_t curtime;
  struct tm *loctime;
  char tmp[64];

  curtime = time(NULL);
  loctime = localtime(&curtime);
  strftime (tmp, sizeof(tmp), "%Y%m%dT%H%M%S.0%z", loctime);
  return strdup(tmp);
}

/**
 * Gets the allowed values (range or enum) for a property
 * @param device a pointer to an MTP device
 * @param property the property to query
 * @param filetype the filetype of the object you want to set values for
 * @param allowed_vals pointer to a LIBMTP_allowed_values_t struct to
 *        receive the allowed values.  Call LIBMTP_destroy_allowed_values_t
 *        on this on successful completion.
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Get_Allowed_Property_Values(LIBMTP_mtpdevice_t *device, LIBMTP_property_t const property,
            LIBMTP_filetype_t const filetype, LIBMTP_allowed_values_t *allowed_vals)
{
  PTPObjectPropDesc opd;
  uint16_t ret = 0;

  ret = ptp_mtp_getobjectpropdesc(device->params, map_libmtp_property_to_ptp_property(property), map_libmtp_type_to_ptp_type(filetype), &opd);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Allowed_Property_Values(): could not get property description.");
    return -1;
  }

  if (opd.FormFlag == PTP_OPFF_Enumeration) {
    int i = 0;

    allowed_vals->is_range = 0;
    allowed_vals->num_entries = opd.FORM.Enum.NumberOfValues;

    switch (opd.DataType)
    {
      case PTP_DTC_INT8:
        allowed_vals->i8vals = malloc(sizeof(int8_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_INT8;
        break;
      case PTP_DTC_UINT8:
        allowed_vals->u8vals = malloc(sizeof(uint8_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT8;
        break;
      case PTP_DTC_INT16:
        allowed_vals->i16vals = malloc(sizeof(int16_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_INT16;
        break;
      case PTP_DTC_UINT16:
        allowed_vals->u16vals = malloc(sizeof(uint16_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT16;
        break;
      case PTP_DTC_INT32:
        allowed_vals->i32vals = malloc(sizeof(int32_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_INT32;
        break;
      case PTP_DTC_UINT32:
        allowed_vals->u32vals = malloc(sizeof(uint32_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT32;
        break;
      case PTP_DTC_INT64:
        allowed_vals->i64vals = malloc(sizeof(int64_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_INT64;
        break;
      case PTP_DTC_UINT64:
        allowed_vals->u64vals = malloc(sizeof(uint64_t) * opd.FORM.Enum.NumberOfValues);
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT64;
        break;
    }

    for (i = 0; i < opd.FORM.Enum.NumberOfValues; i++) {
      switch (opd.DataType)
      {
        case PTP_DTC_INT8:
          allowed_vals->i8vals[i] = opd.FORM.Enum.SupportedValue[i].i8;
          break;
        case PTP_DTC_UINT8:
          allowed_vals->u8vals[i] = opd.FORM.Enum.SupportedValue[i].u8;
          break;
        case PTP_DTC_INT16:
          allowed_vals->i16vals[i] = opd.FORM.Enum.SupportedValue[i].i16;
          break;
        case PTP_DTC_UINT16:
          allowed_vals->u16vals[i] = opd.FORM.Enum.SupportedValue[i].u16;
          break;
        case PTP_DTC_INT32:
          allowed_vals->i32vals[i] = opd.FORM.Enum.SupportedValue[i].i32;
          break;
        case PTP_DTC_UINT32:
          allowed_vals->u32vals[i] = opd.FORM.Enum.SupportedValue[i].u32;
          break;
        case PTP_DTC_INT64:
          allowed_vals->i64vals[i] = opd.FORM.Enum.SupportedValue[i].i64;
          break;
        case PTP_DTC_UINT64:
          allowed_vals->u64vals[i] = opd.FORM.Enum.SupportedValue[i].u64;
          break;
      }
    }
    ptp_free_objectpropdesc(&opd);
    return 0;
  } else if (opd.FormFlag == PTP_OPFF_Range) {
    allowed_vals->is_range = 1;

    switch (opd.DataType)
    {
      case PTP_DTC_INT8:
        allowed_vals->i8min = opd.FORM.Range.MinimumValue.i8;
        allowed_vals->i8max = opd.FORM.Range.MaximumValue.i8;
        allowed_vals->i8step = opd.FORM.Range.StepSize.i8;
        allowed_vals->datatype = LIBMTP_DATATYPE_INT8;
        break;
      case PTP_DTC_UINT8:
        allowed_vals->u8min = opd.FORM.Range.MinimumValue.u8;
        allowed_vals->u8max = opd.FORM.Range.MaximumValue.u8;
        allowed_vals->u8step = opd.FORM.Range.StepSize.u8;
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT8;
        break;
      case PTP_DTC_INT16:
        allowed_vals->i16min = opd.FORM.Range.MinimumValue.i16;
        allowed_vals->i16max = opd.FORM.Range.MaximumValue.i16;
        allowed_vals->i16step = opd.FORM.Range.StepSize.i16;
        allowed_vals->datatype = LIBMTP_DATATYPE_INT16;
        break;
      case PTP_DTC_UINT16:
        allowed_vals->u16min = opd.FORM.Range.MinimumValue.u16;
        allowed_vals->u16max = opd.FORM.Range.MaximumValue.u16;
        allowed_vals->u16step = opd.FORM.Range.StepSize.u16;
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT16;
        break;
      case PTP_DTC_INT32:
        allowed_vals->i32min = opd.FORM.Range.MinimumValue.i32;
        allowed_vals->i32max = opd.FORM.Range.MaximumValue.i32;
        allowed_vals->i32step = opd.FORM.Range.StepSize.i32;
        allowed_vals->datatype = LIBMTP_DATATYPE_INT32;
        break;
      case PTP_DTC_UINT32:
        allowed_vals->u32min = opd.FORM.Range.MinimumValue.u32;
        allowed_vals->u32max = opd.FORM.Range.MaximumValue.u32;
        allowed_vals->u32step = opd.FORM.Range.StepSize.u32;
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT32;
        break;
      case PTP_DTC_INT64:
        allowed_vals->i64min = opd.FORM.Range.MinimumValue.i64;
        allowed_vals->i64max = opd.FORM.Range.MaximumValue.i64;
        allowed_vals->i64step = opd.FORM.Range.StepSize.i64;
        allowed_vals->datatype = LIBMTP_DATATYPE_INT64;
        break;
      case PTP_DTC_UINT64:
        allowed_vals->u64min = opd.FORM.Range.MinimumValue.u64;
        allowed_vals->u64max = opd.FORM.Range.MaximumValue.u64;
        allowed_vals->u64step = opd.FORM.Range.StepSize.u64;
        allowed_vals->datatype = LIBMTP_DATATYPE_UINT64;
        break;
    }
    return 0;
  } else
    return -1;
}

/**
 * Destroys a LIBMTP_allowed_values_t struct
 * @param allowed_vals the struct to destroy
 */
void LIBMTP_destroy_allowed_values_t(LIBMTP_allowed_values_t *allowed_vals)
{
  if (!allowed_vals->is_range)
  {
    switch (allowed_vals->datatype)
    {
      case LIBMTP_DATATYPE_INT8:
        if (allowed_vals->i8vals)
          free(allowed_vals->i8vals);
        break;
      case LIBMTP_DATATYPE_UINT8:
        if (allowed_vals->u8vals)
          free(allowed_vals->u8vals);
        break;
      case LIBMTP_DATATYPE_INT16:
        if (allowed_vals->i16vals)
          free(allowed_vals->i16vals);
        break;
      case LIBMTP_DATATYPE_UINT16:
        if (allowed_vals->u16vals)
          free(allowed_vals->u16vals);
        break;
      case LIBMTP_DATATYPE_INT32:
        if (allowed_vals->i32vals)
          free(allowed_vals->i32vals);
        break;
      case LIBMTP_DATATYPE_UINT32:
        if (allowed_vals->u32vals)
          free(allowed_vals->u32vals);
        break;
      case LIBMTP_DATATYPE_INT64:
        if (allowed_vals->i64vals)
          free(allowed_vals->i64vals);
        break;
      case LIBMTP_DATATYPE_UINT64:
        if (allowed_vals->u64vals)
          free(allowed_vals->u64vals);
        break;
    }
  }
}

/**
 * Determine if a property is supported for a given file type
 * @param device a pointer to an MTP device
 * @param property the property to query
 * @param filetype the filetype of the object you want to set values for
 * @return 0 if not supported, positive if supported, negative on error
 */
int LIBMTP_Is_Property_Supported(LIBMTP_mtpdevice_t *device, LIBMTP_property_t const property,
            LIBMTP_filetype_t const filetype)
{
  uint16_t *props = NULL;
  uint32_t propcnt = 0;
  uint16_t ret = 0;
  unsigned int i = 0;
  int supported = 0;
  uint16_t ptp_prop = map_libmtp_property_to_ptp_property(property);

  if (!ptp_operation_issupported(device->params, PTP_OC_MTP_GetObjectPropsSupported))
    return 0;

  ret = ptp_mtp_getobjectpropssupported(device->params, map_libmtp_type_to_ptp_type(filetype), &propcnt, &props);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Is_Property_Supported(): could not get properties supported.");
    return -1;
  }

	for (i = 0; i < propcnt; i++) {
    if (props[i] == ptp_prop) {
      supported = 1;
      break;
    }
  }

  free(props);

  return supported;
}

/**
 * Retrieves a string from an object
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @return valid string or NULL on failure. The returned string
 *         must bee <code>free()</code>:ed by the caller after
 *         use.
 */
char *LIBMTP_Get_String_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    LIBMTP_property_t const attribute_id)
{
  return get_string_from_object(device, object_id, map_libmtp_property_to_ptp_property(attribute_id));
}

/**
* Retrieves an unsigned 64-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
uint64_t LIBMTP_Get_u64_From_Object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
                                    LIBMTP_property_t const attribute_id, uint64_t const value_default)
{
  return get_u64_from_object(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value_default);
}

/**
 * Retrieves an unsigned 32-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
uint32_t LIBMTP_Get_u32_From_Object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
				    LIBMTP_property_t const attribute_id, uint32_t const value_default)
{
  return get_u32_from_object(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value_default);
}

/**
 * Retrieves an unsigned 16-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
uint16_t LIBMTP_Get_u16_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    LIBMTP_property_t const attribute_id, uint16_t const value_default)
{
  return get_u16_from_object(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value_default);
}

/**
 * Retrieves an unsigned 8-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
uint8_t LIBMTP_Get_u8_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				  LIBMTP_property_t const attribute_id, uint8_t const value_default)
{
  return get_u8_from_object(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value_default);
}

/**
 * Sets an object attribute from a string
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param string string value to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_String(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			     LIBMTP_property_t const attribute_id, char const * const string)
{
  return set_object_string(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), string);
}


/**
 * Sets an object attribute from an unsigned 32-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 32-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  LIBMTP_property_t const attribute_id, uint32_t const value)
{
  return set_object_u32(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value);
}

/**
 * Sets an object attribute from an unsigned 16-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 16-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  LIBMTP_property_t const attribute_id, uint16_t const value)
{
  return set_object_u16(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value);
}

/**
 * Sets an object attribute from an unsigned 8-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 8-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			 LIBMTP_property_t const attribute_id, uint8_t const value)
{
  return set_object_u8(device, object_id, map_libmtp_property_to_ptp_property(attribute_id), value);
}

/**
 * Retrieves a string from an object
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @return valid string or NULL on failure. The returned string
 *         must bee <code>free()</code>:ed by the caller after
 *         use.
 */
static char *get_string_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    uint16_t const attribute_id)
{
  PTPPropertyValue propval;
  char *retstring = NULL;
  PTPParams *params;
  uint16_t ret;
  MTPProperties *prop;

  if (!device || !object_id)
    return NULL;

  params = (PTPParams *) device->params;

  prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
  if (prop) {
    if (prop->propval.str != NULL)
      return strdup(prop->propval.str);
    else
      return NULL;
  }

  ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval, PTP_DTC_STR);
  if (ret == PTP_RC_OK) {
    if (propval.str != NULL) {
      retstring = (char *) strdup(propval.str);
      free(propval.str);
    }
  } else {
    add_ptp_error_to_errorstack(device, ret, "get_string_from_object(): could not get object string.");
  }

  return retstring;
}

/**
* Retrieves an unsigned 64-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
static uint64_t get_u64_from_object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
                                    uint16_t const attribute_id, uint64_t const value_default)
{
  PTPPropertyValue propval;
  uint64_t retval = value_default;
  PTPParams *params;
  uint16_t ret;
  MTPProperties *prop;

  if (!device)
    return value_default;

  params = (PTPParams *) device->params;

  prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
  if (prop)
    return prop->propval.u64;

  ret = ptp_mtp_getobjectpropvalue(params, object_id,
                                   attribute_id,
                                   &propval,
                                   PTP_DTC_UINT64);
  if (ret == PTP_RC_OK) {
    retval = propval.u64;
  } else {
    add_ptp_error_to_errorstack(device, ret, "get_u64_from_object(): could not get unsigned 64bit integer from object.");
  }

  return retval;
}

/**
 * Retrieves an unsigned 32-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
static uint32_t get_u32_from_object(LIBMTP_mtpdevice_t *device,uint32_t const object_id,
				    uint16_t const attribute_id, uint32_t const value_default)
{
  PTPPropertyValue propval;
  uint32_t retval = value_default;
  PTPParams *params;
  uint16_t ret;
  MTPProperties *prop;

  if (!device)
    return value_default;

  params = (PTPParams *) device->params;

  prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
  if (prop)
    return prop->propval.u32;

  ret = ptp_mtp_getobjectpropvalue(params, object_id,
                                   attribute_id,
                                   &propval,
                                   PTP_DTC_UINT32);
  if (ret == PTP_RC_OK) {
    retval = propval.u32;
  } else {
    add_ptp_error_to_errorstack(device, ret, "get_u32_from_object(): could not get unsigned 32bit integer from object.");
  }
  return retval;
}

/**
 * Retrieves an unsigned 16-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
static uint16_t get_u16_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				    uint16_t const attribute_id, uint16_t const value_default)
{
  PTPPropertyValue propval;
  uint16_t retval = value_default;
  PTPParams *params;
  uint16_t ret;
  MTPProperties *prop;

  if (!device)
    return value_default;

  params = (PTPParams *) device->params;

  // This O(n) search should not be used so often, since code
  // using the cached properties don't usually call this function.
  prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
  if (prop)
    return prop->propval.u16;

  ret = ptp_mtp_getobjectpropvalue(params, object_id,
                                   attribute_id,
                                   &propval,
                                   PTP_DTC_UINT16);
  if (ret == PTP_RC_OK) {
    retval = propval.u16;
  } else {
    add_ptp_error_to_errorstack(device, ret, "get_u16_from_object(): could not get unsigned 16bit integer from object.");
  }

  return retval;
}

/**
 * Retrieves an unsigned 8-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
static uint8_t get_u8_from_object(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
				  uint16_t const attribute_id, uint8_t const value_default)
{
  PTPPropertyValue propval;
  uint8_t retval = value_default;
  PTPParams *params;
  uint16_t ret;
  MTPProperties *prop;

  if (!device)
    return value_default;

  params = (PTPParams *) device->params;

  // This O(n) search should not be used so often, since code
  // using the cached properties don't usually call this function.
  prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
  if (prop)
    return prop->propval.u8;

  ret = ptp_mtp_getobjectpropvalue(params, object_id,
                                   attribute_id,
                                   &propval,
                                   PTP_DTC_UINT8);
  if (ret == PTP_RC_OK) {
    retval = propval.u8;
  } else {
    add_ptp_error_to_errorstack(device, ret, "get_u8_from_object(): could not get unsigned 8bit integer from object.");
  }

  return retval;
}

/**
 * Sets an object attribute from a string
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param string string value to set
 * @return 0 on success, any other value means failure
 */
static int set_object_string(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			     uint16_t const attribute_id, char const * const string)
{
  PTPPropertyValue propval;
  PTPParams *params;
  uint16_t ret;

  if (!device || !string)
    return -1;

  params = (PTPParams *) device->params;

  if (!ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_string(): could not set object string: "
				"PTP_OC_MTP_SetObjectPropValue not supported.");
    return -1;
  }
  propval.str = (char *) string;
  ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval, PTP_DTC_STR);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "set_object_string(): could not set object string.");
    return -1;
  }

  return 0;
}


/**
 * Sets an object attribute from an unsigned 32-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 32-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  uint16_t const attribute_id, uint32_t const value)
{
  PTPPropertyValue propval;
  PTPParams *params;
  uint16_t ret;

  if (!device)
    return -1;

  params = (PTPParams *) device->params;

  if (!ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_u32(): could not set unsigned 32bit integer property: "
				"PTP_OC_MTP_SetObjectPropValue not supported.");
    return -1;
  }

  propval.u32 = value;
  ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval, PTP_DTC_UINT32);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "set_object_u32(): could not set unsigned 32bit integer property.");
    return -1;
  }

  return 0;
}

/**
 * Sets an object attribute from an unsigned 16-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 16-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			  uint16_t const attribute_id, uint16_t const value)
{
  PTPPropertyValue propval;
  PTPParams *params;
  uint16_t ret;

  if (!device)
    return 1;

  params = (PTPParams *) device->params;

  if (!ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_u16(): could not set unsigned 16bit integer property: "
				"PTP_OC_MTP_SetObjectPropValue not supported.");
    return -1;
  }
  propval.u16 = value;
  ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval, PTP_DTC_UINT16);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "set_object_u16(): could not set unsigned 16bit integer property.");
    return 1;
  }

  return 0;
}

/**
 * Sets an object attribute from an unsigned 8-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 8-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
			 uint16_t const attribute_id, uint8_t const value)
{
  PTPPropertyValue propval;
  PTPParams *params;
  uint16_t ret;

  if (!device)
    return 1;

  params = (PTPParams *) device->params;

  if (!ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_u8(): could not set unsigned 8bit integer property: "
			    "PTP_OC_MTP_SetObjectPropValue not supported.");
    return -1;
  }
  propval.u8 = value;
  ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval, PTP_DTC_UINT8);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "set_object_u8(): could not set unsigned 8bit integer property.");
    return 1;
  }

  return 0;
}

/**
 * Get connected MTP device by list position.
 * @return a device pointer.
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBMTP_Get_Device(int device_nr)
{
  LIBMTP_mtpdevice_t *device = NULL;
  LIBMTP_raw_device_t *devices;
  int numdevs;
  LIBMTP_error_number_t ret;

  ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
  if (ret != LIBMTP_ERROR_NONE) {
    return NULL;
  }

  if (devices == NULL || numdevs == 0) {
    free(devices);
    return NULL;
  }

  if (device_nr < 0 || device_nr >= numdevs) {
    free(devices);
    return NULL;
  }

  device = LIBMTP_Open_Raw_Device(&devices[device_nr]);
  free(devices);
  return device;
}

/**
 * Get the first (as in "first in the list of") connected MTP device.
 * @return a device pointer.
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void)
{
  return LIBMTP_Get_Device(0);
}

/**
 * Get connected MTP device by serial number.
 * @return a device pointer.
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBMTP_Get_Device_By_SerialNumber(char *serial_number)
{
  LIBMTP_mtpdevice_t *device = NULL;
  LIBMTP_raw_device_t *devices;
  int numdevs;
  LIBMTP_error_number_t ret;
  PTPParams *params;
  int found_device = 0;
  int i;

  if (serial_number == NULL || *serial_number == '\0')
    return NULL;

  ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
  if (ret != LIBMTP_ERROR_NONE)
    return NULL;

  if (devices == NULL || numdevs == 0) {
    free(devices);
    return NULL;
  }

  for (i = 0; i < numdevs; i++) {
    device = LIBMTP_Open_Raw_Device(&devices[i]);
    if (device == NULL)
      continue;

    params = (PTPParams *) device->params;
    if (strcmp(params->deviceinfo.SerialNumber, serial_number) == 0) {
      found_device = 1;
      break;
    }

    LIBMTP_Release_Device(device);
  }

  free(devices);

  if (!found_device)
    return NULL;

  return device;
}

/**
 * Get connected MTP device by list position or serial number.
 * @return a device pointer.
 * @see LIBMTP_Get_Connected_Devices()
 */
LIBMTP_mtpdevice_t *LIBMTP_Get_Device_By_ID(char *device_id)
{
  uint32_t device_nr;
  char *endptr;

  if (device_id == NULL || *device_id == '\0')
    return NULL;

  // 1st try: serial number
  if (strlen(device_id) > 3 && strncmp(device_id, "SN:", 3) == 0)
    return LIBMTP_Get_Device_By_SerialNumber(device_id + 3);

  // 2nd try: device number
  device_nr = strtoul(device_id, &endptr, 10);
  if (*endptr == '\0')
    return LIBMTP_Get_Device(device_nr);

  return NULL;
}

/**
 * Overriding debug function.
 * This way we can disable debug prints.
 */
static void
#ifdef __GNUC__
__attribute__((__format__(printf,2,0)))
#endif
LIBMTP_ptp_debug(void *data, const char *format, va_list args)
{
  if ((LIBMTP_debug & LIBMTP_DEBUG_PTP) != 0) {
    vfprintf (stderr, format, args);
    fprintf (stderr, "\n");
    fflush (stderr);
  }
}

/**
 * Overriding error function.
 * This way we can capture all error etc to our errorstack.
 */
static void
#ifdef __GNUC__
__attribute__((__format__(printf,2,0)))
#endif
LIBMTP_ptp_error(void *data, const char *format, va_list args)
{
  // if (data == NULL) {
    vfprintf (stderr, format, args);
    fflush (stderr);
  /*
    FIXME: find out how we shall get the device here.
  } else {
    PTP_USB *ptp_usb = data;
    LIBMTP_mtpdevice_t *device = ...;
    char buf[2048];

    vsnprintf (buf, sizeof (buf), format, args);
    add_error_to_errorstack(device,
			    LIBMTP_ERROR_PTP_LAYER,
			    buf);
  }
  */
}

/**
 * Parses the extension descriptor, there may be stuff in
 * this that we want to know about.
 */
static void parse_extension_descriptor(LIBMTP_mtpdevice_t *mtpdevice,
                                       char *desc)
{
  unsigned int start = 0;
  unsigned int end = 0;

  /* NULL on Canon A70 */
  if (!desc)
    return;

  /* descriptors are divided by semicolons */
  while (end < strlen(desc)) {
    /* Skip past initial whitespace */
    while ((end < strlen(desc)) && (desc[start] == ' ' )) {
      start++;
      end++;
    }
    /* Detect extension */
    while ((end < strlen(desc)) && (desc[end] != ';'))
      end++;
    if (end < strlen(desc)) {
      char *element = strndup(desc + start, end-start);
      if (element) {
        unsigned int i = 0;
        // printf("  Element: \"%s\"\n", element);

        /* Parse for an extension */
        while ((i < strlen(element)) && (element[i] != ':'))
          i++;
        if (i < strlen(element)) {
          char *name = strndup(element, i);
          int major = 0, minor = 0;

	  /* extension versions have to be MAJOR.MINOR, but Samsung has one
	   * with just 0, so just cope with those cases too */
	  if (	(2 == sscanf(element+i+1,"%d.%d",&major,&minor)) || 
	  	(1 == sscanf(element+i+1,"%d",&major))
	  ) {
            LIBMTP_device_extension_t *extension;

            extension = malloc(sizeof(LIBMTP_device_extension_t));
            extension->name = name;
            extension->major = major;
            extension->minor = minor;
            extension->next = NULL;
            if (mtpdevice->extensions == NULL) {
              mtpdevice->extensions = extension;
            } else {
              LIBMTP_device_extension_t *tmp = mtpdevice->extensions;
              while (tmp->next != NULL)
                tmp = tmp->next;
              tmp->next = extension;
            }
          } else {
            LIBMTP_ERROR("LIBMTP ERROR: couldnt parse extension %s\n",
                         element);
          }
        }
        free(element);
      }
    }
    end++;
    start = end;
  }
}

/**
 * This function opens a device from a raw device. It is the
 * preferred way to access devices in the new interface where
 * several devices can come and go as the library is working
 * on a certain device.
 * @param rawdevice the raw device to open a "real" device for.
 * @return an open device.
 */
LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *rawdevice)
{
  LIBMTP_mtpdevice_t *mtp_device;
  uint8_t bs = 0;
  PTPParams *current_params;
  PTP_USB *ptp_usb;
  LIBMTP_error_number_t err;
  unsigned int i;

  /* Allocate dynamic space for our device */
  mtp_device = (LIBMTP_mtpdevice_t *) malloc(sizeof(LIBMTP_mtpdevice_t));
  /* Check if there was a memory allocation error */
  if(mtp_device == NULL) {
    /* There has been an memory allocation error. We are going to ignore this
       device and attempt to continue */

    /* TODO: This error statement could probably be a bit more robust */
    LIBMTP_ERROR("LIBMTP PANIC: connect_usb_devices encountered a memory "
	    "allocation error with device %d on bus %d, trying to continue",
	    rawdevice->devnum, rawdevice->bus_location);

    return NULL;
  }
  memset(mtp_device, 0, sizeof(LIBMTP_mtpdevice_t));
  // Non-cached by default
  mtp_device->cached = 0;

  /* Create PTP params */
  current_params = (PTPParams *) malloc(sizeof(PTPParams));
  if (current_params == NULL) {
    free(mtp_device);
    return NULL;
  }
  memset(current_params, 0, sizeof(PTPParams));
  current_params->device_flags = rawdevice->device_entry.device_flags;
  current_params->nrofobjects = 0;
  current_params->cachetime = 2;
  current_params->objects = NULL;
  current_params->response_packet_size = 0;
  current_params->response_packet = NULL;
  /* This will be a pointer to PTP_USB later */
  current_params->data = NULL;
  /* Set upp local debug and error functions */
  current_params->debug_func = LIBMTP_ptp_debug;
  current_params->error_func = LIBMTP_ptp_error;
  /* TODO: Will this always be little endian? */
  current_params->byteorder = PTP_DL_LE;
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
  current_params->cd_locale_to_ucs2 = iconv_open("UTF-16LE", "UTF-8");
  current_params->cd_ucs2_to_locale = iconv_open("UTF-8", "UTF-16LE");

  if(current_params->cd_locale_to_ucs2 == (iconv_t) -1 ||
     current_params->cd_ucs2_to_locale == (iconv_t) -1) {
    LIBMTP_ERROR("LIBMTP PANIC: Cannot open iconv() converters to/from UCS-2!\n"
	    "Too old stdlibc, glibc and libiconv?\n");
    free(current_params);
    free(mtp_device);
    return NULL;
  }
#endif
  mtp_device->params = current_params;

  /* Create usbinfo, this also opens the session */
  err = configure_usb_device(rawdevice,
			     current_params,
			     &mtp_device->usbinfo);
  if (err != LIBMTP_ERROR_NONE) {
    free(current_params);
    free(mtp_device);
    return NULL;
  }
  ptp_usb = (PTP_USB*) mtp_device->usbinfo;
  /* Set pointer back to params */
  ptp_usb->params = current_params;

  /* Cache the device information for later use */
  if (ptp_getdeviceinfo(current_params,
			&current_params->deviceinfo) != PTP_RC_OK) {
    LIBMTP_ERROR("LIBMTP PANIC: Unable to read device information on device "
	    "%d on bus %d, trying to continue",
	    rawdevice->devnum, rawdevice->bus_location);

    /* Prevent memory leaks for this device */
    free(mtp_device->usbinfo);
    free(mtp_device->params);
    current_params = NULL;
    free(mtp_device);
    return NULL;
  }

  /* Check: if this is a PTP device, is it really tagged as MTP? */
  if (current_params->deviceinfo.VendorExtensionID != 0x00000006) {
    LIBMTP_ERROR("LIBMTP WARNING: no MTP vendor extension on device "
		 "%d on bus %d",
		 rawdevice->devnum, rawdevice->bus_location);
    LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionID: %08x",
		 current_params->deviceinfo.VendorExtensionID);
    LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionDesc: %s",
		 current_params->deviceinfo.VendorExtensionDesc);
    LIBMTP_ERROR("LIBMTP WARNING: this typically means the device is PTP "
		 "(i.e. a camera) but not an MTP device at all. "
		 "Trying to continue anyway.");
  }

  parse_extension_descriptor(mtp_device,
                             current_params->deviceinfo.VendorExtensionDesc);

  /*
   * Android has a number of bugs, force-assign these bug flags
   * if Android is encountered. Same thing for devices we detect
   * as SONY NWZ Walkmen. I have no clue what "sony.net/WMFU" means
   * I just know only NWZs have it.
   */
  {
    LIBMTP_device_extension_t *tmpext = mtp_device->extensions;
    int is_microsoft_com_wpdna = 0;
    int is_android = 0;
    int is_sony_net_wmfu = 0;
    int is_sonyericsson_com_se = 0;

    /* Loop over extensions and set flags */
    while (tmpext != NULL) {
      if (!strcmp(tmpext->name, "microsoft.com/WPDNA"))
	is_microsoft_com_wpdna = 1;
      if (!strcmp(tmpext->name, "android.com"))
	is_android = 1;
      if (!strcmp(tmpext->name, "sony.net/WMFU"))
	is_sony_net_wmfu = 1;
      if (!strcmp(tmpext->name, "sonyericsson.com/SE"))
	is_sonyericsson_com_se = 1;
      tmpext = tmpext->next;
    }

    /* Check for specific stacks */
    if (is_microsoft_com_wpdna && is_sonyericsson_com_se && !is_android) {
      /*
       * The Aricent stack seems to be detected by providing WPDNA, the SonyEricsson
       * extension and NO Android extension.
       */
      ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_ARICENT_BUGS;
      LIBMTP_INFO("Aricent MTP stack device detected, assigning default bug flags\n");
    }
    else if (is_android) {
      /*
       * If bugs are fixed in later versions, test on tmpext->major, tmpext->minor
       */
      ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_ANDROID_BUGS;
      LIBMTP_INFO("Android device detected, assigning default bug flags\n");
    }
    else if (is_sony_net_wmfu) {
      ptp_usb->rawdevice.device_entry.device_flags |= DEVICE_FLAGS_SONY_NWZ_BUGS;
      LIBMTP_INFO("SONY NWZ device detected, assigning default bug flags\n");
    }
  }

  /*
   * If the OGG or FLAC filetypes are flagged as "unknown", check
   * if the firmware has been updated to actually support it.
   */
  if (FLAG_OGG_IS_UNKNOWN(ptp_usb)) {
    for (i=0;i<current_params->deviceinfo.ImageFormats_len;i++) {
      if (current_params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_OGG) {
        /* This is not unknown anymore, unflag it */
        ptp_usb->rawdevice.device_entry.device_flags &=
          ~DEVICE_FLAG_OGG_IS_UNKNOWN;
        break;
      }
    }
  }
  if (FLAG_FLAC_IS_UNKNOWN(ptp_usb)) {
    for (i=0;i<current_params->deviceinfo.ImageFormats_len;i++) {
      if (current_params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_FLAC) {
        /* This is not unknown anymore, unflag it */
        ptp_usb->rawdevice.device_entry.device_flags &=
          ~DEVICE_FLAG_FLAC_IS_UNKNOWN;
        break;
      }
    }
  }

  /* Determine if the object size supported is 32 or 64 bit wide */
  if (ptp_operation_issupported(current_params,PTP_OC_MTP_GetObjectPropsSupported)) {
    for (i=0;i<current_params->deviceinfo.ImageFormats_len;i++) {
      PTPObjectPropDesc opd;

      if (ptp_mtp_getobjectpropdesc(current_params,
                                    PTP_OPC_ObjectSize,
                                    current_params->deviceinfo.ImageFormats[i],
                                    &opd) != PTP_RC_OK) {
        LIBMTP_ERROR("LIBMTP PANIC: "
                     "could not inspect object property description 0x%04x!\n", current_params->deviceinfo.ImageFormats[i]);
      } else {
        if (opd.DataType == PTP_DTC_UINT32) {
          if (bs == 0) {
            bs = 32;
          } else if (bs != 32) {
            LIBMTP_ERROR("LIBMTP PANIC: "
                         "different objects support different object sizes!\n");
            bs = 0;
            break;
          }
        } else if (opd.DataType == PTP_DTC_UINT64) {
          if (bs == 0) {
            bs = 64;
          } else if (bs != 64) {
            LIBMTP_ERROR("LIBMTP PANIC: "
                         "different objects support different object sizes!\n");
            bs = 0;
            break;
          }
        } else {
          // Ignore if other size.
          LIBMTP_ERROR("LIBMTP PANIC: "
                       "awkward object size data type: %04x\n", opd.DataType);
          bs = 0;
          break;
        }
      }
    }
  }
  if (bs == 0) {
    // Could not detect object bitsize, assume 32 bits
    bs = 32;
  }
  mtp_device->object_bitsize = bs;

  /* No Errors yet for this device */
  mtp_device->errorstack = NULL;

  /* Default Max Battery Level, we will adjust this if possible */
  mtp_device->maximum_battery_level = 100;

  /* Check if device supports reading maximum battery level */
  if(!FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) &&
     ptp_property_issupported( current_params, PTP_DPC_BatteryLevel)) {
    PTPDevicePropDesc dpd;

    /* Try to read maximum battery level */
    if(ptp_getdevicepropdesc(current_params,
			     PTP_DPC_BatteryLevel,
			     &dpd) != PTP_RC_OK) {
      add_error_to_errorstack(mtp_device,
			      LIBMTP_ERROR_CONNECTING,
			      "Unable to read Maximum Battery Level for this "
			      "device even though the device supposedly "
			      "supports this functionality");
    }

    /* TODO: is this appropriate? */
    /* If max battery level is 0 then leave the default, otherwise assign */
    if (dpd.FORM.Range.MaximumValue.u8 != 0) {
      mtp_device->maximum_battery_level = dpd.FORM.Range.MaximumValue.u8;
    }

    ptp_free_devicepropdesc(&dpd);
  }

  /* Set all default folders to 0xffffffffU (root directory) */
  mtp_device->default_music_folder = 0xffffffffU;
  mtp_device->default_playlist_folder = 0xffffffffU;
  mtp_device->default_picture_folder = 0xffffffffU;
  mtp_device->default_video_folder = 0xffffffffU;
  mtp_device->default_organizer_folder = 0xffffffffU;
  mtp_device->default_zencast_folder = 0xffffffffU;
  mtp_device->default_album_folder = 0xffffffffU;
  mtp_device->default_text_folder = 0xffffffffU;

  /* Set initial storage information */
  mtp_device->storage = NULL;
  if (LIBMTP_Get_Storage(mtp_device, LIBMTP_STORAGE_SORTBY_NOTSORTED) == -1) {
    add_error_to_errorstack(mtp_device,
			    LIBMTP_ERROR_GENERAL,
			    "Get Storage information failed.");
    mtp_device->storage = NULL;
  }


  return mtp_device;
}

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device(LIBMTP_raw_device_t *rawdevice)
{
  LIBMTP_mtpdevice_t *mtp_device = LIBMTP_Open_Raw_Device_Uncached(rawdevice);

  if (mtp_device == NULL)
    return NULL;

  /* Check for MTPZ devices. */
  if (use_mtpz) {
    LIBMTP_device_extension_t *tmpext = mtp_device->extensions;

    while (tmpext != NULL) {
      if (!strcmp(tmpext->name, "microsoft.com/MTPZ")) {
	LIBMTP_INFO("MTPZ device detected. Authenticating...\n");
        if (PTP_RC_OK == ptp_mtpz_handshake(mtp_device->params)) {
	  LIBMTP_INFO ("(MTPZ) Successfully authenticated with device.\n");
        } else {
          LIBMTP_INFO ("(MTPZ) Failure - could not authenticate with device.\n");
        }
	break;
      }
      tmpext = tmpext->next;
    }
  }

  // Set up this device as cached
  mtp_device->cached = 1;
  /*
   * Then get the handles and try to locate the default folders.
   * This has the desired side effect of caching all handles from
   * the device which speeds up later operations.
   */
  flush_handles(mtp_device);
  return mtp_device;
}

/**
 * To read events sent by the device, repeatedly call this function from a secondary
 * thread until the return value is < 0.
 *
 * @param device a pointer to the MTP device to poll for events.
 * @param event contains a pointer to be filled in with the event retrieved if the call
 * is successful.
 * @param out1 contains the param1 value from the raw event.
 * @return 0 on success, any other value means the polling loop shall be
 * terminated immediately for this session.
 */
int LIBMTP_Read_Event(LIBMTP_mtpdevice_t *device, LIBMTP_event_t *event, uint32_t *out1)
{
  /*
   * FIXME: Potential race-condition here, if client deallocs device
   * while we're *not* waiting for input. As we'll be waiting for
   * input most of the time, it's unlikely but still worth considering
   * for improvement. Also we cannot affect the state of the cache etc
   * unless we know we are the sole user on the device. A spinlock or
   * mutex in the LIBMTP_mtpdevice_t is needed for this to work.
   */
  PTPParams *params = (PTPParams *) device->params;
  PTPContainer ptp_event;
  uint16_t ret = ptp_usb_event_wait(params, &ptp_event);

  if (ret != PTP_RC_OK) {
    /* Device is closing down or other fatal stuff, exit thread */
    return -1;
  }
  LIBMTP_Handle_Event(&ptp_event, event, out1);
  return 0;
}

void LIBMTP_Handle_Event(PTPContainer *ptp_event,
                         LIBMTP_event_t *event, uint32_t *out1) {
  uint16_t code;
  uint32_t session_id;
  uint32_t param1;

  *event = LIBMTP_EVENT_NONE;

  /* Process the event */
  code = ptp_event->Code;
  session_id = ptp_event->SessionID;
  param1 = ptp_event->Param1;

  switch(code) {
    case PTP_EC_Undefined:
      LIBMTP_INFO("Received event PTP_EC_Undefined in session %u\n", session_id);
      break;
    case PTP_EC_CancelTransaction:
      LIBMTP_INFO("Received event PTP_EC_CancelTransaction in session %u\n", session_id);
      break;
    case PTP_EC_ObjectAdded:
      LIBMTP_INFO("Received event PTP_EC_ObjectAdded in session %u\n", session_id);
      *event = LIBMTP_EVENT_OBJECT_ADDED;
      *out1 = param1;
      break;
    case PTP_EC_ObjectRemoved:
      LIBMTP_INFO("Received event PTP_EC_ObjectRemoved in session %u\n", session_id);
      *event = LIBMTP_EVENT_OBJECT_REMOVED;
      *out1 = param1;
      break;
    case PTP_EC_StoreAdded:
      LIBMTP_INFO("Received event PTP_EC_StoreAdded in session %u\n", session_id);
      /* TODO: rescan storages */
      *event = LIBMTP_EVENT_STORE_ADDED;
      *out1 = param1;
      break;
    case PTP_EC_StoreRemoved:
      LIBMTP_INFO("Received event PTP_EC_StoreRemoved in session %u\n", session_id);
      /* TODO: rescan storages */
      *event = LIBMTP_EVENT_STORE_REMOVED;
      *out1 = param1;
      break;
    case PTP_EC_DevicePropChanged:
      LIBMTP_INFO("Received event PTP_EC_DevicePropChanged in session %u\n", session_id);
      /* TODO: update device properties */
      *event = LIBMTP_EVENT_DEVICE_PROPERTY_CHANGED;
      *out1 = param1;
      break;
    case PTP_EC_ObjectInfoChanged:
      LIBMTP_INFO("Received event PTP_EC_ObjectInfoChanged in session %u\n", session_id);
      /* TODO: rescan object cache or just for this one object */
      break;
    case PTP_EC_DeviceInfoChanged:
      LIBMTP_INFO("Received event PTP_EC_DeviceInfoChanged in session %u\n", session_id);
      /* TODO: update device info */
      break;
    case PTP_EC_RequestObjectTransfer:
      LIBMTP_INFO("Received event PTP_EC_RequestObjectTransfer in session %u\n", session_id);
      break;
    case PTP_EC_StoreFull:
      LIBMTP_INFO("Received event PTP_EC_StoreFull in session %u\n", session_id);
      break;
    case PTP_EC_DeviceReset:
      LIBMTP_INFO("Received event PTP_EC_DeviceReset in session %u\n", session_id);
      break;
    case PTP_EC_StorageInfoChanged :
      LIBMTP_INFO( "Received event PTP_EC_StorageInfoChanged in session %u\n", session_id);
     /* TODO: update storage info */
      break;
    case PTP_EC_CaptureComplete :
      LIBMTP_INFO( "Received event PTP_EC_CaptureComplete in session %u\n", session_id);
      break;
    case PTP_EC_UnreportedStatus :
      LIBMTP_INFO( "Received event PTP_EC_UnreportedStatus in session %u\n", session_id);
      break;
    default :
      LIBMTP_INFO( "Received unknown event in session %u\n", session_id);
      break;
  }
}

static void LIBMTP_Read_Event_Cb(PTPParams *params, uint16_t ret_code,
                                 PTPContainer *ptp_event, void *user_data) {
  event_cb_data_t *data = user_data;
  LIBMTP_event_t event = LIBMTP_EVENT_NONE;
  uint32_t param1 = 0;
  int handler_ret;

  switch (ret_code) {
  case PTP_RC_OK:
    handler_ret = LIBMTP_HANDLER_RETURN_OK;
    LIBMTP_Handle_Event(ptp_event, &event, &param1);
    break;
  case PTP_ERROR_CANCEL:
    handler_ret = LIBMTP_HANDLER_RETURN_CANCEL;
    break;
  default:
    handler_ret = LIBMTP_HANDLER_RETURN_ERROR;
    break;
  }

  data->cb(handler_ret, event, param1, data->user_data);
  free(data);
}

/**
 * This function reads events sent by the device, in a non-blocking manner.
 * The callback function will be called when an event is received, but for the function
 * to make progress, polling must take place, using LIBMTP_Handle_Events_Timeout_Completed.
 *
 * After an event is received, this function should be called again to listen for the next
 * event.
 *
 * For now, this non-blocking mechanism only works with libusb-1.0, and not any of the
 * other usb library backends. Attempting to call this method with another backend will
 * always return an error.
 *
 * @param device a pointer to the MTP device to poll for events.
 * @param cb a callback to be invoked when an event is received.
 * @param user_data arbitrary user data passed to the callback.
 * @return 0 on success, any other value means that the callback was not registered and
 *         no event notification will take place.
 */
int LIBMTP_Read_Event_Async(LIBMTP_mtpdevice_t *device, LIBMTP_event_cb_fn cb, void *user_data) {
  PTPParams *params = (PTPParams *) device->params;
  event_cb_data_t *data =  malloc(sizeof(event_cb_data_t));
  uint16_t ret;

  data->cb = cb;
  data->user_data = user_data;

  ret = ptp_usb_event_async(params, LIBMTP_Read_Event_Cb, data);
  return ret == PTP_RC_OK ? 0 : -1;
}

/**
 * Recursive function that adds MTP devices to a linked list
 * @param devices a list of raw devices to have real devices created for.
 * @return a device pointer to a newly created mtpdevice (used in linked
 * list creation).
 */
static LIBMTP_mtpdevice_t * create_usb_mtp_devices(LIBMTP_raw_device_t *devices, unsigned int numdevs)
{
  unsigned int i;
  LIBMTP_mtpdevice_t *mtp_device_list = NULL;
  LIBMTP_mtpdevice_t *current_device = NULL;

  for (i=0; i < numdevs; i++) {
    LIBMTP_mtpdevice_t *mtp_device;
    mtp_device = LIBMTP_Open_Raw_Device(&devices[i]);

    /* On error, try next device */
    if (mtp_device == NULL)
      continue;

    /* Add the device to the list */
    mtp_device->next = NULL;
    if (mtp_device_list == NULL) {
      mtp_device_list = current_device = mtp_device;
    } else {
      current_device->next = mtp_device;
      current_device = mtp_device;
    }
  }
  return mtp_device_list;
}

/**
 * Get the number of devices that are available in the listed device list
 * @param device_list Pointer to a linked list of devices
 * @return Number of devices in the device list device_list
 * @see LIBMTP_Get_Connected_Devices()
 */
uint32_t LIBMTP_Number_Devices_In_List(LIBMTP_mtpdevice_t *device_list)
{
  uint32_t numdevices = 0;
  LIBMTP_mtpdevice_t *iter;
  for(iter = device_list; iter != NULL; iter = iter->next)
    numdevices++;

  return numdevices;
}

/**
 * Get the first connected MTP device node in the linked list of devices.
 * Currently this only provides access to USB devices
 * @param device_list A list of devices ready to be used by the caller. You
 *        need to know how many there are.
 * @return Any error information gathered from device connections
 * @see LIBMTP_Number_Devices_In_List()
 */
LIBMTP_error_number_t LIBMTP_Get_Connected_Devices(LIBMTP_mtpdevice_t **device_list)
{
  LIBMTP_raw_device_t *devices;
  int numdevs;
  LIBMTP_error_number_t ret;

  ret = LIBMTP_Detect_Raw_Devices(&devices, &numdevs);
  if (ret != LIBMTP_ERROR_NONE) {
    *device_list = NULL;
    return ret;
  }

  /* Assign linked list of devices */
  if (devices == NULL || numdevs == 0) {
    *device_list = NULL;
    free(devices);
    return LIBMTP_ERROR_NO_DEVICE_ATTACHED;
  }

  *device_list = create_usb_mtp_devices(devices, numdevs);
  free(devices);

  /* TODO: Add wifi device access here */

  /* We have found some devices but create failed */
  if (*device_list == NULL)
    return LIBMTP_ERROR_CONNECTING;

  return LIBMTP_ERROR_NONE;
}

/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device_List(LIBMTP_mtpdevice_t *device)
{
  if(device != NULL)
  {
    if(device->next != NULL)
    {
      LIBMTP_Release_Device_List(device->next);
    }

    LIBMTP_Release_Device(device);
  }
}

/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  close_device(ptp_usb, params);
  // Clear error stack
  LIBMTP_Clear_Errorstack(device);
#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_H)
  iconv_close(params->cd_locale_to_ucs2);
  iconv_close(params->cd_ucs2_to_locale);
#endif
  free(ptp_usb);
  ptp_free_params(params);
  free(params);
  free_storage_list(device);
  // Free extension list...
  if (device->extensions != NULL) {
    LIBMTP_device_extension_t *tmp = device->extensions;

    while (tmp != NULL) {
      LIBMTP_device_extension_t *next = tmp->next;

      if (tmp->name)
        free(tmp->name);
      free(tmp);
      tmp = next;
    }
  }
  free(device);
}

/**
 * This can be used by any libmtp-intrinsic code that
 * need to stack up an error on the stack. You are only
 * supposed to add errors to the error stack using this
 * function, do not create and reference error entries
 * directly.
 */
static void add_error_to_errorstack(LIBMTP_mtpdevice_t *device,
				    LIBMTP_error_number_t errornumber,
				    char const * const error_text)
{
  LIBMTP_error_t *newerror;

  if (device == NULL) {
    LIBMTP_ERROR("LIBMTP PANIC: Trying to add error to a NULL device!\n");
    return;
  }
  newerror = (LIBMTP_error_t *) malloc(sizeof(LIBMTP_error_t));
  newerror->errornumber = errornumber;
  newerror->error_text = strdup(error_text);
  newerror->next = NULL;
  if (device->errorstack == NULL) {
    device->errorstack = newerror;
  } else {
    LIBMTP_error_t *tmp = device->errorstack;

    while (tmp->next != NULL) {
      tmp = tmp->next;
    }
    tmp->next = newerror;
  }
}

/**
 * Adds an error from the PTP layer to the error stack.
 */
static void add_ptp_error_to_errorstack(LIBMTP_mtpdevice_t *device,
					uint16_t ptp_error,
					char const * const error_text)
{
  if (device == NULL) {
    LIBMTP_ERROR("LIBMTP PANIC: Trying to add PTP error to a NULL device!\n");
    return;
  } else {
    PTPParams      *params = (PTPParams *) device->params;
    char outstr[256];
    snprintf(outstr, sizeof(outstr), "PTP Layer error %04x: %s", ptp_error, error_text);
    outstr[sizeof(outstr)-1] = '\0';
    add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, outstr);

    snprintf(outstr, sizeof(outstr), "Error %04x: %s", ptp_error, ptp_strerror(ptp_error, params->deviceinfo.VendorExtensionID));
    outstr[sizeof(outstr)-1] = '\0';
    add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, outstr);
  }
}

/**
 * This returns the error stack for a device in case you
 * need to either reference the error numbers (e.g. when
 * creating multilingual apps with multiple-language text
 * representations for each error number) or when you need
 * to build a multi-line error text widget or something like
 * that. You need to call the <code>LIBMTP_Clear_Errorstack</code>
 * to clear it when you're finished with it.
 * @param device a pointer to the MTP device to get the error
 *        stack for.
 * @return the error stack or NULL if there are no errors
 *         on the stack.
 * @see LIBMTP_Clear_Errorstack()
 * @see LIBMTP_Dump_Errorstack()
 */
LIBMTP_error_t *LIBMTP_Get_Errorstack(LIBMTP_mtpdevice_t *device)
{
  if (device == NULL) {
    LIBMTP_ERROR("LIBMTP PANIC: Trying to get the error stack of a NULL device!\n");
    return NULL;
  }
  return device->errorstack;
}

/**
 * This function clears the error stack of a device and frees
 * any memory used by it. Call this when you're finished with
 * using the errors.
 * @param device a pointer to the MTP device to clear the error
 *        stack for.
 */
void LIBMTP_Clear_Errorstack(LIBMTP_mtpdevice_t *device)
{
  if (device == NULL) {
    LIBMTP_ERROR("LIBMTP PANIC: Trying to clear the error stack of a NULL device!\n");
  } else {
    LIBMTP_error_t *tmp = device->errorstack;

    while (tmp != NULL) {
      LIBMTP_error_t *tmp2;

      if (tmp->error_text != NULL) {
	free(tmp->error_text);
      }
      tmp2 = tmp;
      tmp = tmp->next;
      free(tmp2);
    }
    device->errorstack = NULL;
  }
}

/**
 * This function dumps the error stack to <code>stderr</code>.
 * (You still have to clear the stack though.)
 * @param device a pointer to the MTP device to dump the error
 *        stack for.
 */
void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *device)
{
  if (device == NULL) {
    LIBMTP_ERROR("LIBMTP PANIC: Trying to dump the error stack of a NULL device!\n");
  } else {
    LIBMTP_error_t *tmp = device->errorstack;

    while (tmp != NULL) {
      if (tmp->error_text != NULL) {
	LIBMTP_ERROR("Error %d: %s\n", tmp->errornumber, tmp->error_text);
      } else {
	LIBMTP_ERROR("Error %d: (unknown)\n", tmp->errornumber);
      }
      tmp = tmp->next;
    }
  }
}

/**
 * This command gets all handles and stuff by FAST directory retrieveal
 * which is available by getting all metadata for object
 * <code>0xffffffff</code> which simply means "all metadata for all objects".
 * This works on the vast majority of MTP devices (there ARE exceptions!)
 * and is quite quick. Check the error stack to see if there were
 * problems getting the metadata.
 * @return 0 if all was OK, -1 on failure.
 */
static int get_all_metadata_fast(LIBMTP_mtpdevice_t *device)
{
  PTPParams      *params = (PTPParams *) device->params;
  int		 cnt = 0;
  int            i, j, nrofprops;
  uint32_t	 lasthandle = 0xffffffff;
  MTPProperties  *props = NULL;
  MTPProperties  *prop;
  uint16_t       ret;
  int            oldtimeout;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  /*
   * The follow request causes the device to generate
   * a list of every file on the device and return it
   * in a single response.
   *
   * Some slow devices as well as devices with very
   * large file systems can easily take longer then
   * the standard timeout value before it is able
   * to return a response.
   *
   * Temporarly set timeout to allow working with
   * widest range of devices.
   */
  get_usb_device_timeout(ptp_usb, &oldtimeout);
  set_usb_device_timeout(ptp_usb, 60000);

  ret = ptp_mtp_getobjectproplist(params, 0xffffffff, &props, &nrofprops);
  set_usb_device_timeout(ptp_usb, oldtimeout);

  if (ret == PTP_RC_MTP_Specification_By_Group_Unsupported) {
    // What's the point in the device implementing this command if
    // you cannot use it to get all props for AT LEAST one object?
    // Well, whatever...
    add_ptp_error_to_errorstack(device, ret, "get_all_metadata_fast(): "
    "cannot retrieve all metadata for an object on this device.");
    return -1;
  }
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "get_all_metadata_fast(): "
    "could not get proplist of all objects.");
    return -1;
  }
  if (props == NULL && nrofprops != 0) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "get_all_metadata_fast(): "
			    "call to ptp_mtp_getobjectproplist() returned "
			    "inconsistent results.");
    return -1;
  }
  /*
   * We count the number of objects by counting the ObjectHandle
   * references, whenever it changes we get a new object, when it's
   * the same, it is just different properties of the same object.
   */
  prop = props;
  for (i=0;i<nrofprops;i++) {
      if (lasthandle != prop->ObjectHandle) {
	cnt++;
	lasthandle = prop->ObjectHandle;
      }
      prop++;
  }
  lasthandle = 0xffffffff;
  params->objects = calloc (cnt, sizeof(PTPObject));
  prop = props;
  i = -1;
  for (j=0;j<nrofprops;j++) {
    if (lasthandle != prop->ObjectHandle) {
      if (i >= 0) {
        params->objects[i].flags |= PTPOBJECT_OBJECTINFO_LOADED;
	if (!params->objects[i].oi.Filename) {
	  /* I have one such file on my Creative (Marcus) */
	  params->objects[i].oi.Filename = strdup("<null>");
	}
      }
      i++;
      lasthandle = prop->ObjectHandle;
      params->objects[i].oid = prop->ObjectHandle;
    }
    switch (prop->property) {
    case PTP_OPC_ParentObject:
      params->objects[i].oi.ParentObject = prop->propval.u32;
      params->objects[i].flags |= PTPOBJECT_PARENTOBJECT_LOADED;
      break;
    case PTP_OPC_ObjectFormat:
      params->objects[i].oi.ObjectFormat = prop->propval.u16;
      break;
    case PTP_OPC_ObjectSize:
      // We loose precision here, up to 32 bits! However the commands that
      // retrieve metadata for files and tracks will make sure that the
      // PTP_OPC_ObjectSize is read in and duplicated again.
      if (device->object_bitsize == 64) {
	params->objects[i].oi.ObjectCompressedSize = (uint32_t) prop->propval.u64;
      } else {
	params->objects[i].oi.ObjectCompressedSize = prop->propval.u32;
      }
      break;
    case PTP_OPC_StorageID:
      params->objects[i].oi.StorageID = prop->propval.u32;
      params->objects[i].flags |= PTPOBJECT_STORAGEID_LOADED;
      break;
    case PTP_OPC_ObjectFileName:
      if (prop->propval.str != NULL)
        params->objects[i].oi.Filename = strdup(prop->propval.str);
      break;
    default: {
      MTPProperties *newprops;

      /* Copy all of the other MTP oprierties into the per-object proplist */
      if (params->objects[i].nrofmtpprops) {
        newprops = realloc(params->objects[i].mtpprops,
		(params->objects[i].nrofmtpprops+1)*sizeof(MTPProperties));
      } else {
        newprops = calloc(1,sizeof(MTPProperties));
      }
      if (!newprops) return 0; /* FIXME: error handling? */
      params->objects[i].mtpprops = newprops;
      memcpy(&params->objects[i].mtpprops[params->objects[i].nrofmtpprops],
	     &props[j],sizeof(props[j]));
      params->objects[i].nrofmtpprops++;
      params->objects[i].flags |= PTPOBJECT_MTPPROPLIST_LOADED;
      break;
    }
    }
    prop++;
  }
  /* mark last entry also */
  if (i >= 0) {
    params->objects[i].flags |= PTPOBJECT_OBJECTINFO_LOADED;
    params->nrofobjects = i+1;
  } else {
    params->nrofobjects = 0;
  }
  free (props);
  /* The device might not give the list in linear ascending order */
  ptp_objects_sort (params);
  return 0;
}

/**
 * This function will recurse through all the directories on the device,
 * starting at the root directory, gathering metadata as it moves along.
 * It works better on some devices that will only return data for a
 * certain directory and does not respect the option to get all metadata
 * for all objects.
 */
static uint16_t get_handles_recursively(LIBMTP_mtpdevice_t *device,
				    PTPParams *params,
				    uint32_t storageid,
				    uint32_t parent)
{
  PTPObjectHandles currentHandles;
  unsigned int i = 0;
  uint16_t ret = ptp_getobjecthandles(params,
                                      storageid,
                                      PTP_GOH_ALL_FORMATS,
                                      parent,
                                      &currentHandles);

  if (ret != PTP_RC_OK) {
    char buf[80];
    sprintf(buf,"get_handles_recursively(): could not get object handles of %08x", parent);
    add_ptp_error_to_errorstack(device, ret, buf);
    return ret;
  }

  if (currentHandles.Handler == NULL || currentHandles.n == 0)
    return PTP_RC_OK;

  // Now descend into any subdirectories found
  for (i = 0; i < currentHandles.n; i++) {
    PTPObject *ob;
    ret = ptp_object_want(params,currentHandles.Handler[i],
			  PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret == PTP_RC_OK) {
      if (ob->oi.ObjectFormat == PTP_OFC_Association)
        get_handles_recursively(device, params,
				storageid, currentHandles.Handler[i]);
    } else {
      add_error_to_errorstack(device,
			      LIBMTP_ERROR_CONNECTING,
			      "Found a bad handle, trying to ignore it.");
      return ret;
    }
  }
  free(currentHandles.Handler);
  return PTP_RC_OK;
}

/**
 * This function refresh the internal handle list whenever
 * the items stored inside the device is altered. On operations
 * that do not add or remove objects, this is typically not
 * called.
 * @param device a pointer to the MTP device to flush handles for.
 */
static void flush_handles(LIBMTP_mtpdevice_t *device)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  int ret;
  uint32_t i;

  if (!device->cached) {
    return;
  }

  if (params->objects != NULL) {
    for (i=0;i<params->nrofobjects;i++)
      ptp_free_object (&params->objects[i]);
    free(params->objects);
    params->objects = NULL;
    params->nrofobjects = 0;
  }

  if (ptp_operation_issupported(params,PTP_OC_MTP_GetObjPropList)
      && !FLAG_BROKEN_MTPGETOBJPROPLIST(ptp_usb)
      && !FLAG_BROKEN_MTPGETOBJPROPLIST_ALL(ptp_usb)) {
    // Use the fast method. Ignore return value for now.
    ret = get_all_metadata_fast(device);
  }

  // If the previous failed or returned no objects, use classic
  // methods instead.
  if (params->nrofobjects == 0) {
    // Get all the handles using just standard commands.
    if (device->storage == NULL) {
      get_handles_recursively(device, params,
			      PTP_GOH_ALL_STORAGE,
			      PTP_GOH_ROOT_PARENT);
    } else {
      // Get handles for each storage in turn.
      LIBMTP_devicestorage_t *storage = device->storage;
      while(storage != NULL) {
	get_handles_recursively(device, params,
				storage->id,
				PTP_GOH_ROOT_PARENT);
	storage = storage->next;
      }
    }
  }

  /*
   * Loop over the handles, fix up any NULL filenames or
   * keywords, then attempt to locate some default folders
   * in the root directory of the primary storage.
   */
  for(i = 0; i < params->nrofobjects; i++) {
    PTPObject *ob, *xob;

    ob = &params->objects[i];
    ret = ptp_object_want(params,params->objects[i].oid,
			  PTPOBJECT_OBJECTINFO_LOADED, &xob);
    if (ret != PTP_RC_OK) {
	LIBMTP_ERROR("broken! %x not found\n", params->objects[i].oid);
    }
    if (ob->oi.Filename == NULL)
      ob->oi.Filename = strdup("<null>");
    if (ob->oi.Keywords == NULL)
      ob->oi.Keywords = strdup("<null>");

    /* Ignore handles that point to non-folders */
    if(ob->oi.ObjectFormat != PTP_OFC_Association)
      continue;
    /* Only look in the root folder */
    if (ob->oi.ParentObject == 0xffffffffU) {
      LIBMTP_ERROR("object %x has parent 0xffffffff (-1) continuing anyway\n",
		   ob->oid);
    } else if (ob->oi.ParentObject != 0x00000000U)
      continue;
    /* Only look in the primary storage */
    if (device->storage != NULL && ob->oi.StorageID != device->storage->id)
      continue;

    /* Is this the Music Folder */
    if (!strcasecmp(ob->oi.Filename, "My Music") ||
	!strcasecmp(ob->oi.Filename, "My_Music") ||
	!strcasecmp(ob->oi.Filename, "Music")) {
      device->default_music_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "My Playlists") ||
	     !strcasecmp(ob->oi.Filename, "My_Playlists") ||
	     !strcasecmp(ob->oi.Filename, "Playlists")) {
      device->default_playlist_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "My Pictures") ||
	     !strcasecmp(ob->oi.Filename, "My_Pictures") ||
	     !strcasecmp(ob->oi.Filename, "Pictures")) {
      device->default_picture_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "My Video") ||
	     !strcasecmp(ob->oi.Filename, "My_Video") ||
	     !strcasecmp(ob->oi.Filename, "Video")) {
	device->default_video_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "My Organizer") ||
	     !strcasecmp(ob->oi.Filename, "My_Organizer")) {
      device->default_organizer_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "ZENcast") ||
	     !strcasecmp(ob->oi.Filename, "Datacasts")) {
      device->default_zencast_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "My Albums") ||
	     !strcasecmp(ob->oi.Filename, "My_Albums") ||
	     !strcasecmp(ob->oi.Filename, "Albums")) {
      device->default_album_folder = ob->oid;
    }
    else if (!strcasecmp(ob->oi.Filename, "Text") ||
	     !strcasecmp(ob->oi.Filename, "Texts")) {
      device->default_text_folder = ob->oid;
    }
  }
}

/**
 * This function traverses a devices storage list freeing up the
 * strings and the structs.
 * @param device a pointer to the MTP device to free the storage
 * list for.
 */
static void free_storage_list(LIBMTP_mtpdevice_t *device)
{
  LIBMTP_devicestorage_t *storage;
  LIBMTP_devicestorage_t *tmp;

  storage = device->storage;
  while(storage != NULL) {
    if (storage->StorageDescription != NULL) {
      free(storage->StorageDescription);
    }
    if (storage->VolumeIdentifier != NULL) {
      free(storage->VolumeIdentifier);
    }
    tmp = storage;
    storage = storage->next;
    free(tmp);
  }
  device->storage = NULL;

  return;
}

/**
 * This function sorts a devices storage list according to the criteria passed.
 * @param device a pointer to the MTP device to free the storage
 * @param sortby LIBMTP_STORAGE_SORTBY_* flag to sort the list for (either by free space or maximum space)
 */
static int sort_storage_by(LIBMTP_mtpdevice_t *device,int const sortby)
{
  LIBMTP_devicestorage_t *oldhead, *ptr1, *ptr2, *newlist;

  if (device->storage == NULL)
    return -1;
  if (sortby == LIBMTP_STORAGE_SORTBY_NOTSORTED)
    return 0;

  oldhead = ptr1 = ptr2 = device->storage;

  newlist = NULL;

  while(oldhead != NULL) {
    ptr1 = ptr2 = oldhead;
    while(ptr1 != NULL) {

      if (sortby == LIBMTP_STORAGE_SORTBY_FREESPACE && ptr1->FreeSpaceInBytes > ptr2->FreeSpaceInBytes)
        ptr2 = ptr1;
      if (sortby == LIBMTP_STORAGE_SORTBY_MAXSPACE && ptr1->MaxCapacity > ptr2->MaxCapacity)
        ptr2 = ptr1;

      ptr1 = ptr1->next;
    }

    // Make our previous entries next point to our next
    if(ptr2->prev != NULL) {
      ptr1 = ptr2->prev;
      ptr1->next = ptr2->next;
    } else {
      oldhead = ptr2->next;
      if(oldhead != NULL)
        oldhead->prev = NULL;
    }

    // Make our next entries previous point to our previous
    ptr1 = ptr2->next;
    if(ptr1 != NULL) {
      ptr1->prev = ptr2->prev;
    } else {
      ptr1 = ptr2->prev;
      if(ptr1 != NULL)
        ptr1->next = NULL;
    }

    if(newlist == NULL) {
      newlist = ptr2;
      newlist->prev = NULL;
    } else {
      ptr2->prev = newlist;
      newlist->next = ptr2;
      newlist = newlist->next;
    }
  }

  if (newlist != NULL) {
    newlist->next = NULL;
    while(newlist->prev != NULL)
      newlist = newlist->prev;
    device->storage = newlist;
  }

  return 0;
}

/**
 * This function grabs the first writeable storageid from the
 * device storage list.
 * @param device a pointer to the MTP device to locate writeable
 *        storage for.
 * @param fitsize a file of this file must fit on the device.
 */
static uint32_t get_writeable_storageid(LIBMTP_mtpdevice_t *device,
					uint64_t fitsize)
{
  LIBMTP_devicestorage_t *storage;
  uint32_t store = 0x00000000; //Should this be 0xffffffffu instead?
  int subcall_ret;

  // See if there is some storage we can fit this file on.
  storage = device->storage;
  if (storage == NULL) {
    // Sometimes the storage just cannot be detected.
    store = 0x00000000U;
  } else {
    while(storage != NULL) {
      // These storages cannot be used.
      if (storage->StorageType == PTP_ST_FixedROM ||
	  storage->StorageType == PTP_ST_RemovableROM) {
	storage = storage->next;
	continue;
      }
      // Storage IDs with the lower 16 bits 0x0000 are not supposed
      // to be writeable.
      if ((storage->id & 0x0000FFFFU) == 0x00000000U) {
	storage = storage->next;
	continue;
      }
      // Also check the access capability to avoid e.g. deletable only storages
      if (storage->AccessCapability == PTP_AC_ReadOnly ||
	  storage->AccessCapability == PTP_AC_ReadOnly_with_Object_Deletion) {
	storage = storage->next;
	continue;
      }
      // Then see if we can fit the file.
      subcall_ret = check_if_file_fits(device, storage, fitsize);
      if (subcall_ret != 0) {
	storage = storage->next;
      } else {
	// We found a storage that is writable and can fit the file!
	break;
      }
    }
    if (storage == NULL) {
      add_error_to_errorstack(device, LIBMTP_ERROR_STORAGE_FULL,
			      "get_writeable_storageid(): "
			      "all device storage is full or corrupt.");
      return -1;
    }
    store = storage->id;
  }

  return store;
}

/**
 * Tries to suggest a storage_id of a given ID when we have a parent
 * @param device a pointer to the device where to search for the storage ID
 * @param fitsize a file of this file must fit on the device.
 * @param parent_id look for this ID
 * @ret storageID
 */
static int get_suggested_storage_id(LIBMTP_mtpdevice_t *device,
				    uint64_t fitsize,
				    uint32_t parent_id)
{
  PTPParams *params = (PTPParams *) device->params;
  PTPObject *ob;
  uint16_t ret;

  ret = ptp_object_want(params, parent_id, PTPOBJECT_MTPPROPLIST_LOADED, &ob);
  if ((ret != PTP_RC_OK) || (ob->oi.StorageID == 0)) {
    add_ptp_error_to_errorstack(device, ret, "get_suggested_storage_id(): "
				"could not get storage id from parent id.");
    return get_writeable_storageid(device, fitsize);
  } else {
    /* OK we know the parent storage, then use that */
    return ob->oi.StorageID;
  }
}

/**
 * This function grabs the freespace from a certain storage in
 * device storage list.
 * @param device a pointer to the MTP device to free the storage
 * list for.
 * @param storage a pointer to the storage to flush and
 * get free space for.
 * @param freespace the free space on this storage will be returned
 * in this variable.
 */
static int get_storage_freespace(LIBMTP_mtpdevice_t *device,
				 LIBMTP_devicestorage_t *storage,
				 uint64_t *freespace)
{
  PTPParams *params = (PTPParams *) device->params;

  // Always query the device about this, since some models explicitly
  // needs that. We flush all data on queries storage here.
  if (ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
    PTPStorageInfo storageInfo;
    uint16_t ret;

    ret = ptp_getstorageinfo(params, storage->id, &storageInfo);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret,
		"get_storage_freespace(): could not get storage info.");
      return -1;
    }
    if (storage->StorageDescription != NULL) {
      free(storage->StorageDescription);
    }
    if (storage->VolumeIdentifier != NULL) {
      free(storage->VolumeIdentifier);
    }
    storage->StorageType = storageInfo.StorageType;
    storage->FilesystemType = storageInfo.FilesystemType;
    storage->AccessCapability = storageInfo.AccessCapability;
    storage->MaxCapacity = storageInfo.MaxCapability;
    storage->FreeSpaceInBytes = storageInfo.FreeSpaceInBytes;
    storage->FreeSpaceInObjects = storageInfo.FreeSpaceInImages;
    storage->StorageDescription = storageInfo.StorageDescription;
    storage->VolumeIdentifier = storageInfo.VolumeLabel;
  }
  if(storage->FreeSpaceInBytes == (uint64_t) -1)
    return -1;
  *freespace = storage->FreeSpaceInBytes;
  return 0;
}

/**
 * This function dumps out a large chunk of textual information
 * provided from the PTP protocol and additionally some extra
 * MTP-specific information where applicable.
 * @param device a pointer to the MTP device to report info from.
 */
void LIBMTP_Dump_Device_Info(LIBMTP_mtpdevice_t *device)
{
  unsigned int i;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  LIBMTP_devicestorage_t *storage = device->storage;
  LIBMTP_device_extension_t *tmpext = device->extensions;

  printf("USB low-level info:\n");
  dump_usbinfo(ptp_usb);
  /* Print out some verbose information */
  printf("Device info:\n");
  printf("   Manufacturer: %s\n", params->deviceinfo.Manufacturer);
  printf("   Model: %s\n", params->deviceinfo.Model);
  printf("   Device version: %s\n", params->deviceinfo.DeviceVersion);
  printf("   Serial number: %s\n", params->deviceinfo.SerialNumber);
  printf("   Vendor extension ID: 0x%08x\n",
	 params->deviceinfo.VendorExtensionID);
  printf("   Vendor extension description: %s\n",
	 params->deviceinfo.VendorExtensionDesc);
  printf("   Detected object size: %d bits\n",
	 device->object_bitsize);
  printf("   Extensions:\n");
  while (tmpext != NULL) {
    printf("        %s: %d.%d\n",
           tmpext->name,
           tmpext->major,
           tmpext->minor);
    tmpext = tmpext->next;
  }
  printf("Supported operations:\n");
  for (i=0;i<params->deviceinfo.OperationsSupported_len;i++)
    printf("   %04x: %s\n", params->deviceinfo.OperationsSupported[i], ptp_get_opcode_name(params, params->deviceinfo.OperationsSupported[i]));
  printf("Events supported:\n");
  if (params->deviceinfo.EventsSupported_len == 0) {
    printf("   None.\n");
  } else {
    for (i=0;i<params->deviceinfo.EventsSupported_len;i++) {
      printf("   0x%04x: %s\n", params->deviceinfo.EventsSupported[i], ptp_get_event_code_name(params, params->deviceinfo.EventsSupported[i]));
    }
  }
  printf("Device Properties Supported:\n");
  for (i=0;i<params->deviceinfo.DevicePropertiesSupported_len;i++) {
    char const *propdesc = ptp_get_property_description(params,
			params->deviceinfo.DevicePropertiesSupported[i]);

    if (propdesc != NULL) {
      printf("   0x%04x: %s\n",
	     params->deviceinfo.DevicePropertiesSupported[i], propdesc);
    } else {
      uint16_t prop = params->deviceinfo.DevicePropertiesSupported[i];
      printf("   0x%04x: Unknown property\n", prop);
    }
  }

  if (ptp_operation_issupported(params,PTP_OC_MTP_GetObjectPropsSupported)) {
    printf("Playable File (Object) Types and Object Properties Supported:\n");
    for (i=0;i<params->deviceinfo.ImageFormats_len;i++) {
      char txt[256];
      uint16_t ret;
      uint16_t *props = NULL;
      uint32_t propcnt = 0;
      unsigned int j;

      (void) ptp_render_ofc (params, params->deviceinfo.ImageFormats[i],
			     sizeof(txt), txt);
      printf("   %04x: %s\n", params->deviceinfo.ImageFormats[i], txt);

      ret = ptp_mtp_getobjectpropssupported (params,
			params->deviceinfo.ImageFormats[i], &propcnt, &props);
      if (ret != PTP_RC_OK) {
	add_ptp_error_to_errorstack(device, ret, "LIBMTP_Dump_Device_Info(): "
				    "error on query for object properties.");
      } else {
	for (j=0;j<propcnt;j++) {
	  PTPObjectPropDesc opd;
	  int k;

	  printf("      %04x: %s", props[j],
		 LIBMTP_Get_Property_Description(map_ptp_property_to_libmtp_property(props[j])));
	  // Get a more verbose description
	  ret = ptp_mtp_getobjectpropdesc(params, props[j],
					  params->deviceinfo.ImageFormats[i],
					  &opd);
	  if (ret != PTP_RC_OK) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
				    "LIBMTP_Dump_Device_Info(): "
				    "could not get property description.");
	    break;
	  }

	  if (opd.DataType == PTP_DTC_STR) {
	    printf(" STRING data type");
	    switch (opd.FormFlag) {
	    case PTP_OPFF_DateTime:
	      printf(" DATETIME FORM (%s)", opd.FORM.DateTime.String);
	      break;
	    case PTP_OPFF_RegularExpression:
	      printf(" REGULAR EXPRESSION FORM (%s)", opd.FORM.RegularExpression.String);
	      break;
	    case PTP_OPFF_LongString:
	      printf(" LONG STRING FORM");
	      break;
	    default:
	      break;
	    }
	  } else {
	    if (opd.DataType & PTP_DTC_ARRAY_MASK) {
	      printf(" array of");
	    }

	    switch (opd.DataType & (~PTP_DTC_ARRAY_MASK)) {

	    case PTP_DTC_UNDEF:
	      printf(" UNDEFINED data type");
	      break;
	    case PTP_DTC_INT8:
	      printf(" INT8 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
		printf(" range: MIN %d, MAX %d, STEP %d",
		       opd.FORM.Range.MinimumValue.i8,
		       opd.FORM.Range.MaximumValue.i8,
		       opd.FORM.Range.StepSize.i8);
		break;
	      case PTP_OPFF_Enumeration:
		printf(" enumeration: ");
		for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		  printf("%d, ", opd.FORM.Enum.SupportedValue[k].i8);
		}
		break;
	      case PTP_OPFF_ByteArray:
		printf(" byte array: ");
		break;
	      default:
		printf(" ANY 8BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_UINT8:
	      printf(" UINT8 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
		printf(" range: MIN %d, MAX %d, STEP %d",
		       opd.FORM.Range.MinimumValue.u8,
		       opd.FORM.Range.MaximumValue.u8,
		       opd.FORM.Range.StepSize.u8);
		break;
	      case PTP_OPFF_Enumeration:
		printf(" enumeration: ");
		for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		  printf("%d, ", opd.FORM.Enum.SupportedValue[k].u8);
		}
		break;
	      case PTP_OPFF_ByteArray:
		printf(" byte array: ");
		break;
	      default:
		printf(" ANY 8BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_INT16:
	      printf(" INT16 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
	      printf(" range: MIN %d, MAX %d, STEP %d",
		     opd.FORM.Range.MinimumValue.i16,
		     opd.FORM.Range.MaximumValue.i16,
		     opd.FORM.Range.StepSize.i16);
	      break;
	      case PTP_OPFF_Enumeration:
		printf(" enumeration: ");
		for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		  printf("%d, ", opd.FORM.Enum.SupportedValue[k].i16);
		}
		break;
	      default:
		printf(" ANY 16BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_UINT16:
	      printf(" UINT16 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
		printf(" range: MIN %d, MAX %d, STEP %d",
		       opd.FORM.Range.MinimumValue.u16,
		       opd.FORM.Range.MaximumValue.u16,
		       opd.FORM.Range.StepSize.u16);
		break;
	      case PTP_OPFF_Enumeration:
		printf(" enumeration: ");
		for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		  printf("%d, ", opd.FORM.Enum.SupportedValue[k].u16);
		}
		break;
	      default:
		printf(" ANY 16BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_INT32:
	      printf(" INT32 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
		printf(" range: MIN %d, MAX %d, STEP %d",
		       opd.FORM.Range.MinimumValue.i32,
		       opd.FORM.Range.MaximumValue.i32,
		       opd.FORM.Range.StepSize.i32);
		break;
	      case PTP_OPFF_Enumeration:
		printf(" enumeration: ");
		for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		  printf("%d, ", opd.FORM.Enum.SupportedValue[k].i32);
		}
		break;
	      default:
		printf(" ANY 32BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_UINT32:
	      printf(" UINT32 data type");
	      switch (opd.FormFlag) {
	      case PTP_OPFF_Range:
		printf(" range: MIN %u, MAX %u, STEP %u",
		       opd.FORM.Range.MinimumValue.u32,
		       opd.FORM.Range.MaximumValue.u32,
		       opd.FORM.Range.StepSize.u32);
		break;
	      case PTP_OPFF_Enumeration:
		// Special pretty-print for FOURCC codes
		if (params->deviceinfo.ImageFormats[i] == PTP_OPC_VideoFourCCCodec) {
		  printf(" enumeration of u32 casted FOURCC: ");
		  for (k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		    if (opd.FORM.Enum.SupportedValue[k].u32 == 0) {
		      printf("ANY, ");
		    } else {
		      char fourcc[6];
		      fourcc[0] = (opd.FORM.Enum.SupportedValue[k].u32 >> 24) & 0xFFU;
		      fourcc[1] = (opd.FORM.Enum.SupportedValue[k].u32 >> 16) & 0xFFU;
		      fourcc[2] = (opd.FORM.Enum.SupportedValue[k].u32 >> 8) & 0xFFU;
		      fourcc[3] = opd.FORM.Enum.SupportedValue[k].u32 & 0xFFU;
		      fourcc[4] = '\n';
		      fourcc[5] = '\0';
		      printf("\"%s\", ", fourcc);
		    }
		  }
		} else {
		  printf(" enumeration: ");
		  for(k=0;k<opd.FORM.Enum.NumberOfValues;k++) {
		    printf("%u, ", opd.FORM.Enum.SupportedValue[k].u32);
		  }
		}
		break;
	      default:
		printf(" ANY 32BIT VALUE form");
		break;
	      }
	      break;

	    case PTP_DTC_INT64:
	      printf(" INT64 data type");
	      break;

	    case PTP_DTC_UINT64:
	      printf(" UINT64 data type");
	      break;

	    case PTP_DTC_INT128:
	      printf(" INT128 data type");
	      break;

	    case PTP_DTC_UINT128:
	      printf(" UINT128 data type");
	      break;

	    default:
	      printf(" UNKNOWN data type");
	      break;
	    }
	  }
	  if (opd.GetSet) {
	    printf(" GET/SET");
	  } else {
	    printf(" READ ONLY");
	  }
	  printf(" GROUP 0x%x", opd.GroupCode);

	  printf("\n");
	  ptp_free_objectpropdesc(&opd);
	}
	free(props);
      }
    }
  }

  if(storage != NULL &&
     ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
    printf("Storage Devices:\n");
    while(storage != NULL) {
      printf("   StorageID: 0x%08x\n",storage->id);
      printf("      StorageType: 0x%04x ",storage->StorageType);
      switch (storage->StorageType) {
      case PTP_ST_Undefined:
	printf("(undefined)\n");
	break;
      case PTP_ST_FixedROM:
	printf("fixed ROM storage\n");
	break;
      case PTP_ST_RemovableROM:
	printf("removable ROM storage\n");
	break;
      case PTP_ST_FixedRAM:
	printf("fixed RAM storage\n");
	break;
      case PTP_ST_RemovableRAM:
	printf("removable RAM storage\n");
	break;
      default:
	printf("UNKNOWN storage\n");
	break;
      }
      printf("      FilesystemType: 0x%04x ",storage->FilesystemType);
      switch(storage->FilesystemType) {
      case PTP_FST_Undefined:
	printf("(undefined)\n");
	break;
      case PTP_FST_GenericFlat:
	printf("generic flat filesystem\n");
	break;
      case PTP_FST_GenericHierarchical:
	printf("generic hierarchical\n");
	break;
      case PTP_FST_DCF:
	printf("DCF\n");
	break;
      default:
	printf("UNKNONWN filesystem type\n");
	break;
      }
      printf("      AccessCapability: 0x%04x ",storage->AccessCapability);
      switch(storage->AccessCapability) {
      case PTP_AC_ReadWrite:
	printf("read/write\n");
	break;
      case PTP_AC_ReadOnly:
	printf("read only\n");
	break;
      case PTP_AC_ReadOnly_with_Object_Deletion:
	printf("read only + object deletion\n");
	break;
      default:
	printf("UNKNOWN access capability\n");
	break;
      }
      printf("      MaxCapacity: %llu\n",
	     (long long unsigned int) storage->MaxCapacity);
      printf("      FreeSpaceInBytes: %llu\n",
	     (long long unsigned int) storage->FreeSpaceInBytes);
      printf("      FreeSpaceInObjects: %llu\n",
	     (long long unsigned int) storage->FreeSpaceInObjects);
      printf("      StorageDescription: %s\n",storage->StorageDescription);
      printf("      VolumeIdentifier: %s\n",storage->VolumeIdentifier);
      storage = storage->next;
    }
  }

  printf("Special directories:\n");
  printf("   Default music folder: 0x%08x\n",
	 device->default_music_folder);
  printf("   Default playlist folder: 0x%08x\n",
	 device->default_playlist_folder);
  printf("   Default picture folder: 0x%08x\n",
	 device->default_picture_folder);
  printf("   Default video folder: 0x%08x\n",
	 device->default_video_folder);
  printf("   Default organizer folder: 0x%08x\n",
	 device->default_organizer_folder);
  printf("   Default zencast folder: 0x%08x\n",
	 device->default_zencast_folder);
  printf("   Default album folder: 0x%08x\n",
	 device->default_album_folder);
  printf("   Default text folder: 0x%08x\n",
	 device->default_text_folder);
}

/**
 * This resets a device in case it supports the <code>PTP_OC_ResetDevice</code>
 * operation code (0x1010).
 * @param device a pointer to the device to reset.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Reset_Device(LIBMTP_mtpdevice_t *device)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_operation_issupported(params,PTP_OC_ResetDevice)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Reset_Device(): "
			    "device does not support resetting.");
    return -1;
  }
  ret = ptp_resetdevice(params);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "Error resetting.");
    return -1;
  }
  return 0;
}

/**
 * This retrieves the manufacturer name of an MTP device.
 * @param device a pointer to the device to get the manufacturer name for.
 * @return a newly allocated UTF-8 string representing the manufacturer name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Manufacturername(LIBMTP_mtpdevice_t *device)
{
  char *retmanuf = NULL;
  PTPParams *params = (PTPParams *) device->params;

  if (params->deviceinfo.Manufacturer != NULL) {
    retmanuf = strdup(params->deviceinfo.Manufacturer);
  }
  return retmanuf;
}

/**
 * This retrieves the model name (often equal to product name)
 * of an MTP device.
 * @param device a pointer to the device to get the model name for.
 * @return a newly allocated UTF-8 string representing the model name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Modelname(LIBMTP_mtpdevice_t *device)
{
  char *retmodel = NULL;
  PTPParams *params = (PTPParams *) device->params;

  if (params->deviceinfo.Model != NULL) {
    retmodel = strdup(params->deviceinfo.Model);
  }
  return retmodel;
}

/**
 * This retrieves the serial number of an MTP device.
 * @param device a pointer to the device to get the serial number for.
 * @return a newly allocated UTF-8 string representing the serial number.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Serialnumber(LIBMTP_mtpdevice_t *device)
{
  char *retnumber = NULL;
  PTPParams *params = (PTPParams *) device->params;

  if (params->deviceinfo.SerialNumber != NULL) {
    retnumber = strdup(params->deviceinfo.SerialNumber);
  }
  return retnumber;
}

/**
 * This retrieves the device version (hardware and firmware version) of an
 * MTP device.
 * @param device a pointer to the device to get the device version for.
 * @return a newly allocated UTF-8 string representing the device version.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Deviceversion(LIBMTP_mtpdevice_t *device)
{
  char *retversion = NULL;
  PTPParams *params = (PTPParams *) device->params;

  if (params->deviceinfo.DeviceVersion != NULL) {
    retversion = strdup(params->deviceinfo.DeviceVersion);
  }
  return retversion;
}


/**
 * This retrieves the "friendly name" of an MTP device. Usually
 * this is simply the name of the owner or something like
 * "John Doe's Digital Audio Player". This property should be supported
 * by all MTP devices.
 * @param device a pointer to the device to get the friendly name for.
 * @return a newly allocated UTF-8 string representing the friendly name.
 *         The string must be freed by the caller after use.
 * @see LIBMTP_Set_Friendlyname()
 */
char *LIBMTP_Get_Friendlyname(LIBMTP_mtpdevice_t *device)
{
  PTPPropertyValue propval;
  char *retstring = NULL;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName)) {
    return NULL;
  }

  ret = ptp_getdevicepropvalue(params,
			       PTP_DPC_MTP_DeviceFriendlyName,
			       &propval,
			       PTP_DTC_STR);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "Error getting friendlyname.");
    return NULL;
  }
  if (propval.str != NULL) {
    retstring = strdup(propval.str);
    free(propval.str);
  }
  return retstring;
}

/**
 * Sets the "friendly name" of an MTP device.
 * @param device a pointer to the device to set the friendly name for.
 * @param friendlyname the new friendly name for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Friendlyname()
 */
int LIBMTP_Set_Friendlyname(LIBMTP_mtpdevice_t *device,
			 char const * const friendlyname)
{
  PTPPropertyValue propval;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName)) {
    return -1;
  }
  propval.str = (char *) friendlyname;
  ret = ptp_setdevicepropvalue(params,
			       PTP_DPC_MTP_DeviceFriendlyName,
			       &propval,
			       PTP_DTC_STR);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "Error setting friendlyname.");
    return -1;
  }
  return 0;
}

/**
 * This retrieves the synchronization partner of an MTP device. This
 * property should be supported by all MTP devices.
 * @param device a pointer to the device to get the sync partner for.
 * @return a newly allocated UTF-8 string representing the synchronization
 *         partner. The string must be freed by the caller after use.
 * @see LIBMTP_Set_Syncpartner()
 */
char *LIBMTP_Get_Syncpartner(LIBMTP_mtpdevice_t *device)
{
  PTPPropertyValue propval;
  char *retstring = NULL;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner)) {
    return NULL;
  }

  ret = ptp_getdevicepropvalue(params,
			       PTP_DPC_MTP_SynchronizationPartner,
			       &propval,
			       PTP_DTC_STR);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "Error getting syncpartner.");
    return NULL;
  }
  if (propval.str != NULL) {
    retstring = strdup(propval.str);
    free(propval.str);
  }
  return retstring;
}


/**
 * Sets the synchronization partner of an MTP device. Note that
 * we have no idea what the effect of setting this to "foobar"
 * may be. But the general idea seems to be to tell which program
 * shall synchronize with this device and tell others to leave
 * it alone.
 * @param device a pointer to the device to set the sync partner for.
 * @param syncpartner the new synchronization partner for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Syncpartner()
 */
int LIBMTP_Set_Syncpartner(LIBMTP_mtpdevice_t *device,
			 char const * const syncpartner)
{
  PTPPropertyValue propval;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner)) {
    return -1;
  }
  propval.str = (char *) syncpartner;
  ret = ptp_setdevicepropvalue(params,
			       PTP_DPC_MTP_SynchronizationPartner,
			       &propval,
			       PTP_DTC_STR);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "Error setting syncpartner.");
    return -1;
  }
  return 0;
}

/**
 * Checks if the device can stora a file of this size or
 * if it's too big.
 * @param device a pointer to the device.
 * @param filesize the size of the file to check whether it will fit.
 * @param storage a pointer to the storage to try to fit the file on.
 * @return 0 if the file fits, any other value means failure.
 */
static int check_if_file_fits(LIBMTP_mtpdevice_t *device,
			      LIBMTP_devicestorage_t *storage,
			      uint64_t const filesize) {
  PTPParams *params = (PTPParams *) device->params;
  uint64_t freebytes;
  int ret;

  // If we cannot check the storage, no big deal.
  if (!ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
    return 0;
  }

  ret = get_storage_freespace(device, storage, &freebytes);
  if (ret != 0) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "check_if_file_fits(): error checking free storage.");
    return -1;
  } else {
    // See if it fits.
    if (filesize > freebytes) {
      return -1;
    }
  }
  return 0;
}


/**
 * This function retrieves the current battery level on the device.
 * @param device a pointer to the device to get the battery level for.
 * @param maximum_level a pointer to a variable that will hold the
 *        maximum level of the battery if the call was successful.
 * @param current_level a pointer to a variable that will hold the
 *        current level of the battery if the call was successful.
 *        A value of 0 means that the device is on external power.
 * @return 0 if the storage info was successfully retrieved, any other
 *        means failure. A typical cause of failure is that
 *        the device does not support the battery level property.
 */
int LIBMTP_Get_Batterylevel(LIBMTP_mtpdevice_t *device,
			    uint8_t * const maximum_level,
			    uint8_t * const current_level)
{
  PTPPropertyValue propval;
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  *maximum_level = 0;
  *current_level = 0;

  if (FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) ||
      !ptp_property_issupported(params, PTP_DPC_BatteryLevel)) {
    return -1;
  }

  ret = ptp_getdevicepropvalue(params, PTP_DPC_BatteryLevel,
			       &propval, PTP_DTC_UINT8);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret,
				"LIBMTP_Get_Batterylevel(): "
				"could not get device property value.");
    return -1;
  }

  *maximum_level = device->maximum_battery_level;
  *current_level = propval.u8;

  return 0;
}


/**
 * Formats device storage (if the device supports the operation).
 * WARNING: This WILL delete all data from the device. Make sure you've
 * got confirmation from the user BEFORE you call this function.
 *
 * @param device a pointer to the device containing the storage to format.
 * @param storage the actual storage to format.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Format_Storage(LIBMTP_mtpdevice_t *device,
			  LIBMTP_devicestorage_t *storage)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;

  if (!ptp_operation_issupported(params,PTP_OC_FormatStore)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Format_Storage(): "
			    "device does not support formatting storage.");
    return -1;
  }
  ret = ptp_formatstore(params, storage->id);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Format_Storage(): "
				"failed to format storage.");
    return -1;
  }
  return 0;
}

/**
 * Helper function to extract a unicode property off a device.
 * This is the standard way of retrieveing unicode device
 * properties as described by the PTP spec.
 * @param device a pointer to the device to get the property from.
 * @param unicstring a pointer to a pointer that will hold the
 *        property after this call is completed.
 * @param property the property to retrieve.
 * @return 0 on success, any other value means failure.
 */
static int get_device_unicode_property(LIBMTP_mtpdevice_t *device,
				       char **unicstring, uint16_t property)
{
  PTPPropertyValue propval;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t *tmp;
  uint16_t ret;
  unsigned int i;

  if (!ptp_property_issupported(params, property)) {
    return -1;
  }

  // Unicode strings are 16bit unsigned integer arrays.
  ret = ptp_getdevicepropvalue(params,
			       property,
			       &propval,
			       PTP_DTC_AUINT16);
  if (ret != PTP_RC_OK) {
    // TODO: add a note on WHICH property that we failed to get.
    *unicstring = NULL;
    add_ptp_error_to_errorstack(device, ret,
				"get_device_unicode_property(): "
				"failed to get unicode property.");
    return -1;
  }

  // Extract the actual array.
  // printf("Array of %d elements\n", propval.a.count);
  tmp = malloc((propval.a.count + 1)*sizeof(uint16_t));
  for (i = 0; i < propval.a.count; i++) {
    tmp[i] = propval.a.v[i].u16;
    // printf("%04x ", tmp[i]);
  }
  tmp[propval.a.count] = 0x0000U;
  free(propval.a.v);

  *unicstring = utf16_to_utf8(device, tmp);

  free(tmp);

  return 0;
}

/**
 * This function returns the secure time as an XML document string from
 * the device.
 * @param device a pointer to the device to get the secure time for.
 * @param sectime the secure time string as an XML document or NULL if the call
 *         failed or the secure time property is not supported. This string
 *         must be <code>free()</code>:ed by the caller after use.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Secure_Time(LIBMTP_mtpdevice_t *device, char ** const sectime)
{
  return get_device_unicode_property(device, sectime, PTP_DPC_MTP_SecureTime);
}

/**
 * This function returns the device (public key) certificate as an
 * XML document string from the device.
 * @param device a pointer to the device to get the device certificate for.
 * @param devcert the device certificate as an XML string or NULL if the call
 *        failed or the device certificate property is not supported. This
 *        string must be <code>free()</code>:ed by the caller after use.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Device_Certificate(LIBMTP_mtpdevice_t *device, char ** const devcert)
{
  return get_device_unicode_property(device, devcert,
				     PTP_DPC_MTP_DeviceCertificate);
}

/**
 * This function retrieves a list of supported file types, i.e. the file
 * types that this device claims it supports, e.g. audio file types that
 * the device can play etc. This list is mitigated to
 * include the file types that libmtp can handle, i.e. it will not list
 * filetypes that libmtp will handle internally like playlists and folders.
 * @param device a pointer to the device to get the filetype capabilities for.
 * @param filetypes a pointer to a pointer that will hold the list of
 *        supported filetypes if the call was successful. This list must
 *        be <code>free()</code>:ed by the caller after use.
 * @param length a pointer to a variable that will hold the length of the
 *        list of supported filetypes if the call was successful.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Filetype_Description()
 */
int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device, uint16_t ** const filetypes,
				  uint16_t * const length)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint16_t *localtypes;
  uint16_t localtypelen;
  uint32_t i;

  // This is more memory than needed if there are unknown types, but what the heck.
  localtypes = (uint16_t *) malloc(params->deviceinfo.ImageFormats_len * sizeof(uint16_t));
  localtypelen = 0;

  for (i=0;i<params->deviceinfo.ImageFormats_len;i++) {
    uint16_t localtype = map_ptp_type_to_libmtp_type(params->deviceinfo.ImageFormats[i]);
    if (localtype != LIBMTP_FILETYPE_UNKNOWN) {
      localtypes[localtypelen] = localtype;
      localtypelen++;
    }
  }
  // The forgotten Ogg support on YP-10 and others...
  if (FLAG_OGG_IS_UNKNOWN(ptp_usb)) {
    uint16_t *tmp = (uint16_t *) realloc(
        localtypes, (params->deviceinfo.ImageFormats_len+1) * sizeof(uint16_t));
    if (tmp == NULL)
      return -ENOMEM;
    localtypes = tmp;
    localtypes[localtypelen] = LIBMTP_FILETYPE_OGG;
    localtypelen++;
  }
  // The forgotten FLAC support on Cowon iAudio S9 and others...
  if (FLAG_FLAC_IS_UNKNOWN(ptp_usb)) {
    uint16_t *tmp = (uint16_t *) realloc(
        localtypes, (params->deviceinfo.ImageFormats_len+1) * sizeof(uint16_t));
    if (tmp == NULL)
      return -ENOMEM;
    localtypes = tmp;
    localtypes[localtypelen] = LIBMTP_FILETYPE_FLAC;
    localtypelen++;
  }

  *filetypes = localtypes;
  *length = localtypelen;

  return 0;
}

/**
 * This function checks if the device has some specific capabilities, in
 * order to avoid calling APIs that may disturb the device.
 *
 * @param device a pointer to the device to check the capability on.
 * @param cap the capability to check.
 * @return 0 if not supported, any other value means the device has the
 * requested capability.
 */
int LIBMTP_Check_Capability(LIBMTP_mtpdevice_t *device, LIBMTP_devicecap_t cap)
{
  switch (cap) {
  case LIBMTP_DEVICECAP_GetPartialObject:
    return (ptp_operation_issupported(device->params,
				      PTP_OC_GetPartialObject) ||
	    ptp_operation_issupported(device->params,
				      PTP_OC_ANDROID_GetPartialObject64));
  case LIBMTP_DEVICECAP_SendPartialObject:
    return ptp_operation_issupported(device->params,
				     PTP_OC_ANDROID_SendPartialObject);
  case LIBMTP_DEVICECAP_EditObjects:
    return (ptp_operation_issupported(device->params,
				      PTP_OC_ANDROID_TruncateObject) &&
	    ptp_operation_issupported(device->params,
				      PTP_OC_ANDROID_BeginEditObject) &&
	    ptp_operation_issupported(device->params,
				      PTP_OC_ANDROID_EndEditObject));
  case LIBMTP_DEVICECAP_MoveObject:
    return ptp_operation_issupported(device->params,
				     PTP_OC_MoveObject);
  case LIBMTP_DEVICECAP_CopyObject:
    return ptp_operation_issupported(device->params,
				     PTP_OC_CopyObject);
  /*
   * Handle other capabilities here, this is also a good place to
   * blacklist some advanced operations on specific devices if need
   * be.
   */

  default:
    break;
  }
  return 0;
}

/**
 * This function updates all the storage id's of a device and their
 * properties, then creates a linked list and puts the list head into
 * the device struct. It also optionally sorts this list. If you want
 * to display storage information in your application you should call
 * this function, then dereference the device struct
 * (<code>device-&gt;storage</code>) to get out information on the storage.
 *
 * You need to call this every time you want to update the
 * <code>device-&gt;storage</code> list, for example anytime you need
 * to check available storage somewhere.
 *
 * <b>WARNING:</b> since this list is dynamically updated, do not
 * reference its fields in external applications by pointer! E.g
 * do not put a reference to any <code>char *</code> field. instead
 * <code>strncpy()</code> it!
 *
 * @param device a pointer to the device to get the storage for.
 * @param sortby an integer that determines the sorting of the storage list.
 *        Valid sort methods are defined in libmtp.h with beginning with
 *        LIBMTP_STORAGE_SORTBY_. 0 or LIBMTP_STORAGE_SORTBY_NOTSORTED to not
 *        sort.
 * @return 0 on success, 1 success but only with storage id's, storage
 *        properities could not be retrieved and -1 means failure.
 */
int LIBMTP_Get_Storage(LIBMTP_mtpdevice_t *device, int const sortby)
{
  uint32_t i = 0;
  PTPStorageInfo storageInfo;
  PTPParams *params = (PTPParams *) device->params;
  PTPStorageIDs storageIDs;
  LIBMTP_devicestorage_t *storage = NULL;
  LIBMTP_devicestorage_t *storageprev = NULL;

  if (device->storage != NULL)
    free_storage_list(device);

  // if (!ptp_operation_issupported(params,PTP_OC_GetStorageIDs))
  //   return -1;
  if (ptp_getstorageids (params, &storageIDs) != PTP_RC_OK)
    return -1;
  if (storageIDs.n < 1)
    return -1;

  if (!ptp_operation_issupported(params,PTP_OC_GetStorageInfo)) {
    for (i = 0; i < storageIDs.n; i++) {

      storage = (LIBMTP_devicestorage_t *)
	malloc(sizeof(LIBMTP_devicestorage_t));
      storage->prev = storageprev;
      if (storageprev != NULL)
        storageprev->next = storage;
      if (device->storage == NULL)
        device->storage = storage;

      storage->id = storageIDs.Storage[i];
      storage->StorageType = PTP_ST_Undefined;
      storage->FilesystemType = PTP_FST_Undefined;
      storage->AccessCapability = PTP_AC_ReadWrite;
      storage->MaxCapacity = (uint64_t) -1;
      storage->FreeSpaceInBytes = (uint64_t) -1;
      storage->FreeSpaceInObjects = (uint64_t) -1;
      storage->StorageDescription = strdup("Unknown storage");
      storage->VolumeIdentifier = strdup("Unknown volume");
      storage->next = NULL;

      storageprev = storage;
    }
    free(storageIDs.Storage);
    return 1;
  } else {
    for (i = 0; i < storageIDs.n; i++) {
      uint16_t ret;
      ret = ptp_getstorageinfo(params, storageIDs.Storage[i], &storageInfo);
      if (ret != PTP_RC_OK) {
	add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Storage(): "
				    "Could not get storage info.");
	if (device->storage != NULL) {
          free_storage_list(device);
	}
	return -1;
      }

      storage = (LIBMTP_devicestorage_t *)
	malloc(sizeof(LIBMTP_devicestorage_t));
      storage->prev = storageprev;
      if (storageprev != NULL)
        storageprev->next = storage;
      if (device->storage == NULL)
        device->storage = storage;

      storage->id = storageIDs.Storage[i];
      storage->StorageType = storageInfo.StorageType;
      storage->FilesystemType = storageInfo.FilesystemType;
      storage->AccessCapability = storageInfo.AccessCapability;
      storage->MaxCapacity = storageInfo.MaxCapability;
      storage->FreeSpaceInBytes = storageInfo.FreeSpaceInBytes;
      storage->FreeSpaceInObjects = storageInfo.FreeSpaceInImages;
      storage->StorageDescription = storageInfo.StorageDescription;
      storage->VolumeIdentifier = storageInfo.VolumeLabel;
      storage->next = NULL;

      storageprev = storage;
    }

    if (storage != NULL)
      storage->next = NULL;

    sort_storage_by(device,sortby);
    free(storageIDs.Storage);
    return 0;
  }
}

/**
 * This creates a new file metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_file_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * <pre>
 * LIBMTP_file_t *file = LIBMTP_new_file_t();
 * file->filename = strdup(namestr);
 * ....
 * LIBMTP_destroy_file_t(file);
 * </pre>
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_file_t()
 */
LIBMTP_file_t *LIBMTP_new_file_t(void)
{
  LIBMTP_file_t *new = (LIBMTP_file_t *) malloc(sizeof(LIBMTP_file_t));
  if (new == NULL) {
    return NULL;
  }
  new->filename = NULL;
  new->item_id = 0;
  new->parent_id = 0;
  new->storage_id = 0;
  new->filesize = 0;
  new->modificationdate = 0;
  new->filetype = LIBMTP_FILETYPE_UNKNOWN;
  new->next = NULL;
  return new;
}

/**
 * This destroys a file metadata structure and deallocates the memory
 * used by it, including any strings. Never use a file metadata
 * structure again after calling this function on it.
 * @param file the file metadata to destroy.
 * @see LIBMTP_new_file_t()
 */
void LIBMTP_destroy_file_t(LIBMTP_file_t *file)
{
  if (file == NULL) {
    return;
  }
  if (file->filename != NULL)
    free(file->filename);
  free(file);
  return;
}

/**
 * Helper function that takes one PTP object and creates a
 * LIBMTP_file_t metadata entry.
 */
static LIBMTP_file_t *obj2file(LIBMTP_mtpdevice_t *device, PTPObject *ob)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  LIBMTP_file_t *file;
  unsigned int i;

  // Allocate a new file type
  file = LIBMTP_new_file_t();

  file->parent_id = ob->oi.ParentObject;
  file->storage_id = ob->oi.StorageID;

  if (ob->oi.Filename != NULL) {
    file->filename = strdup(ob->oi.Filename);
  }

  // Set the filetype
  file->filetype = map_ptp_type_to_libmtp_type(ob->oi.ObjectFormat);

  /*
   * A special quirk for devices that doesn't quite
   * remember that some files marked as "unknown" type are
   * actually OGG or FLAC files. We look at the filename extension
   * and see if it happens that this was atleast named "ogg" or "flac"
   * and fall back on this heuristic approach in that case,
   * for these bugged devices only.
   */
  if (file->filetype == LIBMTP_FILETYPE_UNKNOWN) {
    if ((FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) ||
        FLAG_OGG_IS_UNKNOWN(ptp_usb)) &&
        has_ogg_extension(file->filename)) {
      file->filetype = LIBMTP_FILETYPE_OGG;
    }

    if (FLAG_FLAC_IS_UNKNOWN(ptp_usb) && has_flac_extension(file->filename)) {
        file->filetype = LIBMTP_FILETYPE_FLAC;
    }
  }

  // Set the modification date
  file->modificationdate = ob->oi.ModificationDate;

  // We only have 32-bit file size here; later we use the PTP_OPC_ObjectSize property
  file->filesize = ob->oi.ObjectCompressedSize;

  // This is a unique ID so we can keep track of the file.
  file->item_id = ob->oid;

  /*
   * If we have a cached, large set of metadata, then use it!
   */
  if (ob->mtpprops) {
    MTPProperties *prop = ob->mtpprops;

    for (i=0; i < ob->nrofmtpprops; i++, prop++) {
      // Pick ObjectSize here...
      if (prop->property == PTP_OPC_ObjectSize) {
	// This may already be set, but this 64bit precision value
	// is better than the PTP 32bit value, so let it override.
	if (device->object_bitsize == 64) {
	  file->filesize = prop->propval.u64;
	} else {
	  file->filesize = prop->propval.u32;
	}
	break;
      }
    }
  } else if (ptp_operation_issupported(params,PTP_OC_MTP_GetObjectPropsSupported)) {
    uint16_t *props = NULL;
    uint32_t propcnt = 0;
    int ret;

    // First see which properties can be retrieved for this object format
    ret = ptp_mtp_getobjectpropssupported(params, map_libmtp_type_to_ptp_type(file->filetype), &propcnt, &props);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "obj2file: call to ptp_mtp_getobjectpropssupported() failed.");
      // Silently fall through.
    } else {
      for (i = 0; i < propcnt; i++) {
	switch (props[i]) {
	case PTP_OPC_ObjectSize:
	  if (device->object_bitsize == 64) {
	    file->filesize = get_u64_from_object(device, file->item_id, PTP_OPC_ObjectSize, 0);
	  } else {
	    file->filesize = get_u32_from_object(device, file->item_id, PTP_OPC_ObjectSize, 0);
	  }
	  break;
	default:
	  break;
	}
      }
      free(props);
    }
  }

  return file;
}


/**
 * This function retrieves the metadata for a single file off
 * the device.
 *
 * Do not call this function repeatedly! The file handles are linearly
 * searched O(n) and the call may involve (slow) USB traffic, so use
 * <code>LIBMTP_Get_Filelisting()</code> and cache the file, preferably
 * as an efficient data structure such as a hash list.
 *
 * Incidentally this function will return metadata for
 * a folder (association) as well, but this is not a proper use
 * of it, it is intended for file manipulation, not folder manipulation.
 *
 * @param device a pointer to the device to get the file metadata from.
 * @param fileid the object ID of the file that you want the metadata for.
 * @return a metadata entry on success or NULL on failure.
 * @see LIBMTP_Get_Filelisting()
 */
LIBMTP_file_t *LIBMTP_Get_Filemetadata(LIBMTP_mtpdevice_t *device, uint32_t const fileid)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;
  PTPObject *ob;

  // Get all the handles if we haven't already done that
  // (Only on cached devices.)
  if (device->cached && params->nrofobjects == 0) {
    flush_handles(device);
  }

  ret = ptp_object_want(params, fileid, PTPOBJECT_OBJECTINFO_LOADED|PTPOBJECT_MTPPROPLIST_LOADED, &ob);
  if (ret != PTP_RC_OK)
    return NULL;

  return obj2file(device, ob);
}

/**
* THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 * @see LIBMTP_Get_Filelisting_With_Callback()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device)
{
  LIBMTP_INFO("WARNING: LIBMTP_Get_Filelisting() is deprecated.\n");
  LIBMTP_INFO("WARNING: please update your code to use LIBMTP_Get_Filelisting_With_Callback()\n");
  return LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);
}

/**
 * This returns a long list of all files available
 * on the current MTP device. Folders will not be returned, but abstract
 * entities like playlists and albums will show up as "files". Typical usage:
 *
 * <pre>
 * LIBMTP_file_t *filelist;
 *
 * filelist = LIBMTP_Get_Filelisting_With_Callback(device, callback, data);
 * while (filelist != NULL) {
 *   LIBMTP_file_t *tmp;
 *
 *   // Do something on each element in the list here...
 *   tmp = filelist;
 *   filelist = filelist->next;
 *   LIBMTP_destroy_file_t(tmp);
 * }
 * </pre>
 *
 * If you want to group your file listing by storage (per storage unit) or
 * arrange files into folders, you must dereference the <code>storage_id</code>
 * and/or <code>parent_id</code> field of the returned <code>LIBMTP_file_t</code>
 * struct. To arrange by folders or files you typically have to create the proper
 * trees by calls to <code>LIBMTP_Get_Storage()</code> and/or
 * <code>LIBMTP_Get_Folder_List()</code> first.
 *
 * @param device a pointer to the device to get the file listing for.
 * @param callback a function to be called during the tracklisting retrieveal
 *        for displaying progress bars etc, or NULL if you don't want
 *        any callbacks.
 * @param data a user-defined pointer that is passed along to
 *        the <code>progress</code> function in order to
 *        pass along some user defined data to the progress
 *        updates. If not used, set this to NULL.
 * @return a list of files that can be followed using the <code>next</code>
 *        field of the <code>LIBMTP_file_t</code> data structure.
 *        Each of the metadata tags must be freed after use, and may
 *        contain only partial metadata information, i.e. one or several
 *        fields may be NULL or 0.
 * @see LIBMTP_Get_Filemetadata()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting_With_Callback(LIBMTP_mtpdevice_t *device,
                                                    LIBMTP_progressfunc_t const callback,
                                                    void const * const data)
{
  uint32_t i = 0;
  LIBMTP_file_t *retfiles = NULL;
  LIBMTP_file_t *curfile = NULL;
  PTPParams *params = (PTPParams *) device->params;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0) {
    flush_handles(device);
  }

  for (i = 0; i < params->nrofobjects; i++) {
    LIBMTP_file_t *file;
    PTPObject *ob;

    if (callback != NULL)
      callback(i, params->nrofobjects, data);

    ob = &params->objects[i];

    if (ob->oi.ObjectFormat == PTP_OFC_Association) {
      // MTP use this object format for folders which means
      // these "files" will turn up on a folder listing instead.
      continue;
    }

    // Look up metadata
    file = obj2file(device, ob);
    if (file == NULL) {
      continue;
    }

    // Add track to a list that will be returned afterwards.
    if (retfiles == NULL) {
      retfiles = file;
      curfile = file;
    } else {
      curfile->next = file;
      curfile = file;
    }

    // Call listing callback
    // double progressPercent = (double)i*(double)100.0 / (double)params->handles.n;

  } // Handle counting loop
  return retfiles;
}

/**
 * This function retrieves the contents of a certain folder
 * with id parent on a certain storage on a certain device.
 * The result contains both files and folders.
 * The device used with this operations must have been opened with
 * LIBMTP_Open_Raw_Device_Uncached() or it will fail.
 *
 * NOTE: the request will always perform I/O with the device.
 * @param device a pointer to the MTP device to report info from.
 * @param storage a storage on the device to report info from. If
 *        0 is passed in, the files for the given parent will be
 *        searched across all available storages.
 * @param parent the parent folder id.
 */
LIBMTP_file_t * LIBMTP_Get_Files_And_Folders(LIBMTP_mtpdevice_t *device,
			     uint32_t const storage,
			     uint32_t const parent)
{
  PTPParams *params = (PTPParams *) device->params;
  LIBMTP_file_t *retfiles = NULL;
  LIBMTP_file_t *curfile = NULL;
  PTPObjectHandles currentHandles;
  uint32_t storageid;
  uint16_t ret;
  unsigned int i = 0;

  if (device->cached) {
    // This function is only supposed to be used by devices
    // opened as uncached!
    LIBMTP_ERROR("tried to use %s on a cached device!\n",
		 __func__);
    return NULL;
  }

  if (storage == 0)
    storageid = PTP_GOH_ALL_STORAGE;
  else
    storageid = storage;

  ret = ptp_getobjecthandles(params,
			     storageid,
			     PTP_GOH_ALL_FORMATS,
			     parent,
			     &currentHandles);

  if (ret != PTP_RC_OK) {
    char buf[80];
    sprintf(buf,"LIBMTP_Get_Files_And_Folders(): could not get object handles of %08x.", parent);
    add_ptp_error_to_errorstack(device, ret, buf);
    return NULL;
  }

  if (currentHandles.Handler == NULL || currentHandles.n == 0)
    return NULL;

  for (i = 0; i < currentHandles.n; i++) {
    LIBMTP_file_t *file;

    // Get metadata for one file, if it fails, try next file
    file = LIBMTP_Get_Filemetadata(device, currentHandles.Handler[i]);
    if (file == NULL)
      continue;

    // Add track to a list that will be returned afterwards.
    if (curfile == NULL) {
      curfile = file;
      retfiles = file;
    } else {
      curfile->next = file;
      curfile = file;
    }
  }

  free(currentHandles.Handler);

  // Return a pointer to the original first file
  // in the big list.
  return retfiles;
}

/**
 * This function retrieves the list of ids of files and folders in a certain
 * folder with id parent on a certain storage on a certain device.
 * The device used with this operations must have been opened with
 * LIBMTP_Open_Raw_Device_Uncached() or it will fail.
 *
 * NOTE: the request will always perform I/O with the device.
 * @param device a pointer to the MTP device to report info from.
 * @param storage a storage on the device to report info from. If
 *        0 is passed in, the files for the given parent will be
 *        searched across all available storages.
 * @param parent the parent folder id.
 * @param out the pointer where the array of ids is returned. It is
 *        set only when the returned value > 0. The caller takes the
 *        ownership of the array and has to free() it.
 * @return the length of the returned array or -1 in case of failure.
 */

int LIBMTP_Get_Children(LIBMTP_mtpdevice_t *device,
                        uint32_t const storage,
                        uint32_t const parent,
                        uint32_t **out)
{
  PTPParams *params = (PTPParams *) device->params;
  PTPObjectHandles currentHandles;
  uint32_t storageid;
  uint16_t ret;

  if (device->cached) {
    // This function is only supposed to be used by devices
    // opened as uncached!
    LIBMTP_ERROR("tried to use %s on a cached device!\n", __func__);
    return -1;
  }

  if (storage == 0)
    storageid = PTP_GOH_ALL_STORAGE;
  else
    storageid = storage;

  ret = ptp_getobjecthandles(params,
                             storageid,
                             PTP_GOH_ALL_FORMATS,
                             parent,
                             &currentHandles);

  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret,
        "LIBMTP_Get_Children(): could not get object handles.");
    return -1;
  }

  if (currentHandles.Handler == NULL || currentHandles.n == 0)
    return 0;
  *out = currentHandles.Handler;
  return currentHandles.n;
}

/**
 * This creates a new track metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_track_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * <pre>
 * LIBMTP_track_t *track = LIBMTP_new_track_t();
 * track->title = strdup(titlestr);
 * ....
 * LIBMTP_destroy_track_t(track);
 * </pre>
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_track_t()
 */
LIBMTP_track_t *LIBMTP_new_track_t(void)
{
  LIBMTP_track_t *new = (LIBMTP_track_t *) malloc(sizeof(LIBMTP_track_t));
  if (new == NULL) {
    return NULL;
  }
  new->item_id = 0;
  new->parent_id = 0;
  new->storage_id = 0;
  new->title = NULL;
  new->artist = NULL;
  new->composer = NULL;
  new->album = NULL;
  new->genre = NULL;
  new->date = NULL;
  new->filename = NULL;
  new->duration = 0;
  new->tracknumber = 0;
  new->filesize = 0;
  new->filetype = LIBMTP_FILETYPE_UNKNOWN;
  new->samplerate = 0;
  new->nochannels = 0;
  new->wavecodec = 0;
  new->bitrate = 0;
  new->bitratetype = 0;
  new->rating = 0;
  new->usecount = 0;
  new->modificationdate = 0;
  new->next = NULL;
  return new;
}

/**
 * This destroys a track metadata structure and deallocates the memory
 * used by it, including any strings. Never use a track metadata
 * structure again after calling this function on it.
 * @param track the track metadata to destroy.
 * @see LIBMTP_new_track_t()
 */
void LIBMTP_destroy_track_t(LIBMTP_track_t *track)
{
  if (track == NULL) {
    return;
  }
  if (track->title != NULL)
    free(track->title);
  if (track->artist != NULL)
    free(track->artist);
  if (track->composer != NULL)
    free(track->composer);
  if (track->album != NULL)
    free(track->album);
  if (track->genre != NULL)
    free(track->genre);
  if (track->date != NULL)
    free(track->date);
  if (track->filename != NULL)
    free(track->filename);
  free(track);
  return;
}

/**
 * This function maps and copies a property onto the track metadata if applicable.
 */
static void pick_property_to_track_metadata(LIBMTP_mtpdevice_t *device, MTPProperties *prop, LIBMTP_track_t *track)
{
  switch (prop->property) {
  case PTP_OPC_Name:
    if (prop->propval.str != NULL)
      track->title = strdup(prop->propval.str);
    else
      track->title = NULL;
    break;
  case PTP_OPC_Artist:
    if (prop->propval.str != NULL)
      track->artist = strdup(prop->propval.str);
    else
      track->artist = NULL;
    break;
  case PTP_OPC_Composer:
    if (prop->propval.str != NULL)
      track->composer = strdup(prop->propval.str);
    else
      track->composer = NULL;
    break;
  case PTP_OPC_Duration:
    track->duration = prop->propval.u32;
    break;
  case PTP_OPC_Track:
    track->tracknumber = prop->propval.u16;
    break;
  case PTP_OPC_Genre:
    if (prop->propval.str != NULL)
      track->genre = strdup(prop->propval.str);
    else
      track->genre = NULL;
    break;
  case PTP_OPC_AlbumName:
    if (prop->propval.str != NULL)
      track->album = strdup(prop->propval.str);
    else
      track->album = NULL;
    break;
  case PTP_OPC_OriginalReleaseDate:
    if (prop->propval.str != NULL)
      track->date = strdup(prop->propval.str);
    else
      track->date = NULL;
    break;
    // These are, well not so important.
  case PTP_OPC_SampleRate:
    track->samplerate = prop->propval.u32;
    break;
  case PTP_OPC_NumberOfChannels:
    track->nochannels = prop->propval.u16;
    break;
  case PTP_OPC_AudioWAVECodec:
    track->wavecodec = prop->propval.u32;
    break;
  case PTP_OPC_AudioBitRate:
    track->bitrate = prop->propval.u32;
    break;
  case PTP_OPC_BitRateType:
    track->bitratetype = prop->propval.u16;
    break;
  case PTP_OPC_Rating:
    track->rating = prop->propval.u16;
    break;
  case PTP_OPC_UseCount:
    track->usecount = prop->propval.u32;
    break;
  case PTP_OPC_ObjectSize:
    if (device->object_bitsize == 64) {
      track->filesize = prop->propval.u64;
    } else {
      track->filesize = prop->propval.u32;
    }
    break;
  default:
    break;
  }
}

/**
 * This function retrieves the track metadata for a track
 * given by a unique ID.
 * @param device a pointer to the device to get the track metadata off.
 * @param objectformat the object format of this track, so we know what it supports.
 * @param track a metadata set to fill in.
 */
static void get_track_metadata(LIBMTP_mtpdevice_t *device, uint16_t objectformat,
			       LIBMTP_track_t *track)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  uint32_t i;
  MTPProperties *prop;
  PTPObject *ob;

  /*
   * If we have a cached, large set of metadata, then use it!
   */
  ret = ptp_object_want(params, track->item_id, PTPOBJECT_MTPPROPLIST_LOADED, &ob);
  if (ob->mtpprops) {
    prop = ob->mtpprops;
    for (i=0;i<ob->nrofmtpprops;i++,prop++)
      pick_property_to_track_metadata(device, prop, track);
  } else {
    uint16_t *props = NULL;
    uint32_t propcnt = 0;

    // First see which properties can be retrieved for this object format
    ret = ptp_mtp_getobjectpropssupported(params, map_libmtp_type_to_ptp_type(track->filetype), &propcnt, &props);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "get_track_metadata(): call to ptp_mtp_getobjectpropssupported() failed.");
      // Just bail out for now, nothing is ever set.
      return;
    } else {
      for (i=0;i<propcnt;i++) {
	switch (props[i]) {
	case PTP_OPC_Name:
	  track->title = get_string_from_object(device, track->item_id, PTP_OPC_Name);
	  break;
	case PTP_OPC_Artist:
	  track->artist = get_string_from_object(device, track->item_id, PTP_OPC_Artist);
	  break;
	case PTP_OPC_Composer:
	  track->composer = get_string_from_object(device, track->item_id, PTP_OPC_Composer);
	  break;
	case PTP_OPC_Duration:
	  track->duration = get_u32_from_object(device, track->item_id, PTP_OPC_Duration, 0);
	  break;
	case PTP_OPC_Track:
	  track->tracknumber = get_u16_from_object(device, track->item_id, PTP_OPC_Track, 0);
	  break;
	case PTP_OPC_Genre:
	  track->genre = get_string_from_object(device, track->item_id, PTP_OPC_Genre);
	  break;
	case PTP_OPC_AlbumName:
	  track->album = get_string_from_object(device, track->item_id, PTP_OPC_AlbumName);
	  break;
	case PTP_OPC_OriginalReleaseDate:
	  track->date = get_string_from_object(device, track->item_id, PTP_OPC_OriginalReleaseDate);
	  break;
	  // These are, well not so important.
	case PTP_OPC_SampleRate:
	  track->samplerate = get_u32_from_object(device, track->item_id, PTP_OPC_SampleRate, 0);
	  break;
	case PTP_OPC_NumberOfChannels:
	  track->nochannels = get_u16_from_object(device, track->item_id, PTP_OPC_NumberOfChannels, 0);
	  break;
	case PTP_OPC_AudioWAVECodec:
	  track->wavecodec = get_u32_from_object(device, track->item_id, PTP_OPC_AudioWAVECodec, 0);
	  break;
	case PTP_OPC_AudioBitRate:
	  track->bitrate = get_u32_from_object(device, track->item_id, PTP_OPC_AudioBitRate, 0);
	  break;
	case PTP_OPC_BitRateType:
	  track->bitratetype = get_u16_from_object(device, track->item_id, PTP_OPC_BitRateType, 0);
	  break;
	case PTP_OPC_Rating:
	  track->rating = get_u16_from_object(device, track->item_id, PTP_OPC_Rating, 0);
	  break;
	case PTP_OPC_UseCount:
	  track->usecount = get_u32_from_object(device, track->item_id, PTP_OPC_UseCount, 0);
	  break;
	case PTP_OPC_ObjectSize:
	  if (device->object_bitsize == 64) {
	    track->filesize = get_u64_from_object(device, track->item_id, PTP_OPC_ObjectSize, 0);
	  } else {
	    track->filesize = (uint64_t) get_u32_from_object(device, track->item_id, PTP_OPC_ObjectSize, 0);
	  }
	  break;
	}
      }
      free(props);
    }
  }
}

/**
 * THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 * @see LIBMTP_Get_Tracklisting_With_Callback()
 */
LIBMTP_track_t *LIBMTP_Get_Tracklisting(LIBMTP_mtpdevice_t *device)
{
  LIBMTP_INFO("WARNING: LIBMTP_Get_Tracklisting() is deprecated.\n");
  LIBMTP_INFO("WARNING: please update your code to use LIBMTP_Get_Tracklisting_With_Callback()\n");
  return LIBMTP_Get_Tracklisting_With_Callback(device, NULL, NULL);
}

/**
 * This returns a long list of all tracks available on the current MTP device.
 * Tracks include multimedia objects, both music tracks and video tracks.
 * Typical usage:
 *
 * <pre>
 * LIBMTP_track_t *tracklist;
 *
 * tracklist = LIBMTP_Get_Tracklisting_With_Callback(device, callback, data);
 * while (tracklist != NULL) {
 *   LIBMTP_track_t *tmp;
 *
 *   // Do something on each element in the list here...
 *   tmp = tracklist;
 *   tracklist = tracklist->next;
 *   LIBMTP_destroy_track_t(tmp);
 * }
 * </pre>
 *
 * If you want to group your track listing by storage (per storage unit) or
 * arrange tracks into folders, you must dereference the <code>storage_id</code>
 * and/or <code>parent_id</code> field of the returned <code>LIBMTP_track_t</code>
 * struct. To arrange by folders or files you typically have to create the proper
 * trees by calls to <code>LIBMTP_Get_Storage()</code> and/or
 * <code>LIBMTP_Get_Folder_List()</code> first.
 *
 * @param device a pointer to the device to get the track listing for.
 * @param callback a function to be called during the tracklisting retrieveal
 *        for displaying progress bars etc, or NULL if you don't want
 *        any callbacks.
 * @param data a user-defined pointer that is passed along to
 *        the <code>progress</code> function in order to
 *        pass along some user defined data to the progress
 *        updates. If not used, set this to NULL.
 * @return a list of tracks that can be followed using the <code>next</code>
 *        field of the <code>LIBMTP_track_t</code> data structure.
 *        Each of the metadata tags must be freed after use, and may
 *        contain only partial metadata information, i.e. one or several
 *        fields may be NULL or 0.
 * @see LIBMTP_Get_Trackmetadata()
 */
LIBMTP_track_t *LIBMTP_Get_Tracklisting_With_Callback(LIBMTP_mtpdevice_t *device,
                                                      LIBMTP_progressfunc_t const callback,
                                                      void const * const data)
{
	return LIBMTP_Get_Tracklisting_With_Callback_For_Storage(device, 0, callback, data);
}


/**
 * This returns a long list of all tracks available on the current MTP device.
 * Tracks include multimedia objects, both music tracks and video tracks.
 * Typical usage:
 *
 * <pre>
 * LIBMTP_track_t *tracklist;
 *
 * tracklist = LIBMTP_Get_Tracklisting_With_Callback_For_Storage(device, storage_id, callback, data);
 * while (tracklist != NULL) {
 *   LIBMTP_track_t *tmp;
 *
 *   // Do something on each element in the list here...
 *   tmp = tracklist;
 *   tracklist = tracklist->next;
 *   LIBMTP_destroy_track_t(tmp);
 * }
 * </pre>
 *
 * If you want to group your track listing by storage (per storage unit) or
 * arrange tracks into folders, you must dereference the <code>storage_id</code>
 * and/or <code>parent_id</code> field of the returned <code>LIBMTP_track_t</code>
 * struct. To arrange by folders or files you typically have to create the proper
 * trees by calls to <code>LIBMTP_Get_Storage()</code> and/or 
 * <code>LIBMTP_Get_Folder_List()</code> first.
 *
 * @param device a pointer to the device to get the track listing for.
 * @param storage_id ID of device storage (if null, no filter)
 * @param callback a function to be called during the tracklisting retrieveal
 *        for displaying progress bars etc, or NULL if you don't want
 *        any callbacks.
 * @param data a user-defined pointer that is passed along to
 *        the <code>progress</code> function in order to
 *        pass along some user defined data to the progress
 *        updates. If not used, set this to NULL.
 * @return a list of tracks that can be followed using the <code>next</code>
 *        field of the <code>LIBMTP_track_t</code> data structure.
 *        Each of the metadata tags must be freed after use, and may
 *        contain only partial metadata information, i.e. one or several
 *        fields may be NULL or 0.
 * @see LIBMTP_Get_Trackmetadata()
 */
LIBMTP_track_t *LIBMTP_Get_Tracklisting_With_Callback_For_Storage(LIBMTP_mtpdevice_t *device, uint32_t const storage_id,
                                                      LIBMTP_progressfunc_t const callback,
                                                      void const * const data)
{
  uint32_t i = 0;
  LIBMTP_track_t *retracks = NULL;
  LIBMTP_track_t *curtrack = NULL;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0) {
    flush_handles(device);
  }

  for (i = 0; i < params->nrofobjects; i++) {
    LIBMTP_track_t *track;
    PTPObject *ob;
    LIBMTP_filetype_t mtptype;

    if (callback != NULL)
      callback(i, params->nrofobjects, data);

    ob = &params->objects[i];
    mtptype = map_ptp_type_to_libmtp_type(ob->oi.ObjectFormat);

    // Ignore stuff we don't know how to handle...
    // TODO: get this list as an intersection of the sets
    // supported by the device and the from the device and
    // all known track files?
    if (!LIBMTP_FILETYPE_IS_TRACK(mtptype) &&
	// This row lets through undefined files for examination since they may be forgotten OGG files.
	(ob->oi.ObjectFormat != PTP_OFC_Undefined ||
	 (!FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) &&
	  !FLAG_OGG_IS_UNKNOWN(ptp_usb) &&
	  !FLAG_FLAC_IS_UNKNOWN(ptp_usb)))
	) {
      //printf("Not a music track (name: %s format: %d), skipping...\n", oi->Filename, oi->ObjectFormat);
      continue;
    }

	// Ignore stuff that isn't into the storage device
	if ((storage_id != 0) && (ob->oi.StorageID != storage_id ))
		continue;

    // Allocate a new track type
    track = LIBMTP_new_track_t();

    // This is some sort of unique ID so we can keep track of the track.
    track->item_id = ob->oid;
    track->parent_id = ob->oi.ParentObject;
    track->storage_id = ob->oi.StorageID;
    track->modificationdate = ob->oi.ModificationDate;

    track->filetype = mtptype;

    // Original file-specific properties
    track->filesize = ob->oi.ObjectCompressedSize;
    if (ob->oi.Filename != NULL) {
      track->filename = strdup(ob->oi.Filename);
    }

    get_track_metadata(device, ob->oi.ObjectFormat, track);

    /*
     * A special quirk for iriver devices that doesn't quite
     * remember that some files marked as "unknown" type are
     * actually OGG or FLAC files. We look at the filename extension
     * and see if it happens that this was atleast named "ogg" or "flac"
     * and fall back on this heuristic approach in that case,
     * for these bugged devices only.
     */
    if (track->filetype == LIBMTP_FILETYPE_UNKNOWN &&
	track->filename != NULL) {
      if ((FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) ||
	   FLAG_OGG_IS_UNKNOWN(ptp_usb)) &&
	  has_ogg_extension(track->filename))
	track->filetype = LIBMTP_FILETYPE_OGG;
      else if (FLAG_FLAC_IS_UNKNOWN(ptp_usb) &&
	       has_flac_extension(track->filename))
	track->filetype = LIBMTP_FILETYPE_FLAC;
      else {
	// This was not an OGG/FLAC file so discard it and continue
	LIBMTP_destroy_track_t(track);
	continue;
      }
    }

    // Add track to a list that will be returned afterwards.
    if (retracks == NULL) {
      retracks = track;
      curtrack = track;
    } else {
      curtrack->next = track;
      curtrack = track;
    }

    // Call listing callback
    // double progressPercent = (double)i*(double)100.0 / (double)params->handles.n;

  } // Handle counting loop
  return retracks;
}

/**
 * This function retrieves the metadata for a single track off
 * the device.
 *
 * Do not call this function repeatedly! The track handles are linearly
 * searched O(n) and the call may involve (slow) USB traffic, so use
 * <code>LIBMTP_Get_Tracklisting()</code> and cache the tracks, preferably
 * as an efficient data structure such as a hash list.
 *
 * @param device a pointer to the device to get the track metadata from.
 * @param trackid the object ID of the track that you want the metadata for.
 * @return a track metadata entry on success or NULL on failure.
 * @see LIBMTP_Get_Tracklisting()
 */
LIBMTP_track_t *LIBMTP_Get_Trackmetadata(LIBMTP_mtpdevice_t *device, uint32_t const trackid)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  PTPObject *ob;
  LIBMTP_track_t *track;
  LIBMTP_filetype_t mtptype;
  uint16_t ret;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0)
    flush_handles(device);

  ret = ptp_object_want (params, trackid, PTPOBJECT_OBJECTINFO_LOADED, &ob);
  if (ret != PTP_RC_OK)
    return NULL;

  mtptype = map_ptp_type_to_libmtp_type(ob->oi.ObjectFormat);

  // Ignore stuff we don't know how to handle...
  if (!LIBMTP_FILETYPE_IS_TRACK(mtptype) &&
      /*
       * This row lets through undefined files for examination
       * since they may be forgotten OGG or FLAC files.
       */
      (ob->oi.ObjectFormat != PTP_OFC_Undefined ||
       (!FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) &&
	!FLAG_OGG_IS_UNKNOWN(ptp_usb) &&
	!FLAG_FLAC_IS_UNKNOWN(ptp_usb)))
      ) {
    //printf("Not a music track (name: %s format: %d), skipping...\n", oi->Filename, oi->ObjectFormat);
    return NULL;
  }

  // Allocate a new track type
  track = LIBMTP_new_track_t();

  // This is some sort of unique ID so we can keep track of the track.
  track->item_id = ob->oid;
  track->parent_id = ob->oi.ParentObject;
  track->storage_id = ob->oi.StorageID;
  track->modificationdate = ob->oi.ModificationDate;

  track->filetype = mtptype;

  // Original file-specific properties
  track->filesize = ob->oi.ObjectCompressedSize;
  if (ob->oi.Filename != NULL) {
    track->filename = strdup(ob->oi.Filename);
  }

  /*
   * A special quirk for devices that doesn't quite
   * remember that some files marked as "unknown" type are
   * actually OGG or FLAC files. We look at the filename extension
   * and see if it happens that this was atleast named "ogg"
   * and fall back on this heuristic approach in that case,
   * for these bugged devices only.
   */
  if (track->filetype == LIBMTP_FILETYPE_UNKNOWN &&
      track->filename != NULL) {
    if ((FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) ||
	 FLAG_OGG_IS_UNKNOWN(ptp_usb)) &&
	has_ogg_extension(track->filename))
      track->filetype = LIBMTP_FILETYPE_OGG;
    else if (FLAG_FLAC_IS_UNKNOWN(ptp_usb) &&
	     has_flac_extension(track->filename))
      track->filetype = LIBMTP_FILETYPE_FLAC;
    else {
      // This was not an OGG/FLAC file so discard it
      LIBMTP_destroy_track_t(track);
      return NULL;
    }
  }
  get_track_metadata(device, ob->oi.ObjectFormat, track);
  return track;
}

/**
 * This is a manual conversion from MTPDataGetFunc to PTPDataGetFunc
 * to isolate the internal type.
 */
static uint16_t get_func_wrapper(PTPParams* params, void* priv, unsigned long wantlen, unsigned char *data, unsigned long *gotlen)
{
  MTPDataHandler *handler = (MTPDataHandler *)priv;
  uint16_t ret;
  uint32_t local_gotlen = 0;
  ret = handler->getfunc(params, handler->priv, wantlen, data, &local_gotlen);
  *gotlen = local_gotlen;
  switch (ret)
  {
    case LIBMTP_HANDLER_RETURN_OK:
      return PTP_RC_OK;
    case LIBMTP_HANDLER_RETURN_ERROR:
      return PTP_ERROR_IO;
    case LIBMTP_HANDLER_RETURN_CANCEL:
      return PTP_ERROR_CANCEL;
    default:
      return PTP_ERROR_IO;
  }
}

/**
 * This is a manual conversion from MTPDataPutFunc to PTPDataPutFunc
 * to isolate the internal type.
 */
static uint16_t put_func_wrapper(PTPParams* params, void* priv, unsigned long sendlen, unsigned char *data)
{
  MTPDataHandler *handler = (MTPDataHandler *)priv;
  uint16_t ret;
  uint32_t local_putlen = 0;

  ret = handler->putfunc(params, handler->priv, sendlen, data, &local_putlen);

  switch (ret)
  {
    case LIBMTP_HANDLER_RETURN_OK:
      if (local_putlen != sendlen)
	return PTP_ERROR_IO;
      return PTP_RC_OK;
    case LIBMTP_HANDLER_RETURN_ERROR:
      return PTP_ERROR_IO;
    case LIBMTP_HANDLER_RETURN_CANCEL:
      return PTP_ERROR_CANCEL;
    default:
      return PTP_ERROR_IO;
  }
}

/**
 * This gets a file off the device to a local file identified
 * by a filename.
 * @param device a pointer to the device to get the track from.
 * @param id the file ID of the file to retrieve.
 * @param path a filename to use for the retrieved file.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_File_To_File_Descriptor()
 */
int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *device, uint32_t const id,
			 char const * const path, LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  int fd = -1;
  int ret;

  // Sanity check
  if (path == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File(): Bad arguments, path was NULL.");
    return -1;
  }

  // Open file
#ifdef __WIN32__
#ifdef USE_WINDOWS_IO_H
  if ( (fd = _open(path, O_RDWR|O_CREAT|O_TRUNC|O_BINARY,_S_IREAD)) == -1 ) {
#else
  if ( (fd = open(path, O_RDWR|O_CREAT|O_TRUNC|O_BINARY,S_IRWXU)) == -1 ) {
#endif
#else
  if ( (fd = open(path, O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRGRP)) == -1) {
#endif
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File(): Could not create file.");
    return -1;
  }

  ret = LIBMTP_Get_File_To_File_Descriptor(device, id, fd, callback, data);

  // Close file
  close(fd);

  // Delete partial file.
  if (ret == -1) {
    unlink(path);
  }

  return ret;
}

/**
 * This gets a file off the device to a file identified
 * by a file descriptor.
 *
 * This function can potentially be used for streaming
 * files off the device for playback or broadcast for example,
 * by downloading the file into a stream sink e.g. a socket.
 *
 * @param device a pointer to the device to get the file from.
 * @param id the file ID of the file to retrieve.
 * @param fd a local file descriptor to write the file to.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_File_To_File()
 */
int LIBMTP_Get_File_To_File_Descriptor(LIBMTP_mtpdevice_t *device,
					uint32_t const id,
					int const fd,
					LIBMTP_progressfunc_t const callback,
					void const * const data)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  LIBMTP_file_t *mtpfile = LIBMTP_Get_Filemetadata(device, id);
  if (mtpfile == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Could not get object info.");
    return -1;
  }
  if (mtpfile->filetype == LIBMTP_FILETYPE_FOLDER) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Bad object format.");
    LIBMTP_destroy_file_t (mtpfile);
    return -1;
  }

  // Callbacks
  ptp_usb->callback_active = 1;
  ptp_usb->current_transfer_total = mtpfile->filesize +
    PTP_USB_BULK_HDR_LEN+sizeof(uint32_t); // Request length, one parameter
  ptp_usb->current_transfer_complete = 0;
  ptp_usb->current_transfer_callback = callback;
  ptp_usb->current_transfer_callback_data = data;

  // Don't need mtpfile anymore
  LIBMTP_destroy_file_t(mtpfile);

  ret = ptp_getobject_tofd(params, id, fd);

  ptp_usb->callback_active = 0;
  ptp_usb->current_transfer_callback = NULL;
  ptp_usb->current_transfer_callback_data = NULL;

  if (ret == PTP_ERROR_CANCEL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Get_File_From_File_Descriptor(): Cancelled transfer.");
    return -1;
  }
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_File_To_File_Descriptor(): Could not get file from device.");
    return -1;
  }

  return 0;
}

/**
 * This gets a file off the device and calls put_func
 * with chunks of data
 *
 * @param device a pointer to the device to get the file from.
 * @param id the file ID of the file to retrieve.
 * @param put_func the function to call when we have data.
 * @param priv the user-defined pointer that is passed to
 *             <code>put_func</code>.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 */
int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device,
					uint32_t const id,
					MTPDataPutFunc put_func,
          void * priv,
					LIBMTP_progressfunc_t const callback,
					void const * const data)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;

  LIBMTP_file_t *mtpfile = LIBMTP_Get_Filemetadata(device, id);
  if (mtpfile == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Could not get object info.");
    return -1;
  }
  if (mtpfile->filetype == LIBMTP_FILETYPE_FOLDER) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_File_To_File_Descriptor(): Bad object format.");
    LIBMTP_destroy_file_t (mtpfile);
    return -1;
  }

  // Callbacks
  ptp_usb->callback_active = 1;
  ptp_usb->current_transfer_total = mtpfile->filesize +
    PTP_USB_BULK_HDR_LEN+sizeof(uint32_t); // Request length, one parameter
  ptp_usb->current_transfer_complete = 0;
  ptp_usb->current_transfer_callback = callback;
  ptp_usb->current_transfer_callback_data = data;

  // Don't need mtpfile anymore
  LIBMTP_destroy_file_t(mtpfile);

  MTPDataHandler mtp_handler;
  mtp_handler.getfunc = NULL;
  mtp_handler.putfunc = put_func;
  mtp_handler.priv = priv;

  PTPDataHandler handler;
  handler.getfunc = NULL;
  handler.putfunc = put_func_wrapper;
  handler.priv = &mtp_handler;

  ret = ptp_getobject_to_handler(params, id, &handler);

  ptp_usb->callback_active = 0;
  ptp_usb->current_transfer_callback = NULL;
  ptp_usb->current_transfer_callback_data = NULL;

  if (ret == PTP_ERROR_CANCEL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Get_File_From_File_Descriptor(): Cancelled transfer.");
    return -1;
  }
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_File_To_File_Descriptor(): Could not get file from device.");
    return -1;
  }

  return 0;
}


/**
 * This gets a track off the device to a file identified
 * by a filename. This is actually just a wrapper for the
 * \c LIBMTP_Get_Track_To_File() function.
 * @param device a pointer to the device to get the track from.
 * @param id the track ID of the track to retrieve.
 * @param path a filename to use for the retrieved track.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_Track_To_File_Descriptor()
 */
int LIBMTP_Get_Track_To_File(LIBMTP_mtpdevice_t *device, uint32_t const id,
			 char const * const path, LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  // This is just a wrapper
  return LIBMTP_Get_File_To_File(device, id, path, callback, data);
}

/**
 * This gets a track off the device to a file identified
 * by a file descriptor. This is actually just a wrapper for
 * the \c LIBMTP_Get_File_To_File_Descriptor() function.
 * @param device a pointer to the device to get the track from.
 * @param id the track ID of the track to retrieve.
 * @param fd a file descriptor to write the track to.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Get_Track_To_File()
 */
int LIBMTP_Get_Track_To_File_Descriptor(LIBMTP_mtpdevice_t *device,
					uint32_t const id,
					int const fd,
					LIBMTP_progressfunc_t const callback,
					void const * const data)
{
  // This is just a wrapper
  return LIBMTP_Get_File_To_File_Descriptor(device, id, fd, callback, data);
}

/**
 * This gets a track off the device to a handler function.
 * This is actually just a wrapper for
 * the \c LIBMTP_Get_File_To_Handler() function.
 * @param device a pointer to the device to get the track from.
 * @param id the track ID of the track to retrieve.
 * @param put_func the function to call when we have data.
 * @param priv the user-defined pointer that is passed to
 *             <code>put_func</code>.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 */
int LIBMTP_Get_Track_To_Handler(LIBMTP_mtpdevice_t *device,
					uint32_t const id,
					MTPDataPutFunc put_func,
          void * priv,
					LIBMTP_progressfunc_t const callback,
					void const * const data)
{
  // This is just a wrapper
  return LIBMTP_Get_File_To_Handler(device, id, put_func, priv, callback, data);
}

/**
 * This function sends a track from a local file to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 * @param device a pointer to the device to send the track to.
 * @param path the filename of a local file which will be sent.
 * @param metadata a track metadata set to be written along with the file.
 *        After this call the field <code>metadata-&gt;item_id</code>
 *        will contain the new track ID. Other fields such
 *        as the <code>metadata-&gt;filename</code>, <code>metadata-&gt;parent_id</code>
 *        or <code>metadata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this track in. Since some
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_Track_From_File_Descriptor()
 * @see LIBMTP_Send_File_From_File()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_Track_From_File(LIBMTP_mtpdevice_t *device,
			 char const * const path, LIBMTP_track_t * const metadata,
                         LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  int fd;
  int ret;

  // Sanity check
  if (path == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_Track_From_File(): Bad arguments, path was NULL.");
    return -1;
  }

  // Open file
#ifdef __WIN32__
#ifdef USE_WINDOWS_IO_H
  if ( (fd = _open(path, O_RDONLY|O_BINARY)) == -1 ) {
#else
  if ( (fd = open(path, O_RDONLY|O_BINARY)) == -1 ) {
#endif
#else
  if ( (fd = open(path, O_RDONLY)) == -1) {
#endif
    LIBMTP_ERROR("LIBMTP_Send_Track_From_File(): Could not open source file \"%s\"\n", path);
    return -1;
  }

  ret = LIBMTP_Send_Track_From_File_Descriptor(device, fd, metadata, callback, data);

  // Close file.
#ifdef USE_WINDOWS_IO_H
  _close(fd);
#else
  close(fd);
#endif

  return ret;
}



/**
 * This helper function checks if a filename already exists on the device
 * @param params the PTP params to check against
 * @param filename a string representing the filename
 * @return 0 if the filename doesn't exist, -1 if it does
 */
static int check_filename_exists(PTPParams* params, char const * const filename)
{
  unsigned int i;

  for (i = 0; i < params->nrofobjects; i++) {
    char *fname = params->objects[i].oi.Filename;
    if ((fname != NULL) && (strcmp(filename, fname) == 0))
    {
      return -1;
    }
  }

  return 0;
}

/**
 * This helper function returns a unique filename, with a random string before the extension
 * @param params the PTP params to check against
 * @param filename string representing the original filename
 * @return a string representing the unique filename
 */
static char *generate_unique_filename(PTPParams* params, char const * const filename)
{
  int suffix;
  char * extension_position;

  if (check_filename_exists(params, filename))
  {
    extension_position = strrchr(filename,'.');

    char basename[extension_position - filename + 1];
    strncpy(basename, filename, extension_position - filename);
    basename[extension_position - filename] = '\0';

    suffix = 1;
    char newname[ strlen(basename) + 6 + strlen(extension_position)];
    sprintf(newname, "%s_%d%s", basename, suffix, extension_position);
    while ((check_filename_exists(params, newname)) && (suffix < 1000000)) {
      suffix++;
      sprintf(newname, "%s_%d%s", basename, suffix, extension_position);
    }
  return strdup(newname);
  }
  else
  {
    return strdup(filename);
  }
}

/**
 * This function sends a track from a file descriptor to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 * @param device a pointer to the device to send the track to.
 * @param fd the filedescriptor for a local file which will be sent.
 * @param metadata a track metadata set to be written along with the file.
 *        After this call the field <code>metadata-&gt;item_id</code>
 *        will contain the new track ID. Other fields such
 *        as the <code>metadata-&gt;filename</code>, <code>metadata-&gt;parent_id</code>
 *        or <code>metadata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this track in. Since some
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_Track_From_File()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_Track_From_File_Descriptor(LIBMTP_mtpdevice_t *device,
			 int const fd, LIBMTP_track_t * const metadata,
                         LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  int subcall_ret;
  LIBMTP_file_t filedata;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  PTPParams *params = (PTPParams *) device->params;

  // Sanity check, is this really a track?
  if (!LIBMTP_FILETYPE_IS_TRACK(metadata->filetype)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_Track_From_File_Descriptor(): "
			    "I don't think this is actually a track, strange filetype...");
  }

  // Wrap around the file transfer function
  filedata.item_id = metadata->item_id;
  filedata.parent_id = metadata->parent_id;
  filedata.storage_id = metadata->storage_id;
  if FLAG_UNIQUE_FILENAMES(ptp_usb) {
    filedata.filename = generate_unique_filename(params, metadata->filename);
  }
  else {
    filedata.filename = metadata->filename;
  }
  filedata.filesize = metadata->filesize;
  filedata.filetype = metadata->filetype;
  filedata.next = NULL;

  subcall_ret = LIBMTP_Send_File_From_File_Descriptor(device,
						      fd,
						      &filedata,
						      callback,
						      data);

  if (subcall_ret != 0) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_Track_From_File_Descriptor(): "
			    "subcall to LIBMTP_Send_File_From_File_Descriptor failed.");
    // We used to delete the file here, but don't... It might be OK after all.
    // (void) LIBMTP_Delete_Object(device, metadata->item_id);
    return -1;
  }

  // Pick up new item (and parent, storage) ID
  metadata->item_id = filedata.item_id;
  metadata->parent_id = filedata.parent_id;
  metadata->storage_id = filedata.storage_id;

  // Set track metadata for the new fine track
  subcall_ret = LIBMTP_Update_Track_Metadata(device, metadata);
  if (subcall_ret != 0) {
    // Subcall will add error to errorstack
    // We used to delete the file here, but don't... It might be OK after all.
    // (void) LIBMTP_Delete_Object(device, metadata->item_id);
    return -1;
  }

  // note we don't need to update the cache here because LIBMTP_Send_File_From_File_Descriptor
  // has added the object handle and LIBMTP_Update_Track_Metadata has added the metadata.

  return 0;
}

/**
 * This function sends a track from a handler function to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 * @param device a pointer to the device to send the track to.
 * @param get_func the function to call when we have data.
 * @param priv the user-defined pointer that is passed to
 *             <code>get_func</code>.
 * @param metadata a track metadata set to be written along with the file.
 *        After this call the field <code>metadata-&gt;item_id</code>
 *        will contain the new track ID. Other fields such
 *        as the <code>metadata-&gt;filename</code>, <code>metadata-&gt;parent_id</code>
 *        or <code>metadata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this track in. Since some
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_Track_From_File()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_Track_From_Handler(LIBMTP_mtpdevice_t *device,
			 MTPDataGetFunc get_func, void * priv, LIBMTP_track_t * const metadata,
                         LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  int subcall_ret;
  LIBMTP_file_t filedata;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  PTPParams *params = (PTPParams *) device->params;

  // Sanity check, is this really a track?
  if (!LIBMTP_FILETYPE_IS_TRACK(metadata->filetype)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_Track_From_Handler(): "
			    "I don't think this is actually a track, strange filetype...");
  }

  // Wrap around the file transfer function
  filedata.item_id = metadata->item_id;
  filedata.parent_id = metadata->parent_id;
  filedata.storage_id = metadata->storage_id;
  if FLAG_UNIQUE_FILENAMES(ptp_usb) {
    filedata.filename = generate_unique_filename(params, metadata->filename);
  }
  else {
    filedata.filename = metadata->filename;
  }
  filedata.filesize = metadata->filesize;
  filedata.filetype = metadata->filetype;
  filedata.next = NULL;

  subcall_ret = LIBMTP_Send_File_From_Handler(device,
					      get_func,
					      priv,
					      &filedata,
					      callback,
					      data);

  if (subcall_ret != 0) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_Track_From_Handler(): "
			    "subcall to LIBMTP_Send_File_From_Handler failed.");
    // We used to delete the file here, but don't... It might be OK after all.
    // (void) LIBMTP_Delete_Object(device, metadata->item_id);
    return -1;
  }

  // Pick up new item (and parent, storage) ID
  metadata->item_id = filedata.item_id;
  metadata->parent_id = filedata.parent_id;
  metadata->storage_id = filedata.storage_id;

  // Set track metadata for the new fine track
  subcall_ret = LIBMTP_Update_Track_Metadata(device, metadata);
  if (subcall_ret != 0) {
    // Subcall will add error to errorstack
    // We used to delete the file here, but don't... It might be OK after all.
    // (void) LIBMTP_Delete_Object(device, metadata->item_id);
    return -1;
  }

  // note we don't need to update the cache here because LIBMTP_Send_File_From_File_Descriptor
  // has added the object handle and LIBMTP_Update_Track_Metadata has added the metadata.

  return 0;
}

/**
 * This function sends a local file to an MTP device.
 * A filename and a set of metadata must be
 * given as input.
 * @param device a pointer to the device to send the track to.
 * @param path the filename of a local file which will be sent.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_File(LIBMTP_mtpdevice_t *device,
			       char const * const path, LIBMTP_file_t * const filedata,
			       LIBMTP_progressfunc_t const callback,
			       void const * const data)
{
  int fd;
  int ret;

  // Sanity check
  if (path == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_File_From_File(): Bad arguments, path was NULL.");
    return -1;
  }

  // Open file
#ifdef __WIN32__
#ifdef USE_WINDOWS_IO_H
  if ( (fd = _open(path, O_RDONLY|O_BINARY)) == -1 ) {
#else
  if ( (fd = open(path, O_RDONLY|O_BINARY)) == -1 ) {
#endif
#else
  if ( (fd = open(path, O_RDONLY)) == -1) {
#endif
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_File_From_File(): Could not open source file.");
    return -1;
  }

  ret = LIBMTP_Send_File_From_File_Descriptor(device, fd, filedata, callback, data);

  // Close file.
#ifdef USE_WINDOWS_IO_H
  _close(fd);
#else
  close(fd);
#endif

  return ret;
}

/**
 * This function sends a generic file from a file descriptor to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 *
 * This can potentially be used for sending in a stream of unknown
 * length. Send music files with
 * <code>LIBMTP_Send_Track_From_File_Descriptor()</code>
 *
 * @param device a pointer to the device to send the file to.
 * @param fd the filedescriptor for a local file which will be sent.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File()
 * @see LIBMTP_Send_Track_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_File_Descriptor(LIBMTP_mtpdevice_t *device,
			 int const fd, LIBMTP_file_t * const filedata,
                         LIBMTP_progressfunc_t const callback,
			 void const * const data)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  LIBMTP_file_t *newfilemeta;
  int oldtimeout;
  int timeout;

  if (send_file_object_info(device, filedata))
  {
    // no need to output an error since send_file_object_info will already have done so
    return -1;
  }

  // Callbacks
  ptp_usb->callback_active = 1;
  // The callback will deactivate itself after this amount of data has been sent
  // One BULK header for the request, one for the data phase. No parameters to the request.
  ptp_usb->current_transfer_total = filedata->filesize+PTP_USB_BULK_HDR_LEN*2;
  ptp_usb->current_transfer_complete = 0;
  ptp_usb->current_transfer_callback = callback;
  ptp_usb->current_transfer_callback_data = data;

  /*
   * We might need to increase the timeout here, files can be pretty
   * large. Take the default timeout and add the calculated time for
   * this transfer
   */
  get_usb_device_timeout(ptp_usb, &oldtimeout);
  timeout = oldtimeout +
    (ptp_usb->current_transfer_total / guess_usb_speed(ptp_usb)) * 1000;
  set_usb_device_timeout(ptp_usb, timeout);

  ret = ptp_sendobject_fromfd(params, fd, filedata->filesize);

  ptp_usb->callback_active = 0;
  ptp_usb->current_transfer_callback = NULL;
  ptp_usb->current_transfer_callback_data = NULL;
  set_usb_device_timeout(ptp_usb, oldtimeout);

  if (ret == PTP_ERROR_CANCEL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Send_File_From_File_Descriptor(): Cancelled transfer.");
    return -1;
  }
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_File_From_File_Descriptor(): "
				"Could not send object.");
    return -1;
  }

  add_object_to_cache(device, filedata->item_id);

  /*
   * Get the device-assigned parent_id from the cache.
   * The operation that adds it to the cache will
   * look it up from the device, so we get the new
   * parent_id from the cache.
   */
  newfilemeta = LIBMTP_Get_Filemetadata(device, filedata->item_id);
  if (newfilemeta != NULL) {
    filedata->parent_id = newfilemeta->parent_id;
    filedata->storage_id = newfilemeta->storage_id;
    LIBMTP_destroy_file_t(newfilemeta);
  } else {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_File_From_File_Descriptor(): "
			    "Could not retrieve updated metadata.");
    return -1;
  }

  return 0;
}

/**
 * This function sends a generic file from a handler function to an
 * MTP device. A filename and a set of metadata must be
 * given as input.
 *
 * This can potentially be used for sending in a stream of unknown
 * length. Send music files with
 * <code>LIBMTP_Send_Track_From_Handler()</code>
 *
 * @param device a pointer to the device to send the file to.
 * @param get_func the function to call to get data to write
 * @param priv a user-defined pointer that is passed along to
 *        <code>get_func</code>. If not used, this is set to NULL.
 * @param filedata a file metadata set to be written along with the file.
 *        After this call the field <code>filedata-&gt;item_id</code>
 *        will contain the new file ID. Other fields such
 *        as the <code>filedata-&gt;filename</code>, <code>filedata-&gt;parent_id</code>
 *        or <code>filedata-&gt;storage_id</code> may also change during this
 *        operation due to device restrictions, so do not rely on the
 *        contents of this struct to be preserved in any way.
 *        <ul>
 *        <li><code>filedata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this file in. If this is 0,
 *        the file will be stored in the root folder.
 *        <li><code>filedata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this file in. Setting this to 0 will store
 *        the file on the primary storage.
 *        </ul>
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 * @see LIBMTP_Send_File_From_File()
 * @see LIBMTP_Send_Track_From_File_Descriptor()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Send_File_From_Handler(LIBMTP_mtpdevice_t *device,
			 MTPDataGetFunc get_func, void * priv, LIBMTP_file_t * const filedata,
       LIBMTP_progressfunc_t const callback, void const * const data)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  LIBMTP_file_t *newfilemeta;

  if (send_file_object_info(device, filedata))
  {
    // no need to output an error since send_file_object_info will already have done so
    return -1;
  }

  // Callbacks
  ptp_usb->callback_active = 1;
  // The callback will deactivate itself after this amount of data has been sent
  // One BULK header for the request, one for the data phase. No parameters to the request.
  ptp_usb->current_transfer_total = filedata->filesize+PTP_USB_BULK_HDR_LEN*2;
  ptp_usb->current_transfer_complete = 0;
  ptp_usb->current_transfer_callback = callback;
  ptp_usb->current_transfer_callback_data = data;

  MTPDataHandler mtp_handler;
  mtp_handler.getfunc = get_func;
  mtp_handler.putfunc = NULL;
  mtp_handler.priv = priv;

  PTPDataHandler handler;
  handler.getfunc = get_func_wrapper;
  handler.putfunc = NULL;
  handler.priv = &mtp_handler;

  ret = ptp_sendobject_from_handler(params, &handler, filedata->filesize);

  ptp_usb->callback_active = 0;
  ptp_usb->current_transfer_callback = NULL;
  ptp_usb->current_transfer_callback_data = NULL;

  if (ret == PTP_ERROR_CANCEL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, "LIBMTP_Send_File_From_Handler(): Cancelled transfer.");
    return -1;
  }
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_File_From_Handler(): "
				"Could not send object.");
    return -1;
  }

  add_object_to_cache(device, filedata->item_id);

  /*
   * Get the device-assined parent_id from the cache.
   * The operation that adds it to the cache will
   * look it up from the device, so we get the new
   * parent_id from the cache.
   */
  newfilemeta = LIBMTP_Get_Filemetadata(device, filedata->item_id);
  if (newfilemeta != NULL) {
    filedata->parent_id = newfilemeta->parent_id;
    filedata->storage_id = newfilemeta->storage_id;
    LIBMTP_destroy_file_t(newfilemeta);
  } else {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
			    "LIBMTP_Send_File_From_Handler(): "
			    "Could not retrieve updated metadata.");
    return -1;
  }

  return 0;
}

/**
 * This function sends the file object info, ready for sendobject
 * @param device a pointer to the device to send the file to.
 * @param filedata a file metadata set to be written along with the file.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 */
static int send_file_object_info(LIBMTP_mtpdevice_t *device, LIBMTP_file_t *filedata)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint32_t store;
  int use_primary_storage = 1;
  uint16_t of = map_libmtp_type_to_ptp_type(filedata->filetype);
  LIBMTP_devicestorage_t *storage;
  uint32_t localph = filedata->parent_id;
  uint16_t ret;
  unsigned int i;

#if 0
  // Sanity check: no zerolength files on some devices?
  // If the zerolength files cause problems on some devices,
  // then add a bug flag for this.
  if (filedata->filesize == 0) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "send_file_object_info(): "
			    "File of zero size.");
    return -1;
  }
#endif
  if (filedata->storage_id != 0) {
    store = filedata->storage_id;
  } else {
    store = get_suggested_storage_id(device, filedata->filesize, localph);
  }

  // Detect if something non-primary is in use.
  storage = device->storage;
  if (storage != NULL && store != storage->id) {
    use_primary_storage = 0;
  }

  /*
   * If no destination folder was given, look up a default
   * folder if possible. Perhaps there is some way of retrieveing
   * the default folder for different forms of content, what
   * do I know, we use a fixed list in lack of any better method.
   * Some devices obviously need to have their files in certain
   * folders in order to find/display them at all (hello Creative),
   * so we have to have a method for this. We only do this if the
   * primary storage is in use.
   */

  if (localph == 0 && use_primary_storage) {
    if (LIBMTP_FILETYPE_IS_AUDIO(filedata->filetype)) {
      localph = device->default_music_folder;
    } else if (LIBMTP_FILETYPE_IS_VIDEO(filedata->filetype)) {
      localph = device->default_video_folder;
    } else if (of == PTP_OFC_EXIF_JPEG ||
	       of == PTP_OFC_JP2 ||
	       of == PTP_OFC_JPX ||
	       of == PTP_OFC_JFIF ||
	       of == PTP_OFC_TIFF ||
	       of == PTP_OFC_TIFF_IT ||
	       of == PTP_OFC_BMP ||
	       of == PTP_OFC_GIF ||
	       of == PTP_OFC_PICT ||
	       of == PTP_OFC_PNG ||
	       of == PTP_OFC_MTP_WindowsImageFormat) {
      localph = device->default_picture_folder;
    } else if (of == PTP_OFC_MTP_vCalendar1 ||
	       of == PTP_OFC_MTP_vCalendar2 ||
	       of == PTP_OFC_MTP_UndefinedContact ||
	       of == PTP_OFC_MTP_vCard2 ||
	       of == PTP_OFC_MTP_vCard3 ||
	       of == PTP_OFC_MTP_UndefinedCalendarItem) {
      localph = device->default_organizer_folder;
    } else if (of == PTP_OFC_Text) {
      localph = device->default_text_folder;
    }
  }

  // Here we wire the type to unknown on bugged, but
  // Ogg or FLAC-supportive devices.
  if (FLAG_OGG_IS_UNKNOWN(ptp_usb) && of == PTP_OFC_MTP_OGG) {
    of = PTP_OFC_Undefined;
  }
  if (FLAG_FLAC_IS_UNKNOWN(ptp_usb) && of == PTP_OFC_MTP_FLAC) {
    of = PTP_OFC_Undefined;
  }

  if (ptp_operation_issupported(params, PTP_OC_MTP_SendObjectPropList) &&
      !FLAG_BROKEN_SEND_OBJECT_PROPLIST(ptp_usb)) {
    /*
     * MTP enhanched does it this way (from a sniff):
     * -> PTP_OC_MTP_SendObjectPropList (0x9808):
     *    20 00 00 00 01 00 08 98 1B 00 00 00 01 00 01 00
     *    FF FF FF FF 00 30 00 00 00 00 00 00 12 5E 00 00
     *    Length: 0x00000020
     *    Type:   0x0001 PTP_USB_CONTAINER_COMMAND
     *    Code:   0x9808
     *    Transaction ID: 0x0000001B
     *    Param1: 0x00010001 <- store
     *    Param2: 0xffffffff <- parent handle (-1 ?)
     *    Param3: 0x00003000 <- file type PTP_OFC_Undefined - we don't know about PDF files
     *    Param4: 0x00000000 <- file length MSB (-0x0c header len)
     *    Param5: 0x00005e12 <- file length LSB (-0x0c header len)
     *
     * -> PTP_OC_MTP_SendObjectPropList (0x9808):
     *    46 00 00 00 02 00 08 98 1B 00 00 00 03 00 00 00
     *    00 00 00 00 07 DC FF FF 0D 4B 00 53 00 30 00 36 - dc07 = file name
     *    00 30 00 33 00 30 00 36 00 2E 00 70 00 64 00 66
     *    00 00 00 00 00 00 00 03 DC 04 00 00 00 00 00 00 - dc03 = protection status
     *    00 4F DC 02 00 01                               - dc4f = non consumable
     *    Length: 0x00000046
     *    Type:   0x0002 PTP_USB_CONTAINER_DATA
     *    Code:   0x9808
     *    Transaction ID: 0x0000001B
     *    Metadata....
     *    0x00000003 <- Number of metadata items
     *    0x00000000 <- Object handle, set to 0x00000000 since it is unknown!
     *    0xdc07     <- metadata type: file name
     *    0xffff     <- metadata type: string
     *    0x0d       <- number of (uint16_t) characters
     *    4b 53 30 36 30 33 30 36 2e 50 64 66 00 "KS060306.pdf", null terminated
     *    0x00000000 <- Object handle, set to 0x00000000 since it is unknown!
     *    0xdc03     <- metadata type: protection status
     *    0x0004     <- metadata type: uint16_t
     *    0x0000     <- not protected
     *    0x00000000 <- Object handle, set to 0x00000000 since it is unknown!
     *    0xdc4f     <- non consumable
     *    0x0002     <- metadata type: uint8_t
     *    0x01       <- non-consumable (this device cannot display PDF)
     *
     * <- Read 0x18 bytes back
     *    18 00 00 00 03 00 01 20 1B 00 00 00 01 00 01 00
     *    00 00 00 00 01 40 00 00
     *    Length: 0x000000018
     *    Type:   0x0003 PTP_USB_CONTAINER_RESPONSE
     *    Code:   0x2001 PTP_OK
     *    Transaction ID: 0x0000001B
     *    Param1: 0x00010001 <- store
     *    Param2: 0x00000000 <- parent handle
     *    Param3: 0x00004001 <- new file/object ID
     *
     * -> PTP_OC_SendObject (0x100d)
     *    0C 00 00 00 01 00 0D 10 1C 00 00 00
     * -> ... all the bytes ...
     * <- Read 0x0c bytes back
     *    0C 00 00 00 03 00 01 20 1C 00 00 00
     *    ... Then update metadata one-by one, actually (instead of sending it first!) ...
     */
    MTPProperties *props = NULL;
    int nrofprops = 0;
    MTPProperties *prop = NULL;
    uint16_t *properties = NULL;
    uint32_t propcnt = 0;

    // default parent handle
    if (localph == 0)
      localph = 0xFFFFFFFFU; // Set to -1

    // Must be 0x00000000U for new objects
    filedata->item_id = 0x00000000U;

    ret = ptp_mtp_getobjectpropssupported(params, of, &propcnt, &properties);

    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], of, &opd);
      if (ret != PTP_RC_OK) {
	add_ptp_error_to_errorstack(device, ret, "send_file_object_info(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_ObjectFileName:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = filedata->item_id;
	  prop->property = PTP_OPC_ObjectFileName;
	  prop->datatype = PTP_DTC_STR;
	  if (filedata->filename != NULL) {
	    prop->propval.str = strdup(filedata->filename);
	    if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
	      strip_7bit_from_utf8(prop->propval.str);
	    }
	  }
	  break;
	case PTP_OPC_ProtectionStatus:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = filedata->item_id;
	  prop->property = PTP_OPC_ProtectionStatus;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = 0x0000U; /* Not protected */
	  break;
	case PTP_OPC_NonConsumable:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = filedata->item_id;
	  prop->property = PTP_OPC_NonConsumable;
	  prop->datatype = PTP_DTC_UINT8;
	  prop->propval.u8 = 0x00; /* It is supported, then it is consumable */
	  break;
	case PTP_OPC_Name:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = filedata->item_id;
	  prop->property = PTP_OPC_Name;
	  prop->datatype = PTP_DTC_STR;
	  if (filedata->filename != NULL)
	    prop->propval.str = strdup(filedata->filename);
	  break;
	case PTP_OPC_DateModified:
	  // Tag with current time if that is supported
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = filedata->item_id;
	    prop->property = PTP_OPC_DateModified;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = get_iso8601_stamp();
	    filedata->modificationdate = time(NULL);
	  }
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }
    free(properties);

    ret = ptp_mtp_sendobjectproplist(params, &store, &localph, &filedata->item_id,
				     of, filedata->filesize, props, nrofprops);

    /* Free property list */
    ptp_destroy_object_prop_list(props, nrofprops);

    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "send_file_object_info():"
				  "Could not send object property list.");
      if (ret == PTP_RC_AccessDenied) {
	add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
      }
      return -1;
    }
  } else if (ptp_operation_issupported(params,PTP_OC_SendObjectInfo)) {
    PTPObjectInfo new_file;

    memset(&new_file, 0, sizeof(PTPObjectInfo));

    new_file.Filename = filedata->filename;
    if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
      strip_7bit_from_utf8(new_file.Filename);
    }
    if (filedata->filesize > 0xFFFFFFFFL) {
      // This is a kludge in the MTP standard for large files.
      new_file.ObjectCompressedSize = (uint32_t) 0xFFFFFFFF;
    } else {
      new_file.ObjectCompressedSize = (uint32_t) filedata->filesize;
    }
    new_file.ObjectFormat = of;
    new_file.StorageID = store;
    new_file.ParentObject = localph;
    new_file.ModificationDate = time(NULL);

    // Create the object
    ret = ptp_sendobjectinfo(params, &store, &localph, &filedata->item_id, &new_file);

    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "send_file_object_info(): "
				  "Could not send object info.");
      if (ret == PTP_RC_AccessDenied) {
	add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
      }
      return -1;
    }
    // NOTE: the char* pointers inside new_file are not copies so don't
    // try to destroy this objectinfo!
  }

  // Now there IS an object with this parent handle.
  filedata->parent_id = localph;

  return 0;
}

/**
 * This function updates the MTP track object metadata on a
 * single file identified by an object ID.
 * @param device a pointer to the device to update the track
 *        metadata on.
 * @param metadata a track metadata set to be written to the file.
 *        notice that the <code>track_id</code> field of the
 *        metadata structure must be correct so that the
 *        function can update the right file. If some properties
 *        of this metadata are set to NULL (strings) or 0
 *        (numerical values) they will be discarded and the
 *        track will not be tagged with these blank values.
 * @return 0 on success, any other value means failure. If some
 *        or all of the properties fail to update we will still
 *        return success. On some devices (notably iRiver T30)
 *        properties that exist cannot be updated.
 */
int LIBMTP_Update_Track_Metadata(LIBMTP_mtpdevice_t *device,
				 LIBMTP_track_t const * const metadata)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint32_t i;
  uint16_t *properties = NULL;
  uint32_t propcnt = 0;

  // First see which properties can be set on this file format and apply accordingly
  // i.e only try to update this metadata for object tags that exist on the current player.
  ret = ptp_mtp_getobjectpropssupported(params, map_libmtp_type_to_ptp_type(metadata->filetype), &propcnt, &properties);
  if (ret != PTP_RC_OK) {
    // Just bail out for now, nothing is ever set.
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
			    "could not retrieve supported object properties.");
    return -1;
  }
  if (ptp_operation_issupported(params, PTP_OC_MTP_SetObjPropList) &&
      !FLAG_BROKEN_SET_OBJECT_PROPLIST(ptp_usb)) {
    MTPProperties *props = NULL;
    MTPProperties *prop = NULL;
    int nrofprops = 0;

    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], map_libmtp_type_to_ptp_type(metadata->filetype), &opd);
      if (ret != PTP_RC_OK) {
	add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_Name:
	  if (metadata->title == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Name;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->title);
	  break;
	case PTP_OPC_AlbumName:
	  if (metadata->album == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_AlbumName;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->album);
	  break;
	case PTP_OPC_Artist:
	  if (metadata->artist == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Artist;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->artist);
	  break;
	case PTP_OPC_Composer:
	  if (metadata->composer == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Composer;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->composer);
	  break;
	case PTP_OPC_Genre:
	  if (metadata->genre == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Genre;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->genre);
	  break;
	case PTP_OPC_Duration:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Duration;
	  prop->datatype = PTP_DTC_UINT32;
	  prop->propval.u32 = adjust_u32(metadata->duration, &opd);
	  break;
	case PTP_OPC_Track:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Track;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = adjust_u16(metadata->tracknumber, &opd);
	  break;
	case PTP_OPC_OriginalReleaseDate:
	  if (metadata->date == NULL)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_OriginalReleaseDate;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(metadata->date);
	  break;
	case PTP_OPC_SampleRate:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_SampleRate;
	  prop->datatype = PTP_DTC_UINT32;
	  prop->propval.u32 = adjust_u32(metadata->samplerate, &opd);
	  break;
	case PTP_OPC_NumberOfChannels:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_NumberOfChannels;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = adjust_u16(metadata->nochannels, &opd);
	  break;
	case PTP_OPC_AudioWAVECodec:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_AudioWAVECodec;
	  prop->datatype = PTP_DTC_UINT32;
	  prop->propval.u32 = adjust_u32(metadata->wavecodec, &opd);
	  break;
	case PTP_OPC_AudioBitRate:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_AudioBitRate;
	  prop->datatype = PTP_DTC_UINT32;
	  prop->propval.u32 = adjust_u32(metadata->bitrate, &opd);
	  break;
	case PTP_OPC_BitRateType:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_BitRateType;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = adjust_u16(metadata->bitratetype, &opd);
	  break;
	case PTP_OPC_Rating:
	  // TODO: shall this be set for rating 0?
	  if (metadata->rating == 0)
	    break;
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_Rating;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = adjust_u16(metadata->rating, &opd);
	  break;
	case PTP_OPC_UseCount:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = metadata->item_id;
	  prop->property = PTP_OPC_UseCount;
	  prop->datatype = PTP_DTC_UINT32;
	  prop->propval.u32 = adjust_u32(metadata->usecount, &opd);
	  break;
	case PTP_OPC_DateModified:
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    // Tag with current time if that is supported
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = metadata->item_id;
	    prop->property = PTP_OPC_DateModified;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = get_iso8601_stamp();
	  }
	  break;
	default:
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }

    // NOTE: File size is not updated, this should not change anyway.
    // neither will we change the filename.

    ret = ptp_mtp_setobjectproplist(params, props, nrofprops);

    ptp_destroy_object_prop_list(props, nrofprops);

    if (ret != PTP_RC_OK) {
      // TODO: return error of which property we couldn't set
      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
			      "could not set object property list.");
      free(properties);
      return -1;
    }

  } else if (ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], map_libmtp_type_to_ptp_type(metadata->filetype), &opd);
      if (ret != PTP_RC_OK) {
	add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_Name:
	  // Update title
	  ret = set_object_string(device, metadata->item_id, PTP_OPC_Name, metadata->title);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				    "could not set track title.");
	  }
	  break;
	case PTP_OPC_AlbumName:
	  // Update album
	  ret = set_object_string(device, metadata->item_id, PTP_OPC_AlbumName, metadata->album);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				    "could not set track album name.");
	  }
	  break;
	case PTP_OPC_Artist:
	  // Update artist
	  ret = set_object_string(device, metadata->item_id, PTP_OPC_Artist, metadata->artist);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				    "could not set track artist name.");
	  }
	  break;
	case PTP_OPC_Composer:
	  // Update composer
	  ret = set_object_string(device, metadata->item_id, PTP_OPC_Composer, metadata->composer);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				    "could not set track composer name.");
	  }
	  break;
	case PTP_OPC_Genre:
	  // Update genre (but only if valid)
	  if (metadata->genre) {
	    ret = set_object_string(device, metadata->item_id, PTP_OPC_Genre, metadata->genre);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				      "could not set genre.");
	    }
	  }
	  break;
	case PTP_OPC_Duration:
	  // Update duration
	  if (metadata->duration != 0) {
	    ret = set_object_u32(device, metadata->item_id, PTP_OPC_Duration, adjust_u32(metadata->duration, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set track duration.");
	    }
	  }
	  break;
	case PTP_OPC_Track:
	  // Update track number.
	  if (metadata->tracknumber != 0) {
	    ret = set_object_u16(device, metadata->item_id, PTP_OPC_Track, adjust_u16(metadata->tracknumber, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set track tracknumber.");
	    }
	  }
	  break;
	case PTP_OPC_OriginalReleaseDate:
	  // Update creation datetime
	  // The date can be zero, but some devices do not support setting zero
	  // dates (and it seems that a zero date should never be set anyway)
	  if (metadata->date) {
	    ret = set_object_string(device, metadata->item_id, PTP_OPC_OriginalReleaseDate, metadata->date);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set track release date.");
	    }
	  }
	  break;
	  // These are, well not so important.
	case PTP_OPC_SampleRate:
	  // Update sample rate
	  if (metadata->samplerate != 0) {
	    ret = set_object_u32(device, metadata->item_id, PTP_OPC_SampleRate, adjust_u32(metadata->samplerate, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set samplerate.");
	    }
	  }
	  break;
	case PTP_OPC_NumberOfChannels:
	  // Update number of channels
	  if (metadata->nochannels != 0) {
	    ret = set_object_u16(device, metadata->item_id, PTP_OPC_NumberOfChannels, adjust_u16(metadata->nochannels, &opd));
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				    "could not set number of channels.");
	  }
	}
	  break;
	case PTP_OPC_AudioWAVECodec:
	  // Update WAVE codec
	  if (metadata->wavecodec != 0) {
	    ret = set_object_u32(device, metadata->item_id, PTP_OPC_AudioWAVECodec, adjust_u32(metadata->wavecodec, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set WAVE codec.");
	    }
	  }
	  break;
	case PTP_OPC_AudioBitRate:
	  // Update bitrate
	  if (metadata->bitrate != 0) {
	    ret = set_object_u32(device, metadata->item_id, PTP_OPC_AudioBitRate, adjust_u32(metadata->bitrate, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set bitrate.");
	  }
	  }
	  break;
	case PTP_OPC_BitRateType:
	  // Update bitrate type
	  if (metadata->bitratetype != 0) {
	    ret = set_object_u16(device, metadata->item_id, PTP_OPC_BitRateType, adjust_u16(metadata->bitratetype, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set bitratetype.");
	    }
	  }
	  break;
	case PTP_OPC_Rating:
	  // Update user rating
	  // TODO: shall this be set for rating 0?
	  if (metadata->rating != 0) {
	    ret = set_object_u16(device, metadata->item_id, PTP_OPC_Rating, adjust_u16(metadata->rating, &opd));
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set user rating.");
	    }
	  }
	  break;
	case PTP_OPC_UseCount:
	  // Update use count, set even to zero if desired.
	  ret = set_object_u32(device, metadata->item_id, PTP_OPC_UseCount, adjust_u32(metadata->usecount, &opd));
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				  "could not set use count.");
	  }
	  break;
	case PTP_OPC_DateModified:
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    // Update modification time if supported
	    char *tmpstamp = get_iso8601_stamp();
	    ret = set_object_string(device, metadata->item_id, PTP_OPC_DateModified, tmpstamp);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
				      "could not set modification date.");
	    }
	    free(tmpstamp);
	  }
	  break;

	  // NOTE: File size is not updated, this should not change anyway.
	  // neither will we change the filename.
	default:
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }
  } else {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Update_Track_Metadata(): "
                            "Your device doesn't seem to support any known way of setting metadata.");
    free(properties);
    return -1;
  }

  // update cached object properties if metadata cache exists
  update_metadata_cache(device, metadata->item_id);

  free(properties);

  return 0;
}

/**
 * This function deletes a single file, track, playlist, folder or
 * any other object off the MTP device, identified by the object ID.
 *
 * If you delete a folder, there is no guarantee that the device will
 * really delete all the files that were in that folder, rather it is
 * expected that they will not be deleted, and will turn up in object
 * listings with parent set to a non-existant object ID. The safe way
 * to do this is to recursively delete all files (and folders) contained
 * in the folder, then the folder itself.
 *
 * @param device a pointer to the device to delete the object from.
 * @param object_id the object to delete.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Delete_Object(LIBMTP_mtpdevice_t *device,
			 uint32_t object_id)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;

  ret = ptp_deleteobject(params, object_id, 0);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Delete_Object(): could not delete object.");
    return -1;
  }

  return 0;
}

/**
 * The function moves an object from one location on a device to another
 * location.
 *
 * The semantics of moving a folder are not defined in the spec, but it
 * appears to do the right thing when tested (but devices that implement
 * this operation are rare).
 *
 * Note that moving an object may take a significant amount of time,
 * particularly if being moved between storages. MTP does not provide
 * any kind of progress mechanism, so the operation will simply block
 * for the duration.
 *
 * @param device a pointer to the device where the object exists.
 * @param object_id the object to move.
 * @param storage_id the id of the destination storage.
 * @param parent_id the id of the destination parent object (folder).
 *	  If the destination is the root of the storage, pass '0'.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Move_Object(LIBMTP_mtpdevice_t *device,
		       uint32_t object_id,
		       uint32_t storage_id,
		       uint32_t parent_id)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;

  ret = ptp_moveobject(params, object_id, storage_id, parent_id);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Move_Object(): could not move object.");
    return -1;
  }

  return 0;
}

/**
 * The function copies an object from one location on a device to another
 * location.
 *
 * The semantics of copying a folder are not defined in the spec, but it
 * appears to do the right thing when tested (but devices that implement
 * this operation are rare).
 *
 * Note that copying an object may take a significant amount of time.
 * MTP does not provide any kind of progress mechanism, so the operation
 * will simply block for the duration.
 *
 * @param device a pointer to the device where the object exists.
 * @param object_id the object to copy.
 * @param storage_id the id of the destination storage.
 * @param parent_id the id of the destination parent object (folder).
 *	  If the destination is the root of the storage, pass '0'.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Copy_Object(LIBMTP_mtpdevice_t *device,
		       uint32_t object_id,
		       uint32_t storage_id,
		       uint32_t parent_id)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;

  ret = ptp_copyobject(params, object_id, storage_id, parent_id);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Copy_Object(): could not copy object.");
    return -1;
  }

  return 0;
}

/**
 * Internal function to update an object filename property.
 */
static int set_object_filename(LIBMTP_mtpdevice_t *device,
			       uint32_t object_id, uint16_t ptp_type,
			       const char **newname_ptr)
{
  PTPParams             *params = (PTPParams *) device->params;
  PTP_USB               *ptp_usb = (PTP_USB*) device->usbinfo;
  PTPObjectPropDesc     opd;
  uint16_t              ret;
  char                  *newname;

  // See if we can modify the filename on this kind of files.
  ret = ptp_mtp_getobjectpropdesc(params, PTP_OPC_ObjectFileName, ptp_type, &opd);
  if (ret != PTP_RC_OK) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_filename(): "
			    "could not get property description.");
    return -1;
  }

  if (!opd.GetSet) {
    ptp_free_objectpropdesc(&opd);
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_filename(): "
            " property is not settable.");
    // TODO: we COULD actually upload/download the object here, if we feel
    //       like wasting time for the user.
    return -1;
  }

  newname = strdup(*newname_ptr);

  if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
    strip_7bit_from_utf8(newname);
  }

  if (ptp_operation_issupported(params, PTP_OC_MTP_SetObjPropList) &&
      !FLAG_BROKEN_SET_OBJECT_PROPLIST(ptp_usb)) {
    MTPProperties *props = NULL;
    MTPProperties *prop = NULL;
    int nrofprops = 0;

    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
    prop->ObjectHandle = object_id;
    prop->property = PTP_OPC_ObjectFileName;
    prop->datatype = PTP_DTC_STR;
    prop->propval.str = newname;

    ret = ptp_mtp_setobjectproplist(params, props, nrofprops);

    ptp_destroy_object_prop_list(props, nrofprops);

    if (ret != PTP_RC_OK) {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_filename(): "
              " could not set object property list.");
        ptp_free_objectpropdesc(&opd);
        return -1;
    }
  } else if (ptp_operation_issupported(params, PTP_OC_MTP_SetObjectPropValue)) {
    ret = set_object_string(device, object_id, PTP_OPC_ObjectFileName, newname);
    if (ret != 0) {
      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_filename(): "
              " could not set object filename.");
      ptp_free_objectpropdesc(&opd);
      return -1;
    }
  } else {
    free(newname);
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "set_object_filename(): "
              " your device doesn't seem to support any known way of setting metadata.");
    ptp_free_objectpropdesc(&opd);
    return -1;
  }

  ptp_free_objectpropdesc(&opd);

  // update cached object properties if metadata cache exists
  update_metadata_cache(device, object_id);

  return 0;
}

/**
 * This function renames a single file.
 * This simply means that the PTP_OPC_ObjectFileName property
 * is updated, if this is supported by the device.
 *
 * @param device a pointer to the device that contains the file.
 * @param file the file metadata of the file to rename.
 *        On success, the filename member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new filename for this object.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Set_File_Name(LIBMTP_mtpdevice_t *device,
                   LIBMTP_file_t *file, const char *newname)
{
  int         ret;

  ret = set_object_filename(device, file->item_id,
			    map_libmtp_type_to_ptp_type(file->filetype),
			    &newname);

  if (ret != 0) {
    return ret;
  }

  free(file->filename);
  file->filename = strdup(newname);
  return ret;
}

/**
 * This function renames a single folder.
 * This simply means that the PTP_OPC_ObjectFileName property
 * is updated, if this is supported by the device.
 *
 * @param device a pointer to the device that contains the file.
 * @param folder the folder metadata of the folder to rename.
 *        On success, the name member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new name for this object.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Set_Folder_Name(LIBMTP_mtpdevice_t *device,
                   LIBMTP_folder_t *folder, const char* newname)
{
  int ret;

  ret = set_object_filename(device, folder->folder_id,
			    PTP_OFC_Association,
			    &newname);

  if (ret != 0) {
    return ret;
    }

  free(folder->name);
  folder->name = strdup(newname);
  return ret;
}

/**
 * This function renames a single track.
 * This simply means that the PTP_OPC_ObjectFileName property
 * is updated, if this is supported by the device.
 *
 * @param device a pointer to the device that contains the file.
 * @param track the track metadata of the track to rename.
 *        On success, the filename member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new filename for this object.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Set_Track_Name(LIBMTP_mtpdevice_t *device,
                   LIBMTP_track_t *track, const char* newname)
{
  int         ret;

  ret = set_object_filename(device, track->item_id,
			    map_libmtp_type_to_ptp_type(track->filetype),
			    &newname);

  if (ret != 0) {
    return ret;
  }

  free(track->filename);
  track->filename = strdup(newname);
  return ret;
}

/**
 * This function renames a single playlist object file holder.
 * This simply means that the <code>PTP_OPC_ObjectFileName</code>
 * property is updated, if this is supported by the device.
 * The playlist filename should nominally end with an extension
 * like ".pla".
 *
 * NOTE: if you want to change the metadata the device display
 * about a playlist you must <i>not</i> use this function,
 * use <code>LIBMTP_Update_Playlist()</code> instead!
 *
 * @param device a pointer to the device that contains the file.
 * @param playlist the playlist metadata of the playlist to rename.
 *        On success, the name member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new name for this object.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Playlist()
 */
int LIBMTP_Set_Playlist_Name(LIBMTP_mtpdevice_t *device,
                   LIBMTP_playlist_t *playlist, const char* newname)
{
  int ret;

  ret = set_object_filename(device, playlist->playlist_id,
			    PTP_OFC_MTP_AbstractAudioVideoPlaylist,
			    &newname);

  if (ret != 0) {
    return ret;
  }

  free(playlist->name);
  playlist->name = strdup(newname);
  return ret;
}

/**
 * This function renames a single album.
 * This simply means that the <code>PTP_OPC_ObjectFileName</code>
 * property is updated, if this is supported by the device.
 * The album filename should nominally end with an extension
 * like ".alb".
 *
 * NOTE: if you want to change the metadata the device display
 * about a playlist you must <i>not</i> use this function,
 * use <code>LIBMTP_Update_Album()</code> instead!
 *
 * @param device a pointer to the device that contains the file.
 * @param album the album metadata of the album to rename.
 *        On success, the name member is updated. Be aware, that
 *        this name can be different than newname depending of device restrictions.
 * @param newname the new name for this object.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Album()
 */
int LIBMTP_Set_Album_Name(LIBMTP_mtpdevice_t *device,
                   LIBMTP_album_t *album, const char* newname)
{
  int ret;

  ret = set_object_filename(device, album->album_id,
			    PTP_OFC_MTP_AbstractAudioAlbum,
			    &newname);

  if (ret != 0) {
    return ret;
  }

  free(album->name);
  album->name = strdup(newname);
  return ret;
}

/**
 * THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 *
 * @see LIBMTP_Set_File_Name()
 * @see LIBMTP_Set_Track_Name()
 * @see LIBMTP_Set_Folder_Name()
 * @see LIBMTP_Set_Playlist_Name()
 * @see LIBMTP_Set_Album_Name()
 */
int LIBMTP_Set_Object_Filename(LIBMTP_mtpdevice_t *device,
                   uint32_t object_id, char* newname)
{
  int             ret;
  LIBMTP_file_t   *file;

  file = LIBMTP_Get_Filemetadata(device, object_id);

  if (file == NULL) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Set_Object_Filename(): "
			    "could not get file metadata for target object.");
    return -1;
  }

  ret = set_object_filename(device, object_id, map_libmtp_type_to_ptp_type(file->filetype), (const char **) &newname);

  free(file);

  return ret;
}

/**
 * Helper function. This indicates if a track exists on the device
 * @param device a pointer to the device to get the track from.
 * @param id the track ID of the track to retrieve.
 * @return TRUE (!=0) if the track exists, FALSE (0) if not
 */
int LIBMTP_Track_Exists(LIBMTP_mtpdevice_t *device,
           uint32_t const id)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;
  PTPObject *ob;

  ret = ptp_object_want (params, id, 0, &ob);
  if (ret == PTP_RC_OK)
      return -1;
  return 0;
}

/**
 * This creates a new folder structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_folder_track_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * @return a pointer to the newly allocated folder structure.
 * @see LIBMTP_destroy_folder_t()
 */
LIBMTP_folder_t *LIBMTP_new_folder_t(void)
{
  LIBMTP_folder_t *new = (LIBMTP_folder_t *) malloc(sizeof(LIBMTP_folder_t));
  if (new == NULL) {
    return NULL;
  }
  new->folder_id = 0;
  new->parent_id = 0;
  new->storage_id = 0;
  new->name = NULL;
  new->sibling = NULL;
  new->child = NULL;
  return new;
}

/**
 * This recursively deletes the memory for a folder structure.
 * This shall typically be called on a top-level folder list to
 * destroy the entire folder tree.
 *
 * @param folder folder structure to destroy
 * @see LIBMTP_new_folder_t()
 */
void LIBMTP_destroy_folder_t(LIBMTP_folder_t *folder)
{

  if(folder == NULL) {
     return;
  }

  //Destroy from the bottom up
  if(folder->child != NULL) {
     LIBMTP_destroy_folder_t(folder->child);
  }

  if(folder->sibling != NULL) {
    LIBMTP_destroy_folder_t(folder->sibling);
  }

  if(folder->name != NULL) {
    free(folder->name);
  }

  free(folder);
}

/**
 * Helper function. Returns a folder structure for a
 * specified id.
 *
 * @param folderlist list of folders to search
 * @param id of folder to look for
 * @return a folder or NULL if not found
 */
LIBMTP_folder_t *LIBMTP_Find_Folder(LIBMTP_folder_t *folderlist, uint32_t id)
{
  LIBMTP_folder_t *ret = NULL;

  if(folderlist == NULL) {
    return NULL;
  }

  if(folderlist->folder_id == id) {
    return folderlist;
  }

  if(folderlist->sibling) {
    ret = LIBMTP_Find_Folder(folderlist->sibling, id);
  }

  if(folderlist->child && ret == NULL) {
    ret = LIBMTP_Find_Folder(folderlist->child, id);
  }

  return ret;
}

/**
 * Function used to recursively get subfolders from params.
 */
static LIBMTP_folder_t *get_subfolders_for_folder(LIBMTP_folder_t *list, uint32_t parent)
{
  LIBMTP_folder_t *retfolders = NULL;
  LIBMTP_folder_t *children, *iter, *curr;

  iter = list->sibling;
  while(iter != list) {
    if (iter->parent_id != parent) {
      iter = iter->sibling;
      continue;
    }

    /* We know that iter is a child of 'parent', therefore we can safely
     * hold on to 'iter' locally since no one else will steal it
     * from the 'list' as we recurse. */
    children = get_subfolders_for_folder(list, iter->folder_id);

    curr = iter;
    iter = iter->sibling;

    // Remove curr from the list.
    curr->child->sibling = curr->sibling;
    curr->sibling->child = curr->child;

    // Attach the children to curr.
    curr->child = children;

    // Put this folder into the list of siblings.
    curr->sibling = retfolders;
    retfolders = curr;
  }

  return retfolders;
}

/**
 * This returns a list of all folders available
 * on the current MTP device.
 *
 * @param device a pointer to the device to get the folder listing for.
 * @param storage a storage ID to get the folder list from
 * @return a list of folders
 */
 LIBMTP_folder_t *LIBMTP_Get_Folder_List_For_Storage(LIBMTP_mtpdevice_t *device,
						    uint32_t const storage)
{
  PTPParams *params = (PTPParams *) device->params;
  LIBMTP_folder_t head, *rv;
  unsigned int i;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0) {
    flush_handles(device);
  }

  /*
   * This creates a temporary list of the folders, this is in a
   * reverse order and uses the Folder pointers that are already
   * in the Folder structure. From this we can then build up the
   * folder hierarchy with only looking at this temporary list,
   * and removing the folders from this temporary list as we go.
   * This significantly reduces the number of operations that we
   * have to do in building the folder hierarchy. Also since the
   * temp list is in reverse order, when we prepend to the sibling
   * list things are in the same order as they were originally
   * in the handle list.
   */
  head.sibling = &head;
  head.child = &head;
  for (i = 0; i < params->nrofobjects; i++) {
    LIBMTP_folder_t *folder;
    PTPObject *ob;

    ob = &params->objects[i];
    if (ob->oi.ObjectFormat != PTP_OFC_Association) {
      continue;
    }

    if (storage != PTP_GOH_ALL_STORAGE && storage != ob->oi.StorageID) {
      continue;
    }

    /*
     * Do we know how to handle these? They are part
     * of the MTP 1.0 specification paragraph 3.6.4.
     * For AssociationDesc 0x00000001U ptp_mtp_getobjectreferences()
     * should be called on these to get the contained objects, but
     * we basically don't care. Hopefully parent_id is maintained for all
     * children, because we rely on that instead.
     */
    if (ob->oi.AssociationDesc != 0x00000000U) {
      LIBMTP_INFO("MTP extended association type 0x%08x encountered\n", ob->oi.AssociationDesc);
    }

    // Create a folder struct...
    folder = LIBMTP_new_folder_t();
    if (folder == NULL) {
      // malloc failure or so.
      return NULL;
    }
    folder->folder_id = ob->oid;
    folder->parent_id = ob->oi.ParentObject;
    folder->storage_id = ob->oi.StorageID;
    folder->name = (ob->oi.Filename) ? (char *)strdup(ob->oi.Filename) : NULL;

    // pretend sibling says next, and child says prev.
    folder->sibling = head.sibling;
    folder->child = &head;
    head.sibling->child = folder;
    head.sibling = folder;
  }

  // We begin at the given root folder and get them all recursively
  rv = get_subfolders_for_folder(&head, 0x00000000U);

  // Some buggy devices may have some files in the "root folder"
  // 0xffffffff so if 0x00000000 didn't return any folders,
  // look for children of the root 0xffffffffU
  if (rv == NULL) {
    rv = get_subfolders_for_folder(&head, 0xffffffffU);
    if (rv != NULL)
      LIBMTP_ERROR("Device have files in \"root folder\" 0xffffffffU - "
		   "this is a firmware bug (but continuing)\n");
  }

  // The temp list should be empty. Clean up any orphans just in case.
  while(head.sibling != &head) {
    LIBMTP_folder_t *curr = head.sibling;

    LIBMTP_INFO("Orphan folder with ID: 0x%08x name: \"%s\" encountered.\n",
	   curr->folder_id,
	   curr->name);
    curr->sibling->child = curr->child;
    curr->child->sibling = curr->sibling;
    curr->child = NULL;
    curr->sibling = NULL;
    LIBMTP_destroy_folder_t(curr);
  }

  return rv;
}

/**
 * This returns a list of all folders available
 * on the current MTP device.
 *
 * @param device a pointer to the device to get the folder listing for.
 * @return a list of folders
 */
LIBMTP_folder_t *LIBMTP_Get_Folder_List(LIBMTP_mtpdevice_t *device)
{
  return LIBMTP_Get_Folder_List_For_Storage(device, PTP_GOH_ALL_STORAGE);
}

/**
 * This create a folder on the current MTP device. The PTP name
 * for a folder is "association". The PTP/MTP devices does not
 * have an internal "folder" concept really, it contains a flat
 * list of all files and some file are "associations" that other
 * files and folders may refer to as its "parent".
 *
 * @param device a pointer to the device to create the folder on.
 * @param name the name of the new folder. Note this can be modified
 *        if the device does not support all the characters in the
 *        name.
 * @param parent_id id of parent folder to add the new folder to,
 *        or 0xFFFFFFFF to put it in the root directory.
 * @param storage_id id of the storage to add this new folder to.
 *        notice that you cannot mismatch storage id and parent id:
 *        they must both be on the same storage! Pass in 0 if you
 *        want to create this folder on the default storage.
 * @return id to new folder or 0 if an error occurred
 */
uint32_t LIBMTP_Create_Folder(LIBMTP_mtpdevice_t *device, char *name,
			      uint32_t parent_id, uint32_t storage_id)
{
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint32_t parenthandle = 0;
  uint32_t store;
  PTPObjectInfo new_folder;
  uint16_t ret;
  uint32_t new_id = 0;

  if (storage_id == 0) {
    // I'm just guessing that a folder may require 512 bytes
    store = get_suggested_storage_id(device, 512, parent_id);
  } else {
    store = storage_id;
  }
  parenthandle = parent_id;

  memset(&new_folder, 0, sizeof(new_folder));
  new_folder.Filename = name;
  if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
    strip_7bit_from_utf8(new_folder.Filename);
  }
  new_folder.ObjectCompressedSize = 0;
  new_folder.ObjectFormat = PTP_OFC_Association;
  new_folder.ProtectionStatus = PTP_PS_NoProtection;
  new_folder.AssociationType = PTP_AT_GenericFolder;
  new_folder.ParentObject = parent_id;
  new_folder.StorageID = store;

  // Create the object
  if (!(params->device_flags & DEVICE_FLAG_BROKEN_SEND_OBJECT_PROPLIST) &&
	ptp_operation_issupported(params,PTP_OC_MTP_SendObjectPropList)) {
	MTPProperties *props = (MTPProperties*)calloc(2,sizeof(MTPProperties));

	props[0].property = PTP_OPC_ObjectFileName;
	props[0].datatype = PTP_DTC_STR;
	props[0].propval.str = name;

	props[1].property = PTP_OPC_Name;
	props[1].datatype = PTP_DTC_STR;
	props[1].propval.str = name;

	ret = ptp_mtp_sendobjectproplist(params, &store, &parenthandle, &new_id, PTP_OFC_Association,
			0, props, 1);
	free(props);
  } else {
	ret = ptp_sendobjectinfo(params, &store, &parenthandle, &new_id, &new_folder);
  }

  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Create_Folder: Could not send object info.");
    if (ret == PTP_RC_AccessDenied) {
      add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
    }
    return 0;
  }
  // NOTE: don't destroy the new_folder objectinfo, because it is statically referencing
  // several strings.

  add_object_to_cache(device, new_id);

  return new_id;
}

/**
 * This creates a new playlist metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_playlist_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * <pre>
 * LIBMTP_playlist_t *pl = LIBMTP_new_playlist_t();
 * pl->name = strdup(str);
 * ....
 * LIBMTP_destroy_playlist_t(pl);
 * </pre>
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_playlist_t()
 */
LIBMTP_playlist_t *LIBMTP_new_playlist_t(void)
{
  LIBMTP_playlist_t *new = (LIBMTP_playlist_t *) malloc(sizeof(LIBMTP_playlist_t));
  if (new == NULL) {
    return NULL;
  }
  new->playlist_id = 0;
  new->parent_id = 0;
  new->storage_id = 0;
  new->name = NULL;
  new->tracks = NULL;
  new->no_tracks = 0;
  new->next = NULL;
  return new;
}

/**
 * This destroys a playlist metadata structure and deallocates the memory
 * used by it, including any strings. Never use a track metadata
 * structure again after calling this function on it.
 * @param playlist the playlist metadata to destroy.
 * @see LIBMTP_new_playlist_t()
 */
void LIBMTP_destroy_playlist_t(LIBMTP_playlist_t *playlist)
{
  if (playlist == NULL) {
    return;
  }
  if (playlist->name != NULL)
    free(playlist->name);
  if (playlist->tracks != NULL)
    free(playlist->tracks);
  free(playlist);
  return;
}

/**
 * This function returns a list of the playlists available on the
 * device. Typical usage:
 *
 * <pre>
 * </pre>
 *
 * @param device a pointer to the device to get the playlist listing from.
 * @return a playlist list on success, else NULL. If there are no playlists
 *         on the device, NULL will be returned as well.
 * @see LIBMTP_Get_Playlist()
 */
LIBMTP_playlist_t *LIBMTP_Get_Playlist_List(LIBMTP_mtpdevice_t *device)
{
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  const int REQ_SPL = FLAG_PLAYLIST_SPL(ptp_usb);
  PTPParams *params = (PTPParams *) device->params;
  LIBMTP_playlist_t *retlists = NULL;
  LIBMTP_playlist_t *curlist = NULL;
  uint32_t i;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0) {
    flush_handles(device);
  }

  for (i = 0; i < params->nrofobjects; i++) {
    LIBMTP_playlist_t *pl;
    PTPObject *ob;
    uint16_t ret;

    ob = &params->objects[i];

    // Ignore stuff that isn't playlists

    // For Samsung players we must look for the .spl extension explicitly since
    // playlists are not stored as playlist objects.
    if ( REQ_SPL && is_spl_playlist(&ob->oi) ) {
      // Allocate a new playlist type
      pl = LIBMTP_new_playlist_t();
      spl_to_playlist_t(device, &ob->oi, ob->oid, pl);
    }
    else if ( ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioVideoPlaylist ) {
      continue;
    }
    else {
      // Allocate a new playlist type
      pl = LIBMTP_new_playlist_t();

      // Try to look up proper name, else use the oi->Filename field.
      pl->name = get_string_from_object(device, ob->oid, PTP_OPC_Name);
      if (pl->name == NULL) {
	pl->name = strdup(ob->oi.Filename);
      }
      pl->playlist_id = ob->oid;
      pl->parent_id = ob->oi.ParentObject;
      pl->storage_id = ob->oi.StorageID;

      // Then get the track listing for this playlist
      ret = ptp_mtp_getobjectreferences(params, pl->playlist_id, &pl->tracks, &pl->no_tracks);
      if (ret != PTP_RC_OK) {
        add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Playlist_List(): "
				    "could not get object references.");
        pl->tracks = NULL;
        pl->no_tracks = 0;
      }
    }

    // Add playlist to a list that will be returned afterwards.
    if (retlists == NULL) {
      retlists = pl;
      curlist = pl;
    } else {
      curlist->next = pl;
      curlist = pl;
    }

    // Call callback here if we decide to add that possibility...
  }
  return retlists;
}


/**
 * This function retrieves an individual playlist from the device.
 * @param device a pointer to the device to get the playlist from.
 * @param plid the unique ID of the playlist to retrieve.
 * @return a valid playlist metadata post or NULL on failure.
 * @see LIBMTP_Get_Playlist_List()
 */
LIBMTP_playlist_t *LIBMTP_Get_Playlist(LIBMTP_mtpdevice_t *device, uint32_t const plid)
{
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  const int REQ_SPL = FLAG_PLAYLIST_SPL(ptp_usb);
  PTPParams *params = (PTPParams *) device->params;
  PTPObject *ob;
  LIBMTP_playlist_t *pl;
  uint16_t ret;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0) {
    flush_handles(device);
  }

  ret = ptp_object_want (params, plid, PTPOBJECT_OBJECTINFO_LOADED, &ob);
  if (ret != PTP_RC_OK)
    return NULL;

  // For Samsung players we must look for the .spl extension explicitly since
  // playlists are not stored as playlist objects.
  if ( REQ_SPL && is_spl_playlist(&ob->oi) ) {
    // Allocate a new playlist type
    pl = LIBMTP_new_playlist_t();
    spl_to_playlist_t(device, &ob->oi, ob->oid, pl);
    return pl;
  }

  // Ignore stuff that isn't playlists
  else if ( ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioVideoPlaylist ) {
    return NULL;
  }

  // Allocate a new playlist type
  pl = LIBMTP_new_playlist_t();

  pl->name = get_string_from_object(device, ob->oid, PTP_OPC_Name);
  if (pl->name == NULL) {
    pl->name = strdup(ob->oi.Filename);
  }
  pl->playlist_id = ob->oid;
  pl->parent_id = ob->oi.ParentObject;
  pl->storage_id = ob->oi.StorageID;

  // Then get the track listing for this playlist
  ret = ptp_mtp_getobjectreferences(params, pl->playlist_id, &pl->tracks, &pl->no_tracks);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Playlist(): Could not get object references.");
    pl->tracks = NULL;
    pl->no_tracks = 0;
  }

  return pl;
}

/**
 * This function creates a new abstract list such as a playlist
 * or an album.
 *
 * @param device a pointer to the device to create the new abstract list
 *        on.
 * @param name the name of the new abstract list.
 * @param artist the artist of the new abstract list or NULL.
 * @param genre the genre of the new abstract list or NULL.
 * @param parenthandle the handle of the parent or 0 for no parent
 *        i.e. the root folder.
 * @param objectformat the abstract list type to create.
 * @param suffix the ".foo" (4 characters) suffix to use for the virtual
 *        "file" created by this operation.
 * @param newid a pointer to a variable that will hold the new object
 *        ID if this call is successful.
 * @param tracks an array of tracks to associate with this list.
 * @param no_tracks the number of tracks in the list.
 * @return 0 on success, any other value means failure.
 */
static int create_new_abstract_list(LIBMTP_mtpdevice_t *device,
				    char const * const name,
				    char const * const artist,
				    char const * const composer,
				    char const * const genre,
				    uint32_t const parenthandle,
				    uint32_t const storageid,
				    uint16_t const objectformat,
				    char const * const suffix,
				    uint32_t * const newid,
				    uint32_t const * const tracks,
				    uint32_t const no_tracks)

{
  unsigned int i;
  int supported = 0;
  uint16_t ret;
  uint16_t *properties = NULL;
  uint32_t propcnt = 0;
  uint32_t store;
  uint32_t localph = parenthandle;
  uint8_t nonconsumable = 0x00U; /* By default it is consumable */
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  char fname[256];
  //uint8_t data[2];

  // NULL check
  if (!name) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): list name was NULL, using default name \"Unknown\"");
    return -1;
  }

  if (storageid == 0) {
    // I'm just guessing that an abstract list may require 512 bytes
    store = get_suggested_storage_id(device, 512, localph);
  } else {
    store = storageid;
  }

  // Check if we can create an object of this type
  for ( i=0; i < params->deviceinfo.ImageFormats_len; i++ ) {
    if (params->deviceinfo.ImageFormats[i] == objectformat) {
      supported = 1;
      break;
    }
  }
  if (!supported) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): player does not support this abstract type");
    LIBMTP_ERROR("Unsupported abstract list type: %04x\n", objectformat);
    return -1;
  }

  // add the new suffix if it isn't there
  fname[0] = '\0';
  if (strlen(name) > strlen(suffix)) {
    char const * const suff = &name[strlen(name)-strlen(suffix)];
    if (!strcmp(suff, suffix)) {
      // Home free.
      strncpy(fname, name, sizeof(fname));
    }
  }
  // If it didn't end with "<suffix>" then add that here.
  if (fname[0] == '\0') {
    strncpy(fname, name, sizeof(fname)-strlen(suffix)-1);
    strcat(fname, suffix);
    fname[sizeof(fname)-1] = '\0';
  }

  if (ptp_operation_issupported(params, PTP_OC_MTP_SendObjectPropList) &&
      !FLAG_BROKEN_SEND_OBJECT_PROPLIST(ptp_usb)) {
    MTPProperties *props = NULL;
    MTPProperties *prop = NULL;
    int nrofprops = 0;

    *newid = 0x00000000U;

    ret = ptp_mtp_getobjectpropssupported(params, objectformat, &propcnt, &properties);

    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], objectformat, &opd);
      if (ret != PTP_RC_OK) {
	add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_ObjectFileName:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = *newid;
	  prop->property = PTP_OPC_ObjectFileName;
	  prop->datatype = PTP_DTC_STR;
	  prop->propval.str = strdup(fname);
	  if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
	    strip_7bit_from_utf8(prop->propval.str);
	  }
	  break;
	case PTP_OPC_ProtectionStatus:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = *newid;
	  prop->property = PTP_OPC_ProtectionStatus;
	  prop->datatype = PTP_DTC_UINT16;
	  prop->propval.u16 = 0x0000U; /* Not protected */
	  break;
	case PTP_OPC_NonConsumable:
	  prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	  prop->ObjectHandle = *newid;
	  prop->property = PTP_OPC_NonConsumable;
	  prop->datatype = PTP_DTC_UINT8;
	  prop->propval.u8 = nonconsumable;
	  break;
	case PTP_OPC_Name:
	  if (name != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_Name;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(name);
	  }
	  break;
	case PTP_OPC_AlbumArtist:
	  if (artist != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_AlbumArtist;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(artist);
	  }
	  break;
	case PTP_OPC_Artist:
	  if (artist != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_Artist;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(artist);
	  }
	  break;
	case PTP_OPC_Composer:
	  if (composer != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_Composer;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(composer);
	  }
	  break;
	case PTP_OPC_Genre:
	  if (genre != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_Genre;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(genre);
	  }
	  break;
 	case PTP_OPC_DateModified:
	  // Tag with current time if that is supported
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    prop = ptp_get_new_object_prop_entry(&props,&nrofprops);
	    prop->ObjectHandle = *newid;
	    prop->property = PTP_OPC_DateModified;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = get_iso8601_stamp();
	  }
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }
    free(properties);

    ret = ptp_mtp_sendobjectproplist(params, &store, &localph, newid,
				     objectformat, 0, props, nrofprops);

    /* Free property list */
    ptp_destroy_object_prop_list(props, nrofprops);

    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "create_new_abstract_list(): Could not send object property list.");
      if (ret == PTP_RC_AccessDenied) {
	add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
      }
      return -1;
    }

    // now send the blank object
    ret = ptp_sendobject(params, NULL, 0);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "create_new_abstract_list(): Could not send blank object data.");
      return -1;
    }

  } else if (ptp_operation_issupported(params,PTP_OC_SendObjectInfo)) {
    PTPObjectInfo new_object;

    new_object.Filename = fname;
    if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb)) {
      strip_7bit_from_utf8(new_object.Filename);
    }
    // At one point this had to be one
    new_object.ObjectCompressedSize = 0;
    new_object.ObjectFormat = objectformat;

    // Create the object
    ret = ptp_sendobjectinfo(params, &store, &localph, newid, &new_object);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "create_new_abstract_list(): Could not send object info (the playlist itself).");
      if (ret == PTP_RC_AccessDenied) {
	add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
      }
      return -1;
    }
    // NOTE: don't destroy new_object objectinfo afterwards - the strings it contains are
    // not copies.

#if 0
    /*
     * At one time we had to send this one blank data byte.
     * If we didn't, the handle will not be created and thus there is
     * no playlist. Possibly this was masking some bug, so removing it
     * now.
     */
    data[0] = '\0';
    data[1] = '\0';
    ret = ptp_sendobject(params, data, 1);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "create_new_abstract_list(): Could not send blank object data.");
      return -1;
    }
#endif

    // set the properties one by one
    ret = ptp_mtp_getobjectpropssupported(params, objectformat, &propcnt, &properties);

    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], objectformat, &opd);
      if (ret != PTP_RC_OK) {
	add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_Name:
	  if (name != NULL) {
	    ret = set_object_string(device, *newid, PTP_OPC_Name, name);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set entity name.");
	      return -1;
	    }
	  }
	  break;
	case PTP_OPC_AlbumArtist:
	  if (artist != NULL) {
	    ret = set_object_string(device, *newid, PTP_OPC_AlbumArtist, artist);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set entity album artist.");
	      return -1;
	    }
	  }
	  break;
	case PTP_OPC_Artist:
	  if (artist != NULL) {
	    ret = set_object_string(device, *newid, PTP_OPC_Artist, artist);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set entity artist.");
	      return -1;
	    }
	  }
	  break;
	case PTP_OPC_Composer:
	  if (composer != NULL) {
	    ret = set_object_string(device, *newid, PTP_OPC_Composer, composer);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set entity composer.");
	      return -1;
	    }
	  }
	  break;
	case PTP_OPC_Genre:
	  if (genre != NULL) {
	    ret = set_object_string(device, *newid, PTP_OPC_Genre, genre);
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set entity genre.");
	      return -1;
	    }
	  }
	  break;
 	case PTP_OPC_DateModified:
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    ret = set_object_string(device, *newid, PTP_OPC_DateModified, get_iso8601_stamp());
	    if (ret != 0) {
	      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "create_new_abstract_list(): could not set date modified.");
	      return -1;
	    }
	  }
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }
    free(properties);
  }

  if (no_tracks > 0) {
    // Add tracks to the list as object references.
    ret = ptp_mtp_setobjectreferences (params, *newid, (uint32_t *) tracks, no_tracks);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "create_new_abstract_list(): could not add tracks as object references.");
      return -1;
    }
  }

  add_object_to_cache(device, *newid);

  return 0;
}

/**
 * This updates the metadata and track listing
 * for an abstract list.
 * @param device a pointer to the device that the abstract list
 *        resides on.
 * @param name the name of the abstract list.
 * @param artist the artist of the abstract list or NULL.
 * @param genre the genre of the abstract list or NULL.
 * @param objecthandle the object to be updated.
 * @param objectformat the abstract list type to update.
 * @param tracks an array of tracks to associate with this list.
 * @param no_tracks the number of tracks in the list.
 * @return 0 on success, any other value means failure.
 */
static int update_abstract_list(LIBMTP_mtpdevice_t *device,
				char const * const name,
				char const * const artist,
				char const * const composer,
				char const * const genre,
				uint32_t const objecthandle,
				uint16_t const objectformat,
				uint32_t const * const tracks,
				uint32_t const no_tracks)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint16_t *properties = NULL;
  uint32_t propcnt = 0;
  unsigned int i;

  // First see which properties can be set
  // i.e only try to update this metadata for object tags that exist on the current player.
  ret = ptp_mtp_getobjectpropssupported(params, objectformat, &propcnt, &properties);
  if (ret != PTP_RC_OK) {
    // Just bail out for now, nothing is ever set.
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
			    "could not retrieve supported object properties.");
    return -1;
  }
  if (ptp_operation_issupported(params,PTP_OC_MTP_SetObjPropList) &&
      !FLAG_BROKEN_SET_OBJECT_PROPLIST(ptp_usb)) {
    MTPProperties *props = NULL;
    MTPProperties *prop = NULL;
    int nrofprops = 0;

    for (i=0;i<propcnt;i++) {
      PTPObjectPropDesc opd;

      ret = ptp_mtp_getobjectpropdesc(params, properties[i], objectformat, &opd);
      if (ret != PTP_RC_OK) {
	add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				"could not get property description.");
      } else if (opd.GetSet) {
	switch (properties[i]) {
	case PTP_OPC_Name:
	  prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	  prop->ObjectHandle = objecthandle;
	  prop->property = PTP_OPC_Name;
	  prop->datatype = PTP_DTC_STR;
	  if (name != NULL)
	    prop->propval.str = strdup(name);
	  break;
	case PTP_OPC_AlbumArtist:
	  if (artist != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = objecthandle;
	    prop->property = PTP_OPC_AlbumArtist;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(artist);
	  }
	  break;
	case PTP_OPC_Artist:
	  if (artist != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = objecthandle;
	    prop->property = PTP_OPC_Artist;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(artist);
	  }
	  break;
	case PTP_OPC_Composer:
	  if (composer != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = objecthandle;
	    prop->property = PTP_OPC_Composer;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(composer);
	  }
	  break;
	case PTP_OPC_Genre:
	  if (genre != NULL) {
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = objecthandle;
	    prop->property = PTP_OPC_Genre;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = strdup(genre);
	  }
	  break;
 	case PTP_OPC_DateModified:
	  if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	    // Tag with current time if that is supported
	    prop = ptp_get_new_object_prop_entry(&props, &nrofprops);
	    prop->ObjectHandle = objecthandle;
	    prop->property = PTP_OPC_DateModified;
	    prop->datatype = PTP_DTC_STR;
	    prop->propval.str = get_iso8601_stamp();
	  }
	  break;
	default:
	  break;
	}
      }
      ptp_free_objectpropdesc(&opd);
    }

    // proplist could be NULL if we can't write any properties
    if (props != NULL) {
      ret = ptp_mtp_setobjectproplist(params, props, nrofprops);

      ptp_destroy_object_prop_list(props, nrofprops);

      if (ret != PTP_RC_OK) {
        // TODO: return error of which property we couldn't set
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
                                "could not set object property list.");
        free(properties);
        return -1;
      }
    }

  } else if (ptp_operation_issupported(params,PTP_OC_MTP_SetObjectPropValue)) {
    for (i=0;i<propcnt;i++) {
      switch (properties[i]) {
      case PTP_OPC_Name:
	// Update title
	ret = set_object_string(device, objecthandle, PTP_OPC_Name, name);
	if (ret != 0) {
	  add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				  "could not set title.");
	}
	break;
      case PTP_OPC_AlbumArtist:
	// Update album artist
	ret = set_object_string(device, objecthandle, PTP_OPC_AlbumArtist, artist);
	if (ret != 0) {
	  add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				  "could not set album artist name.");
	}
	break;
      case PTP_OPC_Artist:
	// Update artist
	ret = set_object_string(device, objecthandle, PTP_OPC_Artist, artist);
	if (ret != 0) {
	  add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				  "could not set artist name.");
	}
	break;
      case PTP_OPC_Composer:
	// Update composer
	ret = set_object_string(device, objecthandle, PTP_OPC_Composer, composer);
	if (ret != 0) {
	  add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				  "could not set composer name.");
	}
	break;
      case PTP_OPC_Genre:
	// Update genre (but only if valid)
	if(genre) {
	  ret = set_object_string(device, objecthandle, PTP_OPC_Genre, genre);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				    "could not set genre.");
	  }
        }
	break;
      case PTP_OPC_DateModified:
	// Update date modified
	if (!FLAG_CANNOT_HANDLE_DATEMODIFIED(ptp_usb)) {
	  char *tmpdate = get_iso8601_stamp();
	  ret = set_object_string(device, objecthandle, PTP_OPC_DateModified, tmpdate);
	  if (ret != 0) {
	    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
				    "could not set modification date.");
	  }
	  free(tmpdate);
	}
	break;
      default:
	break;
      }
    }
  } else {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "update_abstract_list(): "
                            "Your device doesn't seem to support any known way of setting metadata.");
    free(properties);
    return -1;
  }

  // Then the object references...
  ret = ptp_mtp_setobjectreferences (params, objecthandle, (uint32_t *) tracks, no_tracks);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "update_abstract_list(): could not add tracks as object references.");
    free(properties);
    return -1;
  }

  free(properties);

  update_metadata_cache(device, objecthandle);

  return 0;
}


/**
 * This routine creates a new playlist based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * playlist.
 * @param device a pointer to the device to create the new playlist on.
 * @param metadata the metadata for the new playlist. If the function
 *        exits with success, the <code>playlist_id</code> field of this
 *        struct will contain the new playlist ID of the playlist.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent 
 *        (e.g. folder) to store this track in. Since some 
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Playlist()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Create_New_Playlist(LIBMTP_mtpdevice_t *device,
			       LIBMTP_playlist_t * const metadata)
{
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  uint32_t localph = metadata->parent_id;

  // Use a default folder if none given
  if (localph == 0) {
    if (device->default_playlist_folder != 0)
      localph = device->default_playlist_folder;
    else
      localph = device->default_music_folder;
  }
  metadata->parent_id = localph;

  // Samsung needs its own special type of playlists
  if(FLAG_PLAYLIST_SPL(ptp_usb)) {
    return playlist_t_to_spl(device, metadata);
  }

  // Just create a new abstract audio/video playlist...
  return create_new_abstract_list(device,
				  metadata->name,
				  NULL,
				  NULL,
				  NULL,
				  localph,
				  metadata->storage_id,
				  PTP_OFC_MTP_AbstractAudioVideoPlaylist,
				  get_playlist_extension(ptp_usb),
				  &metadata->playlist_id,
				  metadata->tracks,
				  metadata->no_tracks);
}

/**
 * This routine updates a playlist based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * playlist in place of those already present, i.e. the
 * previous track listing will be deleted. For Samsung devices the
 * playlist id (metadata->playlist_id) is likely to change.
 * @param device a pointer to the device to create the new playlist on.
 * @param metadata the metadata for the playlist to be updated.
 *                 notice that the field <code>playlist_id</code>
 *                 must contain the appropriate playlist ID. Playlist ID
 *                 be modified to a new playlist ID by the time the
 *                 function returns since edit-in-place is not always possible.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Create_New_Playlist()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Update_Playlist(LIBMTP_mtpdevice_t *device,
			   LIBMTP_playlist_t * const metadata)
{

  // Samsung needs its own special type of playlists
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  if(FLAG_PLAYLIST_SPL(ptp_usb)) {
    return update_spl_playlist(device, metadata);
  }

  return update_abstract_list(device,
			      metadata->name,
			      NULL,
			      NULL,
			      NULL,
			      metadata->playlist_id,
			      PTP_OFC_MTP_AbstractAudioVideoPlaylist,
			      metadata->tracks,
			      metadata->no_tracks);
}

/**
 * This creates a new album metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_album_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings.
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_album_t()
 */
LIBMTP_album_t *LIBMTP_new_album_t(void)
{
  LIBMTP_album_t *new = (LIBMTP_album_t *) malloc(sizeof(LIBMTP_album_t));
  if (new == NULL) {
    return NULL;
  }
  new->album_id = 0;
  new->parent_id = 0;
  new->storage_id = 0;
  new->name = NULL;
  new->artist = NULL;
  new->composer = NULL;
  new->genre = NULL;
  new->tracks = NULL;
  new->no_tracks = 0;
  new->next = NULL;
  return new;
}

/**
 * This recursively deletes the memory for an album structure
 *
 * @param album structure to destroy
 * @see LIBMTP_new_album_t()
 */
void LIBMTP_destroy_album_t(LIBMTP_album_t *album)
{
  if (album == NULL) {
    return;
  }
  if (album->name != NULL)
    free(album->name);
  if (album->artist != NULL)
    free(album->artist);
  if (album->composer != NULL)
    free(album->composer);
  if (album->genre != NULL)
    free(album->genre);
  if (album->tracks != NULL)
    free(album->tracks);
  free(album);
  return;
}

/**
 * This function maps and copies a property onto the album metadata if applicable.
 */
static void pick_property_to_album_metadata(LIBMTP_mtpdevice_t *device,
					    MTPProperties *prop, LIBMTP_album_t *alb)
{
  switch (prop->property) {
  case PTP_OPC_Name:
    if (prop->propval.str != NULL)
      alb->name = strdup(prop->propval.str);
    else
      alb->name = NULL;
    break;
  case PTP_OPC_AlbumArtist:
    if (prop->propval.str != NULL) {
      // This should take precedence over plain "Artist"
      if (alb->artist != NULL)
	free(alb->artist);
      alb->artist = strdup(prop->propval.str);
    } else
      alb->artist = NULL;
    break;
  case PTP_OPC_Artist:
    if (prop->propval.str != NULL) {
      // Only use of AlbumArtist is not set
      if (alb->artist == NULL)
	alb->artist = strdup(prop->propval.str);
    } else
      alb->artist = NULL;
    break;
  case PTP_OPC_Composer:
    if (prop->propval.str != NULL)
      alb->composer = strdup(prop->propval.str);
    else
      alb->composer = NULL;
    break;
  case PTP_OPC_Genre:
    if (prop->propval.str != NULL)
      alb->genre = strdup(prop->propval.str);
    else
      alb->genre = NULL;
    break;
  }
}

/**
 * This function retrieves the album metadata for an album
 * given by a unique ID.
 * @param device a pointer to the device to get the track metadata off.
 * @param alb an album metadata metadata set to fill in.
 */
static void get_album_metadata(LIBMTP_mtpdevice_t *device,
			       LIBMTP_album_t *alb)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  uint32_t i;
  MTPProperties *prop;
  PTPObject *ob;

  /*
   * If we have a cached, large set of metadata, then use it!
   */
  ret = ptp_object_want(params, alb->album_id, PTPOBJECT_MTPPROPLIST_LOADED, &ob);
  if (ob->mtpprops) {
    prop = ob->mtpprops;
    for (i=0;i<ob->nrofmtpprops;i++,prop++)
      pick_property_to_album_metadata(device, prop, alb);
  } else {
    uint16_t *props = NULL;
    uint32_t propcnt = 0;

    // First see which properties can be retrieved for albums
    ret = ptp_mtp_getobjectpropssupported(params, PTP_OFC_MTP_AbstractAudioAlbum, &propcnt, &props);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "get_album_metadata(): call to ptp_mtp_getobjectpropssupported() failed.");
      // Just bail out for now, nothing is ever set.
      return;
    } else {
      for (i=0;i<propcnt;i++) {
	switch (props[i]) {
	case PTP_OPC_Name:
	  alb->name = get_string_from_object(device, ob->oid, PTP_OPC_Name);
	  break;
	case PTP_OPC_AlbumArtist:
	  if (alb->artist != NULL)
	    free(alb->artist);
	  alb->artist = get_string_from_object(device, ob->oid, PTP_OPC_AlbumArtist);
	  break;
	case PTP_OPC_Artist:
	  if (alb->artist == NULL)
	    alb->artist = get_string_from_object(device, ob->oid, PTP_OPC_Artist);
	  break;
	case PTP_OPC_Composer:
	  alb->composer = get_string_from_object(device, ob->oid, PTP_OPC_Composer);
	  break;
	case PTP_OPC_Genre:
	  alb->genre = get_string_from_object(device, ob->oid, PTP_OPC_Genre);
	  break;
	default:
	  break;
	}
      }
      free(props);
    }
  }
}


/**
 * This function returns a list of the albums available on the
 * device.
 *
 * @param device a pointer to the device to get the album listing from.
 * @return an album list on success, else NULL. If there are no albums
 *         on the device, NULL will be returned as well.
 * @see LIBMTP_Get_Album()
 */
LIBMTP_album_t *LIBMTP_Get_Album_List(LIBMTP_mtpdevice_t *device)
{
	// Read all storage devices
	return LIBMTP_Get_Album_List_For_Storage(device, 0);
}


/**
 * This function returns a list of the albums available on the
 * device. You can filter on the storage ID.
 *
 * @param device a pointer to the device to get the album listing from.
 * @param storage_id ID of device storage (if null, all storages)
 *
 * @return an album list on success, else NULL. If there are no albums
 *         on the device, NULL will be returned as well.
 * @see LIBMTP_Get_Album()
 */
LIBMTP_album_t *LIBMTP_Get_Album_List_For_Storage(LIBMTP_mtpdevice_t *device, uint32_t const storage_id)
{
  PTPParams *params = (PTPParams *) device->params;
  LIBMTP_album_t *retalbums = NULL;
  LIBMTP_album_t *curalbum = NULL;
  uint32_t i;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0)
    flush_handles(device);

  for (i = 0; i < params->nrofobjects; i++) {
    LIBMTP_album_t *alb;
    PTPObject *ob;
    uint16_t ret;

    ob = &params->objects[i];

    // Ignore stuff that isn't an album
    if ( ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioAlbum )
      continue;

	// Ignore stuff that isn't into the storage device
	if ((storage_id != 0) && (ob->oi.StorageID != storage_id ))
		continue;

    // Allocate a new album type
    alb = LIBMTP_new_album_t();
    alb->album_id = ob->oid;
    alb->parent_id = ob->oi.ParentObject;
    alb->storage_id = ob->oi.StorageID;

    // Fetch supported metadata
    get_album_metadata(device, alb);

    // Then get the track listing for this album
    ret = ptp_mtp_getobjectreferences(params, alb->album_id, &alb->tracks, &alb->no_tracks);
    if (ret != PTP_RC_OK) {
      add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Album_List(): Could not get object references.");
      alb->tracks = NULL;
      alb->no_tracks = 0;
    }

    // Add album to a list that will be returned afterwards.
    if (retalbums == NULL) {
      retalbums = alb;
      curalbum = alb;
    } else {
      curalbum->next = alb;
      curalbum = alb;
    }

  }
  return retalbums;
}

/**
 * This function retrieves an individual album from the device.
 * @param device a pointer to the device to get the album from.
 * @param albid the unique ID of the album to retrieve.
 * @return a valid album metadata or NULL on failure.
 * @see LIBMTP_Get_Album_List()
 */
LIBMTP_album_t *LIBMTP_Get_Album(LIBMTP_mtpdevice_t *device, uint32_t const albid)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;
  PTPObject *ob;
  LIBMTP_album_t *alb;

  // Get all the handles if we haven't already done that
  if (params->nrofobjects == 0)
    flush_handles(device);

  ret = ptp_object_want(params, albid, PTPOBJECT_OBJECTINFO_LOADED, &ob);
  if (ret != PTP_RC_OK)
    return NULL;

  // Ignore stuff that isn't an album
  if (ob->oi.ObjectFormat != PTP_OFC_MTP_AbstractAudioAlbum)
    return NULL;

  // Allocate a new album type
  alb = LIBMTP_new_album_t();
  alb->album_id = ob->oid;
  alb->parent_id = ob->oi.ParentObject;
  alb->storage_id = ob->oi.StorageID;

  // Fetch supported metadata
  get_album_metadata(device, alb);

  // Then get the track listing for this album
  ret = ptp_mtp_getobjectreferences(params, alb->album_id, &alb->tracks, &alb->no_tracks);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Album: Could not get object references.");
    alb->tracks = NULL;
    alb->no_tracks = 0;
  }

  return alb;
}

/**
 * This routine creates a new album based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * album.
 * @param device a pointer to the device to create the new album on.
 * @param metadata the metadata for the new album. If the function
 *        exits with success, the <code>album_id</code> field of this
 *        struct will contain the new ID of the album.
 *        <ul>
 *        <li><code>metadata-&gt;parent_id</code> should be set to the parent
 *        (e.g. folder) to store this track in. Since some
 *        devices are a bit picky about where files
 *        are placed, a default folder will be chosen if libmtp
 *        has detected one for the current filetype and this
 *        parameter is set to 0. If this is 0 and no default folder
 *        can be found, the file will be stored in the root folder.
 *        <li><code>metadata-&gt;storage_id</code> should be set to the
 *        desired storage (e.g. memory card or whatever your device
 *        presents) to store this track in. Setting this to 0 will store
 *        the track on the primary storage.
 *        </ul>
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Update_Album()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Create_New_Album(LIBMTP_mtpdevice_t *device,
			    LIBMTP_album_t * const metadata)
{
  uint32_t localph = metadata->parent_id;

  // Use a default folder if none given
  if (localph == 0) {
    if (device->default_album_folder != 0)
      localph = device->default_album_folder;
    else
      localph = device->default_music_folder;
  }
  metadata->parent_id = localph;

  // Just create a new abstract album...
  return create_new_abstract_list(device,
				  metadata->name,
				  metadata->artist,
				  metadata->composer,
				  metadata->genre,
				  localph,
				  metadata->storage_id,
				  PTP_OFC_MTP_AbstractAudioAlbum,
				  ".alb",
				  &metadata->album_id,
				  metadata->tracks,
				  metadata->no_tracks);
}

/**
 * This creates a new sample data metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_sampledata_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings.
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_sampledata_t()
 */
LIBMTP_filesampledata_t *LIBMTP_new_filesampledata_t(void)
{
  LIBMTP_filesampledata_t *new = (LIBMTP_filesampledata_t *) malloc(sizeof(LIBMTP_filesampledata_t));
  if (new == NULL) {
    return NULL;
  }
  new->height=0;
  new->width = 0;
  new->data = NULL;
  new->duration = 0;
  new->size = 0;
  return new;
}

/**
 * This destroys a file sample metadata type.
 * @param sample the file sample metadata to be destroyed.
 */
void LIBMTP_destroy_filesampledata_t(LIBMTP_filesampledata_t * sample)
{
  if (sample == NULL) {
    return;
  }
  if (sample->data != NULL) {
    free(sample->data);
  }
  free(sample);
}

/**
 * This routine figures out whether a certain filetype supports
 * representative samples (small thumbnail images) or not. This
 * typically applies to JPEG files, MP3 files and Album abstract
 * playlists, but in theory any filetype could support representative
 * samples.
 * @param device a pointer to the device which is to be examined.
 * @param filetype the fileype to examine, and return the representative sample
 *        properties for.
 * @param sample this will contain a new sample type with the fields
 *        filled in with suitable default values. For example, the
 *        supported sample type will be set, the supported height and
 *        width will be set to max values if it is an image sample,
 *        and duration will also be given some suitable default value
 *        which should not be exceeded on audio samples. If the
 *        device does not support samples for this filetype, this
 *        pointer will be NULL. If it is not NULL, the user must
 *        destroy this struct with <code>LIBMTP_destroy_filesampledata_t()</code>
 *        after use.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Send_Representative_Sample()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Get_Representative_Sample_Format(LIBMTP_mtpdevice_t *device,
					    LIBMTP_filetype_t const filetype,
					    LIBMTP_filesampledata_t ** sample)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  uint16_t *props = NULL;
  uint32_t propcnt = 0;
  unsigned int i;
  // TODO: Get rid of these when we can properly query the device.
  int support_data = 0;
  int support_format = 0;
  int support_height = 0;
  int support_width = 0;
  int support_duration = 0;
  int support_size = 0;

  PTPObjectPropDesc opd_height;
  PTPObjectPropDesc opd_width;
  PTPObjectPropDesc opd_format;
  PTPObjectPropDesc opd_duration;
  PTPObjectPropDesc opd_size;

  // Default to no type supported.
  *sample = NULL;

  ret = ptp_mtp_getobjectpropssupported(params, map_libmtp_type_to_ptp_type(filetype), &propcnt, &props);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample_Format(): could not get object properties.");
    return -1;
  }
  /*
   * TODO: when walking through these object properties, make calls to
   * a new function in ptp.h/ptp.c that can send the command
   * PTP_OC_MTP_GetObjectPropDesc to get max/min values of the properties
   * supported.
   */
  for (i = 0; i < propcnt; i++) {
    switch(props[i]) {
    case PTP_OPC_RepresentativeSampleData:
      support_data = 1;
      break;
    case PTP_OPC_RepresentativeSampleFormat:
      support_format = 1;
      break;
    case PTP_OPC_RepresentativeSampleSize:
      support_size = 1;
      break;
    case PTP_OPC_RepresentativeSampleHeight:
      support_height = 1;
      break;
    case PTP_OPC_RepresentativeSampleWidth:
      support_width = 1;
      break;
    case PTP_OPC_RepresentativeSampleDuration:
      support_duration = 1;
      break;
    default:
      break;
    }
  }
  free(props);

  if (support_data && support_format && support_height && support_width && !support_duration) {
    // Something that supports height and width and not duration is likely to be JPEG
    LIBMTP_filesampledata_t *retsam = LIBMTP_new_filesampledata_t();
    /*
     * Populate the sample format with the first supported format
     *
     * TODO: figure out how to pass back more than one format if more are
     * supported by the device.
     */
    ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleFormat, map_libmtp_type_to_ptp_type(filetype), &opd_format);
    retsam->filetype = map_ptp_type_to_libmtp_type(opd_format.FORM.Enum.SupportedValue[0].u16);
    ptp_free_objectpropdesc(&opd_format);
    /* Populate the maximum image height */
    ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleWidth, map_libmtp_type_to_ptp_type(filetype), &opd_width);
    retsam->width = opd_width.FORM.Range.MaximumValue.u32;
    ptp_free_objectpropdesc(&opd_width);
    /* Populate the maximum image width */
    ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleHeight, map_libmtp_type_to_ptp_type(filetype), &opd_height);
    retsam->height = opd_height.FORM.Range.MaximumValue.u32;
    ptp_free_objectpropdesc(&opd_height);
    /* Populate the maximum size */
    if (support_size) {
      ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleSize, map_libmtp_type_to_ptp_type(filetype), &opd_size);
      retsam->size = opd_size.FORM.Range.MaximumValue.u32;
      ptp_free_objectpropdesc(&opd_size);
    }
    *sample = retsam;
  } else if (support_data && support_format && !support_height && !support_width && support_duration) {
    // Another qualified guess
    LIBMTP_filesampledata_t *retsam = LIBMTP_new_filesampledata_t();
    /*
     * Populate the sample format with the first supported format
     *
     * TODO: figure out how to pass back more than one format if more are
     * supported by the device.
     */
    ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleFormat, map_libmtp_type_to_ptp_type(filetype), &opd_format);
    retsam->filetype = map_ptp_type_to_libmtp_type(opd_format.FORM.Enum.SupportedValue[0].u16);
    ptp_free_objectpropdesc(&opd_format);
    /* Populate the maximum duration */
    ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleDuration, map_libmtp_type_to_ptp_type(filetype), &opd_duration);
    retsam->duration = opd_duration.FORM.Range.MaximumValue.u32;
    ptp_free_objectpropdesc(&opd_duration);
    /* Populate the maximum size */
    if (support_size) {
      ptp_mtp_getobjectpropdesc (params, PTP_OPC_RepresentativeSampleSize, map_libmtp_type_to_ptp_type(filetype), &opd_size);
      retsam->size = opd_size.FORM.Range.MaximumValue.u32;
      ptp_free_objectpropdesc(&opd_size);
    }
    *sample = retsam;
  }
  return 0;
}

/**
 * This routine sends representative sample data for an object.
 * This uses the RepresentativeSampleData property of the album,
 * if the device supports it. The data should be of a format acceptable
 * to the player (for iRiver and Creative, this seems to be JPEG) and
 * must not be too large. (for a Creative, max seems to be about 20KB.)
 * Check by calling LIBMTP_Get_Representative_Sample_Format() to get
 * maximum size, dimensions, etc..
 * @param device a pointer to the device which the object is on.
 * @param id unique id of the object to set artwork for.
 * @param sampledata pointer to LIBMTP_filesampledata_t struct containing data
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Representative_Sample()
 * @see LIBMTP_Get_Representative_Sample_Format()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Send_Representative_Sample(LIBMTP_mtpdevice_t *device,
                          uint32_t const id,
                          LIBMTP_filesampledata_t *sampledata)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  PTPPropertyValue propval;
  PTPObject *ob;
  uint32_t i;
  uint16_t *props = NULL;
  uint32_t propcnt = 0;
  int supported = 0;

  // get the file format for the object we're going to send representative data for
  ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
  if (ret != PTP_RC_OK) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_Representative_Sample(): could not get object info.");
    return -1;
  }

  // check that we can send representative sample data for this object format
  ret = ptp_mtp_getobjectpropssupported(params, ob->oi.ObjectFormat, &propcnt, &props);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_Representative_Sample(): could not get object properties.");
    return -1;
  }

  for (i = 0; i < propcnt; i++) {
    if (props[i] == PTP_OPC_RepresentativeSampleData) {
      supported = 1;
      break;
    }
  }
  if (!supported) {
    free(props);
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Send_Representative_Sample(): object type doesn't support RepresentativeSampleData.");
    return -1;
  }
  free(props);

  // Go ahead and send the data
  propval.a.count = sampledata->size;
  propval.a.v = malloc(sizeof(PTPPropertyValue) * sampledata->size);
  for (i = 0; i < sampledata->size; i++) {
    propval.a.v[i].u8 = sampledata->data[i];
  }

  ret = ptp_mtp_setobjectpropvalue(params,id,PTP_OPC_RepresentativeSampleData,
				   &propval,PTP_DTC_AUINT8);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Send_Representative_Sample(): could not send sample data.");
    free(propval.a.v);
    return -1;
  }
  free(propval.a.v);

  /* Set the height and width if the sample is an image, otherwise just
   * set the duration and size */
  switch(sampledata->filetype) {
  case LIBMTP_FILETYPE_JPEG:
  case LIBMTP_FILETYPE_JFIF:
  case LIBMTP_FILETYPE_TIFF:
  case LIBMTP_FILETYPE_BMP:
  case LIBMTP_FILETYPE_GIF:
  case LIBMTP_FILETYPE_PICT:
  case LIBMTP_FILETYPE_PNG:
    if (!FLAG_BROKEN_SET_SAMPLE_DIMENSIONS(ptp_usb)) {
      // For images, set the height and width
      set_object_u32(device, id, PTP_OPC_RepresentativeSampleHeight, sampledata->height);
      set_object_u32(device, id, PTP_OPC_RepresentativeSampleWidth, sampledata->width);
    }
    break;
  default:
    // For anything not an image, set the duration and size
    set_object_u32(device, id, PTP_OPC_RepresentativeSampleDuration, sampledata->duration);
    set_object_u32(device, id, PTP_OPC_RepresentativeSampleSize, sampledata->size);
    break;
  }

  return 0;
}

/**
 * This routine gets representative sample data for an object.
 * This uses the RepresentativeSampleData property of the album,
 * if the device supports it.
 * @param device a pointer to the device which the object is on.
 * @param id unique id of the object to get data for.
 * @param sampledata pointer to LIBMTP_filesampledata_t struct to receive data
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Send_Representative_Sample()
 * @see LIBMTP_Get_Representative_Sample_Format()
 * @see LIBMTP_Create_New_Album()
 */
int LIBMTP_Get_Representative_Sample(LIBMTP_mtpdevice_t *device,
                          uint32_t const id,
                          LIBMTP_filesampledata_t *sampledata)
{
  uint16_t ret;
  PTPParams *params = (PTPParams *) device->params;
  PTPPropertyValue propval;
  PTPObject *ob;
  uint32_t i;
  uint16_t *props = NULL;
  uint32_t propcnt = 0;
  int supported = 0;

  // get the file format for the object we're going to send representative data for
  ret = ptp_object_want (params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
  if (ret != PTP_RC_OK) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_Representative_Sample(): could not get object info.");
    return -1;
  }

  // check that we can store representative sample data for this object format
  ret = ptp_mtp_getobjectpropssupported(params, ob->oi.ObjectFormat, &propcnt, &props);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample(): could not get object properties.");
    return -1;
  }

  for (i = 0; i < propcnt; i++) {
    if (props[i] == PTP_OPC_RepresentativeSampleData) {
      supported = 1;
      break;
    }
  }
  if (!supported) {
    free(props);
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, "LIBMTP_Get_Representative_Sample(): object type doesn't support RepresentativeSampleData.");
    return -1;
  }
  free(props);

  // Get the data
  ret = ptp_mtp_getobjectpropvalue(params,id,PTP_OPC_RepresentativeSampleData,
				   &propval,PTP_DTC_AUINT8);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Representative_Sample(): could not get sample data.");
    return -1;
  }

  // Store it
  sampledata->size = propval.a.count;
  sampledata->data = malloc(sizeof(PTPPropertyValue) * propval.a.count);
  for (i = 0; i < propval.a.count; i++) {
    sampledata->data[i] = propval.a.v[i].u8;
  }
  free(propval.a.v);

  // Get the other properties
  sampledata->width = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleWidth, 0);
  sampledata->height = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleHeight, 0);
  sampledata->duration = get_u32_from_object(device, id, PTP_OPC_RepresentativeSampleDuration, 0);
  sampledata->filetype = map_ptp_type_to_libmtp_type(
        get_u16_from_object(device, id, PTP_OPC_RepresentativeSampleFormat, LIBMTP_FILETYPE_UNKNOWN));

  return 0;
}

/**
 * Retrieve the thumbnail for a file.
 * @param device a pointer to the device to get the thumbnail from.
 * @param id the object ID of the file to retrieve the thumbnail for.
 * @param data a memory pointer to retrieved thumbnail (user frees memory later when done with data).
 * @param size data length of retrieved thumbnail.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Get_Thumbnail(LIBMTP_mtpdevice_t *device, uint32_t const id,
                         unsigned char **data, unsigned int *size)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  ret = ptp_getthumb(params, id, data, size);
  if (ret == PTP_RC_OK)
      return 0;
  return -1;
}


int LIBMTP_GetPartialObject(LIBMTP_mtpdevice_t *device, uint32_t const id,
                            uint64_t offset, uint32_t maxbytes,
                            unsigned char **data, unsigned int *size)
{
  PTPParams	*params = (PTPParams *) device->params;
  uint16_t	ret;
  LIBMTP_file_t	*mtpfile = LIBMTP_Get_Filemetadata(device, id);

  if (!mtpfile) {
      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
        "LIBMTP_GetPartialObject: could not find mtpfile");
    *size = 0;
    return -1;
  }

  /* Some devices do not like reading over the end and hang instead of progressing */
  if (offset >= mtpfile->filesize) {
    *size = 0;
    LIBMTP_destroy_file_t (mtpfile);
    return 0;
  }
  if (offset + maxbytes > mtpfile->filesize) {
    maxbytes = mtpfile->filesize - offset;
  }

  /* do not need it anymore */
  LIBMTP_destroy_file_t (mtpfile);

  /* The MTP stack of Samsung Galaxy devices has a mysterious bug in
   * GetPartialObject. When GetPartialObject is invoked to read the
   * last bytes of a file and the amount of data to read is such that
   * the last USB packet sent in the reply matches exactly the USB 2.0
   * packet size, then the Samsung Galaxy device hangs, resulting in a
   * timeout error.
   * As a workaround, we read one less byte instead of reaching the
   * end of the file, forcing the caller to perform an additional read
   * to get the last byte (i.e. the final read that would fail is
   * replaced with two partial reads that succeed).
   */
  if ((params->device_flags & DEVICE_FLAG_SAMSUNG_OFFSET_BUG) &&
      (maxbytes % PTP_USB_BULK_HS_MAX_PACKET_LEN_READ) == (PTP_USB_BULK_HS_MAX_PACKET_LEN_READ - PTP_USB_BULK_HDR_LEN)) {
    maxbytes--;
  }

  if (!ptp_operation_issupported(params, PTP_OC_ANDROID_GetPartialObject64)) {
    if  (!ptp_operation_issupported(params, PTP_OC_GetPartialObject)) {
      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
        "LIBMTP_GetPartialObject: PTP_OC_GetPartialObject not supported");
      return -1;
    }

    if (offset >> 32 != 0) {
      add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
        "LIBMTP_GetPartialObject: PTP_OC_GetPartialObject only supports 32bit offsets");
      return -1;
    }

    ret = ptp_getpartialobject(params, id, (uint32_t)offset, maxbytes, data, size);
  } else {
    ret = ptp_android_getpartialobject64(params, id, offset, maxbytes, data, size);
  }
  if (ret == PTP_RC_OK)
      return 0;
  return -1;
}


int LIBMTP_SendPartialObject(LIBMTP_mtpdevice_t *device, uint32_t const id,
                             uint64_t offset, unsigned char *data, unsigned int size)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_operation_issupported(params, PTP_OC_ANDROID_SendPartialObject)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
      "LIBMTP_SendPartialObject: PTP_OC_ANDROID_SendPartialObject not supported");
    return -1;
  }

  ret = ptp_android_sendpartialobject(params, id, offset, data, size);
  if (ret == PTP_RC_OK)
      return 0;
  return -1;
}


int LIBMTP_BeginEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_operation_issupported(params, PTP_OC_ANDROID_BeginEditObject)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
      "LIBMTP_BeginEditObject: PTP_OC_ANDROID_BeginEditObject not supported");
    return -1;
  }

  ret = ptp_android_begineditobject(params, id);
  if (ret == PTP_RC_OK)
      return 0;
  return -1;
}


int LIBMTP_EndEditObject(LIBMTP_mtpdevice_t *device, uint32_t const id)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_operation_issupported(params, PTP_OC_ANDROID_EndEditObject)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
      "LIBMTP_EndEditObject: PTP_OC_ANDROID_EndEditObject not supported");
    return -1;
  }

  ret = ptp_android_endeditobject(params, id);
  if (ret == PTP_RC_OK) {
      // update cached object properties if metadata cache exists
      update_metadata_cache(device, id);
      return 0;
  }
  return -1;
}


int LIBMTP_TruncateObject(LIBMTP_mtpdevice_t *device, uint32_t const id,
                          uint64_t offset)
{
  PTPParams *params = (PTPParams *) device->params;
  uint16_t ret;

  if (!ptp_operation_issupported(params, PTP_OC_ANDROID_TruncateObject)) {
    add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL,
      "LIBMTP_TruncateObject: PTP_OC_ANDROID_TruncateObject not supported");
    return -1;
  }

  ret = ptp_android_truncate(params, id, offset);
  if (ret == PTP_RC_OK)
      return 0;
  return -1;
}


/**
 * This routine updates an album based on the metadata
 * supplied. If the <code>tracks</code> field of the metadata
 * contains a track listing, these tracks will be added to the
 * album in place of those already present, i.e. the
 * previous track listing will be deleted.
 * @param device a pointer to the device to create the new album on.
 * @param metadata the metadata for the album to be updated.
 *                 notice that the field <code>album_id</code>
 *                 must contain the appropriate album ID.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Create_New_Album()
 * @see LIBMTP_Delete_Object()
 */
int LIBMTP_Update_Album(LIBMTP_mtpdevice_t *device,
			   LIBMTP_album_t const * const metadata)
{
  return update_abstract_list(device,
			      metadata->name,
			      metadata->artist,
			      metadata->composer,
			      metadata->genre,
			      metadata->album_id,
			      PTP_OFC_MTP_AbstractAudioAlbum,
			      metadata->tracks,
			      metadata->no_tracks);
}

/**
 * Dummy function needed to interface to upstream
 * ptp.c/ptp.h files.
 */
void ptp_nikon_getptpipguid (unsigned char* guid) {
  return;
}

/**
 * Add an object to cache.
 * @param device the device which may have a cache to which the object should be added.
 * @param object_id the object to add to the cache.
 */
static void add_object_to_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id)
{
  PTPParams *params = (PTPParams *)device->params;
  uint16_t ret;

  ret = ptp_add_object_to_cache(params, object_id);
  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "add_object_to_cache(): couldn't add object to cache");
  }
}


/**
 * Update cache after object has been modified
 * @param device the device which may have a cache to which the object should be updated.
 * @param object_id the object to update.
 */
static void update_metadata_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id)
{
  PTPParams *params = (PTPParams *)device->params;

  ptp_remove_object_from_cache(params, object_id);
  add_object_to_cache(device, object_id);
}


/**
 * Issue custom (e.g. vendor specific) operation (without data phase)
 * @param device a pointer to the device to send custom operation to.
 * @param code operation code to send.
 * @param n_param number of parameters passed.
 * @param ... uint32_t operation specific parameters.
 */
int LIBMTP_Custom_Operation(LIBMTP_mtpdevice_t *device, uint16_t code, int n_param, ...)
{
  PTPParams *params = (PTPParams *) device->params;
  PTPContainer ptp;
  va_list args;
  uint16_t ret;
  int i;

  ptp.Code = code;
  ptp.Nparam = n_param;
  va_start(args, n_param);
  for (i = 0; i < n_param; i++)
    (&ptp.Param1)[i] = va_arg(args, uint32_t);
  va_end(args);

  ret = ptp_transaction_new(params, &ptp, PTP_DP_NODATA, 0, NULL);

  if (ret != PTP_RC_OK) {
    add_ptp_error_to_errorstack(device, ret, "LIBMTP_Custom_Operation(): failed to execute operation.");
    return -1;
  }

  return 0;
}

/**
 * Free memory allocated by libmtp. Is doing the same as libc free(mem) in most cases.
 * @mem pointer to allocated memory.
 */
void LIBMTP_FreeMemory(void *mem)
{
  free (mem);
}
