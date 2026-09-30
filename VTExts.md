# far2l terminal extensions

## 1. What this is

far2l can run in two ways: as a native GUI application (wx or SDL) or as a console application inside some terminal (the "TTY backend", `far2l --tty`). Inside an ordinary terminal far2l has only what escape sequences give it, which means no real clipboard, no F-key titles, no images beyond what the terminal's own graphics protocol offers, and so on.

The **far2l terminal extensions** are a small private protocol that lets a terminal application talk to the terminal it runs in, in both directions, through APC escape sequences (`ESC _ ... BEL`). Terminals that do not implement APC strings ignore them, so a program can probe for the extensions safely (a few old terminals print what they do not know; far2l prints a short hint and erases it after the probe). The protocol is defined by the source code of far2l; its C header is [`WinPort/FarTTY.h`](WinPort/FarTTY.h) (public domain) and the image constants live in [`WinPort/WinCompat.h`](WinPort/WinCompat.h).

The extensions cover:

- a clipboard with authorization, several formats per clipboard including application-defined ones, caching and chunked uploads (section 5.7);
- desktop notifications (5.4);
- the maximum possible window size, window maximize/restore (5.2, 5.3);
- titles of the F-keys (for example for the Touch Bar of a Mac) (5.5);
- images (5.8);
- a color depth query (5.6);
- richer keyboard and mouse events than plain escape sequences can carry (section 6);
- an "ad-hoc quick edit" request, cursor height and in-band terminal size (5.1, 5.2).

The protocol is also how far2l talks to itself: far2l running in TTY mode inside the far2l built-in terminal (Ctrl+O, and NetRocks remote shells) uses exactly these sequences. This document is written from the source code (as of elfmz/far2l `master`, see the list of sources in section 9), including its quirks, and "far2l" below means what the code does, not what the comments in the header say. Where the two disagree this is stated explicitly.

## 4. Protocol basics

### 4.1. Roles

The header `FarTTY.h` names the two sides *client* and *server*, and this document follows it:

| Side | Who it is | In far2l |
| ---- | --------- | -------- |
| **Client** | The application that draws its UI on the terminal and wants the extras. It sends *requests* and receives *replies* and *events*. | The TTY backend: `WinPort/src/Backend/TTY/` |
| **Server** | The terminal (emulator, terminal multiplexer, terminal widget). It executes requests, answers them and sends input events to the client. | The far2l built-in terminal: `far2l/src/vt/VTFar2lExtensios.cpp` and `far2l/src/vt/vtshell.cpp` |

Do not confuse "server" with a network server: the terminal may be a GUI application, and the client may run on a remote host behind SSH. Other texts use the names the other way round: the first description of the protocol, in KiTTY issue 74 (section 9), calls the application the *server* and the terminal the *client*. The roles in this document are those of `FarTTY.h`.

Some implementations play only one role (a terminal emulator is a server; a text-mode application is a client), some play both (see section 3).

### 4.2. Framing

