#pragma once

/*** This particular file distributed under Public Domain terms. ***/

/** This file contains main definitions of commands used by far2l TTY extensions,
 * as well as some documentation for them.
 */

////////////////////
/**
FARTTY_INTERACT_* commands are send from client to server to request it to perform some action.
Request looks as "\x1b_far2l:"BASE64-encoded-arguments-stack"\x07"
For details of arguments stack encoding see utils/StackSerializer.h and utils/StackSerializer.cpp.
Each request's stack has on top 8-bit ID followed by any of FARTTY_INTERACT_* and related arguments.
If request ID is zero then server doesnt reply on such request, if ID is not zero then upon its
completion server sends back to client reply that has similar encoding and same ID on top of its
arguments stack, however other reply's arguments represent result of requested operation.
Note that in descriptions below arguments are listed in stack top->bottom order.
All integer values are in little-endian format.
*/

/** Initiates ad-hoc copy-to-clipboard starting at last mouse click position
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERACT_CONSOLE_ADHOC_QEDIT       'e'

/** Maximizes window
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERACT_WINDOW_MAXIMIZE           'M'

/** Makes window to be not-maximized
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERACT_WINDOW_RESTORE            'm'

/** Various operations with clipboard, see also FARTTY_INTERACT_CLIP_*
 In:
  char (FARTTY_INTERACT_CLIP_* subcommand)
  ..subcommands-specific
 Out:
  ..subcommands-specific
*/
#define FARTTY_INTERACT_CLIPBOARD                 'c'

/** Changes height of cursor in percents
 In:
  uint8_t (cursor height to set from 0% to 100%)
 Out: N/A
*/
#define FARTTY_INTERACT_SET_CURSOR_HEIGHT         'h'

/** Gets maximum possible size of window
 In: N/A
 Out:
  uint16_t (height)
  uint16_t (width)
*/
#define FARTTY_INTERACT_GET_WINDOW_MAXSIZE        'w'

/** Displays desktop notification with given title and text
 In:
  string (title)
  string (text)
 Out: N/A
*/
#define FARTTY_INTERACT_DESKTOP_NOTIFICATION      'n'

/** Sets titles of F-keys board if host supports this (invented for Mac touchbar)
 In:
  12 elements each is either [uint8_t (state) = 0] either [uint8_t (state) != 0; string (F-key title)]
 Out:
  bool (true on success; false if operation impossible)
*/
#define FARTTY_INTERACT_SET_FKEY_TITLES           'f'

/** Request color palette info
 In:
  N/A
 Out:
   uint8_t maximum count of color resolution bits supported (4, 8, 24)
   uint8_t reserved and set to zero, client should ignore it
*/
#define FARTTY_INTERACT_GET_COLOR_PALETTE         'p'


/** Declares that client supports specified extra features, so server _may_ change its hehaviour accordingly if it also supports some of them
 In:
  uint64_t (set of FARTTY_FEAT_* bit flags)
 Out: N/A
*/
#define FARTTY_INTERACT_CHOOSE_EXTRA_FEATURES     'x'

