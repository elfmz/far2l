#pragma once

/** This file contains main definitions of commands used by far2l TTY extensions,
 as well as some documentation for them.
*/

////////////////////
/**
FARTTY_INTERRACT_* commands are send from client to server to request it to perform some action.
Request looks as "\x1b_far2l:"BASE64-encoded-arguments-stack"\x07"
For details of arguments stack encoding see utils/StackSerializer.h and utils/StackSerializer.cpp.
Each request's stack has on top 8-bit ID followed by any of FARTTY_INTERRACT_* and related arguments.
If request ID is zero then server doesnt reply on such request, if ID is not zero then upon its
completion server sends back to client reply that has similar encoding and same ID on top of its
arguments stack, however other reply's arguments represent result of requested operation.
*/

/** Initiates ad-hoc copy-pasting starting at last mouse click position
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERRACT_CONSOLE_ADHOC_QEDIT       'e'

/** Maximizes window
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERRACT_WINDOW_MAXIMIZE           'M'

/** Makes window to be not-maximized
 In: N/A
 Out: N/A
*/
#define FARTTY_INTERRACT_WINDOW_RESTORE            'm'

/** Various operations with clipboard, see also FARTTY_INTERRACT_CLIP_*
 In:
  char (FARTTY_INTERRACT_CLIP_* subcommand)
  ..subcommands-specific
 Out:
  ..subcommands-specific
*/
#define FARTTY_INTERRACT_CLIPBOARD                 'c'

/** Changes height of cursor in percents
 In:
  uint8_t (cursor height to set from 0% to 100%)
 Out: N/A
*/
#define FARTTY_INTERRACT_SET_CURSOR_HEIGHT         'h'

/** Gets maximum possible size of window
 In: N/A
 Out:
  COORD (window size)
*/
#define FARTTY_INTERRACT_GET_WINDOW_MAXSIZE        'w'

/** Sets titles of F-keys board if host supports this (invented for Mac touchbar)
 In:
  12 elements each is either [uint8_t (state) = 0] either [uint8_t (state) != 0; string (F-key title)]
 Out:
  bool (true on success; false if operation impossible)
*/
#define FARTTY_INTERRACT_DESKTOP_NOTIFICATION      'n'

/** Displays desktop notification with given title and text
 In:
  string (title)
  string (text)
 Out: N/A
*/
#define FARTTY_INTERRACT_SET_FKEY_TITLES           'f'

/** Declares that client supports specified extra features, so server _may_ change its hehaviour accordingly if it also supports some of them
 In:
  uint64_t (set of FARTTY_FEAT_* bit flags)
 Out: N/A
*/
#define FARTTY_INTERRACT_CHOOSE_EXTRA_FEATURES     'x'

///////////////////////

/** Authorizes clipboard accessor and opens clipboard for any subsequent operation.
 In:
  string (32 <= length <=256 - random client ID used as passcode in following clipboard opens)
 Out:
  char (1 - success, 0 - failure, -1 - access denied)
  uint64_t OPTIONAL (combination of FARTTY_FEATCLIP_* supported by server)
*/
#define FARTTY_INTERRACT_CLIP_OPEN                  'o'

/** Closes clipboard, must be used to properly finalize required clipboard action.
 In: N/A
 Out:
  char (1 - success, 0 - failure, -1 - clipboard wasn't open)
*/
#define FARTTY_INTERRACT_CLIP_CLOSE                 'c'

/** Empties clipboard.
 In: N/A
 Out:
  char (1 - success, 0 - failure, -1 - clipboard wasn't open)
*/
#define FARTTY_INTERRACT_CLIP_EMPTY                 'e'

/** Checks if given format available for get'ing from clipboard.
 In: uint32_t - format ID
 Out: char (1 - there is data of such format, 0 - there is no data of such format)
*/
#define FARTTY_INTERRACT_CLIP_ISAVAIL               'a'

/** Allows chunked clipboard data setting by sendings multiple chunks berfore final SETDATA.
 Special case: chunk with zero size treated as dismissing all previously queued chunks.
 OPTIONAL: can be used only if server reported FARTTY_FEATCLIP_CHUNKED_SET as supported feature
 In:
  uint16_t (chunk size, shifted right by 8 bits)
  data of specified size
 Out: N/A
*/
#define FARTTY_INTERRACT_CLIP_SETDATACHUNK          'S'

