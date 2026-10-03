# Terminal drag and drop over far2l extensions (proposal, v1)

Status: proposal. Nothing in this document is implemented in far2l yet; the
letters below are proposed values that are not allocated upstream. A complete
implementation of both sides exists in [f4](https://github.com/unxed/f4)
(`internal/terminal/far2ldnd` is the codec with the test vectors of section
10; the terminal side is `internal/terminal/far2l_dnd.go`, the application
side `internal/terminal/far2l_dnd_client.go`).

## 1. Idea

When the user drops files on a terminal window, the terminal cannot send them
as text: there is no such protocol, and the application may be on another
machine behind SSH. This proposal adds four request/reply sub-commands and one
event to the far2l interaction channel:

* the application switches drop reception on with **BIND**;
* the terminal announces a drop with one small **INPUT_DND** event, naming an
  *offer*;
* the application pulls the list with **LIST** and byte ranges with **READ**,
  one bounded chunk per ordinary reply;
* **CLOSE** releases the offer, or cancels it.

The files themselves never travel unasked, only the ranges the application
requests, so the same protocol serves a local file manager and an SSH session
without a second connection. The application decides where the files go (a
panel, an editor, the command line) and does the copy with its own file
operations, dialogs and progress.

## 2. Terms

* **binding**: the application's current subscription for drops, a random
  16-byte value chosen when it enables reception; a route generation, not proof
  of identity.
* **offer**: the set of objects of one user drop, identified by a
  cryptographically random 16-byte value the terminal creates and ties to the
  binding and the connection. If no secure generator is available the offer
  fails; there is no fixed fallback.
* **item**: one object in an offer, identified by a non-zero `uint64 item_id`
  the terminal issues and never reuses inside the offer. The ID is the key; the
  name is metadata only.
* **RID**: the existing eight-bit far2l interaction id, shared with clipboard,
  images and the other requests. DnD adds no second RPC counter.

The application never sends a path in READ. The terminal reads only objects it
listed in the offer; an offer gives no access to parents, siblings or other
offers.

## 3. Transport and layout

Proposed values (to be agreed before implementation; the symbolic names are
normative, the letters are not):

```text
FARTTY_INTERACT_DND = 'd'      # command family
FARTTY_INPUT_DND    = 'D'      # event
BIND='b', LIST='l', READ='r', CLOSE='c'
```

All field lists below are given **in the order they are popped from the
stack**; code must push them in reverse. Every POD has an explicit width
(`uint8_t('d')`, never a platform `int`). Integers are little endian.

```text
request: RID:u8, 'd':u8, subcommand:u8, arguments...
reply:   RID:u8, status:i8, result...
event:   'D':u8, binding:bytes[16], offer:bytes[16],
         x:i16, y:i16, modifiers:u32, event_flags:u16
```

`status=1` is success. An error carries a single `message:str` (diagnostic, at
most 1024 bytes) and no success body; the machine meaning is the status:

| status | meaning |
| --- | --- |
| 0 | I/O or internal error |
| -1 | access denied |
| -2 | offer revoked, expired or unknown |
| -3 | unknown item / parent / cursor |
| -4 | malformed request or range |
| -5 | representation or version not supported |
| -6 | source changed |
| -7 | negotiated limit exceeded / busy |
| -8 | request cancelled by closing the offer |

`str` is `uint32 length` then `length` bytes when popping, no NUL; `blob` has
the same layout without requiring UTF-8; `bytes[16]` has no length prefix. A
length is checked **before allocating** and may not exceed the rest of the
frame.

On the wire the whole stack is base64-encoded **once**; file content inside it
is plain bytes, not a second base64. No raw binary between APCs, 8-bit C1
codes, JSON or shell commands are used.

A request with a reply has a non-zero RID. Only CLOSE and switching a binding
off may use RID 0, as best-effort cleanup on exit.

## 4. BIND

Ordinary far2l extension negotiation happens first. The receiver of the
protocol must be installed before the request.

```text
in:  version:u16 (1), enable:u8, binding:bytes[16],
     max_frame:u32, max_chunk:u32, window:u16, wanted_features:u32
out (enable, status=1):
     version:u16, binding:bytes[16] (exact echo), max_frame:u32, max_chunk:u32,
     window:u16, idle_seconds:u32, features:u32
out (disable, status=1): empty
```

When disabling, the version stays 1 and the other numeric arguments except
`enable` are zero. Only the matching binding is disabled; a late disable of an
old generation does not touch the new one. Repeating is safe.

Feature bits: `STREAM=1`, `REFERENCE=2`; at least one must be negotiated.
`STREAM` means sequential reading is supported; random access is declared per
item. Unsupported bits are dropped from the result; an unsupported version is
`-5`, never a silent format switch.

The server chooses limits no larger than both requested and its own. The client
checks the whole reply. An empty reply (after the RID) from an old server means
there is no DnD; a plain `far2lok` does not mean DnD. Repeating BIND with the
same binding and parameters is idempotent, different parameters for the same
binding are rejected, a new binding revokes the old offers, and the terminal
queues the success reply **before** the first event of that binding.

## 5. INPUT_DND: notification only

Sent after a drop happened; it contains neither a file list nor file data.

`x`, `y` are zero-based cell coordinates in the terminal's area; unknown is
`(-1,-1)`. `modifiers` uses far2l's `dwControlKeyState`; bit 0 of `event_flags`
says the modifiers are known (otherwise the field is zero, which is not proof
that nothing was held). Other bits are zero in v1.

In v1 the action is always `copy`; Shift must not turn it into `move`. The
application picks the destination from the coordinates (a panel, an editor...);
with an unknown position it uses an explicitly chosen target or asks the user,
and does not substitute `(0,0)`. The terminal does not publish an offer until
it can serve its sources independently of the GUI callback's lifetime.

## 6. LIST

```text
in:  offer:bytes[16], parent_id:u64 (0 = root; only 0 in base v1), cursor:u64 (0 = first)
out: next_cursor:u64 (UINT64_MAX = last page), count:u32, entries:blob[count]
```

A page is bounded by the negotiated `max_frame` and by 64 entries; one entry by
16 KiB. The server never truncates a name, URI or entry to fit: an object that
does not fit gets an explicit error. A non-last page has at least one entry and
the cursor advances. The listing and the item_id mapping are fixed for the
life of the offer, repeating LIST with the same arguments returns the same page,
and the first successful LIST means the application has taken the offer.

Each entry is a self-contained stack:

```text
item_id:u64, kind:u8 (1 = regular file), item_flags:u16, size:u64 (valid with SIZE_KNOWN),
name:str (UTF-8 display name, never a destination path),
native_encoding:u8 (0 none, 1 POSIX bytes, 2 UTF-16LE), native_name:blob,
reference_uri:str (empty without REFERENCE), source_namespace:str (opaque, may be empty)
```

`item_flags`: `REFERENCE=1`, `STREAM=2`, `RANDOM_ACCESS=4`, `FROZEN=8`,
`SIZE_KNOWN=16`. Representations are limited to the negotiated features;
`RANDOM_ACCESS` and `FROZEN` mean something only with `STREAM`. The entry is
length-delimited so a later version can add fields; unknown flags and trailing
bytes grant nothing. Equal display names from different directories are fine:
items differ by item_id.

## 7. READ

```text
in:  offer:bytes[16], item_id:u64, offset:u64, length:u32
out: observed_size:u64, read_flags:u8 (EOF=1, SIZE_KNOWN=2), length:u32, data:bytes[length]
```

There is no second request id and no per-block acknowledgement: the RID ties
the reply to its range, and the offset is not repeated. `1 <= length <=
max_chunk`; **unlike FISH+, zero does not mean "read everything"** but `-4`.
Overflow of `offset+length` is checked. EOF is explicit; an empty success only
with EOF; a short reply without EOF is valid and advances by the bytes actually
read. Reading past the end of a regular file returns zero bytes with EOF.

For `RANDOM_ACCESS` any and overlapping ranges are allowed. A sequential item
allows one unfinished READ, starting at offset 0, each next one after the bytes
actually received; the global window does not lift this.

The server reads at most one chunk into a bounded buffer and then builds the
**whole** APC reply; on failure it sends an error, not half a success frame.

## 8. CLOSE

```text
in:  offer:bytes[16], reason:u8 (0 rejected, 1 processed, 2 cancelled, 3 failed)
out: empty
```

CLOSE forbids new reads and cancels unfinished work. Every already accepted
READ with a non-zero RID still gets exactly one final reply, a success if it is
already built or `-8`; the reply to a CLOSE with a non-zero RID is queued after
them and is the cleanup barrier for the offer. Repeating CLOSE, also after the
offer expired, succeeds. The reason is UI and resource accounting, not a
command to delete the sources.

## 9. Limits, cancellation, nesting

Proposed initial profile: a whole APC (introducer, base64 and terminator) of
at most 65 536 bytes; READ data at most 32 768 bytes; unfinished READs
negotiated between 1 and 32 (a simple first implementation may grant 1);
LIST page at most 64 entries; at most about 8 live offers; idle offer 600 s
(the value BIND returns). `max_frame` at least 4096, `max_chunk` at least 1;
BIND and its reply at most 512 bytes before negotiation. These are **new** DnD
limits, not a statement about existing far2l limits. They are checked while
the frame accumulates, before base64 decode and allocation; an over-limit
recognised DnD frame is dropped whole up to its terminator and the rest never
becomes key presses. Small key events and control replies are served between
whole frames, never inside one. The window is per binding, not per file, and
window 32 must not be advertised by an implementation that makes a synchronous
RPC under one shared lock.

READ and LIST extend the idle lease; a long-lived application may refresh it by
repeating a known LIST page. An active request has a separate finite timeout so
a hung provider cannot hold resources forever.

Cancellation: **cancelling the caller does not free the RID**. The entry stays
as an expected, unwanted reply until the whole frame is received and parsed;
only then may the id be reused (this rule is general for interactions, else a
late DnD reply can land in a clipboard request). Order: the UI stops waiting
and stops issuing READs; CLOSE is sent (RID 0 may be used if RIDs are
exhausted); arriving complete replies are matched to their old RIDs and
dropped; then RIDs, buffers and the VFS object are released. A cut APC is never
handed to the application partially; a table reset that reuses all RIDs on the
same stream is unsafe.

Nesting (SSH, a terminal inside a terminal): each hop is an application for
the outer offer and a source for its child. A semantic proxy gives the child a
new offer bound to the parent's, converts coordinates, maps item ids and RIDs
per hop, serves the child's READ by READs of the parent with backpressure
(no intermediate cache), and releases the parent's offer when the last
consumer ends. A drop goes to the child binding once. Multiplexers (tmux,
screen) need separate testing and are not claimed by the handshake.

## 10. Verified byte examples

Produced with the unmodified `StackSerializer.cpp` and `base64.cpp` of far2l;
they check the serialisation of this format, not that upstream has DnD.

READ request, fields in pop order: RID 42, command `'d'`, subcommand `'r'`,
offer `00 01 .. 0f`, item_id 7, offset 4096, length 32768. Stack on the wire:

```text
00 80 00 00
00 10 00 00 00 00 00 00
07 00 00 00 00 00 00 00
00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
72 64 2a
```

Full APC: `\x1b_far2l:AIAAAAAQAAAAAAAABwAAAAAAAAAAAQIDBAUGBwgJCgsMDQ4PcmQq\x07`

Success reply to another READ with RID 42 (offset 0, length 6, EOF, size not
known): status=1, observed_size=0, read_flags=1, length=6, data
`00 0a 0d 1b 07 ff`. Full APC: `\x1b_far2lAAoNGwf/BgAAAAEAAAAAAAAAAAEq\x07`;
LF, CR, ESC, BEL and 0xff stay data and do not end the APC.

String `a LF b` (three bytes): hex `61 0a 62 03 00 00 00`, base64
`YQpiAwAAAA==`. The length sits **after** the bytes in the physical buffer and
**before** them when popping; getting that order wrong breaks compatibility
even with correct little-endian.

## 11. Open points for upstream

1. The letters `'d'`/`'D'`, the sub-commands and where the backend-neutral API
   lives in far2l.
2. Initial frame/chunk/window and the rules of the shared interaction queue.
3. The representation of native names and reference URIs between POSIX and
   Windows.
4. UX for partial or failed copies after an early accepted desktop drop.
5. Recovery after a lost reply and behaviour on an emergency return to the
   shell.

Wire v1 needs no new OSC/APC number, no new SSH subsystem and no extension of
FISH+. Directories, hover, outbound drag, `move` and resume are extensions the
structure leaves room for (`parent_id` is already in LIST).
