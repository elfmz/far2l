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

## 2. What it gives over the existing solutions

A terminal application can use the escape sequences that other terminals provide: OSC 52 for the clipboard, OSC 9, OSC 99 and OSC 777 for notifications, sixel and the Kitty graphics protocol for pictures, the Kitty keyboard protocol and the `win32-input-mode` for keys. The far2l extensions are one uniform channel that covers all of these and several things that have no standard equivalent, with requests that get replies. Specifically:

1. **Safe clipboard reading.** OSC 52 can ask the terminal for the clipboard (`ESC ] 52 ; c ; ?`), which lets any program that can write to the terminal, including a remote one or one that has just `cat`-ed a hostile file, read what the user copied. Because of that most terminals turn it off or ask every time, and far2l's terminal ignores it. The far2l clipboard requires that the client be authorized by the user and hands out data only within a few seconds after the user's own paste gesture (Ctrl+V, Shift+Insert, middle button), so a paste works without a question and a silent theft does not (5.7.1, 5.7.2).
2. **Desktop notifications** through one request with a title and a text in UTF-8, the same in a local terminal and over SSH; the other conventions (OSC 9, OSC 777, OSC 99) are mutually incompatible and differently supported (5.4).
3. **Ad-hoc copy to the clipboard on demand.** An application that reads the mouse can ask the terminal to turn the mouse press that is in progress into an ordinary text selection, which is then copied to the clipboard. The application decides by itself on the gesture (far2l does it for Shift+click in its viewer and editor, and for a click on its command line), instead of depending on the modifier that a particular terminal uses to bypass the mouse reporting of applications (commonly Shift) (5.2).
4. **The maximum possible size of the window**, which is what the application needs to offer "maximize" (Alt+F9 in far2l), plus requests to maximize and restore the window (5.2, 5.3).
5. **Titles of the F-keys** for hosts that have a place for them: far2l sends the labels of its key bar for the current state of Shift, Ctrl and Alt, and they are shown on the Touch Bar of a Mac (5.5).
6. **Images** that are simpler than sixel and lighter than the Kitty graphics protocol. In contrast to sixel, which paints a palette-based bitmap into the screen contents, a far2l image has a name chosen by the application, is true color (RGB, RGBA) or an ordinary PNG or JPEG file, and can be moved, mirrored, rotated, extended and scrolled on the terminal side and deleted by its name. The whole protocol is four sub-commands; it has no shared-memory transport, no chunked transmission, no animation and no layers, so the protocol stays small (5.8).
7. **Several clipboard formats at the same time, including the application's own.** OSC 52 carries text. Here the clipboard is a set of formats: plain text, HTML, and formats that an application registers by name, such as the format far2l uses to keep the fact that a text is a vertical block. In addition, the client can cache the data by an ID and skip a repeated transfer, and large data can be sent in pieces that are cancelable and do not block the interface (5.7.3, 5.7.5, 5.7.6).
8. **Complete input events.** Key presses and releases with the virtual key code, scan code, the exact state of the modifiers (left and right Ctrl and Alt, lock keys), and mouse events with five buttons, both wheels and the double-click flag, in the form that far2l uses internally, optionally in a compact form. Besides that, terminals that run the application over a pipe can tell it the terminal size (section 6).
9. **Questions to the terminal** with answers: how many colors it can show (4, 8 or 24 bits; instead of guessing from `TERM` and `COLORTERM`), its cell size in pixels, the cursor height. Because the answer comes from the terminal and not from the environment of the application, it stays correct over SSH and in nested terminals (5.6, 5.8.1).
10. **Safe to probe.** A terminal that does not know the extensions ignores the APC strings (or, at worst, prints them, which far2l erases), and one additional standard query (`ESC [ 5 n`) tells the application at once that there is nobody to answer, so no timeout has to be waited for (4.5).

All of this is optional and piecemeal: a terminal can implement the clipboard and the keyboard and leave the rest, and the application keeps working.

## 3. Implementations

