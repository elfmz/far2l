# Unlocking the rest of FISH+ in far2l

The backport in this directory implements the part of FISH+ that today's
`IProtocol` can express: listing, metadata, ranged reads, ranged writes,
mutations, ownership and timestamps. That is already a complete file system
and it is where the safe, self-contained part of the work ends.

FISH+ can do considerably more, and the reason none of it is here is not that
it is hard on the wire - the helper already implements all of it - but that
`IProtocol` has nowhere to put the answer. Each section below is one such
capability: what the remote side already offers, what far2l would need, and how
big the change is. They are independent of each other and can be taken in any
order.

Throughout, "the helper" means `Helpers/helper.sh`, which is a verbatim copy
from f4 and must stay that way (see `Helpers/UPSTREAM.md`). Anything that needs
a *new* remote command belongs upstream in f4 first.

---

## 1. Server-side copy (the cheapest win)

**Remote side:** ready. `cp` + two path lines, recursive, one round trip.
`Client.Copy` in f4 uses it; `FishPlusSession` can issue it as
`ExecPaths("cp", {from, to})` today.

**far2l side:** `IProtocol` has no copy method at all, so `OpXfer` always
downloads and re-uploads even when both panels are the same host. What it
already has is the *move* case: `OpXfer` compares `IHost::Identity` of source
and destination and, when they match, calls `Rename` instead of transferring
(`_on_site_move`, `OpXfer.cpp`). On-site copy is the same shape.

Needed:

1. `IProtocol::FileCopy(const std::string &src, const std::string &dst)` with a
   default that throws `ProtocolUnsupportedError`, exactly like the existing
   `ExecuteCommand` declaration. No other protocol has to change.
2. IPC plumbing: a new `IPC_FILE_COPY` command, `HostRemote::FileCopy`,
   `HostRemoteBroker::OnFileCopy`, and the `IHost` declaration next to
   `Rename`.
3. `OpXfer`: an `_on_site_copy` flag set under the same identity check that
   sets `_on_site_move`, and a loop next to it that calls `FileCopy` and drops
   the item from the transfer list on success, falling back to the normal
   transfer when it throws.

This is implemented as a separate, clearly marked commit alongside this one, so
that it can be dropped without affecting the protocol itself. Everything below
is *not* implemented.

Related: the same hook would let SCP use `cp` and SFTP use its own copy
extension, so the interface is worth more than this one protocol.

---

## 2. Recursive delete in one round trip

**Remote side:** ready. `rmtree` + path deletes a whole tree remotely and
refuses `/` and anything with a `..` component.

**far2l side:** `OpRemove` enumerates the tree itself and issues one
`FileDelete`/`DirectoryDelete` per entry, because that is what gives it a
progress bar and an abort button. `ProtocolFISHPLUS::DirectoryDelete` therefore
maps to `rmdir`, not `rmtree`.

Needed: an optional `IProtocol::DirectoryDeleteRecursive` that `OpRemove` tries
first when the user did not ask to see per-file progress, falling back to the
walk. The trade is real - one round trip instead of thousands, against losing
progress and cancellation - so it probably wants to be a setting rather than a
default.

---

## 3. Remote search (server-side grep)

**Remote side:** ready. `grep <mode> <limit>` + pattern + path returns the byte
offsets of the matches and nothing else, so a pattern matching a million times
costs the same handful of bytes as one matching three times. Mode is `f` for a
fixed string or `e` for an extended regexp, with `i` appended to fold case.

**far2l side:** nothing to hook into. far2l's find-file reads files through the
protocol and scans them locally, which over a network means downloading
everything it looks at.

Needed: an optional interface along the lines of

```cpp
struct IProtocolSearch {
    // returns byte offsets, or refuses so the caller scans by reading
    virtual bool SearchFile(const std::string &path, const std::string &pattern,
                            bool case_insensitive, bool regex,
                            std::vector<unsigned long long> &offsets) = 0;
};
```