///////////////////////
/** Clipboard operations.
Synopsis:

 Usecase - put text into clipboard in single transaction:
  CLIP_OPEN(PASSCODE) -> (STATUS)
  CLIP_SETDATA(CF_TEXT, UTF8 encoded text) -> (STATUS, ID of retrieved data)
  CLIP_CLOSE -> STATUS

 Put text into clipboard in multiple transaction:
  CLIP_OPEN(PASSCODE) -> (STATUS)
  CLIP_SETDATACHUNK(first part of UTF8 encoded text)
  CLIP_SETDATACHUNK(second part of UTF8 encoded text)
  ...
  CLIP_SETDATA(CF_TEXT, last part of UTF8 encoded text) -> (STATUS, ID of stored data)
  CLIP_CLOSE -> (STATUS)

 Check if there text on clipboard:
  CLIP_ISAVAIL(CF_TEXT) -> (TRUE or FALSE)

 Get ID of text on clipboard:
  CLIP_OPEN(PASSCODE) -> (STATUS)
  CLIP_GETDATAID(CF_TEXT) -> (STATUS, ID of remote data)
  CLIP_CLOSE -> (STATUS)

 Get text from clipboard:
  CLIP_OPEN(PASSCODE) -> (STATUS)
  CLIP_GETDATA(CF_TEXT) -> (STATUS, UTF8 encoded text, ID of retrieved data)
  CLIP_CLOSE -> (STATUS)


Glossary:

 Passcode - random string that client sends to server to identify itself. Server on its side may
  ask user for allowing clipboard access and may use this passcode to remember user's choice.
  Its recommended to remember passcode on server side on per-client identity basis, to protect
  against malicious client that somehow stolen other client's passcode.

 Clipboard format ID - value that describes kind of data to be transferred. ID can be predefined
  or dynamically registered. In first case it describes some well-known data format, in another -
  data is treated by protocol as opaque BLOB.
  Predefined ID values used by far2l based on Win32 IDs, but with some modifications, currently
  only following predefined values are used by far2l in reality:
   1  - CF_TEXT - text encoded as UTF8
   13 - CF_UNICODETEXT - text encoded as UTF32 (depcrecated in recent releases in favor of CF_TEXT)
   Also far2l dynamically registers some own data formats to copy-paste vertical text blocks etc.
    At same moment of time clipboard may contain several different formats, thus allowing data to be
    represented in different forms. Also CF_TEXT/CF_UNICODETEXT transparently transcoded if needed.

 Clipboard data ID - 64-bit value that uniquely corresponds to data, currently its implemented as
  crc64 of data. It can be used by client to check if clipboard contains same data as client has in
  its own cache and thus allows to avoid duplicated network transfers.
*/

/** Authorizes clipboard accessor and opens clipboard for any subsequent operation.
 In:
  string (32 <= length <= 256 - random client ID used as passcode in subsequent clipboard opens)
 Out:
  int8_t (1 - success, 0 - failure, -1 - access denied)
  uint64_t OPTIONAL (combination of FARTTY_FEATCLIP_* supported by server)
*/
#define FARTTY_INTERACT_CLIP_OPEN                  'o'

/** Closes clipboard, must be used to properly finalize required clipboard action.
 In: N/A
 Out:
  int8_t (1 - success, 0 - failure, -1 - clipboard wasn't open)
*/
#define FARTTY_INTERACT_CLIP_CLOSE                 'c'

/** Empties clipboard.
 In: N/A
 Out:
  int8_t (1 - success, 0 - failure, -1 - clipboard wasn't open)
*/
#define FARTTY_INTERACT_CLIP_EMPTY                 'e'

/** Checks if given format available for get'ing from clipboard.
 In: uint32_t - format ID
 Out: int8_t (1 - there is data of such format, 0 - there is no data of such format)
*/
#define FARTTY_INTERACT_CLIP_ISAVAIL               'a'

/** Allows chunked clipboard data setting by sendings multiple chunks before final SETDATA.
 Special case: chunk with zero size treated as dismissing all previously queued chunks.
 OPTIONAL: can be used only if server reported FARTTY_FEATCLIP_CHUNKED_SET as supported feature
 In:
  uint16_t (chunk's data size, shifted right by 8 bits)
  data of specified size
 Out: N/A
*/
#define FARTTY_INTERACT_CLIP_SETDATACHUNK          'S'

/** Puts into clipboard data of specified format. Prepends given data with pending chunks (if any).
 In:
  uint32_t (format ID)
  uint32_t (size of data)
  data of specified size
 Out:
  int8_t (1 - success, 0 - failure, -1 - clipboard wasn't open)
  uint64_t OPTIONAL (clipboard data ID, only if server reported FARTTY_FEATCLIP_DATA_ID)
*/
#define FARTTY_INTERACT_CLIP_SETDATA               's'

/** Gets from clipboard data of specified format.
 In:
  uint32_t (format ID)
 Out:
  uint32_t (0 - on failure, -1 - clipboard wasn't open, other value - size of data - on success)
  data of specified size
  uint64_t OPTIONAL (clipboard data ID, only if server reported FARTTY_FEATCLIP_DATA_ID)
*/
#define FARTTY_INTERACT_CLIP_GETDATA               'g'

