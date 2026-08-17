/**
 * \File playlist-spl.c
 *
 * Playlist_t to Samsung (.spl) and back conversion functions.
 *
 * Copyright (C) 2008 Alistair Boyle <alistair.js.boyle@gmail.com>
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
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h> // mkstmp()
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include <fcntl.h>
#include <string.h>

#include "libmtp.h"
#include "libusb-glue.h"
#include "ptp.h"
#include "unicode.h"
#include "util.h"

#include "playlist-spl.h"

/**
 * Debug macro
 */
#define LIBMTP_PLST_DEBUG(format, args...) \
  do { \
    if ((LIBMTP_debug & LIBMTP_DEBUG_PLST) != 0) \
      fprintf(stdout, "LIBMTP %s[%d]: " format, __FUNCTION__, __LINE__, ##args); \
  } while (0)


// Internal singly linked list of strings
// used to hold .spl playlist in memory
typedef struct text_struct {
  char* text; // String
  struct text_struct *next; // Link to next line, NULL if end of list
} text_t;


/**
 * Forward declarations of local (static) functions.
 */
static text_t* read_into_spl_text_t(LIBMTP_mtpdevice_t *device, const int fd);
static void write_from_spl_text_t(LIBMTP_mtpdevice_t *device, const int fd, text_t* p);
static void free_spl_text_t(text_t* p);
static void print_spl_text_t(text_t* p);
static uint32_t trackno_spl_text_t(text_t* p);
static void tracks_from_spl_text_t(text_t* p, uint32_t* tracks, LIBMTP_folder_t* folders, LIBMTP_file_t* files);
static void spl_text_t_from_tracks(text_t** p, uint32_t* tracks, const uint32_t trackno, const uint32_t ver_major, const uint32_t ver_minor, char* dnse, LIBMTP_folder_t* folders, LIBMTP_file_t* files);

static uint32_t discover_id_from_filepath(const char* s, LIBMTP_folder_t* folders, LIBMTP_file_t* files); // TODO add file/dir cached args
static void discover_filepath_from_id(char** p, uint32_t track, LIBMTP_folder_t* folders, LIBMTP_file_t* files);
static void find_folder_name(LIBMTP_folder_t* folders, uint32_t* id, char** name);
static uint32_t find_folder_id(LIBMTP_folder_t* folders, uint32_t parent, char* name);

static void append_text_t(text_t** t, char* s);




/**
 * Decides if the indicated object index is an .spl playlist.
 *
 * @param oi object we are deciding on
 * @return 1 if this is a Samsung .spl object, 0 otherwise
 */
int is_spl_playlist(PTPObjectInfo *oi)
{
  return ((oi->ObjectFormat == PTP_OFC_Undefined) ||
         (oi->ObjectFormat == PTP_OFC_MTP_SamsungPlaylist)) &&
         (strlen(oi->Filename) > 4) &&
         (strcmp((oi->Filename + strlen(oi->Filename) - 4), ".spl") == 0);
}

#ifndef HAVE_MKSTEMP
# ifdef __WIN32__
#  include <fcntl.h>
#  define mkstemp(_pattern) _open(_mktemp(_pattern), _O_CREAT | _O_SHORT_LIVED | _O_EXCL)
# else
#  error Missing mkstemp() function.
# endif
#endif

/**
 * Take an object ID, a .spl playlist on the MTP device,
 * and convert it to a playlist_t object.
 *
 * @param device mtp device pointer
 * @param oi object we are reading
 * @param id .spl playlist id on MTP device
 * @param pl the LIBMTP_playlist_t pointer to be filled with info from id
 */

void spl_to_playlist_t(LIBMTP_mtpdevice_t* device, PTPObjectInfo *oi,
                       const uint32_t id, LIBMTP_playlist_t * const pl)
{
  // Fill in playlist metadata
  // Use the Filename as the playlist name, dropping the ".spl" extension
  pl->name = malloc(sizeof(char)*(strlen(oi->Filename) -4 +1));
  memcpy(pl->name, oi->Filename, strlen(oi->Filename) -4);
  // Set terminating character
  pl->name[strlen(oi->Filename) - 4] = 0;
  pl->playlist_id = id;
  pl->parent_id = oi->ParentObject;
  pl->storage_id = oi->StorageID;
  pl->tracks = NULL;
  pl->no_tracks = 0;

  LIBMTP_PLST_DEBUG("pl->name='%s'\n", pl->name);

  // open a temporary file
  char tmpname[] = "/tmp/mtp-spl2pl-XXXXXX";
  int fd = mkstemp(tmpname);
  if(fd < 0) {
    LIBMTP_ERROR("failed to make temp file for %s.spl -> %s, errno=%s\n", pl->name, tmpname, strerror(errno));
    return;
  }
  // make sure the file will be deleted afterwards
  if(unlink(tmpname) < 0)
    LIBMTP_ERROR("failed to delete temp file for %s.spl -> %s, errno=%s\n", pl->name, tmpname, strerror(errno));
  int ret = LIBMTP_Get_File_To_File_Descriptor(device, pl->playlist_id, fd, NULL, NULL);
  if( ret < 0 ) {
    // FIXME     add_ptp_error_to_errorstack(device, ret, "LIBMTP_Get_Playlist: Could not get .spl playlist file.");
    close(fd);
    LIBMTP_INFO("FIXME closed\n");
  }

  text_t* p = read_into_spl_text_t(device, fd);
  close(fd);

  // FIXME cache these somewhere else so we don't keep calling this!
  LIBMTP_folder_t *folders;
  LIBMTP_file_t *files;
  folders = LIBMTP_Get_Folder_List(device);
  files = LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);

  // convert the playlist listing to track ids
  pl->no_tracks = trackno_spl_text_t(p);
  LIBMTP_PLST_DEBUG("%u track%s found\n", pl->no_tracks, pl->no_tracks==1?"":"s");
  pl->tracks = malloc(sizeof(uint32_t)*(pl->no_tracks));
  tracks_from_spl_text_t(p, pl->tracks, folders, files);

  free_spl_text_t(p);

  // debug: add a break since this is the top level function call
  LIBMTP_PLST_DEBUG("------------\n\n");
}