queried with `dynamic_cast` from the protocol, plus IPC plumbing for a
variable-length answer. The caller is far2l's find-file dialog and, separately,
the viewer's search.

Note the semantics: case folding and the regexp dialect become the remote
`grep`'s, not far2l's, so the same search can match slightly differently on a
remote panel than on a local one. That is the price of not moving the file, and
it should be documented wherever the feature is exposed.

---

## 4. The remote line index (open a 10 GB log instantly)

**Remote side:** ready. `lidx <first> <count>` + path runs one `awk` pass and
returns the byte offset of each requested line plus `T <total>`. `first` is
1-based. What crosses the network is a handful of numbers no matter how large
the file is.

**far2l side:** the viewer needs to know where lines are in order to jump to the
end, jump to line N, or say how many lines there are. Over any current NetRocks
protocol that means reading the file.

Needed: an optional `IProtocolLineIndex` with a
`Lines(path, first, count, offsets, total)` method, and a viewer that asks for
it before falling back to scanning. In f4 this is `vfs.LineIndexer`, and the
payoff there was that "go to end" stopped reading back a megabyte and started
reading exactly one screenful.

Caveat worth carrying over: cache the total against the file size it was
counted at, so paging around a log that is not growing costs one round trip
rather than one per keystroke.

---

## 5. Delta-based saving from the editor

**Remote side:** ready, and this is the most valuable one. `patch <nsegs> raw|b64`
+ two paths + one descriptor line per segment builds the destination out of
`S <off> <len>` ranges of an existing file, copied at local disk speed on the
remote host, and `D <len>` literals that follow their descriptor on the wire.

A one-byte change in a 100 MB file therefore costs one byte of traffic instead
of 100 MB.

**far2l side:** the editor saves by writing the whole file back. Making use of
`patch` needs the editor to know which ranges of the file it did *not* change,
which means an edit-history representation it does not currently keep. In f4
this fell out for free because its editor is built on a piece table: a piece
pointing at the original buffer *is* a range of the file on disk.

So this one is not a NetRocks change at all - it is an editor change, and a
large one. If far2l's editor ever grows a piece table, the interface it would
then want is

```cpp
struct IProtocolDeltaWriter {
    struct Segment { bool from_source; unsigned long long off, len; const void *data; };
    virtual void PatchFile(const std::string &src, const std::string &dst,
                           const std::vector<Segment> &segs) = 0;
};
```

Two properties to preserve: source and destination may not be the same file
(the result is written forward into a second path and renamed over the
original), and it applies only to a file loaded as raw bytes - with a codepage
conversion in the way, buffer offsets say nothing about bytes on disk.

---

## 6. Background jobs: tree sizes, duplicate search, remote commands

**Remote side:** ready. `jstart <kind> <npaths>` forks a detached subshell whose
streams point away from the wire and answers with a job id; `jpoll` brings back
whatever accumulated since the last poll; `jkill` and `jdrop` stop and forget
it; `jlist` enumerates. Kinds implemented upstream: `scan` (bytes, files and
directories of a whole tree, with progress), `hash` (for duplicate detection),
`exec` (run a command line where the files are).

**far2l side:**

- **Directory sizes.** far2l computes them by walking, which over a network is
  one round trip per directory. `scan` does it on the far side and reports
  progress every couple of thousand entries. This needs an optional
  `IProtocolFastScan` and a progress pump; NetRocks already has
  `ComplexOperationProgress` to display it and `BackgroundTasks` to own it.
- **Remote command execution.** `ProtocolFISHPLUS` deliberately does *not*
  implement `ExecuteCommand`, so a command typed on a FISH+ panel is refused
  rather than silently run locally. `IProtocol::ExecuteCommand` expects to
  attach a pseudo terminal through a FIFO, which the FISH+ request stream
  cannot carry: requests are strictly sequential and an interactive session
  cannot share them.

  Two ways forward. The cheap one is `jstart exec`, which gives line-buffered
  output and an exit status but no interactivity - enough for `make`, not
  enough for `top`; it would want a non-pty variant of the `ExecuteCommand`
  contract. The proper one is a second channel, which for the ssh transport
  means a second connection; f4 lists this as its own step for the same reason.

  Until then the honest thing is what this backport does: refuse, so the user
  reaches for a terminal instead of wondering where their command ran.

