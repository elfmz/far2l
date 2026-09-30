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