/**
 * Push a playlist_t onto the device after converting it to a .spl format
 *
 * @param device mtp device pointer
 * @param pl the LIBMTP_playlist_t to convert (pl->playlist_id will be updated
 *           with the newly created object's id)
 * @return 0 on success, any other value means failure.
 */
int playlist_t_to_spl(LIBMTP_mtpdevice_t *device,
                      LIBMTP_playlist_t * const pl)
{
  text_t* t;
  LIBMTP_folder_t *folders;
  LIBMTP_file_t *files;
  folders = LIBMTP_Get_Folder_List(device);
  files = LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);

  char tmpname[] = "/tmp/mtp-spl2pl-XXXXXX"; // must be a var since mkstemp modifies it

  LIBMTP_PLST_DEBUG("pl->name='%s'\n",pl->name);

  // open a file descriptor
  int fd = mkstemp(tmpname);
  if(fd < 0) {
    LIBMTP_ERROR("failed to make temp file for %s.spl -> %s, errno=%s\n", pl->name, tmpname, strerror(errno));
    return -1;
  }
  // make sure the file will be deleted afterwards
  if(unlink(tmpname) < 0)
    LIBMTP_ERROR("failed to delete temp file for %s.spl -> %s, errno=%s\n", pl->name, tmpname, strerror(errno));

  // decide on which version of the .spl format to use
  uint32_t ver_major;
  uint32_t ver_minor = 0;
  PTP_USB *ptp_usb = (PTP_USB*) device->usbinfo;
  if(FLAG_PLAYLIST_SPL_V2(ptp_usb)) ver_major = 2;
  else ver_major = 1; // FLAG_PLAYLIST_SPL_V1()

  LIBMTP_PLST_DEBUG("%u track%s\n", pl->no_tracks, pl->no_tracks==1?"":"s");
  LIBMTP_PLST_DEBUG(".spl version %d.%02d\n", ver_major, ver_minor);

  // create the text for the playlist
  spl_text_t_from_tracks(&t, pl->tracks, pl->no_tracks, ver_major, ver_minor, NULL, folders, files);
  write_from_spl_text_t(device, fd, t);
  free_spl_text_t(t); // done with the text

  // create the file object for storing
  LIBMTP_file_t* f = malloc(sizeof(LIBMTP_file_t));
  f->item_id = 0;
  f->parent_id = pl->parent_id;
  f->storage_id = pl->storage_id;
  f->filename = malloc(sizeof(char)*(strlen(pl->name)+5));
  strcpy(f->filename, pl->name);
  strcat(f->filename, ".spl"); // append suffix
  f->filesize = lseek(fd, 0, SEEK_CUR); // file desc is currently at end of file
  f->filetype = LIBMTP_FILETYPE_UNKNOWN;
  f->next = NULL;

  LIBMTP_PLST_DEBUG("%s is %dB\n", f->filename, (int)f->filesize);

  // push the playlist to the device
  lseek(fd, 0, SEEK_SET); // reset file desc. to start of file
  int ret = LIBMTP_Send_File_From_File_Descriptor(device, fd, f, NULL, NULL);
  pl->playlist_id = f->item_id;
  free(f->filename);
  free(f);

  // release the memory when we're done with it
  close(fd);
  // debug: add a break since this is the top level function call
  LIBMTP_PLST_DEBUG("------------\n\n");

  return ret;
}