| Implementation | Role | What it has |
| -------------- | ---- | ----------- |
| [far2l](https://github.com/elfmz/far2l) | client and server (the reference) | Everything in this document. The client is the TTY backend (`far2l --tty`), the server is the built-in terminal (Ctrl+O and the remote shells of NetRocks). |
| [f4](https://github.com/unxed/f4) (with its libraries [vtui](https://github.com/unxed/vtui) and [vtinput](https://github.com/unxed/vtinput)) | client and server | Server: the built-in terminal; clipboard with chunked upload and an authorization dialog, images (RGB, RGBA), notifications, terminal size event, the drop proposal ([section 11](#11-related-proposal-drag-and-drop)). Client: all input events, the clipboard (text), images. |
| [Turbo Vision for C++ (tvision)](https://github.com/magiblot/tvision), and the applications built on it, such as [turbo](https://github.com/magiblot/turbo) | client | Key press and mouse events; clipboard text, get and set. |
| [putty4far2l](https://github.com/ivanshatsky/putty4far2l) (PuTTY 0.78) and [a second repository](https://github.com/unxed/putty4far2l) (its README says PuTTY 0.76; since December 2024 it holds the code of the first one) | server (Windows) | Key events; the clipboard, through the clipboard of Windows (text and registered formats); the color depth (24 bits); cursor height and notifications in the older fork. |
| [KiTTY](https://github.com/cyd01/KiTTY) (a PuTTY fork; the support is enabled by the `MOD_FAR2L` build option) | server (Windows) | Key events; the clipboard, through the clipboard of Windows (text and registered formats); the color depth (24 bits). |
| [thruPTY](https://github.com/Dazzar56/thruPTY) | server | The terminal part of f4 ported to a GPU-accelerated terminal: clipboard (text), window size, notifications, terminal size event; F-key titles and color depth are answered; images are refused. |

The roles are those of section 4.1: a *client* is an application that runs in the terminal, a *server* is the terminal. The links to the sources of each of them are in [section 9](#9-links-to-implementations); notes on them, their specifics and what to look out for when writing another one are in [section 10](#10-experience-of-other-implementations).

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

far2l's own server replies to **every** request with a non-zero ID: for commands that have no return values the reply consists of the ID only, and for an unknown top-level command and for a command that failed with an exception it is also the ID only (the client then fails to pop the values it expected). For an unknown clipboard or image sub-command the far2l server does not clear the stack, and the reply carries the unread arguments of the request under the ID.

The far2l client **waits for the reply without any timeout**. A server that silently ignores a request with a non-zero ID makes the client hang, unless the connection breaks. So a server implementation has to answer every non-zero ID, even if it does not know the command; an empty reply (the ID alone) is the proper way to say "unsupported".

The far2l client allocates IDs from an 8-bit counter, skipping zero and IDs that are still in flight; it allows at most 255 requests in flight. Replies with unknown IDs are ignored. Note that a late `ESC _ far2lok BEL` (for instance the answer to a repeated `far2l1`) has the prefix of a reply, and its two characters decode to a one-byte stack, that is an ID of `0xA2`; it is harmless unless a request with that ID is in flight.

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
| `e` | `CONSOLE_ADHOC_QEDIT` | none | Turns the left mouse button press that is in progress into a selection of text on the screen, as if the quick edit mode were on: the selection starts at the position of the last mouse click and is copied to the clipboard the usual way. The effect is that of the host backend; in the wx backend the request is ignored if a selection is already going on or the middle or right button is held, and if the left button is held, a release of the left button is queued for the application and the selection starts at the position of the last click. |
| `M` | `WINDOW_MAXIMIZE` | none | Maximizes the window. |
| `m` | `WINDOW_RESTORE` | none | Restores the window from the maximized state. |
| `h` | `SET_CURSOR_HEIGHT` | `uint8_t` height, 0 to 100 (percent) | Sets the height of the cursor as a percentage of the cell height. |

None of them has return values and far2l sends all of them with ID 0. far2l sends `e` for Shift+click in its viewer and editor, for a click on its command line that the command line itself does not use, and in a few other places. far2l sends `h` instead of the usual `ESC [ n q` (DECSCUSR) when it knows it talks to a far2l server, at start and whenever the cursor height changes. `M` and `m` are sent by the "toggle window size" command (Alt+F9): far2l compares its current size with the result of `w` and maximizes if they differ, or restores otherwise.

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

No return values. The far2l client sends it with ID 0. far2l raises notifications when a file operation or a command run in the terminal completes, and in a few other places (for example when a search is finished); which ones is configurable in the notification options. By default notifications are shown only if the far2l window is not the active one; to know that, far2l enables focus reports (`ESC [ ? 1004 h`) and tracks `ESC [ I` and `ESC [ O`. The far2l server applies the same "only if in background" option itself, so it may also decide not to show the notification. A server that cannot show notifications should just ignore the request. If the extensions are not available, the client runs its own `notify.sh` helper on the local machine when it finds X11 or Wayland.

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

The far2l client sends the **first** request with a non-zero ID to find out whether the host supports the feature. If the answer is `0`, or the stack is broken, it does not try again during this activation of the extensions (it asks anew after a resume). After a positive answer all subsequent requests are sent with ID 0 and the server is not asked about it any more. That means a server that can show titles only sometimes cannot report "not now" after the first positive answer.

far2l sends the titles of the key bar as it is currently shown, so they follow the state of the modifier keys: when Shift, Ctrl or Alt is pressed, the titles of the corresponding key bar are sent, and they are sent only when they have changed since the last request. far2l always sends all twelve entries with State `1`; a key that has no label gets an empty string. State `0` is sent only when the whole set is cleared (far2l has no titles to give, for example while an external command runs, and when the server drops the extensions).

### 5.6. `FARTTY_INTERACT_GET_COLOR_PALETTE` (`p`)

Asks how many colors the terminal can show.

| In | none |
| -- | ---- |

| Out | Type | Description |
| --- | ---- | ----------- |
| Color bits | `uint8_t` | The maximum supported color resolution in bits: `4` (16 colors), `8` (256 colors), `24` (true color). |
| Reserved | `uint8_t` | Zero. A client has to ignore it. |

The far2l client sends it with a non-zero ID, and asks every time it needs the answer; if `--norgb` is given or GNU screen is detected (`TERM=screen*`) it answers `4` by itself without asking. When the extensions are not available, far2l guesses: if `COLORTERM` is set, `truecolor` or `24bit` gives 24 bits and any other value 8 bits (`TERM` is then not looked at); otherwise a `TERM` that contains `256` gives 8 bits, and anything else 4.

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

The far2l server does not nest opens: its `OpenClipboard` fails while the clipboard is open (also when the far2l that hosts the terminal holds it itself), so a second `CLIP_OPEN` is answered with status `0`. The far2l client keeps a nesting counter on its own side only (its background set-data thread holds an open of its own) and sends exactly one `CLIP_OPEN` and, after the last close, one `CLIP_CLOSE` (with ID 0). A client should not open twice. Leaving the extensions closes a clipboard that is still open.

Not every sub-command needs an opened clipboard: `CLIP_ISAVAIL` and `CLIP_REGISTER_FORMAT` are not refused by the far2l server without it, and `CLIP_ISAVAIL` does not require authorization; the answer, however, comes from the clipboard backend of the host, and the file based backend of far2l answers "not available" / `0` while the clipboard is not open, so a robust client sends them inside an open. `CLIP_EMPTY`, `CLIP_SETDATA`, `CLIP_GETDATA` and `CLIP_GETDATAID` need an open clipboard (the first two answer `-1` if it is not open, `CLIP_GETDATA` answers `0xFFFFFFFF` and `CLIP_GETDATAID` answers `0`).

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

The server accumulates the chunks in order, and the next `CLIP_SETDATA` **prepends** them to its own data (`size` of that request does not include them). Chunks are accepted only while the clipboard is open, and every `CLIP_OPEN`, `CLIP_CLOSE` and `CLIP_SETDATA` drops the ones not yet consumed. `CLIP_SETDATACHUNK` has no return values, and the far2l client sends its chunks without waiting for replies: only every 16th chunk is sent with a non-zero ID and waited for, to keep the transmission from running too far ahead of the terminal. **A server must therefore answer such a chunk with an empty reply (the ID alone).** The client also sleeps for an eighth of the time it was transmitting when it was busy for at least 256 ms, to leave the bandwidth for other traffic, and when the application cancels the transfer, sends a chunk with size zero, stops and does not send `CLIP_SETDATA`.

When the server has not reported `FARTTY_FEATCLIP_CHUNKED_SET`, the whole data is sent in one `CLIP_SETDATA`, which can be very large.

#### 5.7.4. Reading

```
CLIP_OPEN(passcode)                      -> status, features
CLIP_ISAVAIL(format)                     -> available            (optional, needs no open)
CLIP_GETDATAID(format)                   -> data ID              (optional, only with DATA_ID)
CLIP_GETDATA(format)                     -> size, data, [data ID]
CLIP_CLOSE                                  (the far2l client does not wait)
```

`CLIP_GETDATA` returns the whole data in one reply; there is no chunked download. The reply stack, in pop order, is: `uint32_t` size, then `size` bytes of data, then, if the server always sends it, a `uint64_t` data ID. The size is `0` if there is no data of this format, if the read was not allowed (5.7.2), or if the operation failed; it is `0xFFFFFFFF` if the clipboard was not open. The far2l client pops the data ID only if the server reported `FARTTY_FEATCLIP_DATA_ID`. When the clipboard is open, the far2l server always pushes the ID, also when the size is 0 (the ID is then 0). When it is not open, the reply holds the size `0xFFFFFFFF` alone, with no ID.

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

### 5.8. `FARTTY_INTERACT_IMAGE` (`i`)

Images are displayed by the terminal in the cell grid, over the text. The client uploads the pixels (raw or PNG/JPEG), says where they go, and can later move, flip, rotate, extend, scroll or delete them by the image identity, which is a string chosen by the client. The command `i` takes a sub-command letter, and the arguments follow.

| Sub-command | Name | In | Out | far2l client waits |
| ----------- | ---- | -- | --- | ------------------ |
| `c` | `IMAGE_CAPS` | none | capabilities, cell size | yes |
| `s` | `IMAGE_SET` | see below | `uint8_t` success | yes |
| `t` | `IMAGE_TRANSFORM` | see below | `uint8_t` success | yes |
| `d` | `IMAGE_DEL` | `string` image ID | `uint8_t` success | yes |

All image requests are sent with a non-zero ID. The success value is `1` or `0`.

Versions of far2l before December 2025 had a sub-command `r` (rotate) and the capability `WP_IMGCAP_ROTATE` (`0x400`); they were replaced by `t` (transform) and `WP_IMGCAP_ROTMIR`, and `r` no longer exists.

#### 5.8.1. `IMAGE_CAPS` (`c`)

| Out | Type | Description |
| --- | ---- | ----------- |
| Cell height | `int16_t` | The height of a character cell in pixels. |
| Cell width | `int16_t` | The width of a character cell in pixels. |
| Capabilities | `uint64_t` | A combination of `WP_IMGCAP_*`, or `0` if images are not available. |

**The order is the opposite of what the comment in `FarTTY.h` shows** (it lists capabilities, width, height). In the code both sides use the other order: the server pushes capabilities, then width, then height, and the client pops height first. The same holds for the terminal size event (section 6).

| Flag | Value | Meaning |
| ---- | ----- | ------- |
| `WP_IMGCAP_RGBA` | `0x001` | The server supports `WP_IMG_RGB` and `WP_IMG_RGBA`. |
| `WP_IMGCAP_PNG` | `0x002` | The server supports `WP_IMG_PNG`. |
| `WP_IMGCAP_JPG` | `0x003` | The server supports `WP_IMG_JPG`. |
| `WP_IMGCAP_ATTACH` | `0x100` | The server supports attaching to an existing image. |
| `WP_IMGCAP_SCROLL` | `0x200` | The server supports scrolling of an existing image (with attaching). |
| reserved | `0x400` | Was `WP_IMGCAP_ROTATE` in versions before December 2025; do not use it. |
| `WP_IMGCAP_ROTMIR` | `0x800` | The server supports rotation and mirroring of an existing image. |

`WP_IMGCAP_JPG` is **not a separate bit**: `0x003` is `WP_IMGCAP_RGBA | WP_IMGCAP_PNG`. A test of the form `caps & WP_IMGCAP_JPG` is true when only RGBA or only PNG is reported; to check for JPEG exactly, compare `(caps & 3) == 3`. far2l does not use JPEG (its image viewer needs `WP_IMGCAP_RGBA`, and the optional `ATTACH`, `SCROLL`, `ROTMIR`) and the wx GUI of far2l reports all of these bits at once.

The client needs a non-zero cell size to be able to place images in pixels; far2l's image viewer refuses to work with a zero size.

#### 5.8.2. `IMAGE_SET` (`s`)

Displays a new image, or changes an existing one.

| In | Type | Description |
| -- | ---- | ----------- |
| Image ID | `string` | The identity of the image. Setting an existing identity replaces the image (or modifies it, with the attach flags). |
| Flags | `uint64_t` | The format, the attach mode and other options: `WP_IMG_*`, see below. |
| Left | `int16_t` | Cell column of the left edge, or `-1`. |
| Top | `int16_t` | Cell row of the top edge, or `-1`. |
| Right | `int16_t` | Cell column of the right edge, or an extra pixel offset, or `-1`. |
| Bottom | `int16_t` | Cell row of the bottom edge, or an extra pixel offset, or `-1`. |
| Width | `uint32_t` | Width in pixels for RGB and RGBA, the size in bytes of the encoded data for PNG and JPEG. |
| Height | `uint32_t` | Height in pixels for RGB and RGBA, `1` for PNG and JPEG. |
| Data | raw | `Width * Height * 3` bytes for RGB, `Width * Height * 4` bytes for RGBA (rows top to bottom, bytes `R G B [A]` without padding), `Width` bytes for PNG and JPEG. |

| Out | Type | Description |
| --- | ---- | ----------- |
| Success | `uint8_t` | `1` if the image was loaded and is displayed, `0` if not. |

The fields `Left`, `Top`, `Right` and `Bottom` are `int16_t` (`-1` is `0xFFFF`). `Width` and `Height` are popped in this order, each one is a `uint32_t`. A server with `Width` or `Height` equal to zero answers `0` without reading the rest. An unknown format in the flags makes the far2l server fail the request with an empty reply.

**Flags.**

| Flag | Value | Meaning |
| ---- | ----- | ------- |
| `WP_IMG_RGBA` | `0` | Pixels `R G B A`. Needs `WP_IMGCAP_RGBA`. |
| `WP_IMG_RGB` | `1` | Pixels `R G B`. Needs `WP_IMGCAP_RGBA`. |
| `WP_IMG_PNG` | `2` | A PNG file. Needs `WP_IMGCAP_PNG`. |
| `WP_IMG_JPG` | `3` | A JPEG file. Needs `WP_IMGCAP_JPG`. |
| `WP_IMG_ATTACH_LEFT` | `0x010000` | Attach the given image at the left edge of the existing one. Needs `WP_IMGCAP_ATTACH`. |
| `WP_IMG_ATTACH_RIGHT` | `0x020000` | Attach at the right edge. |
| `WP_IMG_ATTACH_TOP` | `0x030000` | Attach at the top edge. |
| `WP_IMG_ATTACH_BOTTOM` | `0x040000` | Attach at the bottom edge. |
| `WP_IMG_SCROLL` | `0x080000` | With an attach flag: the image keeps its size; the existing content moves to the side opposite to the edge where the new part was attached, so that the new part fits in. Needs `WP_IMGCAP_SCROLL`. |
| `WP_IMG_PIXEL_OFFSET` | `0x100000` | The image is not scaled; `Right` and `Bottom` are pixel offsets, see below. |

The format is the low 16 bits (mask `WP_IMG_MASK_FMT` = `0x00FFFF`), which is **a number, not a set of bits**. The attach mode is the value in bits 16 to 18 (mask `WP_IMG_MASK_ATTACH` = `0x070000`), which is also a number: `1` to `4` shifted left by 16; the four values are **not** independent bits. `WP_IMG_SCROLL` and `WP_IMG_PIXEL_OFFSET` are ordinary bits. The attach values were named `WP_IMG_SCROLL_AT_LEFT`, `_RIGHT`, `_TOP` and `_BOTTOM` (with the mask `WP_IMG_MASK_SCROLL`) in the first versions of the image API of November 2025; the values `0x10000` to `0x40000` are the same.

**Position and size on the screen.** If the four numbers are given and `WP_IMG_PIXEL_OFFSET` is not set, the image is stretched over the cells from (`Left`, `Top`) to (`Right`, `Bottom`), inclusive. With `WP_IMG_PIXEL_OFFSET`, the image is not scaled: it is drawn at its own size at the top-left corner of the cell (`Left`, `Top`), moved by `Right` pixels to the right and `Bottom` pixels down (if these are greater than zero). A value of `-1` in `Right` (`Bottom`) means that the width (height) is the natural size of the image in pixels; the two axes are handled independently. In the wx backend `-1` in any field of an existing image means "keep the current value" (so the "own size" of an axis cannot be restored once set); the SDL backend takes `Left` and `Top` from the cursor in that case. For a new image the far2l GUI backends use the cursor position if `Left` and `Top` are `-1`; the column is the current one, and the row is chosen so that the image ends at the current cursor row (the cursor row minus the number of rows the image takes, but not less than 0). Clients that care should always give `Left` and `Top` explicitly.

**Attaching.** Attaching is defined only for an existing image with the same identity. In the far2l GUI, with `ATTACH_LEFT` or `ATTACH_RIGHT` the new piece has to have the same height in pixels as the existing image; with `ATTACH_TOP` or `ATTACH_BOTTOM` the same width; otherwise the request fails. Without `WP_IMG_SCROLL` the image grows by the size of the piece; with it the size is kept and the content moves. This is how far2l's image viewer scrolls a large image without sending it again. Through the far2l extensions an "empty" attach (a request with zero size used to only move an image) is not possible because of the zero size rule above; use `IMAGE_TRANSFORM` to move.

#### 5.8.3. `IMAGE_TRANSFORM` (`t`)

Moves, mirrors or rotates an existing image.

| In | Type | Description |
| -- | ---- | ----------- |
| Image ID | `string` | The identity of the image. |
| Left | `int16_t` | As in `IMAGE_SET`; `-1` keeps the current value. |
| Top | `int16_t` | |
| Right | `int16_t` | |
| Bottom | `int16_t` | |
| Transform | `uint16_t` | A combination of `WP_IMGTF_*`. |

| Out | Type | Description |
| --- | ---- | ----------- |
| Success | `uint8_t` | `1` if the image was changed, `0` if not (for instance, there is no such image). |

| Flag | Value | Meaning |
| ---- | ----- | ------- |
| `WP_IMGTF_ROTATE0` | `0x00` | No rotation (so the request can be used only to move the image). |
| `WP_IMGTF_ROTATE90` | `0x01` | Rotate by 90 degrees clockwise. |
| `WP_IMGTF_ROTATE180` | `0x02` | Rotate by 180 degrees. |
| `WP_IMGTF_ROTATE270` | `0x03` | Rotate by 270 degrees clockwise. |
| `WP_IMGTF_MASK_ROTATE` | `0x03` | The mask of the rotation value. |
| `WP_IMGTF_MIRROR_H` | `0x04` | Flip horizontally. |
| `WP_IMGTF_MIRROR_V` | `0x08` | Flip vertically. |

The rotation is a number in the two low bits, not a set of flags. Mirroring is applied before rotation. Rotation and mirroring need `WP_IMGCAP_ROTMIR`; moving does not.

#### 5.8.4. `IMAGE_DEL` (`d`)

Removes the image. The success value is `0` if there is no such image.

#### 5.8.5. Images and the screen

In the far2l GUI the images are an overlay that is independent of the text: they stay on the screen until they are deleted, and they do not follow the text that is changed under them. A client has to keep track of the images that it displayed and delete them; far2l deletes its image when its image viewer is closed.

If the terminal is not a far2l server, far2l can show images through the Kitty graphics protocol (APC `G`), if the terminal supports it. In this mode it reports only `WP_IMGCAP_RGBA` and can display RGB, RGBA and PNG; there is no attach, scroll, transform or JPEG.

## 6. Events (server to client)

Events are sent by the server, as `ESC _ f2l<base64> BEL` (no colon, see 4.2), only after the extensions have been activated. They have **no request ID** and no reply. The **top of the stack is the event code**, a `char`; the arguments follow in pop order. Once the extensions are activated, the far2l server sends keyboard and mouse input to the application **only** as events: they replace the usual escape sequences for keys and the mouse. (The far2l server puts the bytes of an event into the input stream of the application, in order with all other bytes it writes there, for instance the replies.) The only key that the far2l server does not forward as an event is the emergency exit: the third Ctrl+Alt+C key press in a row (a key press of any other key but Shift, Ctrl and Alt between them starts the count again; only the key press is withheld, not the release) is left to the terminal itself, which kills the shell.

The arguments of keyboard and mouse events are the fields of the Win32 `KEY_EVENT_RECORD` and `MOUSE_EVENT_RECORD` structures, because far2l is built on the WinPort layer, which emulates the Windows console. Virtual key codes (`VK_*`), scan codes and the control key state bits are those of Windows; see `WinPort/WinCompat.h`.

| Code | Name | Meaning |
| ---- | ---- | ------- |
| `K` / `k` | `FARTTY_INPUT_KEYDOWN` / `FARTTY_INPUT_KEYUP` | Key press / release. |
| `C` / `c` | `FARTTY_INPUT_KEYDOWN_COMPACT` / `FARTTY_INPUT_KEYUP_COMPACT` | Compact key press / release. |
| `M` | `FARTTY_INPUT_MOUSE` | Mouse event. |
| `m` | `FARTTY_INPUT_MOUSE_COMPACT` | Compact mouse event. |
| `S` | `FARTTY_INPUT_TERMINAL_SIZE` | The size of the terminal. |

An event with an unknown code is reported in the log of far2l and ignored, so new events can be added in the future; the client has to ignore unknown codes, too.

### 6.1. Key events: `K`, `k`

| Argument | Type | `KEY_EVENT_RECORD` field |
| -------- | ---- | ------------------------ |
| Character | `uint32_t` | `uChar.UnicodeChar`: the character the key produced, as a Unicode code point (UTF-32), or 0. |
| Control key state | `uint32_t` | `dwControlKeyState`. |
| Virtual scan code | `uint16_t` | `wVirtualScanCode`. |
| Virtual key code | `uint16_t` | `wVirtualKeyCode`. |
| Repeat count | `uint16_t` | `wRepeatCount`. |

`K` is sent for a press, `k` for a release (`bKeyDown` is true or false). Both are needed: far2l uses the key releases too.

The control key state is a combination of:

| Flag | Value |
| ---- | ----- |
| `RIGHT_ALT_PRESSED` | `0x0001` |
| `LEFT_ALT_PRESSED` | `0x0002` |
| `RIGHT_CTRL_PRESSED` | `0x0004` |
| `LEFT_CTRL_PRESSED` | `0x0008` |
| `SHIFT_PRESSED` | `0x0010` |
| `NUMLOCK_ON` | `0x0020` |
| `SCROLLLOCK_ON` | `0x0040` |
| `CAPSLOCK_ON` | `0x0080` |
| `ENHANCED_KEY` | `0x0100` |

As in Windows, the left and right Shift keys cannot be told apart by the virtual key code (it is `VK_SHIFT` for both); the right Shift key has the virtual scan code 54 (`RIGHT_SHIFT_VSC` = `54`).

### 6.2. Compact key events: `C`, `c`

| Argument | Type | Description |
| -------- | ---- | ----------- |
| Character | `uint16_t` | `uChar.UnicodeChar`, only if it fits in 16 bits. |
| Control key state | `uint16_t` | `dwControlKeyState`, only if it fits in 16 bits. |
| Virtual key code | `uint8_t` | `wVirtualKeyCode`, only if it fits in 8 bits. |

There is no scan code and no repeat count: the client takes `MapVirtualKey(vk, MAPVK_VK_TO_VSC)` as the scan code, and the repeat count is 1. Compact events may be used only when the client has negotiated `FARTTY_FEAT_COMPACT_INPUT` (5.1), and only if nothing is lost. The far2l server sends the compact form when **all** of this is true: the repeat count is at most 1, the scan code is not the one of the right Shift key, the character is below 0x10000, the control key state is below 0x10000 and the virtual key code is below 0x100. Otherwise it sends `K` or `k`. A client has to be ready to receive both forms at any time, even if it has negotiated the compact one.

### 6.3. Mouse events: `M`, `m`

| Argument | `M` type | `m` type | `MOUSE_EVENT_RECORD` field |
| -------- | -------- | -------- | -------------------------- |
| Event flags | `uint32_t` | `uint8_t` | `dwEventFlags` |
| Control key state | `uint32_t` | `uint8_t` | `dwControlKeyState` |
| Button state | `uint32_t` | `uint16_t` | `dwButtonState` (compact: encoded, see below) |
| Y | `int16_t` | `int16_t` | `dwMousePosition.Y`: the row, counted from 0 |
| X | `int16_t` | `int16_t` | `dwMousePosition.X`: the column, counted from 0 |

`dwEventFlags`: `MOUSE_MOVED` `0x0001`, `DOUBLE_CLICK` `0x0002`, `MOUSE_WHEELED` `0x0004` (vertical wheel), `MOUSE_HWHEELED` `0x0008` (horizontal wheel), or `0` for a press or a release of a button. There is no separate "release" event: a release is an event where the bit of the button is no more set in `dwButtonState`.

`dwButtonState`: the low bits show the buttons that are held: `FROM_LEFT_1ST_BUTTON_PRESSED` `0x0001` (left), `RIGHTMOST_BUTTON_PRESSED` `0x0002` (right), `FROM_LEFT_2ND_BUTTON_PRESSED` `0x0004` (middle), `FROM_LEFT_3RD_BUTTON_PRESSED` `0x0008`, `FROM_LEFT_4TH_BUTTON_PRESSED` `0x0010`. For wheel events the high 16 bits are a signed wheel delta; far2l looks only at its sign: positive is up (or right), negative is down (or left). The far2l TTY backend itself produces `+1` and `-1` (`0x0001` and `0xFFFF` in the high word) when it translates the wheel reports of an ordinary terminal.

Compact form. The 32-bit button state is squeezed in 16 bits as `(state & 0xFF) | ((state >> 8) & 0xFF00)`, which keeps the bits 0 to 7 and the bits 16 to 23, and the client undoes it with `(v & 0xFF) | ((v & 0xFF00) << 8)`. The far2l server uses the compact form if `(state & 0xFF00FF00) == 0`, the control key state is below `0x100` and the event flags are below `0x100`. So a wheel-up event can be compact, and a wheel-down cannot: its delta has the bits 24 to 31 set.

The far2l client enqueues the mouse event as it is given; the server is responsible for the coordinates, which are counted in character cells of the client's screen.

### 6.4. `FARTTY_INPUT_TERMINAL_SIZE` (`S`)

The size of the terminal in cells. Sent if the client asked for `FARTTY_FEAT_TERMINAL_SIZE`, at once when it asked and then whenever the size changes.

| Argument | Type | Description |
| -------- | ---- | ----------- |
| Height | `uint16_t` | The number of rows. |
| Width | `uint16_t` | The number of columns. |

**The order is the opposite of what `FarTTY.h` says** (there it is width, then height): both far2l sides use height on top, and the server pushes width first, then height, then the code `S`.

This event is useful for a client that cannot ask the kernel for the size (its standard output is not a TTY, for instance it runs in a pipe or over a socket). The far2l client uses the received size **only as a fallback**: it looks first at the TTY size of its standard output and then of its standard input; if that fails, or gives 0 by 0 (a serial console), it uses the `LINES` and `COLUMNS` environment variables, if they are between 11 and 4095, and only then the size from the event (80 by 25 until something is received). As a consequence, a client that has `LINES` and `COLUMNS` set in its environment ignores the event.

## 7. Worked examples

Every wire string below was generated with the code of far2l itself (`StackSerializer` and `base64` from `utils/`, compiled and run), and the stacks were decoded back with it; the bytes are shown as they are on the wire, with `\x1b` for `ESC`, `\x5c` for the backslash and `\x07` for `BEL`, followed by the serialized stack in hexadecimal, from the bottom (the first byte) to the top (the last byte, which is popped first).

### 7.1. Enabling, and a request with no reply

The client enables the extensions and waits for the answer (the `ESC [ 5 n` that follows it in far2l is a separate, ordinary query):

```
client:  \x1b_far2l1\x1b\x5c
server:  \x1b_far2lok\x07
```

Setting the cursor height to 50 percent. The stack, top to bottom, is: ID `0`, command `h`, the height `50`; so the client pushes `50`, then `h`, then `0`:

<!-- ex:cursor-height -->
```
\x1b_far2l:MmgA\x07
```
Stack bytes, bottom to top: `32 68 00`

### 7.2. A request and its reply

`GET_WINDOW_MAXSIZE` with ID 1. The stack, top to bottom: ID `1`, `w`:

<!-- ex:maxsize-request -->
```
\x1b_far2l:dwE=\x07
```
Stack bytes, bottom to top: `77 01`

The server answers that the largest window is 200 columns by 50 rows. The reply stack, top to bottom: ID `1`, height `50`, width `200`; so the server pushes the width first, then the height, then the ID:

<!-- ex:maxsize-reply -->
```
\x1b_far2lyAAyAAE=\x07
```
Stack bytes, bottom to top: `c8 00 32 00 01`

### 7.3. Strings

A desktop notification with the title "Done" and the text "OK", ID 0. The string is its bytes followed by its length as `uint32_t`; the title is popped first, so it is pushed last:

<!-- ex:notification -->
```
\x1b_far2l:T0sCAAAARG9uZQQAAABuAA==\x07
```
Stack bytes, bottom to top: `4f 4b 02 00 00 00 44 6f 6e 65 04 00 00 00 6e 00`

### 7.4. Titles of F-keys

F1 is "Help", F2 is "Menu", F3 to F12 have no title; ID 1. Popping order is F1, F2, ... F12, so F12 is pushed first (ten zero bytes, the states), then F2, then F1; the state is above the string of each entry:

<!-- ex:fkeys -->
```
\x1b_far2l:AAAAAAAAAAAAAE1lbnUEAAAAAUhlbHAEAAAAAWYB\x07
```
Stack bytes, bottom to top: `00 00 00 00 00 00 00 00 00 00 4d 65 6e 75 04 00 00 00 01 48 65 6c 70 04 00 00 00 01 66 01`

The reply is the success flag:

<!-- ex:fkeys-reply -->
```
\x1b_far2lAQE=\x07
```
Stack bytes, bottom to top: `01 01`

### 7.5. Clipboard

Opening the clipboard with the client ID `0123456789abcdef0123456789abcdef` (32 characters), ID 1. Top to bottom: ID, `c`, `o`, the string:

<!-- ex:clip-open -->
```
\x1b_far2l:MDEyMzQ1Njc4OWFiY2RlZjAxMjM0NTY3ODlhYmNkZWYgAAAAb2MB\x07
```
Stack bytes, bottom to top: `30 31 32 33 34 35 36 37 38 39 61 62 63 64 65 66 30 31 32 33 34 35 36 37 38 39 61 62 63 64 65 66 20 00 00 00 6f 63 01`

The server authorized the client and supports both clipboard features (value 3). Top to bottom: ID `1`, status `1`, features `3`:

<!-- ex:clip-open-reply -->
```
\x1b_far2lAwAAAAAAAAABAQ==\x07
```
Stack bytes, bottom to top: `03 00 00 00 00 00 00 00 01 01`

Setting the text "hello" (format 1, `CF_TEXT`), ID 2. Top to bottom: ID, `c`, `s`, the format, the size, the data:

<!-- ex:clip-setdata -->
```
\x1b_far2l:aGVsbG8FAAAAAQAAAHNjAg==\x07
```
Stack bytes, bottom to top: `68 65 6c 6c 6f 05 00 00 00 01 00 00 00 73 63 02`

The server reports success and the data ID `0x1122334455667788`. Top to bottom: ID `2`, status `1`, data ID:

<!-- ex:clip-setdata-reply -->
```
\x1b_far2liHdmVUQzIhEBAg==\x07
```
Stack bytes, bottom to top: `88 77 66 55 44 33 22 11 01 02`

Getting the text, ID 3:

<!-- ex:clip-getdata -->
```
\x1b_far2l:AQAAAGdjAw==\x07
```
Stack bytes, bottom to top: `01 00 00 00 67 63 03`

The reply holds the size, the data and the data ID. Top to bottom: ID `3`, size `5`, the five bytes of "hello", the data ID:

<!-- ex:clip-getdata-reply -->
```
\x1b_far2liHdmVUQzIhFoZWxsbwUAAAAD\x07
```
Stack bytes, bottom to top: `88 77 66 55 44 33 22 11 68 65 6c 6c 6f 05 00 00 00 03`

Closing the clipboard; far2l sends it with ID 0 and does not wait for a reply:

<!-- ex:clip-close -->
```
\x1b_far2l:Y2MA\x07
```
Stack bytes, bottom to top: `63 63 00`

Registering a custom format, ID 4, and the answer (the ID of the format is `0xC000`):

<!-- ex:clip-register -->
```
\x1b_far2l:RkFSX1ZlcnRpY2FsQmxvY2tfVW5pY29kZRkAAAByYwQ=\x07
```
Stack bytes, bottom to top: `46 41 52 5f 56 65 72 74 69 63 61 6c 42 6c 6f 63 6b 5f 55 6e 69 63 6f 64 65 19 00 00 00 72 63 04`

<!-- ex:clip-register-reply -->
```
\x1b_far2lAMAAAAQ=\x07
```
Stack bytes, bottom to top: `00 c0 00 00 04`

### 7.6. Images

Asking for the image capabilities, ID 5. Top to bottom: ID, `i`, `c`:

<!-- ex:image-caps -->
```
\x1b_far2l:Y2kF\x07
```
Stack bytes, bottom to top: `63 69 05`

The server reports the cell size 8 by 16 pixels and the capabilities `0xB03` (`RGBA`, `PNG`, `ATTACH`, `SCROLL`, `ROTMIR`). Top to bottom: ID `5`, height `16`, width `8`, capabilities:

<!-- ex:image-caps-reply -->
```
\x1b_far2lAwsAAAAAAAAIABAABQ==\x07
```
Stack bytes, bottom to top: `03 0b 00 00 00 00 00 00 08 00 10 00 05`

Showing a 1 by 1 pixel RGBA image of one red pixel (`ff 00 00 ff`), named `img`, at column 2, row 3, at its own size; ID 6. Top to bottom: ID, `i`, `s`, the name, the flags (`0`: RGBA, nothing else), Left `2`, Top `3`, Right `-1`, Bottom `-1`, width `1`, height `1`, the pixels:

<!-- ex:image-set -->
```
\x1b_far2l:/wAA/wEAAAABAAAA/////wMAAgAAAAAAAAAAAGltZwMAAABzaQY=\x07
```
Stack bytes, bottom to top: `ff 00 00 ff 01 00 00 00 01 00 00 00 ff ff ff ff 03 00 02 00 00 00 00 00 00 00 00 00 69 6d 67 03 00 00 00 73 69 06`

The answer is `1` (shown):

<!-- ex:image-set-reply -->
```
\x1b_far2lAQY=\x07
```
Stack bytes, bottom to top: `01 06`

Rotating by 90 degrees and mirroring horizontally (`0x05`) the image `img` without moving it (`-1` in all four coordinates), ID 7:

<!-- ex:image-transform -->
```
\x1b_far2l:BQD//////////2ltZwMAAAB0aQc=\x07
```
Stack bytes, bottom to top: `05 00 ff ff ff ff ff ff ff ff 69 6d 67 03 00 00 00 74 69 07`

### 7.7. Choosing features

The client declares that it understands compact input (`FARTTY_FEAT_COMPACT_INPUT`, 1), ID 0:

<!-- ex:features -->
```
\x1b_far2l:AQAAAAAAAAB4AA==\x07
```
Stack bytes, bottom to top: `01 00 00 00 00 00 00 00 78 00`

### 7.8. Events

A key press of the letter `a` (code point 0x61, virtual key `0x41`, scan code `0x1E`, no modifiers, repeat count 1). Events have no ID. The full form, `K`:

<!-- ex:key-down -->
```
\x1b_f2lAQBBAB4AAAAAAGEAAABL\x07
```
Stack bytes, bottom to top: `01 00 41 00 1e 00 00 00 00 00 61 00 00 00 4b`

The same press in the compact form, `C` (the client takes the scan code from the virtual key):

<!-- ex:key-down-compact -->
```
\x1b_f2lQQAAYQBD\x07
```
Stack bytes, bottom to top: `41 00 00 61 00 43`

A press of the left mouse button at column 10, row 5, in the compact form `m` (the button state `1` fits in the 16 bits):

<!-- ex:mouse-compact -->
```
\x1b_f2lCgAFAAEAAABt\x07
```
Stack bytes, bottom to top: `0a 00 05 00 01 00 00 00 6d`

A wheel turned down at the same place. The wheel delta `0xFFFF` is in the high half of the button state, which does not fit in the compact form, so the full form `M` is used. Top to bottom: code, flags `4` (`MOUSE_WHEELED`), control state `0`, button state `0xFFFF0000`, row `5`, column `10`:

<!-- ex:mouse-wheel-down -->
```
\x1b_f2lCgAFAAAA//8AAAAABAAAAE0=\x07
```
Stack bytes, bottom to top: `0a 00 05 00 00 00 ff ff 00 00 00 00 04 00 00 00 4d`

The terminal is 80 columns by 25 rows. Top to bottom: code `S`, height `25`, width `80`:

<!-- ex:terminal-size -->
```
\x1b_f2lUAAZAFM=\x07
```
Stack bytes, bottom to top: `50 00 19 00 53`


## 8. How far2l really behaves: details and oddities

The text above describes the protocol; this section collects what a developer of the other side has to know about the behavior of far2l, including the places where the code differs from the comments in `FarTTY.h`. Everything here was checked in the source.

### 8.1. Differences between `FarTTY.h` and the code

The code is authoritative; these are the places where the header is misleading.

| Place | What the header says | What the code does |
| ----- | -------------------- | ------------------ |
| Events | `ESC _ f2l:` + Base64 | `ESC _ f2l` + Base64, no colon (4.2). |
| Replies | not shown | `ESC _ far2l` + Base64, no colon (4.2). |
| `IMAGE_CAPS` reply | capabilities, then cell width, then cell height | Popped as: cell height, cell width, capabilities (5.8.1). |
| `FARTTY_INPUT_TERMINAL_SIZE` | width, then height | Popped as: height, then width (6.4). |
| `CLIP_GETDATAID` (overview near the top of the header) | "(STATUS, ID of remote data)" | The ID alone (5.7). |
| `SET_FKEY_TITLES` | "12 elements" (the order is not stated) | Popped F1 first; fewer than 12 are accepted by the server (5.5). |
| Clipboard data ID | "OPTIONAL", "only if server reported `FARTTY_FEATCLIP_DATA_ID`" | The far2l server always reports `FARTTY_FEATCLIP_DATA_ID` in the `CLIP_OPEN` reply, so "optional" never comes into play with it. `CLIP_GETDATA` with the clipboard open always holds the ID (0 if there is no data); with the clipboard closed the reply is the size `0xFFFFFFFF` alone. `CLIP_SETDATA` holds the ID only when the status is `1`. |
| `WP_IMGCAP_JPG` | a capability | It is `0x003`, both low bits, so it overlaps `WP_IMGCAP_RGBA` and `WP_IMGCAP_PNG` (5.8.1). |
| `FARTTY_FEAT_*`, `FARTTY_FEATCLIP_*` | written as 32-bit literals | Requests and replies carry them as a `uint64_t`. |

### 8.2. Client (TTY backend)

- **Activation and exit.** `ESC _ far2l1 ESC \` is written when the client starts, together with the `ESC [ 5 n` probe (4.5). `ESC _ far2l0 BEL` is written on exit, on `SIGTSTP`, `SIGTERM` and `SIGHUP` (except with `--mortal`), and when the client is detached from the terminal to be revived later. After resuming, and after a revive in another terminal, the client probes again, so the server can see `far2l1` repeatedly.
- **Flow control.** The client queues the requests and a writer thread sends them. The calls that need an answer block the calling thread until the reply arrives or the input breaks (an exception in the input parser, a read error, a lost connection), with no timeout. Up to 255 requests can wait at once; when that limit is reached, the request that hits it fails with an empty stack and the rest of the queued batch, including requests with ID 0, is dropped.
- **Malformed input.** Events and replies whose stack cannot be popped are dealt with differently depending on the place. A broken key or mouse event is dropped and logged. An event with an empty stack (the event code is missing), a truncated size event and a reply without an ID throw out of the parser: the client then discards its whole input buffer and **fails all requests that are waiting for replies** (they see an empty stack). A server must not send such events; in particular the colon in `f2l:` makes the payload decode as empty.
- **Unknown codes.** An event with an unknown code is logged and ignored.
- **Strings.** Strings are not validated on either side: popping a string copies the raw bytes. The client receives no strings in events and replies; the server converts the format name and the notification text from UTF-8 itself.
- **Focus.** Notifications depend on focus reports (`ESC [ I`, `ESC [ O`) that far2l asks for with `ESC [ ? 1004 h`. While the extensions are active, far2l assumes that its window is **not** focused until the first focus report arrives, so that notifications work best-effort with servers that do not send focus reports.
- **Startup sequences.** Besides the activation, a far2l client switches on the alternate screen (`?47`, `?1049`), bracketed paste (`?2004`), focus reports (`?1004`), the `win32-input-mode` (`?9001`), the iTerm2 input mode (`?1337`), the Kitty keyboard protocol (`CSI = 15 ; 1 u`) and the mouse modes (`?1000`, `?1001`, `?1002`, `?1003`, `?1006`); the `--nodetect=w`, `--nodetect=a` and `--nodetect=k` options skip `?9001`, `?1337` and the Kitty mode. A far2l server that delivers input as events can ignore them.
- **Clipboard.** Once any `CLIP_OPEN` has been answered with `-1`, the client uses its local file clipboard for the rest of the process, even if the user changes their mind in the terminal. A `CLIP_OPEN` answered with `0` is retried on the next use.
- **Window size.** The answer to `w` is cached for the life of the process, including an answer of `0` by `0`; a reply that the server gave "to mean unsupported" by two zero numbers is taken literally.
- **F-key titles.** After the first answer (see 5.5) the support status is fixed until the client attaches to a terminal again (resume, revive), when it is reset to "unknown"; an empty reply counts as "not supported".

### 8.3. Server (far2l built-in terminal)

- The server accepts requests only between `far2l1` and `far2l0`; `far2l:` with an empty payload is ignored, and a request nested in another unterminated escape string is not recognized. Only the first character after `far2l` decides what the string is, so the four forms are `far2l1`, `far2l0`, `far2l:...` and `far2l#...`; other characters after `far2l1` or `far2l0` are ignored.
- Requests are executed in the order in which they arrive, one at a time, on the thread that parses the output of the application, so the parsing of further output waits for each request. A slow request (the clipboard authorization dialog) therefore blocks the terminal until the user has decided; the client waits for the reply.
- Replies are written to the input of the application, in order with the keyboard and mouse events.
- Exceptions inside a request (a stack that is too short, an unknown image format) are caught: the stack is cleared and a reply with only the ID is sent if the ID was not zero.
- The clipboard read check (5.7.2) uses a 32-bit millisecond tick counter (on Linux and macOS the time since boot, without the time of suspend; elsewhere derived from the wall clock). The "last allowed" time of a new activation starts at zero, so as long as no paste gesture has happened since the activation, and the counter is below 5000 (within 5 seconds of the boot of the host, and again each time the counter wraps around, about every 49.7 days), a read is permitted without a paste gesture. The counter is compared with wrap-around taken into account.
- When the clipboard is not open, `CLIP_GETDATA` answers with the size `0xFFFFFFFF`, and the four bytes of the format the client sent stay in the reply stack under it (the client ignores them).
- If the clipboard is open but a paste gesture is absent, `CLIP_GETDATA` answers size 0 and a data ID 0, so the application cannot tell a refusal from an empty clipboard.
- `CLIP_ISAVAIL` does not check authorization, so any client can find out which formats are on the clipboard.
- When the extensions are deactivated (`far2l0`, or the end of the command that was run in the terminal), the server closes every clipboard it had open and clears the F-key titles. The host identity (`far2l#`) is forgotten at the start and at the end of each command.
- The clipboard operations are forwarded to the clipboard of the host of the terminal (`OpenClipboard`, `EmptyClipboard`, `SetClipboardData`, `GetClipboardData`, `IsClipboardFormatAvailable`, `RegisterClipboardFormat` of the WinPort layer).
- OSC 52: the far2l terminal also understands the OSC 52 write; it asks the user (allow once, allow for this command, or block) and ignores the read form, for security reasons.

### 8.4. Advice to implementers

For a **server** (a terminal):

1. Reply `ESC _ far2lok BEL` to `far2l1` (BEL, and before the answer to `ESC [ 5 n`) and accept both BEL and ST as terminators of everything. Do not reply to `far2l0`.
2. Reply to **every** request with a non-zero ID, even to those that you do not implement, at least with the ID alone. Clients that wait without a timeout (far2l) would otherwise hang. For requests with ID 0 never reply.
3. Answer `CLIP_OPEN` with `-1` or `0` if you do not want to give out your clipboard; do not implement what you cannot do safely. Report `FARTTY_FEATCLIP_CHUNKED_SET` if you have a limit on the length of an escape sequence, so that the client sends its data in pieces of 16 KiB. The reading of the clipboard can not be split, so answer with size 0 when the data is too large for you.
4. Protect reading as far2l does (5.7.2), or by another method of your own that needs the user's action.
5. Ordinary key and mouse events may be sent from the moment of the acknowledgement. Send the compact forms of events only after the client has asked for them, and only when nothing is lost (6.2, 6.3); the full forms can always be sent.
6. Answer `SET_FKEY_TITLES` with one `bool` byte, `0` if you cannot show the titles (an empty reply is also read as "not supported", but through an error); answer `GET_WINDOW_MAXSIZE` with the real values, or with the current size if you do not know better, but remember that far2l caches the answer.
7. Put `far2l#<identity>` handling in if the terminal runs commands of several remote hosts.

For a **client** (an application):

1. Send `ESC _ far2l1 ESC \` followed by `ESC [ 5 n`, and read until the reply to the DSR; accept `far2lok` with both terminators. If the acknowledgement was consumed by some other code that reads the input (for instance another terminal probe), announce again: it is safe, the server answers every time.
2. Send `ESC _ far2l0 ESC \` (or BEL) on exit.
3. Put a timeout on replies. far2l has none, but another terminal may fail to answer a request, and nothing in the protocol tells you.
4. Prefer the ST terminator for what you send. BEL is accepted by far2l and by the other servers listed in section 3, but it is not a standard terminator of APC in other terminals and multiplexers.
5. Use a stable client ID (store it in a file). A new ID at each start makes the terminal ask the user again each time. The client ID has 32 to 256 characters of `0`-`9`, `a`-`z`, `-`, `_`.
6. Never assume that a missing reply means a failure: a clipboard read without the user's gesture answers "no data".

## 9. Links to implementations

The state of the code in the list below was checked in September 2026. Permanent links point to the commits that were examined.

**far2l** (the reference implementation)

- The protocol: [`WinPort/FarTTY.h`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/WinPort/FarTTY.h), [`WinPort/WinCompat.h`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/WinPort/WinCompat.h) (image and key constants).
- The stack serializer: [`utils/include/StackSerializer.h`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/utils/include/StackSerializer.h), [`utils/src/StackSerializer.cpp`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/utils/src/StackSerializer.cpp), [`utils/src/base64.cpp`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/utils/src/base64.cpp).
- Client (the TTY backend): [`WinPort/src/Backend/TTY/`](https://github.com/elfmz/far2l/tree/c12197bceb57b645dcbac99af6c51be815cb756b/WinPort/src/Backend/TTY): `TTYCaps.cpp` (detection), `TTYBackend.cpp`, `TTYOutput.cpp`, `TTYInputSequenceParser.cpp`, `TTYFar2lClipboardBackend.cpp`.
- Server (the built-in terminal): [`far2l/src/vt/VTFar2lExtensios.cpp`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/far2l/src/vt/VTFar2lExtensios.cpp), [`far2l/src/vt/vtshell.cpp`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/far2l/src/vt/vtshell.cpp) (`OnApplicationProtocolCommand`).
- The reference of how images are drawn: [`WinPort/src/Backend/WX/wxConsoleImages.cpp`](https://github.com/elfmz/far2l/blob/c12197bceb57b645dcbac99af6c51be815cb756b/WinPort/src/Backend/WX/wxConsoleImages.cpp).

**f4** (client and server), Go

- Repository: <https://github.com/unxed/f4>; the terminal side is in [`internal/terminal/`](https://github.com/unxed/f4/tree/70586aaa3d9ab954c180a53d7984988df481ac7a/internal/terminal): [`view.go`](https://github.com/unxed/f4/blob/70586aaa3d9ab954c180a53d7984988df481ac7a/internal/terminal/view.go) (`HandleFar2lAPC`, `ProcessFar2lInteract`), [`far2l_image.go`](https://github.com/unxed/f4/blob/70586aaa3d9ab954c180a53d7984988df481ac7a/internal/terminal/far2l_image.go) (images).
- The application side: <https://github.com/unxed/vtui> ([`far2l_extensions.go`](https://github.com/unxed/vtui/blob/4521c325683770a21d8acee3b93809b41023bda6/far2l_extensions.go): clipboard; [`graphics_far2l.go`](https://github.com/unxed/vtui/blob/4521c325683770a21d8acee3b93809b41023bda6/graphics_far2l.go): images), <https://github.com/unxed/vtinput> ([`terminal.go`](https://github.com/unxed/vtinput/blob/63fc0dd897598de3ae3b13835a6c8bb8592a9ce1/terminal.go): activation; [`parser.go`](https://github.com/unxed/vtinput/blob/63fc0dd897598de3ae3b13835a6c8bb8592a9ce1/parser.go): events; [`far2l_stack.go`](https://github.com/unxed/vtinput/blob/63fc0dd897598de3ae3b13835a6c8bb8592a9ce1/far2l_stack.go): the stack serializer in Go).
- The drop proposal (section 11): [`docs/FAR2L_DND.md`](https://github.com/unxed/f4/blob/70586aaa3d9ab954c180a53d7984988df481ac7a/docs/FAR2L_DND.md); the code is in [`internal/terminal/far2l_dnd.go`](https://github.com/unxed/f4/blob/70586aaa3d9ab954c180a53d7984988df481ac7a/internal/terminal/far2l_dnd.go) (terminal side), `far2l_dnd_client.go` (application side), `far2l_dnd_bridge.go`, `far2l_dnd_proxy.go` (nested terminals) and the codec in `far2ldnd/` of the same directory.

**Turbo Vision for C++ and turbo** (client), C++

- <https://github.com/magiblot/tvision>: [`source/platform/far2l.cpp`](https://github.com/magiblot/tvision/blob/b4831e2ca16652db327fb7cc964f1c8c2d512524/source/platform/far2l.cpp), [`include/tvision/internal/far2l.h`](https://github.com/magiblot/tvision/blob/b4831e2ca16652db327fb7cc964f1c8c2d512524/include/tvision/internal/far2l.h), its use in [`source/platform/termio.cpp`](https://github.com/magiblot/tvision/blob/b4831e2ca16652db327fb7cc964f1c8c2d512524/source/platform/termio.cpp); the README lists far2l among the supported terminal extensions.
- <https://github.com/magiblot/turbo> has no code of its own for it: it includes tvision as the submodule `deps/tvision` and gets the support from there.

**putty4far2l** (server), C, Windows

- <https://github.com/ivanshatsky/putty4far2l>, branch `far2l`, based on PuTTY 0.78; releases there. Code: [`terminal/terminal.c`](https://github.com/ivanshatsky/putty4far2l/blob/9d2ec04f84a912dc96be798e47faaf419420cfd3/terminal/terminal.c) (APC handling), [`windows/window.c`](https://github.com/ivanshatsky/putty4far2l/blob/9d2ec04f84a912dc96be798e47faaf419420cfd3/windows/window.c) (key events).
- <https://github.com/unxed/putty4far2l>, based on PuTTY 0.76; in December 2024 all changes of the repository above were imported into it. Code: [`terminal/terminal.c`](https://github.com/unxed/putty4far2l/blob/1816bdba0554f9f4465b92bc0df95fb845ff4170/terminal/terminal.c).

**KiTTY** (server), C, Windows

- <https://github.com/cyd01/KiTTY>: built with `MOD_FAR2L` (enabled in `0.76b_My_PuTTY/windows/MAKEFILE.MINGW`); code in [`0.76b_My_PuTTY/terminal/terminal.c`](https://github.com/cyd01/KiTTY/blob/75fa2abcd220c17249ff7252f8d5224137001f2d/0.76b_My_PuTTY/terminal/terminal.c) and [`0.76b_My_PuTTY/windows/window.c`](https://github.com/cyd01/KiTTY/blob/75fa2abcd220c17249ff7252f8d5224137001f2d/0.76b_My_PuTTY/windows/window.c). The request to add the support and the first description of the protocol are in [KiTTY issue 74](https://github.com/cyd01/KiTTY/issues/74).

**thruPTY** (server), Go

- <https://github.com/Dazzar56/thruPTY>: [`far2l.go`](https://github.com/Dazzar56/thruPTY/blob/758410d1633ba01898cbe8149904b3180429ca96/far2l.go), ported from the terminal of f4.

**Other discussions**

- [hpjansson/chafa issue 311](https://github.com/hpjansson/chafa/issues/311): a request to support the far2l image protocol in the chafa image viewer, with a test program.

## 10. Experience of other implementations

What follows is not a part of the specification. It is what was seen in the sources of the implementations listed above, to help the next one and, where it is useful, to explain what the protocol expects.

**The prefixes are easy to copy wrongly.** The comment in `FarTTY.h` writes the events as `"\x1b_f2l:"`. Code that follows it literally sends far2l a payload that decodes to nothing; the event is lost and, for the size event, the client drops its whole input buffer (8.2). The reply has no colon either. The colon belongs to the request only (4.2). The same header lists the `IMAGE_CAPS` values and the size event in an order that is opposite to what the code does, so the order of the code has to be followed (8.1). More than one implementation has been caught by these; the examples in section 7 were produced by the code of far2l and can be used to test a new one.

**ST or BEL.** far2l itself ends what it writes with BEL (and the activation with ST), and accepts both. Other terminal emulators are less tolerant: BEL is a widely accepted terminator of OSC, but far from all terminals accept it for APC, and a standard-conforming ST is the safe choice (this was pointed out in the discussion of the chafa issue, section 9). Turbo Vision sends ST for everything it sends; vtinput sends the activation and deactivation with ST, but the requests of the f4 application side (vtui) end with BEL, as far2l's own do. far2l accepts both. The opposite direction needs care: the input parser of Turbo Vision reads an incoming event up to BEL only, so a terminal should end events and replies with BEL, as far2l does. In short, a client should send ST and a server should send BEL.

**Request pipelining.** The TTY backend of far2l waits for the reply to `CLIP_OPEN` before it goes on. Turbo Vision sends `CLIP_OPEN`, `CLIP_GETDATA` and `CLIP_CLOSE` at once, with no reply requested for the first and the last (ID 0) and one fixed ID, `0xA0`, for the read, and puts the text into the application when that reply arrives. It works with a server that executes the requests in the order they come (far2l does), and saves two round trips. With a server that runs each request in its own thread, as the terminals of f4 and thruPTY do, the order is not guaranteed, and such pipelining can break. The price is that the client does not learn whether the open was permitted. Turbo Vision also sends a NUL-terminated string in `CLIP_SETDATA` and removes the NUL on read; the far2l server handles both.

**The client ID.** A client ID has to be random, stay the same between runs, and not be known to others. Turbo Vision builds it from the current time: each start of the application is a new client for the terminal, which asks the user again each time. A constant that is compiled into the program is also no good: anybody can read it, and the authorization that the user gave to that ID is then given to everybody who uses the same constant. far2l generates 64 characters from the host name and a random string and keeps them in `~/.config/far2l/tty_clipboard/me`.

**Terminals that keep little state.** The Windows PuTTY forks (putty4far2l, KiTTY) do not store the client ID. They ask a single question, whether the far2l clipboard synchronization is allowed (in putty4far2l a setting of the session can answer it in advance: deny, ask, allow), and they forget the answer when the extensions are activated again (that is, each time far2l is started). They give the answer `-1` ("use your own clipboard") to a client that was refused. Once the client is allowed in, they work with the clipboard of Windows directly: `CF_TEXT` and `CF_UNICODETEXT` (converted between UTF-8, UTF-32 and UTF-16), and registered formats, which are registered in Windows by name (`CLIP_REGISTER_FORMAT`) and given out and accepted as opaque data.

**Limits of the size of a sequence.** far2l has no limit on the length of an APC string, and its TTY backend sends the whole clipboard in one `CLIP_SETDATA` if the server did not report `FARTTY_FEATCLIP_CHUNKED_SET`. A terminal that holds the sequence in a fixed buffer has to report that flag, which makes far2l send pieces of 16 KiB, and has to refuse giving out data that do not fit into its buffer, answering with the size 0 (there is no chunked download). the putty4far2l forks grow their buffer for the sequence dynamically, in blocks of 1 MiB, but KiTTY, which has a buffer of 1 MiB, shows a message and exits when it receives a larger sequence, because the request ID would be lost with the rest of the data.

**Stub replies are taken literally.** The PuTTY forks answer the requests that they do not implement with four zero bytes. For `SET_FKEY_TITLES` this is a correct "not supported" (the success flag is 0). For `GET_WINDOW_MAXSIZE` it is 0 by 0, which far2l takes as the answer and keeps for the whole run. The better answer to a request that is not supported is an empty reply (the ID alone), which the client notices as missing values.

**The first answer to `f` decides.** far2l asks about F-key titles once (5.5). A terminal that answered "yes" is not asked again, whatever it does with the titles afterwards; f4 and thruPTY answer "yes" and ignore the titles.

**Images and the Kitty fallback.** far2l that runs inside a terminal that acknowledged `far2l1` uses the far2l image requests and does not try the Kitty graphics protocol, so a terminal that acknowledges the extensions has to answer the image requests, at least with `0` capabilities; if it does not, the calling thread of far2l blocks forever (there is no timeout, 4.4). f4 reports only `WP_IMGCAP_RGBA` and answers `0` to `IMAGE_TRANSFORM`, and far2l then resends whole pictures instead of rotating and scrolling them, which is slower but works. f4 handles `WP_IMG_PIXEL_OFFSET` (the picture is not scaled, and the right and bottom values are pixel shifts), which is the usual case for the image viewer of far2l, and a right-hand value taken as a column there gives a picture of a negative width. thruPTY answers `0` to all image requests.

**Clipboard formats in other terminals.** f4 and thruPTY treat any format as text. `CLIP_ISAVAIL` always says "yes", `CLIP_REGISTER_FORMAT` always gives `0xC000`, the data IDs are not reported, and the clipboard that is given out is cut to 64 KiB. That is enough to paste text into far2l; the vertical block mark and HTML are then not preserved.

**Argument order.** Two implementations derived from the same reading of the header (f4 and thruPTY, as of the commits in section 9) send the terminal size event as `ESC _ f2l:` with a colon and with the width pushed after the height, answer `GET_WINDOW_MAXSIZE` with the width on top, and pop the two strings of the notification in the order text, title, while the title is on top of the stack, so the title and the text are swapped; far2l cannot decode the size event at all (8.2). The order of each request is in its table in section 5, and the byte examples of section 7 can be used to check it. thruPTY answers every image request with a single zero byte, which far2l takes, by an exception while popping, as "no capabilities".

**Handshake interference.** An application or a library that probes the terminal with other queries may swallow the `far2lok` acknowledgement. f4 handles it by announcing again when the acknowledgement was among the swallowed bytes, which is harmless (4.5).

**Order of replies.** f4 executes each request in its own goroutine, so in principle two replies can be sent in a different order than their requests arrived. far2l matches the replies by ID and does not depend on the order, but a client that does should not rely on it with such a terminal.

**The image protocol and chafa.** The issue [hpjansson/chafa#311](https://github.com/hpjansson/chafa/issues/311) asks for support for the far2l image protocol in chafa. Since November 2025 far2l also understands the Kitty graphics protocol, and chafa works in far2l through it; the image part of the far2l extensions has not changed since the beginning of December 2025 (the last commit that touched the image constants is of 4 December 2025); the documentation pull request elfmz/far2l#3082 is still open.

## 11. Related proposal: drag and drop

A proposal to receive **drag and drop** of files in a terminal over this same channel is open as pull request [elfmz/far2l#3647](https://github.com/elfmz/far2l/pull/3647) (the document `WinPort/DND.md`, related to issue #555). It adds four sub-commands (`BIND`, `LIST`, `READ`, `CLOSE`) and one event, `INPUT_DND`: the terminal announces a drop by an *offer*; the application enumerates it and reads the files only in the pieces that it asks for, so it works the same on a local machine and over SSH. Nothing in it is implemented in far2l yet; the letters `d` for the request and `D` for the event are proposed and not allocated, so no other use should be made of them. Both sides are implemented in f4, see section 9. It is not a part of the protocol described in this document until it is merged.