/** Gets ID of current clipboard data of specified format.
 OPTIONAL: can be used only if server reported FARTTY_FEATCLIP_DATA_ID as supported feature
 In:
  uint32_t (format ID)
 Out:
  uint64_t (0 - failure, nonzero value - clipboard data ID)
*/
#define FARTTY_INTERACT_CLIP_GETDATAID             'i'

/** Registers arbitrary clipboard data format.
 In:
  string (format name to be registered)
 Out:
  uint32_t (0 - failure, nonzero value - registered format ID)
*/
#define FARTTY_INTERACT_CLIP_REGISTER_FORMAT       'r'

///////////////////////

/** Client may specify this wanted extra feature if it supports compact input events.
 If server supports this feature it may send such events, however client should be ready to
 receive not-compact events as well.
*/
#define FARTTY_FEAT_COMPACT_INPUT             0x00000001

/** Client may specify this wanted extra feature if it supports in-band terminal size events.
 If server supports this feature it may send such events, however client should be ready to
 handle usual SIGWINCH signals as well.
*/
#define FARTTY_FEAT_TERMINAL_SIZE             0x00000002

/** Server reports this on responce of FARTTY_INTERRACT_CLIP_OPEN if it supports clipboard data ID.
 Clipboard data ID allows client-side caching of clipboard data to avoid known data transfers.
*/
#define FARTTY_FEATCLIP_DATA_ID               0x00000001

/** Server reports this on response of FARTTY_INTERACT_CLIP_OPEN if it supports chunked clipboard data set.
 This feature allows client to implement background and cancellable clipboard copy.
*/
#define FARTTY_FEATCLIP_CHUNKED_SET           0x00000002

///////////////////////
/**
FARTTY_INPUT_* notifications are send from server to client to inform about specific event happened.
Notification looks as "\x1b_f2l:"BASE64-encoded-arguments-stack"\x07"
For details of arguments stack encoding see utils/StackSerializer.h and utils/StackSerializer.cpp.
Unlike FARTTY_INTERACT_* there is no ID and no replies are expected from client to server,
all arguments are defined by FARTTY_INPUT_* notification ID - see below (stack top->bottom order).
*/


/** Server sends this to inform about mouse event. See MOUSE_EVENT_RECORD for details.
  uint32_t (dwEventFlags)
  uint32_t (dwControlKeyState)
  uint32_t (dwButtonState)
  int16_t (pos.Y)
  int16_t (pos.X)
*/
#define FARTTY_INPUT_MOUSE                    'M'

/** Server sends this to inform about mouse event if compact input is enabled and possible. See MOUSE_EVENT_RECORD for details.
  uint8_t (dwEventFlags)
  uint8_t (dwControlKeyState)
  uint16_t (dwButtonState encoded as: (dwButtonState & 0xff) | ( (dwButtonState >> 8) & 0xff00))
  int16_t (pos.Y)
  int16_t (pos.X)
*/
#define FARTTY_INPUT_MOUSE_COMPACT            'm'

/** Server sends this to inform about keydown or keyup event. See KEY_EVENT_RECORD for details.
  uint32_t (UTF32 code)
  uint32_t (dwControlKeyState)
  uint16_t (wVirtualScanCode)
  uint16_t (wVirtualKeyCode)
  uint16_t (wRepeatCount)
*/
#define FARTTY_INPUT_KEYDOWN                  'K'
#define FARTTY_INPUT_KEYUP                    'k'

/** Server sends this to inform about keydown or keyup event if compact input is enabled and possible.
  See KEY_EVENT_RECORD for details. Note that event doesn't specify wVirtualScanCode and wRepeatCount
  thus client must use Virtual Key Code to Virtual Scan Code translation and assume wRepeatCount = 1
  uint16_t (UTF32 code fit to uint16_t)
  uint16_t (dwControlKeyState fit to uint16_t)
  uint8_t (wVirtualKeyCode fit to single-byte)
*/
#define FARTTY_INPUT_KEYDOWN_COMPACT          'C'
#define FARTTY_INPUT_KEYUP_COMPACT            'c'

/** Server sends this to inform about recent terminal size.
  uint16_t (terminal width)
  uint16_t (terminal height)
*/
#define FARTTY_INPUT_TERMINAL_SIZE          'S'