/**
 * Update a playlist on the device. If only the playlist's name is being
 * changed the pl->playlist_id will likely remain the same. An updated track
 * list will result in the old playlist being replaced (ie: new playlist_id).
 * NOTE: Other playlist metadata aside from playlist name and tracks are
 * ignored.
 *
 * @param device mtp device pointer
 * @param newlist the LIBMTP_playlist_t to convert (pl->playlist_id will be updated
 *           with the newly created object's id)
 * @return 0 on success, any other value means failure.
 */
int update_spl_playlist(LIBMTP_mtpdevice_t *device,
			  LIBMTP_playlist_t * const newlist)
{
  LIBMTP_PLST_DEBUG("pl->name='%s'\n",newlist->name);

  // read in the playlist of interest
  LIBMTP_playlist_t * old = LIBMTP_Get_Playlist(device, newlist->playlist_id);
  
  // check to see if we found it
  if (!old)
    return -1;

  // check if the playlists match
  int delta = 0;
  unsigned int i;
  if(old->no_tracks != newlist->no_tracks)
    delta++;
  for(i=0;i<newlist->no_tracks && delta==0;i++) {
    if(old->tracks[i] != newlist->tracks[i])
      delta++;
  }

  // if not, kill the playlist and replace it
  if(delta) {
    LIBMTP_PLST_DEBUG("new tracks detected:\n");
    LIBMTP_PLST_DEBUG("delete old playlist and build a new one\n");
    LIBMTP_PLST_DEBUG(" NOTE: new playlist_id will result!\n");
    if(LIBMTP_Delete_Object(device, old->playlist_id) != 0)
      return -1;

    if(strcmp(old->name,newlist->name) == 0)
      LIBMTP_PLST_DEBUG("name unchanged\n");
    else
      LIBMTP_PLST_DEBUG("name is changing too -> %s\n",newlist->name);

    return LIBMTP_Create_New_Playlist(device, newlist);
  }


  // update the name only
  if(strcmp(old->name,newlist->name) != 0) {
    LIBMTP_PLST_DEBUG("ONLY name is changing -> %s\n",newlist->name);
    LIBMTP_PLST_DEBUG("playlist_id will remain unchanged\n");
    char* s = malloc(sizeof(char)*(strlen(newlist->name)+5));
    strcpy(s, newlist->name);
    strcat(s,".spl"); // FIXME check for success
    int ret = LIBMTP_Set_Playlist_Name(device, newlist, s);
    free(s);
    return ret;
  }

  LIBMTP_PLST_DEBUG("no change\n");
  return 0; // nothing to be done, success
}