---

## 7. Creating symlinks - done upstream

This was a gap: protocol v1 read symlinks (`rdlink`) but could not create one.
Rather than patch `helper.sh` locally, which would have broken the drop-in
property this backport depends on, the command was added upstream in f4 as
`mklink` and the helper here re-synced.

`SymlinkCreate` therefore works, guarded by the `ln` feature in the banner: a
host with an older helper, or with no `ln`, is refused rather than sent a
request that would come back as "unknown command". Nothing further is needed
here; the entry is kept as the worked example of where a fix belongs when the
remote side is missing something.

---

## 8. Connection pooling

NetRocks already pools connections (`ConnectionsPool.cpp`), and FISH+ benefits
from it more than most protocols: a session costs an ssh handshake plus a helper
upload, so reopening a site that was just closed is noticeably slower than it
needs to be. Nothing special is required here - it is listed only because f4 has
the mirror-image task on its own roadmap, and the two should not drift into
different behaviour.

Related: the helper is uploaded on every connect. It compacts to well under
40 KB, which is nothing on a LAN and noticeable on a slow serial link. f4 has a
base64 single-line bootstrap for exactly that case (`Base64BootstrapLine`),
which this port does not implement because the two-step form is the portable
one. It is a small addition if serial users ask for it.

---

## 9. An alternative transport: libssh instead of ways.ini

This backport reaches the remote shell through `WayToShell`, the same machinery
the `shell` protocol uses, driven by `Helpers/ways.ini`. That was a deliberate
choice:

- no new build dependency, so `fish+` is available in every far2l build;
- every transport in `ways.ini` comes for free - ssh, sshpass, a serial line,
  anything that can be spelled as a command;
- it is the mechanism far2l users already know from the `shell` protocol.

The cost is that `WayToShell` allocates a pseudo terminal, which mangles binary
data by default. FISH+ survives this because the helper tames the line
discipline with POSIX `stty` and announces `tty` when it managed to; when it did
not, `ProtocolFISHPLUS::FilePut` switches the payload to base64 rather than
sending bytes a terminal would rewrite. That path is exercised by the tests, but
it costs a third more traffic.

The alternative is `SSHConnection` (libssh), which `ProtocolSFTP` and
`ProtocolSCP` already use. It gives a genuinely binary-clean channel with no pty
and no `stty` dance, which is what f4's own implementation uses. To add it:

1. Give `ProtocolFISHPLUS` a second construction path that builds an
   `SSHConnection`, opens a channel with `ssh_channel_request_exec("exec /bin/sh")`
   and no pty request, and wraps the channel in a small class exposing the three
   methods `FishPlus::Session` actually needs - `Send`, `ReadStdout`, `WaitReply`.
   The session itself is transport-agnostic and needs no changes.
2. Pass `false` for `tty_transport` to `Session::Handshake`, which drops the
   base64 fallback and lets raw `dd` writes be used unconditionally.
3. Guard the whole thing with `HAVE_SFTP` (which is what libssh availability is
   spelled as in `NetRocks/CMakeLists.txt`) and expose it as a way named
   `libssh` in the options dialog, defaulting to it when present.

Doing both is reasonable: `ways.ini` for reach, libssh for throughput.

---

## Reference

- Protocol specification and rationale: f4's `FISH+.md`
- Wire format details, per-command semantics and the list of known limitations
  of version 1: same document, sections "Wire Protocol, Version 1" and
  "Known limitations of v1"
- The Go client this code is a port of: `plugins/netfox/fishplus/` in f4