/** Puts into clipboard data of specified format. Prepends given data with pending chunks (if any).
 In:
  uint32_t (format ID)
  uint32_t (size)
  data of specified size
 Out:
  char (1 - success, 0 - failure, -1 - clipboard wasn't open)
  uint64_t OPTIONAL (clipboard data ID, only if server reported FARTTY_FEATCLIP_DATA_ID)
*/
#define FARTTY_INTERRACT_CLIP_SETDATA               's'

/** Gets from clipboard data of specified format.
 In:
  uint32_t (format ID)
 Out:
  char (1 - success, 0 - failure, -1 - clipboard wasn't open)
  uint32_t (size of data)
  data of specified size
  uint64_t OPTIONAL (clipboard data ID, only if server reported FARTTY_FEATCLIP_DATA_ID)
*/
#define FARTTY_INTERRACT_CLIP_GETDATA               'g'

/** Gets ID of current clipboard data of specified format.
 OPTIONAL: can be used only if server reported FARTTY_FEATCLIP_DATA_ID as supported feature
 In:
  uint32_t (format ID)
 Out:
  uint64_t (0 - failure, nonzero value - clipboard data ID)
*/
#define FARTTY_INTERRACT_CLIP_GETDATAID             'i'

/** Registers arbitrary clipboard data format.
 In:
  string (format name to be registered)
 Out:
  uint32_t (0 - failure, nonzero value - registered format ID)
*/
#define FARTTY_INTERRACT_CLIP_REGISTER_FORMAT       'r'

///////////////////////

/** Client may specify this wanted extra feature if it supports compact input events.
 If server supports this feature it may send such events, however client should be ready to
 receive not-compact events as well.
*/
#define FARTTY_FEAT_COMPACT_INPUT             0x00000001

/** Server reports this on respoce of FARTTY_INTERRACT_CLIP_OPEN if it supports clipboard data ID.
 Clipboard data ID allows client-side caching of clipboard data and avoid known data transfers.
*/
#define FARTTY_FEATCLIP_DATA_ID               0x00000001

/** Server reports this on respoce of FARTTY_INTERRACT_CLIP_OPEN if it supports chunked clipboard data set.
 This feature allows client to implement background and cancellable clipboard copy.
*/
#define FARTTY_FEATCLIP_CHUNKED_SET           0x00000002

///////////////////////
/**
FARTTY_INPUT_* notifications are send from server to client to inform about specific event happened.
Notification looks as "\x1b_f2l:"BASE64-encoded-arguments-stack"\x07"
For details of arguments stack encoding see utils/StackSerializer.h and utils/StackSerializer.cpp.
Unlike FARTTY_INTERRACT_* there is no ID and no replies are expected from client to server,
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

/** Server sends this to inform about keydown or keyup event if compact input is enabled and possible. See KEY_EVENT_RECORD for details.
  Note that event doesn't specify wVirtualScanCode and wRepeatCount thus client must assume wVirtualScanCode = 0 and wRepeatCount = 1
  uint16_t (UTF32 code fit to uint16_t)
  uint16_t (dwControlKeyState fit to uint16_t)
  uint8_t (wVirtualKeyCode fit to single-byte)
*/
#define FARTTY_INPUT_KEYDOWN_COMPACT_CHAR     'C'
#define FARTTY_INPUT_KEYUP_COMPACT_CHAR       'c'

/** Server sends this to inform about keydown or keyup event if compact input is enabled and possible. See KEY_EVENT_RECORD for details.
  Note that event doesn't specify wVirtualScanCode and wRepeatCount thus client must assume wVirtualScanCode = 0 and wRepeatCount = 1
  uint32_t (UTF32 code)
  uint32_t (dwControlKeyState)
  uint16_t (wVirtualKeyCode)
*/
#define FARTTY_INPUT_KEYDOWN_COMPACT_WIDE     'W'
#define FARTTY_INPUT_KEYUP_COMPACT_WIDE       'w'