/**
 * Load a file descriptor into a string.
 *
 * @param device a pointer to the current device.
 *               (needed for ucs2->utf8 charset conversion)
 * @param fd the file descriptor to load
 * @return text_t* a linked list of lines of text, id is left blank, NULL if nothing read in
 */
static text_t* read_into_spl_text_t(LIBMTP_mtpdevice_t *device, const int fd)
{
  // set MAXREAD to match STRING_BUFFER_LENGTH in unicode.h conversion function
  const size_t MAXREAD = 1024*2;
  char t[MAXREAD];
  // upto 3 bytes per utf8 character, 2 bytes per ucs2 character,
  // +1 for '\0' at end of string
  const size_t WSIZE = MAXREAD/2*3+1;
  char w[WSIZE];
  char* it = t; // iterator on t
  char* iw = w;
  ssize_t rdcnt;
  off_t offcnt;
  text_t* head = NULL;
  text_t* tail = NULL;
  int eof = 0;

  // reset file descriptor (fd) to start of file
  offcnt = lseek(fd, 0, SEEK_SET);

  while(!eof) {
    // find the current offset in the file
    // to allow us to determine how many bytes we read if we hit the EOF
    // where returned rdcnt=0 from read()
    offcnt = lseek(fd, 0, SEEK_CUR);
    // read to refill buffer
    // (there might be data left from an incomplete last string in t,
    // hence start filling at it)
    it = t; // set ptr to start of buffer
    rdcnt = read(fd, it, sizeof(char)*MAXREAD);
    if(rdcnt < 0)
      LIBMTP_INFO("load_spl_fd read err %s\n", strerror(errno));
    else if(rdcnt == 0) { // for EOF, fix rdcnt
      if(it-t == MAXREAD)
        LIBMTP_ERROR("error -- buffer too small to read in .spl playlist entry\n");

      rdcnt = lseek(fd, 0, SEEK_CUR) - offcnt;
      eof = 1;
    }

    LIBMTP_PLST_DEBUG("read buff= {%dB new, %dB old/left-over}%s\n",(int)rdcnt, (int)(iw-w), eof?", EOF":"");

    // while more input bytes
    char* it_end = t + rdcnt;
    while(it < it_end) {
      // copy byte, unless EOL (then replace with end-of-string \0)
      if(*it == '\r' || *it == '\n')
        *iw = '\0';
      else
        *iw = *it;

      it++;
      iw++;

      // EOL -- store it
      if( (iw-w) >= 2 && // we must have at least two bytes
          *(iw-1) == '\0' && *(iw-2) == '\0' && // 0x0000 is end-of-string
          // but it must be aligned such that we have an {odd,even} set of
          // bytes since we are expecting to consume bytes two-at-a-time
          !((iw-w)%2) ) {

        // drop empty lines
        //  ... cast as a string of 2 byte characters
        if(ucs2_strlen((uint16_t*)w) == 0) {
          iw = w;
          continue;
        }

        // create a new node in the list
        if(head == NULL) {
          head = malloc(sizeof(text_t));
          tail = head;
        }
        else {
          tail->next = malloc(sizeof(text_t));
          tail = tail->next;
        }
        // fill in the data for the node
        //  ... cast as a string of 2 byte characters
        tail->text = utf16_to_utf8(device, (uint16_t*) w);
        iw = w; // start again

        LIBMTP_PLST_DEBUG("line: %s\n", tail->text);
      }

      // prevent buffer overflow
      if(iw >= w + WSIZE) {
        // if we ever see this error its BAD:
        //   we are dropping all the processed bytes for this line and
        //   proceeding on as if everything is okay, probably losing a track
        //   from the playlist
        LIBMTP_ERROR("ERROR %s:%u:%s(): buffer overflow! .spl line too long @ %zuB\n",
               __FILE__, __LINE__, __func__, WSIZE);
        iw = w; // reset buffer
      }
    }

    // if the last thing we did was save our line, then we finished working
    // on the input buffer and we can start fresh
    // otherwise we need to save our partial work, if we're not quiting (eof).
    // there is nothing special we need to do, to achieve this since the
    // partially completed string will sit in 'w' until we return to complete
    // the line

  }

  // set the next pointer at the end
  // if there is any list
  if(head != NULL)
    tail->next = NULL;

  // return the head of the list (NULL if no list)
  return head;
}