Everything is an APC string: `ESC _` (`0x1B 0x5F`), a body, and a terminator, which is either `BEL` (`0x07`) or ST (`ESC \`, `0x1B 0x5C`). Only the 7-bit forms are recognized; the 8-bit C1 forms (`0x9F`, `0x9C`) are not. Both far2l sides accept either terminator on input. What far2l emits itself is shown below.

| Direction | Bytes | Emitted by far2l with |
| --------- | ----- | --------------------- |
| client to server: enable | `ESC _ far2l1 ST` | ST (GNU screen understands only ST) |
| server to client: acknowledge | `ESC _ far2lok BEL` | BEL |
| client to server: disable | `ESC _ far2l0 BEL` | BEL, no reply |
| client to server: host identity (optional) | `ESC _ far2l#<text> BEL` | BEL, no reply (see 4.6) |
| client to server: **request** | `ESC _ far2l:<base64> BEL` | BEL |
| server to client: **reply** | `ESC _ far2l<base64> BEL` | BEL |
| server to client: **event** | `ESC _ f2l<base64> BEL` | BEL |

Note the difference in the prefixes, which is easy to get wrong and is what the code really does:

- a request has a colon after `far2l`: `ESC _ far2l:` followed by Base64;
- a reply has **no** colon: `ESC _ far2l` followed directly by Base64;
- an event has **no** colon either: `ESC _ f2l` followed directly by Base64.

The client recognizes an incoming APC body by its first characters: `f2l` (an event), then `far2l` (a reply). The Base64 decoder used by far2l stops at the first character that is not part of the Base64 alphabet, so a stray colon in `f2l:...` or `far2l:...` would make the whole payload decode as empty. The comment in `FarTTY.h` that shows events as `"\x1b_f2l:"BASE64` is wrong; replies, which it does not show at all, follow the same rule.

The server recognizes a body that begins with `far2l`, and then looks at the next character: `1`, `0`, `:` and `#` are the four forms above. `far2l_` is used internally by far2l for shell integration markers and is not part of the extensions protocol.

A terminal that does not know the extensions should ignore all of this as unknown APC strings.

### 4.3. The stack serializer

The payload of requests, replies and events is a binary **stack**, Base64-encoded as a whole. The encoding is implemented in `utils/include/StackSerializer.h`, `utils/src/StackSerializer.cpp` and `utils/src/base64.cpp`.

- The stack is a byte array. **Push** appends to the end; **pop** removes from the end. The serialized form is the array from its first byte (the bottom of the stack) to its last byte (the top).
- Consequently the arguments must be pushed **in the reverse order** of how the receiver pops them. All tables in this document list arguments in **pop order: top of the stack first**, as `FarTTY.h` does. The byte that comes first on the wire is the one popped last.
- Integers (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `uint64_t`) are fixed-width and **little-endian**. A `bool` is one byte; a `char` is one byte.
- A **string** is pushed as its raw bytes, followed by its length as a `uint32_t` (so the length is on top and is popped first, then the bytes). There is no terminator and no padding. far2l uses UTF-8.
- **Raw data** (image pixels, clipboard bytes) is pushed as is, without a length. Its size is given by other arguments.
- Base64 is the standard alphabet (`A-Z a-z 0-9 + /`), with `=` padding on output. far2l's decoder stops at the first `=` or at the first character outside the alphabet and does not require padding; it does not accept line breaks or whitespace.
- An empty stack encodes to an empty string.
- Popping more bytes than the stack holds is an error. What the error does is different on each side, see section 8.

### 4.4. Requests, request IDs and replies

The top byte of a request stack is an 8-bit **request ID**. Below it is the command letter (`FARTTY_INTERACT_*`) and then the command arguments. In other words, a client builds a request by pushing the arguments (last argument first), then the command letter, then the ID.

- **ID = 0**: the client does not want a reply. The server sends nothing.
- **ID != 0**: the server replies, after executing the request, with `ESC _ far2l<base64> BEL`. The top of the reply stack is the same ID, and below it are the return values of the command (if any), in pop order as listed for each command.

far2l's own server replies to **every** request with a non-zero ID: for commands that have no return values the reply consists of the ID only, and for unknown commands and for commands that failed with an exception it is also the ID only (the client then fails to pop the values it expected).

The far2l client **waits for the reply without any timeout**. A server that silently ignores a request with a non-zero ID makes the client hang, unless the connection breaks. So a server implementation has to answer every non-zero ID, even if it does not know the command; an empty reply (the ID alone) is the proper way to say "unsupported".

The far2l client allocates IDs from an 8-bit counter, skipping zero and IDs that are still in flight; it allows at most 255 requests in flight. Replies with unknown IDs are ignored.

Which far2l requests are sent with a non-zero ID (that is, need an answer) is stated for each command below.

### 4.5. Activation and detection

1. The client sends `ESC _ far2l1 ESC \`.
2. A server that supports the extensions answers `ESC _ far2lok BEL`. The far2l client searches for exactly these bytes, **with the BEL terminator**; an acknowledgement ended with ST is not recognized.
3. From now on the server accepts requests. Requests that arrive before activation, or after `far2l0`, are ignored without any reply.

To avoid waiting for a timeout in terminals that do not know the extensions, the client follows the activation string with an ordinary query that every terminal answers, the device status report `ESC [ 5 n`, to which a terminal replies `ESC [ 0 n`. The answer to the DSR marks the end of the probe: if `far2lok` was not received before it, the extensions are not supported. Therefore **the acknowledgement has to be sent before the reply to `ESC [ 5 n`** (a terminal that handles its input in order does this naturally).

This is exactly what far2l does (in `WinPort/src/Backend/TTY/TTYCaps.cpp`): it writes `ESC _ far2l1 ESC \`, a human-readable hint ("Press <ENTER> if tired of watching this message"), `ESC E`, a test of VS16 emoji width with `ESC [ 6 n`, and `ESC [ 5 n`; then it reads the answers, waiting up to 10 seconds for each next byte. The probe is skipped if far2l runs on the Linux/BSD kernel console, or if detection was disabled with `--nodetect` (`--nodetect=f` disables only this one).

Repeated `far2l1` is harmless: the server answers with `far2lok` again and keeps its state. The client sends `ESC _ far2l0 BEL` when it exits, is suspended (`SIGTSTP`) or hangs up; it probes again after resuming.

When the server receives `far2l0` it drops the extension state: the clipboard is closed if it was open, F-key titles are cleared, negotiated features are forgotten.

### 4.6. Host identity (`far2l#`)

`ESC _ far2l#<text> BEL` passes a string that identifies the remote host to the server. It has to be sent **before** `far2l1`. far2l NetRocks sends `user@host` (control characters replaced by a space) when it runs a remote shell, before the remote command starts. The server keeps it for the lifetime of the command and ignores later values if one was already set, so the remote command cannot replace it; and uses it as a prefix of the clipboard client ID (see 5.7.1), so that a remote host cannot impersonate a client of another host even if it knows that client's clipboard passcode. The far2l TTY backend itself never sends it. The text must not contain BEL or ESC.

## 5. Requests (client to server)

Every request is sent as described in 4.4: the stack has the request ID on top, then the command letter, then the arguments. The tables list the arguments and the return values in **pop order (top first)**, after the ID and the command letter have been removed.

| Letter | Name | Reply needed by far2l client | Section |
| ------ | ---- | ---------------------------- | ------- |
| `x` | `FARTTY_INTERACT_CHOOSE_EXTRA_FEATURES` | no | 5.1 |
| `e` | `FARTTY_INTERACT_CONSOLE_ADHOC_QEDIT` | no | 5.2 |
| `M` | `FARTTY_INTERACT_WINDOW_MAXIMIZE` | no | 5.2 |
| `m` | `FARTTY_INTERACT_WINDOW_RESTORE` | no | 5.2 |
| `h` | `FARTTY_INTERACT_SET_CURSOR_HEIGHT` | no | 5.2 |
| `w` | `FARTTY_INTERACT_GET_WINDOW_MAXSIZE` | yes | 5.3 |
| `n` | `FARTTY_INTERACT_DESKTOP_NOTIFICATION` | no | 5.4 |
| `f` | `FARTTY_INTERACT_SET_FKEY_TITLES` | first time only | 5.5 |
| `p` | `FARTTY_INTERACT_GET_COLOR_PALETTE` | yes | 5.6 |
| `c` | `FARTTY_INTERACT_CLIPBOARD` (sub-commands) | depends | 5.7 |
| `i` | `FARTTY_INTERACT_IMAGE` (sub-commands) | yes | 5.8 |

### 5.1. `FARTTY_INTERACT_CHOOSE_EXTRA_FEATURES` (`x`)

Declares the optional features the client can handle. The client sends it once, right after the extensions are activated, with ID 0.

| In | Type | Description |
| -- | ---- | ----------- |
| Feature flags | `uint64_t` | A combination of `FARTTY_FEAT_*`. |

No return values.

| Flag | Value | Meaning |
| ---- | ----- | ------- |
| `FARTTY_FEAT_COMPACT_INPUT` | `0x00000001` | The client understands compact keyboard and mouse events (`C`, `c`, `m`, see section 6). |
| `FARTTY_FEAT_TERMINAL_SIZE` | `0x00000002` | The client wants in-band terminal size events (`S`), in addition to or instead of `SIGWINCH`. |

far2l always asks for `FARTTY_FEAT_COMPACT_INPUT`, and adds `FARTTY_FEAT_TERMINAL_SIZE` only when its standard output is **not** a TTY (that is, when it cannot ask the kernel for the size of the terminal). A far2l server **replaces** its set of features by every request rather than adding to it, and, if `FARTTY_FEAT_TERMINAL_SIZE` is set, immediately sends the current size as an event. The server may ignore features it does not know; events that the client has not asked for must not be sent.

Features of the other direction (clipboard) are reported by the server in the reply to `CLIP_OPEN`, see 5.7.

### 5.2. Window, cursor and quick edit

| Letter | Name | In | Effect on far2l server |
| ------ | ---- | -- | ---------------------- |
| `e` | `CONSOLE_ADHOC_QEDIT` | none | Turns the left mouse button press that is in progress into a selection of text on the screen, as if the quick edit mode were on: the selection starts at the position of the last mouse click and is copied to the clipboard the usual way. It is ignored if the middle or right button is held, if the left button is not held, or if such a selection is already going on. |
| `M` | `WINDOW_MAXIMIZE` | none | Maximizes the window. |
| `m` | `WINDOW_RESTORE` | none | Restores the window from the maximized state. |
| `h` | `SET_CURSOR_HEIGHT` | `uint8_t` height, 0 to 100 (percent) | Sets the height of the cursor as a percentage of the cell height. |

None of them has return values and far2l sends all of them with ID 0. far2l sends `e` for Shift+click in its viewer and editor, and for a click on its command line that the command line itself does not use. far2l sends `h` instead of the usual `ESC [ n q` (DECSCUSR) when it knows it talks to a far2l server, at start and whenever the cursor height changes. `M` and `m` are sent by the "toggle window size" command (Alt+F9): far2l compares its current size with the result of `w` and maximizes if they differ, or restores otherwise.

### 5.3. `FARTTY_INTERACT_GET_WINDOW_MAXSIZE` (`w`)

Gets the maximum possible size of the window, in character cells.

| In | none |
| -- | ---- |

| Out | Type | Description |
| --- | ---- | ----------- |
| Height | `int16_t` | Maximum number of rows. |
| Width | `int16_t` | Maximum number of columns. |

The far2l client sends it with a non-zero ID and **caches the first successful answer for the life of the process**. If the terminal can be resized (for example to another monitor) the cached value goes stale.

### 5.4. `FARTTY_INTERACT_DESKTOP_NOTIFICATION` (`n`)

Shows a desktop notification.

| In | Type | Description |
| -- | ---- | ----------- |
| Title | `string` | Notification title, UTF-8. |
| Text | `string` | Notification body, UTF-8. |

No return values. The far2l client sends it with ID 0. far2l raises notifications when a file operation or a command run in the terminal completes, and in a few other places (for example when a search is finished); which ones is configurable in the notification options. By default notifications are shown only if the far2l window is not the active one; to know that, far2l enables focus reports (`ESC [ ? 1004 h`) and tracks `ESC [ I` and `ESC [ O`. A server that cannot show notifications should just ignore the request. If the extensions are not available, the client runs its own `notify.sh` helper on the local machine when it finds X11 or Wayland.

For a text-mode program, and especially for one that runs on a remote host, this is a way to reach the desktop of the user without anything installed on the remote side.

### 5.5. `FARTTY_INTERACT_SET_FKEY_TITLES` (`f`)

Sets the titles of the F-keys, for hosts that have a place for them (the Touch Bar of a Mac).

There are twelve entries, for F1 to F12, **popped in this order: F1 first**. The client pushes F12 first. Each entry is:

| Entry part | Type | Description |
| ---------- | ---- | ----------- |
| State | `uint8_t` | `0`: this key has no title (clear it). Non-zero: a string follows. |
| Title | `string` | Only when State is non-zero. The title, UTF-8. |

The state is on top of the title. A server must stop when it runs out of data: fewer than twelve entries are allowed and the missing ones mean "no title" (far2l's server does exactly this). Passing no titles at all clears all of them.

| Out | Type | Description |
| --- | ---- | ----------- |
| Success | `bool` (1 byte) | `1` if the host can show the titles, `0` if not. |

The far2l client sends the **first** request with a non-zero ID to find out whether the host supports the feature. If the answer is `0`, or the stack is broken, it never tries again. After a positive answer all subsequent requests are sent with ID 0 and the server is not asked about it any more. That means a server that can show titles only sometimes cannot report "not now" after the first positive answer.

far2l sends the titles of the key bar as it is currently shown, so they follow the state of the modifier keys: when Shift, Ctrl or Alt is pressed, the titles of the corresponding key bar are sent, and they are sent only when they have changed since the last request. A cleared entry is an entry for which the key bar has no label.

### 5.6. `FARTTY_INTERACT_GET_COLOR_PALETTE` (`p`)

Asks how many colors the terminal can show.

| In | none |
| -- | ---- |

| Out | Type | Description |
| --- | ---- | ----------- |
| Color bits | `uint8_t` | The maximum supported color resolution in bits: `4` (16 colors), `8` (256 colors), `24` (true color). |
| Reserved | `uint8_t` | Zero. A client has to ignore it. |

The far2l client sends it with a non-zero ID, and asks every time it needs the answer; if `--norgb` is given or GNU screen is detected (`TERM=screen*`) it answers `4` by itself without asking. When the extensions are not available, far2l guesses from `COLORTERM` (`truecolor` or `24bit`: 24 bits) and `TERM` (containing `256`: 8 bits), and otherwise assumes 4 bits.

### 5.7. `FARTTY_INTERACT_CLIPBOARD` (`c`)

All clipboard operations are one command, `c`, whose first argument is a sub-command letter. After the ID, the command letter `c` and the sub-command letter, the arguments follow.

| Sub-command | Name | In | Out | far2l client waits |
| ----------- | ---- | -- | --- | ------------------ |
| `o` | `CLIP_OPEN` | `string` client ID | `int8_t` status; `uint64_t` server features | yes |
| `c` | `CLIP_CLOSE` | none | `int8_t` status | no (ID 0) |
| `e` | `CLIP_EMPTY` | none | `int8_t` status | no (ID 0) |
| `a` | `CLIP_ISAVAIL` | `uint32_t` format | `int8_t` available | yes |
| `S` | `CLIP_SETDATACHUNK` | `uint16_t` size >> 8; raw data | none | only every 16th chunk |
| `s` | `CLIP_SETDATA` | `uint32_t` format; `uint32_t` size; raw data | `int8_t` status; `uint64_t` data ID | yes |
| `g` | `CLIP_GETDATA` | `uint32_t` format | `uint32_t` size; raw data; `uint64_t` data ID | yes |
| `i` | `CLIP_GETDATAID` | `uint32_t` format | `uint64_t` data ID | yes |
| `r` | `CLIP_REGISTER_FORMAT` | `string` format name | `uint32_t` format ID | yes |

The "Out" column is in pop order. Conditions under which a value is present are described below. `FarTTY.h` describes `CLIP_GETDATAID` in its overview as returning a status and an ID; the code returns the ID alone.

Status values (`int8_t`): `1` success, `0` failure, `-1` the clipboard is not open (for `CLIP_OPEN`: access denied, use your own clipboard).

#### 5.7.1. Authorization

`CLIP_OPEN` authorizes the client and opens the clipboard for the operations that follow. The client identifies itself by a **client ID** (the *passcode* of `FarTTY.h`): a string of 32 to 256 characters, each one of `0`-`9`, `a`-`z`, `-`, `_`. A server that gets anything else answers `0`.

The far2l client generates its ID once and keeps it in the file `tty_clipboard/me` of its configuration directory (by default `~/.config/far2l/`; 64 characters: the host name, a dash, and random lowercase letters and digits; other characters of the host name are replaced by `_`).

What the far2l server does with it:

1. The server adds the host identity (4.6) and a colon in front of the ID, if a host identity is set.
2. If the result is in its memory of already authorized clients, or in the file `tty_clipboard/autheds` of the configuration directory of the server (one ID per line), it opens the clipboard.
3. Otherwise it stops and asks the user with a dialog "Clipboard access: Please choose how this terminal application may use clipboard", with four answers, which the far2l server maps to statuses as follows. The dialog ignores the first ("...") item so that an accidental press of Enter, Space or Esc does not choose anything.

| Answer | Reply status | Meaning |
| ------ | ------------ | ------- |
| Block attempt | `0` | The open fails, as if the clipboard could not be opened. The client may try again. |
| Remote clipboard | `-1` | The client must use its own clipboard. The far2l client then switches for the rest of the process to a file based clipboard kept on its own side. |
| Share clipboard | `1` | Access is allowed for as long as the server keeps the activation (until `far2l0` or the end of the session). |
| Share clipboard always | `1` | The same, and the ID is written to `autheds`, so the question is not asked again. |

The `CLIP_OPEN` reply can therefore come after a long time; the client waits for it. On success the stack of the reply carries a `uint64_t` bitmask of server features, **below** the status. The far2l server always sends it, even when the status is not `1`; the far2l client reads it only when the status is `1`, and treats its absence as zero.

| Server feature | Value | Meaning |
| -------------- | ----- | ------- |
| `FARTTY_FEATCLIP_DATA_ID` | `0x00000001` | The server can tell an ID of the data of a format (`CLIP_GETDATAID`, and IDs in `CLIP_SETDATA` and `CLIP_GETDATA` replies), which allows client-side caching. |
| `FARTTY_FEATCLIP_CHUNKED_SET` | `0x00000002` | The server accepts `CLIP_SETDATACHUNK`. |

far2l nests opens. The server counts the opens that succeeded (every `CLIP_CLOSE` undoes one, and leaving the extensions closes all that remain). The far2l client sends one `CLIP_OPEN` when the application opens the clipboard, however deep the nesting, and sends `CLIP_CLOSE` (with ID 0) after the last close. A client that opens the clipboard twice on the wire has to close it twice.

Not every sub-command needs an opened clipboard: `CLIP_ISAVAIL` and `CLIP_REGISTER_FORMAT` work without it, and `CLIP_ISAVAIL` does not require authorization. `CLIP_EMPTY`, `CLIP_SETDATA`, `CLIP_GETDATA` and `CLIP_GETDATAID` need an open clipboard (the first two answer `-1` if it is not open, `CLIP_GETDATA` answers `0xFFFFFFFF` and `CLIP_GETDATAID` answers `0`).

#### 5.7.2. Safe reading

A terminal application that asks the terminal for the clipboard contents can steal whatever the user copied last, for instance a password, without the user noticing. OSC 52 has a read form (`ESC ] 52 ; c ; ? BEL`) which works that way, and this is why many terminals that implement OSC 52 disable reading or ask every time. far2l's terminal ignores the OSC 52 read request altogether.

The far2l extensions protect reading by two independent conditions:

1. The client has to be authorized by the user (5.7.1). Authorization is bound to the client ID, and, for NetRocks sessions, to the host identity.
2. The server returns the clipboard contents **only shortly after the user's own "paste" gesture**. In the far2l server a read is allowed for 5 seconds after any of these inputs, which it sees on their way to the application:
   - the key press Ctrl+V (with or without Shift, without Alt) or Shift+Insert (without Ctrl and Alt);
   - any mouse event during which the middle button is held (the usual "paste the selection" gesture on X11). Every such event restarts the period.

   Each `CLIP_GETDATA` made while reading is allowed restarts the five seconds once more, at most 3 times (the limit is reset by every paste gesture and by `CLIP_CLOSE`), so that an application that needs several formats or several transfers for one paste can get them. `CLIP_GETDATAID` needs the gesture but does not extend it.

When the gesture is missing, `CLIP_GETDATA` answers `0` bytes and `CLIP_GETDATAID` answers `0`, exactly as if the clipboard had been empty, and no status tells the application that it was refused. A client has to treat them as "no data". `CLIP_ISAVAIL` does not check for the gesture. Note that while the extensions are active the far2l server does not paste anything by itself when the user presses Ctrl+V or Shift+Insert: it delivers the key press to the application as an event, and the application turns it into a paste by reading the clipboard within the allowed time.

Writing is not covered by the gesture rule: once a client has been authorized it may set the clipboard at any time.

#### 5.7.3. Writing

```
CLIP_OPEN(passcode)                      -> status, features
CLIP_EMPTY                                  (the far2l client does not wait)
CLIP_SETDATA(format, size, data)         -> status, [data ID]
   ... more CLIP_SETDATA for other formats ...
CLIP_CLOSE                                  (the far2l client does not wait)
```

`CLIP_SETDATA` puts the data of one format on the clipboard. The far2l server forwards it to the clipboard of its host with the semantics of the Win32 `SetClipboardData`: the data of other formats stay, and that is how several formats are put on the clipboard at the same time (far2l applications call `CLIP_EMPTY` first). `CLIP_EMPTY` removes all formats. The reply to `CLIP_SETDATA` holds a `uint64_t` data ID **below the status, and only if the status is `1`** (see 5.7.5); a far2l client reads it if the server reported `FARTTY_FEATCLIP_DATA_ID`.

To avoid blocking the user interface, the far2l client sets the clipboard in a separate thread, so the application is not delayed by the transfer. While the transfer is in progress, `CLIP_ISAVAIL` and `CLIP_GETDATA` are answered by the client itself from the pending data (a request for a format other than the pending one is answered "not available"), and a newer set of the same format or an empty request cancels the transfer; setting a different format waits for the transfer in progress to finish.

**Chunked upload.** If the server reported `FARTTY_FEATCLIP_CHUNKED_SET` and the data is longer than 0x4000 bytes, the far2l client first sends the data by pieces of 0x4000 bytes with `CLIP_SETDATACHUNK`, and ends with one `CLIP_SETDATA` which carries the rest, so that the last piece is between 1 and 0x4000 bytes long.

| CLIP_SETDATACHUNK In | Type | Description |
| -------------------- | ---- | ----------- |
| Encoded size | `uint16_t` | The size of the chunk divided by 256. The chunk size is always a multiple of 256, and is this value shifted left by 8 bits (so at most 0xFFFF00 bytes). The far2l client always sends `0x0040`. **Zero** means: throw away all chunks that were collected so far. |
| Data | raw | The chunk. |

The server accumulates the chunks in order, and the next `CLIP_SETDATA` **prepends** them to its own data (`size` of that request does not include them). Chunks are accepted only while the clipboard is open, and every `CLIP_OPEN`, `CLIP_CLOSE` and `CLIP_SETDATA` drops the ones not yet consumed. `CLIP_SETDATACHUNK` has no return values, and the far2l client sends its chunks without waiting for replies: only every 16th chunk is sent with a non-zero ID and waited for, to keep the transmission from running too far ahead of the terminal. **A server must therefore answer such a chunk with an empty reply (the ID alone).** The client also sleeps for an eighth of the time it was transmitting when it was busy for more than 256 ms, to leave the bandwidth for other traffic, and when the application cancels the transfer, sends a chunk with size zero, stops and does not send `CLIP_SETDATA`.

When the server has not reported `FARTTY_FEATCLIP_CHUNKED_SET`, the whole data is sent in one `CLIP_SETDATA`, which can be very large.

#### 5.7.4. Reading

```
CLIP_OPEN(passcode)                      -> status, features
CLIP_ISAVAIL(format)                     -> available            (optional, needs no open)
CLIP_GETDATAID(format)                   -> data ID              (optional, only with DATA_ID)
CLIP_GETDATA(format)                     -> size, data, [data ID]
CLIP_CLOSE                                  (the far2l client does not wait)
```

`CLIP_GETDATA` returns the whole data in one reply; there is no chunked download. The reply stack, in pop order, is: `uint32_t` size, then `size` bytes of data, then, if the server always sends it, a `uint64_t` data ID. The size is `0` if there is no data of this format, if the read was not allowed (5.7.2), or if the operation failed; it is `0xFFFFFFFF` if the clipboard was not open. The far2l client pops the data ID only if the server reported `FARTTY_FEATCLIP_DATA_ID`. The far2l server always pushes the ID, also when the size is 0 (the ID is then 0).

#### 5.7.5. Data IDs and caching

A **data ID** is a 64-bit number that identifies the current data of a format, so that a client can check whether the clipboard still has what it already has, without transferring it again. Zero always means "no data" or "failure". The client **never computes** IDs: it stores what the server gave it in `CLIP_SETDATA` and `CLIP_GETDATA` replies and compares it to what `CLIP_GETDATAID` returns. The client has a cache of one entry per format. When the client wants the data and has it cached, it asks `CLIP_GETDATAID`; if the ID is the same, the cached data is used, if it differs, the cache entry is dropped and `CLIP_GETDATA` is sent; if the ID is 0 the client concludes that there is no data. The cache is also dropped by `CLIP_EMPTY` and by a `CLIP_ISAVAIL` that answers "not available".

The far2l server calculates the ID as a CRC-64 (`utils/src/crc64.c`) of the data seeded with its length, where the length is the length of the data for binary formats, and for text formats (`CF_TEXT`, `CF_UNICODETEXT`) only up to the first NUL (byte or `wchar_t` respectively). If the result is 0 it uses 1. Another server can use any hash, as long as it is stable and never 0.

#### 5.7.6. Formats

A format is a `uint32_t`. The Windows numbers are used for the standard formats:

| ID | Name | Content |
| -- | ---- | ------- |
| 1 | `CF_TEXT` | Text in UTF-8. The far2l client sends it without a terminating NUL and tolerates one when it receives it. |
| 13 | `CF_UNICODETEXT` | Text in UTF-32 (little-endian). Deprecated: it takes up to four times more space than UTF-8. |
| 15 | `CF_HTML` | HTML text. The far2l application puts HTML together with the plain text when it copies formatted text, and includes the terminating NUL. |

`CF_TEXT` is the format every server has to support. The far2l client **always transfers text as `CF_TEXT`**: if the application asks for or sets `CF_UNICODETEXT`, the client converts it from and to UTF-8 by itself, to save the traffic, including `CLIP_ISAVAIL`, which is sent for `CF_TEXT`. Only if a `CLIP_GETDATA` for `CF_TEXT` returns nothing, it tries once more with the original `CF_UNICODETEXT`. A server with a 16-bit `wchar_t` that stores `CF_UNICODETEXT` as UTF-16 has to convert it to and from UTF-32 at the protocol boundary, as the far2l server does in that case. Other `CF_*` numbers of Windows exist in the header, but far2l does not use them.

**Custom formats.** A client can register its own format with `CLIP_REGISTER_FORMAT`: it passes a name (a string, UTF-8) and gets a format ID that can be used in the commands above, or `0` if the registration failed. The IDs are allocated in the range `0xC000`-`0xFFFF`, the range that Windows uses for registered formats; in the clipboard backends of far2l the same name gets the same ID. To the protocol such data is an opaque block of bytes. The far2l client remembers the IDs that it has received, for the lifetime of the process. Registration needs neither an open clipboard nor authorization.

far2l itself uses this for **vertical block copy** (rectangular selection in the editor): the application puts the text as `CF_UNICODETEXT` and, in addition, an entry of the format named `FAR_VerticalBlock_Unicode` with 4 zero bytes of data. The presence of that format next to the text tells the paste side that the text is a vertical block. A client can do the same with its own formats, for instance for a richer clipboard of its own kind, and the server keeps them together with the standard ones for as long as the user does not overwrite them.