/**
 * Write a .spl text file to a file in preparation for pushing it
 * to the device.
 *
 * @param fd file descriptor to write to
 * @param p the text to output one line per string in the linked list
 * @see playlist_t_to_spl()
 */
static void write_from_spl_text_t(LIBMTP_mtpdevice_t *device,
                                  const int fd,
                                  text_t* p) {
  ssize_t ret;
  // write out BOM for utf16/ucs2 (byte order mark)
  ret = write(fd,"\xff\xfe",2);
  while(p != NULL) {
    char *const t = (char*) utf8_to_utf16(device, p->text);
    // note: 2 bytes per ucs2 character
    const size_t len = ucs2_strlen((uint16_t*)t)*sizeof(uint16_t);
    unsigned int i;

    LIBMTP_PLST_DEBUG("\nutf8=%s ",p->text);
    for(i=0;i<strlen(p->text);i++)
      LIBMTP_PLST_DEBUG("%02x ", p->text[i] & 0xff);
    LIBMTP_PLST_DEBUG("\n");
    LIBMTP_PLST_DEBUG("ucs2=");
    for(i=0;i<ucs2_strlen((uint16_t*)t)*sizeof(uint16_t);i++)
      LIBMTP_PLST_DEBUG("%02x ", t[i] & 0xff);
    LIBMTP_PLST_DEBUG("\n");

    // write: utf8 -> utf16
    ret += write(fd, t, len);

    // release the converted string
    free(t);

    // check for failures
    if(ret < 0)
      LIBMTP_ERROR("write spl file failed: %s\n", strerror(errno));
    else if(ret != len +2)
      LIBMTP_ERROR("write spl file wrong number of bytes ret=%d len=%d '%s'\n", (int)ret, (int)len, p->text);

    // write carriage return, line feed in ucs2
    ret = write(fd, "\r\0\n\0", 4);
    if(ret < 0)
      LIBMTP_ERROR("write spl file failed: %s\n", strerror(errno));
    else if(ret != 4)
      LIBMTP_ERROR("failed to write the correct number of bytes '\\n'!\n");

    // fake out count (first time through has two extra bytes from BOM)
    ret = 2;

    // advance to the next line
    p = p->next;
  }
}

/**
 * Destroy a linked-list of strings.
 *
 * @param p the list to destroy
 * @see spl_to_playlist_t()
 * @see playlist_t_to_spl()
 */
static void free_spl_text_t(text_t* p)
{
  text_t* d;
  while(p != NULL) {
    d = p;
    free(p->text);
    p = p->next;
    free(d);
  }
}

/**
 * Print a linked-list of strings to stdout.
 * Used to debug.
 *
 * @param p the list to print
 */
static void print_spl_text_t(text_t* p)
{
  while(p != NULL) {
    LIBMTP_PLST_DEBUG("%s\n",p->text);
    p = p->next;
  }
}

/**
 * Count the number of tracks in this playlist. A track will be counted as
 * such if the line starts with a leading slash.
 *
 * @param p the text to search
 * @return number of tracks in the playlist
 * @see spl_to_playlist_t()
 */
static uint32_t trackno_spl_text_t(text_t* p) {
  uint32_t c = 0;
  while(p != NULL) {
    if(p->text[0] == '\\' ) c++;
    p = p->next;
  }

  return c;
}

/**
 * Find the track ids for this playlist's files.
 * (ie: \Music\song.mp3 -> 12345)
 *
 * @param p the text to search
 * @param tracks returned list of track id's for the playlist_t, must be large
 *               enough to accomodate all the tracks as reported by
 *               trackno_spl_text_t()
 * @param folders the folders list for the device
 * @param files the files list for the device
 * @see spl_to_playlist_t()
 */
static void tracks_from_spl_text_t(text_t* p,
                                   uint32_t* tracks,
                                   LIBMTP_folder_t* folders,
                                   LIBMTP_file_t* files)
{
  uint32_t c = 0;
  while(p != NULL) {
    if(p->text[0] == '\\' ) {
      tracks[c] = discover_id_from_filepath(p->text, folders, files);
      LIBMTP_PLST_DEBUG("track %d = %s (%u)\n", c+1, p->text, tracks[c]);
      c++;
    }
    p = p->next;
  }
}


/**
 * Find the track names (including path) for this playlist's track ids.
 * (ie: 12345 -> \Music\song.mp3)
 *
 * @param p the text to search
 * @param tracks list of track id's to look up
 * @param folders the folders list for the device
 * @param files the files list for the device
 * @see playlist_t_to_spl()
 */
static void spl_text_t_from_tracks(text_t** p,
                                   uint32_t* tracks,
                                   const uint32_t trackno,
                                   const uint32_t ver_major,
                                   const uint32_t ver_minor,
                                   char* dnse,
                                   LIBMTP_folder_t* folders,
                                   LIBMTP_file_t* files)
{

  // HEADER
  text_t* c = NULL;
  append_text_t(&c, "SPL PLAYLIST");
  *p = c; // save the top of the list!

  char vs[14]; // "VERSION 2.00\0"
  sprintf(vs,"VERSION %d.%02d",ver_major,ver_minor);

  append_text_t(&c, vs);
  append_text_t(&c, "");

  // TRACKS
  unsigned int i;
  char* f;
  for(i=0;i<trackno;i++) {
    discover_filepath_from_id(&f, tracks[i], folders, files);

    if(f != NULL) {
      append_text_t(&c, f);
      LIBMTP_PLST_DEBUG("track %d = %s (%u)\n", i+1, f, tracks[i]);
      free(f);
    }
    else
      LIBMTP_ERROR("failed to find filepath for track=%d\n", tracks[i]);
  }

  // FOOTER
  append_text_t(&c, "");
  append_text_t(&c, "END PLAYLIST");
  if(ver_major == 2) {
    append_text_t(&c, "");
    append_text_t(&c, "myDNSe DATA");
    if(dnse != NULL) {
      append_text_t(&c, dnse);
    }
    else {
      append_text_t(&c, "");
      append_text_t(&c, "");
    }
    append_text_t(&c, "END myDNSe");
  }

  c->next = NULL;

  // debug
  LIBMTP_PLST_DEBUG(".spl playlist:\n");
  print_spl_text_t(*p);
}


/**
 * Find the track names (including path) given a fileid
 * (ie: 12345 -> \Music\song.mp3)
 *
 * @param p returns the file path (ie: \Music\song.mp3),
 *          (*p) == NULL if the look up fails
 * @param track track id to look up
 * @param folders the folders list for the device
 * @param files the files list for the device
 * @see spl_text_t_from_tracks()
 */

// returns p = NULL on failure, else the filepath to the track including track name, allocated as a correct length string
static void discover_filepath_from_id(char** p,
                                      uint32_t track,
                                      LIBMTP_folder_t* folders,
                                      LIBMTP_file_t* files)
{
  // fill in a string from the right side since we don't know the root till the end
  const int M = 1024;
  char w[M];
  char* iw = w + M; // iterator on w

  // in case of failure return NULL string
  *p = NULL;


  // find the right file
  while(files != NULL && files->item_id != track) {
    files = files->next;
  }
  // if we didn't find a matching file, abort
  if(files == NULL)
    return;

  // stuff the filename into our string
  // FIXME: check for string overflow before it occurs
  iw = iw - (strlen(files->filename) +1); // leave room for '\0' at the end
  strcpy(iw,files->filename);

  // next follow the directories to the root
  // prepending folders to the path as we go
  uint32_t id = files->parent_id;
  char* f = NULL;
  while(id != 0) {
    find_folder_name(folders, &id, &f);
    if(f == NULL) return; // fail if the next part of the path couldn't be found
    iw = iw - (strlen(f) +1);
    // FIXME: check for string overflow before it occurs
    strcpy(iw, f);
    iw[strlen(f)] = '\\';
    free(f);
  }

  // prepend a slash
  iw--;
  iw[0] = '\\';

  // now allocate a string of the right length to be returned
  *p = strdup(iw);
}


/**
 * Find the track id given a track's name (including path)
 * (ie: \Music\song.mp3 -> 12345)
 *
 * @param s file path to look up (ie: \Music\song.mp3),
 *          (*p) == NULL if the look up fails
 * @param folders the folders list for the device
 * @param files the files list for the device
 * @return track id, 0 means failure
 * @see tracks_from_spl_text_t()
 */
static uint32_t discover_id_from_filepath(const char* s, LIBMTP_folder_t* folders, LIBMTP_file_t* files)
{
  // abort if this isn't a path
  if(s[0] != '\\')
    return 0;

  unsigned int i;
  uint32_t id = 0;
  char* sc = strdup(s);
  char* sci = sc +1; // iterator
  // skip leading slash in path

  // convert all \ to \0
  size_t len = strlen(s);
  for(i=0;i<len;i++) {
    if(sc[i] == '\\') {
      sc[i] = '\0';
    }
  }

  // now for each part of the string, find the id
  while(sci != sc + len +1) {
    // if its the last part of the string, its the filename
    if(sci + strlen(sci) == sc + len) {

      while(files != NULL) {
        // check parent matches id and name matches sci
        if( (files->parent_id == id) &&
            (strcmp(files->filename, sci) == 0) ) { // found it!
          id = files->item_id;
          break;
        }
        files = files->next;
      }
    }
    else { // otherwise its part of the directory path
      id = find_folder_id(folders, id, sci);
    }

    // move to next folder/file
    sci += strlen(sci) +1;
  }

  // release our copied string
  free(sc);

  // FIXME check that we actually have a file

  return id;
}



/**
 * Find the folder name given the folder's id.
 *
 * @param folders the folders list for the device
 * @param id the folder_id to look up, returns the folder's parent folder_id
 * @param name returns the name of the folder or NULL on failure
 * @see discover_filepath_from_id()
 */
static void find_folder_name(LIBMTP_folder_t* folders, uint32_t* id, char** name)
{

  // FIXME this function is exactly LIBMTP_Find_Folder

  LIBMTP_folder_t* f = LIBMTP_Find_Folder(folders, *id);
  if(f == NULL) {
    *name = NULL;
  }
  else { // found it!
    *name = strdup(f->name);
    *id = f->parent_id;
  }
}


/**
 * Find the folder id given the folder's name and parent id.
 *
 * @param folders the folders list for the device
 * @param parent the folder's parent's id
 * @param name the name of the folder
 * @return the folder_id or 0 on failure
 * @see discover_filepath_from_id()
 */
static uint32_t find_folder_id(LIBMTP_folder_t* folders, uint32_t parent, char* name) {

  if(folders == NULL)
    return 0;

  // found it!
  else if( (folders->parent_id == parent) &&
           (strcmp(folders->name, name) == 0) )
    return folders->folder_id;

  // no luck so far, search both siblings and children
  else {
    uint32_t id = 0;

    if(folders->sibling != NULL)
      id = find_folder_id(folders->sibling, parent, name);
    if( (id == 0) && (folders->child != NULL) )
      id = find_folder_id(folders->child, parent, name);

    return id;
  }
}


/**
 * Append a string to a linked-list of strings.
 *
 * @param t the list-of-strings, returns with the added string
 * @param s the string to append
 * @see spl_text_t_from_tracks()
 */
static void append_text_t(text_t** t, char* s)
{
  if(*t == NULL) {
    *t = malloc(sizeof(text_t));
  }
  else {
    (*t)->next = malloc(sizeof(text_t));
    (*t) = (*t)->next;
  }
  (*t)->text = strdup(s);
}
